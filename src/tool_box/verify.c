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
static int verify_as_possible(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig);
static int verify_internally(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig);
static int verify_calendar_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig);
static int verify_key_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig);
static int verify_publication_based_pubstr(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *comp, KSI_Signature **out);
static int signature_verify_with_user_publication(PARAM_SET *set, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig, KSI_Signature **out);
static int verify_data_hash(ERR_TRCKR *err, KSI_Signature *sig, KSI_DataHash *hsh);

int verify_run(int argc, char** argv, char **envp) {
	int res;
	char buf[2048];
	PARAM_SET *set = NULL;
	TASK_SET *task_set = NULL;
	TASK *task = NULL;
	KSI_CTX *ksi = NULL;
	ERR_TRCKR *err = NULL;
	SMART_FILE *logfile = NULL;
	int d;
	int dump;
	COMPOSITE extra;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_Signature *ver_result_sig = NULL;
	KSI_HashAlgorithm alg = KSI_HASHALG_INVALID;

	/**
	 * Extract command line parameters and also add configuration specific parameters.
     */
	res = PARAM_SET_new(
			CONF_generate_desc("{verify}{i}{o}{D}{x}{f}{d}{pub-str|p}{ver-int}{ver-cal}{ver-key}{ver-pub}{log}{dump}{conf}{log}", buf, sizeof(buf)),
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
	dump = PARAM_SET_isSetByName(set, "dump");

	extra.ctx = ksi;
	extra.err = err;
	extra.fname_out = NULL;

	print_progressDesc(d, "Reading signature... ");
	res = PARAM_SET_getObjExtended(set, "i", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &extra, (void**)&sig);
	if (res != PST_OK) goto cleanup;
	print_progressResult(res);

	/**
	 * Verify the signature accordingly to the selected method.
     */
	switch(TASK_getID(task)) {
		case 0:
			res = verify_as_possible(set, err, ksi, sig);
		break;
		case 1:
			/** TODO: implement*/
			res = verify_internally(set, err, ksi, sig);
		break;
		case 2:
			res = verify_calendar_based(set, err, ksi, sig);
		break;
		case 3:
			res = verify_key_based(set, err, ksi, sig);
		break;
		case 4:
		case 5:
			//res = verify_publication_based_pubfile(set, err, ksi, sig, &extra);
			;
		break;
		case 6:
		case 7:
			res = verify_publication_based_pubstr(set, err, ksi, sig, &extra, &ver_result_sig);
		break;
		default:
			res = KT_UNKNOWN_ERROR;
			goto cleanup;
		break;
	}

	/**
	 * Verify the signatures input hash and document hash.
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

		print_progressDesc(d, "Verifying documents hash... ");
		res = verify_data_hash(err, sig, hsh);
		if (res != PST_OK) goto cleanup;
		print_progressResult(res);
	}

	if (dump) {
		print_result("\n");
		OBJPRINT_signatureDump(ver_result_sig != NULL ? ver_result_sig : sig, print_result);

		if (hsh != NULL) {
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
		DEBUG_verifySignature(ksi, res, ver_result_sig != NULL ? ver_result_sig : sig, hsh);

		print_errors("\n");
		if (d) ERR_TRCKR_printExtendedErrors(err);
		else  ERR_TRCKR_printErrors(err);
	}

	SMART_FILE_close(logfile);
	PARAM_SET_free(set);
	TASK_SET_free(task_set);
	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
	KSI_Signature_free(ver_result_sig);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return KSITOOL_errToExitCode(res);
}

char *verify_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		"%s verify -i <in.ksig> [-f <data>] [more options]\n"
		"%s verify --ver-int -i <in.ksig> [-f <data>] [more options]\n"
		"%s verify --ver-cal -i <in.ksig> [-f <data>] -X <url>\n"
		"        [--ext-user <user> --ext-key <pass>] [-T <time>] [more options]\n"
		"%s verify --ver-key -i <in.ksig> [-f <data>] -P <url>\n"
		"        [--cnstr <oid=value>]... [more options]\n"
		"%s verify --ver-pub -i <in.ksig> [-f <data>] -p <pubstring>\n"
		"        [-x -X <url>  [--ext-user <user> --ext-key <pass>]] [more options]\n"
		"%s verify --ver-pub -i <in.ksig> [-f <data>] -P <url> [--cnstr <oid=value>]...\n"
		"        [-x -X <url>  [--ext-user <user> --ext-key <pass>][-T <time>]] [more options]\n\n"

		" --ver-int - verify just internally.\n"
		" --ver-cal - use calendar based verification (use extender).\n"
		" --ver-key - use key based verification.\n"
		" --ver-pub - use publication based verification (offline if used with --pub-str or -P\n"
		" -i <file> - signature file to be verified.\n"
		" -f <data> - file or data hash to be verified. Hash format: <alg>:<hash in hex>.\n"
		"             as file on local machine).\n"
		" -X <url>  - specify extending service URL.\n"
		" --ext-user <str>\n"
		"           - user name for extending service.\n"
		" --ext-key <str>\n"
		"           - HMAC key for extending service.\n"
		" -T <time> - specify a publication time to extend to as the number of seconds\n"
		"             since 1970-01-01 00:00:00 UTC or time string formatted as \"YYYY-MM-DD hh:mm:ss\".\n"
		" -P <url>  - specify publications file URL (or file with uri scheme 'file://').\n"
		" --cnstr <oid=value>\n"
		"           - publications file certificate verification constraints.\n"
		" -p | --pub-str <str>\n"
		"           - publication string.\n"
		" -x        - allow to use extender when using publication based verification.\n"
		" -d        - print detailed information about processes and errors to stderr.\n"
		" --dump    - dump signature and document hash being verified to stdout.\n"
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


static int verify_as_possible(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig) {
	int res;
	int d;

	if (set == NULL || ksi == NULL || err == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	d = PARAM_SET_isSetByName(set, "d");

	print_progressDesc(d, "Verifying signature... ");
	res = KSITOOL_Signature_verify(err, sig, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature.");
	print_progressResult(res);

	res = KT_OK;

cleanup:
	print_progressResult(res);

	return res;
}

static int verify_internally(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig) {
	int res;
	int d;

	if (set == NULL || err == NULL || ksi == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	d = PARAM_SET_isSetByName(set, "d");

	print_progressDesc(d, "Verifying signature internally... ");
	res = KT_OK;
	print_progressResult(res);
	print_errors("TODO: Internal verification not implemented.\n");

	res = KT_OK;

cleanup:
	print_progressResult(res);

	return res;
}

static int verify_calendar_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig) {
	int res;
	int d;

	if (set == NULL || err == NULL || ksi == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	d = PARAM_SET_isSetByName(set, "d");

	print_progressDesc(d, "Verifying online against extender... ");
	res = KSITOOL_Signature_verifyOnline(err, sig, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature online.");
	print_progressResult(res);

	res = KT_OK;

cleanup:
	print_progressResult(res);

	return res;
}

static int verify_key_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig) {
	int res;
	int d;
	KSI_CalendarAuthRec *cal_auth_rec = NULL;

	if (set == NULL || err == NULL || ksi == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	d = PARAM_SET_isSetByName(set, "d");

	res = KSI_Signature_getCalendarAuthRec(sig, &cal_auth_rec);
	ERR_CATCH_MSG(err, res, "Error: Unable to get calendar authentication record.");

	if (cal_auth_rec == NULL) {
		ERR_TRCKR_ADD(err, res = KT_KSI_SIG_VER_IMPOSSIBLE, "Error: Unable to perform key based verification as calendar authentication record is not present.");
		goto cleanup;
	}

	print_progressDesc(d, "Key based verification... ");
	res = KSITOOL_Signature_verify(err, sig, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature online.");
	print_progressResult(res);

	res = KT_OK;

cleanup:
	print_progressResult(res);

	return res;
}

static int verify_publication_based_pubstr(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *comp, KSI_Signature **out) {
	int res;

	if (set == NULL || err == NULL || ksi == NULL || sig == NULL || comp == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = signature_verify_with_user_publication(set, ksi, err, sig, out);


cleanup:

	return res;
}

static int verify_data_hash(ERR_TRCKR *err, KSI_Signature *sig, KSI_DataHash *hsh) {
	int res;
	KSI_DataHash *input_hash = NULL;

	if (err == NULL || sig == NULL || hsh == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = KSI_Signature_getDocumentHash(sig, &input_hash);
	ERR_CATCH_MSG(err, res, "Error: Unable to extract input hash from the signature.");

	if (!KSI_DataHash_equals(input_hash, hsh)) {
		ERR_TRCKR_ADD(err, res = KSI_VERIFICATION_FAILURE, "Error: Document hash and input hash mismatch.");
		goto cleanup;
	}


	res = KT_OK;

cleanup:

	return res;
}

/**
 * Some functionality to workaround API missing functionality.
 */

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set) {
	int res;

	if (set == NULL || task_set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Configure parameter set, control, repair and object extractor function.
     */
	res = CONF_initialize_set_functions(set);
	if (res != KT_OK) goto cleanup;

	PARAM_SET_addControl(set, "{conf}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{log}", isFormatOk_path, NULL, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, extract_inputSignature);
	PARAM_SET_addControl(set, "{f}", isFormatOk_inputHash, isContentOk_inputHash, convertRepair_path, extract_inputHash);
	PARAM_SET_addControl(set, "{T}", isFormatOk_utcTime, isContentOk_utcTime, NULL, extract_utcTime);
	PARAM_SET_addControl(set, "{d}{x}{ver-int}{ver-cal}{ver-key}{ver-pub}{dump}", isFormatOk_flag, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{pub-str}", isFormatOk_pubString, NULL, NULL, extract_pubString);

	/*					  ID	DESC										MAN							ATL		FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Verify.",									"i",						NULL,	"ver-int,ver-cal,ver-key,ver-pub",		NULL);
	TASK_SET_add(task_set, 1,	"Verify internally.",						"ver-int,i",				NULL,	"ver-cal,ver-key,ver-pub,T,x",		NULL);
	TASK_SET_add(task_set, 2,	"Calendar based verification.",				"ver-cal,i,X",				NULL,	"ver-int,ver-key,ver-pub",		NULL);
	TASK_SET_add(task_set, 3,	"Key based verification.",					"ver-key,i,P,cnstr",		NULL,	"ver-int,ver-cal,ver-pub,T,x",		NULL);

	TASK_SET_add(task_set, 4,	"Publication based verification, use publications file, extending is restricted.",
																			"ver-pub,i,P,cnstr",		NULL,	"ver-int,ver-cal,ver-key,x,p",		NULL);
	TASK_SET_add(task_set, 5,	"Publication based verification, use publications file, extending is permitted.",
																			"ver-pub,i,P,cnstr,x,X",	NULL,	"ver-int,ver-cal,ver-key,p",		NULL);
	TASK_SET_add(task_set, 6,	"Publication based verification, use publications string, extending is restricted.",
																			"ver-pub,i,p",				NULL,	"ver-int,ver-cal,ver-key,x,T",		NULL);
	TASK_SET_add(task_set, 7,	"Publication based verification, use publications string, extending is permitted.",
																			"ver-pub,i,p,x,X",			NULL,	"ver-int,ver-cal,ver-key,T",		NULL);

cleanup:

	return res;
}

static int publication_data_equals(KSI_PublicationData *A, KSI_PublicationData *B) {
	int res;
	KSI_Integer *A_time = NULL;
	KSI_Integer *B_time = NULL;
	KSI_DataHash *A_hash = NULL;
	KSI_DataHash *B_hash = NULL;

	if (A == NULL || B == NULL) return 0;

	res = KSI_PublicationData_getTime(A, &A_time);
	if (res != KSI_OK) return 0;

	res = KSI_PublicationData_getImprint(A, &A_hash);
	if (res != KSI_OK) return 0;

	res = KSI_PublicationData_getTime(B, &B_time);
	if (res != KSI_OK) return 0;

	res = KSI_PublicationData_getImprint(B, &B_hash);
	if (res != KSI_OK) return 0;

	if (KSI_Integer_equals(A_time, B_time) == 0) return 0;
	if (KSI_DataHash_equals(A_hash, B_hash) == 0) return 0;

	return 1;
}

static int signature_add_dummy_pub_rec(KSI_Signature *sig, KSI_CTX *ksi, const char *pub_str) {
	int res;
	KSI_PublicationRecord *dummy_pub_rec = NULL;
	KSI_PublicationData *user_pub_data = NULL;

	if (sig == NULL || ksi == NULL || pub_str == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = KSI_PublicationData_fromBase32(ksi, pub_str, &user_pub_data);
	if (res != KSI_OK) goto cleanup;

	res = KSI_PublicationRecord_new(ksi, &dummy_pub_rec);
	if (res != KSI_OK) goto cleanup;

	res = KSI_PublicationRecord_setPublishedData(dummy_pub_rec, user_pub_data);
	if (res != KSI_OK) goto cleanup;
	user_pub_data = NULL;

	res = KSI_Signature_replacePublicationRecord(sig, dummy_pub_rec);
	if (res != KSI_OK) goto cleanup;
	dummy_pub_rec = NULL;

	res = KSI_OK;

cleanup:

	KSI_PublicationRecord_free(dummy_pub_rec);
	KSI_PublicationData_free(user_pub_data);

	return res;
}

static int signature_without_trust_record_isPubDataExactMatch(const KSI_Signature *sig, KSI_CTX *ksi, const char *pubStr) {
	int res;
	KSI_PublicationData *user_pub_data = NULL;
	KSI_Signature *tmp = NULL;
	const KSI_VerificationResult *ver_result = NULL;

	if (sig == NULL || pubStr == NULL || ksi == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}
	res = KSI_Signature_clone(sig, &tmp);
	if (res != KSI_OK) goto cleanup;


	res = KSI_PublicationData_fromBase32(ksi, pubStr, &user_pub_data);
	if (res != KSI_OK) goto cleanup;

	res = signature_add_dummy_pub_rec(tmp, ksi, pubStr);
	if (res != KSI_OK) goto cleanup;

	res = KSI_Signature_verifyWithPublication(tmp, ksi, user_pub_data);
	if (res != KSI_OK || res != KSI_VERIFICATION_FAILURE) goto cleanup;

	res = KSI_Signature_getVerificationResult(tmp, &ver_result);
	if (res != KSI_OK) goto cleanup;

	if (!KSI_VerificationResult_isStepSuccess(ver_result, KSI_VERIFY_PUBLICATION_WITH_PUBSTRING)) {
		res = KSI_VERIFICATION_FAILURE;
		goto cleanup;
	}

	res = KSI_OK;

cleanup:

	KSI_Signature_free(tmp);
	KSI_PublicationData_free(user_pub_data);

	return res == KSI_OK ? 1 : 0;
}

static int signature_verify_with_user_publication(PARAM_SET *set, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig, KSI_Signature **out) {
	int res;
	char *refStrn = NULL;
	int d;
	int x;
	int isPubrec;
	int isCalAuthRec;
	int exact_match = 0;
	KSI_PublicationData *user_pub_data = NULL;
	KSI_Integer *user_pub_time = NULL;
	KSI_PublicationRecord *sig_pub_rec = NULL;
	KSI_PublicationData *sig_pub_data = NULL;
	KSI_Integer *sig_signing_time = NULL;
	KSI_Signature *verify_that = NULL;
	KSI_Signature *tmp_sig = NULL;

	if (set == NULL || ksi == NULL || err == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = PARAM_SET_getStr(set, "pub-str", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &refStrn);
	ERR_CATCH_MSG(err, res, "Error: Unable to get user publication string.");

	d = PARAM_SET_isSetByName(set, "d");
	x = PARAM_SET_isSetByName(set, "x");

	/**
	 * Extract user publication data from the user publication string.
     */
	res = KSI_PublicationData_fromBase32(ksi, refStrn, &user_pub_data);
	ERR_CATCH_MSG(err, res, "Error: Unable parse publication string.");

	res = KSI_PublicationData_getTime(user_pub_data, &user_pub_time);
	ERR_CATCH_MSG(err, res, "Error: Unable to get publication time from publication string.");


	/**
	 * Check if signature contains publication record. For a workaround check also
	 * the calendar authentication record.
     */
	isPubrec = KSITOOL_Signature_isPublicationRecordPresent(sig);
	isCalAuthRec = KSITOOL_Signature_isCalendarAuthRecPresent(sig);

	/**
	 * If both calendar and authentication record is missing, the signature is
	 * probably extended between publications. Check if it is extended exactly the same
	 * time as encoded in the publication string. In the other case when publication
	 * record is present, check if it matches the user publication. If there is match,
	 * let the real verification for API call.
	 */
	if (!isPubrec && !isCalAuthRec && signature_without_trust_record_isPubDataExactMatch(sig, ksi, refStrn)) {
			print_debug("Signature publication record does not exist.\n");
			print_progressDesc(d, "Adding publication record to the signature generated from publication string... ");

			res = KSI_Signature_clone(sig, &tmp_sig);
			ERR_CATCH_MSG(err, res, "Error: Unable to clone signature for verification.");

			res = signature_add_dummy_pub_rec(tmp_sig, ksi, refStrn);
			ERR_CATCH_MSG(err, res, "Error: Unable to add publications record for verification.");

			exact_match = 1;
			verify_that = tmp_sig;
			print_progressResult(res);
	} else if (isPubrec) {
		print_debug("Signature publication record exists.\n");
		res = KSI_Signature_getPublicationRecord(sig, &sig_pub_rec);
		ERR_CATCH_MSG(err, res, "Error: Unable to extract publication record from signature.");

		res = KSI_PublicationRecord_getPublishedData(sig_pub_rec, &sig_pub_data);
		ERR_CATCH_MSG(err, res, "Error: Unable to get publication data from signatures publication record.");

		if (publication_data_equals(sig_pub_data, user_pub_data)) {
			exact_match = 1;
			verify_that = sig;
		}
	} else {
		print_debug("Signature publication record does not exist.\n");
	}

	/**
	 * There seems to be exact match between publications string and the signature.
	 * Let the API to verify the signature and construct the verification result.
     */
	if (exact_match) {
		print_progressDesc(d, "Verifying signature with user publication... ");

		res = KSI_Signature_verifyWithPublication(verify_that, ksi, user_pub_data);
		ERR_CATCH_MSG(err, res, "Error: Verification failed.");

		print_progressResult(res);
		goto cleanup;
	}

	/**
	 * If publication record in not available or publications are NOT equal
	 * compare the signatures signing time with user publication time to examine
	 * if it is possible to verify the signature after extending.
     */
	print_progressDesc(d, "Check if verification is possible by extending the signature... ");
	res = KSI_Signature_getSigningTime(sig, &sig_signing_time);
	ERR_CATCH_MSG(err, res, "Error: Unable to get signature signing time.");

	if (KSI_Integer_getUInt64(sig_signing_time) > KSI_Integer_getUInt64(user_pub_time)) {
		ERR_TRCKR_ADD(err, res = KT_KSI_SIG_VER_IMPOSSIBLE,
				"Error: Unable to verify signature with user publication as signature is created after user publication.");
		goto cleanup;
	}

	/**
	 * If extending is permitted, extend the signature to the user publication.
     */
	if (!x) {
		ERR_TRCKR_ADD(err, res = KT_KSI_SIG_VER_IMPOSSIBLE,
				"Error: Unable to verify signature as extending is not permitted. Use (-vx) to permit extending.");
		goto cleanup;
	}

	print_progressResult(res);

	/**
	 * As extending is permitted, extend the signature to the user publication.
     */
	print_progressDesc(d, "Extending signature to publication time of publication string... ");
	res = KSITOOL_Signature_extendTo(err, sig, ksi, user_pub_time, &tmp_sig);
	ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	print_progressResult(res);

	/**
	 * If extending was successful, the publication record if present, is removed.
	 * workaround: Create a dummy publication record from the user publication to
	 * make the API verify the signature with user publication.
     */
	res = signature_add_dummy_pub_rec(tmp_sig, ksi, refStrn);
	ERR_CATCH_MSG(err, res, "Error: Unable to add publications record for verification.");

	verify_that = tmp_sig;

	/**
	 * If signature has dummy publication record, it is possible to verify it with
	 * user publication.
	 */
	print_progressDesc(d, "Verifying signature with user publication... ");
	res = KSI_Signature_verifyWithPublication(verify_that, ksi, user_pub_data);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature with user publication.");
	print_progressResult(res);

	res = KT_OK;

cleanup:
	/* If the signature is cloned for the verification, return the signature. */
	if (out != NULL) {
		*out = tmp_sig;
		tmp_sig = NULL;
	}

	print_progressResult(res);

	KSI_PublicationData_free(user_pub_data);
	KSI_Signature_free(tmp_sig);

	return res;
}

