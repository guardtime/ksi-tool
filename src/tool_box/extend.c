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
#include "../gt_cmd_control.h"
#include "../api_wrapper.h"
#include "param_control.h"
#include "gt_task_support.h"
#include "obj_printer.h"
#include "ksi_init.h"

#include "../debug_print.h"
#include "../printer.h"

static int extend(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *comp);

int extend_run(int argc, char** argv, char **envp) {
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
	res = PARAM_SET_new("{verify}{i}{o}{H}{D}{d}{x}{T}{pub-str|p}{ver-int}{ver-cal}{ver-key}{ver-pub}"
			DEF_SERVICE_PAR DEF_PARAMETERS
			, &set);
	if (res != KT_OK) {
		goto cleanup;
	}

	/**
	 * Configure parameter set, control, repair and object extractor function.
     */
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputFile, isContentOk_inputFile, convert_repair_path, extract_inputSignature);
	PARAM_SET_addControl(set, "{X}{P}{S}", isFormatOk_url, NULL, convertRepair_url, NULL);
	PARAM_SET_addControl(set, "{aggre-user}{aggre-pass}{ext-pass}{ext-user}", isUserPassFormatOK, contentIsOK, NULL, NULL);
	PARAM_SET_addControl(set, "{T}", isFormatOk_utcTime, isContentOk_utcTime, NULL, extract_utcTime);
	PARAM_SET_addControl(set, "{d}", isFlagFormatOK, contentIsOK, NULL, NULL);
	PARAM_SET_addControl(set, "{cnstr}", isConstraintFormatOK, contentIsOK, convert_replaceWithOid, NULL);
	PARAM_SET_addControl(set, "{pub-str}", isFormatOk_pubString, NULL, NULL, extract_pubString);

	/**
	 * Define possible tasks.
     */
	res = TASK_SET_new(&task_set);
	if (res != PST_OK) goto cleanup;

	/*					  ID	DESC												MAN					ATL		FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Extend to the nearest publication.",				"i,o,X,P",			NULL,	"T,pub-str",		NULL);
	TASK_SET_add(task_set, 1,	"Extend to the specified time.",					"i,o,X,P,T",		NULL,	"pub-str",		NULL);
	TASK_SET_add(task_set, 2,	"Extend to time specified in publications string.",	"i,o,X,P,pub-str",	NULL,	"T",		NULL);

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
		case 1:
		case 2:
			res = extend(set, err, ksi, sig, &extra);
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
char *extend_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += snprintf(buf + count, len - count,
		"Usage:\n"
		"ksitool extend -i <in.ksig> -o <out.ksig> -X <url>\n"
		"        [--ext-user <user> --ext-pass <pass>] -P <url> [--cnstr <oid=value>]...\n"
		"        [-T <time>] | [--pub-str <str>] [more options]\n\n"

		"-i <file> - signature file to be extended.\n"
		"-o <file> - output file name to store extended signature token.\n"
		"-X <url>  - specify extending service URL.\n"
		"--ext-user <str>\n"
		"          - user name for extending service.\n"
		"--ext-pass <str>\n"
		"          - password for extending service.\n"
		"-P <url>  - specify publications file URL (or file with uri scheme 'file://').\n"
		"--cnstr <oid=value>\n"
		"          - publications file certificate verification constraints.\n"
		"-T <time> - specify a publication time to extend to as the number of seconds\n"
		"            since 1970-01-01 00:00:00 UTC or time string formatted as \"YYYY-MM-DD hh:mm:ss\".\n"
		"--pub-str | -p <str>\n"
		"          - specify a publication string to extend to.\n"
	);

	return buf;
}
const char *extend_get_desc(void) {
	return "KSI signature extending tool.";
}

static int extend(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *extra) {
	int res;
	int d = 0;
	int T = 0;
	int p = 0;
	KSI_Signature *ext = NULL;
	KSI_Integer *pubTime = NULL;
	KSI_PublicationData *pub_data = NULL;
	char *outSigFileName = NULL;
	int publicationTime = 0;
	char buf[1024] = "";

	if (set == NULL || ksi == NULL || err == NULL || sig == NULL || extra == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	d = PARAM_SET_isSetByName(set, "d");
	if (PARAM_SET_isSetByName(set, "T")) {
		res = PARAM_SET_getObjExtended(set, "T", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&pubTime);
		if (res != KT_OK) goto cleanup;
		T = 1;
	} else if (PARAM_SET_isSetByName(set, "pub-str")) {
		res = PARAM_SET_getObjExtended(set, "pub-str", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&pub_data);
		if (res != KT_OK) goto cleanup;

		res = KSI_PublicationData_getTime(pub_data, &pubTime);
		if (res != KT_OK) goto cleanup;
		p = 1;
	}


	/* Make sure the signature is ok. */
	print_progressDesc(d, "Verifying old signature... ");
	res = KSITOOL_Signature_verify(err, sig, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature.");
	print_progressResult(res);

	PARAM_SET_getStrValue(set, "o", NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, &outSigFileName);

	/* Extend the signature. */
	if(pubTime != NULL){
		print_progressDesc(d, "Extending old signature to %s (%i)... ",
				KSI_Integer_toDateString(pubTime, buf, sizeof(buf)),
				KSI_Integer_getUInt64(pubTime));
		res = KSITOOL_Signature_extendTo(err, sig, ksi, pubTime, &ext);
		ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	} else {
		print_progressDesc(d, "Extending old signature... ");
		res = KSITOOL_extendSignature(err, ksi, sig, &ext);
		ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	}
	print_progressResult(res);

	if (p) {
		print_progressDesc(d, "Verifying extended signature with user publication... ");
		print_errors("Error: TODO: implement verification process.");
		ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	} else {
		print_progressDesc(d, "Verifying extended signature... ");
		res = KSITOOL_Signature_verify(err, ext, ksi);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	}
	print_progressResult(res);

	/* Save signature. */
	res = saveSignatureFile(err, ksi, ext, outSigFileName);
	if (res != KT_OK) goto cleanup;
	print_info("Extended signature saved.\n");

	res = KT_OK;

cleanup:

	print_progressResult(res);
	if (T) KSI_Integer_free(pubTime);
	KSI_PublicationData_free(pub_data);

	return res;
}
