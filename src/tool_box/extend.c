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
#include <time.h>
#include <ksi/ksi.h>
#include <ksi/compatibility.h>
#include <ksi/policy.h>
#include "param_set/param_set.h"
#include "param_set/task_def.h"
#include "param_set/parameter.h"
#include "tool_box/ksi_init.h"
#include "tool_box/param_control.h"
#include "tool_box/task_initializer.h"
#include "tool_box.h"
#include "smart_file.h"
#include "err_trckr.h"
#include "api_wrapper.h"
#include "printer.h"
#include "obj_printer.h"
#include "debug_print.h"
#include "conf_file.h"
#include "tool.h"
#include "common.h"

static int extend_to_nearest_publication(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext);
static int extend_to_specified_time(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, COMPOSITE *extra, KSI_Signature *sig, KSI_Signature **ext);
static int extend_to_specified_publication(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext);
static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);
static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err);
static int check_other_input_param_errors(PARAM_SET *set, ERR_TRCKR *err);
static int perform_extending(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, int task_id);

int extend_run(int argc, char** argv, char **envp) {
	int res;
	TASK *task = NULL;
	TASK_SET *task_set = NULL;
	PARAM_SET *set = NULL;
	KSI_CTX *ksi = NULL;
	SMART_FILE *logfile = NULL;
	ERR_TRCKR *err = NULL;
	char buf[2048];
	int d = 0;

	/**
	 * Extract command line parameters.
	 */
	res = PARAM_SET_new(
			CONF_generate_param_set_desc("{i}{input}{o}{d}{x}{T}{pub-str}{dump}{conf}{log}{h|help}{replace-existing}", "XP", buf, sizeof(buf)),
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

	res = check_pipe_errors(set, err);
	if (res != KT_OK) goto cleanup;

	res = check_general_io_errors(set, err, "i,input", "o");
	if (res != PST_OK) goto cleanup;

	res = check_other_input_param_errors(set, err);
	if (res != PST_OK) goto cleanup;

	res = perform_extending(set, err, ksi, TASK_getID(task));
	if (res != KT_OK) goto cleanup;


cleanup:
	/* Debugging and KSITOOL_KSI_ERRTrace_save is called in perform_extending. */
	print_progressResult(res);

	if (res != KT_OK) {
		print_errors("\n");
		if (d) ERR_TRCKR_printExtendedErrors(err);
		else  ERR_TRCKR_printErrors(err);
	}

	SMART_FILE_close(logfile);
	PARAM_SET_free(set);
	TASK_SET_free(task_set);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return KSITOOL_errToExitCode(res);
}
char *extend_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		" %s extend [-i <in.ksig>] [-o <out.ksig>] -X <URL>\n"
		"    [--ext-user <user> --ext-key <key>] -P <URL> [--cnstr <oid=value>]...\n"
		"    [more_options] [--] input...\n"
		" %s extend [-i <in.ksig>] [-o <out.ksig>] -X <URL>\n"
		"    [--ext-user <user> --ext-key <key>] -P <URL> [--cnstr <oid=value>]...\n"
		"    [--pub-str <str>] [more_options] [--] input...\n"
		" %s extend [-i <in.ksig>] [-o <out.ksig>] -X <URL>\n"
		"    [--ext-user <user> --ext-key <key>] -T time [more_options] [--] input...\n"
		"\n"
		" -i <in.ksig>\n"
		"           - File path to the KSI signature file to be extended. Use '-' as the\n"
		"             path to read the signature from stdin.\n"
		"             Flag -i can be omitted when specifying the input. To interpret all\n"
		"             inputs as regular files no matter what the file's name is see\n"
		"             parameter --.\n"
		" -o <out.ksig>\n"
		"             Specify the output file path for the extended signature. Use '-' as\n"
		"             the path to redirect the signature binary stream to stdout. If not\n"
		"             specified, the output is saved to the same directory where the\n"
		"             input file is located. If specified as directory, all the\n"
		"             signatures are saved there. When signature's output file name is\n"
		"             not explicitly specified the signature is saved to\n"
		"             <input[.E]>.ext.ksig or <input[.E]>.ext_<nr>.ksig where E is input\n"
		"             file extension that is NOT equal to ksig and nr is auto-incremented\n"
		"             counter if the output file already exists. If output file name is\n"
		"             explicitly specified, will always overwrite the existing file.\n"
		" -X <URL>  - Extending service (KSI Extender) URL.\n"
		" --ext-user <user>\n"
		"           - Username for extending service.\n"
		" --ext-key <key>\n"
		"           - HMAC key for extending service.\n"
		" -P <URL>  - Publications file URL (or file with URI scheme 'file://').\n"
		" --cnstr <oid=value>\n"
		"           - OID of the PKI certificate field (e.g. e-mail address) and the\n"
		"             expected value to qualify the certificate for verification of\n"
		"             publications file PKI signature. At least one constraint must be\n"
		"             defined.\n"
		" --pub-str <str>\n"
		"           - Publication record as publication string to extend the signature\n"
		"             to.\n"
		" --replace-existing\n"
		"           - Replace input KSI signature with the successfully extended version.\n"
		" -T <time> - Publication time to extend to as the number of seconds since\n"
		"             1970-01-01 00:00:00 UTC or time string formatted as\n"
		"             \"YYYY-MM-DD hh:mm:ss\".\n"
		" -V        - Certificate file in PEM format for publications file verification.\n"
		"             All values from lower priority source are ignored.\n"
		" --        - If used everything specified after the token is interpreted as\n"
		"             input file (command-line parameters (e.g. --conf, -d), stdin (-)\n"
		"             and pre-calculated hash imprints (SHA-256:7647c6...) are all\n"
		"             interpreted as regular files).\n"
		" -d        - Print detailed information about processes and errors.\n"
		" --dump    - Dump extended signature and verification info in human-readable\n"
		"             format to stdout.\n"
		" --conf <file>\n"
		"             Read configuration options from given file. It must be noted\n"
		"             that configuration options given explicitly on command line will\n"
		"             override the ones in the configuration file.\n"
		" --log <file>\n"
		"           - Write libksi log to given file. Use '-' as file name to redirect\n"
		"             log to stdout.\n",
		TOOL_getName(),
		TOOL_getName(),
		TOOL_getName()
	);

	return buf;
}
const char *extend_get_desc(void) {
	return "Extends existing KSI signature to the given publication.";
}

static int save_extended(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *ext, const char *fname, const char *mode) {
	int res = KT_UNKNOWN_ERROR;
	char real_output_name[1024];
	char temp_file_name[1024];
	int d;
	int is_replace = 0;

	if (set == NULL || err == NULL || ksi == NULL || ext == NULL || fname == NULL || mode == NULL) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_ARGUMENT, NULL);
		goto cleanup;
	}

	is_replace = PARAM_SET_isSetByName(set, "replace-existing");
	d = PARAM_SET_isSetByName(set, "d");

	if (is_replace) {
		/* To be extra sure that overwriting is restricted check the mode. */
		if (strchr(mode, 'f') == NULL) {
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Write mode specified '%s' do not contain f.", mode);
			goto cleanup;
		}

		print_progressDesc(d, "Creating backup... ");

		srand(0xffffffff & time(NULL));
		KSI_snprintf(temp_file_name, sizeof(temp_file_name), "%s.backup.%010d%010d", fname, rand(),rand());

		res = SMART_FILE_rename(fname, temp_file_name);
		ERR_CATCH_MSG(err, res, "Error: Unable to create backup file '%s' for '%s'.", temp_file_name, fname)
		print_progressResult(res);
	}

	/* Save signature. */
	print_progressDesc(d, "Saving signature... ");
	res = KSI_OBJ_saveSignature(err, ksi, ext, mode, fname, real_output_name, sizeof(real_output_name));
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);

	if (is_replace) {
		print_progressDesc(d, "Removing backup... ");

		/* To be extra sure that original file was written as expected run some extra checks. */
		if (!SMART_FILE_doFileExist(real_output_name)) {
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Unable to verify that extended signature is actually saved. Original is available at %s.", temp_file_name);
			goto cleanup;
		}

		if (strcmp(fname, real_output_name) != 0) {
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Signature saved to different file name as expected. Original is not removed and is available at %s.", temp_file_name);
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Expected output '%s' instead of 's'.", fname, real_output_name);
			goto cleanup;
		}

		res = SMART_FILE_remove(temp_file_name);
		ERR_CATCH_MSG(err, res, "Error: Unable to remove backup file '%s'.", temp_file_name, fname)
		print_progressResult(res);
	}

	print_debug("Signature saved to '%s'.\n", real_output_name);

	res = KT_OK;

cleanup:
	print_progressResult(res);
	return res;
}

static int verify_and_save(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *ext, const char *fname, const char *mode, KSI_PublicationData *pub_data, KSI_PolicyVerificationResult **result) {
	int res;
	int d;

	if (set == NULL || err == NULL || ksi == NULL || ext == NULL || fname == NULL || mode == NULL) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_ARGUMENT, NULL);
		goto cleanup;
	}


	d = PARAM_SET_isSetByName(set, "d");

	print_progressDesc(d, "Verifying extended signature... ");
	res = KSITOOL_SignatureVerify_general(err, ext, ksi, NULL, pub_data, 1, result);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	print_progressResult(res);

	res = save_extended(set, err, ksi, ext, fname, mode);
	if (res != KT_OK) goto cleanup;

	res = KT_OK;

cleanup:
	print_progressResult(res);

	return res;
}

static int extend_to_nearest_publication(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext) {
	int res;
	int d = 0;
	KSI_Signature *tmp = NULL;
	KSI_PublicationsFile *pubFile = NULL;

	if (set == NULL || ksi == NULL || err == NULL || sig == NULL || ext == NULL) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_ARGUMENT, NULL);
		goto cleanup;
	}


	d = PARAM_SET_isSetByName(set, "d");

	print_progressDesc(d, "%s", getPublicationsFileRetrieveDescriptionString(set));
	res = KSITOOL_receivePublicationsFile(err, ksi, &pubFile);
	ERR_CATCH_MSG(err, res, "Error: Unable receive publications file.");
	print_progressResult(res);

	if (!PARAM_SET_isSetByName(set, "publications-file-no-verify")) {
		print_progressDesc(d, "Verifying publications file... ");
		res = KSITOOL_verifyPublicationsFile(err, ksi, pubFile);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify publications file.");
		print_progressResult(res);
	}

	print_progressDesc(d, "Extend the signature to the earliest available publication... ");
	res = KSITOOL_extendSignature(err, ksi, sig, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	print_progressResult(res);

	*ext = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:
	print_progressResult(res);

	KSI_PublicationsFile_free(pubFile);
	KSI_Signature_free(tmp);

	return res;
}

static int extend_to_specified_time(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, COMPOSITE *extra, KSI_Signature *sig, KSI_Signature **ext) {
	int res;
	int d = 0;
	KSI_Signature *tmp = NULL;
	KSI_Integer *pubTime = NULL;
	char buf[256];


	if (set == NULL || ksi == NULL || err == NULL || sig == NULL || extra == NULL || ext == NULL) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_ARGUMENT, NULL);
		goto cleanup;
	}


	d = PARAM_SET_isSetByName(set, "d");

	res = PARAM_SET_getObjExtended(set, "T", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&pubTime);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to extract the time value to extend to.");
		goto cleanup;
	}

	/* Extend the signature. */
	print_progressDesc(d, "Extending the signature to %s (%i)... ",
			KSI_Integer_toDateString(pubTime, buf, sizeof(buf)),
			KSI_Integer_getUInt64(pubTime));
	res = KSITOOL_Signature_extendTo(err, sig, ksi, pubTime, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	print_progressResult(res);

	*ext = tmp;
	tmp = NULL;

	res = KT_OK;

cleanup:
	print_progressResult(res);

	KSI_Integer_free(pubTime);
	KSI_Signature_free(tmp);

	return res;
}

static int extend_to_specified_publication(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext) {
	int res;
	int d = 0;
	KSI_Signature *tmp = NULL;
	KSI_PublicationRecord *pub_rec = NULL;
	KSI_PublicationsFile *pubFile = NULL;
	char *pubs_str = NULL;

	if (set == NULL || ksi == NULL || err == NULL || sig == NULL || ext == NULL) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_ARGUMENT, NULL);
		goto cleanup;
	}

	d = PARAM_SET_isSetByName(set, "d");
	res = PARAM_SET_getStr(set, "pub-str", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &pubs_str);
	ERR_CATCH_MSG(err, res, "Error: Unable get publication string.");

	print_progressDesc(d, "%s", getPublicationsFileRetrieveDescriptionString(set));
	res = KSITOOL_receivePublicationsFile(err, ksi, &pubFile);
	ERR_CATCH_MSG(err, res, "Error: Unable receive publications file.");
	print_progressResult(res);

	print_progressDesc(d, "Searching for a publication record from publications file... ");
	res = KSI_PublicationsFile_getPublicationDataByPublicationString(pubFile, pubs_str, &pub_rec);
	ERR_CATCH_MSG(err, res, "Error: Unable get publication record from publications file.");
	if (pub_rec == NULL) {
		ERR_TRCKR_ADD(err, res = KT_PUBFILE_HAS_NO_PUBREC_TO_EXTEND_TO, "Error: Unable to extend signature as publication record not found from publications file.");
		goto cleanup;
	}
	print_progressResult(res);


	if (!PARAM_SET_isSetByName(set, "publications-file-no-verify")) {
		print_progressDesc(d, "Verifying publications file... ");
		res = KSITOOL_verifyPublicationsFile(err, ksi, pubFile);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify publications file.");
		print_progressResult(res);
	}

	print_progressDesc(d, "Extend the signature to the specified publication... ");
	res = KSITOOL_Signature_extend(err, sig, ksi, pub_rec, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	print_progressResult(res);

	*ext = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:
	print_progressResult(res);

	KSI_PublicationsFile_free(pubFile);
	KSI_Signature_free(tmp);

	return res;
}

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set) {
	int res;
	int extra_parse_flags = 0;

	if (set == NULL || task_set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Configure parameter set, control, repair and object extractor function.
	 */
	res = CONF_initialize_set_functions(set, "XP");
	if (res != KT_OK) goto cleanup;

	/**
	 * Configure parameter set, control, repair and object extractor function.
	 */
	PARAM_SET_addControl(set, "{conf}", isFormatOk_inputFile, isContentOk_inputFileRestrictPipe, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{log}{o}", isFormatOk_path, NULL, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputFile, isContentOk_inputFileWithPipe, convertRepair_path, extract_inputSignature);
	PARAM_SET_addControl(set, "{input}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, extract_inputSignatureFromFile);
	PARAM_SET_addControl(set, "{T}", isFormatOk_utcTime, isContentOk_utcTime, NULL, extract_utcTime);
	PARAM_SET_addControl(set, "{d}{dump}", isFormatOk_flag, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{pub-str}", isFormatOk_pubString, NULL, NULL, extract_pubString);
	PARAM_SET_setParseOptions(set, "{d}{dump}{replace-existing}", PST_PRSCMD_HAS_NO_VALUE);

	/**
	 * To enable wildcard characters (WC) to work on Windows, configure the WC
	 * expander function and set parsing flag that enables WC parsing after all
	 * values from command-line are read.
	 */
#ifdef _WIN32
	res = PARAM_SET_wildcardExpander(set, "i,input", NULL, Win32FileWildcard);
	extra_parse_flags = PST_PRSCMD_EXPAND_WILDCARD;
#endif

	/**
	 * Make the parameter -i collect:
	 * 1) All values that are exactly after -i (both a and -i are collected -i a, -i -i)
	 * 2) all values that are not potential parameters (unknown / typo) parameters (will ignore -q, --test)
	 * Make the parameter input collect all values after the parsing is closed.
	 * 1) All values that are specified after --.
	 */
	PARAM_SET_setParseOptions(set, "i", PST_PRSCMD_HAS_VALUE | PST_PRSCMD_COLLECT_LOOSE_VALUES | extra_parse_flags);
	PARAM_SET_setParseOptions(set, "input", PST_PRSCMD_CLOSE_PARSING | PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED | PST_PRSCMD_HAS_NO_FLAG | PST_PRSCMD_NO_TYPOS | extra_parse_flags);

	/**
	 * Define possible tasks.
	 */
	/*					  ID	DESC												MAN				ATL			FORBIDDEN		IGN	*/
	TASK_SET_add(task_set, 0,	"Extend to the earliest available publication.",	"X,P",			"i,input",	"T,pub-str",	NULL);
	TASK_SET_add(task_set, 1,	"Extend to the specified time.",					"X,T",			"i,input",	"pub-str",		NULL);
	TASK_SET_add(task_set, 2,	"Extend to time specified in publications string.",	"X,P,pub-str",	"i,input",	"T",			NULL);

cleanup:

	return res;
}

static int generate_file_name(PARAM_SET *set, ERR_TRCKR *err, const char *in_flags, const char *out_flags, int i, char *buf, size_t buf_len) {
	int res = KT_UNKNOWN_ERROR;
	char *in_file_name = NULL;
	int in_count = 0;
	size_t count = 0;
	VARIABLE_IS_NOT_USED(out_flags);
	VARIABLE_IS_NOT_USED(err);

	res = PARAM_SET_getValueCount(set, in_flags, NULL, PST_PRIORITY_NONE, &in_count);
	if (res != PST_OK) goto cleanup;

	res = PARAM_SET_getStr(set, in_flags, NULL, PST_PRIORITY_NONE, i, &in_file_name);
	if (res != PST_OK) goto cleanup;

	if (strcmp(in_file_name, "-") == 0 && in_count == 1) {
		KSI_snprintf(buf, buf_len, "stdin.ext.ksig");
	} else if (SMART_FILE_hasFileExtension(in_file_name, "ksig")) {
		count += KSI_snprintf(buf + count, buf_len - count , "%s", in_file_name);
		count += KSI_snprintf(buf + count - 4, buf_len - count, "ext.ksig");
	} else {
		KSI_snprintf(buf, buf_len, "%s.ext.ksig", in_file_name);
	}

	res = KT_OK;

cleanup:

	return res;
}

static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err) {
	int res;

	res = get_pipe_out_error(set, err, NULL, "o", "dump");
	if (res != KT_OK) goto cleanup;

	res = get_pipe_out_error(set, err, NULL, "o,log", NULL);
	if (res != KT_OK) goto cleanup;

	res = get_pipe_in_error(set, err, "i", NULL, NULL);
	if (res != KT_OK) goto cleanup;

cleanup:
	return res;
}

static int check_other_input_param_errors(PARAM_SET *set, ERR_TRCKR *err) {
	int res;


	if (PARAM_SET_isSetByName(set, "replace-existing")) {
		int i_count = 0;
		int i = 0;

		if (PARAM_SET_isSetByName(set, "o")) {
			ERR_TRCKR_ADD(err, res = KT_INVALID_CMD_PARAM, "Error: --replace-existing can not be used with -o.");
			goto cleanup;
		}

		res = PARAM_SET_getValueCount(set, "i", NULL, PST_PRIORITY_NONE, &i_count);
		if (res != PST_OK) goto cleanup;

		for (i = 0; i < i_count; i++) {
			char *value = NULL;

			/* Note that only i is iterated and not input! input interpretes - as regular file. */
			res = PARAM_SET_getStr(set, "i", NULL, PST_PRIORITY_NONE, i, &value);
			if (res != PST_OK) goto cleanup;

			if (strcmp(value, "-") == 0) {
				ERR_TRCKR_ADD(err, res = KT_INVALID_CMD_PARAM, "Error: --replace-existing can not be used when extending the signature from stdin (-i -).");
				goto cleanup;
			}
		}
	}


	res = KT_OK;

cleanup:
	return res;
}

static int perform_extending(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, int task_id) {
	int res;
	int i = 0;
	int in_count = 0;
	COMPOSITE extra;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;
	int d = 0;
	int dump = 0;
	int how_to_save = 0;
	const char *mode = NULL;
	KSI_PolicyVerificationResult *result_ext = NULL;
	KSI_PolicyVerificationResult *result_sig = NULL;

	if (set == NULL || err == NULL || ksi == NULL || task_id > 2) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = PARAM_SET_getValueCount(set, "i,input", NULL, PST_PRIORITY_NONE, &in_count);
	if (res != PST_OK) goto cleanup;

	d = PARAM_SET_isSetByName(set, "d");
	dump = PARAM_SET_isSetByName(set, "dump");

	extra.ctx = ksi;
	extra.err = err;

	how_to_save = how_is_output_saved_to(set, "i,input", "o");

	res = get_smart_file_mode(err, how_to_save, &mode);
	if (res != KT_OK) goto cleanup;

	print_debug("Extending %d signatures.\n", in_count);
	for (i = 0; i < in_count; i++) {
		char *in_fname = NULL;
		const char *save_to = NULL;
		char buf[1024] = "";
		if (i > 0 && (d || dump)) print_debug(" ----------------------------\n");

		PARAM_SET_getStr(set, "i,input", NULL, PST_PRIORITY_NONE, i, &in_fname);
		print_debug("Extending signature '%s'.\n", in_fname);
		print_progressDesc(d, "Reading signature... ");

		res = PARAM_SET_getObjExtended(set, "i,input", NULL, PST_PRIORITY_NONE, (int)i, &extra, (void**)&sig);
		if (res != PST_OK) goto cleanup;
		print_progressResult(res);

		/* Make sure the signature is ok. */
		print_progressDesc(d, "Verifying old signature... ");
		res = KSITOOL_SignatureVerify_internally(err, sig, ksi, NULL, &result_sig);
		if (res != KSI_OK) {
			if (result_sig != NULL) {
				ERR_TRCKR_ADD(err, res, "Error: [%s] %s", OBJPRINT_getVerificationErrorCode(result_sig->finalResult.errorCode),
					OBJPRINT_getVerificationErrorDescription(result_sig->finalResult.errorCode));
			}
			ERR_TRCKR_ADD(err, res, "Error: Unable to verify signature.");
			goto cleanup;
		}
		print_progressResult(res);

		switch(task_id) {
			case 0:
				res = extend_to_nearest_publication(set, err, ksi, sig, &ext);
			break;

			case 1:
				res = extend_to_specified_time(set, err, ksi, &extra, sig, &ext);
			break;

			case 2:
				res = extend_to_specified_publication(set, err, ksi, sig, &ext);
			break;
		}
		if (res != KT_OK) goto cleanup;

		save_to = get_output_file_name(set, err, "i,input", "o", how_to_save, i, buf, sizeof(buf), generate_file_name);

		res = verify_and_save(set, err, ksi, ext, save_to, mode, NULL, &result_ext);
		if (res != KT_OK) goto cleanup;

		if (PARAM_SET_isSetByName(set, "dump")) {
			print_result("\n");
			print_result("=== Old signature ===\n");
			OBJPRINT_signatureDump(sig, print_result);
			print_result("\n");
			print_result("=== Extended signature ===\n");
			OBJPRINT_signatureDump(ext, print_result);
			print_result("\n");
			print_result("=== Extended signature verification ===\n");
			OBJPRINT_signatureVerificationResultDump(result_ext , print_result);
		}

		KSI_Signature_free(sig);
		KSI_Signature_free(ext);
		KSI_PolicyVerificationResult_free(result_ext);
		KSI_PolicyVerificationResult_free(result_sig);
		result_sig = NULL;
		result_ext = NULL;
		ext = NULL;
		sig = NULL;
	}


	res = KT_OK;
	goto cleanup;

cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		if (ERR_TRCKR_getErrCount(err) == 0) {ERR_TRCKR_ADD(err, res, NULL);}
		KSITOOL_KSI_ERRTrace_LOG(ksi);
		print_debug("\n");
		if (ext == NULL) {
			DEBUG_verifySignature(ksi, res, sig, result_sig, NULL);
		} else {
			print_debug("=== Old signature ===\n");
			DEBUG_verifySignature(ksi, res, sig, NULL, NULL);
			print_debug("=== Extended signature ===\n");
			DEBUG_verifySignature(ksi, res, ext, result_ext, NULL);
		}
	}

	KSI_PolicyVerificationResult_free(result_ext);
	KSI_PolicyVerificationResult_free(result_sig);
	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
	return res;
}