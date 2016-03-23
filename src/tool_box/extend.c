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
#include <string.h>
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
#include "obj_printer.h"
#include "debug_print.h"
#include "conf.h"
#include "tool.h"

static int extend(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *comp, KSI_PolicyVerificationResult **result);
static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);
static char* get_output_file_name_if_not_defined(PARAM_SET *set, ERR_TRCKR *err, char *buf, size_t buf_len);

int extend_run(int argc, char** argv, char **envp) {
	int res;
	TASK *task = NULL;
	TASK_SET *task_set = NULL;
	PARAM_SET *set = NULL;
	KSI_CTX *ksi = NULL;
	SMART_FILE *logfile = NULL;
	ERR_TRCKR *err = NULL;
	KSI_Signature *sig = NULL;
	KSI_PolicyVerificationResult *result;
	COMPOSITE extra;
	char buf[2048];
	int d;

	/**
	 * Extract command line parameters.
	 */
	res = PARAM_SET_new(
			CONF_generate_desc("{i}{o}{H}{D}{d}{x}{T}{pub-str}{conf}{log}", buf, sizeof(buf)),
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
			res = extend(set, err, ksi, sig, &extra, &result);
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
		print_debug("\n");
		DEBUG_verifySignature(ksi, res, sig, result, NULL);

		print_errors("\n");
		if (d) ERR_TRCKR_printExtendedErrors(err);
		else  ERR_TRCKR_printErrors(err);
	}

	SMART_FILE_close(logfile);
	PARAM_SET_free(set);
	TASK_SET_free(task_set);
	KSI_Signature_free(sig);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return KSITOOL_errToExitCode(res);
}
char *extend_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		"%s extend -i <in.ksig> [-o <out.ksig>] -X <url>\n"
		"        [--ext-user <user> --ext-key <pass>] -P <url> [--cnstr <oid=value>]...\n"
		"        [-T <time>] | [--pub-str <str>] [more options]\n\n"

		" -i <file> - signature file to be extended.\n"
		" -o <file> - Output file name to store extended signature token. Use '-'\n"
		"             as file name to redirect signature binary stream to stdout.\n"
		"             If not specified signature is saved to the path described as\n"
		"             <input files path>(nr).ext, where (nr) is generated serial\n"
		"             number if file name already exists. If specified will always\n"
		"             overwrite the existing file.\n"
		" -X <url>  - specify extending service URL.\n"
		" --ext-user <str>\n"
		"           - user name for extending service.\n"
		" --ext-key <str>\n"
		"           - HMAC key for extending service.\n"
		" -P <url>  - specify publications file URL (or file with uri scheme 'file://').\n"
		" --cnstr <oid=value>\n"
		"           - publications file certificate verification constraints.\n"
		" -T <time> - specify a publication time to extend to as the number of seconds\n"
		"             since 1970-01-01 00:00:00 UTC or time string formatted as \"YYYY-MM-DD hh:mm:ss\".\n"
		" --pub-str | -p <str>\n"
		"           - specify a publication string to extend to.\n"
		" -d        - print detailed information about processes and errors.\n"
		" --conf <file>\n"
		"           - specify a configurations file to override default service\n"
		"             information. It must be noted that service info from\n"
		"             command-line will override the configurations file.\n"
		" --log <file>\n"
		"           - Write libksi log into file.",
		TOOL_getName()
		);

	return buf;
}
const char *extend_get_desc(void) {
	return "KSI signature extending tool.";
}

static int extend(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, COMPOSITE *extra, KSI_PolicyVerificationResult **result) {
	int res;
	int d = 0;
	int T = 0;
	int p = 0;
	KSI_Signature *ext = NULL;
	KSI_Integer *pubTime = NULL;
	KSI_PublicationData *pub_data = NULL;
	char *outSigFileName = NULL;
	char buf[1024] = "";
	const char *mode = NULL;

	if (set == NULL || ksi == NULL || err == NULL || sig == NULL || extra == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (!PARAM_SET_isSetByName(set, "o")) {
		mode = "i";
		outSigFileName = get_output_file_name_if_not_defined(set, err, buf, sizeof(buf));
		if (outSigFileName == NULL) {
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Unable to generate output file name.");
			goto cleanup;
		}
	} else {
		res = PARAM_SET_getStr(set, "o", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &outSigFileName);
		if (res != PST_OK) goto cleanup;
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
	res = KSITOOL_SignatureVerify_internally(err, sig, ksi, result);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature.");
	if ((*result)->finalResult.resultCode != VER_RES_OK) {
		ERR_CATCH_MSG(err, res = KT_KSI_SIG_VER_IMPOSSIBLE, "Error: Failed to verify signature.");
	}
	print_progressResult(res);


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

	print_progressDesc(d, "Verifying extended signature... ");
	res = KSITOOL_SignatureVerify_general(err, ext, ksi, pub_data, 1, result);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	if ((*result)->finalResult.resultCode != VER_RES_OK) {
		ERR_CATCH_MSG(err, res = KT_KSI_SIG_VER_IMPOSSIBLE, "Error: Unable to verify extended signature.");
	}
	print_progressResult(res);

	/* Save signature. */
	print_progressDesc(d, "Saving signature... ");
	res = KSI_OBJ_saveSignature(err, ksi, ext, mode, outSigFileName);
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
	TASK_SET_add(task_set, 0,	"Extend to the nearest publication.",				"i,X,P",			NULL,	"T,pub-str",		NULL);
	TASK_SET_add(task_set, 1,	"Extend to the specified time.",					"i,X,P,T",		NULL,	"pub-str",		NULL);
	TASK_SET_add(task_set, 2,	"Extend to time specified in publications string.",	"i,X,P,pub-str",	NULL,	"T",		NULL);

cleanup:

	return res;
}

static char* get_output_file_name_if_not_defined(PARAM_SET *set, ERR_TRCKR *err, char *buf, size_t buf_len) {
	char *ret = NULL;
	int res;
	char *in_file_name = NULL;

	if (set == NULL || err == NULL || buf == NULL || buf_len == 0) goto cleanup;

	res = PARAM_SET_getStr(set, "i", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &in_file_name);
	if (res != PST_OK) goto cleanup;

	if (strcmp(in_file_name, "-") == 0) {
		KSI_snprintf(buf, buf_len, "stdin.ksig.ext");
	} else {
		KSI_snprintf(buf, buf_len, "%s.ext", in_file_name);
	}

	ret = buf;

cleanup:

	return ret;
}

static int verify_signature(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_PolicyVerificationResult **result) {
	return KT_INVALID_ARGUMENT;
}
