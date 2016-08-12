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
#include <ksi/ksi.h>
#include <ksi/compatibility.h>
#include <ksi/policy.h>
#include "param_set/param_set.h"
#include "param_set/task_def.h"
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

static int extend_to_nearest_publication(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext);
static int extend_to_specified_time(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, COMPOSITE *extra, KSI_Signature *sig, KSI_Signature **ext);
static int extend_to_specified_publication(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext);
static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);
static char* get_output_file_name_if_not_defined(PARAM_SET *set, ERR_TRCKR *err, char *buf, size_t buf_len);
static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err);
static char* get_output_name(PARAM_SET *set, ERR_TRCKR *err, char *buf, size_t buf_len, char *mode);
static int verify_and_save(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *ext, const char *fname, char *mode, KSI_PublicationData *pub_data, KSI_PolicyVerificationResult **result);

int extend_run(int argc, char** argv, char **envp) {
	int res;
	TASK *task = NULL;
	TASK_SET *task_set = NULL;
	PARAM_SET *set = NULL;
	KSI_CTX *ksi = NULL;
	SMART_FILE *logfile = NULL;
	ERR_TRCKR *err = NULL;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;
	KSI_PublicationData *pub_data = NULL;
	KSI_PolicyVerificationResult *result_sig = NULL;
	KSI_PolicyVerificationResult *result_ext = NULL;
	COMPOSITE extra;
	char fnmae[2048];
	char mode[2];
	char buf[2048];
	int d = 0;

	/**
	 * Extract command line parameters.
	 */
	res = PARAM_SET_new(
			CONF_generate_param_set_desc("{i}{o}{d}{x}{T}{pub-str}{dump}{conf}{log}{h|help}", "XP", buf, sizeof(buf)),
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

	extra.ctx = ksi;
	extra.err = err;

	print_progressDesc(d, "Reading signature... ");
	res = PARAM_SET_getObjExtended(set, "i", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &extra, (void**)&sig);
	if (res != PST_OK) goto cleanup;
	print_progressResult(res);

	/* Make sure the signature is ok. */
	print_progressDesc(d, "Verifying old signature... ");
	res = KSITOOL_SignatureVerify_internally(err, sig, ksi, NULL, &result_sig);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature.");
	print_progressResult(res);

	if (get_output_name(set, err, fnmae, sizeof(fnmae), mode) == NULL) goto cleanup;

	switch(TASK_getID(task)) {
		case 0:
			res = extend_to_nearest_publication(set, err, ksi, sig, &ext);
		break;

		case 1:
			res = extend_to_specified_time(set, err, ksi, &extra, sig, &ext);
		break;

		case 2:
			res = extend_to_specified_publication(set, err, ksi, sig, &ext);
		break;

		default:
			res = KT_UNKNOWN_ERROR;
			goto cleanup;
		break;
	}

	if (res != KT_OK) goto cleanup;

	res = verify_and_save(set, err, ksi, ext, fnmae, mode, pub_data, &result_ext);
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

cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
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

		print_errors("\n");
		if (d) ERR_TRCKR_printExtendedErrors(err);
		else  ERR_TRCKR_printErrors(err);
	}

	SMART_FILE_close(logfile);
	PARAM_SET_free(set);
	TASK_SET_free(task_set);
	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
	KSI_PublicationData_free(pub_data);
	KSI_PolicyVerificationResult_free(result_ext);
	KSI_PolicyVerificationResult_free(result_sig);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return KSITOOL_errToExitCode(res);
}
char *extend_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		" %s extend -i <in.ksig> [-o <out.ksig>] -X <URL>\n"
		"    [--ext-user <user> --ext-key <key>] -P <URL> [--cnstr <oid=value>]... [more_options]\n"
		" %s extend -i <in.ksig> [-o <out.ksig>] -X <URL>\n"
		"    [--ext-user <user> --ext-key <key>] -P <URL> [--cnstr <oid=value>]... [--pub-str <str>] [more_options]\n"
		" %s extend -i <in.ksig> [-o <out.ksig>] -X <URL>\n"
		"    [--ext-user <user> --ext-key <key>] -T time [more_options]\n"
		"\n"
		" -i <in.ksig>\n"
		"           - File path to the KSI signature file to be extended. Use '-' as the\n"
		"             path to read the signature from stdin.\n"
		" -o <out.ksig>\n"
		"           - Output file path for the extended signature. Use '-' as the path to redirect\n"
		"             the signature binary stream to stdout. If not specified, the signature is saved\n"
		"             to <in.ksig>.ext.ksig (or <in.ksig>.ext_<nr>.ksig where <nr> is\n"
		"             auto-incremented counter if the output file already exists). If specified,\n"
		"             existing file is always overwritten.\n"
		" -X <URL>  - Extending service (KSI Extender) URL.\n"
		" --ext-user <user>\n"
		"           - Username for extending service.\n"
		" --ext-key <key>\n"
		"           - HMAC key for extending service.\n"
		" -P <URL>  - Publications file URL (or file with URI scheme 'file://').\n"
		" --cnstr <oid=value>\n"
		"           - OID of the PKI certificate field (e.g. e-mail address) and the expected\n"
		"             value to qualify the certificate for verification of publications file\n"
		"             PKI signature. At least one constraint must be defined.\n"
		" --pub-str <str>\n"
		"           - Publication record as publication string to extend the signature to.\n"
		" -T <time> - Publication time to extend to as the number of seconds since\n"
		"             1970-01-01 00:00:00 UTC or time string formatted as \"YYYY-MM-DD hh:mm:ss\".\n"
		"\n"
		"\n"
		" -V        - Certificate file in PEM format for publications file verification.\n"
		"             All values from lower priority source are ignored.\n"
		" -d        - Print detailed information about processes and errors.\n"
		" --dump    - Dump extended signature and verification info in human-readable format to stdout.\n"
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

static char* get_output_name(PARAM_SET *set, ERR_TRCKR *err, char *buf, size_t buf_len, char *mode) {
	int res;
	char *outSigFileName = NULL;

	if (set == NULL || err == NULL || buf == NULL || buf_len == 0 || mode == NULL) {
		return NULL;
	}

	buf[0] = '\0';

	if (!PARAM_SET_isSetByName(set, "o")) {
		mode[0] = 'i';
		mode[1] = '\0';

		outSigFileName = get_output_file_name_if_not_defined(set, err, buf, buf_len);
		if (outSigFileName == NULL) {
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Unable to generate output file name.");
			return NULL;
		}
	} else {
		res = PARAM_SET_getStr(set, "o", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &outSigFileName);
		if (res != PST_OK) return NULL;

		KSI_strncpy(buf, outSigFileName, buf_len);
	}

	return buf[0] == '\0' ? NULL : buf;
}

static int verify_and_save(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *ext, const char *fname, char *mode, KSI_PublicationData *pub_data, KSI_PolicyVerificationResult **result) {
	int res;
	char real_output_name[1024];
	int d;

	d = PARAM_SET_isSetByName(set, "d");

	print_progressDesc(d, "Verifying extended signature... ");
	res = KSITOOL_SignatureVerify_general(err, ext, ksi, NULL, pub_data, 1, result);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	print_progressResult(res);

	/* Save signature. */
	print_progressDesc(d, "Saving signature... ");
	res = KSI_OBJ_saveSignature(err, ksi, ext, mode, fname, real_output_name, sizeof(real_output_name));
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);

	print_debug("Signature saved to '%s'.\n", real_output_name);

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
		res = KT_INVALID_ARGUMENT;
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


	if (set == NULL || ksi == NULL || err == NULL || sig == NULL || extra == NULL) {
		res = KT_INVALID_ARGUMENT;
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
		res = KT_INVALID_ARGUMENT;
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
	PARAM_SET_addControl(set, "{i}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, extract_inputSignature);
	PARAM_SET_addControl(set, "{T}", isFormatOk_utcTime, isContentOk_utcTime, NULL, extract_utcTime);
	PARAM_SET_addControl(set, "{d}{dump}", isFormatOk_flag, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{pub-str}", isFormatOk_pubString, NULL, NULL, extract_pubString);

	/**
	 * Define possible tasks.
	 */
	/*					  ID	DESC												MAN					ATL		FORBIDDEN		IGN	*/
	TASK_SET_add(task_set, 0,	"Extend to the earliest available publication.",	"i,X,P",			NULL,	"T,pub-str",	NULL);
	TASK_SET_add(task_set, 1,	"Extend to the specified time.",					"i,X,T",			NULL,	"pub-str",		NULL);
	TASK_SET_add(task_set, 2,	"Extend to time specified in publications string.",	"i,X,P,pub-str",	NULL,	"T",			NULL);

cleanup:

	return res;
}

static char* get_output_file_name_if_not_defined(PARAM_SET *set, ERR_TRCKR *err, char *buf, size_t buf_len) {
	char *ret = NULL;
	int res;
	char *in_file_name = NULL;
	size_t count = 0;

	if (set == NULL || err == NULL || buf == NULL || buf_len == 0) goto cleanup;

	res = PARAM_SET_getStr(set, "i", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &in_file_name);
	if (res != PST_OK) goto cleanup;

	if (strcmp(in_file_name, "-") == 0) {
		KSI_snprintf(buf, buf_len, "stdin.ext.ksig");
	} else {
		if (SMART_FILE_hasFileExtension(in_file_name, "ksig")) {
			count += KSI_snprintf(buf + count, buf_len - count , "%s", in_file_name);
			count += KSI_snprintf(buf + count - 4, buf_len - count, "ext.ksig");
		} else {
			KSI_snprintf(buf, buf_len, "%s.ext.ksig", in_file_name);
		}
	}

	ret = buf;

cleanup:

	return ret;
}

static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err) {
	int res;

	res = get_pipe_out_error(set, err, "o", "dump");
	if (res != KT_OK) goto cleanup;

	res = get_pipe_out_error(set, err, "o,log", NULL);
	if (res != KT_OK) goto cleanup;

cleanup:
	return res;
}
