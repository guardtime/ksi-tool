/* 
 * File:   param_control.h
 * Author: Taavi
 *
 * Created on February 5, 2016, 10:38 AM
 */

#ifndef PARAM_CONTROL_H
#define	PARAM_CONTROL_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct COMPOSITE_st COMPOSITE;;

struct COMPOSITE_st {
	void *ctx;
	void *err;
};


enum contentStatus {
	PARAM_OK = 0x00,
	PARAM_INVALID,
	HASH_ALG_INVALID_NAME,
	HASH_IMPRINT_INVALID_LEN,
	INTEGER_TOO_LARGE,
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
int extract_hashAlg(void *extra, const char* str, void** obj);

int isFormatOk_inputFile(const char *path);
int isContentOk_inputFile(const char* path);
int extract_inputFile(void *extra, const char* str, void** obj);

int isFormatOk_path(const char *path);
int convertRepair_path(const char* arg, char* buf, unsigned len);

int isFormatOk_inputHash(const char *str);
int isContentOk_inputHash(const char *str);
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

#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_CONTROL_H */

