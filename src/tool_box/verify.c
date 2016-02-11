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

#include "../param_set/param_set.h"
#include "../param_set/task_def.h"
#include "../api_wrapper.h"
#include "param_control.h"
#include "gt_task_support.h"
#include "obj_printer.h"
#include "ksi_init.h"

#include "../debug_print.h"
#include "../printer.h"

static int verify_as_possible(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig);
static int verify_internally(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig);
static int verify_calendar_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig);
static int verify_key_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig);
static int verify_publication_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *comp);

int verify_run(int argc, char** argv, char **envp) {
	int res;
	TASK *task = NULL;
	TASK_SET *task_set = NULL;
	PARAM_SET *set = NULL;
	KSI_CTX *ksi = NULL;
	FILE *file = NULL;
	ERR_TRCKR *err = NULL;
	KSI_Signature *sig = NULL;
	COMPOSITE extra;
	char buf[2048];
	int d;

	/**
	 * Extract command line parameters.
     */
	res = PARAM_SET_new("{verify}{i}{o}{H}{D}{d}{x}{pub-str|p}{ver-int}{ver-cal}{ver-key}{ver-pub}"
			DEF_SERVICE_PAR DEF_PARAMETERS
			, &set);
	if (res != KT_OK) {
		goto cleanup;
	}

	/**
	 * Configure parameter set, control, repair and object extractor function.
     */
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, extract_inputSignature);
	PARAM_SET_addControl(set, "{f}", isFormatOk_inputHash, isContentOk_inputHash, convertRepair_path, extract_inputHash);
	PARAM_SET_addControl(set, "{X}{P}{S}", isFormatOk_url, NULL, convertRepair_url, NULL);
	PARAM_SET_addControl(set, "{aggre-user}{aggre-pass}{ext-pass}{ext-user}", isFormatOk_userPass, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{T}", isFormatOk_utcTime, isContentOk_utcTime, NULL, extract_utcTime);
	PARAM_SET_addControl(set, "{d}{x}{ver-int}{ver-cal}{ver-key}{ver-pub}", isFormatOk_flag, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{cnstr}", isFormatOk_constraint, NULL, convertRepair_constraint, NULL);
	PARAM_SET_addControl(set, "{pub-str}", isFormatOk_pubString, NULL, NULL, extract_pubString);

	/**
	 * Define possible tasks.
     */
	res = TASK_SET_new(&task_set);
	if (res != PST_OK) goto cleanup;

	/*					  ID	DESC										MAN							ATL		FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Verify.",									"i",						NULL,	"ver-int,ver-cal,ver-key,ver-pub",		NULL);
	TASK_SET_add(task_set, 1,	"Verify internally.",						"ver-int,i",				NULL,	"ver-cal,ver-key,ver-pub,T,x",		NULL);
	TASK_SET_add(task_set, 2,	"Calendar based verification.",				"ver-cal,i,X",				NULL,	"ver-int,ver-key,ver-pub",		NULL);
	TASK_SET_add(task_set, 3,	"Key based verification.",					"ver-key,i,P,cnstr",		NULL,	"ver-int,ver-cal,ver-pub,T,x",		NULL);

	TASK_SET_add(task_set, 4,	"Publication based verification, use publications file, extending is not allowed.",
																			"ver-pub,i,P,cnstr",		NULL,	"ver-int,ver-cal,ver-key,x,p",		NULL);
	TASK_SET_add(task_set, 5,	"Publication based verification, use publications file, extending is allowed.",
																			"ver-pub,i,P,cnstr,x,X",	NULL,	"ver-int,ver-cal,ver-key,p",		NULL);
	TASK_SET_add(task_set, 6,	"Publication based verification, use publications string, extending is not allowed.",
																			"ver-pub,i,p",	NULL,	"ver-int,ver-cal,ver-key,x,T",		NULL);
	TASK_SET_add(task_set, 7,	"Publication based verification, use publications string, extending is allowed.",
																			"ver-pub,i,p,x,X",	NULL,	"ver-int,ver-cal,ver-key,T",		NULL);

	PARAM_SET_readFromCMD(argc, argv, set, 0);

	d = PARAM_SET_isSetByName(set, "d");

	if (!PARAM_SET_isFormatOK(set)) {
		PARAM_SET_invalidParametersToString(set, NULL, getParameterErrorString, buf, sizeof(buf));
		print_errors("%s", buf);
		res = KT_INVALID_CMD_PARAM;
		goto cleanup;
	}

	/**
	 * Analyze task set and Extract the task if consistent one exists, print help
	 * messaged otherwise.
     */
	res = TASK_SET_analyzeConsistency(task_set, set, 0.2);
	if (res != PST_OK) goto cleanup;

	res = TASK_SET_getConsistentTask(task_set, &task);
	if (res != PST_OK && res != PST_TASK_ZERO_CONSISTENT_TASKS && res !=PST_TASK_MULTIPLE_CONSISTENT_TASKS) goto cleanup;

	if (PARAM_SET_isTypoFailure(set)) {
			printf("%s\n", PARAM_SET_typosToString(set, NULL, buf, sizeof(buf)));
			res = KT_INVALID_CMD_PARAM;
			goto cleanup;
	} else if (PARAM_SET_isUnknown(set)){
			printf("%s\n", PARAM_SET_unknownsToString(set, NULL, buf, sizeof(buf)));
	}

	if (task == NULL) {
		int ID;
		if (TASK_SET_isOneFromSetTheTarget(task_set, 0.1, &ID)) {
			printf("%s", TASK_SET_howToRepair_toString(task_set, set, ID, NULL, buf, sizeof(buf)));
		} else {
			printf("%s", TASK_SET_suggestions_toString(task_set, 3, buf, sizeof(buf)));
		}

		res = KT_INVALID_CMD_PARAM;
		goto cleanup;
	}

	/**
	 * As the task is extracted, initialize a KSI object, read the input file
	 * and execute the verification process.
     */
	res = TOOL_init_ksi(set, &ksi, &err, &file);
	if (res != KT_OK) goto cleanup;

	extra.ctx = ksi;
	extra.err = err;

	print_progressDesc(d, "Reading signature... ");
	res = PARAM_SET_getObjExtended(set, "i", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &extra, (void**)&sig);
	if (res != PST_OK) goto cleanup;
	print_progressResult(res);


	switch(TASK_getID(task)) {
		case 0:
			res = verify_as_possible(set, err, ksi, sig);
		break;
		case 1:
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
		case 6:
		case 7:
			res = verify_publication_based(set, err, ksi, sig, &extra);
		break;
		default:
			res = KT_UNKNOWN_ERROR;
			goto cleanup;
		break;
	}









cleanup:
	print_progressResult(res);

	if (res != KT_OK) {
		KSITOOL_KSI_ERRTrace_save(ksi);
		//TODO: fix debug print.
//		DEBUG_verifySignature(ksi, task, res, sig);
		if (d) ERR_TRCKR_printExtendedErrors(err);
		else  ERR_TRCKR_printErrors(err);
	}

	if (file != NULL) fclose(file);
	PARAM_SET_free(set);
	TASK_SET_free(task_set);
	KSI_Signature_free(sig);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return errToExitCode(res);
}
char *verify_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += snprintf(buf + count, len - count,
		"Usage:\n"
		"ksitool verify -i <in.ksig> [-f <data>] [more options]\n"
		"ksitool verify --ver-int -i <in.ksig> [-f <data>] [more options]\n"
		"ksitool verify --ver-cal -i <in.ksig> [-f <data>] -X <url>\n"
		"        [--ext-user <user> --ext-pass <pass>] [-T <time>] [more options]\n"
		"ksitool verify --ver-key -i <in.ksig> [-f <data>] -P <url>\n"
		"        [--cnstr <oid=value>]... [more options]\n"
		"ksitool verify --ver-pub -i <in.ksig> [-f <data>] -p <pubstring>\n"
		"        [-x -X <url>  [--ext-user <user> --ext-pass <pass>]] [more options]\n"
		"ksitool verify --ver-pub -i <in.ksig> [-f <data>] -P <url> [--cnstr <oid=value>]...\n"
		"        [-x -X <url>  [--ext-user <user> --ext-pass <pass>][-T <time>]] [more options]\n\n"

		"--ver-int - verify just internally.\n"
		"--ver-cal - use calendar based verification (use extender).\n"
		"--ver-key - use key based verification.\n"
		"--ver-pub - use publication based verification (offline if used with --pub-str or -P\n"
		"-i <file> - signature file to be verified.\n"
		"-f <data> - file or data hash to be verified. Hash format: <alg>:<hash in hex>.\n"
		"            as file on local machine).\n"
		"-X <url>  - specify extending service URL.\n"
		"--ext-user <str>\n"
		"          - user name for extending service.\n"
		"--ext-pass <str>\n"
		"          - password for extending service.\n"
		"-T <time> - specify a publication time to extend to as the number of seconds\n"
		"            since 1970-01-01 00:00:00 UTC or time string formatted as \"YYYY-MM-DD hh:mm:ss\".\n"
		"-P <url>  - specify publications file URL (or file with uri scheme 'file://').\n"
		"--cnstr <oid=value>\n"
		"          - publications file certificate verification constraints.\n"
		"-p | --pub-str <str>\n"
		"          - publication string.\n"
		"-x        - allow to use extender when using publication based verification.\n"
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
	print_info("TODO: Internal verification not implemented.\n");

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

static int verify_publication_based(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *comp) {
	int res;
	int d, isExtended, x;

	if (set == NULL || err == NULL || ksi == NULL || sig == NULL || comp == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	d = PARAM_SET_isSetByName(set, "d");
	x = PARAM_SET_isSetByName(set, "x");
	isExtended = isSignatureExtended(sig);

	//TODO:
	print_errors("Error: Not implemented.");
	res = KT_UNKNOWN_ERROR;
	goto cleanup;


cleanup:

	return res;
}