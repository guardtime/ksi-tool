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
#include "tool_box/smart_file.h"
#include "api_wrapper.h"

static int ksitool_ErrToExitcode(int error_code) {
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
		case KT_INVALID_CONF:
			return EXIT_INVALID_CONF;
		case KT_UNKNOWN_ERROR:
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
	}
}

static int param_set_ErrToExitcode(int error_code) {
	switch (error_code) {
		case PST_OK:
			return EXIT_SUCCESS;
		case PST_OUT_OF_MEMORY:
			return EXIT_OUT_OF_MEMORY;
		case PST_IO_ERROR:
			return EXIT_IO_ERROR;
		case PST_INVALID_FORMAT:
			return EXIT_INVALID_FORMAT;
		case PST_TASK_MULTIPLE_CONSISTENT_TASKS:
		case PST_TASK_ZERO_CONSISTENT_TASKS:
			return EXIT_INVALID_CL_PARAMETERS;
		default:
			return EXIT_FAILURE;
	}
}

static int smart_file_ErrToExitcode(int error_code) {
	switch (error_code) {
		case SMART_FILE_OK:
			return EXIT_SUCCESS;
		case SMART_FILE_OUT_OF_MEM:
			return EXIT_OUT_OF_MEMORY;
		case SMART_FILE_INVALID_MODE:
		case SMART_FILE_UNABLE_TO_OPEN:
		case SMART_FILE_UNABLE_TO_READ:
		case SMART_FILE_UNABLE_TO_WRITE:
		case SMART_FILE_DOES_NOT_EXIST:
		case SMART_FILE_PIPE_ERROR:
			return EXIT_IO_ERROR;
		case SMART_FILE_ACCESS_DENIED:
			return EXIT_NO_PRIVILEGES;
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
		exit = KSITOOL_KSI_ERR_toExitCode(error);
	else if (error >= KSITOOL_ERR_BASE && error < PARAM_SET_ERROR_BASE)
		exit = ksitool_ErrToExitcode(error);
	else if (error >= PARAM_SET_ERROR_BASE && error < SMART_FILE_ERROR_BASE)
		exit = param_set_ErrToExitcode(error);
	else
		exit = smart_file_ErrToExitcode(error);

	return exit;
}

const char* errToString(int error) {
	const char* str;

	if(error < KSITOOL_ERR_BASE)
		str = KSI_getErrorString(error);
	else if (error >= KSITOOL_ERR_BASE && error < PARAM_SET_ERROR_BASE)
		str = ksitoolErrToString(error);
	else if (error >= PARAM_SET_ERROR_BASE && error < SMART_FILE_ERROR_BASE)
		str = PARAM_SET_errorToString(error);
	else
		str = SMART_FILE_errorToString(error);

	return str;
}
