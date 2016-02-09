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

#include "gt_task_support.h"
#include "obj_printer.h"
#include "param_set/param_value.h"
#include "../param_set/param_set.h"
#include "../param_set/task_def.h"
#include "../gt_cmd_control.h"
#include "ksi_init.h"
#include "../api_wrapper.h"

#include "param_control.h"

static int sign(PARAM_SET *set);

int sign_run(int argc, char** argv, char **envp) {
	int exit_code;
	int res;
	TASK *task = NULL;
	TASK_SET *task_set = NULL;
	PARAM_SET *set = NULL;
	char buf[2048];

	/**
	 * Extract command line parameters.
     */
	res = PARAM_SET_new("{sign}{i}{o}{H}{S}{aggre-user}{aggre-pass}{D}{d}"
			DEF_SERVICE_PAR DEF_PARAMETERS
			, &set);
	if (res != KT_OK) {
		goto cleanup;
	}

	/**
	 * Configure parameter set, control, repair and object extractor function.
     */
	PARAM_SET_addControl(set, "{o}{D}", isPathFormOk, isOutputFileContOK, NULL, NULL);
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputHash, isContentOk_inputHash, NULL, extract_inputHash);
	PARAM_SET_addControl(set, "{H}", isHashAlgFormatOK, isHashAlgContOK, NULL, NULL);
	PARAM_SET_addControl(set, "{S}", isURLFormatOK, contentIsOK, convert_repairUrl, NULL);
	PARAM_SET_addControl(set, "{aggre-user}{aggre-pass}", isUserPassFormatOK, contentIsOK, NULL, NULL);
	PARAM_SET_addControl(set, "{d}", isFlagFormatOK, contentIsOK, NULL, NULL);

	/**
	 * Define possible tasks (for error handling only).
     */
	res = TASK_SET_new(&task_set);
	if (res != PST_OK) goto cleanup;

	/*					  ID	DESC										MAN			 ATL	FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Sign data.",								"S,i,o",	 NULL,	NULL,		NULL);
	TASK_SET_add(task_set, 1,	"Sign data, specify hash alg.",				"S,i,o,H",	 NULL,	"D",		NULL);
	TASK_SET_add(task_set, 2,	"Sign and save data.",						"S,i,o,D",	 NULL,	"H",		NULL);
	TASK_SET_add(task_set, 3,	"Sign and save data, specify hash alg.",	"S,i,o,H,D", NULL,	NULL,		NULL);

	PARAM_SET_readFromCMD(argc, argv, set, 0);

	if (!PARAM_SET_isFormatOK(set)) {
		PARAM_SET_invalidParametersToString(set, NULL, getParameterErrorString, buf, sizeof(buf));
		printf("%s", buf);
	}

	/**
	 * Analyze task set and Extract the task if consistent one exists, print help
	 * messaged otherwise.
     */
	res = TASK_SET_analyzeConsistency(task_set, set, 0.2);
	if (res != PST_OK) goto cleanup;

	res = TASK_SET_getConsistentTask(task_set, &task);
	if (res != PST_OK && res != PST_TASK_ZERO_CONSISTENT_TASKS && res !=PST_TASK_MULTIPLE_CONSISTENT_TASKS) goto cleanup;

	if (task == NULL) {
		int ID;
		if (TASK_SET_isOneFromSetTheTarget(task_set, 0.1, &ID)) {
			printf("%s", TASK_SET_howToRepair_toString(task_set, set, ID, NULL, buf, sizeof(buf)));
		} else {
			printf("%s", TASK_SET_suggestions_toString(task_set, 2, buf, sizeof(buf)));
		}
	}

	/**
	 * If everything OK, run the task.
     */
	exit_code = sign(set);

cleanup:

	//TODO: Extract exit_code from error code.
	TASK_SET_free(task_set);
	PARAM_SET_free(set);

	return exit_code;
}
char *sign_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += snprintf(buf + count, len - count,
		"Usage:\n"
		"ksitool sign -i <input> -o <out.ksig> [-H <alg>] -S <url>\n"
		"[--aggre-user <user> --aggre-pass <pass>][--data-out][more options]\n\n"
		"-i <data> file or data hash to be signed. Hash format: <alg>:<hash in hex>.\n"
		"-o <file> output file name to store signature token.\n"
		"-H <alg>  use a specific hash algorithm to hash the file to be signed.\n"
		"-S <url>  specify signing service URL.\n"
		"--aggre-user <str> user name for signing service.\n"
		"--aggre-pass <str> password for signing service.\n"
		"--data-out <file>  save signed data to file.\n"
	);

	return buf;
}
const char *sign_get_desc(void) {return "ksi general signing tool";}


static int sign(PARAM_SET *set) {
	int res;
	int d = 0;
	ERR_TRCKR *err = NULL;
	KSI_CTX *ksi = NULL;
	KSI_DataHash *hash = NULL;
	KSI_Signature *sign = NULL;
	FILE *ksi_log = NULL;
	int retval = EXIT_SUCCESS;
	char *outSigFileName = NULL;
	COMPOSITE extra;


	PARAM_SET_getStrValue(set, "o", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &outSigFileName);
	d = PARAM_SET_isSetByName(set, "d");

	res = TOOL_init_ksi(set, &ksi, &err, &ksi_log);
	if (res != KT_OK) goto cleanup;

	extra.ctx = ksi;
	extra.err = err;

	print_progressDesc(d, "Extracting hash from input... ");
	res = PARAM_SET_getObjExtended(set, "i", NULL, PST_PRIORITY_HIGHEST, 0, &extra, (void**)&hash);
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);


	/* Sign the data hash. */
	print_progressDesc(d, "Creating signature from hash... ");
	res = KSITOOL_createSignature(err, ksi, hash, &sign);
	ERR_CATCH_MSG(err, res, "Error: Unable to create signature.");
	print_progressResult(res);

	/* Save signature file */
	res = saveSignatureFile(err, ksi, sign, outSigFileName);
	if (res != KT_OK) goto cleanup;
	print_info("Signature saved.\n");

	/*Print info*/
	if(d) print_info("\n");
	if (d) OBJPRINT_signerIdentity(sign);
	if (d) OBJPRINT_signatureSigningTime(sign);


cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		if (d) 	ERR_TRCKR_printExtendedErrors(err);
		else 	ERR_TRCKR_printErrors(err);
		retval = errToExitCode(res);
	}

	KSI_Signature_free(sign);
	KSI_DataHash_free(hash);

	if (ksi_log != NULL) fclose(ksi_log);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return retval;
}