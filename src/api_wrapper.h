/*
 * Copyright 2013-2016 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#ifndef API_WRAPPER_H
#define	API_WRAPPER_H

#include "ksitool_err.h"
#include <ksi/ksi.h>
#include <ksi/blocksigner.h>
#include <ksi/policy.h>
#include "err_trckr.h"

#ifdef	__cplusplus
extern "C" {
#endif



#define ERR_CATCH_MSG(err, res, msg, ...) \
	if (res != KT_OK) { \
		ERR_TRCKR_add(err, res, __FILE__, __LINE__, msg, ##__VA_ARGS__); \
		goto cleanup; \
	}

#define ERR_APPEND_KSI_ERR(err, res, ref_err) \
		if (res == ref_err) { \
			ERR_TRCKR_add(err, res, __FILE__, __LINE__, "Error: %s", KSI_getErrorString(res)); \
		}

int KSITOOL_extendSignature(ERR_TRCKR *err, KSI_CTX *ctx, KSI_Signature *sig, KSI_PublicationsFile* pubfile, KSI_Signature **ext);
int KSITOOL_Signature_extendTo(ERR_TRCKR *err, const KSI_Signature *signature, KSI_CTX *ctx, KSI_Integer *to, KSI_Signature **extended);
int KSITOOL_Signature_extend(ERR_TRCKR *err, const KSI_Signature *signature, KSI_CTX *ctx, const KSI_PublicationRecord *pubRec, KSI_Signature **extended);
int KSITOOL_RequestHandle_getExtendResponse(ERR_TRCKR *err, KSI_CTX *ctx, KSI_RequestHandle *handle, KSI_ExtendResp **resp);
int KSITOOL_Extender_getConf(ERR_TRCKR *err, KSI_CTX *ctx, KSI_Config **config);
int KSITOOL_Aggregator_getConf(ERR_TRCKR *err, KSI_CTX *ctx, KSI_Config **config);
int KSITOOL_Signature_isCalendarAuthRecPresent(const KSI_Signature *sig);
int KSITOOL_Signature_isPublicationRecordPresent(const KSI_Signature *sig);
int KSITOOL_KSI_BlockSigner_new(ERR_TRCKR *err, KSI_CTX *ctx, KSI_HashAlgorithm algoId, KSI_DataHash *prevLeaf, KSI_OctetString *initVal, KSI_BlockSigner **signer);
int KSITOOL_BlockSigner_closeAndSign(ERR_TRCKR *err, KSI_CTX *ctx, KSI_BlockSigner *signer);
int KSITOOL_BlockSigner_addLeaf(ERR_TRCKR *err, KSI_CTX *ctx, KSI_BlockSigner *signer, KSI_DataHash *hsh, int level, KSI_MetaData *metaData, KSI_BlockSignerHandle **handle);
int KSITOOL_receivePublicationsFile(ERR_TRCKR *err ,KSI_CTX *ctx, KSI_PublicationsFile **pubFile);
int KSITOOL_verifyPublicationsFile(ERR_TRCKR *err, KSI_CTX *ctx, KSI_PublicationsFile *pubfile);
void KSITOOL_KSI_ERRTrace_save(KSI_CTX *ctx);
const char *KSITOOL_KSI_ERRTrace_get(void);
void KSITOOL_KSI_ERRTrace_LOG(KSI_CTX *ksi);

int KSITOOL_SignatureVerify_general(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh, KSI_PublicationsFile* pubFile, KSI_PublicationData *pubdata, int extperm, KSI_PolicyVerificationResult **result);
int KSITOOL_SignatureVerify_internally(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh, KSI_PolicyVerificationResult **result);
int KSITOOL_SignatureVerify_calendarBased(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh, KSI_PolicyVerificationResult **result);
int KSITOOL_SignatureVerify_keyBased(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh, KSI_PublicationsFile* pubFile, KSI_PolicyVerificationResult **result);
int KSITOOL_SignatureVerify_publicationsFileBased(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh, KSI_PublicationsFile* pubFile, int extperm, KSI_PolicyVerificationResult **result);
int KSITOOL_SignatureVerify_userProvidedPublicationBased(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh, KSI_PublicationData *pubdata, int extperm, KSI_PolicyVerificationResult **result);
int KSITOOL_SignatureVerify_with_publications_file_or_calendar(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh, KSI_PublicationsFile* pubFile, int extperm, KSI_PolicyVerificationResult **result);

char *KSITOOL_DataHash_toString(KSI_DataHash *hsh, char *buf, size_t buf_len);
char *KSITOOL_PublicationData_toString(KSI_PublicationData *data, char *buf, size_t buf_len);
char *KSITOOL_PublicationRecord_toString(KSI_PublicationRecord *rec, char *buf, size_t buf_len);

int KSI_OBJ_saveSignature(ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sign, const char *mode, const char *fname, char *f, size_t f_len);
int KSI_OBJ_savePublicationsFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_PublicationsFile *pubfile, const char *mode, const char *fname) ;
int KSI_OBJ_loadSignature(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, const char* mode, KSI_Signature **sig);
int KSI_OBJ_isSignatureExtended(const KSI_Signature *sig);

int KSITOOL_LOG_SmartFile(void *logCtx, int logLevel, const char *message);

int KSITOOL_KSI_ERR_toExitCode(int error_code);

#ifdef	__cplusplus
}
#endif

#endif	/* API_WRAPPER_H */

