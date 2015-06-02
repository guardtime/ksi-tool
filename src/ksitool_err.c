#include <stdlib.h>
#include <ksi/ksi.h>
#include "ksitool_err.h"

static int ksiErrToExitcode(int error_code){
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
	else
		str = ksitoolErrToString(error);

	return str;
}
