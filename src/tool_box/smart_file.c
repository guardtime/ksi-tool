/**************************************************************************
 *
 * GUARDTIME CONFIDENTIAL
 *
 * Copyright (C) [2016] Guardtime, Inc
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
#include <ksi/compatibility.h>
#include "tool_box/smart_file.h"
#include "tool_box/tool_box.h"

#ifdef _WIN32
#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#	define WIN_HANDLE
#	define F_OK 0
#	define W_OK 2
#	define R_OK 4
#else
#	include <unistd.h>
#	define OPENF
#endif

#include <sys/types.h>
#include <sys/stat.h>

struct SMART_FILE_st {
	char fname[1024];
	void *file;

	int (*file_open)(const char *fname, const char *mode, void **file);
	int (*file_write)(void *file, char *raw, size_t raw_len, size_t *count);
	int (*file_read)(void *file, char *raw, size_t raw_len, size_t *count);
	int (*file_get_stream)(const char *mode, void **stream, int *is_close_mandatory);
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
static int smart_file_get_error_win(unsigned);
static int smart_file_get_stream_win(const char *mode, void **stream, int *is_close_mandatory);

#else
static int smart_file_open_unix(const char *fname, const char *mode, void **file);
static void smart_file_close_unix(void *file);
static int smart_file_read_unix(void *file, char *raw, size_t raw_len, size_t *count);
static int smart_file_write_unix(void *file, char *raw, size_t raw_len, size_t *count);
static int smart_file_get_stream_unix(const char *mode, void **stream, int *is_close_mandatory);
static int smart_file_get_error_unix(void);
#endif

static int is_access(const char *path, int mode) {
	int res;
	if (path == NULL) return 0;
#ifdef _WIN32
	res = _access(path, mode) == 0 ? 1 : 0;
#else
	res = access(path, mode) == 0 ? 1 : 0;
#endif
	return res;
}

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
		res = smart_file_get_error_win(GetLastError());
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
	DWORD error_code = 0;

	if (file == NULL || raw == NULL || raw_len == 0) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	raw_size = (raw_len > ULONG_MAX) ? ULONG_MAX : (DWORD)raw_len;

	if (!ReadFile(tmp, (void*)raw, raw_size, &read_count, NULL)) {
		/**
		 * When ReadFile reads from pipe (stdin HANDLE) it finishes with success
		 * and the next read operation fails with ERROR_BROKEN_PIPE that means
		 * PIPE has been closed. Set status code to success and read count to zero
		 * to indicate EOF.
         */
		error_code = GetLastError();
		if (GetFileType(tmp) == FILE_TYPE_PIPE && error_code == ERROR_BROKEN_PIPE) {
			res = SMART_FILE_OK;
			read_count = 0;
		} else {
			res = smart_file_get_error_win(error_code);
			res = (res == SMART_FILE_UNKNOWN_ERROR) ? SMART_FILE_UNABLE_TO_READ : res;
			goto cleanup;
		}
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
		res = smart_file_get_error_win(GetLastError());
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

static int smart_file_get_error_win(unsigned error) {
	int smart_file_error_code = 0;


	switch(error) {
		case ERROR_SUCCESS:
			smart_file_error_code = SMART_FILE_OK;
		break;

		case ERROR_BAD_PIPE:
		case ERROR_PIPE_BUSY:
		case ERROR_BROKEN_PIPE:
		case ERROR_NO_DATA:
		case ERROR_PIPE_NOT_CONNECTED:
			smart_file_error_code = SMART_FILE_PIPE_ERROR;
		break;

		case ERROR_DISK_FULL:
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			smart_file_error_code = SMART_FILE_OUT_OF_MEM;
		break;

		case ERROR_INSUFFICIENT_BUFFER:
		case ERROR_MORE_DATA:
			smart_file_error_code =  SMART_FILE_BUFFER_TOO_SMALL;
		break;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_INVALID_DRIVE:
			smart_file_error_code =  SMART_FILE_DOES_NOT_EXIST;
		break;

		case ERROR_BUSY:
		case ERROR_OPEN_FAILED:
		case ERROR_DRIVE_LOCKED:
			smart_file_error_code =  SMART_FILE_UNABLE_TO_OPEN;
		break;

		case ERROR_FILE_TOO_LARGE:
			smart_file_error_code =  SMART_FILE_UNABLE_TO_WRITE;
		break;

		case ERROR_ACCESS_DENIED:
		case ERROR_WRITE_PROTECT:
			smart_file_error_code =  SMART_FILE_ACCESS_DENIED;
		break;

		case ERROR_DIRECTORY:
		case ERROR_INVALID_NAME:
		case ERROR_BAD_PATHNAME:
			smart_file_error_code =  SMART_FILE_INVALID_PATH;
		break;

		default:
			smart_file_error_code = SMART_FILE_UNKNOWN_ERROR;
		break;
	}

	return smart_file_error_code;
}

static int smart_file_get_stream_win(const char *mode, void **stream, int *is_close_mandatory) {
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

	if (fp == INVALID_HANDLE_VALUE || fp == NULL) {
		res = smart_file_get_error_win(GetLastError());
		res = (res == SMART_FILE_UNKNOWN_ERROR) ? SMART_FILE_UNABLE_TO_OPEN : res;
		goto cleanup;
	}

	/**
	 * Handle to stdin or stdout must be closed.
     */
	*is_close_mandatory = 1;
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
		res = smart_file_get_error_unix();
		res = (res == SMART_FILE_UNKNOWN_ERROR) ? SMART_FILE_UNABLE_TO_OPEN : res;
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
		res = smart_file_get_error_unix();
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

static int smart_file_write_unix(void *file, char *raw, size_t raw_len, size_t *count) {
	int res;
	FILE *fp = file;
	size_t write_count = 0;

	if (file == NULL || raw == NULL || raw_len == 0) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}

	write_count = fwrite(raw, 1, raw_len, fp);

	if (write_count != raw_len) {
		res = smart_file_get_error_unix();
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

static int smart_file_get_stream_unix(const char *mode, void **stream, int *is_close_mandatory) {
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

	*is_close_mandatory = 0;
	*stream = fp;
	fp = NULL;

	res = SMART_FILE_OK;

cleanup:

	return res;
}

static int smart_file_get_error_unix(void) {
	int error_code = 0;
	int smart_file_error_code = 0;


	error_code = errno;

	switch(error_code) {
		case 0:
			smart_file_error_code =  SMART_FILE_OK;
		break;

		case ENOENT:
			smart_file_error_code =  SMART_FILE_DOES_NOT_EXIST;
		break;

		case EACCES:
			smart_file_error_code =  SMART_FILE_ACCESS_DENIED;
		break;

		case EINVAL:
			smart_file_error_code =  SMART_FILE_INVALID_PATH;
		break;

		default:
			smart_file_error_code = SMART_FILE_UNKNOWN_ERROR;
		break;
	}

	return smart_file_error_code;
}

#endif

char *generate_file_name(const char *fname, int count, char *buf, size_t buf_len) {
	char *ret = NULL;
	char ext[1024] = "";
	char num[1024] = "";
	char root[1024] = "";
	int is_extension = 0;
	int is_counting = 0;
	int i = 0;
	int root_offset = 0;


	/**
	 * Extract the files extension.
	 */
	ret = STRING_extractAbstract(fname, ".", NULL, ext, sizeof(ext), find_charAfterLastStrn, NULL, NULL);
	is_extension = (ret == ext) ? 1 : 0;

	if (is_extension) {
		root_offset += (int)strlen(ext);
	}

	KSI_strncpy(root, fname, strlen(fname) - root_offset);

	KSI_snprintf(buf, buf_len, "%s_%i%s%s", root, count,
		is_extension ? "." : "",
		is_extension ? ext : "");
	return buf;
}

const char *generate_not_existing_file_name(const char *fname, char *buf, size_t buf_len, int use_binary_search) {
	const char *pFname = fname;
	int i = 1;
	unsigned ceil = 16;
	int j = 1;
	int a = 0;
	int b = 0;
	int d = 0;

	if (fname == NULL || buf == NULL || buf_len == 0) return NULL;

	/**
	 * Support for binary search algorithm.
	 */
	if (use_binary_search) {
		/**
		 * Search the highest file name that does not exist.
		 */
		do {
			ceil <<=1;
			pFname = generate_file_name(fname, ceil, buf, buf_len);
			if (pFname == NULL) return NULL;
		} while (SMART_FILE_doFileExist(pFname));

		/**
		 * Use the binary search algorithm to find file name range.
		 */
		a = j;
		b = ceil;
		do {
			j = (a + b) / 2;
			d = b - a;

			pFname = generate_file_name(fname, j, buf, buf_len);
			if (pFname == NULL) return NULL;


			if (SMART_FILE_doFileExist(pFname)) {
				a = j;
			} else {
				b = j;
			}
		} while (d > 2);
		i = a;
	}

	do {
		pFname = generate_file_name(fname, i++, buf, buf_len);
		if (pFname == NULL) return NULL;
	} while (SMART_FILE_doFileExist(pFname));

	return pFname;
}


int SMART_FILE_open(const char *fname, const char *mode, SMART_FILE **file) {
	int res;
	SMART_FILE *tmp = NULL;
	int doClose = 0;
	int must_free = 0;
	int isStream = 0;
	const char *pFname = fname;
	char buf[2048];

	int is_w;
	int is_f;
	int is_i;
	int is_r;

	if (fname == NULL || mode == NULL || file == NULL) {
		res = SMART_FILE_INVALID_ARG;
		goto cleanup;
	}


	isStream = strcmp(fname, "-") == 0 ? 1 : 0,
	is_w = strchr(mode, 'w') == NULL ? 0 : 1;
	is_f = strchr(mode, 'f') == NULL ? 0 : 1;
	is_i = strchr(mode, 'i') == NULL ? 0 : 1;
	is_r = strchr(mode, 'r') == NULL ? 0 : 1;

	/**
	 * Some special flags that should be checked before going ahead.
     */
	if (!isStream) {
		if (is_w && is_f && SMART_FILE_doFileExist(fname)) {
			res = SMART_FILE_OVERWRITE_RESTRICTED;
			goto cleanup;
		}

		if (is_w && is_i && SMART_FILE_doFileExist(fname)) {
			pFname = generate_not_existing_file_name(fname, buf, sizeof(buf), 1);
		}
	}


	tmp = (SMART_FILE*)malloc(sizeof(SMART_FILE));
	if (tmp == NULL) {
		res = SMART_FILE_OUT_OF_MEM;
		goto cleanup;
	}

		/**
	 * Initialize smart file.
     */
	tmp->file = NULL;
	tmp->fname[0] = '\0';
	tmp->isEOF = 0;
	tmp->isOpen = 0;
	tmp->mustBeFreed = 0;

	KSI_snprintf(tmp->fname, sizeof(tmp->fname), "%s", pFname);


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
	if (isStream) {
		res = tmp->file_get_stream(mode, &(tmp->file), &must_free);
		if (res != SMART_FILE_OK) goto cleanup;
		tmp->mustBeFreed = must_free;
	} else {
		res = tmp->file_open(pFname, mode, &(tmp->file));
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

	/**
	 * EOF is detected as Read finished without an error and read count is zero.
     */
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

const char *SMART_FILE_getFname(SMART_FILE *file) {
	if (file == NULL) return NULL;
	if (file->isOpen == 0) return NULL;
	return file->fname;
}

int SMART_FILE_isEof(SMART_FILE *file) {
	if (file == NULL) return 0;
	if (file->isOpen == 0) return 0;
	return file->isEOF;
}

int SMART_FILE_doFileExist(const char *path) {
	int res = 0;
	if (path == NULL) return 0;
	res = is_access(path, F_OK);
	return res;
}

int SMART_FILE_isWriteAccess(const char *path) {
	int res = 0;
	if (path == NULL) return 0;
	res = is_access(path, F_OK | W_OK);
	return res;
}

int SMART_FILE_isReadAccess(const char *path) {
	int res = 0;
	if (path == NULL) return 0;
	res = is_access(path, F_OK | R_OK);
	return res;
}

const char* SMART_FILE_errorToString(int error_code) {
	switch (error_code) {
		case SMART_FILE_OK:
			return "OK.";
		case SMART_FILE_OUT_OF_MEM:
			return "Out of memory.";
		case SMART_FILE_INVALID_ARG:
			return "Invalid argument.";
		case SMART_FILE_INVALID_MODE:
			return "Invalid file open mode.";
		case SMART_FILE_UNABLE_TO_OPEN:
			return "Unable to open file.";
		case SMART_FILE_UNABLE_TO_READ:
			return "Unable to read from file.";
		case SMART_FILE_UNABLE_TO_WRITE:
			return "Unable to write to file.";
		case SMART_FILE_BUFFER_TOO_SMALL:
			return "Insufficient buffer size.";
		case SMART_FILE_NOT_OPEND:
			return "File is not opened.";
		case SMART_FILE_DOES_NOT_EXIST:
			return "File does not exist.";
		case SMART_FILE_OVERWRITE_RESTRICTED:
			return "Overwriting is restricted.";
		case SMART_FILE_ACCESS_DENIED:
			return "File access denied.";
		case SMART_FILE_PIPE_ERROR:
			return "Pipe error.";
		case SMART_FILE_INVALID_PATH:
			return "Invalid path.";
		case SMART_FILE_UNKNOWN_ERROR:
		default:
			return "Unknown error.";
	}
}