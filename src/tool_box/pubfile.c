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
#include <ksi/net.h>
#include <ksi/hashchain.h>
#include <ksi/compatibility.h>
#include "param_set/param_set.h"
#include "param_set/task_def.h"
#include "param_set/strn.h"
#include "tool_box/ksi_init.h"
#include "tool_box/param_control.h"
#include "tool_box/task_initializer.h"
#include "tool_box.h"
#include "smart_file.h"
#include "err_trckr.h"
#include "api_wrapper.h"
#include "printer.h"
#include "debug_print.h"
#include "obj_printer.h"
#include "conf_file.h"
#include "tool.h"

static int pubfile_task(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, int id, KSI_PublicationsFile **pubfile);
static int pubfile_create_pub_string(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, COMPOSITE *extra);
static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set);
static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err);

#define PARAMS "{o}{d}{v}{T}{conf}{dump}{log}{h|help}"


int pubfile_run(int argc, char** argv, char **envp) {
	int res;
	TASK *task = NULL;
	TASK_SET *task_set = NULL;
	PARAM_SET *set = NULL;
	KSI_PublicationsFile *pubfile = NULL;
	KSI_CTX *ksi = NULL;
	SMART_FILE *logfile = NULL;
	ERR_TRCKR *err = NULL;
	COMPOSITE extra;
	char buf[2048];
	int d = 0;
	int id;

	/**
	 * Extract command line parameters.
	 */
	res = PARAM_SET_new(
			CONF_generate_param_set_desc(PARAMS, "XP", buf, sizeof(buf)),
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

	d = PARAM_SET_isSetByName(set, "d");

	res = TOOL_init_ksi(set, &ksi, &err, &logfile);
	if (res != KT_OK) goto cleanup;

	res = check_pipe_errors(set, err);
	if (res != KT_OK) goto cleanup;

	extra.ctx = ksi;
	extra.err = err;

	switch(id = TASK_getID(task)) {
		case 0:
		case 1:
		case 2:
			res = pubfile_task(set, err, ksi, id, &pubfile);
		break;
		case 3:
			res = pubfile_create_pub_string(set, err, ksi, &extra);
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
		if (ERR_TRCKR_getErrCount(err) == 0) {ERR_TRCKR_ADD(err, res, NULL);}
		KSITOOL_KSI_ERRTrace_LOG(ksi);
		print_debug("\n");
		DEBUG_verifyPubfile(ksi, set, res, pubfile);
	}
	ERR_TRCKR_print(err, d);

	KSI_PublicationsFile_free(pubfile);
	SMART_FILE_close(logfile);
	PARAM_SET_free(set);
	TASK_SET_free(task_set);
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return KSITOOL_errToExitCode(res);
}
char *pubfile_help_toString(char *buf, size_t len) {
	int res;
	char *ret = NULL;
	PARAM_SET *set;
	size_t count = 0;
	char tmp[1024];

	res = PARAM_SET_new(CONF_generate_param_set_desc(PARAMS, "XP", tmp, sizeof(tmp)), &set);
	if (res != PST_OK) goto cleanup;

	res = CONF_initialize_set_functions(set, "XP");
	if (res != PST_OK) goto cleanup;



	PARAM_SET_setHelpText(set, "o", "<pubfile.bin>", "Output file path to store publications file. Use '-' as file name to redirect publications file binary stream to stdout. Publications file is always verified before saving.");
	PARAM_SET_setHelpText(set, "T", "<time>", "Time to create a publication string for as the number of seconds since 1970-01-01 00:00:00 UTC or time string formatted as \"YYYY-MM-DD hh:mm:ss\".");
	PARAM_SET_setHelpText(set, "cnstr", "<oid=value>", "OID of the PKI certificate field (e.g. e-mail address) and the expected value to qualify the certificate for verification of publications file PKI signature. At least one constraint must be defined.");
	PARAM_SET_setHelpText(set, "v", NULL, "Perform publications file verification. Note that when -o is used to save publications file, the verification is performed implicitly.");
	PARAM_SET_setHelpText(set, "dump", NULL, "Dump publications file in human-readable format to stdout. Without any extra flags publications file verification is not performed.");
	PARAM_SET_setHelpText(set, "d", NULL, "Print detailed information about processes and errors to stderr.");
	PARAM_SET_setHelpText(set, "conf", "<file>", "Read configuration options from given file. It must be noted that configuration options given explicitly on command line will override the ones in the configuration file.");
	PARAM_SET_setHelpText(set, "log", "<file>", "Write libksi log to given file. Use '-' as file name to redirect log to stdout.");


	count += PST_snhiprintf(buf + count, len - count, 80, 0, 0, NULL, ' ', "Usage:\\>1\n"
				"ksi pubfile -P <URL> --dump [-d]\n\\>8"
				"ksi pubfile -P <URL> -v --cnstr <oid=value>... [-V <file>]... [-W <file>]...\n"
				"[-d] [more_options]\\>1\n\\>8"
				"ksi pubfile -P <URL> -o <pubfile.bin> --cnstr <oid=value>... [-V <file>]...\\>8\n"
				"[-W <dir>]... [-d] [more_options]\\>1\n"
				"ksi pubfile -T <time> -X <URL> [--ext-user <user> --ext-key <key>]\n\n\n");

	ret = PARAM_SET_helpToString(set, "P,cnstr,v,V,o,X,ext-user,ext-key,ext-hmac-alg,T,d,dump,conf,log", 1, 13, 80, buf + count, len - count);




cleanup:
	if (res != PST_OK || ret == NULL) {
		PST_snprintf(buf + count, len - count, "\nError: There were failures while generating help by PARAM_SET.\n");
	}
	PARAM_SET_free(set);
	return buf;
}

const char *pubfile_get_desc(void) {
	return "Downloads and verifies KSI publications file.";
}

static int pubfile_task(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, int id, KSI_PublicationsFile **pubfile) {
	int res;
	int d = 0;
	int dump = 0;
	char *save_to = NULL;
	KSI_PublicationsFile *tmp = NULL;
	KSI_Integer *pubTime = NULL;
	KSI_PublicationRecord *pubRec = NULL;
	KSI_PublicationData *pubData = NULL;
	char buf[1024];

	if (set == NULL || ksi == NULL || err == NULL || pubfile == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	dump = PARAM_SET_isSetByName(set, "dump");
	d = PARAM_SET_isSetByName(set, "d");

	res = PARAM_SET_getObj(set, "o", NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, (void**)&save_to);
	if (res != PST_OK && res != PST_PARAMETER_EMPTY) goto cleanup;

	/**
	 * Retrieve the publications file and set the output variable for debugging.
	 * Extract the latest publication time.
	 */
	print_progressDesc(d, "%s", getPublicationsFileRetrieveDescriptionString(set));
	res = KSITOOL_receivePublicationsFile(err, ksi, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to get publications file.");
	print_progressResult(res);

	*pubfile = tmp;

	print_progressDesc(d, "Extracting latest publication time... ");
	res = KSI_PublicationsFile_getLatestPublication(tmp, NULL, &pubRec);
	ERR_CATCH_MSG(err, res, "Error: Unable to extract publication record.");
	res = KSI_PublicationRecord_getPublishedData(pubRec, &pubData);
	ERR_CATCH_MSG(err, res, "Error: Unable to extract publication data.");
	res = KSI_PublicationData_getTime(pubData, &pubTime);
	ERR_CATCH_MSG(err, res, "Error: Unable to extract publication time.");
	print_progressResult(res);

	print_debug("Latest publication (%llu) %s+00:00.\n",
			KSI_Integer_getUInt64(pubTime),
			KSI_Integer_toDateString(pubTime, buf, sizeof(buf)));

	/**
	 * Perform the verification if output is defined or verification is insisted.
	 */
	if (id != 1) {
		print_progressDesc(d, "Verifying publications file... ");
		res = KSITOOL_verifyPublicationsFile(err, ksi, tmp);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify publications file.");
		print_progressResult(res);
	}

	if (id == 2 && save_to != NULL) {
		int isStdout = strcmp(save_to, "-") == 0;
		const char *mode = isStdout ? "wbs" : "wb";

		print_progressDesc(d, "Saving publications file... ");
		res = KSI_OBJ_savePublicationsFile(err, ksi, tmp, mode, save_to);
		ERR_CATCH_MSG(err, res, "Error: Unable to save publications file.");
		print_progressResult(res);

		if (isStdout) print_debug("Publications file saved to stdout.\n");
		else print_debug("Publications file saved to '%s'.\n", save_to);
	}


	res = KT_OK;

cleanup:
	if (dump && tmp != NULL) {
		OBJPRINT_publicationsFileDump(tmp, print_result);
	}
	print_progressResult(res);

	return res;
}

static int pubfile_create_pub_string(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, COMPOSITE *extra) {
	int res;
	int d;
	KSI_Integer *start = NULL;
	KSI_Integer *end = NULL;
	KSI_Integer *reqID = NULL;
	KSI_ExtendReq *extReq = NULL;
	KSI_RequestHandle *request = NULL;
	KSI_ExtendResp *extResp = NULL;
	KSI_Integer *respStatus = NULL;
	KSI_CalendarHashChain *chain = NULL;
	KSI_DataHash *extHsh = NULL;
	KSI_PublicationData *tmpPubData = NULL;
	KSI_Integer *pubTime = NULL;
	char buf[1024];


	if (set == NULL || ksi == NULL || err == NULL || extra  == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	d = PARAM_SET_isSetByName(set, "d");

	res = PARAM_SET_getObjExtended(set, "T", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&start);
	ERR_CATCH_MSG(err, res, "Error: Unable to extract the time value to create the publication string for.");
	end = KSI_Integer_ref(start);

	print_progressDesc(d, "Sending extend request to %s (%llu)... ",
			KSI_Integer_toDateString(start, buf, sizeof(buf)),
			KSI_Integer_getUInt64(start));

	srand((unsigned)time(NULL));
	res = KSI_Integer_new(ksi, (KSI_uint64_t)rand(), &reqID);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	res = KSI_ExtendReq_new(ksi, &extReq);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	res = KSI_ExtendReq_setAggregationTime(extReq, start);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	start = NULL;
	res = KSI_ExtendReq_setPublicationTime(extReq, end);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	end = NULL;

	res = KSI_ExtendReq_setRequestId(extReq, reqID);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	res = KSI_sendExtendRequest(ksi, extReq, &request);
	ERR_CATCH_MSG(err, res, "Error: Unable to send extend request.");
	res = KSI_RequestHandle_perform(request);
	ERR_CATCH_MSG(err, res, "Error: Unable to send extend request.");

	res = KSITOOL_RequestHandle_getExtendResponse(err, ksi, request, &extResp);
	ERR_APPEND_KSI_ERR(err, res, KSI_NETWORK_ERROR);
	ERR_CATCH_MSG(err, res, "Error: Unable to get extend response.");
	res = KSI_ExtendResp_getStatus(extResp, &respStatus);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));

	if (respStatus == NULL || !KSI_Integer_equalsUInt(respStatus, 0)) {
		KSI_Utf8String *errm = NULL;
		res = KSI_ExtendResp_getErrorMsg(extResp, &errm);
		if (res == KSI_OK && KSI_Utf8String_cstr(errm) != NULL) {
			KSI_Integer *extender_status = NULL;

			res = KSI_ExtendResp_getStatus(extResp, &extender_status);
			ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));

			ERR_TRCKR_ADD(err, res = KSI_convertExtenderStatusCode(extender_status), "Extender returned error %llu: '%s'.", KSI_Integer_getUInt64(respStatus), KSI_Utf8String_cstr(errm));
		}else{
			ERR_TRCKR_ADD(err, res, "Extender returned error %llu.", (unsigned long long)KSI_Integer_getUInt64(respStatus));
		}
	}
	if (res != KT_OK) goto cleanup;

	print_progressResult(res);


	print_progressDesc(1, "Getting publication string... ");

	res = KSI_ExtendResp_getCalendarHashChain(extResp, &chain);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	res = KSI_CalendarHashChain_aggregate(chain, &extHsh);
	ERR_CATCH_MSG(err, res, "Error: Unable to aggregate calendar hash chain.");
	res = KSI_CalendarHashChain_getPublicationTime(chain, &pubTime);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	res = KSI_PublicationData_new(ksi, &tmpPubData);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	res = KSI_PublicationData_setImprint(tmpPubData, extHsh);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	res = KSI_PublicationData_setTime(tmpPubData, pubTime);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));
	res = KSI_CalendarHashChain_setPublicationTime(chain, NULL);
	ERR_CATCH_MSG(err, res, "Error: %s", KSITOOL_errToString(res));

	print_progressResult(res);

	print_result("%s\n", KSITOOL_PublicationData_toString(tmpPubData, buf,sizeof(buf)));


cleanup:
	print_progressResult(res);

	KSI_Integer_free(start);
	KSI_Integer_free(end);
	KSI_ExtendReq_free(extReq);
	KSI_ExtendResp_free(extResp);
	KSI_RequestHandle_free(request);
	KSI_PublicationData_free(tmpPubData);

	return res;
}

static int generate_tasks_set(PARAM_SET *set, TASK_SET *task_set) {
	int res;

	if (set == NULL || task_set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Configure parameter set, check, repair and object extractor function.
	 */
	res = CONF_initialize_set_functions(set, "XP");
	if (res != KT_OK) goto cleanup;

	PARAM_SET_addControl(set, "{conf}", isFormatOk_inputFile, isContentOk_inputFileRestrictPipe, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{o}{log}", isFormatOk_path, NULL, convertRepair_path, NULL);
	PARAM_SET_addControl(set, "{T}", isFormatOk_utcTime, isContentOk_utcTime, NULL, extract_utcTime);
	PARAM_SET_addControl(set, "{d}{v}{dump}", isFormatOk_flag, NULL, NULL, NULL);

	/**
	 * Define possible tasks.
	 */
	/*					  ID	DESC										MAN				ATL		FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Verify publications file.",				"P,cnstr,v",	NULL,	"T,o",		NULL);
	TASK_SET_add(task_set, 1,	"Dump publications file.",					"P,dump",		NULL,	"T,o,v",	NULL);
	TASK_SET_add(task_set, 2,	"Download and Verify publications file.",	"P,o,cnstr",	NULL,	"T",		NULL);
	TASK_SET_add(task_set, 3,	"Create publication string.",				"T,X",			NULL,	"o,dump,v",	NULL);

cleanup:

	return res;
}


static int check_pipe_errors(PARAM_SET *set, ERR_TRCKR *err) {
	int res;

	res = get_pipe_out_error(set, err, NULL, "o,log", NULL);
	if (res != KT_OK) goto cleanup;

cleanup:
	return res;
}
