#include <ctype.h>
#include <stdio.h>		//input output
#include <string.h>


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

PARAM_RES isPathFormOk(const char *path){
	CheckNullPtr(path);
	return PARAM_OK;
}

PARAM_RES isHexFormatOK(const char *hex){
	int i = 0;
	char C;
	bool failure = false;

	CheckNullPtr(hex);
	CheckEmpty(hex);

	while (C = hex[i++]) {
		if (!isxdigit(C)) {
			return PARAM_INVALID;
		}
	}
	return PARAM_OK;
}

PARAM_RES isURLFormatOK(const char *url){
	CheckNullPtr(url);
	CheckEmpty(url);
	
	if(strstr(url, "http://")==url)
		return PARAM_OK;
	else if(strstr(url, "file://")==url)
		return PARAM_OK;
	else
		return URL_UNKNOWN_SCHEME;
	
	return PARAM_UNKNOWN_ERROR;
}

PARAM_RES isIntegerFormatOK(const char *integer){
	int i = 0;
	int C;
	CheckNullPtr(integer);
	CheckEmpty(integer);

	while (C = integer[i++]) {
		if (isdigit(C) == 0) {
			return PARAM_INVALID;
		}
	}
	return PARAM_OK;
}

PARAM_RES isHashAlgFormatOK(const char *hashAlg){
	CheckNullPtr(hashAlg);
	CheckEmpty(hashAlg);
	return PARAM_OK;
}

static int doFileExists(const char* path){
	if(_access_s(path, F_OK) == 0) return 0;
	else return errno;
}

PARAM_RES analyseInputFile(const char* path){
	PARAM_RES res = PARAM_UNKNOWN_ERROR;
	int fileStatus = EINVAL;
	res = isPathFormOk(path);
	if (res == PARAM_OK)
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
		return FILE_DOSE_NOT_EXIST;
		break;
	case EINVAL:
		return FILE_INVALID_PATH;
		break;
	}

	return PARAM_UNKNOWN_ERROR;
}

//TODO add some functionality
PARAM_RES analyseOutputFile(const char* path){
	CheckNullPtr(path);
	return PARAM_OK;
}

const char * getFormatErrorString(PARAM_RES res){
	switch (res) {
	case PARAM_OK: return "(Parameter OK)";
		break;
	case PARAM_NULLPTR: return "(Null)";
		break;
	case PARAM_NOCONTENT: return "(Parameter has no content)";
		break;
	case PARAM_INVALID: return "(Parameter is invalid)";
		break;
	case FILE_ACCESS_DENIED: return "(File access denied)";
		break;
	case FILE_DOSE_NOT_EXIST: return "(File dose not exist)";
		break;
	case FILE_INVALID_PATH: return "(Invalid path)";
		break;
	case URL_UNKNOWN_SCHEME: return "(URL scheme is unknown)";
		break;
	}

	return "(Unknown error)";
}
