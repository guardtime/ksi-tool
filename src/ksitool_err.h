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

#ifndef KSITOOL_ERR_H
#define	KSITOOL_ERR_H



#ifdef	__cplusplus
extern "C" {
#endif

//    KSITOOL_ERR_BASE  0x10001
//PARAM_SET_ERROR_BASE  0x30001
//SMART_FILE_ERROR_BASE 0x40001

#define KSITOOL_ERR_BASE 0x10001

enum Ksitool_exit {
	EXIT_INVALID_CL_PARAMETERS = 3,
	EXIT_INVALID_FORMAT = 4,
	EXIT_NETWORK_ERROR = 5,
	EXIT_VERIFY_ERROR = 6,
	EXIT_EXTEND_ERROR = 7,
	EXIT_AGGRE_ERROR = 8,
	EXIT_IO_ERROR = 9,
	EXIT_CRYPTO_ERROR = 10,
	EXIT_HMAC_ERROR = 11,
	EXIT_NO_PRIVILEGES = 12,
	EXIT_OUT_OF_MEMORY = 13,
	EXIT_AUTH_FAILURE = 14,
	EXIT_VALUE_RESERVED = 15,
	EXIT_INVALID_CONF = 16,
};

enum Ksitool_errors {
	KT_OK = 0,
	KT_OUT_OF_MEMORY = KSITOOL_ERR_BASE,
	KT_INVALID_ARGUMENT,
	KT_UNABLE_TO_SET_STREAM_MODE,
	KT_IO_ERROR,
	KT_INDEX_OVF,
	KT_INVALID_INPUT_FORMAT,
	KT_HASH_LENGTH_IS_NOT_EVEN,
	KT_INVALID_HEX_CHAR,
	KT_UNKNOWN_HASH_ALG,
	KT_INVALID_CMD_PARAM,
	KT_NO_PRIVILEGES,
	KT_FUNCTION_NOT_FOUND,
	KT_COMPONENT_HAS_NO_IMPLEMENTATION,
	KT_UNABLE_TO_LOAD_LIB,
	KT_INVALID_IO_WRITE,
	KT_INVALID_IO_READ,
	KT_FILE_NOT_OPEND,
	KT_KSI_SIG_VER_IMPOSSIBLE,
	KT_INVALID_CONF,
	KT_PUBFILE_HAS_NO_PUBREC_TO_EXTEND_TO,
	KT_AGGR_LVL_LIMIT_TOO_SMALL,
	KT_UNKNOWN_ERROR,
};

int KSITOOL_errToExitCode(int error);
const char* KSITOOL_errToString(int error);



#ifdef	__cplusplus
}
#endif

#endif	/* KSITOOL_ERR_H */

