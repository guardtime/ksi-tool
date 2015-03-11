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

#include <string.h>
#include <time.h>
#include <ksi/net_http.h>
#include <ksi/net_tcp.h>
#include <ksi/hashchain.h>
#include <stdio.h>
#include "gt_task_support.h"
#include "try-catch.h"
#include "param_set.h"
#include <ctype.h>

#define ON_ERROR_THROW_MSG(_exception, ...) \
	if (res != KSI_OK){  \
		THROW_MSG(_exception,getReturnValue(res),__VA_ARGS__); \
	}

static int url_getHost(const char* url, char* buf, int len){
	char *startOfHost = NULL;
	char *endOfHost = NULL;
	int hostLen = 0;
	
	if(url == NULL) return 1;
	startOfHost = strstr(url, "://");
	if(startOfHost == NULL) return 1;
	startOfHost+=3;
	
	endOfHost = strstr(startOfHost, ":");
	if(endOfHost == NULL)
	endOfHost = strstr(startOfHost, "/");
	if(endOfHost == NULL) return 1;
	
	hostLen = (int)(endOfHost - startOfHost);
	if(len < hostLen+1 && hostLen > 0) return 1;
	
	strncpy(buf, startOfHost, hostLen);
	buf[hostLen] = 0;
	return 0;
}

static int url_getPort(const char* url, unsigned* port){
	char *startOfHost = NULL;
	char *startOfPort = NULL;
	int tmp = 0;
	
	if(url == NULL) return 1;
	startOfHost = strstr(url, "://");
	if(startOfHost == NULL) return 1;
	startOfHost+=3;
	startOfPort = strstr(startOfHost, ":");
	if(startOfPort == NULL) return 1;
	startOfPort++;
	
	sscanf(startOfPort, "%i", &tmp);
	
	*port = tmp;
	return 0;
}

static int url_getScheme(const char* url, char* buf, int len){
	char *end;
	int subLen;
	
	if(url == NULL || buf == NULL) return 1;
	end = strstr(url, "://");
	if(end == NULL) return 1;
	
	subLen = (int)(end - url);
	if(len < subLen+1 && subLen > 0) return 1;
	
	strncpy(buf, url, subLen);
	buf[subLen] = 0;
	return 0;
}



/**
 * Configures NetworkProvider using info from command-line.
 * Sets urls and timeouts.
 * @param[in] cmdparam pointer to command-line parameters.
 * @param[in] ksi pointer to KSI context.
 * 
 * @throws KSI_EXCEPTION
 */
static void configureNetworkProvider_throws(KSI_CTX *ksi, Task *task){
	paramSet *set = NULL;
	int res = KSI_OK;
	bool S, P, X, C, c, bUser, bPass, s, x, p, T, aggre;
	char *signingService_url = NULL;
	char *publicationsFile_url = NULL;
	char *verificationService_url = NULL;
	int networkConnectionTimeout = 0;
	int networkTransferTimeout = 0;
	char *user = NULL;
	char *pass = NULL;
	bool useTCP = false;
	
	set = Task_getSet(task);
	S = paramSet_getStrValueByNameAt(set, paramSet_isSetByName(set, "S") ? "S" : "sysvar_aggre_url",0,&signingService_url);
	X = paramSet_getStrValueByNameAt(set, paramSet_isSetByName(set, "X") ? "X" : "sysvar_ext_url",0,&verificationService_url);
	P = paramSet_getStrValueByNameAt(set, "P",0,&publicationsFile_url);
	
	C = paramSet_getIntValueByNameAt(set, "C", 0,&networkConnectionTimeout);
	c = paramSet_getIntValueByNameAt(set, "c", 0,&networkTransferTimeout);
	aggre = paramSet_isSetByName(set, "aggre");
	s = paramSet_isSetByName(set, "s");
	x = paramSet_isSetByName(set, "x");
	p = paramSet_isSetByName(set, "p");
	T = paramSet_isSetByName(set, "T");
	
	

	if(x || (p && T)){
		bUser = paramSet_getStrValueByNameAt(set, paramSet_isSetByName(set, "user") ? "user" : "sysvar_ext_user",0,&user);
		bPass = paramSet_getStrValueByNameAt(set, paramSet_isSetByName(set, "pass") ? "pass" : "sysvar_ext_pass",0,&pass);
	}
	else if(s || aggre){
		bUser = paramSet_getStrValueByNameAt(set, paramSet_isSetByName(set, "user") ? "user" : "sysvar_aggre_user",0,&user);
		bPass = paramSet_getStrValueByNameAt(set, paramSet_isSetByName(set, "pass") ? "pass" : "sysvar_aggre_pass",0,&pass);
	}
	
	if(user == NULL) user = "anon";
	if(pass == NULL) pass = "anon";
	
	try
	   CODE{
			if(x || (p && T))
				KSI_CTX_setExtender_throws(ksi, verificationService_url, user, pass);
			else if(s || aggre)
				KSI_CTX_setAggregator_throws(ksi, signingService_url, user, pass);

			if (P)
				KSI_CTX_setPublicationUrl_throws(ksi, publicationsFile_url);

			if (C)
				KSI_CTX_setConnectionTimeoutSeconds_throws(ksi, networkConnectionTimeout);

			if (c)
				KSI_CTX_setTransferTimeoutSeconds_throws(ksi, networkTransferTimeout);
		} 
		CATCH_ALL{
			THROW_FORWARD_APPEND_MESSAGE("Error: Unable to configure network provider.\n");
		}
	end_try
	return;

}

/**
 * File object for logging
 */
static FILE *logFile = NULL;

void initTask_throws(Task *task ,KSI_CTX **ksi){
	paramSet *set = NULL;
	int res = KSI_UNKNOWN_ERROR;
	KSI_CTX *tmpKsi = NULL;
	KSI_PublicationsFile *tmpPubFile = NULL;
	KSI_PKITruststore *refTrustStore = NULL;
	int i=0;
	
	bool log, b,V, W, E;
	char *outLogfile = NULL;
	char *inPubFileName = NULL;
	char *lookupFile = NULL;
	char *lookupDir = NULL;
	char *magicEmail = NULL;

	set = Task_getSet(task);
	log = paramSet_getStrValueByNameAt(set, "log",0, &outLogfile);
	b = paramSet_getStrValueByNameAt(set, "b",0, &inPubFileName);
	V = paramSet_isSetByName(set,"V");
	W = paramSet_getStrValueByNameAt(set, "W",0, &lookupDir);
	E = paramSet_getStrValueByNameAt(set, "E",0, &magicEmail);
	
	try
		CODE{
			res = KSI_CTX_new(&tmpKsi);
			ON_ERROR_THROW_MSG(KSI_EXCEPTION, "Error: Unable to initialize KSI context.\n");
			
			if(log){
				logFile = fopen(outLogfile, "w");
				if (logFile == NULL){
					THROW_MSG(IO_EXCEPTION, EXIT_IO_ERROR, "Error: Unable to open log file '%s'.\n", outLogfile);
				}
				KSI_CTX_setLoggerCallback(tmpKsi, KSI_LOG_StreamLogger, logFile);
				KSI_CTX_setLogLevel(tmpKsi, KSI_LOG_DEBUG);
			}
			
			configureNetworkProvider_throws(tmpKsi, task);
			
			if(b && (Task_getID(task) != downloadPublicationsFile && Task_getID(task) != verifyPublicationsFile)){
				KSI_LOG_debug(tmpKsi, "Setting publications file '%s'", inPubFileName);
				KSI_PublicationsFile_fromFile_throws(tmpKsi, inPubFileName, &tmpPubFile);
				KSI_setPublicationsFile(tmpKsi, tmpPubFile);
				tmpPubFile = NULL;
			}

			if(V || W){
				KSI_getPKITruststore(tmpKsi, &refTrustStore);
				if(V){
					while(paramSet_getStrValueByNameAt(set, "V",i++,&lookupFile))
						KSI_PKITruststore_addLookupFile_throws(tmpKsi, refTrustStore, lookupFile);
				}
				if(W){
					KSI_PKITruststore_addLookupDir_throws(tmpKsi, refTrustStore, lookupDir);
				}
			}
			
			if(E){
				KSI_setPublicationCertEmail_throws(tmpKsi, magicEmail);
			}
			
			*ksi = tmpKsi;
		}
		CATCH_ALL{
			KSI_PublicationsFile_free(tmpPubFile);
			KSI_CTX_free(tmpKsi);
			THROW_FORWARD_APPEND_MESSAGE("Error: Unable to configure KSI.\n");
		}
	end_try

	return;
}

void closeTask(KSI_CTX *ksi){
	if(logFile) fclose(logFile);
	KSI_CTX_free(ksi);
}

void getFilesHash_throws(KSI_CTX *ksi, KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash){
	FILE *in = NULL;
	unsigned char buf[1024];
	size_t buf_len;
	
	try
		CODE{
			if(ksi == NULL || hsr == NULL || fname == NULL || hash == NULL)
				THROW_MSG(INVALID_ARGUMENT_EXCEPTION, EXIT_FAILURE, "Error: Invalid function parameters.");

			in = fopen(fname, "rb");
			if (in == NULL) 
				THROW_MSG(IO_EXCEPTION,EXIT_IO_ERROR, "Error: Unable to open input file '%s'\n", fname);

			while (!feof(in)) {
				buf_len = fread(buf, 1, sizeof (buf), in);
				KSI_DataHasher_add_throws(ksi, hsr, buf, buf_len);
			}

			if (in != NULL) fclose(in);
			KSI_DataHasher_close_throws(ksi, hsr, hash);
		}
		CATCH_ALL{
			if(in) fclose(in);
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
			out = fopen(fname, "wb");
			if (out == NULL)
				THROW_MSG(IO_EXCEPTION,res, "Error: Unable to open output file '%s'\n",fname);

			res = KSI_Signature_serialize(sign, &raw, &raw_len);
			ON_ERROR_THROW_MSG(KSI_EXCEPTION, "Error: Unable to serialize signature.\n");

			count = fwrite(raw, 1, raw_len, out);
			if (count != raw_len)
				THROW_MSG(KSI_EXCEPTION,res, "Error: Failed to write output file.\n");
		}
		CATCH_ALL{
			fclose(out);
			KSI_free(raw);
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
			if(h++ < 3)
				printf("%s %s\n", "  ", pStart);
			else
				printf("%s %2i) %s\n", "    ", j++, pStart);
			pStart = pLineBreak+1;
		}
		
		printf("%s %2i) %s\n", "    ", j++, pStart);
	}
	printf("\n");
	return;
}

void printSignaturePublicationReference(const KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	KSI_PublicationRecord *publicationRecord;
	char buf[1024];
	char *pLineBreak = NULL;
	char *pStart = buf;
	int i=1;
	int h=0;
	if(sig == NULL) return;
	
	printf("Signatures publication references:\n");
	res = KSI_Signature_getPublicationRecord(sig, &publicationRecord);
	if(res != KSI_OK)return ;
	
	if(publicationRecord == NULL) {
		fprintf(stderr, "  (No publication records available)\n\n");
		return;
	}
	
	if(KSI_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;
	pStart = buf;
	
	while(pLineBreak = strchr(pStart, '\n')){
		*pLineBreak = 0;

		if(h++<3)
			printf("%s %s\n", "  ", pStart);
		else
			printf("%s %2i) %s\n", "    ", i++, pStart);

		pStart = pLineBreak+1;
	}
	
	printf("%s %2i) %s\n", "    ", i++, pStart);
	printf("\n");
	
	return;
}

void printSignerIdentity(KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	char *signerIdentity = NULL;

	if(sig == NULL) goto cleanup;
	
	printf("Signer identity: ");
	res = KSI_Signature_getSignerIdentity(sig, &signerIdentity);
	if(res != KSI_OK){
		fprintf(stderr, "Unable to get signer identity.\n");
		goto cleanup;
	}
	
	printf("'%s'\n", signerIdentity == NULL || strlen(signerIdentity) == 0 ? "Unknown" : signerIdentity);
	printf("\n");
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
		printf("Unable to get verification steps\n\n");
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
	printf("\n");
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

// helpers for hex decoding
static int x(char c){
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	abort(); // isxdigit lies.
	return -1; // makes compiler happy
}

static int xx(char c1, char c2){
	if(!isxdigit(c1) || !isxdigit(c2))
		return -1;
	return x(c1) * 16 + x(c2);
}

/**
 * Converts a string into binary array.
 * 
 * @param[in] ksi Pointer to KSI KSI_CTX object.
 * @param[in] hexin Pointer to string for conversion.
 * @param[out] binout Pointer to receiving pointer to binary array.  
 * @param[out] lenout Pointer to binary array length.
 * 
 * @throws INVALID_ARGUMENT_EXCEPTION, OUT_OF_MEMORY_EXCEPTION.
 */
static void getBinaryFromHexString(KSI_CTX *ksi, const char *hexin, unsigned char **binout, size_t *lenout){
	size_t len = strlen(hexin);
	unsigned char *tmp=NULL;
	size_t arraySize = len/2;
	int i,j;

	if(hexin == NULL || binout == NULL || lenout == NULL)
		THROW(INVALID_ARGUMENT_EXCEPTION,EXIT_FAILURE);
	
	if(len%2 != 0){
		THROW_MSG(INVALID_ARGUMENT_EXCEPTION,EXIT_INVALID_FORMAT, "Error: The hash length is not even number!\n");
	}

	tmp = KSI_calloc(arraySize, sizeof(unsigned char));
	if(tmp == NULL){
		THROW_MSG(OUT_OF_MEMORY_EXCEPTION,EXIT_OUT_OF_MEMORY, "Error: Unable to get memory for parsing hex to binary.\n");
	}

	for(i=0,j=0; i<arraySize; i++, j+=2){
		int res = xx(hexin[j], hexin[j+1]);
		if(res == -1){
			KSI_free(tmp);
			tmp = NULL;
			THROW_MSG(INVALID_ARGUMENT_EXCEPTION,EXIT_INVALID_FORMAT, "Error: The hex number is invalid: %c%c!\n", hexin[j], hexin[j+1]);
		}
		tmp[i] = res;
		//printf("%c%c -> %i\n", hexin[j], hexin[j+1], tempBin[i]);
	}

	*lenout = arraySize;
	*binout = tmp;

	return;
}

static bool getHashAndAlgStrings(const char *instrn, char **strnAlgName, char **strnHash){
	char *colon = NULL;
	size_t algLen = 0;
	size_t hahsLen = 0;
	char *temp_strnAlg = NULL;
	char *temp_strnHash = NULL;
	
	
	if(strnAlgName == NULL || strnHash == NULL) return false;
	*strnAlgName = NULL;
	*strnHash = NULL;
	if (instrn == NULL) return false;

	colon = strchr(instrn, ':');
	if (colon != NULL) {
		algLen = (colon - instrn) / sizeof (char);
		hahsLen = strlen(instrn) - algLen - 1;
		temp_strnAlg = calloc(algLen + 1, sizeof (char));
		temp_strnHash = calloc(hahsLen + 1, sizeof (char));
		memcpy(temp_strnAlg, instrn, algLen);
		temp_strnAlg[algLen] = 0;
		memcpy(temp_strnHash, colon + 1, hahsLen);
		temp_strnHash[hahsLen] = 0;
	}

	*strnAlgName = temp_strnAlg;
	*strnHash = temp_strnHash;
	return true;
}

int getHashAlgorithm_throws(const char *hashAlg){
	int hasAlgID = KSI_getHashAlgorithmByName(hashAlg);
	if(hasAlgID == -1) THROW_MSG(KSI_EXCEPTION, EXIT_CRYPTO_ERROR, "Error: The hash algorithm \"%s\" is unknown\n", hashAlg);
	return hasAlgID;
}

void getHashFromCommandLine_throws(const char *imprint,KSI_CTX *ksi, KSI_DataHash **hash){
	unsigned char *data = NULL;
	size_t len;
	int res = KSI_UNKNOWN_ERROR;
	int hasAlg = -1;

	char *strAlg = NULL;
	char *strHash = NULL;
	try
		CODE{
			if(imprint == NULL) THROW_MSG(INVALID_ARGUMENT_EXCEPTION,EXIT_INVALID_CL_PARAMETERS, "");
			getHashAndAlgStrings(imprint, &strAlg, &strHash);
			if(strAlg == NULL || strHash== NULL ) THROW_MSG(INVALID_ARGUMENT_EXCEPTION,EXIT_INVALID_CL_PARAMETERS, "");
			
			getBinaryFromHexString(ksi, strHash, &data, &len);
			hasAlg = getHashAlgorithm_throws(strAlg);
			KSI_DataHash_fromDigest_throws(ksi, hasAlg, data, (unsigned int)len, hash);
		}
		CATCH_ALL{
			free(strAlg);
			free(strHash);
			free(data);
			THROW_FORWARD_APPEND_MESSAGE("Error: Unable to get hash from command-line.\n");
		}
	end_try

	free(strAlg);
	free(strHash);
	free(data);
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

int getReturnValue(int error_code){
	switch (error_code) {
		case KSI_OK:
			return EXIT_SUCCESS;
		case KSI_INVALID_ARGUMENT:
			return EXIT_FAILURE;
		case KSI_INVALID_FORMAT:
			return EXIT_INVALID_FORMAT;
		case KSI_UNTRUSTED_HASH_ALGORITHM:
			return EXIT_CRYPTO_ERROR;
		case KSI_UNAVAILABLE_HASH_ALGORITHM:
			return EXIT_CRYPTO_ERROR;
		case KSI_BUFFER_OVERFLOW:
			return EXIT_FAILURE;
		case KSI_TLV_PAYLOAD_TYPE_MISMATCH:
			return EXIT_FAILURE;
		case KSI_ASYNC_NOT_FINISHED:
			return EXIT_FAILURE;
		case KSI_INVALID_SIGNATURE:
			return EXIT_INVALID_FORMAT;
		case KSI_INVALID_PKI_SIGNATURE:
			return EXIT_CRYPTO_ERROR;
		case KSI_PKI_CERTIFICATE_NOT_TRUSTED:
			return EXIT_CRYPTO_ERROR;
		case KSI_OUT_OF_MEMORY:
			return EXIT_OUT_OF_MEMORY;
		case KSI_IO_ERROR:
			return EXIT_IO_ERROR;
		case KSI_NETWORK_ERROR:
			return EXIT_NETWORK_ERROR;
		case KSI_NETWORK_CONNECTION_TIMEOUT:
			return EXIT_NETWORK_ERROR;
		case KSI_NETWORK_SEND_TIMEOUT:
			return EXIT_NETWORK_ERROR;
		case KSI_NETWORK_RECIEVE_TIMEOUT:
			return EXIT_NETWORK_ERROR;
		case KSI_HTTP_ERROR:
			return EXIT_NETWORK_ERROR;
		case KSI_EXTEND_WRONG_CAL_CHAIN:
			return EXIT_EXTEND_ERROR;
		case KSI_EXTEND_NO_SUITABLE_PUBLICATION:
			return EXIT_EXTEND_ERROR;
		case KSI_VERIFICATION_FAILURE:
			return EXIT_VERIFY_ERROR;
		case KSI_INVALID_PUBLICATION:
			return EXIT_INVALID_FORMAT;
		case KSI_PUBLICATIONS_FILE_NOT_SIGNED_WITH_PKI:
			return EXIT_CRYPTO_ERROR;
		case KSI_CRYPTO_FAILURE:
			return EXIT_CRYPTO_ERROR;
		case KSI_HMAC_MISMATCH:
			return EXIT_HMAC_ERROR;
		case KSI_UNKNOWN_ERROR:
			return EXIT_FAILURE;
		case KSI_SERVICE_INVALID_REQUEST:
			return 0;
		/*generic*/
		case KSI_SERVICE_AUTHENTICATION_FAILURE:
			return EXIT_AUTH_FAILURE;
		case KSI_SERVICE_INVALID_PAYLOAD:
			return EXIT_INVALID_FORMAT;
		case KSI_SERVICE_INTERNAL_ERROR:
			return EXIT_FAILURE;
		case KSI_SERVICE_UPSTREAM_ERROR:
			return EXIT_FAILURE;
		case KSI_SERVICE_UPSTREAM_TIMEOUT:
			return EXIT_FAILURE;
		case KSI_SERVICE_UNKNOWN_ERROR:
			return EXIT_FAILURE;
		/*aggre*/
		case KSI_SERVICE_AGGR_REQUEST_TOO_LARGE:
			return EXIT_AGGRE_ERROR;
		case KSI_SERVICE_AGGR_REQUEST_OVER_QUOTA:
			return EXIT_AGGRE_ERROR;
		/*extender*/
		case KSI_SERVICE_EXTENDER_INVALID_TIME_RANGE:
			return EXIT_EXTEND_ERROR;
		case KSI_SERVICE_EXTENDER_DATABASE_MISSING:
			return EXIT_EXTEND_ERROR;
		case KSI_SERVICE_EXTENDER_DATABASE_CORRUPT:
			return EXIT_EXTEND_ERROR;
		case KSI_SERVICE_EXTENDER_REQUEST_TIME_TOO_OLD:
			return EXIT_EXTEND_ERROR;
		case KSI_SERVICE_EXTENDER_REQUEST_TIME_TOO_NEW:
			return EXIT_EXTEND_ERROR;
		default:
			return EXIT_FAILURE;
	}
}

#define THROWABLE3(_ctx,func, ...) \
	int res = KSI_UNKNOWN_ERROR; \
	int extError = KSI_UNKNOWN_ERROR; \
	char buf[1024]; \
	char buf2[1024]; \
	\
	res = func;  \
	if(res != KSI_OK) { \
		KSI_ERR_getBaseErrorMessage(_ctx, buf, sizeof(buf),&res, &extError); \
		if(extError) \
			snprintf(buf2, sizeof(buf2), "Error: %s (KSI:0x%x, EXT:0x%x)", buf, res, extError); \
		else \
			snprintf(buf2, sizeof(buf2), "Error: %s (KSI:0x%x)", buf, res); \
		appendMessage(buf2); \
		THROW_MSG(KSI_EXCEPTION,getReturnValue(res), __VA_ARGS__); \
	} \
	return res;	 \
	
int KSI_receivePublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile **publicationsFile){
	THROWABLE3(ksi, KSI_receivePublicationsFile(ksi, publicationsFile), "Error: Unable to read publications file.");
}

int KSI_verifyPublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile){
	THROWABLE3(ksi, KSI_verifyPublicationsFile(ksi, publicationsFile), "Error: Unable to verify publications file.");
}

int KSI_PublicationsFile_serialize_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile, char **raw, int *raw_len){
	THROWABLE3(ksi, KSI_PublicationsFile_serialize(ksi, publicationsFile, raw, raw_len), "Error: Unable serialize publications file.");
}

int KSI_DataHasher_open_throws(KSI_CTX *ksi,int hasAlgID ,KSI_DataHasher **hsr){
	THROWABLE3(ksi, KSI_DataHasher_open(ksi, hasAlgID, hsr), "Error: Unable to create hasher.");
}

int KSI_DataHasher_add_throws(KSI_CTX *ksi, KSI_DataHasher *hasher, const void *data, size_t data_length){
	THROWABLE3(ksi, KSI_DataHasher_add(hasher, data, data_length), "Error: Unable to add data to hasher.");
}

int KSI_DataHasher_close_throws(KSI_CTX *ksi, KSI_DataHasher *hasher, KSI_DataHash **hash){
	 THROWABLE3(ksi, KSI_DataHasher_close(hasher, hash), "Error:Unable to close hasher.");
}

int KSI_createSignature_throws(KSI_CTX *ksi, KSI_DataHash *hash, KSI_Signature **sign){
	 THROWABLE3(ksi, KSI_createSignature(ksi, hash, sign), "Error: Unable to sign.");
}
int KSI_DataHash_fromDigest_throws(KSI_CTX *ksi, int hasAlg, const unsigned char *digest, unsigned int len, KSI_DataHash **hash){
	THROWABLE3(ksi, KSI_DataHash_fromDigest(ksi, hasAlg, digest, len, hash), "Error: Unable to create hash from digest.");
}

int KSI_PublicationsFile_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_PublicationsFile **pubFile){
	THROWABLE3(ksi, KSI_PublicationsFile_fromFile(ksi, fileName, pubFile), "Error: Unable to read publications file (%s).\n", fileName)
}

int KSI_Signature_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_Signature **sig){
	THROWABLE3(ksi, KSI_Signature_fromFile(ksi, fileName, sig), "Error: Unable to read signature from file.");
}

int KSI_Signature_verify_throws(KSI_Signature *sig, KSI_CTX *ksi){
	THROWABLE3(ksi, KSI_Signature_verify(sig, ksi), "Error: Unable to verify signature.");
}

int KSI_Signature_create_throws(KSI_CTX *ksi, KSI_DataHash *hsh, KSI_Signature **signature){
	THROWABLE3(ksi, KSI_Signature_create(ksi, hsh, signature), "Error: Unable to create signature.");
}

int KSI_Signature_createDataHasher_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHasher **hsr){
	THROWABLE3(ksi, KSI_Signature_createDataHasher(sig, hsr), "Error: Unable to create data hasher.");
}

int KSI_Signature_verifyDataHash_throws(KSI_Signature *sig, KSI_CTX *ksi, KSI_DataHash *hash){
	THROWABLE3(ksi, KSI_Signature_verifyDataHash(sig, ksi,  hash), "Error: Wrong document or signature.");
}
/*To earliest available publication available*/
int KSI_extendSignature_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext){
	THROWABLE3(ksi, KSI_extendSignature(ksi, sig, ext),"Error: Unable to extend signature.");
}
/*To Publication record*/
int KSI_Signature_extend_throws(const KSI_Signature *signature, KSI_CTX *ksi, const KSI_PublicationRecord *pubRec, KSI_Signature **extended){
	THROWABLE3(ksi, KSI_Signature_extend(signature, ksi, pubRec, extended), "Error: Unable to extend signature.");
}

int KSI_PKITruststore_addLookupFile_throws(KSI_CTX *ksi, KSI_PKITruststore *store, const char *path){
	THROWABLE3(ksi, KSI_PKITruststore_addLookupFile(store,path), "Error: Unable to set PKI trust store lookup file.");
}
		
int KSI_PKITruststore_addLookupDir_throws(KSI_CTX *ksi, KSI_PKITruststore *store, const char *path){
	THROWABLE3(ksi, KSI_PKITruststore_addLookupDir(store,path), "Error: Unable to set PKI trust store lookup directory.");
}

int KSI_Integer_new_throws(KSI_CTX *ksi, KSI_uint64_t value, KSI_Integer **kint){
	THROWABLE3(ksi, KSI_Integer_new(ksi, value, kint), "Error: Unable to construct KSI Integer.");
}

int KSI_ExtendReq_new_throws(KSI_CTX *ksi, KSI_ExtendReq **t){
	THROWABLE3(ksi, KSI_ExtendReq_new(ksi, t);, "Error: Unable to construct KSI extend request.");
}

int KSI_ExtendReq_setAggregationTime_throws(KSI_CTX *ksi, KSI_ExtendReq *t, KSI_Integer *aggregationTime){
	THROWABLE3(ksi, KSI_ExtendReq_setAggregationTime(t, aggregationTime);, "Error: Unable to set request aggregation time.");
}

int KSI_ExtendReq_setPublicationTime_throws(KSI_CTX *ksi, KSI_ExtendReq *t, KSI_Integer *publicationTime){
	THROWABLE3(ksi, KSI_ExtendReq_setPublicationTime(t, publicationTime), "Error: Unable to set request publication time.");
}

int KSI_ExtendReq_setRequestId_throws(KSI_CTX *ksi, KSI_ExtendReq *t, KSI_Integer *requestId){
	THROWABLE3(ksi, KSI_ExtendReq_setRequestId(t, requestId), "Error: Unable to set request ID.");
}

int KSI_sendExtendRequest_throws(KSI_CTX *ksi, KSI_ExtendReq *request, KSI_RequestHandle **handle){
	THROWABLE3(ksi, KSI_sendExtendRequest(ksi, request, handle), "Error: Unable to send extend request.");
}

int KSI_RequestHandle_getExtendResponse_throws(KSI_CTX *ksi, KSI_RequestHandle *handle, KSI_ExtendResp **resp){
	THROWABLE3(ksi, KSI_RequestHandle_getExtendResponse(handle, resp), "Error: Unable to get extend request.");
}

int KSI_ExtendResp_getStatus_throws(KSI_CTX *ksi, const KSI_ExtendResp *t, KSI_Integer **status){
	THROWABLE3(ksi, KSI_ExtendResp_getStatus(t, status), "Error: Unable to get request response status.");
}

int KSI_ExtendResp_getCalendarHashChain_throws(KSI_CTX *ksi, const KSI_ExtendResp *t, KSI_CalendarHashChain **calendarHashChain){
	THROWABLE3(ksi, KSI_ExtendResp_getCalendarHashChain(t, calendarHashChain), "Error: Unable to get calendar hash chain.");
}

int KSI_CalendarHashChain_aggregate_throws(KSI_CTX *ksi, KSI_CalendarHashChain *chain, KSI_DataHash **hsh){
	THROWABLE3(ksi, KSI_CalendarHashChain_aggregate(chain, hsh), "Error: Unable to aggregate.");
}

int KSI_CalendarHashChain_getPublicationTime_throws(KSI_CTX *ksi, const KSI_CalendarHashChain *t, KSI_Integer **publicationTime){
	THROWABLE3(ksi, KSI_CalendarHashChain_getPublicationTime(t, publicationTime), "Error: Unable to get publications time.");
}

int KSI_CalendarHashChain_setPublicationTime_throws(KSI_CTX *ksi, KSI_CalendarHashChain *t, KSI_Integer *publicationTime){
	THROWABLE3(ksi, KSI_CalendarHashChain_setPublicationTime(t, publicationTime), "Error: Unable to set publications time.");
}

int KSI_PublicationData_new_throws(KSI_CTX *ksi, KSI_PublicationData **t){
	THROWABLE3(ksi, KSI_PublicationData_new(ksi, t), "Error: Unable to construct publication data.");
}

int KSI_PublicationData_setImprint_throws(KSI_CTX *ksi, KSI_PublicationData *t, KSI_DataHash *imprint){
	THROWABLE3(ksi, KSI_PublicationData_setImprint(t,imprint), "Error: Unable to set publication data imprint.");
}

int KSI_PublicationData_setTime_throws(KSI_CTX *ksi, KSI_PublicationData *t, KSI_Integer *time){
	THROWABLE3(ksi, KSI_PublicationData_setTime(t, time), "Error: Unable to set publication data time attribute.");
}

int KSI_PublicationData_toBase32_throws(KSI_CTX *ksi, const KSI_PublicationData *published_data, char **publication){
	THROWABLE3(ksi, KSI_PublicationData_toBase32(published_data, publication), "Error: Unable to convert publication data to base 32.");
}

int KSI_Signature_clone_throws(KSI_CTX *ksi, const KSI_Signature *sig, KSI_Signature **clone){
	THROWABLE3(ksi, KSI_Signature_clone(sig, clone), "Error: Unable to clone signature.");
}

int KSI_Signature_getSigningTime_throws(KSI_CTX *ksi, const KSI_Signature *sig, KSI_Integer **signTime){
	THROWABLE3(ksi, KSI_Signature_getSigningTime(sig, signTime), "Error: Unable to get signatures signing time.");
}

int KSI_ExtendResp_setCalendarHashChain_throws(KSI_CTX *ksi, KSI_ExtendResp *t, KSI_CalendarHashChain *calendarHashChain){
	THROWABLE3(ksi, KSI_ExtendResp_setCalendarHashChain(t, calendarHashChain), "Error: Unable to get extension response signing time.");
}

int KSI_Signature_replaceCalendarChain_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_CalendarHashChain *calendarHashChain){
	THROWABLE3(ksi, KSI_Signature_replaceCalendarChain(sig,calendarHashChain), "Error: Unable to replace signatures calender hash chain.");
}

int KSI_PublicationsFile_getPublicationDataByTime_throws(KSI_CTX *ksi, const KSI_PublicationsFile *pubFile, const KSI_Integer *pubTime, KSI_PublicationRecord **pubRec){
	THROWABLE3(ksi, KSI_PublicationsFile_getPublicationDataByTime(pubFile, pubTime, pubRec), "Error: Unable to get publication date by time.");
}

int KSI_Signature_replacePublicationRecord_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_PublicationRecord *pubRec){
	THROWABLE3(ksi, KSI_Signature_replacePublicationRecord(sig, pubRec), "Error: Unable set signatures publication record.");
}

int KSI_PublicationRecord_clone_throws(KSI_CTX *ksi, const KSI_PublicationRecord *rec, KSI_PublicationRecord **clone){
	THROWABLE3(ksi, KSI_PublicationRecord_clone(rec, clone), "Error: Unable clone signatures publication record.");
}

int KSI_setPublicationCertEmail_throws(KSI_CTX *ksi, const char *email){
	THROWABLE3(ksi, KSI_setPublicationCertEmail(ksi, email), "Error: Unable set publication certificate email.");
}

int KSI_NetworkClient_setExtenderUser_throws(KSI_CTX *ksi, KSI_NetworkClient *netProvider, const char *val){
	THROWABLE3(ksi,KSI_NetworkClient_setExtenderUser(netProvider, val) , "Error: Unable set extender user name.");
}

int KSI_NetworkClient_setExtenderPass_throws(KSI_CTX *ksi, KSI_NetworkClient *netProvider, const char *val){
	THROWABLE3(ksi,KSI_NetworkClient_setExtenderPass(netProvider, val) , "Error: Unable set extender password.");
}

int KSI_NetworkClient_setAggregatorUser_throws(KSI_CTX *ksi, KSI_NetworkClient *netProvider, const char *val){
	THROWABLE3(ksi,KSI_NetworkClient_setAggregatorUser(netProvider, val) , "Error: Unable set aggregator user name.");
}

int KSI_NetworkClient_setAggregatorPass_throws(KSI_CTX *ksi, KSI_NetworkClient *netProvider, const char *val){
	THROWABLE3(ksi, KSI_NetworkClient_setAggregatorPass(netProvider, val) , "Error: Unable set aggregator password.");
}

int KSI_CTX_setPublicationUrl_throws(KSI_CTX *ksi, const char *uri){
	THROWABLE3(ksi, KSI_CTX_setPublicationUrl(ksi, uri) , "Error: Unable set publication URL.");
}

int KSI_CTX_setExtender_throws(KSI_CTX *ksi, const char *uri, const char *loginId, const char *key){
	THROWABLE3(ksi, KSI_CTX_setExtender(ksi, uri, loginId, key) , "Error: Unable set extender.");
}

int KSI_CTX_setAggregator_throws(KSI_CTX *ksi, const char *uri, const char *loginId, const char *key){
	THROWABLE3(ksi, KSI_CTX_setAggregator(ksi, uri, loginId, key) , "Error: Unable set aggregator.");
}

int KSI_CTX_setTransferTimeoutSeconds_throws(KSI_CTX *ksi, int timeout){
	THROWABLE3(ksi, KSI_CTX_setTransferTimeoutSeconds(ksi, timeout) , "Error: Unable set transfer timeout.");
}
int KSI_CTX_setConnectionTimeoutSeconds_throws(KSI_CTX *ksi, int timeout){
	THROWABLE3(ksi, KSI_CTX_setConnectionTimeoutSeconds(ksi, timeout) , "Error: Unable set connection timeout.");
}
