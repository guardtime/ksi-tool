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
#include <math.h>
#include <ctype.h>
#include <limits.h>

#include "param_control.h"
#include "gt_task_support.h"
#include "obj_printer.h"
#include "param_set/param_value.h"
#include "../param_set/param_set.h"
#include "../param_set/task_def.h"
#include "../gt_cmd_control.h"
#include "ksi_init.h"
#include "../api_wrapper.h"
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

static int load_ksi_obj(ERR_TRCKR *err, KSI_CTX *ksi, const char *path, void **obj,
					int (*parse)(KSI_CTX *ksi, unsigned char *raw, unsigned raw_len, void **obj),
					void (*obj_free)()){
	int res;
	SMART_FILE *file = NULL;
	unsigned char *buf = NULL;
	size_t buf_size = 0xffff;
	size_t buf_len = 0;
	void *tmp = NULL;
	size_t count = 0;


	if (ksi == NULL || path == NULL || obj == NULL || parse == NULL || obj_free == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = SMART_FILE_open(path, "rb", &file);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
		goto cleanup;
	}

	buf = (unsigned char*)malloc(buf_size);
	if (buf == NULL) {
		res = KT_OUT_OF_MEMORY;
		ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
		goto cleanup;
	}


	while (!SMART_FILE_isEof(file)) {
		if (buf_len + 1 >= buf_size) {
			buf_size += 0xffff;
			buf = realloc(buf, buf_size);
			if (buf == NULL) {
				res = KT_OUT_OF_MEMORY;
				ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
				goto cleanup;
			}
		}

		res = SMART_FILE_read(file, buf + buf_len, buf_size - buf_len, &count);

		if(res != KT_OK || SMART_FILE_isError(file)) {
			ERR_TRCKR_ADD(err, res = KT_IO_ERROR, "Error: Unable to read data from file.");
			goto cleanup;
		}

		buf_len += count;
		count = 0;
	}

	if (buf_len > UINT_MAX) {
		res = KT_INDEX_OVF;
		ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
		goto cleanup;
	}

	res = parse(ksi, buf, (unsigned)buf_len, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable to parse.");

	*obj = tmp;
	tmp = NULL;
	res = KT_OK;


cleanup:

	SMART_FILE_close(file);
	free(buf);
	obj_free(tmp);

	return res;
}

static int KSI_Signature_serialize_wrapper(KSI_CTX *ksi, KSI_Signature *sig, unsigned char **raw, size_t *raw_len) {
	return KSI_Signature_serialize(sig, raw, raw_len);
}

static int load_ksi_obj_signature(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_Signature **sig) {
	int res;

	if (ksi == NULL || fname == NULL || sig == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = load_ksi_obj(err, ksi, fname,
				(void**)sig,
				(int (*)(KSI_CTX *, unsigned char*, unsigned, void**))KSI_Signature_parse,
				(void (*)(void *))KSI_Signature_free);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to load signature file from '%s'.", fname);
	}
	return res;
}

static int file_get_hash(PARAM_SET *set, ERR_TRCKR *err, KSI_CTX *ctx, const char *fnamein, KSI_DataHash **hash){
	int res;
	KSI_HashAlgorithm id = KSI_HASHALG_INVALID;
	KSI_DataHasher *hasher = NULL;
	SMART_FILE *file = NULL;
	unsigned char buf[1024];
	size_t read_count = 0;
	KSI_DataHash *tmp = NULL;

	if(err == NULL || fnamein == NULL || hash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Consulte the parameter set and check if special hash algorithm is needed.
	 * Open Hasher.
     */
	res = PARAM_SET_getObj(set, "H", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, (void**)&id);
	if (res != PST_OK && res != PST_PARAMETER_EMPTY) goto cleanup;

	id = (id == KSI_HASHALG_INVALID) ? KSI_getHashAlgorithmByName("default") : id;

	res = KSI_DataHasher_open(ctx, id, &hasher);
	if (res != KSI_OK) goto cleanup;


	/**
	 * Open the file , read and hash.
     */
	res = SMART_FILE_open(fnamein, "rb", &file);
	if (res != KT_OK) goto cleanup;


	while (!SMART_FILE_isEof(file)) {
		read_count = 0;
		res = SMART_FILE_read(file, buf, sizeof(buf), &read_count);
		if (res != KT_OK) goto cleanup;

		if(SMART_FILE_isError(file)) {
			ERR_TRCKR_ADD(err, res = KT_IO_ERROR, "Error: Unable to read data from file.");
			goto cleanup;
		}

		res = KSI_DataHasher_add(hasher, buf, read_count);
		ERR_CATCH_MSG(err, res, "Error: Unable to add data to hasher.");
	}

	res = KSI_DataHasher_close(hasher, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable close hasher.");

	*hash = tmp;
	tmp = NULL;

	res = KT_OK;

cleanup:

	SMART_FILE_close(file);
	KSI_DataHasher_free(hasher);
	KSI_DataHash_free(tmp);

	return res;
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
	else
		return FORMAT_URL_UNKNOWN_SCHEME;
}

int convertRepair_url(const char* arg, char* buf, unsigned len) {
	char *scheme = NULL;
	unsigned i = 0;
	if(arg == NULL || buf == NULL) return false;
	scheme = strstr(arg, "://");

	if(scheme == NULL){
		strncpy(buf, "http://", len-1);
		if(strlen(buf)+strlen(arg) < len)
			strcat(buf, arg);
		else
			return false;
	}
	else{
		while(arg[i] && i < len - 1){
			if(&arg[i] < scheme){
				buf[i] = (char)tolower(arg[i]);
			}
			else
				buf[i] = arg[i];
			i++;
		}
		buf[i] = 0;
	}
	return true;
}

int isFormatOk_int(const char *integer) {
	int i = 0;
	int C;
	if(integer == NULL) return FORMAT_NULLPTR;
	if(strlen(integer) == 0) return FORMAT_NOCONTENT;

	while ((C = integer[i++]) != '\0') {
		if (isdigit(C) == 0) {
			return FORMAT_INVALID;
		}
	}
	return FORMAT_OK;
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

int convert_repair_path(const char* arg, char* buf, unsigned len){
	char *toBeReplaced = NULL;

	if(arg == NULL || buf == NULL) return false;
	strncpy(buf, arg, len-1);


	toBeReplaced = buf;
	while((toBeReplaced = strchr(toBeReplaced, '\\')) != NULL){
		*toBeReplaced = '/';
		toBeReplaced++;
	}

	return true;
}

int extract_inputFile(void *extra, const char* str, void** obj) {
	int res;
	SMART_FILE *tmp = NULL;

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
	KSI_HashAlgorithm tmp = KSI_HASHALG_INVALID;

	hash_alg_name = str == NULL ? (str) : ("default");
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
	PARAM_SET *set = (PARAM_SET*)(extra_array[0]);
	KSI_CTX *ctx = comp->ctx;
	ERR_TRCKR *err = comp->err;
	KSI_DataHash *tmp = NULL;

	if (obj == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	if (is_imprint(str)) {
		res = extract_imprint(extra, str, (void**)&tmp);
		if (res != KT_OK) goto cleanup;
	} else {
		res = file_get_hash(set, err, ctx, str, &tmp);
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
	PARAM_SET *set = (PARAM_SET*)(extra_array[0]);
	KSI_CTX *ctx = comp->ctx;
	ERR_TRCKR *err = comp->err;

	if (obj == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = load_ksi_obj_signature(err, ctx, str, (KSI_Signature**)obj);
	if (res != KT_OK) goto cleanup;

cleanup:

	return res;
}

int isFormatOk_pubString(const char *str) {
	int C;
	int i = 0;
	const char base32EncodeTable[33] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567-";

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
	PARAM_SET *set = (PARAM_SET*)(extra_array[0]);
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