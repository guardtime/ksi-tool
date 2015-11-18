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
#include "obj_printer.h"
#include "ksi/net.h"
#include "ksi/hashchain.h"

static int GT_publicationsFileTask_downloadPublicationsFile(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_PublicationsFile **pubfile);
static int GT_publicationsFileTask_createPublicationString(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_PublicationData **pubData);
static int GT_publicationsFileTask_dumpPublicationsFile(Task *task, KSI_CTX *ksi, ERR_TRCKR *err);

int GT_publicationsFileTask(Task *task){
	int res;
	paramSet *set = NULL;
	KSI_CTX *ksi = NULL;
	ERR_TRCKR *err = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	bool d, r;
	char *outPubFileName = NULL;
	char *pubstring = NULL;
	KSI_PublicationData *pubData = NULL;
	char buf[1024];
	int retval = EXIT_SUCCESS;


	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "o",0, &outPubFileName);
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");


	res = initTask(task ,&ksi, &err);
	if (res != KT_OK) goto cleanup;

	if(Task_getID(task) == downloadPublicationsFile) {
		res = GT_publicationsFileTask_downloadPublicationsFile(task, ksi, err, &publicationsFile);
		if (res != KT_OK) goto cleanup;

		res = savePublicationFile(err, ksi, publicationsFile, outPubFileName);
		if (res != KT_OK) goto cleanup;
		print_info("Publications file '%s' saved.\n", outPubFileName);

		if(d || r) print_info("\n");
		if(d || r) OBJPRINT_publicationsFileReferences(publicationsFile);
		if(d) OBJPRINT_publicationsFileCertificates(publicationsFile);
	} else if(Task_getID(task) == createPublicationString){
		res = GT_publicationsFileTask_createPublicationString(task, ksi, err, &pubData);
		if (res != KT_OK) goto cleanup;

		if(KSI_PublicationData_toString(pubData, buf,sizeof(buf)) != NULL) {
				print_result("%s\n", buf);
		}
	} else if (Task_getID(task) == dumpPublicationsFile) {
		res = GT_publicationsFileTask_dumpPublicationsFile(task, ksi, err);
		if (res != KSI_OK) goto cleanup;
	}



cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		ERR_TRCKR_printErrors(err);
		retval = errToExitCode(res);
	}

	KSI_free(pubstring);
	ERR_TRCKR_free(err);
	KSI_PublicationData_free(pubData);
	closeTask(ksi);

	return retval;
}


static int GT_publicationsFileTask_downloadPublicationsFile(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_PublicationsFile **pubfile) {
	int res;
	paramSet *set = NULL;
	KSI_PublicationsFile *tmp = NULL;
	bool t;


	if (task == NULL || ksi == NULL || err == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	set = Task_getSet(task);
	t = paramSet_isSetByName(set, "t");


	print_progressDesc(t, "Downloading publications file... ");
	res = KSITOOL_receivePublicationsFile(err, ksi, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to get publication file.");
	print_progressResult(res);

	print_progressDesc(t, "Verifying publications file... ");
	res = KSITOOL_verifyPublicationsFile(err, ksi, tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify publication file.");
	print_progressResult(res);

	if (pubfile != NULL) {
		*pubfile = tmp;
	}

	res = KT_OK;

cleanup:

	print_progressResult(res);

	return res;
}

static int GT_publicationsFileTask_createPublicationString(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_PublicationData **pubData) {
	paramSet *set = NULL;
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
	bool t;
	int publicationTime = 0;


	if (task == NULL || ksi == NULL || err == NULL || pubData == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	set = Task_getSet(task);
	t = paramSet_isSetByName(set, "t");
	paramSet_getIntValueByNameAt(set,"T",0,&publicationTime);

	print_progressDesc(t, "Sending extend request... ");

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

	*pubData = tmpPubData;
	tmpPubData = NULL;



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

static int GT_publicationsFileTask_dumpPublicationsFile(Task *task, KSI_CTX *ksi, ERR_TRCKR *err) {
	int res;
	KSI_PublicationsFile *pubfile = NULL;

	if (task == NULL || ksi == NULL || err == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	print_progressDesc(false, "Downloading publications file... ");
	res = KSITOOL_receivePublicationsFile(err, ksi, &pubfile);
	ERR_CATCH_MSG(err, res, "Error: Unable to get publication file.");
	print_progressResult(res);

	OBJPRINT_publicationsFileReferences(pubfile);
	OBJPRINT_publicationsFileCertificates(pubfile);
	OBJPRINT_publicationsFileSigningCert(pubfile);

cleanup:

	return res;
}
