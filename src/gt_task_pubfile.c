#include "gt_task_support.h"
#include "try-catch.h"

void getPublicationString_throws(KSI_CTX *ctx, KSI_Integer *time);

bool GT_publicationsFileTask(Task *task){
	KSI_CTX *ksi = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	FILE *out = NULL;
	size_t bytesWritten;
	char *rawPubfile = NULL;
	int rawLen = 0;

	KSI_Integer *end = NULL;
	KSI_Integer *start = NULL;
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
	
	bool state = true;
	
	bool d,t;
	char *outPubFileName = NULL;
	int publicationTime = 0;
		
	paramSet_getStrValueByNameAt(task->set, "o",0,&outPubFileName);
	paramSet_getIntValueByNameAt(task->set,"T",0,&publicationTime);
	d = paramSet_isSetByName(task->set, "d");
	t = paramSet_isSetByName(task->set, "t");
	
	/*Initalization of KSI */
	resetExeptionHandler();
	try
		CODE{
			initTask_throws(task ,&ksi);

			if(task->id == downloadPublicationsFile){
				printf("Downloading publications file...");
				MEASURE_TIME(KSI_receivePublicationsFile_throws(ksi, &publicationsFile));
				printf("ok. %s\n",t ? str_measuredTime() : "");

				printf("Verifying publications file...");
				MEASURE_TIME(KSI_verifyPublicationsFile_throws(ksi, publicationsFile));
				printf("ok. %s\n",t ? str_measuredTime() : "");

				KSI_PublicationsFile_serialize(ksi, publicationsFile, &rawPubfile, &rawLen);
				out = fopen(outPubFileName, "wb");
				if(out == NULL) THROW_MSG(IO_EXEPTION, "Unable to ope publications file '%s' for writing.\n", outPubFileName);

				bytesWritten = fwrite(rawPubfile, 1, rawLen, out);
				if(bytesWritten != rawLen) THROW_MSG(IO_EXEPTION, "Error: Unable to write publications file '%s'.\n", outPubFileName);
				printf("Publications file '%s' saved.\n", outPubFileName);
			} else if(task->id == createPublicationString){
				printf("Sending aggregation request...");
				measureLastCall();
				KSI_Integer_new_throws(ksi, publicationTime, &start);
				KSI_Integer_new_throws(ksi, publicationTime, &end);
				KSI_ExtendReq_new_throws(ksi, &extReq);
				KSI_ExtendReq_setAggregationTime_throws(extReq, start);
				KSI_ExtendReq_setPublicationTime_throws(extReq, end);
				KSI_sendExtendRequest_throws(ksi, extReq, &request);
				
				KSI_RequestHandle_getExtendResponse_throws(request, &extResp);
				KSI_ExtendResp_getStatus_throws(extResp, &respStatus);

				if (respStatus == NULL || !KSI_Integer_equalsUInt(respStatus, 0)) {
					KSI_Utf8String *errm = NULL;
					int res = KSI_ExtendResp_getErrorMsg(extResp, &errm);
					if (res == KSI_OK && KSI_Utf8String_cstr(errm) != NULL) {
						THROW_MSG(KSI_EXEPTION, "Extender returned error %llu: '%s'.\n", (unsigned long long)KSI_Integer_getUInt64(respStatus), KSI_Utf8String_cstr(errm));
					}else{
						THROW_MSG(KSI_EXEPTION, "Extender returned error %llu.\n", (unsigned long long)KSI_Integer_getUInt64(respStatus));
					}
				}
				measureLastCall();
				printf("ok. %s\n",t ? str_measuredTime() : "");

				
				printf("Getting publication string...");
				KSI_ExtendResp_getCalendarHashChain_throws(extResp, &chain);
				KSI_CalendarHashChain_aggregate_throws(chain, &extHsh);
				KSI_CalendarHashChain_getPublicationTime_throws(chain, &pubTime);
				KSI_PublicationData_new_throws(ksi, &pubData);
				KSI_PublicationData_setImprint_throws(pubData, extHsh);
				KSI_PublicationData_setTime_throws(pubData, pubTime);
				KSI_PublicationData_toBase32_throws(pubData, &base32);
				printf("ok\n\n");

				
				
				pubTm = (time_t)KSI_Integer_getUInt64(pubTime);
				gmtime_r(&pubTm, &tm);
				strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", &tm);

				printf("[%s]\n", strTime);
				printf("pub=%s\n", base32);
			}else{
				THROW_MSG(KSI_EXEPTION, "Unkown error");
			}
			
		}
		CATCH(KSI_EXEPTION){
				printf("failed.\n");
				printErrorLocations();
				exeptionSolved();
				state = false;
		}
		CATCH(IO_EXEPTION){
				fprintf(stderr , _EXP.tmp);
				printErrorLocations();
				state = false;
		}
	end_try

	if(d) printf("\n");
	if(d) printPublicationsFileReferences(publicationsFile);	
	if(d) printPublicationsFileCertificates(publicationsFile);	
				
	if (out != NULL) fclose(out);
	
	KSI_Integer_free(start);
	KSI_Integer_free(end);
	KSI_ExtendReq_free(extReq);
	KSI_ExtendResp_free(extResp);
	KSI_RequestHandle_free(request);
	KSI_PublicationData_free(pubData);
	KSI_free(base32);
	KSI_CTX_free(ksi);
	
	return state;
}


