/* 
 * File:   gt_cmd_control.h
 * Author: Taavi
 *
 * Created on July 10, 2014, 6:19 PM
 */

#ifndef GT_CMD_CONTROL_H
#define	GT_CMD_CONTROL_H

#include "gt_cmd_common.h"

#ifdef	__cplusplus
extern "C" {
#endif


typedef enum contentStatus {
	PARAM_OK,
	PARAM_INVALID,
	HASH_ALG_INVALID_NAME,
	HASH_IMPRINT_INVALID_LEN,
	FILE_ACCESS_DENIED,
	FILE_DOES_NOT_EXIST,
	FILE_INVALID_PATH,
	PARAM_UNKNOWN_ERROR
} ContentStatus;

typedef enum formatStatus_enum{
	FORMAT_OK,
	FORMAT_NULLPTR,
	FORMAT_NOCONTENT,
	FORMAT_INVALID,
	FORMAT_URL_UNKNOWN_SCHEME,
	FORMAT_FLAG_HAS_ARGUMENT,
	FORMAT_UNKNOWN_ERROR
} FormatStatus;

FormatStatus isPathFormOk(const char *path);
FormatStatus isHexFormatOK(const char *hex);
FormatStatus isURLFormatOK(const char *url);
FormatStatus isIntegerFormatOK(const char *integer);
FormatStatus isHashAlgFormatOK(const char *hashAlg);
FormatStatus isImprintFormatOK(const char *hashAlg);
FormatStatus isFlagFormatOK(const char *hashAlg);
FormatStatus isEmailFormatOK(const char *email);
FormatStatus isUserPassFormatOK(const char *uss_pass);
FormatStatus formatIsOK(const char *obj);

ContentStatus isInputFileContOK(const char* path);
ContentStatus isOutputFileContOK(const char* path);
ContentStatus isHashAlgContOK(const char *alg);
ContentStatus isImprintContOK(const char *imprint);
ContentStatus contentIsOK(const char *alg);

const char *getFormatErrorString(FormatStatus res);
const char *getParameterContentErrorString(ContentStatus res);


#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMD_CONTROL_H */

