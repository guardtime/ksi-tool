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

int isFormatOk_inputFile(const char *path);
int isContentOk_inputFile(const char* path);
int convert_repair_path(const char* arg, char* buf, unsigned len);
int extract_inputFile(void *extra, const char* str, void** obj);

int isFormatOk_inputHash(const char *str);
int isContentOk_inputHash(const char *str);
int extract_inputHash(void *extra, const char* str, void** obj);

int isFormatOk_int(const char *integer);

int isFormatOk_url(const char *url);
int convertRepair_url(const char* arg, char* buf, unsigned len);

int isFormatOk_pubString(const char *str);
int extract_pubString(void *extra, const char* str, void** obj);


int extract_inputSignature(void *extra, const char* str, void** obj);

#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_CONTROL_H */

