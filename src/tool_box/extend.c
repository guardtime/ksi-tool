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
#include "api_wrapper.h"
#include "printer.h"
#include "obj_printer.h"
#include "debug_print.h"
#include "conf.h"

static int extend(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *comp);
static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);

int extend_run(int argc, char** argv, char **envp) {
	int res;
	TASK *task = NULL;
	TASK_SET *task_set = NULL;
	PARAM_SET *set = NULL;
	KSI_CTX *ksi = NULL;
	SMART_FILE *logfile = NULL;
	ERR_TRCKR *err = NULL;
	KSI_Signature *sig = NULL;
	COMPOSITE extra;
	char buf[2048];
	int d;

	/**
	 * Extract command line parameters.
     */
	res = PARAM_SET_new(
			CONF_generate_desc("{i}{o}{H}{D}{d}{x}{T}{pub-str|p}{conf}{log}", buf, sizeof(buf)),
			&set);
	if (res != KT_OK) goto cleanup;

	res = TASK_SET_new(&task_set);
	if (res != PST_OK) goto cleanup;

	res = generate_tasks_set(set, task_set);
	if (res != PST_OK) goto cleanup;

	res = TASK_INITIALIZER_getServiceInfo(set, argc, argv, envp);
	if (res != PST_OK) goto cleanup;

	res = TASK_INITIALIZER_check_analyze_report(set, task_set, 0.5, 0.1, &task);
	if (res != KT_OK) goto cleanup;

	res = TOOL_init_ksi(set, &ksi, &err, &logfile);
	if (res != KT_OK) goto cleanup;

	d = PARAM_SET_isSetByName(set, "d");

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
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		KSI_LOG_debug(ksi, "\n%s", KSITOOL_KSI_ERRTrace_get());
		print_info("\n");
		DEBUG_verifySignature(ksi, res, sig, NULL);

		print_info("\n");
		if (d) ERR_TRCKR_printExtendedErrors(err);
		else  ERR_TRCKR_printErrors(err);
	}

	SMART_FILE_close(logfile);
	PARAM_SET_free(set);
	TASK_SET_free(task_set);
	KSI_Signature_free(sig);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return errToExitCode(res);
}
char *extend_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		"ksitool extend -i <in.ksig> -o <out.ksig> -X <url>\n"
		"        [--ext-user <user> --ext-pass <pass>] -P <url> [--cnstr <oid=value>]...\n"
		"        [-T <time>] | [--pub-str <str>] [more options]\n\n"

		" -i <file> - signature file to be extended.\n"
		" -o <file> - output file name to store extended signature token.\n"
		" -X <url>  - specify extending service URL.\n"
		" --ext-user <str>\n"
		"           - user name for extending service.\n"
		" --ext-pass <str>\n"
		"           - password for extending service.\n"
		" -P <url>  - specify publications file URL (or file with uri scheme 'file://').\n"
		" --cnstr <oid=value>\n"
		"           - publications file certificate verification constraints.\n"
		" -T <time> - specify a publication time to extend to as the number of seconds\n"
		"             since 1970-01-01 00:00:00 UTC or time string formatted as \"YYYY-MM-DD hh:mm:ss\".\n"
		" --pub-str | -p <str>\n"
		"           - specify a publication string to extend to.\n"
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

	PARAM_SET_getStr(set, "o", NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, &outSigFileName);

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
		fprintf(stderr, "Error: TODO: implement verification process.");
		ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	} else {
		print_progressDesc(d, "Verifying extended signature... ");
		res = KSITOOL_Signature_verify(err, ext, ksi);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	}
	print_progressResult(res);

	/* Save signature. */
	print_progressDesc(d, "Saving signature... ");
	res = KSI_OBJ_saveSignature(err, ksi, ext, outSigFileName);
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);

	res = KT_OK;

cleanup:
	print_progressResult(res);

	KSI_Integer_free(pubTime);
	KSI_PublicationData_free(pub_data);

	return res;
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
	res = CONF_initialize_set_functions(set);
	if (res != KT_OK) goto cleanup;

	/**
	 * Configure parameter set, control, repair and object extractor function.
     */
	PARAM_SET_addControl(set, "{conf}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{log}", isFormatOk_path, NULL, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, extract_inputSignature);
	PARAM_SET_addControl(set, "{T}", isFormatOk_utcTime, isContentOk_utcTime, NULL, extract_utcTime);
	PARAM_SET_addControl(set, "{d}", isFormatOk_flag, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{pub-str}", isFormatOk_pubString, NULL, NULL, extract_pubString);

	/**
	 * Define possible tasks.
     */
	/*					  ID	DESC												MAN					ATL		FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Extend to the nearest publication.",				"i,o,X,P",			NULL,	"T,pub-str",		NULL);
	TASK_SET_add(task_set, 1,	"Extend to the specified time.",					"i,o,X,P,T",		NULL,	"pub-str",		NULL);
	TASK_SET_add(task_set, 2,	"Extend to time specified in publications string.",	"i,o,X,P,pub-str",	NULL,	"T",		NULL);

cleanup:

	return res;
}