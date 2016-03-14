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
#include <math.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <ksi/ksi.h>
#include "tool_box.h"

#include "param_control.h"
//#include "gt_task_support.h"
#include "obj_printer.h"
#include "param_set/param_value.h"
#include "../param_set/param_set.h"
#include "../param_set/task_def.h"
#include "ksi_init.h"
#include "../api_wrapper.h"
#include "ksi/compatibility.h"
#ifdef _WIN32
#	include <io.h>
#	define F_OK 0
#else
#	include <unistd.h>
#	define _access_s access
#endif

#include <errno.h>

#include "smart_file.h"



static int analyze_hexstring_format(const char *hex, double *cor) {
	int i = 0;
	int C;
	size_t count = 0;
	size_t valid_count = 0;
	double tmp = 0;

	if(hex == NULL) {
		return FORMAT_NULLPTR;
	}

	while ((C = 0xff & hex[i++]) != '\0') {
		count++;
		if (isxdigit(C)) {
			valid_count++;
		}
	}
	if(count > 0) {
		tmp = (double)valid_count / (double)count;
	}

	if (cor != NULL) {
		*cor = tmp;
	}

	if (valid_count == count && count > 0) {
		return FORMAT_OK;
	} else if (count == 0) {
		return FORMAT_NOCONTENT;
	} else if (valid_count != count) {
		return FORMAT_INVALID_HEX_CHAR;
	} else {
		return FORMAT_INVALID;
	}

}

static int imprint_extract_fields(const char *imprint, KSI_HashAlgorithm *ID, int *isColon, char *hash, size_t buf_len) {
	int res;
	int i = 0;
	int n = 0;
	int reading_alg = 1;
	const char *colon = NULL;
	char alg[1024];
	size_t size_limit = sizeof(alg);

	if (imprint == NULL || hash == NULL || buf_len == 0) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	colon = strchr(imprint, ':');

	if (colon != NULL) {
		while (imprint[i] != '\0') {
			if (colon == &imprint[i]) {
				reading_alg = 0;
				alg[n] = '\0';
				size_limit = buf_len;
				n = 0;
				i++;
			}

			if (n < size_limit - 1) {
				if (reading_alg) {
					alg[n] = imprint[i];
				} else {
					hash[n] = imprint[i];
				}
				n++;
			}
		i++;
		}

		hash[n] = '\0';
	} else {
		alg[0] = '\0';
		hash[0] = '\0';
	}

	if (ID != NULL) {
		*ID = KSI_getHashAlgorithmByName(alg);
	}

	if (isColon != NULL) {
		*isColon = colon == NULL ? 0 : 1;
	}

	res = KT_OK;

cleanup:

	return res;
}

static int is_imprint(const char *str) {
	int res;
	KSI_HashAlgorithm alg = KSI_HASHALG_INVALID;
	int isColon = 0;
	char hex[1024];
	double correctness = 0;
	double tmp = 0;


	res = imprint_extract_fields(str, &alg, &isColon, hex, sizeof(hex));
	if (res != KT_OK) return 0;

	if (isColon) correctness += 0.2;
	if (alg != KSI_HASHALG_INVALID) correctness += 0.4;

	analyze_hexstring_format(hex, &tmp);
	correctness += 0.4 * tmp;

	if (correctness > 0.5) return 1;
	else return 0;
}

static int doFileExists(const char* path){
	if(_access_s(path, F_OK) == 0) return 0;
	else return errno;
}

static int x(char c){
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	abort(); // isxdigit lies.
	return -1; // makes compiler happy
}

static int xx(char c1, char c2){
	if(!isxdigit(c1) || !isxdigit(c2))
		return -1;
	return x(c1) * 16 + x(c2);
}

static int hex_string_to_bin(const char *hexin, unsigned char *buf, size_t buf_len, size_t *lenout){
	int res;
	size_t len;
	size_t arraySize;
	unsigned int i, j;
	int tmp;

	if (hexin == NULL || buf == NULL || lenout == NULL || buf_len == 0) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	len = strlen(hexin);
	arraySize = len / 2;

	if (len%2 != 0) {
		res = KT_HASH_LENGTH_IS_NOT_EVEN;
		goto cleanup;
	}

	for (i = 0, j = 0; i < arraySize; i++, j += 2){
		tmp = xx(hexin[j], hexin[j+1]);
		if (tmp == -1) {
			res = KT_INVALID_HEX_CHAR;
			goto cleanup;
		}

		if (i < buf_len) {
			buf[i] = (unsigned char)tmp;
		} else {
			res = KT_INDEX_OVF;
			goto cleanup;
		}
	}

	*lenout = arraySize;
	res = KT_OK;

cleanup:

	return res;
}

static int imprint_get_hash_obj(const char *imprint, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hash){
	int res;
	char hash_hex[1024];
	unsigned char bin[1024];
	size_t bin_len = 0;
	KSI_HashAlgorithm alg_id = KSI_HASHALG_INVALID;
	KSI_DataHash *tmp = NULL;

	if (imprint == NULL || ksi == NULL || hash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = imprint_extract_fields(imprint, &alg_id, NULL, hash_hex, sizeof(hash_hex));
	if (res != KT_OK) goto cleanup;

	res = hex_string_to_bin(hash_hex, bin, sizeof(bin), &bin_len);
	if (res != KT_OK) goto cleanup;

	if (alg_id == KSI_HASHALG_INVALID) {
		res = KT_UNKNOWN_HASH_ALG;
		goto cleanup;
	}

	res = KSI_DataHash_fromDigest(ksi, alg_id, bin, bin_len, &tmp);
	if (res != KSI_OK) goto cleanup;

	*hash = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	KSI_DataHash_free(tmp);

	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to get hash from command-line");
		ERR_TRCKR_ADD(err, res, "Error: %s", errToString(res));
	}

	return res;
}

static int file_get_hash(ERR_TRCKR *err, KSI_CTX *ctx, const char *fname_in, const char *fname_out, KSI_HashAlgorithm *algo, KSI_DataHash **hash){
	int res;
	KSI_DataHasher *hasher = NULL;
	SMART_FILE *in = NULL;
	SMART_FILE *out = NULL;
	char buf[1024];
	size_t read_count = 0;
	size_t write_count = 0;
	KSI_DataHash *tmp = NULL;

	if(err == NULL || fname_in == NULL || hash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (algo == NULL) {
		ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Unable to hash data file as hash algorithm is not specified (null).");
		goto cleanup;
	} else if (!KSI_isHashAlgorithmSupported(*algo)) {
		ERR_TRCKR_ADD(err, res = KT_UNKNOWN_HASH_ALG, "Error: Unable to hash data file as hash algorithm is not supported.");
		goto cleanup;
	}


	res = KSI_DataHasher_open(ctx, *algo, &hasher);
	if (res != KSI_OK) goto cleanup;

	/**
	 * Open the file, read and hash.
     */
	res = SMART_FILE_open(fname_in, "rb", &in);
	if (res != KT_OK) goto cleanup;

	if (fname_out != NULL) {
		res = SMART_FILE_open(fname_out, "wb", &out);
		if (res != KT_OK) goto cleanup;
	}


	while (!SMART_FILE_isEof(in)) {
		read_count = 0;
		write_count = 0;

		res = SMART_FILE_read(in, buf, sizeof(buf), &read_count);
		if (res != SMART_FILE_OK) {
			ERR_TRCKR_ADD(err, res, "Error: Unable to read data from file.");
			goto cleanup;
		}

		res = KSI_DataHasher_add(hasher, buf, read_count);
		ERR_CATCH_MSG(err, res, "Error: Unable to add data to hasher.");

		if (!SMART_FILE_isEof(in)) {
			if (out != NULL) {
				res = SMART_FILE_write(out, buf, read_count, &write_count);
				if (res != SMART_FILE_OK || write_count != read_count) {
					ERR_TRCKR_ADD(err, res, "Error: Unable to write to file.");
					goto cleanup;
				}
			}
		}
	}

	res = KSI_DataHasher_close(hasher, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable close hasher.");

	*hash = tmp;
	tmp = NULL;

	res = KT_OK;

cleanup:

	SMART_FILE_close(in);
	SMART_FILE_close(out);
	KSI_DataHasher_free(hasher);
	KSI_DataHash_free(tmp);

	return res;
}

static int date_is_valid(struct tm *time_st) {
    int days = 31;
    int dd = time_st->tm_mday;
    int mm = time_st->tm_mon + 1;
    int yy = time_st->tm_year + 1900;

	if (mm < 1 || mm > 12 || dd < 1 || yy < 1900) {
        return 0;
    }

    if (mm == 2) {
        days = 28;
		/* Its a leap year */
        if (yy % 400 == 0 || (yy % 4 == 0 && yy % 100 != 0)) {
            days = 29;
        }
    } else if (mm == 4 || mm == 6 || mm == 9 || mm == 11) {
        days = 30;
    }

    if (dd > days) {
        return 0;
    }
    return 1;
}

static int string_to_tm(const char *time, struct tm *time_st) {
	const char *ret = NULL;
	const char *next = NULL;
	/* If its not possible to convert date string with such a buffer, there is something wrong! */
	char buf[1024];

	if (time == NULL || time_st == NULL) return FORMAT_NULLPTR;

	memset(time_st, 0, sizeof(struct tm));


	next = time;
	ret = STRING_extractAbstract(next, NULL, "-", buf, sizeof(buf), NULL, find_charBeforeStrn, &next);
	if (ret != buf || next == NULL || *buf == '\0' || strlen(buf) > 4 || isFormatOk_int(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_year = atoi(buf) - 1900;

	ret = STRING_extractAbstract(next, "-", "-", buf, sizeof(buf), find_charAfterStrn, find_charBeforeLastStrn, &next);
	if (ret != buf || next == NULL || strlen(buf) > 2 || isFormatOk_int(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_mon = atoi(buf) - 1;

	ret = STRING_extractAbstract(next, "-", " ", buf, sizeof(buf), find_charAfterStrn, find_charBeforeLastStrn, &next);
	if (ret != buf || next == NULL || strlen(buf) > 2 || isFormatOk_int(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_mday = atoi(buf);

	if (date_is_valid(time_st) == 0) return FORMAT_INVALID_UTC_OUT_OF_RANGE;

	ret = STRING_extractAbstract(next, " ", ":", buf, sizeof(buf), find_charAfterStrn, find_charBeforeStrn, &next);
	if (ret != buf || next == NULL || *buf == '\0' || strlen(buf) > 2 || isFormatOk_int(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_hour = atoi(buf);
	if (time_st->tm_hour < 0 || time_st->tm_hour > 23) return FORMAT_INVALID_UTC_OUT_OF_RANGE;

	ret = STRING_extractAbstract(next, ":", ":", buf, sizeof(buf), find_charAfterStrn, find_charBeforeLastStrn, &next);
	if (ret != buf || next == NULL || strlen(buf) > 2 || isFormatOk_int(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_min = atoi(buf);
	if (time_st->tm_min < 0 || time_st->tm_min > 59) return FORMAT_INVALID_UTC_OUT_OF_RANGE;

	ret = STRING_extractAbstract(next, ":", NULL, buf, sizeof(buf), find_charAfterStrn, find_charBeforeLastStrn, &next);
	if (ret != buf || strlen(buf) > 2 || isFormatOk_int(buf) != FORMAT_OK) return FORMAT_INVALID_UTC;
	time_st->tm_sec = atoi(buf);
	if (time_st->tm_sec < 0 || time_st->tm_sec > 59) return FORMAT_INVALID_UTC_OUT_OF_RANGE;

	return FORMAT_OK;
}

static int convert_UTC_to_UNIX2(const char* arg, time_t *time) {
	int res;
	struct tm time_st;
	time_t t;

	if(arg == NULL || time == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (string_to_tm(arg, &time_st) != FORMAT_OK) {
		res = KT_INVALID_INPUT_FORMAT;
		goto cleanup;
	}

	t = KSI_CalendarTimeToUnixTime(&time_st);
	if (t == -1) {
		res = KT_INVALID_INPUT_FORMAT;
		goto cleanup;
	}

	*time = t;
	res = KT_OK;

cleanup:

	return res;
}

int isInteger(const char *str) {
	int i = 0;
	int C;
	if (str == NULL) return 0;
	if (str[0] == '\0') return 0;

	while (C = 0xff & str[i++]) {
		if (!isdigit(C)) return 0;
	}
	return 1;
}

int isFormatOk_url(const char *url) {
	if(url == NULL) return FORMAT_NULLPTR;
	if(strlen(url) == 0) return FORMAT_NOCONTENT;

	if(strstr(url, "ksi://") == url)
		return FORMAT_OK;
	else if(strstr(url, "http://") == url || strstr(url, "ksi+http://") == url)
		return FORMAT_OK;
	else if(strstr(url, "https://") == url || strstr(url, "ksi+https://") == url)
		return FORMAT_OK;
	else if(strstr(url, "ksi+tcp://") == url)
		return FORMAT_OK;
	else if(strstr(url, "file://") == url)
		return FORMAT_OK;
	else
		return FORMAT_URL_UNKNOWN_SCHEME;
}

int convertRepair_url(const char* arg, char* buf, unsigned len) {
	char *scheme = NULL;
	unsigned i = 0;
	int isFile;

	if(arg == NULL || buf == NULL) return 0;
	scheme = strstr(arg, "://");
	isFile = strstr(arg, "file://") == arg ? 1 : 0;

	if (scheme == NULL) {
		strncpy(buf, "http://", len-1);
		if (strlen(buf)+strlen(arg) < len)
			strcat(buf, arg);
		else
			return 0;
	} else {
		while (arg[i] && i < len - 1) {
			if (&arg[i] < scheme) {
				buf[i] = (char)tolower(arg[i]);
			} else {
#ifdef _WIN32
				buf[i] = arg[i] == '\\' ? '/' : arg[i];
#else
				buf[i] = arg[i];
#endif
			}

			i++;
		}
		buf[i] = 0;
	}
	return 1;
}


int isFormatOk_int(const char *integer) {
	int i = 0;
	int C;
	if(integer == NULL) return FORMAT_NULLPTR;
	if(strlen(integer) == 0) return FORMAT_NOCONTENT;

	while ((C = integer[i++]) != '\0') {
		if (isdigit(C) == 0) {
			return FORMAT_NOT_INTEGER;
		}
	}
	return FORMAT_OK;
}

int isContentOk_int(const char* integer) {
	unsigned int i;
	size_t len;
	size_t int_max_len;
	char tmp[32];

	sprintf(tmp, "%d", INT_MAX);
	len  = strlen(integer);
	int_max_len = strlen(tmp);

	if (len > int_max_len) {
		return INTEGER_TOO_LARGE;
	} else if (len == int_max_len) {

		for (i = 0; i < int_max_len; i++) {
			if (tmp[i] < integer[i])
				return INTEGER_TOO_LARGE;
			else if (tmp[i] > integer[i])
				break;
		}
	}
	return PARAM_OK;
}

int extract_int(void *extra, const char* str,  void** obj){
	int *pI = (int*)obj;
	extra;
	*pI = atoi(str);
	return PST_OK;
}


int isFormatOk_inputFile(const char *path){
	if(path == NULL) return FORMAT_NULLPTR;
	if(strlen(path) == 0) return FORMAT_NOCONTENT;
	return FORMAT_OK;
}

int isContentOk_inputFile(const char* path){
	int fileStatus = EINVAL;
	if (strcmp(path, "-") == 0) {
		return PARAM_OK;
	}else if (isFormatOk_inputFile(path) == FORMAT_OK)
		fileStatus = doFileExists(path);
	else
		return FILE_INVALID_PATH;

	switch (fileStatus) {
	case 0:
		return PARAM_OK;
		break;
	case EACCES:
		return FILE_ACCESS_DENIED;
		break;
	case ENOENT:
		return FILE_DOES_NOT_EXIST;
		break;
	case EINVAL:
		return FILE_INVALID_PATH;
		break;
	}

	return PARAM_UNKNOWN_ERROR;
}

int convertRepair_path(const char* arg, char* buf, unsigned len){
	char *toBeReplaced = NULL;

	if(arg == NULL || buf == NULL) return 0;
	strncpy(buf, arg, len-1);


	toBeReplaced = buf;
	while((toBeReplaced = strchr(toBeReplaced, '\\')) != NULL){
		*toBeReplaced = '/';
		toBeReplaced++;
	}

	return 1;
}

int isFormatOk_path(const char *path) {
	if(path == NULL) return FORMAT_NULLPTR;
	return FORMAT_OK;
}

int extract_inputFile(void *extra, const char* str, void** obj) {
	int res;
	SMART_FILE *tmp = NULL;

	extra;
	if (str == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/*TODO: Check if mode must be changed. */
	res = SMART_FILE_open(str, "rb", &tmp);
	if (res != KT_OK) goto cleanup;

	*obj = (void*)tmp;

cleanup:

	SMART_FILE_close(tmp);

	return res;
}


int isFormatOk_hashAlg(const char *hashAlg){
	if(hashAlg == NULL) return FORMAT_NULLPTR;
	if(strlen(hashAlg) == 0) return FORMAT_NOCONTENT;
	return FORMAT_OK;
}

int isContentOk_hashAlg(const char *alg){
	if(KSI_getHashAlgorithmByName(alg) != KSI_HASHALG_INVALID) return PARAM_OK;
	else return HASH_ALG_INVALID_NAME;
}

int extract_hashAlg(void *extra, const char* str, void** obj) {
	const char *hash_alg_name = NULL;
	KSI_HashAlgorithm *hash_id = (KSI_HashAlgorithm*)obj;

	extra;
	hash_alg_name = str != NULL ? (str) : ("default");
	*hash_id = KSI_getHashAlgorithmByName(hash_alg_name);

	if (*hash_id == KSI_HASHALG_INVALID) return KT_UNKNOWN_HASH_ALG;

	return PST_OK;
}


int isFormatOk_imprint(const char *imprint){
	char *colon;

	if(imprint == NULL) return FORMAT_NULLPTR;
	if(strlen(imprint) == 0) return FORMAT_NOCONTENT;

	colon = strchr(imprint, ':');

	if(colon == NULL) return FORMAT_IMPRINT_NO_COLON;
	if(colon != NULL && colon == imprint) return FORMAT_IMPRINT_NO_HASH_ALG;
	if(*(colon + 1) == '\0') return FORMAT_IMPRINT_NO_HASH;

	return analyze_hexstring_format(colon + 1, NULL);
}

int isContentOk_imprint(const char *imprint) {
	int res = PARAM_INVALID;
	char hash[1024];
	KSI_HashAlgorithm alg_id = KSI_HASHALG_INVALID;
	unsigned len = 0;
	unsigned expected_len = 0;

	if(imprint == NULL) return PARAM_INVALID;

	res = imprint_extract_fields(imprint, &alg_id, NULL, hash, sizeof(hash));
	if (res != KT_OK) {
		res = PARAM_INVALID;
		goto cleanup;
	}

	if (alg_id == KSI_HASHALG_INVALID) {
		res = HASH_ALG_INVALID_NAME;
		goto cleanup;
	}

	expected_len = KSI_getHashLength(alg_id);
	len = (unsigned)strlen(hash);

	if (len%2 != 0 && len/2 != expected_len) {
		res = HASH_IMPRINT_INVALID_LEN;
		goto cleanup;
	}

	res = PARAM_OK;
cleanup:

	return res;
}

int extract_imprint(void *extra, const char* str, void** obj) {
	int res;
	void **extra_array = (void**)extra;
	COMPOSITE *comp = NULL;
	PARAM_SET *set = NULL;
	KSI_CTX *ctx = NULL;
	ERR_TRCKR *err = NULL;
	KSI_DataHash *tmp = NULL;

	set = (PARAM_SET*)extra_array[0];
	comp = (COMPOSITE*)extra_array[1];
	ctx = comp->ctx;
	err = comp->err;


	res = imprint_get_hash_obj(str, ctx, err, &tmp);
	if (res != KT_OK) goto cleanup;

	*obj = (void*)tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	KSI_DataHash_free(tmp);

	return res;
}


int isFormatOk_inputHash(const char *str) {
	if (str == NULL) return FORMAT_NULLPTR;
	if(strlen(str) == 0) return FORMAT_NOCONTENT;

	if (is_imprint(str)) {
		return isFormatOk_imprint(str);
	} else {
		return isFormatOk_inputFile(str);
	}
}

int isContentOk_inputHash(const char *str) {
	if (str == NULL) FORMAT_NULLPTR;
	if(strlen(str) == 0) return FORMAT_NOCONTENT;

	if (is_imprint(str)) {
		return isContentOk_imprint(str);
	} else {
		return isContentOk_inputFile(str);
	}
}

int extract_inputHash(void *extra, const char* str, void** obj) {
	int res;
	void **extra_array = extra;
	COMPOSITE *comp = (COMPOSITE*)(extra_array[1]);
	KSI_CTX *ctx = comp->ctx;
	ERR_TRCKR *err = comp->err;
	KSI_HashAlgorithm *algo = comp->h_alg;
	char *fname_out = comp->fname_out;
	KSI_DataHash *tmp = NULL;

	if (obj == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	if (is_imprint(str)) {
		res = extract_imprint(extra, str, (void**)&tmp);
		if (res != KT_OK) goto cleanup;
	} else {
		res = file_get_hash(err, ctx, str, fname_out, algo, &tmp);
		if (res != KT_OK) goto cleanup;
	}


	*obj = (void*)tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	KSI_DataHash_free(tmp);

	return res;
}

int extract_inputSignature(void *extra, const char* str, void** obj) {
	int res;
	void **extra_array = extra;
	COMPOSITE *comp = (COMPOSITE*)(extra_array[1]);
	KSI_CTX *ctx = comp->ctx;
	ERR_TRCKR *err = comp->err;

	if (obj == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = KSI_OBJ_loadSignature(err, ctx, str, (KSI_Signature**)obj);
	if (res != KT_OK) goto cleanup;

cleanup:

	return res;
}


int isFormatOk_pubString(const char *str) {
	int C;
	int i = 0;
	const char base32EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567-";

	if (str == NULL) return FORMAT_NULLPTR;
	if (str[0] == '\0') return FORMAT_NOCONTENT;

	while (C = 0xff & str[i++]) {
		if (strchr(base32EncodeTable, C) == NULL) {
			return FORMAT_INVALID_BASE32_CHAR;
		}
	}
	return FORMAT_OK;
}

int extract_pubString(void *extra, const char* str, void** obj) {
	int res;
	void **extra_array = extra;
	COMPOSITE *comp = (COMPOSITE*)(extra_array[1]);
	KSI_CTX *ctx = comp->ctx;
	ERR_TRCKR *err = comp->err;
	KSI_PublicationData *tmp = NULL;

	if (obj == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = KSI_PublicationData_fromBase32(ctx, str, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable parse publication string.");

	*obj = (void*)tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	KSI_PublicationData_free(tmp);

	return res;
}


int isFormatOk_timeString(const char *time) {
	struct tm time_st;
	return string_to_tm(time, &time_st);
}

int isFormatOk_utcTime(const char *time) {
	if (isInteger(time)) {
		return isFormatOk_int(time);
	} else {
		return isFormatOk_timeString(time);
	}
}

int isContentOk_utcTime(const char *time) {
	if (isInteger(time)) {
		return isContentOk_int(time);
	} else {
		return PARAM_OK;
	}
}

int extract_utcTime(void *extra, const char* str, void** obj) {
	int res;
	void **extra_array = extra;
	COMPOSITE *comp = (COMPOSITE*)(extra_array[1]);
	KSI_CTX *ctx = NULL;
	ERR_TRCKR *err = NULL;
	KSI_Integer *tmp = NULL;
	int time = 0;

	if (obj == NULL || comp == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/* TODO: make all extractors to not fail if comp is NULL */
	ctx = comp->ctx;
	err = comp->err;

	/**
	 * If input is integer, extract its value. If input is time string convert it
	 * to time.
     */
	if (isInteger(str)) {
		int t = 0;
		res = extract_int(extra, str,  (void**)&t);
		if (res != KT_OK) goto cleanup;
		time = t;
	} else {
		time_t t;
		res = convert_UTC_to_UNIX2(str, &t);
		if (res != KT_OK) goto cleanup;
		time = (int)t;
	}


	/**
	 * Create KSI_Integer for output parameter.
     */

	res = KSI_Integer_new(ctx, time, &tmp);
	ERR_CATCH_MSG(err, res, "Error: %s.", errToString(res));

	*obj = (void*)tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	KSI_Integer_free(tmp);

	return res;
}


int isFormatOk_flag(const char *flag) {
	if(flag == NULL) return FORMAT_OK;
	else return FORMAT_FLAG_HAS_ARGUMENT;
}

int isFormatOk_userPass(const char *uss_pass) {
	if(uss_pass == NULL) return FORMAT_NULLPTR;
	if(strlen(uss_pass) == 0) return FORMAT_NOCONTENT;
	return FORMAT_OK;
}


int isFormatOk_constraint(const char *constraint) {
	char *at = NULL;
	unsigned i = 0;

	if(constraint == NULL) return FORMAT_NULLPTR;
	if(strlen(constraint) == 0) return FORMAT_NOCONTENT;

	if((at = strchr(constraint,'=')) == NULL) return FORMAT_INVALID;
	if(at == constraint || *(at+1)==0) return FORMAT_INVALID;

	while (constraint[i] != 0 &&  constraint[i] != '=') {
		if (!isdigit(constraint[i]) && constraint[i] != '.')
			return FORMAT_INVALID_OID;
		i++;
	}

	return FORMAT_OK;
}

int convertRepair_constraint(const char* arg, char* buf, unsigned len) {
	char *value = NULL;
	const char *oid = NULL;

	if(arg == NULL || buf == NULL) return 0;
	strncpy(buf, arg, len-1);

	value = strchr(arg, '=');
	if (value == NULL) return 0;
	else value++;

	oid = OID_getFromString(arg);

	if (oid != NULL && value != NULL)
		KSI_snprintf(buf, len, "%s=%s", oid, value);

	return 1;
}



const char *getParameterErrorString(int res) {
	switch (res) {
		case PARAM_OK: return "OK";
		case FORMAT_NULLPTR: return "Format error: Parameter must have value";
		case FORMAT_NOCONTENT: return "Parameter has no content";
		case FORMAT_INVALID: return "Parameter is invalid";
		case FORMAT_INVALID_OID: return "OID is invalid";
		case FORMAT_URL_UNKNOWN_SCHEME: return "URL scheme is unknown";
		case FORMAT_FLAG_HAS_ARGUMENT: return "Parameter must not have arguments";
		case FORMAT_INVALID_UTC: return "Time not formatted as YYYY-MM-DD hh:mm:ss";
		case FORMAT_INVALID_UTC_OUT_OF_RANGE: return "Time out of range";
		case PARAM_INVALID: return "Parameter is invalid";
		case FORMAT_NOT_INTEGER: return "Invalid integer";
		case HASH_ALG_INVALID_NAME: return "Algorithm name is incorrect";
		case HASH_IMPRINT_INVALID_LEN: return "Hash length is incorrect";
		case FORMAT_INVALID_HEX_CHAR: return "Invalid hex character";
		case FORMAT_INVALID_BASE32_CHAR: return "Invalid base32 character";
		case FORMAT_IMPRINT_NO_COLON: return "Imprint format must be <alg>:<hash>. ':' missing";
		case FORMAT_IMPRINT_NO_HASH_ALG: return "Imprint format must be <alg>:<hash>. <alg> missing";
		case FORMAT_IMPRINT_NO_HASH: return "Imprint format must be <alg>:<hash>. <hash> missing";
		case FILE_ACCESS_DENIED: return "File access denied";
		case FILE_DOES_NOT_EXIST: return "File does not exist";
		case FILE_INVALID_PATH: return "Invalid path";
		case INTEGER_TOO_LARGE: return "Integer value is too large";
		default: return "Unknown error";
	}
}
