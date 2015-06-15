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
 * @return KT_OK if successful, error code otherwise.
 */
int initTask(Task *task ,KSI_CTX **ksi, ERR_TRCKR **err);


/**
 * Closes KSI and logging file.
 * @param[in] task	Pointer to task object.
 * @param[in] ksi	Pointer to receiving pointer to KSI context object.
 */
void closeTask(KSI_CTX *ksi);

/**
 * Calculates the hash of an input file.
 *
 * @param err	Error tracker.
 * @param ksi	KSI Context.
 * @param hsr	Hasher for file hashing.
 * @param fname	Files name to be hashed.
 * @param hash	Hash value of the file.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int getFilesHash(ERR_TRCKR *err, KSI_CTX *ksi, KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash);

/**
 * Saves signature object to a file. If files name is -, file is written to stdout.
 *
 * @param err	Error tracker.
 * @param ksi	KSI Context.
 * @param sign	Signature object.
 * @param fname	Files name where the signature is saved.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int saveSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sign, const char *fname);

/**
 * Saves publication file object to a file. If files name is -, file is written to stdout.
 *
 * @param err		Error tracker.
 * @param ksi		KSI Context.
 * @param pubfile	Publication file object.
 * @param fname		Files name where the signature is saved.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int savePublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_PublicationsFile *pubfile, const char *fname);

/**
 * Reads publication file from file. If files name is -, file is read from stdin.
 *
 * @param err		Error tracker.
 * @param ksi		KSI Context.
 * @param fname		Files name where the signature is saved.
 * @param pubfile	Publication file object.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int loadPublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_PublicationsFile **pubfile);

/**
 * Reads signature file from file. If files name is -, file is read from stdin.
 *
 * @param err		Error tracker.
 * @param ksi		KSI Context.
 * @param fname		Files name where the signature is saved.
 * @param sig		Signature file object.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int loadSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_Signature **sig);

/**
 * Returns true if signature is extended. False if not.
 */
bool isSignatureExtended(const KSI_Signature *sig);

/*Controls parameter set and returns true if user wants to write data files to stdout*/
bool isPiping(paramSet *set);

void printSignerIdentity(KSI_Signature *sig);
void printPublicationsFileReferences(const KSI_PublicationsFile *pubFile);
void printSignaturePublicationReference(const KSI_Signature *sig);
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

void print_progressDesc(bool showTiming, const char *msg, ...);

void print_progressResult(int res);
/*************************************************
 * KSI api functions capable throwing exceptions  *
 *************************************************/

#define ERR_CATCH_MSG(err, res, msg, ...) \
	if (res != KT_OK) { \
		ERR_TRCKR_add(err, res, __FILE__, __LINE__, msg, ##__VA_ARGS__); \
		goto cleanup; \
	}

#define ERR_APPEND_KSI_ERR(err, res, ref_err) \
		if (res == ref_err) { \
			ERR_TRCKR_add(err, res, __FILE__, __LINE__, "Error: %s", KSI_getErrorString(res)); \
		}

#define ERR_APPEND_KSI_BASE_ERR(err, res, ksi) \
		if (res != KSI_OK) { \
			char buf[1024]; \
			KSI_ERR_getBaseErrorMessage(ksi, buf, sizeof(buf), NULL, NULL); \
			if (buf[0] != 0) \
				ERR_TRCKR_add(err, res, __FILE__, __LINE__, "Error: %s", buf); \
		}

#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

