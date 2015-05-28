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
#include "try-catch.h"
#include "ksi/net.h"

void getPublicationString_throws(KSI_CTX *ctx, KSI_Integer *time);

int GT_publicationsFileTask(Task *task){
	paramSet *set = NULL;
	KSI_CTX *ksi = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	FILE *out = NULL;
	size_t bytesWritten;
	char *rawPubfile = NULL;
	unsigned rawLen = 0;

	KSI_Integer *end = NULL;
	KSI_Integer *start = NULL;
	KSI_Integer *reqID = NULL;
	KSI_ExtendReq *extReq = NULL;
	KSI_RequestHandle *request = NULL;
	KSI_ExtendResp *extResp = NULL;
	KSI_Integer *respStatus = NULL;
	KSI_CalendarHashChain *chain = NULL;
	KSI_DataHash *extHsh= NULL;
	KSI_PublicationData *pubData = NULL;
	KSI_Integer *pubTime = NULL;
	char *base32 = NULL;
	char strTime[1024];
	time_t pubTm;
	struct tm tm;

	int retval = EXIT_SUCCESS;

	bool d, t, r;
	char *outPubFileName = NULL;
	int publicationTime = 0;

	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "o",0,&outPubFileName);
	paramSet_getIntValueByNameAt(set,"T",0,&publicationTime);
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");
	t = paramSet_isSetByName(set, "t");

	/*Initalization of KSI */
	resetExceptionHandler();
	try
		CODE{
			initTask_throws(task ,&ksi);

			if(Task_getID(task) == downloadPublicationsFile){
				print_info("Downloading publications file... ");
				MEASURE_TIME(KSI_receivePublicationsFile_throws(ksi, &publicationsFile));
				print_info("ok. %s\n",t ? str_measuredTime() : "");

				print_info("Verifying publications file... ");
				MEASURE_TIME(KSI_verifyPublicationsFile_throws(ksi, publicationsFile));
				print_info("ok. %s\n",t ? str_measuredTime() : "");

				KSI_PublicationsFile_serialize(ksi, publicationsFile, &rawPubfile, &rawLen);
				out = fopen(outPubFileName, "wb");
				if(out == NULL) THROW_MSG(IO_EXCEPTION,EXIT_IO_ERROR, "Error: Unable to ope publications file '%s' for writing.\n", outPubFileName);

				bytesWritten = fwrite(rawPubfile, 1, rawLen, out);
				if(bytesWritten != rawLen) THROW_MSG(IO_EXCEPTION,EXIT_IO_ERROR, "Error: Unable to write publications file '%s'.\n", outPubFileName);
				print_info("Publications file '%s' saved.\n", outPubFileName);
			} else if(Task_getID(task) == createPublicationString){
				print_info("Sending extend request... ");
				measureLastCall();
				KSI_Integer_new_throws(ksi, publicationTime, &start);
				KSI_Integer_new_throws(ksi, publicationTime, &end);
				KSI_Integer_new_throws(ksi, (KSI_uint64_t)start, &reqID);
				KSI_ExtendReq_new_throws(ksi, &extReq);
				KSI_ExtendReq_setAggregationTime_throws(ksi, extReq, start);
				start = NULL;
				KSI_ExtendReq_setPublicationTime_throws(ksi, extReq, end);
				end = NULL;
				KSI_ExtendReq_setRequestId_throws(ksi, extReq, reqID);
				KSI_sendExtendRequest_throws(ksi, extReq, &request);

				KSI_RequestHandle_getExtendResponse_throws(ksi, request, &extResp);
				KSI_ExtendResp_getStatus_throws(ksi, extResp, &respStatus);

				if (respStatus == NULL || !KSI_Integer_equalsUInt(respStatus, 0)) {
					KSI_Utf8String *errm = NULL;
					int res = KSI_ExtendResp_getErrorMsg(extResp, &errm);
					if (res == KSI_OK && KSI_Utf8String_cstr(errm) != NULL) {
						THROW_MSG(KSI_EXCEPTION,EXIT_EXTEND_ERROR, "Extender returned error %llu: '%s'.\n", (unsigned long long)KSI_Integer_getUInt64(respStatus), KSI_Utf8String_cstr(errm));
					}else{
						THROW_MSG(KSI_EXCEPTION,EXIT_EXTEND_ERROR, "Extender returned error %llu.\n", (unsigned long long)KSI_Integer_getUInt64(respStatus));
					}
				}
				measureLastCall();
				print_info("ok. %s\n",t ? str_measuredTime() : "");


				print_info("Getting publication string... ");
				KSI_ExtendResp_getCalendarHashChain_throws(ksi, extResp, &chain);
				KSI_CalendarHashChain_aggregate_throws(ksi, chain, &extHsh);
				KSI_CalendarHashChain_getPublicationTime_throws(ksi, chain, &pubTime);
				KSI_PublicationData_new_throws(ksi, &pubData);
				KSI_PublicationData_setImprint_throws(ksi, pubData, extHsh);
				KSI_PublicationData_setTime_throws(ksi, pubData, pubTime);
				KSI_CalendarHashChain_setPublicationTime_throws(ksi, chain, NULL);
				KSI_PublicationData_toBase32_throws(ksi, pubData, &base32);
				print_info("ok\n\n");



				pubTm = (time_t)KSI_Integer_getUInt64(pubTime);
				gmtime_r(&pubTm, &tm);
				strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", &tm);

				print_result("[%s]\n", strTime);
				print_result("pub=%s\n", base32);
			}
		}
		CATCH(KSI_EXCEPTION){
				if(ksi)
					print_errors("failed.\n");
				printErrorMessage();
				retval = _EXP.exep.ret;
				exceptionSolved();
		}
		CATCH(IO_EXCEPTION){
				printErrorMessage();
				retval = _EXP.exep.ret;
				exceptionSolved();
		}
		CATCH(INVALID_ARGUMENT_EXCEPTION){
				printErrorMessage();
				retval = _EXP.exep.ret;
				exceptionSolved();
		}
		end_try

	if(d || r) print_info("\n");
	if(d || r) printPublicationsFileReferences(publicationsFile);
	if(d) printPublicationsFileCertificates(publicationsFile);

	if (out != NULL) fclose(out);

	KSI_free(rawPubfile);
	KSI_Integer_free(start);
	KSI_Integer_free(end);
	KSI_ExtendReq_free(extReq);
	KSI_ExtendResp_free(extResp);
	KSI_RequestHandle_free(request);
	KSI_PublicationData_free(pubData);
	KSI_free(base32);
	closeTask(ksi);

	return retval;
}


