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

#include "smart_file.h"
#include "../ksitool_err.h"

#ifdef _WIN32
#	include <io.h>
#	define F_OK 0
#else
#	include <unistd.h>
#	define _access_s access
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#include <stdlib.h>
#else
#       include <limits.h>
#	include <sys/time.h>
#endif

#include <errno.h>




struct SMART_FILE_st {
	FILE *file;
	int isOpen;
	int mustBeFreed;
};

/* TODO: Check if access rights can be examined. */
static int smart_file_get_stream_from_path(const char *fname, const char *mode, FILE **stream, int *close) {
	int res;
	FILE *in = NULL;
	FILE *tmp = NULL;
	int doClose = 0;

	if (fname == NULL || mode == NULL || stream == NULL || close == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (strcmp(fname, "-") == 0) {
		tmp = strcmp(mode, "rb") == 0 ? stdin : stdout;
#ifdef _WIN32
		res = _setmode(_fileno(tmp),_O_BINARY);
		if (res == -1) {
			res = KT_UNABLE_TO_SET_STREAM_MODE;
			goto cleanup;
		}
#endif
	} else {
		in = fopen(fname, mode);
		if (in == NULL) {
			res = KT_IO_ERROR;
			goto cleanup;
		}

		doClose = 1;
		tmp = in;
	}

	*stream = tmp;
	in = NULL;
	*close = 1;

	res = KT_OK;

cleanup:

	if (in) fclose(in);
	return res;
}

int SMART_FILE_open(const char *fname, const char *mode, SMART_FILE **file) {
	int res;
	FILE *fp = NULL;
	SMART_FILE *tmp = NULL;
	int doClose = 0;

	tmp = (SMART_FILE*)malloc(sizeof(SMART_FILE));
	if (tmp == NULL) {
		res = KT_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->file = NULL;
	tmp->isOpen = 0;
	tmp->mustBeFreed = 0;

	res = smart_file_get_stream_from_path(fname, mode, &fp, &doClose);
	if (res != KT_OK) goto cleanup;

	tmp->file = fp;
	tmp->mustBeFreed = doClose;
	tmp->isOpen = 1;
	fp = NULL;

	*file = tmp;
	tmp = NULL;

	res = KT_OK;

cleanup:

	SMART_FILE_close(tmp);
	if (doClose && fp != NULL) fclose(fp);
	return res;
}

void SMART_FILE_close(SMART_FILE *file) {
	if (file != NULL) {
		if (file->mustBeFreed && file->file != NULL) {
			fclose(file->file);
		}

		free(file);
	}
}

int SMART_FILE_write(SMART_FILE *file, char *raw, size_t raw_len, size_t *count) {
	size_t c = 0;

	if (file == NULL || raw == NULL) return KT_INVALID_ARGUMENT;

	if (file->file != NULL && file->isOpen) {
		c = fwrite(raw, 1, raw_len, file->file);
	} else {
		return KT_FILE_NOT_OPEND;
	}

	if (count != NULL) {
		*count = c;
	}

	if (c != raw_len) {
		return KT_INVALID_IO_WRITE;
	}
	return KT_OK;
}

int SMART_FILE_read(SMART_FILE *file, char *raw, size_t raw_len, size_t *count) {
	size_t c = 0;

	if (file == NULL || raw == NULL) return KT_INVALID_ARGUMENT;

	if (file->file != NULL && file->isOpen) {
		c = fread(raw, 1, raw_len, file->file);
	} else {
		return KT_FILE_NOT_OPEND;
	}

	if (count != NULL) {
		*count = c;
	}
	/* TODO: Check if count must be validated. */
	return KT_OK;
}

int SMART_FILE_isEof(SMART_FILE *file) {
	if (file == NULL || file->file == NULL || file->isOpen == 0) return 0;
	return feof(file->file);
}

int SMART_FILE_isError(SMART_FILE *file) {
	if (file == NULL || file->file == NULL || file->isOpen == 0) return 0;
	return ferror(file->file);
}

