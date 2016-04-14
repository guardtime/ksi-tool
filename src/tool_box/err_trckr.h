/**************************************************************************
 *
 * GUARDTIME CONFIDENTIAL
 *
 * Copyright (C) [2015 - 2016] Guardtime, Inc
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

#ifndef ERR_TRCKR_H
#define	ERR_TRCKR_H

#include <stdio.h>

#define MAX_MESSAGE_LEN 1024
#define MAX_FILE_NAME_LEN 256
#define MAX_ERROR_COUNT 128

typedef struct ERR_TRCKR_st ERR_TRCKR;

#ifdef	__cplusplus
extern "C" {
#endif

ERR_TRCKR *ERR_TRCKR_new(int (*printErrors)(const char*, ...), const char *(*errCodeToString)(int));
void ERR_TRCKR_free(ERR_TRCKR *obj);
void ERR_TRCKR_add(ERR_TRCKR *err, int code, const char *fname, int lineN, const char *msg, ...);
void ERR_TRCKR_reset(ERR_TRCKR *err);
void ERR_TRCKR_printErrors(ERR_TRCKR *err);
void ERR_TRCKR_printExtendedErrors(ERR_TRCKR *err);

#define ERR_TRCKR_ADD(err, code, msg, ...) ERR_TRCKR_add(err, code, __FILE__, __LINE__, msg, ##__VA_ARGS__)

#ifdef	__cplusplus
}
#endif

#endif	/* ERR_TRCKR_H */

