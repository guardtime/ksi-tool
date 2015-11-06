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

#include "api_wrapper.h"
#include <ksi/ksi.h>

/**
 * Returns 1 if base error was appended.
 */
static int appendBaseErrorIfPresent(ERR_TRCKR *err, int res, KSI_CTX *ksi, long line) {
	char buf[2048];

	if (res != KSI_OK) {
		KSI_ERR_getBaseErrorMessage(ksi, buf, sizeof(buf), NULL, NULL);
		if (buf[0] != 0) {
			ERR_TRCKR_add(err, res, __FILE__, line, "Error: %s", buf);
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

int KSITOOL_extendSignature(ERR_TRCKR *err, KSI_CTX *ctx, KSI_Signature *sig, KSI_Signature **ext) {
	int res;
	res = KSI_extendSignature(ctx, sig, ext);
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
	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
	}
	return res;
}

int KSITOOL_Signature_verify(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx) {
	int res;
	res = KSI_Signature_verify(sig, ctx);
	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
		appendPubFileErros(err, res);
	}
	return res;
}

int KSITOOL_Signature_verifyOnline(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx) {
	int res;
	res = KSI_Signature_verifyOnline(sig, ctx);
	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
	}
	return res;
}

int KSITOOL_Signature_verifyDataHash(ERR_TRCKR *err, KSI_Signature *sig, KSI_CTX *ctx, const KSI_DataHash *docHash) {
	int res;
	res = KSI_Signature_verifyDataHash(sig, ctx, docHash);
	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendExtenderErrors(err, res);
	}
	return res;
}

int KSITOOL_createSignature(ERR_TRCKR *err, KSI_CTX *ctx, KSI_DataHash *dataHash, KSI_Signature **sig) {
	int res;
	res = KSI_createSignature(ctx, dataHash, sig);

	if (appendBaseErrorIfPresent(err, res, ctx, __LINE__) == 0) {
		appendNetworkErrors(err, res);
		appendAggreErrors(err, res);
	}
	return res;
}

int KSITOOL_verifyPublicationsFile(ERR_TRCKR *err, KSI_CTX *ctx, KSI_PublicationsFile *pubfile) {
	int res;
	res = KSI_verifyPublicationsFile(ctx, pubfile);
	appendPubFileErros(err, res);
	return res;
}
