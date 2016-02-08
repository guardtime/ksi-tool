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
	
int isFormatOk_inputHash(const char *str);
int isContentOk_inputHash(const char *str);
int extract_inputHash(void *extra, const char* str, void** obj);


#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_CONTROL_H */

