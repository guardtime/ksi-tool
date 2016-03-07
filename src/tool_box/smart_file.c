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
#include <errno.h>
#include <limits.h>
#include "tool_box/smart_file.h"

#ifdef _WIN32
#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#	define WIN_HANDLE
#else
#	define OPENF
#endif


struct SMART_FILE_st {
	void *file;

	int (*file_open)(const char *fname, const char *mode, void **file);
	int (*file_write)(void *file, char *raw, size_t raw_len, size_t *count);
	int (*file_read)(void *file, char *raw, size_t raw_len, size_t *count);
	int (*file_get_stream)(const char *mode, void **stream);
	void (*file_close)(void *file);

	int isEOF;
	int isOpen;
	int mustBeFreed;
};


/**
 * Select the implementation.
 */
#ifdef WIN_HANDLE
static int smart_file_open_win(const char *fname, const char *mode, void **file);
static void smart_file_close_win(void *file);
static int smart_file_read_win(void *file, char *raw, size_t raw_len, size_t *count);
static int smart_file_write_win(void *file, char *raw, size_t raw_len, size_t *count);
static int smart_file_get_error_win(voif);
static int smart_file_get_stream_win(const char *mode, void **stream);

#else
static int smart_file_open_unix(const char *fname, const char *mode, void **file);
static void smart_file_close_unix(void *file);
static int smart_file_read_unix(void *file, char *raw, size_t raw_len, size_t *count);
static int smart_file_write_unix(void *file, char *raw, size_t raw_len, size_t *count);
static int smart_file_get_stream_unix(const char *mode, void **stream);
#endif

#ifdef WIN_HANDLE
static int smart_file_init_win(SMART_FILE *file) {
	int res;

	if (file == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	file->file = NULL;
	file->file_open = smart_file_open_win;
	file->file_close = smart_file_close_win;
	file->file_read = smart_file_read_win;
	file->file_write = smart_file_write_win;
	file->file_get_stream = smart_file_get_stream_win;

	res = SMART_FILE_OK;

cleanup:

	return res;
}

static int smart_file_open_win(const char *fname, const char *mode, void **file) {
	int res;
	int is_plus = 0;
	int is_b = 0;
	int is_r = 0;
	int is_w = 0;
	int is_a = 0;
	HANDLE tmp = NULL;
	DWORD access = 0;
	DWORD open_mode = 0;

	if (fname == NULL || mode == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	is_plus = strchr(mode, '+') == NULL ? 0 : 1;
	is_b = strchr(mode, 'b') == NULL ? 0 : 1;
	is_r = strchr(mode, 'r') == NULL ? 0 : 1;
	is_w = strchr(mode, 'w') == NULL ? 0 : 1;
	is_a = strchr(mode, 'a') == NULL ? 0 : 1;

	/**
	 * Configure the IO mode.
     */
	if (is_r) {
		access |= GENERIC_READ;
		open_mode |= OPEN_EXISTING;
	} else if (is_w) {
		access |= GENERIC_WRITE;
		if (is_plus) {
			open_mode |= (OPEN_ALWAYS | TRUNCATE_EXISTING);
		} else {
			open_mode |= CREATE_ALWAYS;
		}
	} else {
		res = SMART_FILE_INVALID_MODE;
		goto cleanup;
	}

	tmp = CreateFile(fname, access, 0, NULL, open_mode, FILE_ATTRIBUTE_NORMAL, NULL);
	if (tmp == INVALID_HANDLE_VALUE) {
		res = smart_file_get_error_win();
		res = (res == SMART_FILE_UNKNOWN_ERROR) ? SMART_FILE_UNABLE_TO_OPEN : res;
		goto cleanup;
	}

	*file = (void*)tmp;
	tmp = NULL;
	res = SMART_FILE_OK;

cleanup:

	smart_file_close_win(tmp);

	return res;
}

static void smart_file_close_win(void *file) {
	HANDLE tmp = file;
	if (file == NULL) return;
	CloseHandle(tmp);
}

static int smart_file_read_win(void *file, char *raw, size_t raw_len, size_t *count) {
	int res;
	HANDLE tmp = file;
	DWORD read_count = 0;
	DWORD raw_size = 0;

	if (file == NULL || raw == NULL || raw_len == 0) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	raw_size = (raw_len > ULONG_MAX) ? ULONG_MAX : (DWORD)raw_len;

	/* TODO: Improve error handling.*/
	if (!ReadFile(tmp, (void*)raw, raw_size, &read_count, NULL)) {
		res = smart_file_get_error_win();
		res = (res == SMART_FILE_UNKNOWN_ERROR) ? SMART_FILE_UNABLE_TO_READ : res;
		goto cleanup;
	}

	if (count != NULL) {
		*count = (size_t)read_count;
	}

	res = SMART_FILE_OK;

cleanup:

	return res;
}

static int smart_file_write_win(void *file, char *raw, size_t raw_len, size_t *count) {
	int res;
	HANDLE tmp = file;
	DWORD write_count = 0;
	DWORD raw_size = 0;

	if (file == NULL || raw == NULL || raw_len == 0) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	raw_size = (raw_len > ULONG_MAX) ? ULONG_MAX : (DWORD)raw_len;

	if (!WriteFile(tmp, (void*)raw, raw_size, &write_count, NULL)) {
		res = smart_file_get_error_win();
		res = (res == SMART_FILE_UNKNOWN_ERROR) ? SMART_FILE_UNABLE_TO_WRITE : res;
		goto cleanup;
	}

	if (count != NULL) {
		*count = (size_t)write_count;
	}

	res = SMART_FILE_OK;

cleanup:

	return res;
}

static int smart_file_get_error_win(void) {
	DWORD error_code = 0;
	int smart_file_error_code = 0;


	error_code = GetLastError();

	switch(error_code) {
		case ERROR_BROKEN_PIPE:
			smart_file_error_code = SMART_FILE_PIPE_ERROR;
		break;

		case ERROR_INSUFFICIENT_BUFFER:
		case ERROR_MORE_DATA:
			smart_file_error_code =  SMART_FILE_BUFFER_TOO_SMALL;
		break;

		case ERROR_FILE_NOT_FOUND:
			smart_file_error_code =  SMART_FILE_DOES_NOT_EXIST;
		break;

		case ERROR_ACCESS_DENIED:
			smart_file_error_code =  SMART_FILE_ACCESS_DENIED;
		break;

		default:
			smart_file_error_code = SMART_FILE_UNKNOWN_ERROR;
		break;
	}

	return smart_file_error_code;
}

static int smart_file_get_stream_win(const char *mode, void **stream) {
	int res;
	int is_r = 0;
	int is_w = 0;
	int is_b = 0;
	HANDLE fp = NULL;

	if (mode == NULL || stream == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	is_r = strchr(mode, 'r') == NULL ? 0 : 1;
	is_w = strchr(mode, 'w') == NULL ? 0 : 1;
	is_b = strchr(mode, 'b') == NULL ? 0 : 1;


	if (is_r) {
		fp = GetStdHandle(STD_INPUT_HANDLE);
	} else if (is_w) {
		fp = GetStdHandle(STD_OUTPUT_HANDLE);
	} else {
		res = SMART_FILE_INVALID_MODE;
		goto cleanup;
	}

	if (fp == INVALID_HANDLE_VALUE) {
		res = smart_file_get_error_win();
		res = (res == SMART_FILE_UNKNOWN_ERROR) ? SMART_FILE_UNABLE_TO_OPEN : res;
		goto cleanup;
	}

	*stream = fp;
	fp = NULL;

	res = SMART_FILE_OK;

cleanup:

	return res;
}
#else
static int smart_file_init_unix(SMART_FILE *file) {
	int res;

	if (file == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	file->file = NULL;
	file->file_open = smart_file_open_unix;
	file->file_close = smart_file_close_unix;
	file->file_read = smart_file_read_unix;
	file->file_write = smart_file_write_unix;
	file->file_get_stream = smart_file_get_stream_unix;

	res = SMART_FILE_OK;

cleanup:

	return res;
}

static int smart_file_open_unix(const char *fname, const char *mode, void **file) {
	int res;
	FILE *tmp = NULL;

	if (fname == NULL || mode == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}


	if (fname == NULL || mode == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	tmp = fopen(fname, mode);
	if (tmp == NULL) {
		res = SMART_FILE_UNABLE_TO_OPEN;
		goto cleanup;
	}

	*file = (void*)tmp;
	tmp = NULL;
	res = SMART_FILE_OK;

cleanup:

	smart_file_close_unix(tmp);

	return res;
}

static void smart_file_close_unix(void *file) {
	FILE *tmp = file;
	if (file == NULL) return;
	fclose(tmp);
}

static int smart_file_read_unix(void *file, char *raw, size_t raw_len, size_t *count) {
	int res;
	FILE *fp = file;
	size_t read_count = 0;

	if (file == NULL || raw == NULL || raw_len == 0) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	read_count = fread(raw, 1, raw_len, fp);
	/* TODO: Improve error handling.*/
	if (read_count == 0 && !feof(fp)) {
		res = SMART_FILE_UNABLE_TO_READ;
		goto cleanup;
	}


	if (count != NULL) {
		*count = (size_t)read_count;
	}

	res = SMART_FILE_OK;

cleanup:

	return res;
}

static int smart_file_write_unix(void *file, char *raw, size_t raw_len, size_t *count) {
	int res;
	FILE *fp = file;
	size_t wrrite_count = 0;

	if (file == NULL || raw == NULL || raw_len == 0) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	wrrite_count = fwrite(raw, 1, raw_len, fp);
	/* TODO: Improve error handling.*/

	if (count != NULL) {
		*count = (size_t)wrrite_count;
	}

	res = SMART_FILE_OK;

cleanup:

	return res;
}

static int smart_file_get_stream_unix(const char *mode, void **stream) {
	int res;
	int is_r = 0;
	int is_w = 0;
	int is_b = 0;
	FILE *fp = NULL;

	if (mode == NULL || stream == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	is_r = strchr(mode, 'r') == NULL ? 0 : 1;
	is_w = strchr(mode, 'w') == NULL ? 0 : 1;
	is_b = strchr(mode, 'b') == NULL ? 0 : 1;


	if (is_r) {
		fp = stdin;
	} else if (is_w) {
		fp = stdout;
	} else {
		res = SMART_FILE_INVALID_MODE;
		goto cleanup;
	}

#ifdef _WIN32
	if (is_b) {
		res = _setmode(_fileno(fp),_O_BINARY);
		if (res == -1) {
			res = SMART_FILE_PIPE_ERROR;
			goto cleanup;
		}
	}
#endif

	*stream = fp;
	fp = NULL;

	res = SMART_FILE_OK;

cleanup:

	return res;
}
#endif


int SMART_FILE_open(const char *fname, const char *mode, SMART_FILE **file) {
	int res;
	SMART_FILE *tmp = NULL;
	int doClose = 0;

	tmp = (SMART_FILE*)malloc(sizeof(SMART_FILE));
	if (tmp == NULL) {
		res = SMART_FILE_OUT_OF_MEM;
		goto cleanup;
	}

	/**
	 * Initialize smart file.
     */
	tmp->file = NULL;
	tmp->isEOF = 0;
	tmp->isOpen = 0;
	tmp->mustBeFreed = 0;

	/**
	 * Initialize implementations.
     */
#ifdef WIN_HANDLE
	res = smart_file_init_win(tmp);
	if (res != SMART_FILE_OK) goto cleanup;
#else
	res = smart_file_init_unix(tmp);
	if (res != SMART_FILE_OK) goto cleanup;
#endif

	/**
	 * If standard strem is wanted, extract the stream object. Otherwise use
	 * smart file opener function.
     */
	if (strcmp(fname, "-") == 0) {
		res = tmp->file_get_stream(mode, &(tmp->file));
		if (res != SMART_FILE_OK) goto cleanup;
	} else {
		res = tmp->file_open(fname, mode, &(tmp->file));
		if (res != SMART_FILE_OK) goto cleanup;
		tmp->mustBeFreed = 1;
	}

	if (res != SMART_FILE_OK) goto cleanup;

	/**
	 * File is opened.
	 */
	tmp->isOpen = 1;
	*file = tmp;
	tmp = NULL;

	res = SMART_FILE_OK;

cleanup:

	SMART_FILE_close(tmp);

	return res;
}

void SMART_FILE_close(SMART_FILE *file) {
	if (file != NULL) {
		if (file->mustBeFreed && file->file != NULL && file->file_close != NULL) {
			file->file_close(file->file);
		}

		free(file);
	}
}

int SMART_FILE_write(SMART_FILE *file, char *raw, size_t raw_len, size_t *count) {
	int res;
	size_t c = 0;

	if (file == NULL || raw == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	if (file->file != NULL && file->isOpen) {
		res = file->file_write(file->file, raw, raw_len, &c);
		if (res != SMART_FILE_OK) goto cleanup;
	} else {
		return SMART_FILE_NOT_OPEND;
	}

	if (count != NULL) {
		*count = c;
	}

	if (c != raw_len) {
		res = SMART_FILE_UNABLE_TO_WRITE;
		goto cleanup;
	}

	res = SMART_FILE_OK;

cleanup:

	return res;
}

int SMART_FILE_read(SMART_FILE *file, char *raw, size_t raw_len, size_t *count) {
	int res;
	size_t c = 0;

	if (file == NULL || raw == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	if (file->file != NULL && file->isOpen) {
		res = file->file_read(file->file, raw, raw_len, &c);
		if (res != SMART_FILE_OK) goto cleanup;
	} else {
		return SMART_FILE_NOT_OPEND;
	}

	if (c == 0) {
		file->isEOF = 1;
	}

	if (count != NULL) {
		*count = c;
	}
	res = SMART_FILE_OK;

cleanup:

	return res;
}

int SMART_FILE_isEof(SMART_FILE *file) {
	if (file == NULL) return 0;
	if (file->isOpen == 0) return 0;
	return file->isEOF;
}