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

#ifndef PARAM_CONTROL_H
#define	PARAM_CONTROL_H

#include "tool_box/err_trckr.h"
#include "param_set/param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct COMPOSITE_st COMPOSITE;

/**
 * A helper data structure to enable error handling and extra context for extracting
 * different objects.
 */
struct COMPOSITE_st {
	/** A pointer to KSI context. Mandatory. */
	void *ctx;
	
	/** A pointer to tool error handler. Mandatory. */
	void *err;
	
	/** A pointer to hash algorithm. Mandatory if hashing input file. */
	void *h_alg;
	
	/** A optional pointer to file name to save input data to file when hashing. */
	void *fname_out;
};


enum contentStatus {
	PARAM_OK = 0x00,
	PARAM_INVALID,
	HASH_ALG_INVALID_NAME,
	HASH_IMPRINT_INVALID_LEN,
	INTEGER_TOO_LARGE,
	ONLY_REGULAR_FILES,
	FILE_ACCESS_DENIED,
	FILE_DOES_NOT_EXIST,
	FILE_INVALID_PATH,
	PARAM_UNKNOWN_ERROR
};

/* TODO: Refactor error codes*/
enum formatStatus_enum{
	FORMAT_OK = PARAM_OK,
	FORMAT_NULLPTR = PARAM_UNKNOWN_ERROR + 1,
	FORMAT_NOCONTENT,
	FORMAT_INVALID,
	FORMAT_IMPRINT_NO_COLON,
	FORMAT_IMPRINT_NO_HASH_ALG,
	FORMAT_IMPRINT_NO_HASH,
	FORMAT_INVALID_HEX_CHAR,
	FORMAT_NOT_INTEGER,
	FORMAT_INVALID_BASE32_CHAR,
	FORMAT_INVALID_OID,
	FORMAT_URL_UNKNOWN_SCHEME,
	FORMAT_FLAG_HAS_ARGUMENT,
	FORMAT_INVALID_UTC,		
	FORMAT_INVALID_UTC_OUT_OF_RANGE,		
	FORMAT_UNKNOWN_ERROR
};

const char *getParameterErrorString(int res);

int isFormatOk_hashAlg(const char *hashAlg);
int isContentOk_hashAlg(const char *alg);
/** extra is not used.*/
int extract_hashAlg(void *extra, const char* str, void** obj);

int isFormatOk_inputFile(const char *path);
int isContentOk_inputFile(const char* path);
int isContentOk_inputFileRestrictPipe(const char* path);
int extract_inputFile(void *extra, const char* str, void** obj);

int isFormatOk_path(const char *path);
int convertRepair_path(const char* arg, char* buf, unsigned len);

int isFormatOk_inputHash(const char *str);
int isContentOk_inputHash(const char *str);

/**
 * Requires \c COMPOSITE as extra. \c ctx, and \c err must not be NULL. \c h_alg
 * can be omitted if extracting imprint. \c fname_out must be NULL if input data
 * is not written into file / stream.
 */
int extract_inputHash(void *extra, const char* str, void** obj);

int isFormatOk_int(const char *integer);
int isContentOk_int(const char* integer);
int extract_int(void *extra, const char* str,  void** obj);

int isFormatOk_url(const char *url);
int convertRepair_url(const char* arg, char* buf, unsigned len);

int isFormatOk_pubString(const char *str);
int extract_pubString(void *extra, const char* str, void** obj);

int isFormatOk_timeString(const char *time);
int isFormatOk_utcTime(const char *time);
int isContentOk_utcTime(const char *time);
int extract_utcTime(void *extra, const char* str, void** obj);

int isFormatOk_flag(const char *flag);
int isFormatOk_constraint(const char *constraint);
int isFormatOk_userPass(const char *uss_pass);

int isFormatOk_oid(const char *constraint);
int convertRepair_constraint(const char* arg, char* buf, unsigned len);


int extract_inputSignature(void *extra, const char* str, void** obj);


int get_pipe_out_error(PARAM_SET *set, ERR_TRCKR *err, const char *out_file_names, const char *print_out_names);

int get_pipe_in_error(PARAM_SET *set, ERR_TRCKR *err, const char *in_file_names, const char *read_in_flags);

int is_imprint(const char *str);
#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_CONTROL_H */

