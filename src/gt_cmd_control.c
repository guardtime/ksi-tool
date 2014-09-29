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

FormatStatus isPathFormOk(const char *path){
	if(path == NULL) return FORMAT_NULLPTR;
	return FORMAT_OK;
}

FormatStatus isHexFormatOK(const char *hex){
	int i = 0;
	char C;
	bool failure = false;

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
	
	if(strstr(url, "http://")==url)
		return FORMAT_OK;
	else if(strstr(url, "file://")==url)
		return FORMAT_OK;
	else
		return FORMAT_URL_UNKNOWN_SCHEME;
	
	return FORMAT_UNKNOWN_ERROR;
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
	FormatStatus status = FORMAT_UNKNOWN_ERROR;
	
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



static int doFileExists(const char* path){
	if(_access_s(path, F_OK) == 0) return 0;
	else return errno;
}

ContentStatus isInputFileContOK(const char* path){
	ContentStatus res = PARAM_UNKNOWN_ERROR;
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
ContentStatus isOutputFileContOK(const char* path){
	return PARAM_OK;
}

ContentStatus isHashAlgContOK(const char *alg){
	if(strcmp("SHA-1" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SHA-256" ,alg) == 0) return PARAM_OK;
	else if(strcmp("RIPEMD-160" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SHA-224" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SHA-384" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SHA-512" ,alg) == 0) return PARAM_OK;
	else if(strcmp("RIPEMD-256" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SHA3-244" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SHA3-256" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SHA3-384" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SHA3-512" ,alg) == 0) return PARAM_OK;
	else if(strcmp("SM3" ,alg) == 0) return PARAM_OK;
	else return HASH_ALG_INVALID_NAME;
}

ContentStatus ContentIsOK(const char *alg){
	return PARAM_OK;
}






const char *getFormatErrorString(FormatStatus res){
	switch (res) {
	case FORMAT_OK: return "(Parameter OK)";
		break;
	case FORMAT_NULLPTR: return "(Nullptr)";
		break;
	case FORMAT_NOCONTENT: return "(Parameter has no content)";
		break;
	case FORMAT_INVALID: return "(Parameter is invalid)";
		break;
	case FORMAT_URL_UNKNOWN_SCHEME: return "(URL scheme is unknown)";
		break;
	case FORMAT_FLAG_HAS_ARGUMENT: return "(Parameter must have no arguments)";
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
	case FILE_ACCESS_DENIED: return "(File access denied)";
		break;
	case FILE_DOSE_NOT_EXIST: return "(File dose not exist)";
		break;
	case FILE_INVALID_PATH: return "(Invalid path)";
		break;
	}

	return "(Unknown error)";
}
