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

#ifndef ERR_TRCKR_H
#define	ERR_TRCKR_H

#include <stdio.h>

#define MAX_ADDITIONAL_INFO_LEN 1024
#define MAX_MESSAGE_LEN 1024
#define MAX_FILE_NAME_LEN 256
#define MAX_ERROR_COUNT 16

typedef struct ERR_TRCKR_st ERR_TRCKR;

#ifdef	__cplusplus
extern "C" {
#endif

ERR_TRCKR *ERR_TRCKR_new(int (*printErrors)(const char*, ...), const char *(*errCodeToString)(int));
void ERR_TRCKR_free(ERR_TRCKR *obj);
void ERR_TRCKR_add(ERR_TRCKR *err, int code, const char *fname, int lineN, const char *msg, ...);
void ERR_TRCKR_reset(ERR_TRCKR *err);
void ERR_TRCKR_addAdditionalInfo(ERR_TRCKR *err, const char *info, ...);
void ERR_TRCKR_addWarning(ERR_TRCKR *err, const char *info, ...);
void ERR_TRCKR_printErrors(ERR_TRCKR *err);
void ERR_TRCKR_printExtendedErrors(ERR_TRCKR *err);
void ERR_TRCKR_printWarnings(ERR_TRCKR *err);
int ERR_TRCKR_getErrCount(ERR_TRCKR *err);

#define ERR_TRCKR_ADD(err, code, msg, ...) ERR_TRCKR_add(err, code, __FILE__, __LINE__, msg, ##__VA_ARGS__)

#ifdef	__cplusplus
}
#endif

#endif	/* ERR_TRCKR_H */

