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

#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ksi/ksi.h>
#include <ksi/net.h>
#include <ksi/hashchain.h>
#include <ksi/pkitruststore.h>
#include <stdio.h>
#include "gt_task_support.h"
#include "param_set.h"
#include <ctype.h>
#include "ksitool_err.h"

#ifdef _WIN32
#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#include <stdlib.h>
#else
#       include <limits.h>
#	include <sys/time.h>
#endif


/**
 * File object for logging
 */
static FILE *logFile = NULL;

static int getStreamFromPath(const char *fname, const char *mode, FILE **stream, bool *close) {
	int res;
	FILE *in = NULL;
	FILE *tmp = NULL;
	bool doClose = false;

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

		doClose = true;
		tmp = in;
	}

	*stream = tmp;
	in = NULL;
	*close = doClose;

	res = KT_OK;

cleanup:

	if (in) fclose(in);
	return res;
}

static int ksitool_initLogger(Task *task, KSI_CTX *ksi, ERR_TRCKR *err) {
	int res;
	paramSet *set = NULL;
	FILE *writeLogTo = NULL;
	bool log;
	bool closeLogStream = false;
	char *outLogfile = NULL;

	if (task == NULL || ksi == NULL || err == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	set = Task_getSet(task);
	log = paramSet_getStrValueByNameAt(set, "log",0, &outLogfile);

	/*Set logging*/
	if (log){
		res = getStreamFromPath(outLogfile, "w", &writeLogTo, &closeLogStream);
		if (res != KT_OK) {
			ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
			goto cleanup;
		}

		if (closeLogStream == true) logFile = writeLogTo;
		res = KSI_CTX_setLoggerCallback(ksi, KSI_LOG_StreamLogger, writeLogTo);
		ERR_CATCH_MSG(err, res, "Error: Unable to set logger callback function.");
		res = KSI_CTX_setLogLevel(ksi, KSI_LOG_DEBUG);
		ERR_CATCH_MSG(err, res, "Error: Unable to set logger log level.");
	}

	res = KT_OK;

cleanup:

	return res;
}

static int ksitool_initNetworkProvider(Task *task, KSI_CTX *ksi, ERR_TRCKR *err){
	int res;
	paramSet *set = NULL;
	bool P, C, c, s, v, x, p, T, aggre;
	char *signingService_url = NULL;
	char *publicationsFile_url = NULL;
	char *verificationService_url = NULL;
	int networkConnectionTimeout = 0;
	int networkTransferTimeout = 0;
	char *user = NULL;
	char *pass = NULL;


	if (task == NULL || ksi == NULL || err == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	set = Task_getSet(task);
	paramSet_getHighestPriorityStrValueByName(set, "S", &signingService_url);
	paramSet_getHighestPriorityStrValueByName(set, "X", &verificationService_url);
	P = paramSet_getStrValueByNameAt(set, "P",0,&publicationsFile_url);

	C = paramSet_getIntValueByNameAt(set, "C", 0,&networkConnectionTimeout);
	c = paramSet_getIntValueByNameAt(set, "c", 0,&networkTransferTimeout);
	aggre = paramSet_isSetByName(set, "aggre");
	s = paramSet_isSetByName(set, "s");
	v = paramSet_isSetByName(set, "v");
	x = paramSet_isSetByName(set, "x");
	p = paramSet_isSetByName(set, "p");
	T = paramSet_isSetByName(set, "T");


	paramSet_getHighestPriorityStrValueByName(set, "user", &user);
	paramSet_getHighestPriorityStrValueByName(set, "pass", &pass);

	if (user == NULL) user = "anon";
	if (pass == NULL) pass = "anon";

	if (x || v || (p && T)) {
		if (verificationService_url == NULL){
			if ((x && v) || (p && T)) {
				ERR_TRCKR_ADD(err, res = KT_INVALID_CMD_PARAM, "Error: Extender url required.");
				goto cleanup;
			} else if (v) {
				print_warnings("Warning: verification may require extender url.\n");
			}
		}else {
			res = KSI_CTX_setExtender(ksi, verificationService_url, user, pass);
			ERR_CATCH_MSG(err, res, "Error: Unable set extender.");
		}
	}else if (s || aggre) {
		if (signingService_url == NULL) {
			ERR_TRCKR_ADD(err, res  = KT_INVALID_CMD_PARAM, "Error: Aggregator url required.");
			goto cleanup;
		}

		res = KSI_CTX_setAggregator(ksi, signingService_url, user, pass);
		ERR_CATCH_MSG(err, res, "Error: Unable set aggregator.");
	}

	if (P){
		res = KSI_CTX_setPublicationUrl(ksi, publicationsFile_url);
		ERR_CATCH_MSG(err, res, "Error: Unable set publication URL.");
	}

	if (C) {
		res = KSI_CTX_setConnectionTimeoutSeconds(ksi, networkConnectionTimeout);
		ERR_CATCH_MSG(err, res, "Error: Unable set connection timeout.");
	}

	if (c) {
		res = KSI_CTX_setTransferTimeoutSeconds(ksi, networkTransferTimeout);
		ERR_CATCH_MSG(err, res, "Error: Unable set transfer timeout.");
	}

	res = KT_OK;

cleanup:

	return res;
}

static int ksitool_addConstraints(Task *task, KSI_CTX *ksi, ERR_TRCKR *err) {
	int res;
	paramSet *set = NULL;
	unsigned i = 0;
	bool cnstr;
	char *constraint = NULL;
	unsigned constraint_count = 0;
	KSI_CertConstraint *constraintArray = NULL;

	/*Get parameter values*/
	set = Task_getSet(task);
	cnstr = paramSet_isSetByName(set,"cnstr");


	if (cnstr) {
		if (paramSet_getValueCountByName(set,	"cnstr", &constraint_count) == false) {
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, NULL);
			goto cleanup;
		}

		constraintArray = KSI_malloc(sizeof(KSI_CertConstraint) * (1 + constraint_count));
		if (constraintArray == NULL) {
			ERR_TRCKR_ADD(err, res = KT_OUT_OF_MEMORY, NULL);
			goto cleanup;
		}

		for (i = 0; i < constraint_count + 1; i++) {
			constraintArray[i].oid = NULL;
			constraintArray[i].val = NULL;
		}

		for (i = 0; i < constraint_count; i++) {
			char *oid = NULL;
			char *value = NULL;
			char tmp[1024];

				paramSet_getStrValueByNameAt(set, "cnstr", i, &constraint);
				strncpy(tmp, constraint, sizeof(tmp));

				oid = tmp;
				value = strchr(tmp, '=');
				if (value == NULL) {
					ERR_TRCKR_ADD(err, res = KT_INVALID_CMD_PARAM, "Error: Unable to parse constraint.");
					goto cleanup;
				}

				*value = '\0';
				value++;

			constraintArray[i].oid = KSI_malloc(strlen(oid) + 1);
			constraintArray[i].val = KSI_malloc(strlen(value) + 1);
			if (constraintArray[i].oid == NULL || constraintArray[i].val == NULL) {
				ERR_TRCKR_ADD(err, res = KSI_OUT_OF_MEMORY, NULL);
				goto cleanup;
			}

			strcpy(constraintArray[i].oid, oid);
			strcpy(constraintArray[i].val, value);
		}

		res = KSI_CTX_setDefaultPubFileCertConstraints(ksi, constraintArray);
		ERR_CATCH_MSG(err, res, "Error: Unable to add cert constraints.");
	}

cleanup:

	if (constraintArray)  {
		for (i = 0; i < constraint_count; i++) {
			KSI_free(constraintArray[i].oid);
			KSI_free(constraintArray[i].val);
		}

		KSI_free(constraintArray);
	}

	return res;
}

static int ksitool_initTrustStore(Task *task, KSI_CTX *ksi, ERR_TRCKR *err) {
	int res;
	paramSet *set = NULL;
	KSI_PKITruststore *refTrustStore = NULL;
	int i=0;
	bool V, W, cnstr;
	char *lookupFile = NULL;
	char *lookupDir = NULL;

	if (task == NULL || ksi == NULL || err == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	/*Get parameter values*/
	set = Task_getSet(task);
	V = paramSet_isSetByName(set,"V");
	W = paramSet_getStrValueByNameAt(set, "W",0, &lookupDir);
	cnstr = paramSet_isSetByName(set,"cnstr");


	if (cnstr) {
		res = ksitool_addConstraints(task, ksi, err);
		if (res != KT_OK) {
			goto cleanup;
		}
	}

	if(V || W) {
		res = KSI_CTX_getPKITruststore(ksi, &refTrustStore);
		ERR_CATCH_MSG(err, res, "Error: Unable to get PKI trust store.");
		if(V){
			i = 0;
			while(paramSet_getStrValueByNameAt(set, "V", i++, &lookupFile)) {
				res = KSI_PKITruststore_addLookupFile(refTrustStore, lookupFile);
				ERR_CATCH_MSG(err, res, "Error: Unable to add cert to PKI trust store.");
			}
		}
		if(W){
			res = KSI_PKITruststore_addLookupDir(refTrustStore, lookupDir);
			ERR_CATCH_MSG(err, res, "Error: Unable to add lookup dir to PKI trust store.");
		}
	}

	res = KT_OK;

cleanup:


	return res;
}

static int ksitool_initPublicationFile(Task *task, KSI_CTX *ksi, ERR_TRCKR *err) {
	int res = KSI_UNKNOWN_ERROR;
	paramSet *set = NULL;
	KSI_PublicationsFile *tmpPubFile = NULL;
	bool b;
	char *inPubFileName = NULL;

	if (task == NULL || ksi == NULL || err == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	/*Get parameter values*/
	set = Task_getSet(task);
	b = paramSet_getStrValueByNameAt(set, "b",0, &inPubFileName);

	if(b && (Task_getID(task) != downloadPublicationsFile && Task_getID(task) != verifyPublicationsFile)){
		res = loadPublicationFile(err, ksi, inPubFileName, &tmpPubFile);
		if (res != KT_OK) goto cleanup;

		res = KSI_CTX_setPublicationsFile(ksi, tmpPubFile);
		ERR_CATCH_MSG(err, res, "Error: Unable to configure publications file.");
		tmpPubFile = NULL;
	}

	res = KT_OK;

cleanup:

	KSI_PublicationsFile_free(tmpPubFile);

	return res;
}

int initTask(Task *task ,KSI_CTX **ksi, ERR_TRCKR **error) {
	int res;
	ERR_TRCKR *err = NULL;
	KSI_CTX *tmpKsi = NULL;

	if (task == NULL || ksi == NULL || error == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/*Init error tracker*/
	err = ERR_TRCKR_new(print_errors);
	if (err == NULL) {
		res = KT_OUT_OF_MEMORY;
		print_errors("Error: Unable to initialize error tracker.");
		goto cleanup;
	}

	*error = err;

	/*Init KSI*/
	res = KSI_CTX_new(&tmpKsi);
	if (res != KSI_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to initialize KSI context.");
		goto cleanup;
	}

	res = ksitool_initLogger(task, tmpKsi, err);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure KSI logger.");
		goto cleanup;
	}

	res = ksitool_initNetworkProvider(task, tmpKsi, err);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure network provider.");
		goto cleanup;
	}

	res = ksitool_initPublicationFile(task, tmpKsi, err);
	if (res != KT_OK) goto cleanup;


	res = ksitool_initTrustStore(task, tmpKsi, err);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure KSI trust store.");
		goto cleanup;
	}

	*ksi = tmpKsi;
	tmpKsi = NULL;
	res = KT_OK;


cleanup:

	KSI_CTX_free(tmpKsi);

	return res;
}

bool isPiping(paramSet *set) {
	int j;
	char *files[5] = {NULL, NULL, NULL, NULL, NULL};

	paramSet_getStrValueByNameAt(set, "o", 0, &files[0]);
	paramSet_getStrValueByNameAt(set, "log", 0, &files[1]);

	for (j = 0; j < 2; j++) {
		if (files[j] != NULL && strcmp(files[j], "-") == 0) {
			return true;
		}
	}

	return false;
}

void closeTask(KSI_CTX *ksi){
	if (ksi == NULL) goto cleanup;

	KSI_LOG_logCtxError(ksi, KSI_LOG_DEBUG);
	KSI_CTX_free(ksi);

cleanup:
	if(logFile != NULL) fclose(logFile);
}



int getFilesHash(ERR_TRCKR *err, KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash){
	int res;
	FILE *readFrom = NULL;
	unsigned char buf[1024];
	size_t buf_len;
	bool close = false;
	KSI_DataHash *tmp = NULL;

	if(err == NULL || hsr == NULL || fname == NULL || hash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = getStreamFromPath(fname, "rb", &readFrom, &close);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
		goto cleanup;
	}

	while (!feof(readFrom)) {
		buf_len = fread(buf, 1, sizeof (buf), readFrom);
		if(ferror(readFrom)) {
			ERR_TRCKR_ADD(err, res = KT_IO_ERROR, "Error: Unable to read data from file.");
			goto cleanup;
		}

		res = KSI_DataHasher_add(hsr, buf, buf_len);
		ERR_CATCH_MSG(err, res, "Error: Unable to add data to hasher.");
	}

	res = KSI_DataHasher_close(hsr, &tmp);
	ERR_CATCH_MSG(err, res, "Error: Unable close hasher.");

	*hash = tmp;
	tmp = NULL;

	res = KT_OK;

cleanup:

	if (close == true && readFrom != NULL) fclose(readFrom);
	KSI_DataHash_free(tmp);

	return res;
}

static int loadKsiObj(ERR_TRCKR *err, KSI_CTX *ksi, const char *path, void **obj,
					int (*parse)(KSI_CTX *ksi, unsigned char *raw, unsigned raw_len, void **obj),
					void (*obj_free)()){
	int res;
	FILE *readFrom = NULL;
	bool close = false;
	unsigned char *buf = NULL;
	size_t buf_size = 0xffff;
	size_t buf_len = 0;
	void *tmp = NULL;

	if (ksi == NULL || path == NULL || obj == NULL || parse == NULL || obj_free == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = getStreamFromPath(path, "rb", &readFrom, &close);
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

	while (!feof(readFrom)) {
		if (buf_len + 1 >= buf_size) {
			buf_size += 0xffff;
			buf = realloc(buf, buf_size);
			if (buf == NULL) {
				res = KT_OUT_OF_MEMORY;
				ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
				goto cleanup;
			}
		}
		buf_len += fread(buf + buf_len, 1, buf_size - buf_len, readFrom);
		if(ferror(readFrom)) {
			ERR_TRCKR_ADD(err, res = KT_IO_ERROR, "Error: Unable to read data from file.");
			goto cleanup;
		}
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

	if (close == true && readFrom != NULL) fclose(readFrom);
	free(buf);
	obj_free(tmp);

	return res;
}

int loadPublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_PublicationsFile **pubfile) {
	int res;

	if (err == NULL || ksi == NULL || fname == NULL || pubfile == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = loadKsiObj(err, ksi, fname,
				(void**)pubfile,
				(int (*)(KSI_CTX *, unsigned char*, unsigned, void**))KSI_PublicationsFile_parse,
				(void (*)(void *))KSI_PublicationsFile_free);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to load publication file from '%s'.", fname);
	}

	return res;
}

int loadSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_Signature **sig) {
	int res;

	if (ksi == NULL || fname == NULL || sig == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = loadKsiObj(err, ksi, fname,
				(void**)sig,
				(int (*)(KSI_CTX *, unsigned char*, unsigned, void**))KSI_Signature_parse,
				(void (*)(void *))KSI_Signature_free);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to load signature file from '%s'.", fname);
	}
	return res;
}

static int saveKsiObj(ERR_TRCKR *err, KSI_CTX *ksi, void *obj,
							int (*serialize)(KSI_CTX *ksi, void *obj, unsigned char **raw, size_t *raw_len),
							const char *path) {
	int res;
	FILE *writeInto = NULL;
	bool close = false;
	unsigned char *raw = NULL;
	size_t raw_len;
	size_t count;


	if (err == NULL || ksi == NULL || obj == NULL || serialize == NULL || path == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}


	res = serialize(ksi, obj, &raw, &raw_len);
	if (res != KSI_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to serialize.");
		goto cleanup;
	}

	res = getStreamFromPath(path, "wb", &writeInto, &close);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
		goto cleanup;
	}

	count = fwrite(raw, 1, raw_len, writeInto);
	if (count != raw_len) {
		ERR_TRCKR_ADD(err, res = KT_IO_ERROR, "Error: Unable to write to file.");
		goto cleanup;
	}

	res = KT_OK;

cleanup:

	KSI_free(raw);
	if (close == true && writeInto != NULL) fclose(writeInto);

	return res;
}

static int KSI_Signature_serialize_wrapper(KSI_CTX *ksi, KSI_Signature *sig, unsigned char **raw, size_t *raw_len) {
	return KSI_Signature_serialize(sig, raw, raw_len);
}

int saveSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sign, const char *fname) {
	int res;

	if (ksi == NULL || fname == NULL || sign == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = saveKsiObj(err, ksi, sign,
				(int (*)(KSI_CTX *, void *, unsigned char **, size_t *))KSI_Signature_serialize_wrapper,
				fname);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to save signature file to '%s'.", fname);
	}

	return res;
}

int savePublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_PublicationsFile *pubfile, const char *fname) {
	int res;

	if (ksi == NULL || fname == NULL || pubfile == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = saveKsiObj(err, ksi, pubfile,
				(int (*)(KSI_CTX *, void *, unsigned char **, size_t *))KSI_PublicationsFile_serialize,
				fname);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to save publication file to '%s'.", fname);
	}

	return res;
}

bool isSignatureExtended(const KSI_Signature *sig) {
	KSI_PublicationRecord *pubRec = NULL;

	if (sig == NULL) return false;
	KSI_Signature_getPublicationRecord(sig, &pubRec);

	return pubRec == NULL ? false : true;
}

// helpers for hex decoding
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

/**
 * Converts a string into binary array.
 *
 * @param[in] ksi Pointer to KSI KSI_CTX object.
 * @param[in] hexin Pointer to string for conversion.
 * @param[out] binout Pointer to receiving pointer to binary array.
 * @param[out] lenout Pointer to binary array length.
 */
static int getBinaryFromHexString(const char *hexin, unsigned char **binout, size_t *lenout){
	int res;
	size_t len;
	unsigned char *tmp = NULL;
	size_t arraySize;
	unsigned int i, j;

	if (hexin == NULL || binout == NULL || lenout == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	len = strlen(hexin);
	arraySize = len / 2;

	if (len%2 != 0) {
		res = KT_HASH_LENGTH_IS_NOT_EVEN;
		goto cleanup;
	}

	tmp = KSI_calloc(arraySize, sizeof(unsigned char));
	if(tmp == NULL){
		res = KT_OUT_OF_MEMORY;
		goto cleanup;
	}

	for (i = 0, j = 0; i < arraySize; i++, j += 2){
		int value = xx(hexin[j], hexin[j+1]);
		if(value == -1){
			res = KT_INVALID_HEX_CHAR;
			goto cleanup;
		}

		tmp[i] = (unsigned char)value;
	}

	*lenout = arraySize;
	*binout = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	KSI_free(tmp);

	return res;
}

static int getHashAndAlgStrings(const char *instrn, char **strnAlgName, char **strnHash){
	int res ;
	char *colon = NULL;
	size_t algLen = 0;
	size_t hahsLen = 0;
	char *temp_strnAlg = NULL;
	char *temp_strnHash = NULL;
	*strnAlgName = NULL;
	*strnHash = NULL;


	if(instrn == NULL || strnAlgName == NULL || strnHash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	colon = strchr(instrn, ':');
	if (colon != NULL) {
		algLen = (colon - instrn) / sizeof (char);
		hahsLen = strlen(instrn) - algLen - 1;

		temp_strnAlg = calloc(algLen + 1, sizeof (char));
		if (temp_strnAlg == NULL) {
			res = KT_OUT_OF_MEMORY;
			goto cleanup;
		}

		temp_strnHash = calloc(hahsLen + 1, sizeof (char));
		if (temp_strnHash == NULL) {
			res = KT_OUT_OF_MEMORY;
			goto cleanup;
		}

		memcpy(temp_strnAlg, instrn, algLen);
		temp_strnAlg[algLen] = 0;
		memcpy(temp_strnHash, colon + 1, hahsLen);
		temp_strnHash[hahsLen] = 0;
	} else {
		res = KT_INVALID_INPUT_FORMAT;
		goto cleanup;
	}

	*strnAlgName = temp_strnAlg;
	*strnHash = temp_strnHash;
	temp_strnAlg = NULL;
	temp_strnHash = NULL;

	res = KT_OK;

cleanup:

	free(temp_strnAlg);
	free(temp_strnHash);

	return res;
}

int getHashFromCommandLine(const char *imprint, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hash){
	int res;
	unsigned char *data = NULL;
	size_t len;
	KSI_HashAlgorithm hasAlg;
	char *strAlg = NULL;
	char *strHash = NULL;
	KSI_DataHash *tmp = NULL;

	if (imprint == NULL || ksi == NULL || hash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = getHashAndAlgStrings(imprint, &strAlg, &strHash);
	if (res != KT_OK) goto cleanup;

	res = getBinaryFromHexString(strHash, &data, &len);
	if (res != KT_OK) goto cleanup;

	hasAlg = KSI_getHashAlgorithmByName(strAlg);
	if (hasAlg == -1) {
		res = KT_UNKNOWN_HASH_ALG;
		goto cleanup;
	}

	res = KSI_DataHash_fromDigest(ksi, hasAlg, data, (unsigned int)len, &tmp);
	if (res != KSI_OK) goto cleanup;

	*hash = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	free(strAlg);
	free(strHash);
	free(data);
	KSI_DataHash_free(tmp);

	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to get hash from command-line");
		ERR_TRCKR_ADD(err, res, "Error: ", errToString(res));
	}

	return res;
}

static unsigned int elapsed_time_ms;
static bool inProgress = false;
static bool timerOn = false;


static unsigned int measureLastCall(void){
#ifdef _WIN32
    static LARGE_INTEGER thisCall;
    static LARGE_INTEGER lastCall;
    LARGE_INTEGER frequency;        // ticks per second

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&thisCall);

    elapsed_time_ms = (unsigned)((thisCall.QuadPart - lastCall.QuadPart) * 1000.0 / frequency.QuadPart);
#else
    static struct timeval thisCall = {0, 0};
    static struct timeval lastCall = {0, 0};

    gettimeofday(&thisCall, NULL);

    elapsed_time_ms = (unsigned)((thisCall.tv_sec - lastCall.tv_sec) * 1000.0 + (thisCall.tv_usec - lastCall.tv_usec) / 1000.0);
#endif

    lastCall = thisCall;
    return elapsed_time_ms;
}

void print_progressDesc(bool showTiming, const char *msg, ...) {
	va_list va;
	char buf[1024];


	if (inProgress == false) {
		inProgress = true;
		/*If timing info is needed, then measure time*/
		if (showTiming == true) {
			timerOn = true;
			measureLastCall();
		}

		va_start(va, msg);
		vsnprintf(buf, sizeof(buf), msg, va);
		buf[sizeof(buf) - 1] = 0;
		va_end(va);

		print_info("%s", buf);
	}
}

void print_progressResult(int res) {
	static char time_str[32];

	if (inProgress == true) {
		inProgress = false;

		if (timerOn == true) {
			measureLastCall();

			snprintf(time_str, sizeof(time_str), " (%i ms)", elapsed_time_ms);
			time_str[sizeof(time_str) - 1] = 0;
		}

		if (res == KT_OK) {
			print_info("ok.%s\n", timerOn ? time_str : "");
		} else {
			print_info("failed.%s\n", timerOn ? time_str : "");
		}

		timerOn = false;
	}
}


