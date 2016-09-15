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

#ifndef SMART_FILE_H
#define	SMART_FILE_H

#define SMART_FILE_ERROR_BASE 0x40001

enum smart_file_enum {
	SMART_FILE_OK = 0x00,
	SMART_FILE_INVALID_ARG = SMART_FILE_ERROR_BASE,
	SMART_FILE_INVALID_PATH,
	SMART_FILE_OUT_OF_MEM,
	SMART_FILE_INVALID_MODE,
	SMART_FILE_UNABLE_TO_OPEN,
	SMART_FILE_UNABLE_TO_READ,
	SMART_FILE_UNABLE_TO_WRITE,
	SMART_FILE_BUFFER_TOO_SMALL,
	SMART_FILE_NOT_OPEND,
	SMART_FILE_DOES_NOT_EXIST,
	SMART_FILE_OVERWRITE_RESTRICTED,
	SMART_FILE_ACCESS_DENIED,
	SMART_FILE_PIPE_ERROR,
	SMART_FILE_UNABLE_TO_GET_STATUS,
	SMART_FILE_UNKNOWN_ERROR
};

enum {
	SMART_FILE_TYPE_REGULAR = 0x01,
	SMART_FILE_TYPE_DIR,
	SMART_FILE_TYPE_UNKNOWN,
};

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct SMART_FILE_st SMART_FILE;

/**
 * Smart file object that is used to open read and write files and streams. If user
 * wants to read from stdin or write to stdout file name '-' must be used with mode
 * r or w accordingly together with s.
 *
 * Possible file open modes:
 * r - for reading.
 * w - for writing.
 * wf - fail if exists.
 * wi - generate new file name as name[num++].ext
 * rs - enable operations on stdin.
 * ws - enable operations on stdout.
 *
 * \param fname file name to be used.
 * \param mode	file open mode.
 * \param file	smart file return pointer.
 * \return SMART_FILE_OK if successful, error code otherwise.
 */
int SMART_FILE_open(const char *fname, const char *mode, SMART_FILE **file);

void SMART_FILE_close(SMART_FILE *file);
int SMART_FILE_write(SMART_FILE *file, char *raw, size_t raw_len, size_t *count);
int SMART_FILE_read(SMART_FILE *file, char *raw, size_t raw_len, size_t *count);
const char *SMART_FILE_getFname(SMART_FILE *file);
/**
 * 
 * @param file
 * @return A non-zero value is returned in the case that the end-of-file indicator associated with the stream is set.
 * Otherwise, zero is returned.
 */
int SMART_FILE_isEof(SMART_FILE *file);

int SMART_FILE_doFileExist(const char *path);
int SMART_FILE_isWriteAccess(const char *path);
int SMART_FILE_isReadAccess(const char *path);
int SMART_FILE_hasFileExtension(const char *path, const char *ext);
int SMART_FILE_isFileType(const char *path, int ftype);

const char* SMART_FILE_errorToString(int error_code);

#ifdef	__cplusplus
}
#endif

#endif	/* SMART_FILE_H */

