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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "err_trckr.h"
#include "common.h"
#include <ksi/compatibility.h>

typedef struct ERR_ENTRY_st {
	int code;
	int line;
	char message[MAX_MESSAGE_LEN];
	char fileName[MAX_FILE_NAME_LEN];
} ERR_ENTRY;

struct ERR_TRCKR_st {
	ERR_ENTRY err[MAX_ERROR_COUNT];
	char additionalInfo[MAX_ADDITIONAL_INFO_LEN];
	size_t additionalInfo_len;
	unsigned count;
	int (*printErrors)(const char*, ...);
	const char *(*errCodeToString)(int);
};

static const char *dummy_errocode_to_string(int a) {
	VARIABLE_IS_NOT_USED(a);
	return "-";
}

ERR_TRCKR *ERR_TRCKR_new(int (*printErrors)(const char*, ...),
		const char *(*errCodeToString)(int)) {
	ERR_TRCKR *tmp = NULL;

	tmp = (ERR_TRCKR*)malloc(sizeof(ERR_TRCKR));
	if (tmp == NULL) return NULL;

	tmp->count = 0;
	tmp->additionalInfo_len = 0;
	tmp->additionalInfo[0] = '\0';

	tmp->printErrors = printErrors != NULL ? printErrors : printf;
	tmp->errCodeToString = errCodeToString != NULL ? errCodeToString : dummy_errocode_to_string;
	return tmp;
}

void ERR_TRCKR_free(ERR_TRCKR *obj) {
	free(obj);
	return;
}

void ERR_TRCKR_add(ERR_TRCKR *err, int code, const char *fname, int lineN, const char *msg, ...) {
	va_list va;
	if (err == NULL || fname == NULL) return;
	if (err->count >= MAX_ERROR_COUNT ) return;

	va_start(va, msg);
	KSI_vsnprintf(err->err[err->count].message, MAX_MESSAGE_LEN, msg == NULL ? "" : msg, va);
	va_end(va);
	err->err[err->count].message[MAX_MESSAGE_LEN -1] = 0;


	KSI_strncpy(err->err[err->count].fileName, fname, MAX_FILE_NAME_LEN);
	err->err[err->count].fileName[MAX_FILE_NAME_LEN -1] = 0;
	err->err[err->count].line = lineN;
	err->err[err->count].code = code;
	err->count++;

	return;
}

void ERR_TRCKR_addAdditionalInfo(ERR_TRCKR *err, const char *info, ...) {
	size_t count = 0;
	va_list va;

	if (err == NULL || info == NULL) return;
	if (err->additionalInfo_len >= MAX_ADDITIONAL_INFO_LEN) return;

	count = err->additionalInfo_len;

	va_start(va, info);
	count += KSI_vsnprintf(err->additionalInfo + count, MAX_ADDITIONAL_INFO_LEN - count, info, va);
	va_end(va);

	err->additionalInfo_len = count;

	return;
}

void ERR_TRCKR_reset(ERR_TRCKR *err) {
	if (err == NULL) return;
	err->count = 0;
	err->additionalInfo_len = 0;
	err->additionalInfo[0] = '\0';
}

void ERR_TRCKR_printErrors(ERR_TRCKR *err) {
	int i;

	if (err == NULL) return;

	for (i = err->count - 1; i >= 0; i--) {
		if (err->err[i].message[0] == '\0') {
			err->printErrors("%i) %s%s",i+1,  err->errCodeToString(err->err[i].code), (err->err[i].message[strlen(err->err[i].message) - 1] == '\n') ? ("") : ("\n"));
		} else {
			err->printErrors("%i) %s%s",i+1,  err->err[i].message, (err->err[i].message[strlen(err->err[i].message) - 1] == '\n') ? ("") : ("\n"));
		}
	}

	if (err->additionalInfo_len > 0) {
		err->printErrors("\nAdditional info:\n");
		err->printErrors("%s\n", err->additionalInfo);
	}

	return;
	}

void ERR_TRCKR_printExtendedErrors(ERR_TRCKR *err) {
	int i;

	if (err == NULL) return;

	for (i = err->count - 1; i >= 0; i--) {
		err->printErrors("%i) %s (%i) %s [%s 0x%02x]%s",i+1,
				err->err[i].fileName,
				err->err[i].line,
				err->err[i].message,
				err->errCodeToString(err->err[i].code),
				err->err[i].code,
				(err->err[i].message[strlen(err->err[i].message) - 1] == '\n') ? ("") : ("\n") );
	}

	if (err->additionalInfo_len > 0) {
		err->printErrors("\nAdditional info:\n");
		err->printErrors("%s\n", err->additionalInfo);
	}

	return;
}

int ERR_TRCKR_getErrCount(ERR_TRCKR *err) {
	if (err == NULL) return 0;
	else return err->count;
}
