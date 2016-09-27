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

#include "api_wrapper.h"
#include <stdlib.h>
#include <string.h>
#include <ksi/compatibility.h>
#include <ksi/policy.h>
#include "ksi/net.h"
#include "tool_box.h"
#include "smart_file.h"
#include "err_trckr.h"

#define ERR_APPEND_KSI_ERR_EXT_MSG(err, res, ref_err, msg) \
		if (res == ref_err) { \
			ERR_TRCKR_add(err, res, __FILE__, __LINE__, "Error: %s", msg); \
		}
static int appendInvalidPubfileUrlOrFileError(ERR_TRCKR *err, int res, KSI_CTX *ksi, long line) {
	char buf[2048];

	if (res == KSI_OK) return 0;

	KSI_ERR_getBaseErrorMessage(ksi, buf, sizeof(buf), NULL, NULL);

	if (res == KSI_INVALID_FORMAT) {
		if (strcmp(buf, "Unrecognized header.") == 0) {
			ERR_TRCKR_add(err, res, __FILE__, line, "Error: Unable to parse publications file. Check URL or file!");
			return 1;
		}
	}
	return 0;
}

static int appendInvalidServiceUrlError(ERR_TRCKR *err, int res, KSI_CTX *ksi, long line) {
	char errTrcae[8192];
	char serviceName[2048];
	char *ret = NULL;
	if (res == KSI_OK) return 0;

	/* If is HTTP error with code 4 */
	if (res == KSI_HTTP_ERROR) {
		ret = STRING_extractRmWhite(KSI_ERR_toString(ksi, errTrcae, sizeof(errTrcae)), "Unable to parse", "pdu", serviceName, sizeof(serviceName));
		if (ret == NULL) return 0;
		if (strcmp(serviceName, "aggregation") == 0 || strcmp(serviceName, "extend") == 0) {
			ERR_TRCKR_add(err, res, __FILE__, line, "Error: Service returned unknown PDU and HTTP error 400. Check the service URL!", serviceName);
			return 1;
		}
	} else if (res == KSI_PUBLICATIONS_FILE_NOT_CONFIGURED) {
			ERR_TRCKR_add(err, res, __FILE__, line, "Error: Publications file must be configured (-P <URL>)!", serviceName);
	} else {
		appendInvalidPubfileUrlOrFileError(err, res, ksi, line);
	}

	return 0;
}



/**
 * Returns 1 if base error was appended.
 */
static int appendBaseErrorIfPresent(ERR_TRCKR *err, int res, KSI_CTX *ksi, long line) {
	char buf[2048];
	int ext = 0;
	int tmp;
	if (res != KSI_OK) {
		KSI_ERR_getBaseErrorMessage(ksi, buf, sizeof(buf), &tmp, &ext);
		if (buf[0] != 0 && tmp != KSI_OK) {
			ERR_TRCKR_add(err, res, __FILE__, line, "Error: %s", buf);
			appendInvalidServiceUrlError(err, res, ksi, line);
			return 1;
		} else if (appendInvalidServiceUrlError(err, res, ksi, line) ) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

static void appendNetworkErrors(ERR_TRCKR *err, int res) {
	if (res == KSI_OK) return;
	ERR_APPEND_KSI_ERR(err, res, KSI_NETWORK_ERROR);
	ERR_APPEND_KSI_ERR(err, res, KSI_NETWORK_CONNECTION_TIMEOUT);
	ERR_APPEND_KSI_ERR(err, res, KSI_NETWORK_SEND_TIMEOUT);
	ERR_APPEND_KSI_ERR(err, res, KSI_NETWORK_RECIEVE_TIMEOUT);
	ERR_APPEND_KSI_ERR(err, res, KSI_HTTP_ERROR);
}

static void appendExtenderErrors(ERR_TRCKR *err, int res) {
	if (res == KSI_OK) return;
	ERR_APPEND_KSI_ERR_EXT_MSG(err, res, KSI_EXTENDER_NOT_CONFIGURED, "Extender URL is not configured.");
	ERR_APPEND_KSI_ERR(err, res, KSI_EXTEND_NO_SUITABLE_PUBLICATION);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_DATABASE_CORRUPT);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_DATABASE_MISSING);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_REQUEST_TIME_IN_FUTURE);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_REQUEST_TIME_TOO_NEW);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_REQUEST_TIME_TOO_OLD);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_INVALID_TIME_RANGE);
}

static void appendAggreErrors(ERR_TRCKR *err, int res) {
	if (res == KSI_OK) return;
	ERR_APPEND_KSI_ERR_EXT_MSG(err, res, KSI_AGGREGATOR_NOT_CONFIGURED, "Extender URL is not configured.");
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_AGGR_REQUEST_TOO_LARGE);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_AGGR_REQUEST_OVER_QUOTA);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_AGGR_TOO_MANY_REQUESTS);
	ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_AGGR_INPUT_TOO_LONG);
}

static void appendPubFileErros(ERR_TRCKR *err, int res) {
	if (res == KSI_OK) return;

	ERR_APPEND_KSI_ERR_EXT_MSG(err, res, KSI_PUBLICATIONS_FILE_NOT_CONFIGURED, "Publications file URL is not configured.");
	ERR_APPEND_KSI_ERR(err, res, KSI_PUBFILE_VERIFICATION_NOT_CONFIGURED);
	ERR_APPEND_KSI_ERR(err, res, KSI_PUBLICATIONS_FILE_NOT_SIGNED_WITH_PKI);
	ERR_APPEND_KSI_ERR(err, res, KSI_PKI_CERTIFICATE_NOT_TRUSTED);
}

static int verify_signature(KSI_Signature *sig, KSI_CTX *ctx,
							KSI_DataHash *hsh, KSI_uint64_t rootLevel,
							int extAllowed, KSI_PublicationsFile *pubFile, KSI_PublicationData *pubData,
							const KSI_Policy *policy,
							KSI_PolicyVerificationResult **result) {

	int res = KSI_UNKNOWN_ERROR;
	KSI_VerificationContext info;

	if (sig == NULL || ctx == NULL || policy == NULL || result == NULL) {
		res = KSI_INVALID_ARGUMENT;
		goto cleanup;
	}

	/* Create verification context */
	res = KSI_VerificationContext_init(&info, ctx);
	if (res != KSI_OK) goto cleanup;

	/* Init signature in verification context */
	info.signature = sig;

	/* Init document hash in verification context */
	info.documentHash = hsh;

	/* Init publications file in verification context*/
	info.userPublicationsFile = pubFile;

	/* Init user publication data in verification context */
	info.userPublication = pubData;

	/* Init aggregation level in verification context */
	if (rootLevel > 0xff) {
		res = KSI_INVALID_FORMAT;
		goto cleanup;
	}

	info.docAggrLevel = rootLevel;

	/* Init extention permission in verification context */
	info.extendingAllowed = !!extAllowed;

	/* Verify signature */
	res = KSI_SignatureVerifier_verify(policy, &info, result);
	if (res != KSI_OK) goto cleanup;

	if (*result && (*result)->finalResult.resultCode != KSI_VER_RES_OK) {
		res = KSI_VERIFICATION_FAILURE;
	}

cleanup:

	/* Clear data references in verification context as we do not own the memory */
	KSI_VerificationContext_clean(&info);

	return res;
}

int KSITOOL_extendSignature(ERR_TRCKR *err, KSI_CTX *ctx, KSI_Signature *sig, KSI_Signature **ext) {
	int res;

	res = KSI_extendSignature(ctx, sig, ext);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
		appendPubFileErros(err, res);
	}

	return res;
}

int KSITOOL_Signature_extendTo(ERR_TRCKR *err, const KSI_Signature *signature, KSI_CTX *ctx, KSI_Integer *to, KSI_Signature **extended) {
	int res;

	res = KSI_Signature_extendTo(signature, ctx, to, extended);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
	}
	return res;
}

int KSITOOL_Signature_extend(ERR_TRCKR *err, const KSI_Signature *signature, KSI_CTX *ctx, const KSI_PublicationRecord *pubRec, KSI_Signature **extended) {
	int res;

	res = KSI_Signature_extend(signature, ctx, pubRec, extended);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
		appendPubFileErros(err, res);
	}
	return res;
}

int KSITOOL_RequestHandle_getExtendResponse(ERR_TRCKR *err, KSI_CTX *ctx, KSI_RequestHandle *handle, KSI_ExtendResp **resp) {
	int res;

	res = KSI_RequestHandle_getExtendResponse(handle, resp);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
		appendPubFileErros(err, res);
	}
	return res;
}

int KSITOOL_SignatureVerify_general(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh,
									KSI_PublicationData *pubdata, int extperm,
									KSI_PolicyVerificationResult **result) {
	int res;

	/* First check if user has provided publications */
	if (pubdata != NULL) {
		res = KSITOOL_SignatureVerify_userProvidedPublicationBased(err, sig, ctx, hsh, pubdata, extperm, result);
	} else {
		/* Get available trust anchor from the signature */
		if (KSITOOL_Signature_isCalendarAuthRecPresent(sig)) {
			res = KSITOOL_SignatureVerify_keyBased(err, sig, ctx, hsh, result);
		} else if (KSITOOL_Signature_isPublicationRecordPresent(sig)) {
			res = KSITOOL_SignatureVerify_publicationsFileBased(err, sig, ctx, hsh, extperm, result);
		} else {
			res = KSITOOL_SignatureVerify_calendarBased(err, sig, ctx, hsh, result);
		}
	}

	return res;
}

int KSITOOL_SignatureVerify_internally(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh,
									   KSI_PolicyVerificationResult **result) {
	int res;

	res = verify_signature(sig, ctx, hsh, 0, 0, NULL, NULL, KSI_VERIFICATION_POLICY_INTERNAL, result);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);
	appendBaseErrorIfPresent(err, res, ctx, __LINE__);

	return res;
}

int KSITOOL_SignatureVerify_calendarBased(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh,
										  KSI_PolicyVerificationResult **result) {
	int res;

	res = verify_signature(sig, ctx, hsh, 0, 1, NULL, NULL, KSI_VERIFICATION_POLICY_CALENDAR_BASED, result);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
	}
	return res;
}

int KSITOOL_SignatureVerify_keyBased(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh,
									 KSI_PolicyVerificationResult **result){
	int res;

	res = verify_signature(sig, ctx, hsh, 0, 0, NULL, NULL, KSI_VERIFICATION_POLICY_KEY_BASED, result);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendPubFileErros(err, res);
	}
	return res;
}

int KSITOOL_SignatureVerify_publicationsFileBased(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh,
												  int extperm,
												  KSI_PolicyVerificationResult **result){
	int res;

	res = verify_signature(sig, ctx, hsh, 0, extperm, NULL, NULL, KSI_VERIFICATION_POLICY_PUBLICATIONS_FILE_BASED, result);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendPubFileErros(err, res);
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
	}
	return res;
}

int KSITOOL_SignatureVerify_userProvidedPublicationBased(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *hsh,
														 KSI_PublicationData *pubdata, int extperm,
														 KSI_PolicyVerificationResult **result){
	int res;

	if (pubdata == NULL) return KSI_INVALID_FORMAT;

	res = verify_signature(sig, ctx, hsh, 0, extperm, NULL, pubdata, KSI_VERIFICATION_POLICY_USER_PUBLICATION_BASED, result);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
	}
	return res;
}

int KSITOOL_BlockSigner_close(ERR_TRCKR *err, KSI_CTX *ctx, KSI_BlockSigner *signer, KSI_MultiSignature **ms) {
	int res;
	res = KSI_BlockSigner_close(signer, ms);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendAggreErrors(err, res);
	}
	return res;
}

int KSITOOL_receivePublicationsFile(ERR_TRCKR *err, KSI_CTX *ctx, KSI_PublicationsFile **pubFile) {
	int res;
	res = KSI_receivePublicationsFile(ctx, pubFile);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendPubFileErros(err, res);
		appendNetworkErrors(err, res);
	}
	return res;
}

int KSITOOL_verifyPublicationsFile(ERR_TRCKR *err, KSI_CTX *ctx, KSI_PublicationsFile *pubfile) {
	int res;

	res = KSI_verifyPublicationsFile(ctx, pubfile);
	if (res != KSI_OK) KSITOOL_KSI_ERRTrace_save(ctx);
	appendBaseErrorIfPresent(err, res, ctx, __LINE__);
	appendPubFileErros(err, res);

	return res;
}

int KSITOOL_Signature_isPublicationRecordPresent(const KSI_Signature *sig) {
	KSI_PublicationRecord *pubRec = NULL;

	if (sig == NULL) return 0;
	KSI_Signature_getPublicationRecord(sig, &pubRec);

	return pubRec == NULL ? 0 : 1;
}

int KSITOOL_Signature_isCalendarAuthRecPresent(const KSI_Signature *sig) {
	KSI_CalendarAuthRec *calRec = NULL;

	if (sig == NULL) return 0;
	KSI_Signature_getCalendarAuthRec(sig, &calRec);

	return calRec == NULL ? 0 : 1;
}

char err_buf[0x2000] = "";

void KSITOOL_KSI_ERRTrace_save(KSI_CTX *ctx) {
	static int lock = 0;
	int error = KSI_UNKNOWN_ERROR;
	char dummybuf[1];

	if (lock) return;

	KSI_ERR_getBaseErrorMessage(ctx, dummybuf, sizeof(dummybuf), &error, NULL);
	if (error != KSI_OK) {
		KSI_ERR_toString(ctx, err_buf, sizeof(err_buf));
		lock = 1;
	}
}

const char *KSITOOL_KSI_ERRTrace_get(void) {
	return err_buf;
}

void KSITOOL_KSI_ERRTrace_LOG(KSI_CTX *ksi) {
	const char *err_trace = NULL;
	if (ksi == NULL) return;

	err_trace = KSITOOL_KSI_ERRTrace_get();
	if (err_trace != NULL && err_trace[0] != '\0') {
		KSI_LOG_debug(ksi, "\n%s", KSITOOL_KSI_ERRTrace_get());
	}
}

char *KSITOOL_DataHash_toString(KSI_DataHash *hsh, char *buf, size_t buf_len) {
	char tmp[1024];
	char alg_hex_str[3] = {0, 0, 0};
	KSI_HashAlgorithm alg = KSI_HASHALG_INVALID;

	if (hsh == NULL || buf == NULL || buf_len == 0) return NULL;
	if (KSI_DataHash_toString(hsh, tmp, sizeof(tmp)) ==  NULL) return NULL;
	alg_hex_str[0] = tmp[0];
	alg_hex_str[1] = tmp[1];
	alg = strtol(alg_hex_str, NULL, 16);

	KSI_snprintf(buf, buf_len, "%s:%s", KSI_getHashAlgorithmName(alg), tmp + 2);

	return buf;
}

char *KSITOOL_PublicationData_toString(KSI_PublicationData *data, char *buf, size_t buf_len) {
	char *ret = NULL;
	int res;
	char tmp[1024];
	char hash_str[1024];
	char *p_hsh = NULL;
	KSI_DataHash *data_hash = NULL;

	if (data == NULL || buf == NULL || buf_len == 0) goto cleanup;

	if (KSI_PublicationData_toString(data, tmp, sizeof(tmp)) == NULL) goto cleanup;
	p_hsh = strstr(tmp, "Published hash:");

	if (p_hsh == NULL) {
		KSI_strncpy(buf, tmp, buf_len);
	} else {
		*p_hsh = '\0';

		res = KSI_PublicationData_getImprint(data, &data_hash);
		if (res != KT_OK) goto cleanup;

		if (KSITOOL_DataHash_toString(data_hash, hash_str, sizeof(hash_str)) == NULL) goto cleanup;
		KSI_snprintf(buf, buf_len, "%s%s%s", tmp, "Published hash: ", hash_str);
	}

	ret = buf;

cleanup:

	return ret;
}

char *KSITOOL_PublicationRecord_toString(KSI_PublicationRecord *rec, char *buf, size_t buf_len) {
	int res;
	KSI_PublicationData *pub_data = NULL;
	KSI_Utf8StringList *pub_ref_list = NULL;
	char *ret = NULL;
	char tmp[1024];
	size_t count = 0;
	size_t i;


	if (rec == NULL || buf == NULL || buf_len == 0) goto cleanup;


	res = KSI_PublicationRecord_getPublishedData(rec, &pub_data);
	if (res != KSI_OK) goto cleanup;

	res = KSI_PublicationRecord_getPublicationRefList(rec, &pub_ref_list);
	if (res != KSI_OK) goto cleanup;

	count += KSI_snprintf(buf + count, buf_len - count, "%s", KSITOOL_PublicationData_toString(pub_data, tmp, sizeof(tmp)));

	for (i = 0; i < KSI_Utf8StringList_length(pub_ref_list); i++) {
		KSI_Utf8String *ref = NULL;

		res = KSI_Utf8StringList_elementAt(pub_ref_list, i, &ref);
		if (res != KSI_OK) goto cleanup;

		count += KSI_snprintf(buf + count, buf_len - count, "\nRef: %s", KSI_Utf8String_cstr(ref));
	}


	ret = buf;

cleanup:

	return ret;
}

static const char *level2str(int level) {
	switch (level) {
		case KSI_LOG_DEBUG: return "DEBUG";
		case KSI_LOG_INFO: return "INFO";
		case KSI_LOG_NOTICE: return "NOTICE";
		case KSI_LOG_WARN: return "WARN";
		case KSI_LOG_ERROR: return "ERROR";
		default: return "UNKNOWN LOG LEVEL";
	}
}

int KSITOOL_LOG_SmartFile(void *logCtx, int logLevel, const char *message) {
	char time_buf[32];
	char buf[0xffff];
	struct tm *tm_info;
	time_t timer;
	SMART_FILE *f = (SMART_FILE *) logCtx;
	size_t count = 0;

	timer = time(NULL);

	tm_info = localtime(&timer);
	if (tm_info == NULL) {
		return KSI_UNKNOWN_ERROR;
	}

	if (f != NULL) {
		strftime(time_buf, sizeof(time_buf), "%d.%m.%Y %H:%M:%S", tm_info);
		count = KSI_snprintf(buf, sizeof(buf), "%s [%s] - %s\n", level2str(logLevel), time_buf, message);
		SMART_FILE_write(f, buf, count, NULL);
	}

	return KSI_OK;
}

static int KSI_Signature_serialize_wrapper(KSI_CTX *ksi, KSI_Signature *sig, unsigned char **raw, size_t *raw_len) {
	if (ksi);
	return KSI_Signature_serialize(sig, raw, raw_len);
}

static int load_ksi_obj(ERR_TRCKR *err, KSI_CTX *ksi, const char *path, const char* mode, void **obj,	int (*parse)(KSI_CTX *ksi, unsigned char *raw, unsigned raw_len, void **obj), void (*obj_free)(void*), const char *name) {
	int res;
	SMART_FILE *file = NULL;
	unsigned char buf[0xffff + 4];
	unsigned char dummy[1];
	void *tmp = NULL;
	size_t data_len = 0;
	size_t dummy_len = 0;


	if (err == NULL || ksi == NULL || path == NULL || obj == NULL || parse == NULL || obj_free == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = SMART_FILE_open(path, mode, &file);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: %s", KSITOOL_errToString(res));
		goto cleanup;
	}

	res = SMART_FILE_read(file, (char *)buf, sizeof(buf), &data_len);
	if(res != SMART_FILE_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to read %s from file.", name);
		goto cleanup;
	}

	res = SMART_FILE_read(file, (char *)dummy, sizeof(dummy), &dummy_len);
	if(res != SMART_FILE_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to read data from file.");
		goto cleanup;
	}

	if (dummy_len != 0 || !SMART_FILE_isEof(file)) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_INPUT_FORMAT, "Error: Input file too long for a valid %s file.", name);
		goto cleanup;
	}

	if (data_len == 0) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_INPUT_FORMAT, "Error: Input file is empty.", name);
		goto cleanup;
	}

	res = parse(ksi, buf, (unsigned)data_len, &tmp);
	if (res != KSI_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to parse %s.", name);
		goto cleanup;
	}

	*obj = tmp;
	tmp = NULL;
	res = KT_OK;


cleanup:

	SMART_FILE_close(file);
	obj_free(tmp);

	return res;
}

static int saveKsiObj(ERR_TRCKR *err, KSI_CTX *ksi, const char *mode, void *obj, int (*serialize)(KSI_CTX *ksi, void *obj, unsigned char **raw, size_t *raw_len), const char *path, char *f, size_t f_len) {
	int res;
	SMART_FILE *file = NULL;
	unsigned char *raw = NULL;
	size_t raw_len = 0;
	size_t count = 0;
	char mode_sum[32];

	if (err == NULL || ksi == NULL || obj == NULL || serialize == NULL || path == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	KSI_snprintf(mode_sum, sizeof(mode_sum), "wb%s", (mode == NULL) ? "" : mode);

	res = serialize(ksi, obj, &raw, &raw_len);
	if (res != KSI_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to serialize.");
		goto cleanup;
	}

	res = SMART_FILE_open(path, mode_sum, &file);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: %s", KSITOOL_errToString(res));
		goto cleanup;
	}

	if (f != NULL && f_len != 0) {
		KSI_strncpy(f, SMART_FILE_getFname(file), f_len);
	}

	res = SMART_FILE_write(file, (char *)raw, raw_len, &count);
	if(res != SMART_FILE_OK || count != raw_len) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to write to file.");
		goto cleanup;
	}

	res = KT_OK;

cleanup:

	KSI_free(raw);
	SMART_FILE_close(file);

	return res;
}

int KSI_OBJ_saveSignature(ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sign, const char *mode, const char *fname, char *f, size_t f_len) {
	int res;

	if (ksi == NULL || fname == NULL || sign == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = saveKsiObj(err, ksi, mode, sign,
				(int (*)(KSI_CTX *, void *, unsigned char **, size_t *))KSI_Signature_serialize_wrapper,
				fname, f, f_len);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to save signature file to '%s'.", fname);
	}

	return res;
}

int KSI_OBJ_savePublicationsFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_PublicationsFile *pubfile, const char *mode, const char *fname) {
	int res;

	if (ksi == NULL || fname == NULL || pubfile == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = saveKsiObj(err, ksi, mode, pubfile,
				(int (*)(KSI_CTX *, void *, unsigned char **, size_t *))KSI_PublicationsFile_serialize,
				fname, NULL, 0);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to save publications file to '%s'.", fname);
	}

	return res;
}

int KSI_OBJ_loadSignature(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, const char* mode, KSI_Signature **sig) {
	int res;

	if (ksi == NULL || fname == NULL || sig == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = load_ksi_obj(err, ksi, fname, mode,
				(void**)sig,
				(int (*)(KSI_CTX *, unsigned char*, unsigned, void**))KSI_Signature_parse,
				(void (*)(void *))KSI_Signature_free,
				"KSI Signature");

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to load signature file from '%s'.", fname);
	}
	return res;
}

int KSI_OBJ_isSignatureExtended(const KSI_Signature *sig) {
	KSI_PublicationRecord *pubRec = NULL;

	if (sig == NULL) return 0;
	KSI_Signature_getPublicationRecord(sig, &pubRec);

	return pubRec == NULL ? 0 : 1;
}

int KSITOOL_KSI_ERR_toExitCode(int error_code) {

	switch (error_code) {
		case KSI_OK:
			return EXIT_SUCCESS;

		/**
		 * General errors.
		 */
		case KSI_INVALID_ARGUMENT:
		case KSI_BUFFER_OVERFLOW:
		case KSI_TLV_PAYLOAD_TYPE_MISMATCH:
		case KSI_ASYNC_NOT_FINISHED:
		case KSI_INVALID_PUBLICATION:
		case KSI_UNKNOWN_ERROR:
		case KSI_MULTISIG_NOT_FOUND:
		case KSI_MULTISIG_INVALID_STATE:
		case KSI_SERVICE_INVALID_REQUEST:
		case KSI_SERVICE_INVALID_PAYLOAD:
		case KSI_SERVICE_INTERNAL_ERROR:
		case KSI_SERVICE_UPSTREAM_ERROR:
		case KSI_SERVICE_UPSTREAM_TIMEOUT:
		case KSI_SERVICE_UNKNOWN_ERROR:
			return EXIT_FAILURE;

		/**
		 * Extender errors.
		 */
		case KSI_EXTEND_WRONG_CAL_CHAIN:
		case KSI_EXTEND_NO_SUITABLE_PUBLICATION:
		case KSI_SERVICE_EXTENDER_INVALID_TIME_RANGE:
		case KSI_SERVICE_EXTENDER_DATABASE_MISSING:
		case KSI_SERVICE_EXTENDER_DATABASE_CORRUPT:
		case KSI_SERVICE_EXTENDER_REQUEST_TIME_TOO_OLD:
		case KSI_SERVICE_EXTENDER_REQUEST_TIME_TOO_NEW:
		case KSI_SERVICE_EXTENDER_REQUEST_TIME_IN_FUTURE:
			return EXIT_EXTEND_ERROR;

		/**
		 * Aggregator erros.
		 */
		case KSI_SERVICE_AGGR_REQUEST_TOO_LARGE:
		case KSI_SERVICE_AGGR_REQUEST_OVER_QUOTA:
		case KSI_SERVICE_AGGR_TOO_MANY_REQUESTS:
		case KSI_SERVICE_AGGR_INPUT_TOO_LONG:
			return EXIT_AGGRE_ERROR;

		/**
		 * Network errors.
		 */
		case KSI_NETWORK_ERROR:
		case KSI_NETWORK_CONNECTION_TIMEOUT:
		case KSI_NETWORK_SEND_TIMEOUT:
		case KSI_NETWORK_RECIEVE_TIMEOUT:
		case KSI_HTTP_ERROR:
			return EXIT_NETWORK_ERROR;

		/**
		 * Configuration errors.
		 */
		case KSI_AGGREGATOR_NOT_CONFIGURED:
		case KSI_EXTENDER_NOT_CONFIGURED:
		case KSI_PUBLICATIONS_FILE_NOT_CONFIGURED:
		case KSI_PUBFILE_VERIFICATION_NOT_CONFIGURED:
			return EXIT_INVALID_CONF;

		/**
		 * General cryptographic errors.
		 */
		case KSI_UNTRUSTED_HASH_ALGORITHM:
		case KSI_UNAVAILABLE_HASH_ALGORITHM:
		case KSI_PUBLICATIONS_FILE_NOT_SIGNED_WITH_PKI:
		case KSI_CRYPTO_FAILURE:
		case KSI_INVALID_PKI_SIGNATURE:
		case KSI_PKI_CERTIFICATE_NOT_TRUSTED:
			return EXIT_CRYPTO_ERROR;

		/**
		 * Verification errors.
		 */
		case KSI_VERIFICATION_FAILURE:
			return EXIT_VERIFY_ERROR;

		/**
		 * Format errors.
		 */
		case KSI_INVALID_FORMAT:
		case KSI_INVALID_SIGNATURE:
			return EXIT_INVALID_FORMAT;

		/**
		 * HMAC error.
		 */
		case KSI_HMAC_MISMATCH:
			return EXIT_HMAC_ERROR;

		case KSI_SERVICE_AUTHENTICATION_FAILURE:
			return EXIT_AUTH_FAILURE;

		case KSI_OUT_OF_MEMORY:
			return EXIT_OUT_OF_MEMORY;

		case KSI_IO_ERROR:
			return EXIT_IO_ERROR;

		default:
			return EXIT_FAILURE;
	}
}
