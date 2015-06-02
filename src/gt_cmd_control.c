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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ksitool_err.h"

#ifdef _WIN32
#	include <io.h>
#	define F_OK 0
#else
#	include <unistd.h>
#	define _access_s access
#endif
#include <errno.h>
#include "gt_cmd_control.h"


#define CheckNullPtr(strn) if(strn==NULL) return PARAM_NULLPTR;
#define CheckEmpty(strn) if(strlen(strn) == 0) return PARAM_NOCONTENT;

FormatStatus isPathFormOk(const char *path){
	if(path == NULL) return FORMAT_NULLPTR;
	return FORMAT_OK;
}

FormatStatus isHexFormatOK(const char *hex){
	int i = 0;
	char C;

	if(hex == NULL) return FORMAT_NULLPTR;
	if(strlen(hex) == 0) return FORMAT_NOCONTENT;

	while (C = hex[i++]) {
		if (!isxdigit(C)) {
			return FORMAT_INVALID;
		}
	}
	return FORMAT_OK;
}

FormatStatus isURLFormatOK(const char *url){
	if(url == NULL) return FORMAT_NULLPTR;
	if(strlen(url) == 0) return FORMAT_NOCONTENT;

	if(strstr(url, "ksi://") == url)
		return FORMAT_OK;
	else if(strstr(url, "http://") == url || strstr(url, "ksi+http://") == url)
		return FORMAT_OK;
	else if(strstr(url, "https://") == url || strstr(url, "ksi+https://") == url)
		return FORMAT_OK;
	else if(strstr(url, "tcp://") == url || strstr(url, "ksi+tcp://") == url)
		return FORMAT_OK;
	else
		return FORMAT_URL_UNKNOWN_SCHEME;
}

FormatStatus isIntegerFormatOK(const char *integer){
	int i = 0;
	int C;
	if(integer == NULL) return FORMAT_NULLPTR;
	if(strlen(integer) == 0) return FORMAT_NOCONTENT;

	while (C = integer[i++]) {
		if (isdigit(C) == 0) {
			return FORMAT_INVALID;
		}
	}
	return FORMAT_OK;
}

FormatStatus isHashAlgFormatOK(const char *hashAlg){
	if(hashAlg == NULL) return FORMAT_NULLPTR;
	if(strlen(hashAlg) == 0) return FORMAT_NOCONTENT;
	return FORMAT_OK;
}

FormatStatus isImprintFormatOK(const char *imprint){
	char *colon;

	if(imprint == NULL) return FORMAT_NULLPTR;
	if(strlen(imprint) == 0) return FORMAT_NOCONTENT;

	colon = strchr(imprint, ':');

	if(colon == NULL || colon == imprint) return FORMAT_INVALID;
	if((colon +1) == NULL) return FORMAT_INVALID;

	if(isHexFormatOK(colon+1) != FORMAT_OK) return FORMAT_INVALID;

	return FORMAT_OK;
}

FormatStatus isFlagFormatOK(const char *hashAlg){
	if(hashAlg == NULL) return FORMAT_OK;
	else return FORMAT_FLAG_HAS_ARGUMENT;
}

FormatStatus isEmailFormatOK(const char *email){
	char *at = NULL;
	char *dot = NULL;
	if(email == NULL) return FORMAT_NULLPTR;
	if(strlen(email) == 0) return FORMAT_NOCONTENT;

	if((at = strchr(email,'@')) == NULL) return FORMAT_INVALID;
	if(at == email || *(at+1)==0) return FORMAT_INVALID;

	if((dot = strchr(email,'.')) == NULL) return FORMAT_INVALID;
	if(dot == email || *(dot+1)==0 || (dot-1) == at) return FORMAT_INVALID;

	return FORMAT_OK;
}

FormatStatus isUserPassFormatOK(const char *uss_pass){
	if(uss_pass == NULL) return FORMAT_NULLPTR;
	if(strlen(uss_pass) == 0) return FORMAT_NOCONTENT;
	return FORMAT_OK;
}

FormatStatus formatIsOK(const char *obj){
	return FORMAT_OK;
}


static int doFileExists(const char* path){
	if(_access_s(path, F_OK) == 0) return 0;
	else return errno;
}

ContentStatus isInputFileContOK(const char* path){
	ContentStatus res = PARAM_UNKNOWN_ERROR;
	int fileStatus = EINVAL;
	res = isPathFormOk(path);
	if (strcmp(path, "-") == 0) {
		return PARAM_OK;
	}else if (res == PARAM_OK)
		fileStatus = doFileExists(path);
	else
		return res;

	switch (fileStatus) {
	case 0:
		return PARAM_OK;
		break;
	case EACCES:
		return FILE_ACCESS_DENIED;
		break;
	case ENOENT:
		return FILE_DOES_NOT_EXIST;
		break;
	case EINVAL:
		return FILE_INVALID_PATH;
		break;
	}

	return PARAM_UNKNOWN_ERROR;
}

//TODO add some functionality
ContentStatus isOutputFileContOK(const char* path){
	return PARAM_OK;
}

ContentStatus isHashAlgContOK(const char *alg){
	if(KSI_getHashAlgorithmByName(alg) != -1) return PARAM_OK;
	else return HASH_ALG_INVALID_NAME;
}

ContentStatus isImprintContOK(const char *imprint){
	char *tmp = NULL;
	char *colon = NULL;
	char *imp = NULL;
	char *alg = NULL;
	int alg_id = -1;
	ContentStatus status = PARAM_INVALID;
	unsigned len = 0;
	unsigned expected_len = 0;
	if(imprint == NULL) return PARAM_INVALID;

	tmp = malloc(strlen(imprint)+1);
	if(tmp == NULL)
		exit(EXIT_OUT_OF_MEMORY);

	strcpy(tmp, imprint);
	colon = strchr(tmp, ':');

	*colon = 0;
	alg = tmp;
	imp = colon+1;

	alg_id = KSI_getHashAlgorithmByName(alg);
	if(alg_id == -1){
		status = HASH_ALG_INVALID_NAME;
		goto cleanup;
	}

	expected_len = KSI_getHashLength(alg_id);

	len = (unsigned)strlen(imp);

	if(len%2 != 0){
		status = HASH_IMPRINT_INVALID_LEN;
		goto cleanup;
	}

	if(len/2 != expected_len){
		status = HASH_IMPRINT_INVALID_LEN;
		goto cleanup;
	}

	status = PARAM_OK;
cleanup:

	free(tmp);
	return status;
}

ContentStatus isIntegerContOk(const char* integer) {
	int i;
	size_t len;
	size_t int_max_len;
	char tmp[32];

	sprintf(tmp, "%d", INT_MAX);
	len  = strlen(integer);
	int_max_len = strlen(tmp);

	if (len > int_max_len) {
		return INTEGER_TOO_LARGE;
	} else if (len == int_max_len) {

		for (i = 0; i < int_max_len; i++) {
			if (tmp[i] < integer[i])
				return INTEGER_TOO_LARGE;
			else if (tmp[i] > integer[i])
				break;
		}
	}
	return PARAM_OK;
}

ContentStatus contentIsOK(const char *alg){
	return PARAM_OK;
}






const char *getFormatErrorString(FormatStatus res){
	switch (res) {
	case FORMAT_OK: return "(Parameter OK)";
		break;
	case FORMAT_NULLPTR: return "(Parameter has no content)";
		break;
	case FORMAT_NOCONTENT: return "(Parameter has no content)";
		break;
	case FORMAT_INVALID: return "(Parameter is invalid)";
		break;
	case FORMAT_URL_UNKNOWN_SCHEME: return "(URL scheme is unknown)";
		break;
	case FORMAT_FLAG_HAS_ARGUMENT: return "(Parameter must not have arguments)";
		break;
	}

	return "(Unknown error)";
}

const char *getParameterContentErrorString(ContentStatus res){
	switch (res) {
	case PARAM_OK: return "(Parameter OK)";
		break;
	case PARAM_INVALID: return "(Parameter is invalid)";
		break;
	case HASH_ALG_INVALID_NAME: return "(Algorithm name is incorrect)";
		break;
	case HASH_IMPRINT_INVALID_LEN: return "(Hash length is incorrect)";
		break;
	case FILE_ACCESS_DENIED: return "(File access denied)";
		break;
	case FILE_DOES_NOT_EXIST: return "(File does not exist)";
		break;
	case FILE_INVALID_PATH: return "(Invalid path)";
		break;
	case INTEGER_TOO_LARGE: return "(Integer value is too large)";
		break;
	}

	return "(Unknown error)";
}

bool convert_repairUrl(const char* arg, char* buf, unsigned len){
	char *scheme = NULL;
	unsigned i = 0;
	if(arg == NULL || buf == NULL) return false;
	scheme = strstr(arg, "://");

	if(scheme == NULL){
		strncpy(buf, "http://", len-1);
		if(strlen(buf)+strlen(arg) < len)
			strcat(buf, arg);
		else
			return false;
	}
	else{
		while(arg[i] && i < len - 1){
			if(&arg[i] < scheme){
				buf[i] = (char)tolower(arg[i]);
			}
			else
				buf[i] = arg[i];
			i++;
		}
		buf[i] = 0;
	}
	return true;
}

bool convert_repairPath(const char* arg, char* buf, unsigned len){
	char *toBeReplaced = NULL;

	if(arg == NULL || buf == NULL) return false;
	strncpy(buf, arg, len-1);


	toBeReplaced = buf;
	while((toBeReplaced = strchr(toBeReplaced, '\\')) != NULL){
		*toBeReplaced = '/';
		toBeReplaced++;
	}

	return true;
}
