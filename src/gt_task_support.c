#include <string.h>
#include <time.h>
#include <ksi/net_http.h>
#include <stdio.h>
#include "gt_task_support.h"
#include "try-catch.h"


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
static void configureNetworkProvider_throws(KSI_CTX *ksi, GT_CmdParameters *cmdparam){
	int res = KSI_OK;
	KSI_NetworkClient *net = NULL;
	try
	   CODE{
			/* Check if uri's are specified. */
			if (cmdparam->S || cmdparam->P || cmdparam->X || cmdparam->C || cmdparam->c) {
				res = KSI_UNKNOWN_ERROR;
				res = KSI_HttpClient_new(ksi, &net);
				ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to create new network provider.\n");

				/* Check aggregator url */
				if (cmdparam->S) {
					res = KSI_HttpClient_setSignerUrl(net, cmdparam->signingService_url);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set aggregator url '%s'.\n", cmdparam->signingService_url);
				}

				/* Check publications file url. */
				if (cmdparam->P) {
					res = KSI_HttpClient_setPublicationUrl(net, cmdparam->publicationsFile_url);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set publications file url '%s'.\n", cmdparam->publicationsFile_url);
				}

				/* Check extending/verification service url. */
				if (cmdparam->X) {
					res = KSI_HttpClient_setExtenderUrl(net, cmdparam->verificationService_url);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set extender/verifier url '%s'.\n", cmdparam->verificationService_url);
				}

				/* Check Network connection timeout. */
				if (cmdparam->C) {
					res = KSI_HttpClient_setConnectTimeoutSeconds(net, cmdparam->networkConnectionTimeout);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set network connection timeout %i.\n", cmdparam->networkConnectionTimeout);
				}

				/* Check Network transfer timeout. */
				if (cmdparam->c) {
					res = KSI_HttpClient_setReadTimeoutSeconds(net, cmdparam->networkTransferTimeout);
					ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to set network transfer timeout %i.\n", cmdparam->networkTransferTimeout);
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

void initTask_throws(GT_CmdParameters *cmdparam ,KSI_CTX **ksi){
	int res = KSI_UNKNOWN_ERROR;
	KSI_CTX *tmpKsi = NULL;
	KSI_PublicationsFile *tmpPubFile = NULL;
	KSI_PKITruststore *refTrustStore = NULL;
	int i=0;
	
	try
		CODE{
			res = KSI_CTX_new(&tmpKsi);
			ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to init KSI context.\n");
			configureNetworkProvider_throws(tmpKsi, cmdparam);

			if(cmdparam->b && (cmdparam->task != downloadPublicationsFile && cmdparam->task != verifyPublicationsFile)){
				KSI_LOG_debug(tmpKsi, "Setting publications file '%s'", cmdparam->inPubFileName);
				KSI_PublicationsFile_fromFile_throws(tmpKsi, cmdparam->inPubFileName, &tmpPubFile);
				KSI_setPublicationsFile(tmpKsi, tmpPubFile);
			}

			if(cmdparam->V || cmdparam->W){
				KSI_getPKITruststore(tmpKsi, &refTrustStore);
				if(cmdparam->V){
					for(i=0; i<cmdparam->sizeOpenSSLTruststoreFileName;i++){
						KSI_PKITruststore_addLookupFile_throws(refTrustStore, cmdparam->openSSLTruststoreFileName[i]);
					}
				}
				if(cmdparam->W){
					KSI_PKITruststore_addLookupDir_throws(refTrustStore, cmdparam->openSSLTrustStoreDirName);
				}
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
	int buf_len;
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
	int count = 0;
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
		res = KSI_PublicationRecordList_elementAt(list_publicationRecord, i, &publicationRecord);
		if(res != KSI_OK) return;
		
		if(KSI_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;
		pStart = buf;
		j=1;
		while(pLineBreak = strchr(pStart, '\n')){
			*pLineBreak = 0;
			printf("%s %2i) %s\n", (pStart == buf) ? "  " : "    ", (pStart == buf) ? (i+1) : j++, pStart);
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
	int i=0;
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
		*pLineBreak = 0;
		printf("%s", (pStart == buf) ? "  " : "    ");
		if(i++) 
			printf(" %2i) %s\n", i, pStart);
		else
			printf(" %s\n", pStart);
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
	KSI_VerificationStepResult *result = NULL;
	const char *desc;
	int i=0;

	if(sig == NULL){
		return;
	}
	
	printf("Verification steps:\n");
	res = KSI_Signature_getVerificationResult(sig, &sigVerification);
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
	 THROWABLE(KSI_createSignature(ksi, hash, sign), "Error: Unable to sign. (%s)\n", KSI_getErrorString(res));
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

