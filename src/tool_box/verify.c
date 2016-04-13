/**************************************************************************
 *
 * GUARDTIME CONFIDENTIAL
 *
 * Copyright (C) [2015] Guardtime, Inc
 * All Rights Reserved
 *
 * NOTICE:  All information contained herein is, and remains, the
 * property of Guardtime Inc and its suppliers, if any.
 * The intellectual and technical concepts contained herein are
 * proprietary to Guardtime Inc and its suppliers and may be
 * covered by U.S. and Foreign Patents and patents in process,
 * and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this
 * material is strictly forbidden unless prior written permission
 * is obtained from Guardtime Inc.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ksi/ksi.h>
#include <ksi/compatibility.h>
#include <ksi/policy.h>
#include "param_set/param_set.h"
#include "param_set/task_def.h"
#include "tool_box/ksi_init.h"
#include "tool_box/tool_box.h"
#include "tool_box/param_control.h"
#include "tool_box/task_initializer.h"
#include "tool_box/smart_file.h"
#include "tool_box/err_trckr.h"
#include "api_wrapper.h"
#include "printer.h"
#include "debug_print.h"
#include "obj_printer.h"
#include "conf.h"
#include "tool.h"

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);

static int signature_verify(int id, PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_general(PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_internally(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_key_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_publication_based_with_user_pub(PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_publication_based_with_pubfile(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi,  KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int signature_verify_calendar_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh, KSI_PolicyVerificationResult **out);
static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err);

int verify_run(int argc, char **argv, char **envp) {
	int res;
	char buf[2048];
	PARAM_SET *set = NULL;
	TASK_SET *task_set = NULL;
	TASK *task = NULL;
	KSI_CTX *ksi = NULL;
	ERR_TRCKR *err = NULL;
	SMART_FILE *logfile = NULL;
	int d;
	COMPOSITE extra;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_PolicyVerificationResult *result = NULL;
	KSI_HashAlgorithm alg = KSI_HASHALG_INVALID;

	/**
	 * Extract command line parameters and also add configuration specific parameters.
	 */
	res = PARAM_SET_new(
			CONF_generate_param_set_desc("{i}{x}{f}{d}{pub-str}{ver-int}{ver-cal}{ver-key}{ver-pub}{dump}{conf}{log}", "XP", buf, sizeof(buf)),
			&set);
	if (res != KT_OK) goto cleanup;

	res = TASK_SET_new(&task_set);
	if (res != PST_OK) goto cleanup;

	res = generate_tasks_set(set, task_set);
	if (res != PST_OK) goto cleanup;

	res = TASK_INITIALIZER_getServiceInfo(set, argc, argv, envp);
	if (res != PST_OK) goto cleanup;

	res = TASK_INITIALIZER_check_analyze_report(set, task_set, 0.2, 0.1, &task);
	if (res != KT_OK) goto cleanup;

	res = TOOL_init_ksi(set, &ksi, &err, &logfile);
	if (res != KT_OK) goto cleanup;

	d = PARAM_SET_isSetByName(set, "d");

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
	 * Get document hash if provided by user
	 */
	if (PARAM_SET_isSetByName(set, "f")) {
		res = KSI_Signature_getHashAlgorithm(sig, &alg);
		if (res != KSI_OK) goto cleanup;
		extra.h_alg = &alg;

		print_progressDesc(d, "Reading documents hash... ");
		/* TODO: fix hash extractor from file. */
		res = PARAM_SET_getObjExtended(set, "f", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &extra, (void**)&hsh);
		if (res != PST_OK) goto cleanup;
		print_progressResult(res);
	}

	/**
	 * Verify the signature accordingly to the selected method.
	 */
	res = signature_verify(TASK_getID(task), set, err, &extra, ksi, sig, hsh, &result);
	if (res != KT_OK) goto cleanup;

	/**
	 * Dump the gathered data
	 */
	if (PARAM_SET_isSetByName(set, "dump")) {
		print_result("\n");
		OBJPRINT_signatureDump(sig, print_result);
		print_result("\n");
		OBJPRINT_signatureVerificationResultDump(result, print_result);

		if (PARAM_SET_isSetByName(set, "f")) {
			print_result("\n");
			OBJPRINT_Hash(hsh, "Document hash: ", print_result);
		}
	}

cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		KSI_LOG_debug(ksi, "\n%s", KSITOOL_KSI_ERRTrace_get());
		print_debug("\n");
		DEBUG_verifySignature(ksi, res, sig, result, hsh);

		print_errors("\n");
		if (d) ERR_TRCKR_printExtendedErrors(err);
		else  ERR_TRCKR_printErrors(err);
	}

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
	size_t count = 0;

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		"%s verify -i <in.ksig> [-f <data>] [more options]\n"
		"%s verify --ver-int -i <in.ksig> [-f <data>] [more options]\n"
		"%s verify --ver-cal -i <in.ksig> [-f <data>] -X <url>\n"
		"        [--ext-user <user> --ext-key <pass>] [more options]\n"
		"%s verify --ver-key -i <in.ksig> [-f <data>] -P <url>\n"
		"        [--cnstr <oid=value>]... [more options]\n"
		"%s verify --ver-pub -i <in.ksig> [-f <data>] --pub-str <pubstring>\n"
		"        [-x -X <url>  [--ext-user <user> --ext-key <pass>]] [more options]\n"
		"%s verify --ver-pub -i <in.ksig> [-f <data>] -P <url> [--cnstr <oid=value>]...\n"
		"        [-x -X <url>  [--ext-user <user> --ext-key <pass>]] [more options]\n\n"

		" --ver-int - verify just internally.\n"
		" --ver-cal - use calendar based verification (use extender).\n"
		" --ver-key - use key based verification.\n"
		" --ver-pub - use publication based verification (offline if used with --pub-str or -P).\n"
		" -i <file> - signature file to be verified.\n"
		" -f <data> - file or data hash to be verified. Hash format: <alg>:<hash in hex>.\n"
		"             as file on local machine).\n"
		" -X <url>  - specify extending service URL.\n"
		" --ext-user <str>\n"
		"           - user name for extending service.\n"
		" --ext-key <str>\n"
		"           - HMAC key for extending service.\n"
		" -P <url>  - specify publications file URL (or file with uri scheme 'file://').\n"
		" --cnstr <oid=value>\n"
		"           - publications file certificate verification constraints.\n"
		" --pub-str <str>\n"
		"           - publication string.\n"
		" -x        - allow to use extender when using publication based verification.\n"
		" -d        - print detailed information about processes and errors to stderr.\n"
		" --dump    - dump signature, document hash being verified and verification result to stdout.\n"
		" --conf <file>\n"
		"           - specify a configurations file to override default service\n"
		"             information. It must be noted that service info from\n"
		"             command-line will override the configurations file.\n"
		" --log <file>\n"
		"           - Write libksi log into file.",
		TOOL_getName(),
		TOOL_getName(),
		TOOL_getName(),
		TOOL_getName(),
		TOOL_getName(),
		TOOL_getName()
	);

	return buf;
}

const char *verify_get_desc(void) {
	return "KSI signature verification tool.";
}

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set) {
	int res;

	if (set == NULL || task_set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Configure parameter set, control, repair and object extractor function.
	 */
	res = CONF_initialize_set_functions(set, "XP");
	if (res != KT_OK) goto cleanup;

	PARAM_SET_addControl(set, "{conf}", isFormatOk_inputFile, isContentOk_inputFileRestrictPipe, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{log}", isFormatOk_path, NULL, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, extract_inputSignature);
	PARAM_SET_addControl(set, "{f}", isFormatOk_inputHash, isContentOk_inputHash, convertRepair_path, extract_inputHash);
	PARAM_SET_addControl(set, "{d}{x}{ver-int}{ver-cal}{ver-key}{ver-pub}{dump}", isFormatOk_flag, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{pub-str}", isFormatOk_pubString, NULL, NULL, extract_pubString);

	/*					  ID	DESC								MAN							ATL		FORBIDDEN							IGN	*/
	TASK_SET_add(task_set, 0,	"Verify.",							"i",						NULL,	"ver-int,ver-cal,ver-key,ver-pub",	NULL);
	TASK_SET_add(task_set, 1,	"Verify internally.",				"ver-int,i",				NULL,	"ver-cal,ver-key,ver-pub,T,x",		NULL);
	TASK_SET_add(task_set, 2,	"Calendar based verification.",		"ver-cal,i,X",				NULL,	"ver-int,ver-key,ver-pub",			NULL);
	TASK_SET_add(task_set, 3,	"Key based verification.",			"ver-key,i,P,cnstr",		NULL,	"ver-int,ver-cal,ver-pub,T,x",		NULL);

	TASK_SET_add(task_set, 4,	"Publication based verification, "
								"use publications file, "
								"extending is restricted.",			"ver-pub,i,P,cnstr",		NULL,	"ver-int,ver-cal,ver-key,x,T",		NULL);
	TASK_SET_add(task_set, 5,	"Publication based verification, "
								"use publications file, "
								"extending is permitted.",			"ver-pub,i,P,cnstr,x,X",	NULL,	"ver-int,ver-cal,ver-key,T",		NULL);

	TASK_SET_add(task_set, 6,	"Publication based verification, "
								"use publications string, "
								"extending is restricted.",			"ver-pub,i,pub-str",		NULL,	"ver-int,ver-cal,ver-key,x,T",		NULL);
	TASK_SET_add(task_set, 7,	"Publication based verification, "
								"use publications string, "
								"extending is permitted.",			"ver-pub,i,pub-str,x,X",	NULL,	"ver-int,ver-cal,ver-key,T",		NULL);

cleanup:

	return res;
}

static int signature_verify(int id, PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra,
							KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
							KSI_PolicyVerificationResult **out) {
	int res;
	int d = PARAM_SET_isSetByName(set, "d");

	if (set == NULL || err == NULL || ksi == NULL || sig == NULL || out == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	switch(id) {
		case 0:
			print_progressDesc(d, "Signature verification acctording to trust anchor... ");
			res = signature_verify_general(set, err, extra, ksi, sig, hsh, out);
			break;
		case 1:
			print_progressDesc(d, "Signature internal verification... ");
			res = signature_verify_internally(set, err, ksi, sig, hsh, out);
			break;
		case 2:
			print_progressDesc(d, "Signature calendar-based verification... ");
			res = signature_verify_calendar_based(set, err, ksi, sig, hsh, out);
			break;
		case 3:
			print_progressDesc(d, "Signature key-based verification... ");
			res = signature_verify_key_based(set, err, ksi, sig, hsh, out);
			break;
		case 4:
		case 5:
			print_progressDesc(d, "Signature publication-based verification with publications file... ");
			res = signature_verify_publication_based_with_pubfile(set, err, ksi, sig, hsh, out);
			break;
		case 6:
		case 7:
			print_progressDesc(d, "Signature publication-based verification with user publication string... ");
			res = signature_verify_publication_based_with_user_pub(set, err, extra, ksi, sig, hsh, out);
			break;
		default:
			ERR_CATCH_MSG(err, res = KT_UNKNOWN_ERROR, "Error: Unknown signature verification task.");
			break;
	}
	ERR_CATCH_MSG(err, res, "Error: Signature verification failed.");

cleanup:
	print_progressResult(res);

	return res;
}

static int signature_verify_general(PARAM_SET *set, ERR_TRCKR *err, COMPOSITE *extra,
									KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
									KSI_PolicyVerificationResult **out) {
	int res;
	int d = PARAM_SET_isSetByName(set, "d");
	int x = PARAM_SET_isSetByName(set, "x");
	KSI_PublicationData *pub_data = NULL;

	/**
	 * Get Publication data if available
	 */
	if (PARAM_SET_isSetByName(set, "pub-str")) {
		res = PARAM_SET_getObjExtended(set, "pub-str", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&pub_data);
		ERR_CATCH_MSG(err, res, "Error: Failed to get publication data.");
	}

	/**
	 * Verify signature
	 */
	print_progressDesc(d, "Verifying signature... ");
	res = KSITOOL_SignatureVerify_general(err, sig, ksi, hsh, pub_data, x, out);
	ERR_CATCH_MSG(err, res, "Error: Failed to verify signature.");

	res = KT_OK;

cleanup:

	print_progressResult(res);

	KSI_PublicationData_free(pub_data);

	return res;
}

static int signature_verify_internally(PARAM_SET *set, ERR_TRCKR *err,
									   KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHash *hsh,
									   KSI_PolicyVerificationResult **out) {
	int res;
	int d;

	d = PARAM_SET_isSetByName(set, "d");

	print_progressDesc(d, "Verifying signature... ");
	res = KSITOOL_SignatureVerify_internally(err, sig, ksi, hsh, out);
	ERR_CATCH_MSG(err, res, "Error: Failed to verify signature.");

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

	/**
	 * Verify signature
	 */
	print_progressDesc(d, "Verifying signature... ");
	res = KSITOOL_SignatureVerify_keyBased(err, sig, ksi, hsh, out);
	ERR_CATCH_MSG(err, res, "Error: Failed to verify signature.");

	res = KT_OK;

cleanup:

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

	/**
	 * Get Publication data
	 */
	res = PARAM_SET_getObjExtended(set, "pub-str", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&pub_data);
	ERR_CATCH_MSG(err, res, "Error: Failed to get publication data.");

	/**
	 * Verify signature
	 */
	print_progressDesc(d, "Verifying signature... ");
	res = KSITOOL_SignatureVerify_userProvidedPublicationBased(err, sig, ksi, hsh, pub_data, x, out);
	ERR_CATCH_MSG(err, res, "Error: Failed to verify signature.");

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

	/**
	 * Verify signature
	 */
	print_progressDesc(d, "Verifying signature... ");
	res = KSITOOL_SignatureVerify_publicationsFileBased(err, sig, ksi, hsh, x, out);
	ERR_CATCH_MSG(err, res, "Error: Failed to verify signature.");

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

	/**
	 * Verify signature
	 */
	print_progressDesc(d, "Verifying signature... ");
	res = KSITOOL_SignatureVerify_calendarBased(err, sig, ksi, hsh, out);
	ERR_CATCH_MSG(err, res, "Error: Failed to verify signature.");

	res = KT_OK;

cleanup:

	print_progressResult(res);

	KSI_Integer_free(pubTime);

	return res;
}

static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err) {
	int res;

	res = get_pipe_in_error(set, err, "i,f", NULL);
	if (res != KT_OK) goto cleanup;

cleanup:
	return res;
}
