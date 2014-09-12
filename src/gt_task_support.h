#ifndef GT_TASK_H
#define	GT_TASK_H

#include <stdio.h>
#include <string.h>
#include <ksi/ksi.h>
#include "gt_cmd_parameters.h"




#ifdef	__cplusplus
extern "C" {
#endif

    
/**
 * Configures KSI using parameters extracted from command line. 
 * 
 * @param[in] cmdparam Pointer to command line parameter object.
 * @param[out] ksi Pointer to receiving pointer to KSI context object.
 * 
 * @throws KSI_EXEPTION.
 */
void initTask_throws(GT_CmdParameters *cmdparam ,KSI_CTX **ksi);

/**
 * Calculates the hash of an input file.
 * @param[in] hsr Pointer to hasher object. 
 * @param[in] fname Pointer to file path.
 * @param[out] hash Pointer to the receiving pointer to the KSI hash object.
 * 
 * @throws KSI_EXEPTION, IO_EXEPTION.
 */
void getFilesHash_throws(KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash ); 

/**
 * Saves signature object to file.
 * @param [in] sign Pointer to signature object for saving.
 * @param [in] fname Pointer to file path.
 * 
 * @throws KSI_EXEPTION, IO_EXEPTION.
 */
void saveSignatureFile_throws(KSI_Signature *sign, const char *fname);


/**
 * Prints the signer identity. If sig == NULL dose nothing.
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
 * Prints signatures publication references. If sig == NULL dose nothing.
 * @param[in] sig Pointer to KSI signature object.
 * 
 */
void printSignaturePublicationReference(const KSI_Signature *sig);

/**
 * Prints signature verification info.  If sig == NULL dose nothing.
 * @param[in] sig Pointer to KSI signature object.
 */
void printSignatureVerificationInfo(const KSI_Signature *sig);

void printPublicationsFileCertificates(const KSI_PublicationsFile *pubfile);

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


/*************************************************
 * KSI api functions capable throwing exeptions  *
 *************************************************/

int KSI_receivePublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile **publicationsFile);
int KSI_verifyPublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile);
int KSI_PublicationsFile_serialize_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile, char **raw, int *raw_len);
int KSI_DataHasher_open_throws(KSI_CTX *ksi,int hasAlgID ,KSI_DataHasher **hsr);
int KSI_createSignature_throws(KSI_CTX *ksi, const KSI_DataHash *hash, KSI_Signature **sign);
int KSI_DataHash_fromDigest_throws(KSI_CTX *ksi, int hasAlg, char *data, unsigned int len, KSI_DataHash **hash);
int KSI_PublicationsFile_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_PublicationsFile **pubFile);
int KSI_Signature_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_Signature **sig);
int KSI_Signature_verify_throws(KSI_Signature *sig, KSI_CTX *ksi);
int KSI_Signature_createDataHasher_throws(KSI_Signature *sig, KSI_DataHasher **hsr);
int KSI_Signature_verifyDataHash_throws(KSI_Signature *sig, KSI_CTX *ksi, KSI_DataHash *hash);
int KSI_extendSignature_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext);
int KSI_Signature_extend_throws(const KSI_Signature *signature, KSI_CTX *ctx, const KSI_PublicationRecord *pubRec, KSI_Signature **extended);
int KSI_PKITruststore_addLookupFile_throws(KSI_PKITruststore *store, const char *path);
int KSI_PKITruststore_addLookupDir_throws(KSI_PKITruststore *store, const char *path);

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

