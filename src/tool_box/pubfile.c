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
#include <ksi/net.h>
#include <ksi/hashchain.h>
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

static int pubfile_download(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi);
static int pubfile_create_pub_string(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, COMPOSITE *extra);

int pubfile_run(int argc, char** argv, char **envp) {
	int res;
	TASK *task = NULL;
	TASK_SET *task_set = NULL;
	PARAM_SET *set = NULL;
	KSI_CTX *ksi = NULL;
	FILE *file = NULL;
	ERR_TRCKR *err = NULL;
	COMPOSITE extra;
	char buf[2048];
	int d;

	/**
	 * Extract command line parameters.
     */
	res = PARAM_SET_new("{verify}{o}{d}{v}{x}{T}{ver-int}{ver-cal}{ver-key}{ver-pub}"
			DEF_SERVICE_PAR DEF_PARAMETERS
			, &set);
	if (res != KT_OK) {
		goto cleanup;
	}

	/**
	 * Configure parameter set, control, repair and object extractor function.
     */

	PARAM_SET_addControl(set, "{o}", isPathFormOk, isOutputFileContOK, NULL, NULL);
	PARAM_SET_addControl(set, "{X}{P}{S}", isFormatOk_url, NULL, convertRepair_url, NULL);
	PARAM_SET_addControl(set, "{aggre-user}{aggre-pass}{ext-pass}{ext-user}", isUserPassFormatOK, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{T}", isFormatOk_utcTime, isContentOk_utcTime, NULL, extract_utcTime);
	PARAM_SET_addControl(set, "{d}", isFlagFormatOK, NULL, NULL, NULL);
	PARAM_SET_addControl(set, "{cnstr}", isConstraintFormatOK, NULL, convert_replaceWithOid, NULL);

	/**
	 * Define possible tasks.
     */
	res = TASK_SET_new(&task_set);
	if (res != PST_OK) goto cleanup;

	/*					  ID	DESC										MAN			ATL		FORBIDDEN	IGN	*/
	TASK_SET_add(task_set, 0,	"Dump publications file.",					"P,d",		NULL,	"T,o",		NULL);
	TASK_SET_add(task_set, 1,	"Save publications.",						"P,o",		NULL,	"T,d",		NULL);
	TASK_SET_add(task_set, 2,	"Save and dump publications file.",			"P,o,d",	NULL,	"T",		NULL);
	TASK_SET_add(task_set, 3,	"Create publication string.",				"T,X",		NULL,	NULL,		NULL);


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
	res = TASK_SET_analyzeConsistency(task_set, set, 0.5);
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

	switch(TASK_getID(task)) {
		case 0:
		case 1:
		case 2:
			res = pubfile_download(set, err, ksi);
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
	ERR_TRCKR_free(err);
	KSI_CTX_free(ksi);

	return errToExitCode(res);
}
char *pubfile_help_toString(char*buf, size_t len) {
	size_t count = 0;

	count += snprintf(buf + count, len - count,
		"Usage:\n"
		"ksitool pubfile -P <url> [--cnstr <oid=value>]... [-V <file>]...\n"
		"        [-W <file>]... [-o <pubfile.bin>] [-d] [-v] [more options]\n"
		"ksitool pubfile -T <time> -X <url> [--ext-user <user> --ext-pass <pass>]\n\n"

		"-P <url>  - specify publications file URL (or file with uri scheme 'file://').\n"
		"--cnstr <oid=value>\n"
		"          - publications file certificate verification constraints.\n"
		"-o <file> - output file name to store publications file.\n"
		"-v        - perform publications file verification."
		"-X <url>  - specify extending service URL.\n"
		"--ext-user <str>\n"
		"          - user name for extending service.\n"
		"--ext-pass <str>\n"
		"          - password for extending service.\n"
		"-T <time> - specify time to create a publication string for as the number of seconds\n"
		"            since 1970-01-01 00:00:00 UTC or time string formatted as \"YYYY-MM-DD hh:mm:ss\".\n"
	);

	return buf;
}
const char *pubfile_get_desc(void) {return "KSI general publications file tool.";}

static int pubfile_download(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi) {
	int res;
	KSI_PublicationsFile *pubfile = NULL;
	int d;
	int v;
	char *save_to = NULL;

	if (set == NULL || ksi == NULL || err == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	d = PARAM_SET_isSetByName(set, "d");
	v = PARAM_SET_isSetByName(set, "v");
	PARAM_SET_getObj(set, "o", NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, (void**)&save_to);

	print_progressDesc(d, "Downloading publications file... ");
	res = KSITOOL_receivePublicationsFile(err, ksi, &pubfile);
	ERR_CATCH_MSG(err, res, "Error: Unable to get publication file.");
	print_progressResult(res);

	if (d) {
		OBJPRINT_publicationsFileReferences(pubfile);
		OBJPRINT_publicationsFileCertificates(pubfile);
		OBJPRINT_publicationsFileSigningCert(pubfile);
	}

	if (v) {
		print_progressDesc(d, "Verifying publications file... ");
		res = KSITOOL_verifyPublicationsFile(err, ksi, pubfile);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify publication file.");
		print_progressResult(res);
	}

	if (save_to != NULL) {
		print_progressDesc(d, "Saveing publications file... ");
		res = savePublicationFile(err, ksi, pubfile, save_to);
		ERR_CATCH_MSG(err, res, "Error: Unable to save publications file.");
		print_progressResult(res);
	}

	res = KT_OK;

cleanup:
	print_progressResult(res);

	if (res != KT_OK) {
//		DEBUG_verifyPubfile();
	}

	return res;
}

static int pubfile_create_pub_string(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ksi, COMPOSITE *extra) {
	int res;

	KSI_Integer *end = NULL;
	KSI_Integer *start = NULL;
	KSI_Integer *reqID = NULL;
	KSI_ExtendReq *extReq = NULL;
	KSI_RequestHandle *request = NULL;
	KSI_ExtendResp *extResp = NULL;
	KSI_Integer *respStatus = NULL;
	KSI_CalendarHashChain *chain = NULL;
	KSI_DataHash *extHsh = NULL;
	KSI_PublicationData *tmpPubData = NULL;
	KSI_Integer *pubTime = NULL;
	int d;
	int publicationTime = 0;
	char buf[1024];


	if (set == NULL || ksi == NULL || err == NULL || extra  == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	d = PARAM_SET_isSetByName(set, "t");
//	PARAM_SET_getIntValue(set, "T", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &start);
	res = PARAM_SET_getObjExtended(set, "T", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, extra, (void**)&start);
	if (res != KT_OK) goto cleanup;
	end = KSI_Integer_ref(start);

	print_progressDesc(d, "Sending extend request... ");

	res = KSI_Integer_new(ksi, (KSI_uint64_t)start, &reqID);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_ExtendReq_new(ksi, &extReq);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_ExtendReq_setAggregationTime(extReq, start);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	start = NULL;
	res = KSI_ExtendReq_setPublicationTime(extReq, end);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	end = NULL;

	res = KSI_ExtendReq_setRequestId(extReq, reqID);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_sendExtendRequest(ksi, extReq, &request);
	ERR_CATCH_MSG(err, res, "Error: Unable to send extend request.");
	res = KSI_RequestHandle_perform(request);
	ERR_CATCH_MSG(err, res, "Error: Unable to send extend request.");

	res = KSITOOL_RequestHandle_getExtendResponse(err, ksi, request, &extResp);
	ERR_APPEND_KSI_ERR(err, res, KSI_NETWORK_ERROR);
	ERR_CATCH_MSG(err, res, "Error: Unable to get extend response.");
	res = KSI_ExtendResp_getStatus(extResp, &respStatus);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));

	if (respStatus == NULL || !KSI_Integer_equalsUInt(respStatus, 0)) {
		KSI_Utf8String *errm = NULL;
		res = KSI_ExtendResp_getErrorMsg(extResp, &errm);
		if (res == KSI_OK && KSI_Utf8String_cstr(errm) != NULL) {
			ERR_TRCKR_ADD(err, res, "Extender returned error %llu: '%s'.", (unsigned long long)KSI_Integer_getUInt64(respStatus), KSI_Utf8String_cstr(errm));
		}else{
			ERR_TRCKR_ADD(err, res, "Extender returned error %llu.", (unsigned long long)KSI_Integer_getUInt64(respStatus));
		}
	}

	print_progressResult(res);


	print_progressDesc(false, "Getting publication string... ");

	res = KSI_ExtendResp_getCalendarHashChain(extResp, &chain);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_CalendarHashChain_aggregate(chain, &extHsh);
	ERR_CATCH_MSG(err, res, "Error: Unable to aggregate calendar hash chain.");
	res = KSI_CalendarHashChain_getPublicationTime(chain, &pubTime);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_PublicationData_new(ksi, &tmpPubData);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_PublicationData_setImprint(tmpPubData, extHsh);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_PublicationData_setTime(tmpPubData, pubTime);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_CalendarHashChain_setPublicationTime(chain, NULL);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));

	print_progressResult(res);
	print_info("\n");

	print_result("%s\n", KSI_PublicationData_toString(tmpPubData, buf,sizeof(buf)));


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