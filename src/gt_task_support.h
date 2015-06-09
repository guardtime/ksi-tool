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
#include "ksitool_err.h"
#include "printer.h"


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
int initTask(Task *task ,KSI_CTX **ksi, ERR_TRCKR **err);


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
int getFilesHash(KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash);

/**
 * Saves signature object to file.
 * @param [in] sign Pointer to signature object for saving.
 * @param [in] fname Pointer to file path.
 * 
 * @throws KSI_EXCEPTION, IO_EXCEPTION.
 */
int saveSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sign, const char *fname);

int savePublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_PublicationsFile *pubfile, const char *fname);

bool isSignatureExtended(const KSI_Signature *sig);

int loadPublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_PublicationsFile **pubfile);
int loadSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_Signature **sig);

bool isPiping(paramSet *set);
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

void printSignatureStructure(KSI_CTX *ksi, const KSI_Signature *sig);

/**
 * Reads hash from command line and creates the KSI_DataHash object.
 * 
 * @param[in] cmdparam Pointer to command line data object.
 * @param[in] ksi Pointer to ksi context object.
 * @param[out] hash Pointer to receiving pointer to KSI_DataHash object.
 * 
 * @throws KSI_EXCEPTION, INVALID_ARGUMENT_EXCEPTION.
 */
int getHashFromCommandLine(const char *imprint, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hash);
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
 * KSI api functions capable throwing exceptions  *
 *************************************************/

int ksi_error_wrapper(ERR_TRCKR *err, KSI_CTX *ksi, int res, const char *file, unsigned line, char *msg, ...);

#define ERR_CATCH_KSI(ksi, msg, ...) if (ksi_error_wrapper(err, ksi, res, __FILE__, __LINE__, msg, __VA_ARGS__) != KSI_OK) goto cleanup
//#define KSI_WRAPPER_GTC(err, ksi, func, msg, ...) if (func != KSI_OK) goto cleanup




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

