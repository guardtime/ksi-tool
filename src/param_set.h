/* 
 * File:   gt_cmd_raw.h
 * Author: Taavi
 *
 * Created on September 18, 2014, 6:07 PM
 */

#ifndef PARAM_SET_H
#define	PARAM_SET_H

#include "gt_cmd_control.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct paramValu_st paramValue;
typedef struct rawParam_st rawParam;
typedef struct paramSet_st paramSet;	

bool paramSet_new(const char *names, paramSet **set);
void paramSet_readFromcCMD(int argc, char **argv, char* definition, paramSet *rawParam);
bool paramSet_isFormatOK(paramSet *set);
void paramSet_Print(paramSet *set);
void paramSet_free(paramSet *set);

bool paramSet_isSetByName(paramSet *set, char *name);

bool paramSet_getIntValueByNameAt(paramSet *set, char *name,int at, int *value);
bool paramSet_getStrValueByNameAt(paramSet *set, char *name,int at, char **value);
bool paramSet_getValueCountByName(paramSet *set, char *name, int *count);
void paramSet_removeParameterByName(paramSet *set, char *name);
bool paramSet_isSetByName(paramSet *set, char *name);
void paramSet_addControl(paramSet *set, const char *names, FormatStatus (*controlFormat)(const char *), ContentStatus (*controlContent)(const char *));

char *getParametersName(const char* names, char *buf, short len, bool *isMultiple);

#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMD_RAW_H */

