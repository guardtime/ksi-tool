/**************************************************************************
 *
 * GUARDTIME CONFIDENTIAL
 *
 * Copyright (C) [2016] Guardtime, Inc
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
#include "param_set/param_set.h"
#include "param_set/task_def.h"
#include "api_wrapper.h"
#include "tool_box/tool_box.h"
#include "tool_box/param_control.h"
#include "tool_box/ksi_init.h"
#include "tool_box/task_initializer.h"
#include "tool_box/smart_file.h"
#include "tool_box/err_trckr.h"
#include "printer.h"
#include "obj_printer.h"
#include "conf.h"
#include "tool.h"

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);
static int sign_save_to_file(PARAM_SET *set, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature **sig);
static char* get_output_file_name_if_not_defined(PARAM_SET *set, ERR_TRCKR *err, char *buf, size_t buf_len);
static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err);
static int check_hash_algo_errors(PARAM_SET *set, ERR_TRCKR *err);

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
	int dump = 0;

	/**
	 * Extract command line parameters.
	 */
	res = PARAM_SET_new(
			CONF_generate_param_set_desc("{sign}{i}{o}{H}{data-out}{d}{dump}{log}{conf}{h|help}", "S", buf, sizeof(buf)),
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

	res = check_pipe_errors(set, err);
	if (res != KT_OK) goto cleanup;

	res = check_hash_algo_errors(set, err);
	if (res != KT_OK) goto cleanup;

	/**
	 * If everything OK, run the task.
	 */
	res = sign_save_to_file(set, ksi, err, &sig);
	if (res != KSI_OK) goto cleanup;

	/**
	 * If signature was created without errors print some info on demand.
	 */
	if (dump) {
		print_result("\n");
		OBJPRINT_signatureDump(sig, print_result);
	}

cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		KSITOOL_KSI_ERRTrace_LOG(ksi);

		print_errors("\n");
		if (d) 	ERR_TRCKR_printExtendedErrors(err);
		else 	ERR_TRCKR_printErrors(err);
	}

	SMART_FILE_close(logfile);
	KSI_Signature_free(sig);
	TASK_SET_free(task_set);
	PARAM_SET_free(set);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return KSITOOL_errToExitCode(res);
}
char *sign_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		" %s sign -i <input> [-o <out.ksig>] -S <URL>\n"
		"         [--aggr-user <user> --aggr-key <key>] [-H <alg>] [--data-out <file>] [more_options]\n"
		"\n"
		"\n"
		" -i <input>\n"
		"           - The data is either the path to the file to be hashed and signed or\n"
		"             a hash imprint in case  the  data  to be signed has been hashed\n"
		"             already. Use '-' as file name to read data to be hashed from stdin.\n"
		"             Hash imprint format: <alg>:<hash in hex>.\n"
		" -o <out.ksig>\n"
		"           - Output file path for the signature. Use '-' as file name to redirect\n"
		"             signature binary stream to stdout. If not specified, the signature is\n"
		"             saved to <input file>.ksig (or <input file>_<nr>.ksig, where <nr> is\n"
		"             auto-incremented counter if the output file already exists). If specified,\n"
		"             will always overwrite the existing file.\n"
		" -H <alg> \n"
		"           - use the given hash algorithm to hash the file to be signed.\n"
		"             Use ksi -h to get the list of supported hash algorithms.\n"
		" -S <URL>  - Signing service (KSI Aggregator) URL.\n"
		" --aggr-user <str>\n"
		"           - Username for signing service.\n"
		" --aggr-key <str>\n"
		"           - HMAC key for signing service.\n"
		" --data-out <file>\n"
		"           - Save signed data to file. Use when signing an incoming stream.\n"
		"             Use '-' as file name to redirect data being hashed to stdout.\n"
		" -d        - Print detailed information about processes and errors to stderr.\n"
		" --dump    - Dump signature created in human-readable format to stdout.\n"
		" --conf <file>\n"
		"           - Read configuration options from given file. It must be noted\n"
		"             that configuration options given explicitly on command line will\n"
		"             override the ones in the configuration file.\n"
		" --log <file>\n"
		"           - Write libksi log to given file. Use '-' as file name to redirect\n"
		"             log to stdout.\n",
		TOOL_getName()
	);

	return buf;
}

const char *sign_get_desc(void) {
	return "Signs the given input with KSI.";
}


static int sign_save_to_file(PARAM_SET *set, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature **sig) {
	int res;
	int d = 0;
	KSI_DataHash *hash = NULL;
	KSI_Signature *tmp = NULL;
	KSI_HashAlgorithm algo = KSI_HASHALG_INVALID;
	char *outSigFileName = NULL;
	char *signed_data_out = NULL;
	COMPOSITE extra;
	const char *mode = NULL;
	char buf[1024];
	char real_output_name[1024];

	if(set == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Extract the signature output file name and signed data output file name if present.
	 * Set file save extra mode to 'i' as incremental write.
	 */
	if (!PARAM_SET_isSetByName(set, "o")) {
		mode = "i";
		outSigFileName = get_output_file_name_if_not_defined(set, err, buf, sizeof(buf));
		if (outSigFileName == NULL) {
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Unable to generate output file name.");
			goto cleanup;
		}
	} else {
		res = PARAM_SET_getStr(set, "o", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &outSigFileName);
		if (res != PST_OK && res != PST_PARAMETER_EMPTY) goto cleanup;
	}

	res = PARAM_SET_getStr(set, "data-out", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &signed_data_out);
	if (res != PST_OK && res != PST_PARAMETER_EMPTY) goto cleanup;

	d = PARAM_SET_isSetByName(set, "d");

	/**
	 * Extract the hash algorithm. If not specified, set algorithm as default.
	 * It must be noted that if hash is extracted from imprint, has algorithm has
	 * no effect.
	 */
	if (PARAM_SET_isSetByName(set, "H")) {
		res = PARAM_SET_getObjExtended(set, "H", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, NULL, (void**)&algo);
		if (res != PST_OK && res != PST_PARAMETER_EMPTY) goto cleanup;
	} else {
		algo = KSI_getHashAlgorithmByName("default");
	}

	/**
	 * Initialize helper data structure, retrieve the has and sign the hash value.
	 */
	extra.ctx = ksi;
	extra.err = err;
	extra.h_alg = &algo;
	extra.fname_out = signed_data_out;

	print_progressDesc(d, "Extracting hash from input... ");
	res = PARAM_SET_getObjExtended(set, "i", NULL, PST_PRIORITY_HIGHEST, 0, &extra, (void**)&hash);
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);

	print_progressDesc(d, "Creating signature from hash... ");
	res = KSITOOL_createSignature(err, ksi, hash, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to create signature.");
	print_progressResult(res);


	/**
	 * Save KSI signature to file.
	 */
	res = KSI_OBJ_saveSignature(err, ksi, tmp, mode, outSigFileName, real_output_name, sizeof(real_output_name));
	if (res != KT_OK) goto cleanup;
	print_debug("Signature saved to '%s'.\n", real_output_name);

	*sig = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:
	print_progressResult(res);

	KSI_Signature_free(tmp);
	KSI_DataHash_free(hash);

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
	res = CONF_initialize_set_functions(set, "S");
	if (res != KT_OK) goto cleanup;

	PARAM_SET_addControl(set, "{conf}", isFormatOk_inputFile, isContentOk_inputFileRestrictPipe, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{o}{data-out}{log}", isFormatOk_path, NULL, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputHash, isContentOk_inputHash, NULL, extract_inputHash);
	PARAM_SET_addControl(set, "{H}", isFormatOk_hashAlg, isContentOk_hashAlg, NULL, extract_hashAlg);
	PARAM_SET_addControl(set, "{d}{dump}", isFormatOk_flag, NULL, NULL, NULL);

	/*					  ID	DESC										MAN			 ATL	FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Sign data.",								"S,i",	 NULL,	"H,data-out",		NULL);
	TASK_SET_add(task_set, 1,	"Sign data, specify hash alg.",				"S,i,H",	 NULL,	"data-out",		NULL);
	TASK_SET_add(task_set, 2,	"Sign and save data.",						"S,i,data-out",	 NULL,	"H",		NULL);
	TASK_SET_add(task_set, 3,	"Sign and save data, specify hash alg.",	"S,i,H,data-out", NULL,	NULL,		NULL);

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
		KSI_snprintf(buf, buf_len, "stdin.ksig");
	} else {
		KSI_snprintf(buf, buf_len, "%s.ksig", in_file_name);
	}

	ret = buf;

cleanup:

	return ret;
}


static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err) {
	int res;

	res = get_pipe_out_error(set, err, "o,data-out", "dump");
	if (res != KT_OK) goto cleanup;

	res = get_pipe_out_error(set, err, "o,data-out,log", NULL);
	if (res != KT_OK) goto cleanup;

cleanup:
	return res;
}

static int check_hash_algo_errors(PARAM_SET *set, ERR_TRCKR *err) {
	int res;
	char *i_value = NULL;

	res = PARAM_SET_getStr(set, "i", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &i_value);
	if (res != KT_OK) goto cleanup;

	if (PARAM_SET_isSetByName(set, "H") && is_imprint(i_value)) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_CMD_PARAM, "Error: Unable to use -H and -i together as input is hash imprint.");
		goto cleanup;
	}

	res = KT_OK;

cleanup:

	return res;
}
