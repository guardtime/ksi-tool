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

#ifndef GT_TASK_SUPPORT_H
#define	GT_TASK_SUPPORT_H

#ifdef _WIN32
#ifdef _DEBUG
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif	
#endif	


#include <stdio.h>
#include <string.h>
#include <ksi/ksi.h>
#include "task_def.h"
#include <stdlib.h>
#include "gt_cmd_common.h"



#ifdef	__cplusplus
extern "C" {
#endif

typedef enum tasks_en{
	noTask = 0,
	downloadPublicationsFile,
	createPublicationString,
	signDataFile,
	signHash,
	extendTimestamp,
	verifyPublicationsFile,
	verifyTimestamp,
	verifyTimestampOnline,
	getRootH_T,
	setSysTime,
	showHelp,
	invalid		
} TaskID;
    
/**
 * Configures KSI using parameters extracted from command line. 
 * 
 * @param[in] task Pointer to task object.
 * @param[out] ksi Pointer to receiving pointer to KSI context object.
 * 
 * @throws KSI_EXCEPTION.
 */
void initTask_throws(Task *task ,KSI_CTX **ksi);

/**
 * Closes KSI and logging file.
 * @param[in] task Pointer to task object.
 * @param[in] ksi Pointer to receiving pointer to KSI context object.
 */
void closeTask(KSI_CTX *ksi);

/**
 * Calculates the hash of an input file.
 * @param[in] ksi KSI context.
 * @param[in] hsr Pointer to hasher object. 
 * @param[in] fname Pointer to file path.
 * @param[out] hash Pointer to the receiving pointer to the KSI hash object.
 * 
 * @throws KSI_EXCEPTION, IO_EXCEPTION.
 */
void getFilesHash_throws(KSI_CTX *ksi, KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash); 

/**
 * Saves signature object to file.
 * @param [in] sign Pointer to signature object for saving.
 * @param [in] fname Pointer to file path.
 * 
 * @throws KSI_EXCEPTION, IO_EXCEPTION.
 */
void saveSignatureFile_throws(KSI_Signature *sign, const char *fname);

bool isSignatureExtended(const KSI_Signature *sig);

/**
 * Prints the signer identity. If sig == NULL does nothing.
 * @param[in] sig Pointer to KSI signature object.
 * 
 */
void printSignerIdentity(KSI_Signature *sig);

/**
 * Prints publications file publications references.
 *  [date]
 *   list of references
 * 
 * @param[in] pubFile Pointer to KSI publications file object.
 * 
 */
void printPublicationsFileReferences(const KSI_PublicationsFile *pubFile);

/**
 * Prints signatures publication references. If sig == NULL does nothing.
 * @param[in] sig Pointer to KSI signature object.
 * 
 */
void printSignaturePublicationReference(const KSI_Signature *sig);

/**
 * Prints signature verification info.  If sig == NULL does nothing.
 * @param[in] sig Pointer to KSI signature object.
 */
void printSignatureVerificationInfo(const KSI_Signature *sig);

void printPublicationsFileCertificates(const KSI_PublicationsFile *pubfile);

void printSignaturesRootHash_n_Time(const KSI_Signature *sig);

void printSignatureSigningTime(const KSI_Signature *sig);

/**
 * Gives hash algorithm identifier by name.
 * 
 * @param[in] hashAlg Hash algorithm name.
 * @return Hash algorithm identifier.
 * 
 * @throws KSI_EXCEPTION.
 */
int getHashAlgorithm_throws(const char *hashAlg);

/**
 * Reads hash from command line and creates the KSI_DataHash object.
 * 
 * @param[in] cmdparam Pointer to command line data object.
 * @param[in] ksi Pointer to ksi context object.
 * @param[out] hash Pointer to receiving pointer to KSI_DataHash object.
 * 
 * @throws KSI_EXCEPTION, INVALID_ARGUMENT_EXCEPTION.
 */
void getHashFromCommandLine_throws(const char *imprint,KSI_CTX *ksi, KSI_DataHash **hash);

/**
 * Gives time difference between the current and last call in ms.
 * @return Time difference in ms.
 */
unsigned int measureLastCall(void);
/**
 * Gives time difference between the current and last call of function measureLastCall in ms
 * @return Time difference in ms.
 */
unsigned int measuredTime(void);

/**
 * Gives a string representing string value of measured time by measureLastCall.
 * String format is (%i ms).
 * @return The pointer to the string
 * 
 * @note Pointer is always pointing to the same memory fields. 
 */
char* str_measuredTime(void);

int getReturnValue(int error_code);


/*************************************************
 * KSI api functions capable throwing exceptions  *
 *************************************************/

int KSI_receivePublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile **publicationsFile);
int KSI_verifyPublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile);
int KSI_PublicationsFile_serialize_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile, char **raw, unsigned *raw_len);
int KSI_DataHasher_open_throws(KSI_CTX *ksi,int hasAlgID ,KSI_DataHasher **hsr);
int KSI_DataHasher_add_throws(KSI_CTX *ksi, KSI_DataHasher *hasher, const void *data, size_t data_length);
int KSI_DataHasher_close_throws(KSI_CTX *ksi, KSI_DataHasher *hasher, KSI_DataHash **hash);
int KSI_createSignature_throws(KSI_CTX *ksi, KSI_DataHash *hash, KSI_Signature **sign);
int KSI_DataHash_fromDigest_throws(KSI_CTX *ksi, int hasAlg, const unsigned char *digest, unsigned int len, KSI_DataHash **hash);
int KSI_PublicationsFile_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_PublicationsFile **pubFile);
int KSI_Signature_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_Signature **sig);
int KSI_Signature_verify_throws(KSI_Signature *sig, KSI_CTX *ksi);
int KSI_Signature_verifyWithPublication_throws(KSI_Signature *sig, KSI_CTX *ksi, const KSI_PublicationData *publication);
int KSI_Signature_verifyOnline_throws(KSI_CTX *ksi, KSI_Signature *sig);
int KSI_Signature_createDataHasher_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_DataHasher **hsr);
int KSI_Signature_create_throws(KSI_CTX *ksi, KSI_DataHash *hsh, KSI_Signature **signature);
int KSI_Signature_verifyDataHash_throws(KSI_Signature *sig, KSI_CTX *ksi, KSI_DataHash *hash);
int KSI_extendSignature_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext);
int KSI_Signature_extendTo_throws(const KSI_Signature *signature, KSI_CTX *ksi, KSI_Integer *to, KSI_Signature **extended);
int KSI_Signature_extend_throws(const KSI_Signature *signature, KSI_CTX *ksi, const KSI_PublicationRecord *pubRec, KSI_Signature **extended);
int KSI_PKITruststore_addLookupFile_throws(KSI_CTX *ksi, KSI_PKITruststore *store, const char *path);
int KSI_PKITruststore_addLookupDir_throws(KSI_CTX *ksi, KSI_PKITruststore *store, const char *path);
int KSI_Integer_new_throws(KSI_CTX *ksi, KSI_uint64_t value, KSI_Integer **kint);
int KSI_ExtendReq_new_throws(KSI_CTX *ksi, KSI_ExtendReq **t);
int KSI_ExtendReq_setAggregationTime_throws(KSI_CTX *ksi, KSI_ExtendReq *t, KSI_Integer *aggregationTime);
int KSI_ExtendReq_setPublicationTime_throws(KSI_CTX *ksi, KSI_ExtendReq *t, KSI_Integer *publicationTime);
int KSI_ExtendReq_setRequestId_throws(KSI_CTX *ksi, KSI_ExtendReq *t, KSI_Integer *requestId);
int KSI_sendExtendRequest_throws(KSI_CTX *ksi, KSI_ExtendReq *request, KSI_RequestHandle **handle);
int KSI_RequestHandle_getExtendResponse_throws(KSI_CTX *ksi, KSI_RequestHandle *handle, KSI_ExtendResp **resp);
int KSI_ExtendResp_getStatus_throws(KSI_CTX *ksi, const KSI_ExtendResp *t, KSI_Integer **status);
int KSI_ExtendResp_getCalendarHashChain_throws(KSI_CTX *ksi, const KSI_ExtendResp *t, KSI_CalendarHashChain **calendarHashChain);
int KSI_CalendarHashChain_aggregate_throws(KSI_CTX *ksi, KSI_CalendarHashChain *chain, KSI_DataHash **hsh);
int KSI_CalendarHashChain_getPublicationTime_throws(KSI_CTX *ksi, const KSI_CalendarHashChain *t, KSI_Integer **publicationTime);
int KSI_CalendarHashChain_setPublicationTime_throws(KSI_CTX *ksi, KSI_CalendarHashChain *t, KSI_Integer *publicationTime);
int KSI_PublicationData_new_throws(KSI_CTX *ksi, KSI_PublicationData **t);
int KSI_PublicationData_setImprint_throws(KSI_CTX *ksi, KSI_PublicationData *t, KSI_DataHash *imprint);
int KSI_PublicationData_setTime_throws(KSI_CTX *ksi, KSI_PublicationData *t, KSI_Integer *time);
int KSI_PublicationData_toBase32_throws(KSI_CTX *ksi, const KSI_PublicationData *published_data, char **publication);
int KSI_PublicationData_fromBase32_throws(KSI_CTX *ksi, const char *publication, KSI_PublicationData **published_data);
int KSI_Signature_getSigningTime_throws(KSI_CTX *ksi, const KSI_Signature *sig, KSI_Integer **signTime);
int KSI_PublicationRecord_clone_throws(KSI_CTX *ksi, const KSI_PublicationRecord *rec, KSI_PublicationRecord **clone);
int KSI_CTX_setPublicationCertEmail_throws(KSI_CTX *ksi, const char *email);
int KSI_CTX_setPublicationUrl_throws(KSI_CTX *ksi, const char *uri);
int KSI_CTX_setAggregator_throws(KSI_CTX *ksi, const char *uri, const char *loginId, const char *key);
int KSI_CTX_setExtender_throws(KSI_CTX *ksi, const char *uri, const char *loginId, const char *key);
int KSI_CTX_setTransferTimeoutSeconds_throws(KSI_CTX *ksi, int timeout);
int KSI_CTX_setConnectionTimeoutSeconds_throws(KSI_CTX *ksi, int timeout);

int KSI_PublicationsFile_getPublicationDataByPublicationString_throws(KSI_CTX *ksi, const KSI_PublicationsFile *pubFile, const char *pubString, KSI_PublicationRecord **pubRec);
int KSI_PublicationRecord_getPublishedData_throws(KSI_CTX *ksi, const KSI_PublicationRecord *t, KSI_PublicationData **publishedData);
int KSI_PublicationRecord_setPublishedData_throws(KSI_CTX *ksi, KSI_PublicationRecord *t, KSI_PublicationData *publishedData);
int KSI_Signature_verifyWithPublication_throws(KSI_Signature *sig, KSI_CTX *ksi, const KSI_PublicationData *publication);
int KSI_PublicationData_getTime_throws(KSI_CTX *ksi, const KSI_PublicationData *t, KSI_Integer **time);
int KSI_PublicationRecord_new_throws(KSI_CTX *ksi, KSI_PublicationRecord **t);
int KSI_Signature_getPublicationRecord_throws(KSI_CTX *ksi, const KSI_Signature *sig, KSI_PublicationRecord **pubRec);

#define MEASURE_TIME(code_here) \
	{   \
	measureLastCall(); \
	code_here; \
	measureLastCall(); \
	}


#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

