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
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "gt_cmd_control.h"
#include "param_set.h"
#include "param_set_obj_impl.h"
#include "Parameter.h"
#include "ParamValue.h"

#define SINGLE false
#define MULTIPLE true

#define UNKNOWN_PARAMETER_NAME "_UN_KNOWN_"
#define TYPO_PARAMETER_NAME "_TYPO_"


static char *getParametersName(const char* names, char *name, char *alias, short len, bool *isMultiple, bool *isSingleHighestPriority){
	char *pName = NULL;
	int i = 0;
	bool isAlias = false;
	short i_name=0, i_alias=0;

	if(names == NULL || name == NULL) return NULL;
	if(names[0] == 0) return NULL;

	pName=strchr(names, '{');
	if(pName == NULL) return NULL;
	pName++;
	while(pName[i] != '}' && pName[i] != 0){
		if(len-1 <= i_name || len-1 <= i_alias){
			return NULL;
		}

		if(pName[i] == '|'){
			isAlias = true;
			i++;
		}

		if(isAlias && alias){
			alias[i_alias++] = pName[i];
		}else{
			name[i_name++] = pName[i];
		}
		i++;
	}
	if(isMultiple != NULL && pName[i] == '}'){
		if(pName[i+1] == '*')
			*isMultiple = true;
		else
			*isMultiple = false;
	}
	if(isSingleHighestPriority != NULL && pName[i] == '}'){
		if(pName[i+1] == '>')
			*isSingleHighestPriority = true;
		else
			*isSingleHighestPriority = false;
	}


	name[i_name] = 0;
	if(alias)
		alias[i_alias] = 0;

	return &pName[i];
}

static void paramSet_getParameterByName(const PARAM_SET *set, const char *name, PARAM **param){
	int numOfElements = 0;
	PARAM **array = NULL;
	PARAM *tmp = NULL;
	int i =0;

	if(set == NULL || param == NULL || name == NULL) return;
	*param = NULL;
	numOfElements = set->count;

	array = set->parameter;

	for(i=0; i<numOfElements; i++){
		tmp = array[i];
		if(tmp != NULL){
			if(strcmp(tmp->flagName, name) == 0 || (tmp->flagAlias && strcmp(tmp->flagAlias, name) == 0)){
				*param = tmp;
				return;
			}
		}
	}
	return;
}

bool paramSet_priorityAppendParameterByName(const char *name, const char *value, const char *source, int priority, PARAM_SET *set){
	PARAM *param = NULL;

	if(set == NULL || name == NULL) return false;

	paramSet_getParameterByName(set, name, &param);
	if(param == NULL) return false;

	return parameter_addArgument(param, value, source, priority);
}

bool paramSet_appendParameterByName(const char *name, const char *value, const char *source, PARAM_SET *set){
	return paramSet_priorityAppendParameterByName(name, value, source, 0, set);
}

void paramSet_removeParameterByName(PARAM_SET *set, const char *name){
	int numOfElements = 0;
	PARAM **array = NULL;
	PARAM *tmp = NULL;
	int i =0;

	if(set == NULL || name == NULL) return;
	numOfElements = set->count;

	array = set->parameter;


	for(i=0; i<numOfElements; i++){
		tmp = array[i];
		if(tmp != NULL){
			if(strcmp(tmp->flagName, name) == 0 || (tmp->flagAlias && strcmp(tmp->flagAlias, name) == 0)){
				tmp->argCount = 0;
				if(tmp->arg) PARAM_VAL_free(tmp->arg);
				tmp->arg = NULL;
				return;
			}
		}
	}
	return;
}

static void paramSet_getFirstValueByName(const PARAM_SET *set, const char *name, PARAM_VAL **value){
	PARAM *tmp = NULL;

	if(set == NULL || value == NULL || name == NULL) return;
	*value = NULL;

	paramSet_getParameterByName(set, name, &tmp);
	if(tmp == NULL) return;

	*value = tmp->arg;

	return;
}

static void paramSet_getHighestPriorityValueByName(const PARAM_SET *set, const char *name, PARAM_VAL **value){
	PARAM *tmp = NULL;

	if(set == NULL || value == NULL || name == NULL) return;
	*value = NULL;

	paramSet_getParameterByName(set, name, &tmp);
	if(tmp == NULL) return;

	*value = (PARAM_VAL*)PARAM_VAL_getFirstHighestPriorityValue(tmp->arg);
	return;
}

static void paramSet_getValueByNameAt(const PARAM_SET *set, const char *name, unsigned at, PARAM_VAL **value){
	PARAM_VAL *tmp = NULL;

	if(set == NULL || value == NULL || name == NULL) return;
	*value = NULL;

	paramSet_getFirstValueByName(set,name, &tmp);
	if(tmp == NULL) return;

	tmp = PARAM_VAL_getElementAt(tmp, at);
	*value = tmp;

	return;
}


bool paramSet_new(const char *names,
		int (*printInfo)(const char*, ...), int (*printWarnings)(const char*, ...), int (*printErrors)(const char*, ...),
		PARAM_SET **set){
	PARAM_SET *tmp = NULL;
	bool status = false;
	const char *pName = NULL;
	int paramCount = 0;
	int i = 0;
	int iter = 0;
	char mem = 0;
	char buf[256];
	char alias[256];
	short len = 0;
	bool isMultiple = false;
	bool isSingleHighestPriority = false;

	if(set == NULL || names == NULL || printInfo == NULL || printWarnings == NULL || printErrors == NULL) return false;

	while(names[i]){
		if(names[i] == '{') mem = names[i];
		else if(mem == '{' && names[i] == '}'){
			paramCount++;
			mem = 0;
		}
		i++;
	}
	/*Add extra count for Unknowns and Typos*/
	paramCount+=2;

	tmp = (PARAM_SET*)calloc(sizeof(PARAM_SET), 1);
	if(tmp == NULL) goto cleanup;
	tmp->parameter = (PARAM**)calloc(paramCount, sizeof(PARAM*));
	if(tmp->parameter == NULL) goto cleanup;

	tmp->count = paramCount;

	tmp->printInfo = printInfo;
	tmp->printWarning = printWarnings;
	tmp->printError = printErrors;

	len = sizeof(buf);
	pName = names;

	while((pName = getParametersName(pName, buf, alias, len, &isMultiple, &isSingleHighestPriority)) != NULL){
		if(!parameter_new(buf,alias[0] ? alias : NULL, isMultiple, isSingleHighestPriority, NULL, NULL, NULL, &tmp->parameter[iter]))
			goto cleanup;
		iter++;
	}

	parameter_new(UNKNOWN_PARAMETER_NAME, NULL,MULTIPLE, false,  NULL, NULL, NULL, &tmp->parameter[iter++]);
	parameter_new(TYPO_PARAMETER_NAME, NULL,MULTIPLE, false, NULL, NULL, NULL, &tmp->parameter[iter]);

	*set = tmp;
	tmp = NULL;
	status = true;

cleanup:

	paramSet_free(tmp);
	return status;
}


int (*paramSet_getErrorPrinter(PARAM_SET *set))(const char*, ...) {
	return set->printError;
}

int (*paramSet_getWarningPrinter(PARAM_SET *set))(const char*, ...) {
	return set->printWarning;
}

void paramSet_free(PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i =0;

	if(set == NULL) return;
	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++)
		parameter_free(array[i]);
	free(set->parameter);

	free(set);
	return;
	}

void paramSet_addControl(PARAM_SET *set, const char *names,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		bool (*convert)(const char*, char*, unsigned)){
	PARAM *tmp = NULL;
	const char *pName = NULL;
	char buf[256];

	pName = names;
	while((pName = getParametersName(pName,buf, NULL, sizeof(buf), NULL, NULL)) != NULL){
		paramSet_getParameterByName(set, buf, &tmp);
		if(tmp != NULL){
			tmp->controlFormat = controlFormat;
			tmp->controlContent = controlContent;
			tmp->convert = convert;
		}
	}
}

static unsigned min_of_3(unsigned A, unsigned B,unsigned C){
	unsigned tmp;
	tmp = A < B ? A : B;
	return tmp < C ? tmp : C;
}

#define UP(m,i,j) m[i-1][j]	//del
#define LEFT(m,i,j) m[i][j-1] //ins
#define DIAG(m,i,j) m[i-1][j-1] //rep

/*TODO refactor*/
static int editDistance_levenshtein(const char *A, const char *B){
	unsigned lenA, lenB;
	unsigned M_H, M_W;
	unsigned i=0, j=0;
	char **m;
	unsigned rows_created = 0;
	int edit_distance = -1;

	/*Get the size of each string*/
	lenA = (unsigned)strlen(A);//vertical
	lenB = (unsigned)strlen(B);//horizontal
	/*Data matrix is larger by 1 row and column*/
	M_H = lenA+1;
	M_W = lenB+1;

	/*Creating of initial matrix*/
	m=(char**)malloc(M_H*sizeof(char*));
	if(m == NULL) goto cleanup;

	for(i=0; i<M_H; i++){
		m[i]=(char*)malloc(M_W*sizeof(char));
		if(m[i] == NULL) goto cleanup;
		m[i][0] = 0xff & i;
		rows_created++;
	}

	for(j=0; j<M_W; j++) m[0][j] = 0xff & j;

	for(j=1; j<M_W; j++){
		for(i=1; i<M_H; i++){
			if(A[i-1] == B[j-1]) m[i][j] = DIAG(m,i,j);
			else m[i][j] = 0xff & min_of_3(UP(m,i,j), LEFT(m,i,j), DIAG(m,i,j)) + 1;
		}
	}
	edit_distance = m[i-1][j-1];


cleanup:
	if(m)
		for(i=0; i<rows_created; i++) free(m[i]);
	free(m);

	return edit_distance;
}

static bool paramSet_couldItBeTypo(const char *str, const PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i =0;

	if(set == NULL) return false;
	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++){
		unsigned lenName = 0;
		int editDist = 0;

		editDist = editDistance_levenshtein(array[i]->flagName, str);
		lenName = (unsigned)strlen(array[i]->flagName);

		if((editDist*100)/lenName <= 49)
			return true;

		if(array[i]->flagAlias){
			editDist = editDistance_levenshtein(array[i]->flagAlias, str);
			lenName = (unsigned)strlen(array[i]->flagAlias);

			if((editDist*100)/lenName <= 49)
				return true;
		}
	}

	return false;
}

/**
 * This functions adds raw parameter to the set. It parses parameters formatted as:
 * --long		- long parameter without argument.
 * --long <arg>	- long parameter with argument.
 * -i <arg>		- short parameter with argument.
 * -vxn			- bunch of flags.
 * @param param - parameter.
 * @param arg
 * @param set
 */
static void paramSet_addRawParameter(const char *param, const char *arg, const char *source, PARAM_SET *set, int priority){
	const char *flag = NULL;
	unsigned len;

	len = (unsigned)strlen(param);
	if(param[0] == '-' && param[1] != 0){
		flag = param+(param[1] == '-' ? 2 : 1);

		/*It is long parameter*/
		if(strncmp("--", param, 2)==0 && len >= 3){
			if(param[3] == 0){
				paramSet_appendParameterByName(TYPO_PARAMETER_NAME, flag, source, set);
				if(arg)
					paramSet_appendParameterByName(paramSet_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, arg, source, set);
				return;
			}

			if(paramSet_priorityAppendParameterByName(flag, arg, source, priority, set)==false){
				paramSet_appendParameterByName(paramSet_couldItBeTypo(flag, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, flag, source, set);
				if(arg)
					paramSet_appendParameterByName(paramSet_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, arg, source, set);
			}
		}
		/*It is short parameter*/
		else if(param[0] == '-' && len == 2){
			if(paramSet_priorityAppendParameterByName(flag, arg, source, priority, set)==false){
				paramSet_appendParameterByName(UNKNOWN_PARAMETER_NAME, flag, source, set);
				if(arg)
					paramSet_appendParameterByName(paramSet_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, arg, source, set);
			}
		}
		/*It is bunch of flags*/
		else{
			//paramSet_appendParameterByName(arg,flag,set)==false
			char str_flg[2] = {255,0};
			int itr = 0;

			if(arg)
				paramSet_appendParameterByName(paramSet_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, arg, source, set);

			while((str_flg[0] = flag[itr++]) != '\0'){
				if(paramSet_priorityAppendParameterByName(str_flg, NULL, source, priority, set)==false){
					if(paramSet_couldItBeTypo(flag, set)){
						paramSet_appendParameterByName(TYPO_PARAMETER_NAME, flag, source, set);
						break;
					}
					else
						paramSet_appendParameterByName(UNKNOWN_PARAMETER_NAME, str_flg, source, set);
				}
			}

		}
	}
	else{
		paramSet_appendParameterByName(UNKNOWN_PARAMETER_NAME, param, source, set);
	}
}

void paramSet_readFromFile(const char *fname, PARAM_SET *set, int priority){
	FILE *file = NULL;
	char *ln = NULL;
	char line[1024];
	char flag[1024];
	char arg[1024];

	if(fname == NULL || set == NULL) goto cleanup;

	file = fopen(fname, "r");
	if(file == NULL) goto cleanup;

	while(fgets(line, sizeof(line),file)){
		ln = strchr(line, '\n');
		if(ln != NULL) *ln = 0;

		if(sscanf(line, "%s %s", flag, arg) == 2)
			paramSet_addRawParameter(flag, arg, fname, set, priority);
		else
			paramSet_addRawParameter(line,NULL, fname, set, priority);
	}

cleanup:

	if(file) fclose(file);
	return;
}

void paramSet_readFromCMD(int argc, char **argv, PARAM_SET *set, int priority){
	int i=0;
	char *tmp = NULL;
	char *arg = NULL;

	if(set == NULL) return;

	for (i = 1; i < argc; i++){
		tmp = argv[i];
		arg = NULL;

		if (i + 1 < argc) {
			if(argv[i + 1][0] != '-' || (argv[i + 1][0] == '-' && argv[i + 1][1] == 0))
				arg = argv[++i];
		}

		paramSet_addRawParameter(tmp, arg, NULL, set, priority);
	}

	return;
}

bool paramSet_isFormatOK(const PARAM_SET *set){
	PARAM **array = NULL;
	PARAM *pParam = NULL;
	int i = 0;
	int numOfElements = 0;
	PARAM_VAL *value = NULL;

	if(set == NULL) return false;
	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++){
		if((pParam = array[i]) != NULL){
			if(strcmp(pParam->flagName, UNKNOWN_PARAMETER_NAME)==0) continue;
			if(strcmp(pParam->flagName, TYPO_PARAMETER_NAME)==0) continue;

			/*Control duplicate conflicts*/
			if(parameter_isDuplicateConflict(pParam) == true){

				return false;
			}

			value = pParam->arg;
			while(value){
				if(value->formatStatus == FORMAT_OK){
					if(value->contentStatus != PARAM_OK){
						return false;
					}
				}
				else{
					return false;
				}

				value = value->next;
			}
		}
	}

	return true;
}

static bool getValue(const PARAM_SET *set, const char *name, unsigned at, bool controlFormat,
	void (*getter)(const PARAM_SET*, const char*, unsigned, PARAM_VAL**),
	void (*convert)(char*, void**),
	void **value){

	PARAM_VAL *tmp = NULL;

	if(set == NULL || name == NULL || getter == NULL) return false;
	/*Get parameter->value at position*/
	getter(set, name, at, &tmp);
	if (tmp == NULL) return false;
	if (tmp->formatStatus != FORMAT_OK && controlFormat) return false;
	/*Convert value->string into format*/
	if(convert && value) convert(tmp->cstr_value, value);
	return true;
}

static void wrapper_getHighestPriorityValueByName(const PARAM_SET *set, const char *name, unsigned at, PARAM_VAL **value){
	paramSet_getHighestPriorityValueByName(set, name, value);
}

void wrapper_returnStr(char* str, void** obj){
	*obj = (void*)str;
}

typedef struct INT_st {int value;} INT;

void wrapper_returnInt(char* str,  void** obj){
	((INT*)obj)->value = atoi(str);
}

bool paramSet_getIntValueByNameAt(const PARAM_SET *set, const char *name, unsigned at, int *value){
	INT val;
	bool stat;
	stat = getValue(set, name, at, true, paramSet_getValueByNameAt, wrapper_returnInt, (void**)&val);
	*value = val.value;
	return stat;
}

bool paramSet_getStrValueByNameAt(const PARAM_SET *set, const char *name, unsigned at, char **value){
	return getValue(set, name, at, true, paramSet_getValueByNameAt, wrapper_returnStr, (void**)value);
}

bool paramSet_getHighestPriorityIntValueByName(const PARAM_SET *set, const char *name, int *value){
	INT val;
	bool stat;
	stat = getValue(set, name, 0xFFFF, true, wrapper_getHighestPriorityValueByName, wrapper_returnInt, (void**)&val);
	*value = val.value;
	return stat;
}

bool paramSet_getHighestPriorityStrValueByName(const PARAM_SET *set, const char *name, char **value){
	return getValue(set, name, 0xFFFF, true, wrapper_getHighestPriorityValueByName, wrapper_returnStr, (void**)value);
}

bool paramSet_getValueCountByName(const PARAM_SET *set, const char *name, unsigned *count){
	PARAM *param = NULL;
	if(set == NULL || name == NULL) return false;

	paramSet_getParameterByName(set, name, &param);

	if(param == NULL) return false;
	*count = param->argCount;

	return true;
}

bool paramSet_isSetByName(const PARAM_SET *set, const char *name){
	return getValue(set, name, 0, false, paramSet_getValueByNameAt, NULL, NULL);
}

void paramSet_Print(const PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i =0;

	if(set == NULL) return;
	numOfElements = set->count;
	set->printInfo("Raw param set (%i)::\n", numOfElements);
	array = set->parameter;

	for(i=0; i<numOfElements;i++){
		if(array[i]->arg){
			parameter_Print(array[i], set->printInfo);
		}
	}

	return;
	}

void paramSet_PrintErrorMessages(const PARAM_SET *set){
	PARAM **array = NULL;
	PARAM *pParam = NULL;
	int i = 0;
	int numOfElements = 0;
	PARAM_VAL *value = NULL;

	if(set == NULL) return;
	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++){
		if((pParam = array[i]) != NULL){
			if(strcmp(pParam->flagName, UNKNOWN_PARAMETER_NAME)==0) continue;
			if(strcmp(pParam->flagName, TYPO_PARAMETER_NAME)==0) continue;
			value = pParam->arg;

			if(parameter_isDuplicateConflict(pParam) == true){
				set->printError("Error: Duplicate values '%s'", pParam->flagName);
				if(value->source) set->printError(" from '%s'", value->source);
				set->printError("!\n");
			}

			while(value){
				if(value->formatStatus == FORMAT_OK){
					if(value->contentStatus != PARAM_OK){
					set->printError("Content error:");
					if(value->source) set->printError(" from '%s'", value->source);
					set->printError(" %s%s '%s'. %s\n",
								strlen(pParam->flagName)>1 ? "--" : "-",
								pParam->flagName,
								value->cstr_value ? value->cstr_value : "",
								getParameterContentErrorString(value->contentStatus)
								);
					}
				}
				else{
					set->printError("Format error:");
					if(value->source) set->printError(" from '%s'", value->source);
					set->printError(" %s%s '%s'. %s\n",
							strlen(pParam->flagName)>1 ? "--" : "-",
							pParam->flagName,
							value->cstr_value ? value->cstr_value : "",
							getFormatErrorString(value->formatStatus)
							);
				}

				value = value->next;
			}
		}
	}

	return;
}

void paramSet_printUnknownParameterWarnings(const PARAM_SET *set){
	unsigned i = 0;
	unsigned count = 0;

	if(set == NULL) return;


	if(paramSet_getValueCountByName(set, UNKNOWN_PARAMETER_NAME, &count)){
		for(i = 0; i < count; i++){
			PARAM_VAL *value = NULL;
			paramSet_getValueByNameAt(set, UNKNOWN_PARAMETER_NAME, i, &value);
			if(value){
				set->printWarning("Warning: Unknown parameter '%s'", value->cstr_value);
				if(value->source) set->printWarning(" from '%s'", value->source);
				set->printWarning(".\n");
			}
		}
	}
}



static void paramSet_PrintSimilar(const char *str, const PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i =0;

	if(set == NULL) return;

	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++){
		int editDist = 0;
		unsigned lenName = 0;
		editDist = editDistance_levenshtein(array[i]->flagName, str);
		lenName = (unsigned)strlen(array[i]->flagName);

		if((editDist*100)/lenName <= 49)
			set->printWarning("Typo: Did You mean '%s%s'.\n",lenName>1 ? "--" : "-", array[i]->flagName);

		if(array[i]->flagAlias){
			editDist = editDistance_levenshtein(array[i]->flagAlias, str);
			lenName = (unsigned)strlen(array[i]->flagAlias);

			if((editDist*100)/lenName <= 33)
				set->printWarning("Typo: Did You mean '%s%s'.\n",lenName>1 ? "--" : "-", array[i]->flagAlias);
		}
	}

	return;
}

void paramSet_printIgnoredLowerPriorityWarnings(const PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i = 0;
	int highestPriority;
	if (set == NULL) return;

	numOfElements = set->count;
	array = set->parameter;

	for (i = 0; i < numOfElements; i++){
		PARAM_VAL *pValue = array[i]->arg;
		if(array[i]->isSingleHighestPriority == true){
			highestPriority = array[i]->highestPriority;

			do{
				if(pValue != NULL && pValue->priority < highestPriority){
					set->printWarning("Warning: Lower priority parameter %s%s '%s'", strlen(array[i]->flagName) > 1 ? "--" : "-",  array[i]->flagName, pValue->cstr_value);
					if(pValue->source) set->printWarning(" from '%s'", pValue->source);
					set->printWarning(" is ignored.\n");
					}
				if(pValue) pValue = pValue->next;
			}while(pValue);
		}
	}

	return;
}

void paramSet_printTypoWarnings(const PARAM_SET *set){
	unsigned i = 0;
	unsigned count = 0;

	if(set == NULL) return;

	if(paramSet_getValueCountByName(set, TYPO_PARAMETER_NAME, &count)){
		for(i=0; i<count; i++){
			PARAM_VAL *value = NULL;
			paramSet_getValueByNameAt(set, TYPO_PARAMETER_NAME, i, &value);
			if(value)
				paramSet_PrintSimilar(value->cstr_value, set);
		}
	}
}

bool paramSet_isTypos(const PARAM_SET *set){
	unsigned count = 0;
	paramSet_getValueCountByName(set, TYPO_PARAMETER_NAME, &count);
	return count > 0 ? true : false;
}