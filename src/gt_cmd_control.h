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

#ifndef GT_CMD_CONTROL_H
#define	GT_CMD_CONTROL_H

#include "gt_cmd_common.h"

#ifdef	__cplusplus
extern "C" {
#endif


enum contentStatus {
	PARAM_OK = 0x00,
	PARAM_INVALID,
	HASH_ALG_INVALID_NAME,
	HASH_IMPRINT_INVALID_LEN,
	INTEGER_TOO_LARGE,
	FILE_ACCESS_DENIED,
	FILE_DOES_NOT_EXIST,
	FILE_INVALID_PATH,
	PARAM_UNKNOWN_ERROR
};

enum formatStatus_enum{
	FORMAT_OK = PARAM_OK,
	FORMAT_NULLPTR = PARAM_UNKNOWN_ERROR + 1,
	FORMAT_NOCONTENT,
	FORMAT_INVALID,
	FORMAT_INVALID_OID,
	FORMAT_URL_UNKNOWN_SCHEME,
	FORMAT_FLAG_HAS_ARGUMENT,
	FORMAT_INVALID_UTC,		
	FORMAT_INVALID_UTC_OUT_OF_RANGE,		
	FORMAT_UNKNOWN_ERROR
};

typedef int FormatStatus;
typedef int ContentStatus;

FormatStatus isPathFormOk(const char *path);
FormatStatus isHexFormatOK(const char *hex);
FormatStatus isURLFormatOK(const char *url);
FormatStatus isIntegerFormatOK(const char *integer);
FormatStatus isHashAlgFormatOK(const char *hashAlg);
FormatStatus isImprintFormatOK(const char *hashAlg);
FormatStatus isFlagFormatOK(const char *hashAlg);
FormatStatus isEmailFormatOK(const char *email);
FormatStatus isConstraintFormatOK(const char *constraint);
FormatStatus isUserPassFormatOK(const char *uss_pass);
FormatStatus formatIsOK(const char *obj);
FormatStatus isUTCTimeFormatOk(const char *time);

ContentStatus isInputFileContOK(const char* path);
ContentStatus isOutputFileContOK(const char* path);
ContentStatus isHashAlgContOK(const char *alg);
ContentStatus isImprintContOK(const char *imprint);
ContentStatus isIntegerContOk(const char* integer);
ContentStatus contentIsOK(const char *alg);

int convert_repairUrl(const char* arg, char* buf, unsigned len);
int convert_repairPath(const char* arg, char* buf, unsigned len);
int convert_replaceWithOid(const char* arg, char* buf, unsigned len);
int convert_UTC_to_UNIX(const char* arg, char* buf, unsigned len);

const char *getParameterErrorString(int res);

#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMD_CONTROL_H */

