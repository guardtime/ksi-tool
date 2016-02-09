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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ksi/ksi.h>
#include "ksitool_err.h"
#include "param_set/param_set.h"

static int ksiErrToExitcode(int error_code){
	switch (error_code) {
		case KSI_OK:
			return EXIT_SUCCESS;
		case KSI_INVALID_ARGUMENT:
			return EXIT_INVALID_FORMAT;
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

static int ksitoolErrToExitcode(int error_code) {
	switch (error_code) {
		case KSI_OK:
			return EXIT_SUCCESS;
		case KT_OUT_OF_MEMORY:
			return EXIT_OUT_OF_MEMORY;
		case KT_INVALID_ARGUMENT:
			return EXIT_FAILURE;
		case KT_UNABLE_TO_SET_STREAM_MODE:
			return EXIT_IO_ERROR;
		case KT_IO_ERROR:
			return EXIT_IO_ERROR;
		case KT_INDEX_OVF:
			return EXIT_FAILURE;
		case KT_INVALID_INPUT_FORMAT:
			return EXIT_INVALID_FORMAT;
		case KT_UNKNOWN_HASH_ALG:
			return EXIT_CRYPTO_ERROR;
		case KT_INVALID_CMD_PARAM:
			return EXIT_INVALID_CL_PARAMETERS;
		case KT_NO_PRIVILEGES:
			return EXIT_NO_PRIVILEGES;
		case KT_UNKNOWN_ERROR:
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
	}
}

static const char* ksitoolErrToString(int error_code) {
	switch (error_code) {
		case KSI_OK:
			return "OK.";
		case KT_OUT_OF_MEMORY:
			return "Ksitool out of memory.";
		case KT_INVALID_ARGUMENT:
			return "Invalid argument.";
		case KT_UNABLE_TO_SET_STREAM_MODE:
			return "Unable to set stream mode.";
		case KT_IO_ERROR:
			return "IO error.";
		case KT_INDEX_OVF:
			return "Index is too large.";
		case KT_INVALID_INPUT_FORMAT:
			return "Invalid input data format";
		case KT_HASH_LENGTH_IS_NOT_EVEN:
			return "The hash length is not even number.";
		case KT_INVALID_HEX_CHAR:
			return "The hex data contains invalid characters.";
		case KT_UNKNOWN_HASH_ALG:
			return "The hash algorithm is unknown or unimplemented.";
		case KT_INVALID_CMD_PARAM:
			return "The command-line parameters is invalid or missing.";
		case KT_NO_PRIVILEGES:
			return "User has no privileges.";
		case KT_KSI_SIG_VER_IMPOSSIBLE:
			return "Verification can't be performed.";
		case KT_UNKNOWN_ERROR:
			return "Unknown error.";
		default:
			return "Unknown error.";
	}
}


int errToExitCode(int error) {
	int exit;

	if(error < KSITOOL_ERR_BASE)
		exit = ksiErrToExitcode(error);
	else
		exit = ksitoolErrToExitcode(error);

	return exit;
}

const char* errToString(int error) {
	const char* str;

	if(error < KSITOOL_ERR_BASE)
		str = KSI_getErrorString(error);
	else if (error >= KSITOOL_ERR_BASE && error < PARAM_SET_ERROR_BASE)
		str = ksitoolErrToString(error);
	else
		str = PARAM_SET_errorToString(error);

	return str;
}



typedef struct ERR_ENTRY_st {
	int code;
	int line;
	char message[MAX_MESSAGE_LEN];
	char fileName[MAX_FILE_NAME_LEN];
} ERR_ENTRY;

struct ERR_TRCKR_st {
	ERR_ENTRY err[MAX_ERROR_COUNT];
	unsigned count;
	int (*printErrors)(const char*, ...);
};




ERR_TRCKR *ERR_TRCKR_new(int (*printErrors)(const char*, ...)) {
	ERR_TRCKR *tmp = NULL;

	tmp = (ERR_TRCKR*)malloc(sizeof(ERR_TRCKR));
	if (tmp == NULL) return NULL;

	tmp->count = 0;


	tmp->printErrors = printErrors != NULL ? printErrors : printf;

	return tmp;
}

void ERR_TRCKR_free(ERR_TRCKR *obj) {
	free(obj);
	return;
}

void ERR_TRCKR_add(ERR_TRCKR *err, int code, const char *fname, int lineN, const char *msg, ...) {
	va_list va;
	if (err == NULL || fname == NULL) return;
	if(err->count >= MAX_ERROR_COUNT ) return;

	va_start(va, msg);
	vsnprintf(err->err[err->count].message, MAX_MESSAGE_LEN, msg == NULL ? "" : msg, va);
	va_end(va);
	err->err[err->count].message[MAX_MESSAGE_LEN -1] = 0;


	strncpy(err->err[err->count].fileName, fname, MAX_FILE_NAME_LEN);
	err->err[err->count].fileName[MAX_FILE_NAME_LEN -1] = 0;
	err->err[err->count].line = lineN;
	err->err[err->count].code = code;
	err->count++;

	return;
}

void ERR_TRCKR_reset(ERR_TRCKR *err) {
	if (err == NULL) return;
	err->count = 0;
}


void ERR_TRCKR_printErrors(ERR_TRCKR *err) {
	int i;

	if (err == NULL) return;

	for (i = err->count - 1; i >= 0; i--) {
		if (err->err[i].message[0] == '\0') {
			err->printErrors("%i) %s%s",i+1,  errToString(err->err[i].code), (err->err[i].message[strlen(err->err[i].message) - 1] == '\n') ? ("") : ("\n"));
		} else {
			err->printErrors("%i) %s%s",i+1,  err->err[i].message, (err->err[i].message[strlen(err->err[i].message) - 1] == '\n') ? ("") : ("\n"));
		}
	}

	return;
	}

void ERR_TRCKR_printExtendedErrors(ERR_TRCKR *err) {
	int i;

	if (err == NULL) return;

	for (i = err->count - 1; i >= 0; i--) {
		err->printErrors("%i) %s (%i) %s [%s 0x%02x]%s",i+1,
				err->err[i].fileName,
				err->err[i].line,
				err->err[i].message,
				errToString(err->err[i].code),
				err->err[i].code,
				(err->err[i].message[strlen(err->err[i].message) - 1] == '\n') ? ("") : ("\n") );
	}

	return;
}