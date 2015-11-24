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
#include <ksi/compatibility.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ksitool_err.h"
#include <time.h>

#ifdef _WIN32
#	include <io.h>
#	define F_OK 0
#else
#	include <unistd.h>
#	define _access_s access
#endif
#include <errno.h>
#include "gt_cmd_control.h"
#include "gt_task_support.h"


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

	while ((C = hex[i++]) != '\0') {
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
	else if(strstr(url, "ksi+tcp://") == url)
		return FORMAT_OK;
	else
		return FORMAT_URL_UNKNOWN_SCHEME;
}

FormatStatus isIntegerFormatOK(const char *integer){
	int i = 0;
	int C;
	if(integer == NULL) return FORMAT_NULLPTR;
	if(strlen(integer) == 0) return FORMAT_NOCONTENT;

	while ((C = integer[i++]) != '\0') {
		if (isdigit(C) == 0) {
			return FORMAT_INVALID;
		}
	}
	return FORMAT_OK;
}

static int date_isValid(struct tm *time_st) {
    int days = 31;
    int dd = time_st->tm_mday;
    int mm = time_st->tm_mon + 1;
    int yy = time_st->tm_year + 1900;

	if (mm < 1 || mm > 12 || dd < 1 || yy < 1900) {
        return 0;
    }

    if (mm == 2) {
        days = 28;
		/* Its a leap year */
        if (yy % 400 == 0 || (yy % 4 == 0 && yy % 100 != 0)) {
            days = 29;
        }
    } else if (mm == 4 || mm == 6 || mm == 9 || mm == 11) {
        days = 30;
    }

    if (dd > days) {
        return 0;
    }
    return 1;
}

static int string_to_tm(const char *time, struct tm *time_st) {
	const char *ret = NULL;
	const char *next = NULL;
	/* If its not possible to convert date string with such a buffer, there is something wrong! */
	char buf[1024];

	if (time == NULL || time_st == NULL) return FORMAT_NULLPTR;

	memset(time_st, 0, sizeof(struct tm));


	next = time;
	ret = STRING_extractAbstract(next, NULL, "-", buf, sizeof(buf), NULL, find_charBeforeStrn, &next);
	if (ret != buf || next == NULL || *buf == '\0' || strlen(buf) > 4 || isIntegerFormatOK(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_year = atoi(buf) - 1900;

	ret = STRING_extractAbstract(next, "-", "-", buf, sizeof(buf), find_charAfterStrn, find_charBeforeLastStrn, &next);
	if (ret != buf || next == NULL || strlen(buf) > 2 || isIntegerFormatOK(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_mon = atoi(buf) - 1;

	ret = STRING_extractAbstract(next, "-", " ", buf, sizeof(buf), find_charAfterStrn, find_charBeforeLastStrn, &next);
	if (ret != buf || next == NULL || strlen(buf) > 2 || isIntegerFormatOK(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_mday = atoi(buf);

	if (date_isValid(time_st) == 0) return FORMAT_INVALID_UTC_OUT_OF_RANGE;

	ret = STRING_extractAbstract(next, " ", ":", buf, sizeof(buf), find_charAfterStrn, find_charBeforeStrn, &next);
	if (ret != buf || next == NULL || *buf == '\0' || strlen(buf) > 2 || isIntegerFormatOK(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_hour = atoi(buf);
	if (time_st->tm_hour < 0 || time_st->tm_hour > 23) return FORMAT_INVALID_UTC_OUT_OF_RANGE;

	ret = STRING_extractAbstract(next, ":", ":", buf, sizeof(buf), find_charAfterStrn, find_charBeforeLastStrn, &next);
	if (ret != buf || next == NULL || strlen(buf) > 2 || isIntegerFormatOK(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_min = atoi(buf);
	if (time_st->tm_min < 0 || time_st->tm_min > 59) return FORMAT_INVALID_UTC_OUT_OF_RANGE;

	ret = STRING_extractAbstract(next, ":", NULL, buf, sizeof(buf), find_charAfterStrn, find_charBeforeLastStrn, &next);
	if (ret != buf || strlen(buf) > 2 || isIntegerFormatOK(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_sec = atoi(buf);
	if (time_st->tm_sec < 0 || time_st->tm_sec > 59) return FORMAT_INVALID_UTC_OUT_OF_RANGE;

	return FORMAT_OK;
}

FormatStatus isUTCTimeFormatOk(const char *time) {
	char buf[32] = "";
	const char *next = NULL;
	struct tm time_st;

	/* As it must be converted to integer, check if its OK. If conversion failed return error.*/
	if (isIntegerFormatOK(time) == FORMAT_OK) return FORMAT_OK;
	else return string_to_tm(time, &time_st);
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

FormatStatus isConstraintFormatOK(const char *constraint) {
	char *at = NULL;
	unsigned i = 0;

	if(constraint == NULL) return FORMAT_NULLPTR;
	if(strlen(constraint) == 0) return FORMAT_NOCONTENT;

	if((at = strchr(constraint,'=')) == NULL) return FORMAT_INVALID;
	if(at == constraint || *(at+1)==0) return FORMAT_INVALID;

	while (constraint[i] != 0 &&  constraint[i] != '=') {
		if (!isdigit(constraint[i]) && constraint[i] != '.')
			return FORMAT_INVALID_OID;
		i++;
	}


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
	int fileStatus = EINVAL;
	if (strcmp(path, "-") == 0) {
		return PARAM_OK;
	}else if (isPathFormOk(path) == FORMAT_OK)
		fileStatus = doFileExists(path);
	else
		return FILE_INVALID_PATH;

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
	KSI_HashAlgorithm alg_id = -1;
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
	unsigned int i;
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
		case FORMAT_NULLPTR: return "(Parameter must have value)";
			break;
		case FORMAT_NOCONTENT: return "(Parameter has no content)";
			break;
		case FORMAT_INVALID: return "(Parameter is invalid)";
			break;
		case FORMAT_INVALID_OID: return "(OID is invalid)";
			break;
		case FORMAT_URL_UNKNOWN_SCHEME: return "(URL scheme is unknown)";
			break;
		case FORMAT_FLAG_HAS_ARGUMENT: return "(Parameter must not have arguments)";
			break;
		case FORMAT_INVALID_UTC: return "(Time not formatted as YYYY-MM-DD hh:mm:ss)";
			break;
		case FORMAT_INVALID_UTC_OUT_OF_RANGE: return "(Time out of range)";
			break;
		default: return "(Unknown error)";
			break;
	}
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
	default: return "(Unknown error)";
		break;
	}
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

bool convert_replaceWithOid(const char* arg, char* buf, unsigned len) {
	char *value = NULL;
	const char *oid = NULL;

	if(arg == NULL || buf == NULL) return false;
	strncpy(buf, arg, len-1);

	value = strchr(arg, '=');
	if (value == NULL) return false;
	else value++;

	if(strncmp(buf, "email=", 6) == 0) {
		oid = KSI_CERT_EMAIL;
	} else if (strncmp(buf, "country=", 8) == 0) {
		oid = KSI_CERT_COUNTRY;
	} else if (strncmp(buf, "org=", 4) == 0) {
		oid = KSI_CERT_ORGANIZATION;
	} else if (strncmp(buf, "common_name=", 12) == 0) {
		oid = KSI_CERT_COMMON_NAME;
	}

	if (oid != NULL && value != NULL)
		KSI_snprintf(buf, len, "%s=%s", oid, value);

	return true;
}

bool convert_UTC_to_UNIX(const char* arg, char* buf, unsigned len) {
	const char *ret = NULL;
	const char *next = NULL;
	struct tm time_st;
	time_t time;

	if(arg == NULL || buf == NULL) return false;
	KSI_strncpy(buf, arg, len);

	if (isIntegerFormatOK(arg) == FORMAT_OK) {
		return true;
	}

	if (string_to_tm(arg, &time_st) != FORMAT_OK) return false;

	time = KSI_CalendarTimeToUnixTime(&time_st);
	if (time == -1) return false;

	KSI_snprintf(buf, len, "%d", time);

	return true;
}
