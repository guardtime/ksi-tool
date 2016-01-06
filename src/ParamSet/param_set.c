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
#include "param_set_obj_impl.h"
#include "Parameter.h"
#include "ParamValue.h"
#include "param_set.h"
#include "types.h"

#define UNKNOWN_PARAMETER_NAME "_UN_KNOWN_"
#define TYPO_PARAMETER_NAME "_TYPO_"


static char *getParametersName(const char* names, char *name, char *alias, short len, int *isMultiple, int *isSingleHighestPriority){
	char *pName = NULL;
	int i = 0;
	int isAlias = 0;
	short i_name = 0;
	short i_alias = 0;

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
			isAlias = 1;
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
			*isMultiple = 1;
		else
			*isMultiple = 0;
	}
	if(isSingleHighestPriority != NULL && pName[i] == '}'){
		if(pName[i+1] == '>')
			*isSingleHighestPriority = 1;
		else
			*isSingleHighestPriority = 0;
	}


	name[i_name] = 0;
	if(alias)
		alias[i_alias] = 0;

	return &pName[i];
}

int PARAM_SET_new(const char *names,
		int (*printInfo)(const char*, ...), int (*printWarnings)(const char*, ...), int (*printErrors)(const char*, ...),
		PARAM_SET **set){
	int res;
	PARAM_SET *tmp = NULL;
	PARAM **tmp_param = NULL;
	const char *pName = NULL;
	int paramCount = 0;
	int i = 0;
	char mem = 0;
	char buf[1024];
	char alias[1024];
	int isMultiple = 0;
	int isSingleHighestPriority = 0;

	if (set == NULL || names == NULL
			|| printInfo == NULL|| printWarnings == NULL || printErrors == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	while(names[i]){
		if(names[i] == '{') mem = names[i];
		else if(mem == '{' && names[i] == '}'){
			paramCount++;
			mem = 0;
		}
		i++;
	}
	/* Add extra count for Unknowns and Typos. */
	paramCount += 2;

	tmp = (PARAM_SET*)malloc(sizeof(PARAM_SET));
	if(tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp_param = (PARAM**)calloc(paramCount, sizeof(PARAM*));
	if(tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->count = paramCount;
	tmp->printError = printErrors;
	tmp->printInfo = printInfo;
	tmp->printWarning = printWarnings;
	tmp->parameter = tmp_param;

	tmp_param = NULL;

	i = 0;
	pName = names;
	while((pName = getParametersName(pName, buf, alias, sizeof(buf), &isMultiple, &isSingleHighestPriority)) != NULL){
		res = PARAM_new(buf, alias[0] ? alias : NULL, isMultiple, isSingleHighestPriority, NULL, NULL, NULL, &tmp->parameter[i]);
		if(res != PST_OK) goto cleanup;

		i++;
	}

	res = PARAM_new(UNKNOWN_PARAMETER_NAME, NULL, 1, 0,  NULL, NULL, NULL, &tmp->parameter[i++]);
	if(res != PST_OK) goto cleanup;

	res = PARAM_new(TYPO_PARAMETER_NAME, NULL, 1, 0, NULL, NULL, NULL, &tmp->parameter[i]);
	if(res != PST_OK) goto cleanup;

	*set = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	PARAM_SET_free(tmp);
	free(tmp_param);

	return res;
}

void PARAM_SET_free(PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i =0;

	if(set == NULL) return;
	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++)
		PARAM_free(array[i]);
	free(set->parameter);

	free(set);
	return;
}

static int param_set_getParameterByName(const PARAM_SET *set, const char *name, PARAM **param){
	int res = 0;
	PARAM *parameter = NULL;
	PARAM *tmp = NULL;
	int i = 0;

	if (set == NULL || param == NULL || name == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}


	for(i = 0; i < set->count; i++) {
		parameter = set->parameter[i];
		if(parameter != NULL){
			if(strcmp(parameter->flagName, name) == 0 || (parameter->flagAlias && strcmp(parameter->flagAlias, name) == 0)) {
				tmp = parameter;
				break;
			}
		}
	}

	if (tmp == NULL) {
		res = PST_PARAMETER_NOT_FOUND;
		goto cleanup;
	}

	*param = parameter;
	res = PST_OK;

cleanup:

	return res;
}

int PARAM_SET_add(PARAM_SET *set, const char *name, const char *value, const char *source, int priority) {
	int res;
	PARAM *param = NULL;

	if (set == NULL || name == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;;
	}

	res = param_set_getParameterByName(set, name, &param);
	if(res != PST_OK) goto cleanup;

	res = PARAM_addArgument(param, value, source, priority);
	if(res != PST_OK) goto cleanup;

	res = PST_OK;

cleanup:

	return res;
}

int PARAM_SET_clear(PARAM_SET *set, const char *name){
	int res;
	PARAM *tmp = NULL;
	PARAM_VAL *rem = NULL;
	int i =0;

	if (set == NULL || name == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = param_set_getParameterByName(set, name, &tmp);
	if (res != PST_OK) goto cleanup;

	tmp->argCount = 0;
	if(tmp->arg) PARAM_VAL_free(tmp->arg);
	res = PST_OK;

cleanup:

	return res;
}

static int param_set_getValue(const PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, PARAM_VAL **value){
	int res;
	PARAM_VAL *tmp = NULL;
	PARAM *parameter = NULL;

	if (set == NULL || value == NULL || name == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = param_set_getParameterByName(set, name, &parameter);
	if (res != PST_OK) goto cleanup;

	res = PARAM_getValue(parameter, name, source, prio, at, &tmp);
	if (res != PST_OK) goto cleanup;

	*value = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	return res;
}

int (*PARAM_SET_getErrorPrinter(PARAM_SET *set))(const char*, ...) {
	return set->printError;
}

int (*PARAM_SET_getWarningPrinter(PARAM_SET *set))(const char*, ...) {
	return set->printWarning;
}

void PARAM_SET_addControl(PARAM_SET *set, const char *names,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		int (*convert)(const char*, char*, unsigned)){
	int res;
	PARAM *tmp = NULL;
	const char *pName = NULL;
	char buf[256];

	pName = names;
	while((pName = getParametersName(pName,buf, NULL, sizeof(buf), NULL, NULL)) != NULL){
		res = param_set_getParameterByName(set, buf, &tmp);
		if (res == PST_OK) {
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

static int param_set_couldItBeTypo(const char *str, const PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i =0;

	if(set == NULL) return 0;
	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++){
		unsigned lenName = 0;
		int editDist = 0;

		editDist = editDistance_levenshtein(array[i]->flagName, str);
		lenName = (unsigned)strlen(array[i]->flagName);

		if((editDist*100)/lenName <= 49)
			return 1;

		if(array[i]->flagAlias){
			editDist = editDistance_levenshtein(array[i]->flagAlias, str);
			lenName = (unsigned)strlen(array[i]->flagAlias);

			if((editDist*100)/lenName <= 49)
				return 1;
		}
	}

	return 0;
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
static void param_set_addRawParameter(const char *param, const char *arg, const char *source, PARAM_SET *set, int priority){
	const char *flag = NULL;
	unsigned len;

	len = (unsigned)strlen(param);
	if(param[0] == '-' && param[1] != 0){
		flag = param+(param[1] == '-' ? 2 : 1);

		/*It is long parameter*/
		if(strncmp("--", param, 2)==0 && len >= 3){
			if(param[3] == 0){
				PARAM_SET_add(set, TYPO_PARAMETER_NAME, flag, source, PST_PRIORITY_VALID_BASE);
				if(arg)
					PARAM_SET_add(set, param_set_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, arg, source, PST_PRIORITY_VALID_BASE);
				return;
			}

			if (PARAM_SET_add(set, flag, arg, source, priority) != PST_OK){
				PARAM_SET_add(set, param_set_couldItBeTypo(flag, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, flag, source, PST_PRIORITY_VALID_BASE);
				if(arg)
					PARAM_SET_add(set, param_set_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, arg, source, PST_PRIORITY_VALID_BASE);
			}
		}
		/*It is short parameter*/
		else if(param[0] == '-' && len == 2){
			if (PARAM_SET_add(set, flag, arg, source, priority) != PST_OK) {
				PARAM_SET_add(set, UNKNOWN_PARAMETER_NAME, flag, source, PST_PRIORITY_VALID_BASE);
				if(arg)
					PARAM_SET_add(set, param_set_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, arg, source, PST_PRIORITY_VALID_BASE);
			}
		}
		/*It is bunch of flags*/
		else{
			char str_flg[2] = {255,0};
			int itr = 0;

			if(arg)
				PARAM_SET_add(set, param_set_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME, arg, source, PST_PRIORITY_VALID_BASE);

			while((str_flg[0] = flag[itr++]) != '\0'){
				if (PARAM_SET_add(set, str_flg, NULL, source, priority) != PST_OK) {
					if(param_set_couldItBeTypo(flag, set)){
						PARAM_SET_add(set, TYPO_PARAMETER_NAME, flag, source, PST_PRIORITY_VALID_BASE);
						break;
					}
					else
						PARAM_SET_add(set, UNKNOWN_PARAMETER_NAME, str_flg, source, PST_PRIORITY_VALID_BASE);
				}
			}

		}
	}
	else{
		PARAM_SET_add(set, UNKNOWN_PARAMETER_NAME, param, source, PST_PRIORITY_VALID_BASE);
	}
}

void PARAM_SET_readFromFile(const char *fname, PARAM_SET *set, int priority){
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
			param_set_addRawParameter(flag, arg, fname, set, priority);
		else
			param_set_addRawParameter(line,NULL, fname, set, priority);
	}

cleanup:

	if(file) fclose(file);
	return;
}

void PARAM_SET_readFromCMD(int argc, char **argv, PARAM_SET *set, int priority){
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

		param_set_addRawParameter(tmp, arg, NULL, set, priority);
	}

	return;
}

int PARAM_SET_isFormatOK(const PARAM_SET *set){
	PARAM **array = NULL;
	PARAM *pParam = NULL;
	int i = 0;
	int numOfElements = 0;
	PARAM_VAL *value = NULL;

	if(set == NULL) return 0;
	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++){
		if((pParam = array[i]) != NULL){
			if(strcmp(pParam->flagName, UNKNOWN_PARAMETER_NAME)==0) continue;
			if(strcmp(pParam->flagName, TYPO_PARAMETER_NAME)==0) continue;

			/*Control duplicate conflicts*/
			if (PARAM_isDuplicateConflict(pParam)) {
				return 0;
			}

			value = pParam->arg;
			while(value){
				if(value->formatStatus == FORMAT_OK){
					if(value->contentStatus != PARAM_OK){
						return 0;
					}
				}
				else{
					return 0;
				}

				value = value->next;
			}
		}
	}

	return 1;
}

static int getValue(const PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, int controlFormat,
	int (*getter)(const PARAM_SET *, const char *, const char *, int, unsigned , PARAM_VAL **value),
	int (*convert)(char*, void**),
	void **value){
	int res;
	PARAM_VAL *tmp = NULL;

	if(set == NULL || name == NULL || getter == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}
	/*Get parameter->value at position*/
	res = getter(set, name, source, prio, at, &tmp);
	if (res != PST_OK) goto cleanup;
	if (tmp->formatStatus != FORMAT_OK && controlFormat) {
		res = PST_PARAMETER_INVALID_FORMAT;
		goto cleanup;
	}
	/*Convert value->string into format*/
	if(convert && value) {
		res = convert(tmp->cstr_value, value);
		if (res != PST_OK) goto cleanup;
	}

	res = PST_OK;

cleanup:
	return res;
}

int wrapper_returnStr(char* str, void** obj){
	*obj = (void*)str;
	return PST_OK;
}

typedef struct INT_st {int value;} INT;

int wrapper_returnInt(char* str,  void** obj){
	((INT*)obj)->value = atoi(str);
	return PST_OK;
}
//const PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, PARAM_VAL **value
int PARAM_SET_getIntValue(const PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, int *value) {
	INT val;
	int res;
	res = getValue(set, name, source, prio, at, 1, param_set_getValue, wrapper_returnInt, (void**)&val);
	*value = val.value;
	return res;
}

int PARAM_SET_getStrValue(const PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, char **value) {
	return getValue(set, name, source, prio, at, 1, param_set_getValue, wrapper_returnStr, (void**)value);
}

int PARAM_SET_getValueCountByName(const PARAM_SET *set, const char *name, unsigned *count){
	int res;
	PARAM *param = NULL;

	if(set == NULL || name == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = param_set_getParameterByName(set, name, &param);
	if(res != PST_OK) goto cleanup;

	*count = param->argCount;
	res = PST_OK;

cleanup:

	return res;
}

int PARAM_SET_isSetByName(const PARAM_SET *set, const char *name){
	return getValue(set, name, NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, 0, param_set_getValue, NULL, NULL) == PST_OK ? 1 : 0;
}

void PARAM_SET_Print(const PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i =0;

	if(set == NULL) return;
	numOfElements = set->count;
	set->printInfo("Raw param set (%i)::\n", numOfElements);
	array = set->parameter;

	for(i=0; i<numOfElements;i++){
		if(array[i]->arg){
			PARAM_print(array[i], set->printInfo);
		}
	}

	return;
	}

void PARAM_SET_PrintErrorMessages(const PARAM_SET *set){
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

			if (PARAM_isDuplicateConflict(pParam)) {
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

void PARAM_SET_printUnknownParameterWarnings(const PARAM_SET *set){
	unsigned i = 0;
	unsigned count = 0;

	if(set == NULL) return;


	if(PARAM_SET_getValueCountByName(set, UNKNOWN_PARAMETER_NAME, &count) == PST_OK){
		for(i = 0; i < count; i++){
			PARAM_VAL *value = NULL;
			param_set_getValue(set, UNKNOWN_PARAMETER_NAME, NULL, PST_PRIORITY_NONE, i, &value);
			if(value){
				set->printWarning("Warning: Unknown parameter '%s'", value->cstr_value);
				if(value->source) set->printWarning(" from '%s'", value->source);
				set->printWarning(".\n");
			}
		}
	}
}



static void param_set_PrintSimilar(const char *str, const PARAM_SET *set){
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

void PARAM_SET_printIgnoredLowerPriorityWarnings(const PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i = 0;
	int highestPriority;
	if (set == NULL) return;

	numOfElements = set->count;
	array = set->parameter;

	for (i = 0; i < numOfElements; i++){
		PARAM_VAL *pValue = array[i]->arg;
		if (array[i]->isSingleHighestPriority) {
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

void PARAM_SET_printTypoWarnings(const PARAM_SET *set){
	unsigned i = 0;
	unsigned count = 0;

	if(set == NULL) return;

	if(PARAM_SET_getValueCountByName(set, TYPO_PARAMETER_NAME, &count) == PST_OK){
		for(i=0; i<count; i++){
			PARAM_VAL *value = NULL;
			param_set_getValue(set, TYPO_PARAMETER_NAME, NULL, PST_PRIORITY_NONE, i, &value);
			if(value)
				param_set_PrintSimilar(value->cstr_value, set);
		}
	}
}

int PARAM_SET_isTypos(const PARAM_SET *set){
	unsigned count = 0;
	PARAM_SET_getValueCountByName(set, TYPO_PARAMETER_NAME, &count);
	return count > 0 ? 1 : 0;
}