#include <string.h>
#include <time.h>
#include <ksi/net_http.h>
#include <stdio.h>
#include "gt_task_support.h"
#include "try-catch.h"
#include "param_set.h"

#define ON_ERROR_THROW_MSG(_exeption, ...) \
	if (res != KSI_OK){  \
		THROW_MSG(_exeption,__VA_ARGS__); \
	}

/**
 * Configures NetworkProvider using info from commandline.
 * Sets urls and timeouts.
 * @param[in] cmdparam pointer to command-line parameters.
 * @param[in] ksi pointer to KSI context.
 * 
 * @throws KSI_EXEPTION
 */
static void configureNetworkProvider_throws(KSI_CTX *ksi, Task *task){
	int res = KSI_OK;
	KSI_NetworkClient *net = NULL;
	bool S=false, P=false, X=false, C=false, c=false;
	char *signingService_url = NULL;
	char *publicationsFile_url = NULL;
	char *verificationService_url = NULL;
	int networkConnectionTimeout = 0;
	int networkTransferTimeout = 0;
	
	S = paramSet_getStrValueByNameAt(task->set, 'S',0,&signingService_url);
	P = paramSet_getStrValueByNameAt(task->set, 'P',0,&publicationsFile_url);
	X = paramSet_getStrValueByNameAt(task->set, 'X',0,&verificationService_url);
	C = paramSet_getIntValueByNameAt(task->set, 'C', 0,&networkConnectionTimeout);
	c = paramSet_getIntValueByNameAt(task->set, 'c', 0,&networkTransferTimeout);

	try
	   CODE{
			/* Check if uri's are specified. */
			if (S || P || X || C || c) {
				res = KSI_UNKNOWN_ERROR;
				res = KSI_HttpClient_new(ksi, &net);
				ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to create new network provider.\n");

				/* Check aggregator url */
				if (S) {
					res = KSI_HttpClient_setSignerUrl(net, signingService_url);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set aggregator url '%s'.\n", signingService_url);
				}

				/* Check publications file url. */
				if (P) {
					res = KSI_HttpClient_setPublicationUrl(net, publicationsFile_url);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set publications file url '%s'.\n", publicationsFile_url);
				}

				/* Check extending/verification service url. */
				if (X) {
					res = KSI_HttpClient_setExtenderUrl(net, verificationService_url);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set extender/verifier url '%s'.\n", verificationService_url);
				}

				/* Check Network connection timeout. */
				if (C) {
					res = KSI_HttpClient_setConnectTimeoutSeconds(net, networkConnectionTimeout);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set network connection timeout %i.\n", networkConnectionTimeout);
				}

				/* Check Network transfer timeout. */
				if (c) {
					res = KSI_HttpClient_setReadTimeoutSeconds(net, networkTransferTimeout);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set network transfer timeout %i.\n", networkTransferTimeout);
				}

				/* Set the new network provider. */
				res = KSI_setNetworkProvider(ksi, net);
				ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set network provider.\n");
			}

		} 
		CATCH_ALL{
			THROW_FORWARD_APPEND_MESSAGE("Error: Unable to configure network provider.\n");
		}
	end_try
	return;

}

void initTask_throws(Task *task ,KSI_CTX **ksi){
	int res = KSI_UNKNOWN_ERROR;
	KSI_CTX *tmpKsi = NULL;
	KSI_PublicationsFile *tmpPubFile = NULL;
	KSI_PKITruststore *refTrustStore = NULL;
	int i=0;
	
	bool b,V, W, E;
	char *inPubFileName;
	char *lookupFile = NULL;
	char *lookupDir = NULL;
	char *magicEmail = NULL;
	
	b = paramSet_getStrValueByNameAt(task->set, 'b',0, &inPubFileName);
	V = paramSet_isSetByName(task->set,'V');
	W = paramSet_getStrValueByNameAt(task->set, 'W',0, &lookupDir);
	E = paramSet_getStrValueByNameAt(task->set, 'E',0, &magicEmail);
	
	try
		CODE{
			res = KSI_CTX_new(&tmpKsi);
			ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to init KSI context.\n");
			configureNetworkProvider_throws(tmpKsi, task);

			if(b && (task->id != downloadPublicationsFile && task->id != verifyPublicationsFile)){
				KSI_LOG_debug(tmpKsi, "Setting publications file '%s'", inPubFileName);
				KSI_PublicationsFile_fromFile_throws(tmpKsi, inPubFileName, &tmpPubFile);
				KSI_setPublicationsFile(tmpKsi, tmpPubFile);
			}

			if(V || W){
				KSI_getPKITruststore(tmpKsi, &refTrustStore);
				if(V){
					while(paramSet_getStrValueByNameAt(task->set, 'V',i++,&lookupFile))
						KSI_PKITruststore_addLookupFile_throws(refTrustStore, lookupFile);
				}
				if(W){
					KSI_PKITruststore_addLookupDir_throws(refTrustStore, lookupDir);
				}
			}
			
			if(E){
				KSI_setPublicationCertEmail_throws(tmpKsi, magicEmail);
			}
			
			*ksi = tmpKsi;
		}
		CATCH(KSI_EXEPTION){
			KSI_PublicationsFile_free(tmpPubFile);
			KSI_CTX_free(tmpKsi);
			THROW_FORWARD_APPEND_MESSAGE("Error: Unable to configure KSI.\n");
		}
	end_try

	return;
}

void getFilesHash_throws(KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash){
	FILE *in = NULL;
	int res = KSI_UNKNOWN_ERROR;
	unsigned char buf[1024];
	size_t buf_len;
	try
		CODE{
			/* Open Input file */
			in = fopen(fname, "rb");
			if (in == NULL) 
				THROW_MSG(IO_EXEPTION, "Error: Unable to open input file '%s'\n", fname);

			/* Read the input file and calculate the hash of its contents. */
			while (!feof(in)) {
				buf_len = fread(buf, 1, sizeof (buf), in);
				/* Add  next block to the calculation. */
				res = KSI_DataHasher_add(hsr, buf, buf_len);
				if(res != KSI_OK){
					fclose(in);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to add data to hasher.\n");
				}
			}

			if (in != NULL) fclose(in);
			/* Close the data hasher and retreive the data hash. */
			res = KSI_DataHasher_close(hsr, hash);
			ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to create hash.\n");
		}
		CATCH_ALL{
			THROW_FORWARD_APPEND_MESSAGE("Error: Unable to hash file '%s'.\n", fname);
		}
	end_try

	return;
}

void saveSignatureFile_throws(KSI_Signature *sign, const char *fname){
	int res = KSI_UNKNOWN_ERROR;
	unsigned char *raw = NULL;
	int raw_len = 0;
	size_t count = 0;
	FILE *out = NULL;

	try
		CODE{
			/* Serialize the extended signature. */
			res = KSI_Signature_serialize(sign, &raw, &raw_len);
			ON_ERROR_THROW_MSG(KSI_EXEPTION,"Error: Unable to serialize signature.\n");

			/* Open output file. */
			out = fopen(fname, "wb");
			if (out == NULL) {
				KSI_free(raw);
				THROW_MSG(IO_EXEPTION, "Error: Unable to open output file '%s'\n",fname);
			}

			count = fwrite(raw, 1, raw_len, out);
			if (count != raw_len) {
				fclose(out);
				KSI_free(raw);
				THROW_MSG(KSI_EXEPTION, "Error: Failed to write output file.\n");
			}

		}
		CATCH_ALL{
			THROW_FORWARD_APPEND_MESSAGE("Error: Unable to save signature '%s'", fname);
		}
	end_try


	fclose(out);
	KSI_free(raw);
	return;
}

void printPublicationsFileReferences(const KSI_PublicationsFile *pubFile){
	int res = KSI_UNKNOWN_ERROR;
	KSI_LIST(KSI_PublicationRecord)* list_publicationRecord = NULL;
	KSI_PublicationRecord *publicationRecord = NULL;
	char buf[1024];
	int i, j;
	char *pLineBreak = NULL;
	char *pStart = NULL;
	
	if(pubFile == NULL) return;
	
	printf("Publications file references:\n");
	
	res = KSI_PublicationsFile_getPublications(pubFile, &list_publicationRecord);
	if(res != KSI_OK) return;

	for (i = 0; i < KSI_PublicationRecordList_length(list_publicationRecord); i++) {
		int h=0;
		res = KSI_PublicationRecordList_elementAt(list_publicationRecord, i, &publicationRecord);
		if(res != KSI_OK) return;
		
		if(KSI_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;
		pStart = buf;
		j=1;
		h=0;
		if(i) printf("\n");
		while(pLineBreak = strchr(pStart, '\n')){
			*pLineBreak = 0;
			if(h++<3)
				printf("%s %s\n", "  ", pStart);
			else
				printf("%s %2i) %s\n", "    ", j++, pStart);
			pStart = pLineBreak+1;
		}
	}

	return;
}

void printSignaturePublicationReference(const KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	KSI_PublicationRecord *publicationRecord;
	char buf[1024];
	char *pLineBreak = NULL;
	char *pStart = buf;
	int i=1;
	if(sig == NULL) return;
	
	printf("Signature publication references:\n");
	res = KSI_Signature_getPublicationRecord(sig, &publicationRecord);
	if(res != KSI_OK)return ;
	
	if(publicationRecord == NULL) {
		printf("  (No publication Records available)\n");
		return;
	}
	
	if(KSI_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;
	pStart = buf;
	
	while(pLineBreak = strchr(pStart, '\n')){
		int h=0;
		*pLineBreak = 0;

		if(h++<3)
			printf("%s %s\n", "  ", pStart);
		else
			printf("%s %2i) %s\n", "    ", i++, pStart);

		pStart = pLineBreak+1;
	}
	
	return;
}

void printSignerIdentity(KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	char *signerIdentity = NULL;

	if(sig == NULL) goto cleanup;
	
	printf("Signer identity: ");
	res = KSI_Signature_getSignerIdentity(sig, &signerIdentity);
	if(res != KSI_OK){
		printf("Unable to get signer identity.\n");
		goto cleanup;
	}
	
	printf("'%s'\n", signerIdentity == NULL ? "Unknown" : signerIdentity);

cleanup:	
	
	KSI_free(signerIdentity);
	return;
}

void printSignatureVerificationInfo(const KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	const KSI_VerificationResult *sigVerification = NULL;
	const KSI_VerificationStepResult *result = NULL;
	const char *desc;
	int i=0;

	if(sig == NULL){
		return;
	}
	
	printf("Verification steps:\n");
	res = KSI_Signature_getVerificationResult((KSI_Signature*)sig, &sigVerification);
	if(res != KSI_OK){
		return;
	}
	
	if(sigVerification != NULL){
		for(i=0; i< KSI_VerificationResult_getStepResultCount(sigVerification); i++){
			res = KSI_VerificationResult_getStepResult(sigVerification, i, &result);
			if(res != KSI_OK){
				return;
			}
			printf("  0x%03x:\t%s", KSI_VerificationStepResult_getStep(result), KSI_VerificationStepResult_isSuccess(result) ? "OK" : "FAIL");
			desc = KSI_VerificationStepResult_getDescription(result);
			if (desc && *desc) {
				printf(" (%s)", desc);
			}
			printf("\n");
		}
	}
	return;
}

void printPublicationsFileCertificates(const KSI_PublicationsFile *pubfile){
	KSI_CertificateRecordList *certReclist = NULL;
	KSI_CertificateRecord *certRec = NULL;
	KSI_PKICertificate *cert = NULL;
	char buf[1024];
	int i=0;
	int res = 0;
	
	if(pubfile == NULL) goto cleanup;
	printf("Publications file certificates::\n");
	
	res = KSI_PublicationsFile_getCertificates(pubfile, &certReclist);
	if(res != KSI_OK || certReclist == NULL) goto cleanup;
	
	for(i=0; i<KSI_CertificateRecordList_length(certReclist); i++){
		res = KSI_CertificateRecordList_elementAt(certReclist, i, &certRec);
		if(res != KSI_OK || certRec == NULL) goto cleanup;
		
		res = KSI_CertificateRecord_getCert(certRec, &cert);
		if(res != KSI_OK || cert == NULL) goto cleanup;

		if(KSI_PKICertificate_toString(cert, buf, sizeof(buf)) != NULL)
			printf("  %2i)  %s\n",i, buf);
		else
			printf("  %2i)  null\n",i);
	}
	
cleanup:
				
	return;	
	}

static unsigned int elapsed_time_ms;

unsigned int measureLastCall(void){
	static clock_t thisCall = 0;
	static clock_t lastCall = 0;

	thisCall = clock();
	elapsed_time_ms = 1000*(thisCall - lastCall) / CLOCKS_PER_SEC;
	lastCall = thisCall;
	return elapsed_time_ms;
}

unsigned int measuredTime(void){
	return elapsed_time_ms;
}

char* str_measuredTime(void){
	static char buf[32];
	snprintf(buf,32,"(%i ms)", elapsed_time_ms);
	return buf;
}





#define THROWABLE(func, ...) \
	int res = KSI_UNKNOWN_ERROR; \
	res = func;  \
	if(res != KSI_OK) {THROW_MSG(KSI_EXEPTION, __VA_ARGS__);} \
	return res;


int KSI_receivePublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile **publicationsFile){
	//THROWABLE(KSI_receivePublicationsFile(ksi, publicationsFile), "Error: Unable to read publications file. (%s)\n", KSI_getErrorString(res));

	int res = KSI_UNKNOWN_ERROR; 
	res = KSI_receivePublicationsFile(ksi, publicationsFile);  
	if(res != KSI_OK) {
		printf("\n");
		KSI_ERR_statusDump(ksi, stderr);
		THROW_MSG(KSI_EXEPTION, "Error: Unable to read publications file. (%s)\n", KSI_getErrorString(res));
	} 
	return res;
}

int KSI_verifyPublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile){
	THROWABLE(KSI_verifyPublicationsFile(ksi, publicationsFile), "Error: Unable to verify publications file. (%s)\n", KSI_getErrorString(res));
}

int KSI_PublicationsFile_serialize_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile, char **raw, int *raw_len){
	THROWABLE(KSI_PublicationsFile_serialize(ksi, publicationsFile, raw, raw_len), "Error: Unable serialize publications file. (%s)\n", KSI_getErrorString(res));
}

int KSI_DataHasher_open_throws(KSI_CTX *ksi,int hasAlgID ,KSI_DataHasher **hsr){
	THROWABLE(KSI_DataHasher_open(ksi, hasAlgID, hsr), "Error: Unable to create hasher. (%s)\n", KSI_getErrorString(res));
}

int KSI_createSignature_throws(KSI_CTX *ksi, const KSI_DataHash *hash, KSI_Signature **sign){
/*
	 THROWABLE(KSI_createSignature(ksi, hash, sign), "Error: Unable to sign. (%s)\n", KSI_getErrorString(res));
*/
	int res = KSI_UNKNOWN_ERROR; 
	res = KSI_createSignature(ksi, hash, sign);
	if(res != KSI_OK) {
		printf("\n");
		KSI_ERR_statusDump(ksi, stderr);
		THROW_MSG(KSI_EXEPTION, "Error: Unable to sign. (%s)\n", KSI_getErrorString(res));
	} 
	return res;

}

int KSI_DataHash_fromDigest_throws(KSI_CTX *ksi, int hasAlg, char *data, unsigned int len, KSI_DataHash **hash){
	THROWABLE(KSI_DataHash_fromDigest(ksi, hasAlg, data, len, hash), "Error: Unable to create hash from digest. (%s)\n", KSI_getErrorString(res));
}

int KSI_PublicationsFile_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_PublicationsFile **pubFile){
	//THROWABLE(KSI_PublicationsFile_fromFile(ksi, fileName, pubFile), "Error: Unable to read publications file '%s'.\n", fileName)
	
		int res = KSI_UNKNOWN_ERROR; 
	res = KSI_PublicationsFile_fromFile(ksi, fileName, pubFile);  
	if(res != KSI_OK) {
		printf("\n");
		KSI_ERR_statusDump(ksi, stderr);
		THROW_MSG(KSI_EXEPTION, "Error: Get pubfile. (%s)\n", KSI_getErrorString(res));
	} 
	return res;
}

int KSI_Signature_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_Signature **sig){
	THROWABLE(KSI_Signature_fromFile(ksi, fileName, sig), "Error: Unable to read signature from file. (%s)\n", KSI_getErrorString(res));
}

int KSI_Signature_verify_throws(KSI_Signature *sig, KSI_CTX *ksi){
	THROWABLE(KSI_Signature_verify(sig, ksi), "Error: Unable verify signature. (%s)\n", KSI_getErrorString(res));
}

int KSI_Signature_createDataHasher_throws(KSI_Signature *sig, KSI_DataHasher **hsr){
	THROWABLE(KSI_Signature_createDataHasher(sig, hsr), "Error: Unable to create data hasher. (%s)\n", KSI_getErrorString(res));
}

int KSI_Signature_verifyDataHash_throws(KSI_Signature *sig, KSI_CTX *ksi, KSI_DataHash *hash){
	THROWABLE(KSI_Signature_verifyDataHash(sig, ksi,  hash), "Error: Wrong document or signature. (%s)\n", KSI_getErrorString(res));
}
/*To nearest publication available*/
int KSI_extendSignature_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext){
	THROWABLE(KSI_extendSignature(ksi, sig, ext),"Error: Unable to extend signature. (%s)\n", KSI_getErrorString(res));
}
/*To Publication record*/
int KSI_Signature_extend_throws(const KSI_Signature *signature, KSI_CTX *ctx, const KSI_PublicationRecord *pubRec, KSI_Signature **extended){
	//THROWABLE(KSI_Signature_extend(signature, ctx, pubRec, extended), "Error: Unable to extend signature. (%s)\n", KSI_getErrorString(res));

	int res = KSI_UNKNOWN_ERROR; 
	res = KSI_Signature_extend(signature, ctx, pubRec, extended);  
	if(res != KSI_OK) {
		printf("\n");
		KSI_ERR_statusDump(ctx, stderr);
		THROW_MSG(KSI_EXEPTION, "Error: Unable to extend signature. (%s)\n", KSI_getErrorString(res));
	} 
	return res;
}

int KSI_PKITruststore_addLookupFile_throws(KSI_PKITruststore *store, const char *path){
	THROWABLE(KSI_PKITruststore_addLookupFile(store,path), "Error: Unable to set PKI trust store lookup file. (%s)\n",  KSI_getErrorString(res));
}
		
int KSI_PKITruststore_addLookupDir_throws(KSI_PKITruststore *store, const char *path){
	THROWABLE(KSI_PKITruststore_addLookupDir(store,path), "Error: Unable to set PKI trust store lookup dir. (%s)\n",  KSI_getErrorString(res));
}



int KSI_Integer_new_throws(KSI_CTX *ctx, KSI_uint64_t value, KSI_Integer **kint){
	THROWABLE(KSI_Integer_new(ctx, value, kint), "Error: Unable construkt KSI Interger. (%s)\n",  KSI_getErrorString(res));
}

int KSI_ExtendReq_new_throws(KSI_CTX *ctx, KSI_ExtendReq **t){
	THROWABLE(KSI_ExtendReq_new(ctx, t);, "Error: Unable construct KSI extend request. (%s)\n",  KSI_getErrorString(res));
}

int KSI_ExtendReq_setAggregationTime_throws(KSI_ExtendReq *t, KSI_Integer *aggregationTime){
	THROWABLE(KSI_ExtendReq_setAggregationTime(t, aggregationTime);, "Error: Unable to set request aggregation time. (%s)\n",  KSI_getErrorString(res));
}

int KSI_ExtendReq_setPublicationTime_throws(KSI_ExtendReq *t, KSI_Integer *publicationTime){
	THROWABLE(KSI_ExtendReq_setPublicationTime(t, publicationTime), "Error: Unable to set request publication time. (%s)\n",  KSI_getErrorString(res));
}

int KSI_sendExtendRequest_throws(KSI_CTX *ctx, KSI_ExtendReq *request, KSI_RequestHandle **handle){
	THROWABLE(KSI_sendExtendRequest(ctx, request, handle), "Error: Unable to send extend request. (%s)\n",  KSI_getErrorString(res));
}

int KSI_RequestHandle_getExtendResponse_throws(KSI_RequestHandle *handle, KSI_ExtendResp **resp){
	THROWABLE(KSI_RequestHandle_getExtendResponse(handle, resp), "Error: Unable to get extend request. (%s)\n",  KSI_getErrorString(res));
}

int KSI_ExtendResp_getStatus_throws(const KSI_ExtendResp *t, KSI_Integer **status){
	THROWABLE(KSI_ExtendResp_getStatus(t, status), "Error: Unable to get request response status. (%s)\n",  KSI_getErrorString(res));
}
int KSI_ExtendResp_getCalendarHashChain_throws(const KSI_ExtendResp *t, KSI_CalendarHashChain **calendarHashChain){
	THROWABLE(KSI_ExtendResp_getCalendarHashChain(t, calendarHashChain), "Error: Unable to get calendar hash chain. (%s)\n",  KSI_getErrorString(res));
}

int KSI_CalendarHashChain_aggregate_throws(KSI_CalendarHashChain *chain, KSI_DataHash **hsh){
	THROWABLE(KSI_CalendarHashChain_aggregate(chain, hsh), "Error: Unable to aggregate. (%s)\n",  KSI_getErrorString(res));
}

int KSI_CalendarHashChain_getPublicationTime_throws(const KSI_CalendarHashChain *t, KSI_Integer **publicationTime){
	THROWABLE(KSI_CalendarHashChain_getPublicationTime(t, publicationTime), "Error: Unable to get publications time. (%s)\n",  KSI_getErrorString(res));
}

int KSI_PublicationData_new_throws(KSI_CTX *ctx, KSI_PublicationData **t){
	THROWABLE(KSI_PublicationData_new(ctx, t), "Error: Unable to construct publication data. (%s)\n",  KSI_getErrorString(res));
}

int KSI_PublicationData_setImprint_throws(KSI_PublicationData *t, KSI_DataHash *imprint){
	THROWABLE(KSI_PublicationData_setImprint(t,imprint), "Error: Unable to set publication data imprint. (%s)\n",  KSI_getErrorString(res));
}

int KSI_PublicationData_setTime_throws(KSI_PublicationData *t, KSI_Integer *time){
	THROWABLE(KSI_PublicationData_setTime(t, time), "Error: Unable to set publication data time. (%s)\n",  KSI_getErrorString(res));
}

int KSI_PublicationData_toBase32_throws(const KSI_PublicationData *published_data, char **publication){
	THROWABLE(KSI_PublicationData_toBase32(published_data, publication), "Error: Unable to convert publication data to base 32. (%s)\n",  KSI_getErrorString(res));
}

int KSI_Signature_clone_throws(const KSI_Signature *sig, KSI_Signature **clone){
	THROWABLE(KSI_Signature_clone(sig, clone), "Error: Unable to clone signature. (%s)\n",  KSI_getErrorString(res));
}

int KSI_Signature_getSigningTime_throws(const KSI_Signature *sig, KSI_Integer **signTime){
	THROWABLE(KSI_Signature_getSigningTime(sig, signTime), "Error: Unable to get signatures signing time. (%s)\n",  KSI_getErrorString(res));
}

int KSI_ExtendResp_setCalendarHashChain_throws(KSI_ExtendResp *t, KSI_CalendarHashChain *calendarHashChain){
	THROWABLE(KSI_ExtendResp_setCalendarHashChain(t, calendarHashChain), "Error: Unable to get extension response signing time. (%s)\n",  KSI_getErrorString(res));
}

int KSI_Signature_replaceCalendarChain_throws(KSI_Signature *sig, KSI_CalendarHashChain *calendarHashChain){
	THROWABLE(KSI_Signature_replaceCalendarChain(sig,calendarHashChain), "Error: Unable to replace signatures calender hash chain. (%s)\n",  KSI_getErrorString(res));
}

int KSI_PublicationsFile_getPublicationDataByTime_throws(const KSI_PublicationsFile *pubFile, const KSI_Integer *pubTime, KSI_PublicationRecord **pubRec){
	THROWABLE(KSI_PublicationsFile_getPublicationDataByTime(pubFile, pubTime, pubRec), "Error: Unable to get publication date by time. (%s)\n",  KSI_getErrorString(res));
}

int KSI_Signature_replacePublicationRecord_throws(KSI_Signature *sig, KSI_PublicationRecord *pubRec){
	THROWABLE(KSI_Signature_replacePublicationRecord(sig, pubRec), "Error: Unable set signatures publication record. (%s)\n",  KSI_getErrorString(res));
}

int KSI_PublicationRecord_clone_throws(const KSI_PublicationRecord *rec, KSI_PublicationRecord **clone){
	THROWABLE(KSI_PublicationRecord_clone(rec, clone), "Error: Unable clone signatures publication record. (%s)\n",  KSI_getErrorString(res));
	
}

int KSI_setPublicationCertEmail_throws(KSI_CTX *ctx, const char *email){
	THROWABLE(KSI_setPublicationCertEmail(ctx, email), "Error: Unable set publication cert email. (%s)\n",  KSI_getErrorString(res));
}

