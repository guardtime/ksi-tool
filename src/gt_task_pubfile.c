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

#include "gt_task_support.h"
#include "ksi/net.h"
#include "ksi/hashchain.h"

static int GT_publicationsFileTask_downloadPublicationsFile(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_PublicationsFile **pubfile);
static int GT_publicationsFileTask_createPublicationString(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, char **pubstring, time_t *time);

int GT_publicationsFileTask(Task *task){
	int res;
	paramSet *set = NULL;
	KSI_CTX *ksi = NULL;
	ERR_TRCKR *err = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	bool d, r;
	char *outPubFileName = NULL;
	char *pubstring = NULL;
	char strTime[1024];
	time_t pubTm;
	struct tm tm;
	int retval = EXIT_SUCCESS;


	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "o",0, &outPubFileName);
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");


	res = initTask(task ,&ksi, &err);
	if (res != KT_OK) goto cleanup;

	if(Task_getID(task) == downloadPublicationsFile){
		res = GT_publicationsFileTask_downloadPublicationsFile(task, ksi, err, &publicationsFile);
		if (res != KT_OK) goto cleanup;

		res = savePublicationFile(err, ksi, publicationsFile, outPubFileName);
		if (res != KT_OK) goto cleanup;
		print_info("Publications file '%s' saved.\n", outPubFileName);

		if(d || r) print_info("\n");
		if(d || r) printPublicationsFileReferences(publicationsFile);
		if(d) printPublicationsFileCertificates(publicationsFile);
	} else if(Task_getID(task) == createPublicationString){
		res = GT_publicationsFileTask_createPublicationString(task, ksi, err, &pubstring, &pubTm);
		if (res != KT_OK) goto cleanup;

		gmtime_r(&pubTm, &tm);
		strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", &tm);

		print_result("[%s]\n", strTime);
		print_result("pub=%s\n", pubstring);
	}



cleanup:

	if (res != KT_OK) {
		print_errors("failed.\n");
		ERR_TRCKR_printErrors(err);
		retval = errToExitCode(res);
	}

	KSI_free(pubstring);
	ERR_TRCKR_free(err);
	closeTask(ksi);

	return retval;
}


static int GT_publicationsFileTask_downloadPublicationsFile(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_PublicationsFile **pubfile) {
	int res;
	paramSet *set = NULL;
	KSI_PublicationsFile *tmp = NULL;
	bool d, t, r;


	if (task == NULL || ksi == NULL || err == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	set = Task_getSet(task);
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");
	t = paramSet_isSetByName(set, "t");


	print_info("Downloading publications file... ");
	MEASURE_TIME(res = KSI_receivePublicationsFile(ksi, &tmp));
	ERR_CATCH_MSG(err, res, "Error: Unable to get publication file.");
	print_info("ok. %s\n",t ? str_measuredTime() : "");

	print_info("Verifying publications file... ");
	MEASURE_TIME(res = KSI_verifyPublicationsFile(ksi, tmp));
	ERR_CATCH_MSG(err, res, "Error: Unable to verify publication file.");
	print_info("ok. %s\n",t ? str_measuredTime() : "");

	if (pubfile != NULL) {
		*pubfile = tmp;
	}

	res = KT_OK;

cleanup:

	return res;
}

static int GT_publicationsFileTask_createPublicationString(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, char **pubstring, time_t *time) {
	paramSet *set = NULL;
	int res;
	KSI_PublicationsFile *publicationsFile = NULL;

	KSI_Integer *end = NULL;
	KSI_Integer *start = NULL;
	KSI_Integer *reqID = NULL;
	KSI_ExtendReq *extReq = NULL;
	KSI_RequestHandle *request = NULL;
	KSI_ExtendResp *extResp = NULL;
	KSI_Integer *respStatus = NULL;
	KSI_CalendarHashChain *chain = NULL;
	KSI_DataHash *extHsh = NULL;
	KSI_PublicationData *pubData = NULL;
	KSI_Integer *pubTime = NULL;
	char *base32 = NULL;
	bool d, t, r;
	int publicationTime = 0;


	if (task == NULL || ksi == NULL || err == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	set = Task_getSet(task);
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");
	t = paramSet_isSetByName(set, "t");
	paramSet_getIntValueByNameAt(set,"T",0,&publicationTime);

	print_info("Sending extend request... ");
	measureLastCall();

	res = KSI_Integer_new(ksi, publicationTime, &start);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_Integer_new(ksi, publicationTime, &end);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
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

	res = KSI_RequestHandle_getExtendResponse(request, &extResp);
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

	measureLastCall();
	print_info("ok. %s\n",t ? str_measuredTime() : "");


	print_info("Getting publication string... ");

	res = KSI_ExtendResp_getCalendarHashChain(extResp, &chain);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_CalendarHashChain_aggregate(chain, &extHsh);
	ERR_CATCH_MSG(err, res, "Error: Unable to aggregate calendar hash chain.");
	res = KSI_CalendarHashChain_getPublicationTime(chain, &pubTime);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_PublicationData_new(ksi, &pubData);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_PublicationData_setImprint(pubData, extHsh);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_PublicationData_setTime(pubData, pubTime);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_CalendarHashChain_setPublicationTime(chain, NULL);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
	res = KSI_PublicationData_toBase32(pubData, &base32);
	ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));

	print_info("ok\n\n");

	if (pubstring != NULL) {
		*pubstring = base32;
		base32 = NULL;
	}

	if (time != NULL) {
		*time = (time_t)KSI_Integer_getUInt64(pubTime);
	}


cleanup:

	KSI_Integer_free(start);
	KSI_Integer_free(end);
	KSI_ExtendReq_free(extReq);
	KSI_ExtendResp_free(extResp);
	KSI_RequestHandle_free(request);
	KSI_PublicationData_free(pubData);
	KSI_free(base32);

	return res;
}

