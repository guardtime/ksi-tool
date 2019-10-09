/*
 * Copyright 2013-2016 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ksi/ksi.h>
#include <ksi/compatibility.h>
#include <ksi/policy.h>
#include <ksi/err.h>
#include "param_set/param_set.h"
#include "param_set/parameter.h"
#include "param_set/task_def.h"
#include "param_set/strn.h"
#include "tool_box/ksi_init.h"
#include "tool_box/param_control.h"
#include "tool_box/task_initializer.h"
#include "smart_file.h"
#include "err_trckr.h"
#include "api_wrapper.h"
#include "printer.h"
#include "debug_print.h"
#include "obj_printer.h"
#include "conf_file.h"
#include "tool.h"

enum {
	/* Trust anchor based verification. */
	ANC_BASED_DEFAULT,
	ANC_BASED_PUB_FILE,
	ANC_BASED_PUB_FILE_X,
	ANC_BASED_PUB_SRT,
	ANC_BASED_PUB_SRT_X,
	/* Internal verification. */
	INT_BASED,
	/* Calendar based verification. */
	CAL_BASED,
	KEY_BASED,
	/* Publication based verification, use publications file. */
	PUB_BASED_FILE,
	PUB_BASED_FILE_X,
	/* Publication based verification, use publication string. */
	PUB_BASED_STR,
	PUB_BASED_STR_X
};

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);

static int signature_verify(int id, PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_general(PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_internally(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_key_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_publication_based_with_user_pub(PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_publication_based_with_pubfile(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi,  KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_calendar_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err);
static void signature_print_suggestions_for_publication_based_verification(PARAM_SET *set, ERR_TRCKR *err, int errCode, KSI_CTX *ksi,
											KSI_Signature *sig, KSI_RuleVerificationResult *verRes, KSI_PublicationData *userPubData);

#define PARAMS "{i}{x}{f}{d}{pub-str}{ver-int}{ver-cal}{ver-key}{ver-pub}{dump}{conf}{log}{h|help}"

int verify_run(int argc, char **argv, char **envp) {
	int res;
	char buf[2048];
	PARAM_SET *set = NULL;
	TASK_SET *task_set = NULL;
	TASK *task = NULL;
	KSI_CTX *ksi = NULL;
	ERR_TRCKR *err = NULL;
	SMART_FILE *logfile = NULL;
	int d = 0;
	COMPOSITE extra;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_PolicyVerificationResult *result = NULL;
	KSI_HashAlgorithm alg = KSI_HASHALG_INVALID_VALUE;

	/**
	 * Extract command line parameters and also add configuration specific parameters.
	 */
	res = PARAM_SET_new(CONF_generate_param_set_desc(PARAMS, "XP", buf, sizeof(buf)), &set);
	if (res != KT_OK) goto cleanup;

	res = TASK_SET_new(&task_set);
	if (res != PST_OK) goto cleanup;

	res = generate_tasks_set(set, task_set);
	if (res != PST_OK) goto cleanup;

	res = TASK_INITIALIZER_getServiceInfo(set, argc, argv, envp);
	if (res != PST_OK) goto cleanup;

	res = TASK_INITIALIZER_check_analyze_report(set, task_set, 0.2, 0.1, &task);
	if (res != KT_OK) goto cleanup;

	d = PARAM_SET_isSetByName(set, "d");

	res = TOOL_init_ksi(set, &ksi, &err, &logfile);
	if (res != KT_OK) goto cleanup;

	res = check_pipe_errors(set, err);
	if (res != KT_OK) goto cleanup;


	extra.ctx = ksi;
	extra.err = err;
	extra.fname_out = NULL;

	print_progressDesc(d, "Reading signature... ");
	res = PARAM_SET_getObjExtended(set, "i", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &extra, (void**)&sig);
	if (res != PST_OK) goto cleanup;
	print_progressResult(res);

	/**
	 * Get document hash if provided by user.
	 */
	if (PARAM_SET_isSetByName(set, "f")) {
		res = KSI_Signature_getHashAlgorithm(sig, &alg);
		if (res != KSI_OK) goto cleanup;
		extra.h_alg = &alg;

		print_progressDesc(d, "Reading document's hash... ");
		/* TODO: fix hash extractor from file. */
		res = PARAM_SET_getObjExtended(set, "f", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &extra, (void**)&hsh);
		if (res != PST_OK) goto cleanup;
		print_progressResult(res);
	}

	/**
	 * Verify the signature accordingly to the selected method.
	 */
	res = signature_verify(TASK_getID(task), set, err, &extra, ksi, sig, hsh, &result);
	/* Fall through: if (res != KT_OK) goto cleanup; */

	if (PARAM_SET_isSetByName(set, "dump")) {
		int dump_flags = OBJPRINT_NONE;

		PARAM_SET_getObj(set, "dump", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, (void**)&dump_flags);

		/**
		 * Dump signature.
		 */
		print_result("\n");
		OBJPRINT_signatureDump(ksi, sig, dump_flags, print_result);
		/**
		 * Dump verification result data.
		 */
		print_result("\n");
		OBJPRINT_signatureVerificationResultDump(result, print_result);
		/**
		 * Dump document hash.
		 */
		if (PARAM_SET_isSetByName(set, "f")) {
			print_result("\n");
			OBJPRINT_Hash(hsh, "Document hash: ", print_result);
		}
	}

cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		if (ERR_TRCKR_getErrCount(err) == 0) {ERR_TRCKR_ADD(err, res, NULL);}
		KSITOOL_KSI_ERRTrace_LOG(ksi);
		print_debug("\n");
		DEBUG_verifySignature(ksi, res, sig, result, hsh);
	}
	ERR_TRCKR_print(err, d);

	SMART_FILE_close(logfile);
	PARAM_SET_free(set);
	TASK_SET_free(task_set);
	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
	KSI_PolicyVerificationResult_free(result);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return KSITOOL_errToExitCode(res);
}

char *verify_help_toString(char *buf, size_t len) {
	int res;
	char *ret = NULL;
	PARAM_SET *set;
	size_t count = 0;
	char tmp[1024];

	res = PARAM_SET_new(CONF_generate_param_set_desc(PARAMS, "XP", tmp, sizeof(tmp)), &set);
	if (res != PST_OK) goto cleanup;

	res = CONF_initialize_set_functions(set, "XP");
	if (res != PST_OK) goto cleanup;


	PARAM_SET_setHelpText(set, "ver-int", NULL, "Perform internal verification.");
	PARAM_SET_setHelpText(set, "ver-cal", NULL, "Perform calendar-based verification (use extending service).");
	PARAM_SET_setHelpText(set, "ver-key", NULL, "Perform key-based verification.");
	PARAM_SET_setHelpText(set, "ver-pub", NULL, "Perform publication-based verification (use with -x to permit extending).");
	PARAM_SET_setHelpText(set, "i", "<in.ksig>", "Signature file to be verified. Use '-' as file name to read the signature from stdin. Flag -i can be omitted when specifying the input. Without -i it is not possible to sign files that look like command-line parameters (e.g. -a, --option).");
	PARAM_SET_setHelpText(set, "f", "<data>", "Path to file to be hashed or data hash imprint to extract the hash value that is going to be verified. Hash format: <alg>:<hash in hex>. Use '-' as file name to read data to be hashed from stdin.");
	PARAM_SET_setHelpText(set, "x", NULL, "Permit to use extender for publication-based verification.");
	PARAM_SET_setHelpText(set, "pub-str", "<str>", "Publication string to verify with.");
	PARAM_SET_setHelpText(set, "dump", "[G]", "Dump signature and document hash being verified in human-readable format to stdout. In verification report 'OK' means that the step is performed successfully, 'NA' means that it could not be performed as there was not enough information and 'FAILED' means that the verification was unsuccessful. To make signature dump suitable for processing with grep, use 'G' as argument.");
	PARAM_SET_setHelpText(set, "d", NULL, "Print detailed information about processes and errors to stderr.");
	PARAM_SET_setHelpText(set, "conf", "<file>", "Read configuration options from given file. It must be noted that configuration options given explicitly on command line will override the ones in the configuration file.");
	PARAM_SET_setHelpText(set, "log", "<file>", "Write libksi log to given file. Use '-' as file name to redirect log to stdout.");


	count += PST_snhiprintf(buf + count, len - count, 80, 0, 0, NULL, ' ', "Usage:\\>1\n"
			"ksi verify -i <in.ksig> [-f <data>] [more_options]\n"
			"ksi verify --ver-int -i <in.ksig> [-f <data>] [more_options]\\>1\n\\>5"
			"ksi verify --ver-cal -i <in.ksig> [-f <data>] -X <URL>\n"
			"[--ext-user <user> --ext-key <key>] [more_options]\\>1\n\\>5"
			"ksi verify --ver-key -i <in.ksig> [-f <data>] -P <URL>\n"
			"[--cnstr <oid=value>]... [more_options]\\>1\n\\>5"
			"ksi verify --ver-pub -i <in.ksig> [-f <data>] --pub-str <pubstring>\n"
			"[-x -X <URL> [--ext-user <user> --ext-key <key>]] [more_options]\\>1\n\\>5"
			"ksi verify --ver-pub -i <in.ksig> [-f <data>] -P <URL> [--cnstr <oid=value>]...\n"
			"[-x -X <URL> [--ext-user <user> --ext-key <key>]] [more_options]\\>\n\n\n");

	ret = PARAM_SET_helpToString(set, "ver-int, ver-cal, ver-key, ver-pub,i,f,x,X,ext-user,ext-key,ext-hmac-alg,pub-str,P,cnstr,V,d,dump,conf,log", 1, 13, 80, buf + count, len - count);

cleanup:
	if (res != PST_OK || ret == NULL) {
		PST_snprintf(buf + count, len - count, "\nError: There were failures while generating help by PARAM_SET.\n");
	}
	PARAM_SET_free(set);
	return buf;
}

const char *verify_get_desc(void) {
	return "Verifies existing KSI signature.";
}

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set) {
	int res;

	if (set == NULL || task_set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Configure parameter set, check, repair and object extractor function.
	 */
	res = CONF_initialize_set_functions(set, "XP");
	if (res != KT_OK) goto cleanup;

	PARAM_SET_addControl(set, "{conf}", isFormatOk_inputFile, isContentOk_inputFileRestrictPipe, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{log}", isFormatOk_path, NULL, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputFile, isContentOk_inputFileWithPipe, convertRepair_path, extract_inputSignature);
	PARAM_SET_addControl(set, "{f}", isFormatOk_inputHash, isContentOk_inputHash, convertRepair_path, extract_inputHash);
	PARAM_SET_addControl(set, "{d}{x}{ver-int}{ver-cal}{ver-key}{ver-pub}", isFormatOk_flag, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{pub-str}", isFormatOk_pubString, NULL, NULL, extract_pubString);
	PARAM_SET_addControl(set, "{dump}", NULL, isContentOk_dump_flag, NULL, extract_dump_flag);

	PARAM_SET_setParseOptions(set, "i", PST_PRSCMD_HAS_VALUE | PST_PRSCMD_COLLECT_LOOSE_VALUES);
	PARAM_SET_setParseOptions(set, "{x}{ver-int}{ver-cal}{ver-key}{ver-pub}", PST_PRSCMD_HAS_NO_VALUE);

	/*						ID						DESC								MAN							ATL		FORBIDDEN											IGN	*/
	TASK_SET_add(task_set,	ANC_BASED_DEFAULT,		"Verify.",							"i",						NULL,	"ver-int,ver-cal,ver-key,ver-pub,P,cnstr,pub-str",	NULL);
	TASK_SET_add(task_set,	ANC_BASED_PUB_FILE,		"Verify, "
													"use publications file, "
													"extending is restricted.",			"i,P,cnstr",				NULL,	"ver-int,ver-cal,ver-key,ver-pub,x,T,pub-str",		NULL);
	TASK_SET_add(task_set,	ANC_BASED_PUB_FILE_X,	"Verify, "
													"use publications file, "
													"extending is permitted.",			"i,P,cnstr,x,X",			NULL,	"ver-int,ver-cal,ver-key,ver-pub,T,pub-str",		NULL);
	TASK_SET_add(task_set,	ANC_BASED_PUB_SRT,		"Verify, "
													"use publications string, "
													"extending is restricted.",			"i,pub-str",				NULL,	"ver-int,ver-cal,ver-key,ver-pub,x",				NULL);
	TASK_SET_add(task_set,	ANC_BASED_PUB_SRT_X,	"Verify, "
													"use publications string, "
													"extending is permitted.",			"i,pub-str,x,X",			NULL,	"ver-int,ver-cal,ver-key,ver-pub",					NULL);

	TASK_SET_add(task_set,	INT_BASED,				"Verify internally.",				"ver-int,i",				NULL,	"ver-cal,ver-key,ver-pub,T,x,pub-str",				NULL);

	TASK_SET_add(task_set,	CAL_BASED,				"Calendar based verification.",		"ver-cal,i,X",				NULL,	"ver-int,ver-key,ver-pub,pub-str",					NULL);

	TASK_SET_add(task_set,	KEY_BASED,				"Key based verification.",			"ver-key,i,P,cnstr",		NULL,	"ver-int,ver-cal,ver-pub,T,x,pub-str",				NULL);

	TASK_SET_add(task_set,	PUB_BASED_FILE,			"Publication based verification, "
													"use publications file, "
													"extending is restricted.",			"ver-pub,i,P,cnstr",		NULL,	"ver-int,ver-cal,ver-key,x,T,pub-str",				NULL);
	TASK_SET_add(task_set,	PUB_BASED_FILE_X,		"Publication based verification, "
													"use publications file, "
													"extending is permitted.",			"ver-pub,i,P,cnstr,x,X",	NULL,	"ver-int,ver-cal,ver-key,T,pub-str",				NULL);

	TASK_SET_add(task_set,	PUB_BASED_STR,			"Publication based verification, "
													"use publications string, "
													"extending is restricted.",			"ver-pub,i,pub-str",		NULL,	"ver-int,ver-cal,ver-key,x,T",						NULL);
	TASK_SET_add(task_set,	PUB_BASED_STR_X,		"Publication based verification, "
													"use publications string, "
													"extending is permitted.",			"ver-pub,i,pub-str,x,X",	NULL,	"ver-int,ver-cal,ver-key,T",						NULL);
cleanup:

	return res;
}

static int signature_verify(int id, PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra,
							KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
							KSI_PolicyVerificationResult **out) {
	int res;

	if (set == NULL || err == NULL || ksi == NULL || sig == NULL || out == NULL) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_ARGUMENT, NULL);
		goto cleanup;
	}

	switch(id) {
		case ANC_BASED_DEFAULT:
		case ANC_BASED_PUB_FILE:
		case ANC_BASED_PUB_FILE_X:
		case ANC_BASED_PUB_SRT:
		case ANC_BASED_PUB_SRT_X:
			res = signature_verify_general(set, err, extra, ksi, sig, hsh, out);
			goto cleanup;
		case INT_BASED:
			res = signature_verify_internally(set, err, ksi, sig, hsh, out);
			goto cleanup;
		case CAL_BASED:
			res = signature_verify_calendar_based(set, err, ksi, sig, hsh, out);
			goto cleanup;
		case KEY_BASED:
			res = signature_verify_key_based(set, err, ksi, sig, hsh, out);
			goto cleanup;
		case PUB_BASED_FILE:
		case PUB_BASED_FILE_X:
			res = signature_verify_publication_based_with_pubfile(set, err, ksi, sig, hsh, out);
			goto cleanup;
		case PUB_BASED_STR:
		case PUB_BASED_STR_X:
			res = signature_verify_publication_based_with_user_pub(set, err, extra, ksi, sig, hsh, out);
			goto cleanup;
		default:
			ERR_CATCH_MSG(err, (res = KT_UNKNOWN_ERROR), "Error: Unknown signature verification task.");
			goto cleanup;
	}

cleanup:
	print_progressResult(res);

	return res;
}


static void append_verification_result_error(KSI_RuleVerificationResult *verificationResult, ERR_TRCKR *err, int res, const char *task, int line) {
	int is_na = verificationResult->errorCode == KSI_VerificationErrorCode_fromString("GEN-02");
	const char *errorSuffix = is_na ? "inconclusive" : "failed";

	print_progressResult(res);

	ERR_TRCKR_add(err, res, __FILE__, line, "Error: [%s] %s. %s %s.",
			OBJPRINT_getVerificationErrorCode(verificationResult->errorCode),
			OBJPRINT_getVerificationErrorDescription(verificationResult->errorCode), task, errorSuffix);
}

static int signature_verify_general(PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra,
									KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
									KSI_PolicyVerificationResult **out) {
	int res;
	int d = PARAM_SET_isSetByName(set, "d");
	int x = PARAM_SET_isSetByName(set, "x");
	KSI_PublicationData *pub_data = NULL;
	KSI_PublicationsFile *pubFile = NULL;
	static const char *task = "Signature verification according to trust anchor";

	/**
	 * Get Publication data if available.
	 */
	if (PARAM_SET_isSetByName(set, "pub-str")) {
		res = PARAM_SET_getObjExtended(set, "pub-str", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&pub_data);
		ERR_CATCH_MSG(err, res, "Error: Failed to get publication data.");
	}

	/* If user insists to ignore the publications file and publications file URI is set, try to retrieve it.
	   If it fails ignore the incident and let the general verification handle the case. */
	if (PARAM_SET_isSetByName(set, "publications-file-no-verify,P")) {
		res = KSITOOL_receivePublicationsFile(err, ksi, &pubFile);
		if (res != KSI_OK) {
			KSI_ERR_clearErrors(ksi);
			res = KSI_OK;
		}
	}

	/**
	 * Verify signature.
	 */
	print_progressDesc(d, "%s... ", task);
	res = KSITOOL_SignatureVerify_general(err, sig, ksi, hsh, pubFile, pub_data, x, out);
	if (res != KSI_OK && *out != NULL) {
		KSI_RuleVerificationResult *verificationResult = NULL;

		if (KSI_RuleVerificationResultList_elementAt(
				(*out)->ruleResults, KSI_RuleVerificationResultList_length((*out)->ruleResults) - 1,
				&verificationResult) == KSI_OK && verificationResult != NULL) {
			append_verification_result_error(verificationResult, err, res, task, __LINE__);
		}
		goto cleanup;
	} else {
		ERR_CATCH_MSG(err, res, "Error: %s failed.", task);
	}

	res = KT_OK;

cleanup:

	print_progressResult(res);

	KSI_PublicationData_free(pub_data);
	KSI_PublicationsFile_free(pubFile);

	return res;
}

static int signature_verify_internally(PARAM_SET *set, ERR_TRCKR *err,
									   KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
									   KSI_PolicyVerificationResult **out) {
	int res;
	int d;
	static const char *task = "Signature internal verification";

	d = PARAM_SET_isSetByName(set, "d");

	print_progressDesc(d, "%s... ", task);
	res = KSITOOL_SignatureVerify_internally(err, sig, ksi, hsh, out);
	if (res != KSI_OK && *out != NULL) {
		KSI_RuleVerificationResult *verificationResult = NULL;

		if (KSI_RuleVerificationResultList_elementAt(
				(*out)->ruleResults, KSI_RuleVerificationResultList_length((*out)->ruleResults) - 1,
				&verificationResult) == KSI_OK && verificationResult != NULL) {
			append_verification_result_error(verificationResult, err, res, task, __LINE__);
		}
		goto cleanup;
	} else {
		ERR_CATCH_MSG(err, res, "Error: %s failed.", task);
	}

	res = KT_OK;

cleanup:

	print_progressResult(res);

	return res;
}


static int signature_verify_key_based(PARAM_SET *set, ERR_TRCKR *err,
									  KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
									  KSI_PolicyVerificationResult **out) {
	int res;
	int d = PARAM_SET_isSetByName(set, "d");
	static const char *task = "Signature key-based verification";
	KSI_PublicationsFile *pubFile = NULL;


	/**
	 * Verify signature.
	 */
	print_progressDesc(d, "%s... ", task);

	if (PARAM_SET_isSetByName(set, "publications-file-no-verify")) {
		res = KSITOOL_receivePublicationsFile(err, ksi, &pubFile);
		ERR_CATCH_MSG(err, res, "Error: Unable receive publications file.");
	}

	res = KSITOOL_SignatureVerify_keyBased(err, sig, ksi, hsh, pubFile, out);
	if (res != KSI_OK && *out != NULL) {
		KSI_RuleVerificationResult *verificationResult = NULL;

		if (KSI_RuleVerificationResultList_elementAt(
				(*out)->ruleResults, KSI_RuleVerificationResultList_length((*out)->ruleResults) - 1,
				&verificationResult) == KSI_OK && verificationResult != NULL) {
			append_verification_result_error(verificationResult, err, res, task, __LINE__);
		}
		goto cleanup;
	} else {
		ERR_CATCH_MSG(err, res, "Error: %s failed.", task);
	}

	res = KT_OK;

cleanup:

	KSI_PublicationsFile_free(pubFile);
	print_progressResult(res);

	return res;
}

static int signature_verify_publication_based_with_user_pub(PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra,
															KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
															KSI_PolicyVerificationResult **out) {
	int res;
	int d = PARAM_SET_isSetByName(set, "d");
	int x = PARAM_SET_isSetByName(set, "x");
	KSI_PublicationData *pub_data = NULL;
	static const char *task = "Signature publication-based verification with user publication string";

	/**
	 * Get Publication data.
	 */
	res = PARAM_SET_getObjExtended(set, "pub-str", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&pub_data);
	ERR_CATCH_MSG(err, res, "Error: Failed to get publication data.");

	/**
	 * Verify signature.
	 */
	print_progressDesc(d, "%s... ", task);
	res = KSITOOL_SignatureVerify_userProvidedPublicationBased(err, sig, ksi, hsh, pub_data, x, out);
	if (res != KSI_OK && *out != NULL) {
		if (res != KT_OK) {
			KSI_RuleVerificationResult *verificationResult = NULL;

			if (KSI_RuleVerificationResultList_elementAt(
					(*out)->ruleResults, KSI_RuleVerificationResultList_length((*out)->ruleResults) - 1,
					&verificationResult) == KSI_OK && verificationResult != NULL) {
				signature_print_suggestions_for_publication_based_verification(set, err, res, ksi, sig, verificationResult, pub_data);

				append_verification_result_error(verificationResult, err, res, task, __LINE__);
			}
			goto cleanup;
		}
	} else {
		ERR_CATCH_MSG(err, res, "Error: %s failed.", task);
	}

	res = KT_OK;

cleanup:

	print_progressResult(res);

	KSI_PublicationData_free(pub_data);

	return res;
}

static int signature_verify_publication_based_with_pubfile(PARAM_SET *set, ERR_TRCKR *err,
														   KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
														   KSI_PolicyVerificationResult **out) {
	int res;
	int d = PARAM_SET_isSetByName(set, "d");
	int x = PARAM_SET_isSetByName(set, "x");
	static const char *task = "Signature publication-based verification with publications file";
	KSI_PublicationsFile *pubFile = NULL;

	/**
	 * Verify signature.
	 */
	print_progressDesc(d, "%s... ", task);

	if (PARAM_SET_isSetByName(set, "publications-file-no-verify")) {
		res = KSITOOL_receivePublicationsFile(err, ksi, &pubFile);
		ERR_CATCH_MSG(err, res, "Error: Unable receive publications file.");
	}

	res = KSITOOL_SignatureVerify_publicationsFileBased(err, sig, ksi, hsh, pubFile, x, out);
	if (res != KSI_OK && *out != NULL) {
		if (res != KT_OK) {
			KSI_RuleVerificationResult *verificationResult = NULL;

			if (KSI_RuleVerificationResultList_elementAt(
					(*out)->ruleResults, KSI_RuleVerificationResultList_length((*out)->ruleResults) - 1,
					&verificationResult) == KSI_OK && verificationResult != NULL) {
				signature_print_suggestions_for_publication_based_verification(set, err, res, ksi, sig, verificationResult, NULL);

				append_verification_result_error(verificationResult, err, res, task, __LINE__);
			}
			goto cleanup;
		}
	} else {
		ERR_CATCH_MSG(err, res, "Error: %s failed.", task);
	}

	res = KT_OK;

cleanup:

	print_progressResult(res);

	return res;
}

static int signature_verify_calendar_based(PARAM_SET *set, ERR_TRCKR *err,
										   KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
										   KSI_PolicyVerificationResult **out) {
	int res;
	int d = PARAM_SET_isSetByName(set, "d");
	KSI_Integer *pubTime = NULL;
	static const char *task = "Signature calendar-based verification";

	/**
	 * Verify signature.
	 */
	print_progressDesc(d, "%s... ", task);
	res = KSITOOL_SignatureVerify_calendarBased(err, sig, ksi, hsh, out);
	if (res != KSI_OK && *out != NULL) {
		KSI_RuleVerificationResult *verificationResult = NULL;

		if (KSI_RuleVerificationResultList_elementAt(
				(*out)->ruleResults, KSI_RuleVerificationResultList_length((*out)->ruleResults) - 1,
				&verificationResult) == KSI_OK && verificationResult != NULL) {
				append_verification_result_error(verificationResult, err, res, task, __LINE__);
		}
		goto cleanup;
	} else {
		ERR_CATCH_MSG(err, res, "Error: %s failed.", task);
	}

	res = KT_OK;

cleanup:

	print_progressResult(res);

	KSI_Integer_free(pubTime);

	return res;
}

static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err) {
	int res;

	res = get_pipe_in_error(set, err, NULL, "i,f", NULL);
	if (res != KT_OK) goto cleanup;

cleanup:
	return res;
}

static void signature_print_suggestions_for_publication_based_verification(PARAM_SET *set, ERR_TRCKR *err, int errCode,
														   KSI_CTX *ksi, KSI_Signature *sig,
														   KSI_RuleVerificationResult *verRes, KSI_PublicationData *userPubData) {

	int res = KT_UNKNOWN_ERROR;
	KSI_PublicationRecord *rec = NULL;
	KSI_PublicationData *pubData = NULL;
	KSI_PublicationsFile *pubFile = NULL;
	KSI_Integer *sigTime = NULL;
	KSI_Integer *userPubTime = NULL;
	KSI_Integer *latestPubTimeInPubfile = NULL;
	KSI_PublicationRecord *possibilityToExtendTo = NULL;
	int x = 0;
	int isExtended = 0;
	int ispubfile = userPubData == NULL ? 1 : 0;

	if (verRes == NULL || verRes->errorCode != KSI_VER_ERR_GEN_2 || sig == NULL) return;


	x = PARAM_SET_isSetByName(set, "x");
	isExtended = KSI_OBJ_isSignatureExtended(sig);

	res = KSI_Signature_getSigningTime(sig, &sigTime);
	if (res != KSI_OK) return;

	/* Get publications file and check if it is possible to extend the signature to some available publication. */
	res = KSI_CTX_getPublicationsFile(ksi, &pubFile);
	if (res != KSI_OK) return;

	if (pubFile != NULL) {
		KSI_PublicationRecord *lastRec = NULL;
		KSI_PublicationData *lastRecData = NULL;

		res = KSI_PublicationsFile_getLatestPublication(pubFile, sigTime, &possibilityToExtendTo);
		if (res != KSI_OK) return;

		res = KSI_PublicationsFile_getLatestPublication(pubFile, NULL, &lastRec);
		if (res != KSI_OK) return;

		res = KSI_PublicationRecord_getPublishedData(lastRec, &lastRecData);
		if (res != KSI_OK) return;

		res = KSI_PublicationData_getTime(lastRecData, &latestPubTimeInPubfile);
		if (res != KSI_OK) return;
	}

	/* If there is user publication specified get its time. */
	if (!ispubfile && userPubData != NULL) {
		res = KSI_PublicationData_getTime(userPubData, &userPubTime);
	}

	if (!isExtended && ispubfile) {
		if (possibilityToExtendTo != NULL && !x) {
			ERR_TRCKR_addAdditionalInfo(err, "  * Suggestion:  Use -x to permit automatic extending or use KSI tool extend command to extend the signature.\n");
		} else if (possibilityToExtendTo == NULL) {
			ERR_TRCKR_addAdditionalInfo(err, "  * Suggestion:  Check if publications file is up-to-date as there is not (yet) a publication record in the publications file specified to extend the signature to.\n");
			ERR_TRCKR_addAdditionalInfo(err, "  * Suggestion:  Wait until next publication and try again.\n");
			if (!x) ERR_TRCKR_addAdditionalInfo(err, "  * Suggestion:  When a suitable publication is available use -x to permit automatic extending or use KSI tool extend command to extend the signature.\n");
		}

		ERR_TRCKR_ADD(err, errCode, "Error: Signature is not extended.");
	} else {


		if (ispubfile) {
			KSI_PublicationRecord *pubrecInPubfile = NULL;
			KSI_Integer *pubTime = NULL;
			int isPubfileOlderThanSig;

			/* Get the publication time. */
			res = KSI_Signature_getPublicationRecord(sig, &rec);
			if (res != KSI_OK) return;
			res = KSI_PublicationRecord_getPublishedData(rec, &pubData);
			if (res != KSI_OK) return;
			res = KSI_PublicationData_getTime(pubData, &pubTime);
			if (res != KSI_OK) return;

			isPubfileOlderThanSig = KSI_Integer_compare(latestPubTimeInPubfile, sigTime) == -1 ? 1 : 0;

			res = KSI_PublicationsFile_getPublicationDataByTime(pubFile, pubTime, &pubrecInPubfile);
			if (res != KSI_OK) return;

			if (pubrecInPubfile == NULL) {
				ERR_TRCKR_ADD(err, errCode, "Error: Signature is extended to a publication that does not exist in publications file.");

				if (possibilityToExtendTo == NULL && isPubfileOlderThanSig) {
					ERR_TRCKR_addAdditionalInfo(err, "  * Suggestion:  Check if publications file is up-to-date as the latest publication in the publications file is older than the signatures publication record.\n");
				} else if (possibilityToExtendTo != NULL && !x) {
					ERR_TRCKR_addAdditionalInfo(err, "  * Suggestion:  Try to use -x to permit automatic extending or use KSI tool extend command to re-extend the signature.\n");
				}
			}
		} else {
			if (KSI_Integer_compare(userPubTime, sigTime) == -1) {
				ERR_TRCKR_ADD(err, errCode, "Error: User publication string can not be older than the signatures signing time.");
				return;
			} else if (!x) {
				ERR_TRCKR_addAdditionalInfo(err, "  * Suggestion:  Use -x to permit automatic extending.\n");
			}
		}

	}
}
