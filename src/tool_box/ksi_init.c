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

#include <ksi/ksi.h>
#include <ksi/pkitruststore.h>
#include <string.h>
#include "ksitool_err.h"
#include "../param_set/param_set.h"
#include "../printer.h"
#include "../api_wrapper.h"
#include "ksi_init.h"

#include "../gt_task_support.h"

#ifdef _WIN32
//#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#include <stdlib.h>
#else
#	include <limits.h>
#	include <sys/time.h>
#endif

static int getStreamFromPath(const char *fname, const char *mode, FILE **stream, int *close) {
	int res;
	FILE *in = NULL;
	FILE *tmp = NULL;
	int doClose = 0;

	if (fname == NULL || mode == NULL || stream == NULL || close == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (strcmp(fname, "-") == 0) {
		tmp = strcmp(mode, "rb") == 0 ? stdin : stdout;
#ifdef _WIN32
		res = _setmode(_fileno(tmp),_O_BINARY);
		if (res == -1) {
			res = KT_UNABLE_TO_SET_STREAM_MODE;
			goto cleanup;
		}
#endif
	} else {
		in = fopen(fname, mode);
		if (in == NULL) {
			res = KT_IO_ERROR;
			goto cleanup;
		}

		doClose = 1;
		tmp = in;
	}

	*stream = tmp;
	in = NULL;
	*close = 1;

	res = KT_OK;

cleanup:

	if (in) fclose(in);
	return res;
}

static int tool_init_ksi_logger(KSI_CTX *ksi, ERR_TRCKR *err, PARAM_SET *set, FILE **log) {
	int res;
	FILE *writeLogTo = NULL;
	FILE *tmp = NULL;
	char *outLogfile = NULL;
	int logFile_must_be_closed;

	if (ksi == NULL || err == NULL || set == NULL || log == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	PARAM_SET_getStrValue(set, "log", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &outLogfile);

	/* Set logging. */
	if (outLogfile != NULL) {
		res = getStreamFromPath(outLogfile, "w", &writeLogTo, &logFile_must_be_closed);
		if (res != KT_OK) {
			ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
			goto cleanup;
		}

		if (logFile_must_be_closed) tmp = writeLogTo;

		res = KSI_CTX_setLoggerCallback(ksi, KSI_LOG_StreamLogger, writeLogTo);
		ERR_CATCH_MSG(err, res, "Error: Unable to set logger callback function.");

		res = KSI_CTX_setLogLevel(ksi, KSI_LOG_DEBUG);
		ERR_CATCH_MSG(err, res, "Error: Unable to set logger log level.");
	}

	res = KT_OK;
	*log = tmp;
	tmp = NULL;

cleanup:

	if (tmp) fclose(tmp);

	return res;
}

static int tool_init_ksi_network_provider(KSI_CTX *ksi, ERR_TRCKR *err, PARAM_SET *set) {
	int res;
	char *aggr_url = NULL;
	char *aggre_user = NULL;
	char *aggre_pass = NULL;
	char *ext_url = NULL;
	char *ext_user = NULL;
	char *ext_pass = NULL;
	char *pub_url = NULL;
	int networkConnectionTimeout = -1;
	int networkTransferTimeout = -1;


	if (ksi == NULL || err == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Extract values from the set.
     */
	PARAM_SET_getStrValue(set, "S", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &aggr_url);
	PARAM_SET_getStrValue(set, "X", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &ext_url);
	PARAM_SET_getStrValue(set, "P", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &pub_url);

	PARAM_SET_getStrValue(set, "aggre-user", NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, &aggre_user);
	PARAM_SET_getStrValue(set, "aggre-pass", NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, &aggre_pass);
	PARAM_SET_getStrValue(set, "ext-user", NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, &ext_user);
	PARAM_SET_getStrValue(set, "ext-pass", NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, &ext_pass);

	PARAM_SET_getIntValue(set, "C", NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, &networkConnectionTimeout);
	PARAM_SET_getIntValue(set, "c", NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, &networkTransferTimeout);

	aggre_user = aggre_user == NULL ? "anon" : aggre_user;
	aggre_pass = aggre_pass == NULL ? "anon" : aggre_pass;
	ext_user = ext_user == NULL ? "anon" : ext_user;
	ext_pass = ext_pass == NULL ? "anon" : ext_pass;


	/**
	 * Set service urls.
     */
	if (ext_url != NULL) {
		res = KSI_CTX_setExtender(ksi, ext_url, ext_user, ext_pass);
		ERR_CATCH_MSG(err, res, "Error: Unable set extender.");
	}

	if (aggr_url != NULL) {
		res = KSI_CTX_setAggregator(ksi, aggr_url, aggre_user, aggre_pass);
		ERR_CATCH_MSG(err, res, "Error: Unable set aggregator.");
	}

	if (pub_url != NULL) {
		res = KSI_CTX_setPublicationUrl(ksi, pub_url);
		ERR_CATCH_MSG(err, res, "Error: Unable set publication URL.");
	}

	/**
	 * Set service timeouts.
     */
	if (networkTransferTimeout > 0) {
		res = KSI_CTX_setConnectionTimeoutSeconds(ksi, networkConnectionTimeout);
		ERR_CATCH_MSG(err, res, "Error: Unable set connection timeout.");
	}

	if (networkTransferTimeout > 0) {
		res = KSI_CTX_setTransferTimeoutSeconds(ksi, networkTransferTimeout);
		ERR_CATCH_MSG(err, res, "Error: Unable set transfer timeout.");
	}

	res = KT_OK;

cleanup:

	return res;
}

static int tool_init_ksi_pub_cert_constraints(KSI_CTX *ksi, ERR_TRCKR *err, PARAM_SET *set) {
	int res;
	unsigned i = 0;
	char *constraint = NULL;
	unsigned constraint_count = 0;
	KSI_CertConstraint *constraintArray = NULL;

	if (ksi == NULL || err == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Extract all constraints from the highest priority level.
	 */
	res = PARAM_SET_getValueCount(set, "{cnstr}", NULL, PST_PRIORITY_HIGHEST, &constraint_count);
	if (res != PST_OK && res != PST_PARAMETER_EMPTY) {
		ERR_TRCKR_ADD(err, res, NULL);
		goto cleanup;
	}

	if (constraint_count == 0) {
		res = KT_OK;
		goto cleanup;
	}

	/**
	 * Generate array of publications file signatures certificate constraints and
	 * load it with values.
     */
	constraintArray = KSI_malloc(sizeof(KSI_CertConstraint) * (1 + constraint_count));
	if (constraintArray == NULL) {
		ERR_TRCKR_ADD(err, res = KT_OUT_OF_MEMORY, NULL);
		goto cleanup;
	}

	for (i = 0; i < constraint_count + 1; i++) {
		constraintArray[i].oid = NULL;
		constraintArray[i].val = NULL;
	}

	for (i = 0; i < constraint_count; i++) {
		char *oid = NULL;
		char *value = NULL;
		char tmp[1024];
			PARAM_SET_getStrValue(set, "cnstr", NULL, PST_PRIORITY_NONE, i, &constraint);
			strncpy(tmp, constraint, sizeof(tmp));

			oid = tmp;
			value = strchr(tmp, '=');
			if (value == NULL) {
				ERR_TRCKR_ADD(err, res = KT_INVALID_CMD_PARAM, "Error: Unable to parse constraint.");
				goto cleanup;
			}

			*value = '\0';
			value++;

		constraintArray[i].oid = KSI_malloc(strlen(oid) + 1);
		constraintArray[i].val = KSI_malloc(strlen(value) + 1);
		if (constraintArray[i].oid == NULL || constraintArray[i].val == NULL) {
			ERR_TRCKR_ADD(err, res = KSI_OUT_OF_MEMORY, NULL);
			goto cleanup;
		}

		strcpy(constraintArray[i].oid, oid);
		strcpy(constraintArray[i].val, value);
	}

	/**
	 * Configure KSI publications file constraints.
     */
	res = KSI_CTX_setDefaultPubFileCertConstraints(ksi, constraintArray);
	ERR_CATCH_MSG(err, res, "Error: Unable to add cert constraints.");

	res = KT_OK;

cleanup:

	if (constraintArray)  {
		for (i = 0; i < constraint_count; i++) {
			KSI_free(constraintArray[i].oid);
			KSI_free(constraintArray[i].val);
		}

		KSI_free(constraintArray);
	}

	return res;
}

static int tool_init_ksi_trust_store(KSI_CTX *ksi, ERR_TRCKR *err, PARAM_SET *set) {
	int res;
	KSI_PKITruststore *refTrustStore = NULL;
	int i = 0;
	int V, W;
	char *lookupFile = NULL;
	char *lookupDir = NULL;

	if (ksi == NULL || err == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Check if there are trust store related files or directories.
     */
	V = PARAM_SET_isSetByName(set,"V");
	W = PARAM_SET_isSetByName(set,"W");

	/**
	 * Configure KSI trust store.
	 * TODO: look over Windows and Linux compatibility related with trust store
	 * configuration.
	 * TODO: Fix trust store to ignore other certificates if there are certificates
	 * specified by the user (TOOL-20).
     */
	if (V || W) {
		res = KSI_CTX_getPKITruststore(ksi, &refTrustStore);
		ERR_CATCH_MSG(err, res, "Error: Unable to get PKI trust store.");

		i = 0;
		while(PARAM_SET_getStrValue(set, "V", NULL, PST_PRIORITY_HIGHEST, i++, &lookupFile) == PST_OK) {
			res = KSI_PKITruststore_addLookupFile(refTrustStore, lookupFile);
			ERR_CATCH_MSG(err, res, "Error: Unable to add cert to PKI trust store.");
		}

		i = 0;
		while(PARAM_SET_getStrValue(set, "W", NULL, PST_PRIORITY_HIGHEST, i++, &lookupDir) == PST_OK) {
			res = KSI_PKITruststore_addLookupDir(refTrustStore, lookupDir);
			ERR_CATCH_MSG(err, res, "Error: Unable to add lookup dir to PKI trust store.");
		}
	}

	res = KT_OK;

cleanup:

	return res;
}

static int tool_init_ksi_publications_file(KSI_CTX *ksi, ERR_TRCKR *err, PARAM_SET *set) {
	int res = KSI_UNKNOWN_ERROR;
	KSI_PublicationsFile *tmpPubFile = NULL;
	char *inPubFileName = NULL;

	if (ksi == NULL || err == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

//	TODO: Implement workaround to get publications file from file.
//	res = KSI_CTX_setPublicationsFile(ksi, tmpPubFile);
//	ERR_CATCH_MSG(err, res, "Error: Unable to configure publications file.");
//	tmpPubFile = NULL;

	res = KT_OK;

cleanup:

	KSI_PublicationsFile_free(tmpPubFile);

	return res;
}

int TOOL_init_ksi(PARAM_SET *set, KSI_CTX **ksi, ERR_TRCKR **error, FILE **ksi_log) {
	int res;
	ERR_TRCKR *err = NULL;
	KSI_CTX *tmp = NULL;
	FILE *tmp_log = NULL;

	if (set == NULL || ksi == NULL || error == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Initialize error tracker and configure output parameter immediately to be
	 * able to track errors if this function fails.
     */
	err = ERR_TRCKR_new(print_errors);
	if (err == NULL) {
		res = KT_OUT_OF_MEMORY;
		print_errors("Error: Unable to initialize error tracker.");
		goto cleanup;
	}

	*error = err;

	/**
	 * Initialize KSI_CTX and configure:
	 * 1) Logger.
	 * 2) Network provider (service info).
	 * 3) TODO:pubfile.
	 * 4) Publications file constraints.
	 * 5) Trust store.
     */
	res = KSI_CTX_new(&tmp);
	if (res != KSI_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to initialize KSI context.");
		goto cleanup;
	}

	res = tool_init_ksi_logger(tmp, err, set, &tmp_log);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure KSI logger.");
		goto cleanup;
	}

	res = tool_init_ksi_network_provider(tmp, err, set);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure network provider.");
		goto cleanup;
	}

	res = tool_init_ksi_publications_file(tmp, err, set);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure KSI publications file.");
		goto cleanup;
	}

	res = tool_init_ksi_pub_cert_constraints(tmp, err, set);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure KSI publications file constraints.");
		goto cleanup;
	}

	res = tool_init_ksi_trust_store(tmp, err, set);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure KSI trust store.");
		goto cleanup;
	}

	*ksi = tmp;
	*ksi_log = tmp_log;
	tmp = NULL;
	tmp_log = NULL;
	res = KT_OK;


cleanup:

	KSI_CTX_free(tmp);
	if (tmp_log != NULL) fclose(tmp_log);

	return res;
}

