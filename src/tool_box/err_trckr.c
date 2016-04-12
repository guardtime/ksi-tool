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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tool_box/err_trckr.h"
#include <ksi/compatibility.h>

typedef struct ERR_ENTRY_st {
	int code;
	int line;
	char message[MAX_MESSAGE_LEN];
	char fileName[MAX_FILE_NAME_LEN];
} ERR_ENTRY;

struct ERR_TRCKR_st {
	ERR_ENTRY err[MAX_ERROR_COUNT];
	unsigned count;
	int (*printErrors)(const char*, ...);
	const char *(*errCodeToString)(int);
};

static const char *dummy_errocode_to_string(int a) {
	return "-";
}

ERR_TRCKR *ERR_TRCKR_new(int (*printErrors)(const char*, ...),
		const char *(*errCodeToString)(int)) {
	ERR_TRCKR *tmp = NULL;

	tmp = (ERR_TRCKR*)malloc(sizeof(ERR_TRCKR));
	if (tmp == NULL) return NULL;

	tmp->count = 0;


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
	if(err->count >= MAX_ERROR_COUNT ) return;

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

void ERR_TRCKR_reset(ERR_TRCKR *err) {
	if (err == NULL) return;
	err->count = 0;
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

	return;
}
