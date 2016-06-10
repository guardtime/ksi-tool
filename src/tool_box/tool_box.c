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

#include "tool_box.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ksi/ksi.h>
#include <ksi/compatibility.h>
#include "tool_box/smart_file.h"
#include "tool_box/err_trckr.h"
#include "param_set/param_set.h"
#include "printer.h"

#ifdef _WIN32
#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#include <stdlib.h>
#else
#   include <limits.h>
#	include <sys/time.h>
#endif

static char *OID_EMAIL[] = {KSI_CERT_EMAIL, "E", "email", "e-mail", "e_mail", "emailAddress", NULL};
static char *OID_COMMON_NAME[] = {KSI_CERT_COMMON_NAME, "CN", "common name", "common_name", "cname", NULL};
static char *OID_COUNTRY[] = {KSI_CERT_COUNTRY, "C", "country", NULL};
static char *OID_ORGANIZATION[] = {KSI_CERT_ORGANIZATION, "O", "org", "organization", NULL};
static char **OID_INFO[] = {OID_EMAIL, OID_COMMON_NAME, OID_COUNTRY, OID_ORGANIZATION, NULL};

static int KSI_Signature_serialize_wrapper(KSI_CTX *ksi, KSI_Signature *sig, unsigned char **raw, size_t *raw_len) {
	if (ksi);
	return KSI_Signature_serialize(sig, raw, raw_len);
}

static int load_ksi_obj(ERR_TRCKR *err, KSI_CTX *ksi, const char *path, void **obj,	int (*parse)(KSI_CTX *ksi, unsigned char *raw, unsigned raw_len, void **obj), void (*obj_free)(void*), const char *name) {
	int res;
	SMART_FILE *file = NULL;
	unsigned char buf[0xffff + 4];
	unsigned char dummy[1];
	void *tmp = NULL;
	size_t data_len = 0;
	size_t dummy_len = 0;


	if (ksi == NULL || path == NULL || obj == NULL || parse == NULL || obj_free == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = SMART_FILE_open(path, "rb", &file);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: %s", KSITOOL_errToString(res));
		goto cleanup;
	}

	res = SMART_FILE_read(file, (char *)buf, sizeof(buf), &data_len);
	if(res != SMART_FILE_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to read %s from file.", name);
		goto cleanup;
	}

	res = SMART_FILE_read(file, (char *)dummy, sizeof(dummy), &dummy_len);
	if(res != SMART_FILE_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to read data from file.");
		goto cleanup;
	}

	if (dummy_len != 0 || !SMART_FILE_isEof(file)) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_INPUT_FORMAT, "Error: Input file too long for a valid %s file.", name);
		goto cleanup;
	}

	if (data_len == 0) {
		ERR_TRCKR_ADD(err, res = KT_INVALID_INPUT_FORMAT, "Error: Input file is empty.", name);
		goto cleanup;
	}

	res = parse(ksi, buf, (unsigned)data_len, &tmp);
	if (res != KSI_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to parse %s.", name);
		goto cleanup;
	}

	*obj = tmp;
	tmp = NULL;
	res = KT_OK;


cleanup:

	SMART_FILE_close(file);
	obj_free(tmp);

	return res;
}

static int saveKsiObj(ERR_TRCKR *err, KSI_CTX *ksi, const char *mode, void *obj, int (*serialize)(KSI_CTX *ksi, void *obj, unsigned char **raw, size_t *raw_len), const char *path, char *f, size_t f_len) {
	int res;
	SMART_FILE *file = NULL;
	unsigned char *raw = NULL;
	size_t raw_len = 0;
	size_t count = 0;
	char mode_sum[32];

	if (err == NULL || ksi == NULL || obj == NULL || serialize == NULL || path == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	KSI_snprintf(mode_sum, sizeof(mode_sum), "wb%s", (mode == NULL) ? "" : mode);

	res = serialize(ksi, obj, &raw, &raw_len);
	if (res != KSI_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to serialize.");
		goto cleanup;
	}

	res = SMART_FILE_open(path, mode_sum, &file);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: %s", KSITOOL_errToString(res));
		goto cleanup;
	}

	if (f != NULL && f_len != 0) {
		KSI_strncpy(f, SMART_FILE_getFname(file), f_len);
	}

	res = SMART_FILE_write(file, (char *)raw, raw_len, &count);
	if(res != SMART_FILE_OK || count != raw_len) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to write to file.");
		goto cleanup;
	}

	res = KT_OK;

cleanup:

	KSI_free(raw);
	SMART_FILE_close(file);

	return res;
}

int KSI_OBJ_saveSignature(ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sign, const char *mode, const char *fname, char *f, size_t f_len) {
	int res;

	if (ksi == NULL || fname == NULL || sign == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = saveKsiObj(err, ksi, mode, sign,
				(int (*)(KSI_CTX *, void *, unsigned char **, size_t *))KSI_Signature_serialize_wrapper,
				fname, f, f_len);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to save signature file to '%s'.", fname);
	}

	return res;
}

int KSI_OBJ_savePublicationsFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_PublicationsFile *pubfile, const char *mode, const char *fname) {
	int res;

	if (ksi == NULL || fname == NULL || pubfile == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = saveKsiObj(err, ksi, mode, pubfile,
				(int (*)(KSI_CTX *, void *, unsigned char **, size_t *))KSI_PublicationsFile_serialize,
				fname, NULL, 0);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to save publications file to '%s'.", fname);
	}

	return res;
}

int KSI_OBJ_loadSignature(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_Signature **sig) {
	int res;

	if (ksi == NULL || fname == NULL || sig == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = load_ksi_obj(err, ksi, fname,
				(void**)sig,
				(int (*)(KSI_CTX *, unsigned char*, unsigned, void**))KSI_Signature_parse,
				(void (*)(void *))KSI_Signature_free,
				"KSI Signature");

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to load signature file from '%s'.", fname);
	}
	return res;
}

int KSI_OBJ_isSignatureExtended(const KSI_Signature *sig) {
	KSI_PublicationRecord *pubRec = NULL;

	if (sig == NULL) return 0;
	KSI_Signature_getPublicationRecord(sig, &pubRec);

	return pubRec == NULL ? 0 : 1;
}


const char *OID_getShortDescriptionString(const char *OID) {
	unsigned i = 0;

	if (OID == NULL) return NULL;

	while (OID_INFO[i] != NULL) {
		if (strcmp(OID_INFO[i][0], OID) == 0) return OID_INFO[i][1];
		i++;
	}

	return OID;
}

const char *OID_getFromString(const char *str) {
	unsigned i = 0;
	unsigned n = 0;
	const char *OID = NULL;
	size_t len;

	if (str == NULL) {
		OID = NULL;
		goto cleanup;
	};

	while (OID_INFO[i] != NULL) {
		n = 1;
		while (OID_INFO[i][n] != NULL) {
			len = strlen(OID_INFO[i][n]);
			if (strncmp(OID_INFO[i][n], str, len) == 0 && str[len] == '=') {
				OID = OID_INFO[i][0];
				goto cleanup;
			}
			n++;
		}
		i++;
	}

cleanup:
	return OID;
}


static unsigned int elapsed_time_ms;
static int inProgress = 0;
static int timerOn = 0;


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

void print_progressDesc(int showTiming, const char *msg, ...) {
	va_list va;
	char buf[1024];


	if (inProgress == 0) {
		inProgress = 1;
		/*If timing info is needed, then measure time*/
		if (showTiming == 1) {
			timerOn = 1;
			measureLastCall();
		}

		va_start(va, msg);
		KSI_vsnprintf(buf, sizeof(buf), msg, va);
		buf[sizeof(buf) - 1] = 0;
		va_end(va);

		print_debug("%s", buf);
	}
}

void print_progressResult(int res) {
	static char time_str[32];

	if (inProgress == 1) {
		inProgress = 0;

		if (timerOn == 1) {
			measureLastCall();

			KSI_snprintf(time_str, sizeof(time_str), " (%i ms)", elapsed_time_ms);
			time_str[sizeof(time_str) - 1] = 0;
		}

		if (res == KT_OK) {
			print_debug("ok.%s\n", timerOn ? time_str : "");
		} else {
			print_debug("failed.%s\n", timerOn ? time_str : "");
		}

		timerOn = 0;
	}
}

const char *getPublicationsFileRetrieveDescriptionString(PARAM_SET *set) {
	int res;
	const char *pub_desc_str = "Receiving publications file... ";
	char *pubfile_uri = NULL;

	if (set == NULL) goto cleanup;

	res = PARAM_SET_getStr(set, "P", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &pubfile_uri);
	if (res != PST_OK) goto cleanup;

	if (strstr(pubfile_uri, "file://") != NULL) {
		pub_desc_str = "Loading publications file from file... ";
	}

cleanup:

	return pub_desc_str;
}

char *STRING_getBetweenWhitespace(const char *strn, char *buf, size_t buf_len) {
	const char *beginning = NULL;
	const char *end = NULL;
	size_t strn_len;
	size_t new_len;

	if (strn == NULL || buf == NULL || buf_len == 0) return NULL;

	strn_len = strlen(strn);
	beginning = strn;
	end = strn + strn_len; //End should be at terminating NUL


	while(beginning != end && isspace(*beginning)) beginning++;
	while(end != beginning && (isspace(*end) || *end == '\0')) end--;


	new_len = end + 1 - beginning;
	if (KSI_strncpy(buf, beginning, new_len + 1) == NULL) return NULL;

	return buf;
}

const char * STRING_locateLastOccurance(const char *str, const char *findIt) {
	const char * ret = NULL;
	const char * isFound = NULL;
	const char *searchFrom = NULL;

	if (str == NULL || findIt == NULL) return NULL;
	if(*str == '\0' && *findIt == '\0') return str;


	searchFrom = str;

	while (*searchFrom != '\0' && (isFound = strstr(searchFrom, findIt)) != NULL) {
		searchFrom = isFound + 1;
		ret = isFound;
	}
	return ret;
}

const char* find_charAfterStrn(const char *str, const char *findIt) {
	size_t findIt_len = 0;
	const char * beginning = NULL;
	findIt_len = strlen(findIt);
	beginning = strstr(str, findIt);
	return (beginning == NULL) ? NULL : beginning + findIt_len;
}

static const char* find_charAfter_abstract(const char *str, const char *findIt, const char* (*find)(const char *str, const char *findIt)) {
	size_t findIt_len = 0;
	const char * beginning = NULL;
	const char * ret = NULL;
	findIt_len = strlen(findIt);
	beginning = find(str, findIt);
	if (beginning == NULL) return NULL;
	ret = beginning + findIt_len;
	return ret;
}

const char* find_charAfterLastStrn(const char* str, const char* findIt) {
	return find_charAfter_abstract(str, findIt, STRING_locateLastOccurance);
}

static const char* find_charBefore_abstract(const char *str, const char *findIt, const char* (*find)(const char *str, const char *findIt)) {
	const char *right = NULL;
	right = find(str, findIt);
	right = (right == str || right == NULL) ? right : right - 1;
	return right;
}

const char* find_charBeforeStrn(const char* str, const char* findIt) {
	return find_charBefore_abstract(str, findIt,
			(const char* (*)(const char *, const char *))strstr);
}

const char* find_charBeforeLastStrn(const char* str, const char* findIt) {
	return find_charBefore_abstract(str, findIt, STRING_locateLastOccurance);
}

char *STRING_extractAbstract(const char *strn, const char *from, const char *to, char *buf, size_t buf_len,
		const char* (*find_from)(const char *str, const char *findIt),
		const char* (*find_to)(const char *str, const char *findIt),
		const char** firstChar) {
	const char *beginning = NULL;
	const char *end = NULL;
	size_t new_len;

	if (strn == NULL || buf == NULL || buf_len == 0) return NULL;

	if (find_from == NULL) find_from = find_charAfterStrn;
	if (find_to == NULL) find_to = find_charBeforeStrn;

	beginning = (from == NULL) ? strn : find_from(strn, from);
	end = (to == NULL) ? (strn + strlen(strn) - 1) : find_to(strn, to);

	if (beginning == NULL || end == NULL) return NULL;

	if ((end + 1) < beginning) new_len = 0;
	else new_len = end + 1 - beginning;

	new_len = (new_len + 1 > buf_len) ? buf_len - 1 : new_len;
	if (KSI_strncpy(buf, beginning, new_len + 1) == NULL) return NULL;

	if (firstChar != NULL) {
		*firstChar = (*end == '\0') ? NULL : end + 1;
	}

	return buf;
}

char *STRING_extract(const char *strn, const char *from, const char *to, char *buf, size_t buf_len) {
	return STRING_extractAbstract(strn, from, to ,buf, buf_len,
			NULL,
			find_charBeforeLastStrn,
			NULL);
}

char *STRING_extractRmWhite(const char *strn, const char *from, const char *to, char *buf, size_t buf_len) {
	char *tmp = NULL;
	char *ret = NULL;
	char *isBuf = NULL;

	if (strn == NULL || buf == NULL || buf_len == 0) goto cleanup;

	tmp = (char *)malloc(buf_len);
	if (tmp == NULL) goto cleanup;

	isBuf = STRING_extract(strn, from, to, tmp, buf_len);
	if (isBuf != tmp) goto cleanup;

	isBuf = STRING_getBetweenWhitespace(tmp, buf, buf_len);
	if (isBuf != buf) goto cleanup;

	ret = buf;

cleanup:

	free(tmp);

	return ret;
}

static const char* find_group_start(const char *str, const char *ignore) {
	size_t i = 0;
	if (ignore);
	while (str[i] && isspace(str[i])) i++;
	return (i == 0) ? str : &str[i];
}

static const char* find_group_end(const char *str, const char *ignore) {
	int is_quato_opend = 0;
	int is_firs_non_whsp_found = 0;
	size_t i = 0;

	if (ignore);
	while (str[i]) {
		if (!isspace(str[i])) is_firs_non_whsp_found = 1;
		if (isspace(str[i]) && is_quato_opend == 0 && is_firs_non_whsp_found) return &str[i - 1];

		if (str[i] == '"' && is_quato_opend == 0) is_quato_opend = 1;
		else if (str[i] == '"' && is_quato_opend == 1) return &str[i];

		i++;
	}

	return i == 0 ? str : &str[i];
}


static void string_removeChars(char *str, char rem) {
	size_t i = 0;
	size_t str_index = 0;
	char *itr = str;

	if (itr == NULL) return;

	while(itr[i]) {
		if (itr[i] != rem) {
			str[str_index] = itr[i];
			str_index++;
		}
		i++;
	}
	str[str_index] = '\0';
}

const char *STRING_getChunks(const char *strn, char *buf, size_t buf_len) {
	const char *next = NULL;
	STRING_extractAbstract(strn, "", "", buf, buf_len, find_group_start, find_group_end, &next);
	string_removeChars(buf, '"');
	return next;
}

static const char *path_removeFile(const char *origPath, char *buf, size_t buf_len) {
	char *beginingOfFile = NULL;
	size_t path_len;
	char *ret = NULL;

#ifdef _WIN32
	beginingOfFile = strrchr(origPath, '\\');
	if (beginingOfFile == NULL) {
		beginingOfFile = strrchr(origPath, '/');
	}
#else
	beginingOfFile = strrchr(origPath, '/');
#endif

	if (beginingOfFile ==  NULL) {
		buf[0] = '\0';
		return buf;
	}

	path_len = beginingOfFile - origPath;
	if (path_len + 1 > buf_len) return NULL;

	ret = KSI_strncpy(buf, origPath, path_len + 1);
	buf[path_len + 1] = 0;
	return  ret;
}

const char *PATH_getPathRelativeToFile(const char *refFilePath, const char *origPath, char *buf, size_t buf_len) {
	size_t origPath_len;
	size_t refPath_len;
	char refPath_last_char;
	char refPathAppend[2] = {0,0};
	int isAbsolute = 0;
	char path_buf[2048];
	const char *refPath = NULL;

	refPath = path_removeFile(refFilePath, path_buf, sizeof(path_buf));
	refPath_len = strlen(refPath);
	origPath_len = strlen(origPath);
	if (origPath_len == 0) return NULL;

		/**
	 * Check if origPath is relative or absolute path.
	 */
#ifndef _WIN32
	if (origPath[0] == '/') isAbsolute = 1;
	else isAbsolute = 0;
#else
	if (origPath_len >= 3 && origPath[1] == ':' && (origPath[2] == '/' || origPath[2] == '\\')) isAbsolute = 1;
	else isAbsolute = 0;
#endif

	if (isAbsolute == 1) {
		return origPath;
	} else {
		/**
		 * If ref path has last char as \ or / then do nothing
		 */
		refPath_last_char = (refPath_len == 0) ? '\0' : refPath[refPath_len - 1];

		if (refPath_last_char != '\0' && refPath_last_char != '/' && refPath_last_char != '\\') {
			refPathAppend[0] = '/';
		} else {
			refPathAppend[0] = '\0';
		}
		KSI_snprintf(buf, buf_len, "%s%s%s", refPath, refPathAppend, origPath);
		path_buf[buf_len - 1] = '\0';
		return buf;
	}
}

const char *PATH_URI_getPathRelativeToFile(const char *refFilePath, const char *origPath, char *buf, size_t buf_len) {
	int isFileUri = 0;
	const char *pOrigPath = origPath;
	char buf_inner[2048];

	/**
	 * Check if uri scheme is file, if not return the original uri.
	 */
	if (origPath == NULL) return NULL;
	isFileUri = (strstr(origPath, "file://") == origPath) ? 1 : 0;
	if (isFileUri && strlen(origPath) == 7) return origPath;
	if (!isFileUri) return origPath;

	/**
	 * If uri scheme is file, cut the files path and convert. Append to file scheme.
	 */
	pOrigPath = isFileUri ? origPath + 7 : origPath;
	KSI_snprintf(buf, buf_len, "file://%s", PATH_getPathRelativeToFile(refFilePath, pOrigPath, buf_inner, sizeof(buf_inner)));
	return buf;
}
