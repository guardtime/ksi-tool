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
#include "api_wrapper.h"
#include "tool_box/tool_box.h"
#include "tool_box/param_control.h"
#include "tool_box/ksi_init.h"
#include "tool_box/task_initializer.h"
#include "tool_box/smart_file.h"
#include "printer.h"
#include "obj_printer.h"
#include "conf.h"

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);
static int sign_save_to_file(PARAM_SET *set, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature **sig);

int sign_run(int argc, char** argv, char **envp) {
	int res;
	char buf[2048];
	PARAM_SET *set = NULL;
	TASK_SET *task_set = NULL;
	TASK *task = NULL;
	KSI_CTX *ksi = NULL;
	ERR_TRCKR *err = NULL;
	SMART_FILE *logfile = NULL;
	KSI_Signature *sig = NULL;
	int d = 0;

	/**
	 * Extract command line parameters.
     */
	res = PARAM_SET_new(
			CONF_generate_desc("{sign}{i}{o}{H}{D}{d}{log}{conf}", buf, sizeof(buf)),
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

	/**
	 * If everything OK, run the task.
     */
	res = sign_save_to_file(set, ksi, err, &sig);
	if (res != KSI_OK) goto cleanup;

	/**
	 * If signature was created without errors print some info on demand.
     */
	if (d) {
		print_info("\n");
		OBJPRINT_signatureDump(sig);
	}

cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		KSI_LOG_debug(ksi, "\n%s", KSITOOL_KSI_ERRTrace_get());

		print_info("\n");
		if (d) 	ERR_TRCKR_printExtendedErrors(err);
		else 	ERR_TRCKR_printErrors(err);
	}

	SMART_FILE_close(logfile);
	KSI_Signature_free(sig);
	TASK_SET_free(task_set);
	PARAM_SET_free(set);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return errToExitCode(res);
}
char *sign_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		"ksitool sign -i <input> -o <out.ksig> [-H <alg>] -S <url>\n"
		"[--aggre-user <user> --aggre-pass <pass>][--data-out][more options]\n\n"
		" -i <data> - file or data hash to be signed. Hash format: <alg>:<hash in hex>.\n"
		" -o <file> - output file name to store signature token.\n"
		" -H <alg>  - use a specific hash algorithm to hash the file to be signed.\n"
		" -S <url>  - specify signing service URL.\n"
		" --aggre-user <str>\n"
		"           - user name for signing service.\n"
		" --aggre-pass <str>\n"
		"           - password for signing service.\n"
		" --data-out <file>\n"
		"           - save signed data to file.\n"
	);

	return buf;
}
const char *sign_get_desc(void) {
	return "KSI general signing tool.";
}


static int sign_save_to_file(PARAM_SET *set, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature **sig) {
	int res;
	int d = 0;
	KSI_DataHash *hash = NULL;
	KSI_Signature *tmp = NULL;
	int retval = EXIT_SUCCESS;
	char *outSigFileName = NULL;
	COMPOSITE extra;

	if(set == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	PARAM_SET_getStr(set, "o", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &outSigFileName);
	d = PARAM_SET_isSetByName(set, "d");

	extra.ctx = ksi;
	extra.err = err;

	print_progressDesc(d, "Extracting hash from input... ");
	res = PARAM_SET_getObjExtended(set, "i", NULL, PST_PRIORITY_HIGHEST, 0, &extra, (void**)&hash);
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);


	/* Sign the data hash. */
	print_progressDesc(d, "Creating signature from hash... ");
	res = KSITOOL_createSignature(err, ksi, hash, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to create signature.");
	print_progressResult(res);

	/* Save signature file */
	res = KSI_OBJ_saveSignature(err, ksi, tmp, outSigFileName);
	if (res != KT_OK) goto cleanup;
	print_info("Signature saved.\n");

	*sig = tmp;
	tmp = NULL;

cleanup:
	print_progressResult(res);

	KSI_Signature_free(tmp);
	KSI_DataHash_free(hash);

	return retval;
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

	PARAM_SET_addControl(set, "{conf}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{o}{D}{log}", isFormatOk_path, NULL, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputHash, isContentOk_inputHash, NULL, extract_inputHash);
	PARAM_SET_addControl(set, "{H}", isFormatOk_hashAlg, isContentOk_hashAlg, NULL, extract_hashAlg);
	PARAM_SET_addControl(set, "{d}", isFormatOk_flag, NULL, NULL, NULL);

	/*					  ID	DESC										MAN			 ATL	FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Sign data.",								"S,i,o",	 NULL,	"H",		NULL);
	TASK_SET_add(task_set, 1,	"Sign data, specify hash alg.",				"S,i,o,H",	 NULL,	"D",		NULL);
	TASK_SET_add(task_set, 2,	"Sign and save data.",						"S,i,o,D",	 NULL,	"H",		NULL);
	TASK_SET_add(task_set, 3,	"Sign and save data, specify hash alg.",	"S,i,o,H,D", NULL,	NULL,		NULL);

cleanup:

	return res;
}