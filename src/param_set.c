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

#define SINGLE false
#define MULTIPLE true

#define UNKNOWN_PARAMETER_NAME "_UN_KNOWN_"
#define TYPO_PARAMETER_NAME "_TYPO_"

/*TODO: refactor priority system*/

char *new_string(const char *str){
	char *tmp = NULL;
	if(str == NULL) return NULL;
	tmp = (char*)malloc(strlen(str)*sizeof(char)+1);
	if(tmp == NULL) return NULL;
	return strcpy(tmp, str);
}

typedef struct paramValu_st{
	char *cstr_value;		//c-string value for raw argument
	char *source;			//optional c-string describing the source of the value;
	int priority;
	FormatStatus formatStatus;	//parameters status
	ContentStatus contentStatus;
	struct paramValu_st *next;		//link to the next value
} paramValue;

/**
 * Creates a new parameter value object.
 * @param value - value as c-string. Can be NULL.
 * @param source - describes the source e.g. file name or environment variable. Can be NULL.
 * @param priority - priority of the parameter.
 * @param newObj - receiving pointer.
 * @return true on success, false otherwise.
 */
static bool paramValue_new(const char *value, const char* source, int priority, paramValue **newObj){
	paramValue *tmp = NULL;
	char *tmp_value = NULL;
	char *tmp_source = NULL;
	bool status = false;

	if(newObj == NULL) return false;

	/*Create obj itself*/
	tmp = (paramValue*)malloc(sizeof(paramValue));
	if(tmp == NULL) goto cleanup;

	tmp->cstr_value = NULL;
	tmp->source = NULL;
	tmp->formatStatus= FORMAT_UNKNOWN_ERROR;
	tmp->contentStatus = PARAM_UNKNOWN_ERROR;
	tmp->next = NULL;
	tmp->priority = priority;

	if(value != NULL){
		tmp_value = new_string(value);
		if(tmp_value == NULL) goto cleanup;
	}

	if(source != NULL){
		tmp_source = new_string(source);
		if(tmp_source == NULL) goto cleanup;
	}

	tmp->cstr_value = tmp_value;
	tmp->source = tmp_source;
	*newObj = tmp;

	tmp = NULL;
	tmp_value = NULL;
	tmp_source = NULL;
	status = true;

cleanup:

	free(tmp_value);
	free(tmp_source);
	free(tmp);
	return status;
}

static void paramValue_recursiveFree(paramValue *rootValue){
	if(rootValue == NULL) return;

	if(rootValue->next != NULL)
		paramValue_recursiveFree(rootValue->next);

	free(rootValue->cstr_value);
	free(rootValue->source);
	free(rootValue);
}

static paramValue* paramValue_getElementAt(paramValue *rootValue, unsigned at){
	paramValue *tmp = NULL;
	unsigned i=0;

	if(rootValue == NULL) return NULL;
	if(at == 0) return rootValue;

	tmp = rootValue;
	for(i=0; i<at;i++){
		if(tmp->next == NULL) return NULL;
		tmp = tmp->next;
	}

	return tmp;
}


static paramValue* paramValue_getFirstHighestPriorityValue(paramValue *rootValue){
	paramValue *pValue = NULL;
	paramValue *master = NULL;

	if(rootValue == NULL) return NULL;

	pValue = rootValue;
	master = pValue;
	do{
		if (pValue != NULL){
			if(pValue->priority > master->priority)
				master = pValue;
			pValue = pValue->next;
		}
	}while (pValue);

	return master;
}


typedef struct param_st{
	char *flagName;
	char *flagAlias;
	bool isMultipleAllowed;							//If there is more than 1 parameters allowed
	bool isSingleHighestPriority;					//Allows to have more than one value but a single highest priority vale.
	int highestPriority;							//Highest priority of inserted values
	int argCount;									//count of arguments in chain

	paramValue *arg;								//argument(s)

	FormatStatus (*controlFormat)(const char*);		//function pointer for format control
	ContentStatus (*controlContent)(const char*);	//function pointer for content control
	bool (*convert)(const char*, char*, unsigned);	//function pointer to convert the content
} parameter;

static void parameter_free(parameter *obj){
	if(obj == NULL) return;
	free(obj->flagName);
	free(obj->flagAlias);
	if(obj->arg) paramValue_recursiveFree(obj->arg);
	free(obj);
}

static bool parameter_new(const char *flagName,const char *flagAlias, bool isMultipleAllowed, bool isSingleHighestPriority,
		FormatStatus (*controlFormat)(const char *),
		ContentStatus (*controlContent)(const char *),
		bool (*convert)(const char*, char*, unsigned),
		parameter **newObj){
	parameter *tmp = NULL;

	if(newObj == NULL || flagName == NULL) return false;

	tmp = (parameter*)malloc(sizeof(parameter));
	if(tmp == NULL) goto cleanup;

	tmp->flagName = NULL;
	tmp->flagAlias = NULL;
	tmp->arg = NULL;

	tmp->flagName = new_string(flagName);
	if(tmp->flagName == NULL) goto cleanup;

	if(flagAlias){
		tmp->flagAlias = new_string(flagAlias);
		if(tmp->flagAlias == NULL) goto cleanup;
	}

	tmp->controlFormat = controlFormat;
	tmp->controlContent = controlContent;
	tmp->convert = convert;
	tmp->isMultipleAllowed = isMultipleAllowed;
	tmp->isSingleHighestPriority = isSingleHighestPriority;
	tmp->highestPriority = 0;
	tmp->argCount = 0;

	*newObj = tmp;
	tmp = NULL;

cleanup:

	parameter_free(tmp);
	return true;
}

/**
 * Appends a argument to the parameter and performs a format check. Parameter
 * is copied.
 * @param param - parameter where the argument is inserted.
 * @param argument - argument.
 * @param source - describes the source e.g. file name or environment variable. Can be NULL.
 * @param priority - priority of the parameter.

 * @return Return true if successful, false otherwise.
 */
static bool parameter_addArgument(parameter *param, const char *argument, const char* source, int priority){
	paramValue *newValue = NULL;
	paramValue *pLastValue = NULL;
	bool status = false;
	const char *arg = NULL;
	char buf[1024];
	if(param == NULL) return false;

	/*If conversion function exists convert the argument*/
	if(param->convert)
		arg = param->convert(argument, buf, sizeof(buf)) ? buf : arg;
	else
		arg = argument;

	/*Create new object and control the format*/
	if(!paramValue_new(arg, source, priority, &newValue)) goto cleanup;

	if(param->controlFormat)
		newValue->formatStatus = param->controlFormat(arg);
	if(newValue->formatStatus == FORMAT_OK && param->controlContent)
		newValue->contentStatus = param->controlContent(arg);

	if(param->arg == NULL){
		param->arg = newValue;
	}
	else{
		pLastValue = (paramValue*)paramValue_getElementAt(param->arg, param->argCount-1);
		if(pLastValue == NULL || pLastValue->next != NULL) goto cleanup;
		pLastValue->next = newValue;
	}
	param->argCount++;

	if(param->highestPriority < priority)
		param->highestPriority = priority;

	newValue = NULL;
	status = true;

cleanup:

	paramValue_recursiveFree(newValue);

	return status;
	}

static void parameter_Print(const parameter *param, int (*print)(const char*, ...)){
	paramValue *pValue = NULL;

	if(param == NULL) return;

	print("%s\n", param->flagName);
	pValue = param->arg;
	do{
		if(pValue != NULL){
			print("  '%s' p:%i  err: %2x %2x\n", pValue->cstr_value, pValue->priority, pValue->formatStatus, pValue->contentStatus);
			pValue = pValue->next;
			}
		else
			print("  <null>\n");
	}while(pValue);

}
/*
 * Duplicate conflict is defined when flag \isMultipleAllowed is not set and value count
 * is > 1 OR flag \isSingleHighestPriority is set and there is more than one highest
 * priority values present.
 */
static bool parameter_isDuplicateConflict(const parameter *param){
	int highestPriority = 0;
	int count = 0;
	paramValue *value = NULL;

	if(param->argCount > 1 && !(param->isMultipleAllowed || param->isSingleHighestPriority)){
		return true;
	}
	else if(param->isSingleHighestPriority){
		value = param->arg;
		do{
			if (value != NULL){
				if(value->priority > highestPriority){
					count = 1;
					highestPriority = value->priority;
				}
				else if (value->priority == highestPriority){
					count++;
				}
				value = value->next;
			}
		}while (value);
		if(count > 1)
			return true;
	}
	return false;
}


struct paramSet_st {
	parameter **parameter;
	int count;
	int (*printInfo)(const char*, ...);
	int (*printWarning)(const char*, ...);
	int (*printError)(const char*, ...);
};

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

static void paramSet_getParameterByName(const paramSet *set, const char *name, parameter **param){
	int numOfElements = 0;
	parameter **array = NULL;
	parameter *tmp = NULL;
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

bool paramSet_priorityAppendParameterByName(const char *name, const char *value, const char *source, int priority, paramSet *set){
	parameter *param = NULL;

	if(set == NULL || name == NULL) return false;

	paramSet_getParameterByName(set, name, &param);
	if(param == NULL) return false;

	return parameter_addArgument(param, value, source, priority);
}

bool paramSet_appendParameterByName(const char *name, const char *value, const char *source, paramSet *set){
	return paramSet_priorityAppendParameterByName(name, value, source, 0, set);
}

void paramSet_removeParameterByName(paramSet *set, const char *name){
	int numOfElements = 0;
	parameter **array = NULL;
	parameter *tmp = NULL;
	int i =0;

	if(set == NULL || name == NULL) return;
	numOfElements = set->count;

	array = set->parameter;


	for(i=0; i<numOfElements; i++){
		tmp = array[i];
		if(tmp != NULL){
			if(strcmp(tmp->flagName, name) == 0 || (tmp->flagAlias && strcmp(tmp->flagAlias, name) == 0)){
				tmp->argCount = 0;
				if(tmp->arg) paramValue_recursiveFree(tmp->arg);
				tmp->arg = NULL;
				return;
			}
		}
	}
	return;
}

static void paramSet_getFirstValueByName(const paramSet *set, const char *name, paramValue **value){
	parameter *tmp = NULL;

	if(set == NULL || value == NULL || name == NULL) return;
	*value = NULL;

	paramSet_getParameterByName(set, name, &tmp);
	if(tmp == NULL) return;

	*value = tmp->arg;

	return;
}

static void paramSet_getHighestPriorityValueByName(const paramSet *set, const char *name, paramValue **value){
	parameter *tmp = NULL;

	if(set == NULL || value == NULL || name == NULL) return;
	*value = NULL;

	paramSet_getParameterByName(set, name, &tmp);
	if(tmp == NULL) return;

	*value = (paramValue*)paramValue_getFirstHighestPriorityValue(tmp->arg);
	return;
}

static void paramSet_getValueByNameAt(const paramSet *set, const char *name, unsigned at, paramValue **value){
	paramValue *tmp = NULL;

	if(set == NULL || value == NULL || name == NULL) return;
	*value = NULL;

	paramSet_getFirstValueByName(set,name, &tmp);
	if(tmp == NULL) return;

	tmp = paramValue_getElementAt(tmp, at);
	*value = tmp;

	return;
}


bool paramSet_new(const char *names,
		int (*printInfo)(const char*, ...), int (*printWarnings)(const char*, ...), int (*printErrors)(const char*, ...),
		paramSet **set){
	paramSet *tmp = NULL;
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

	tmp = (paramSet*)calloc(sizeof(paramSet), 1);
	if(tmp == NULL) goto cleanup;
	tmp->parameter = (parameter**)calloc(paramCount, sizeof(parameter*));
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


int (*paramSet_getErrorPrinter(paramSet *set))(const char*, ...) {
	return set->printError;
}

int (*paramSet_getWarningPrinter(paramSet *set))(const char*, ...) {
	return set->printWarning;
}

void paramSet_free(paramSet *set){
	int numOfElements = 0;
	parameter **array = NULL;
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

void paramSet_addControl(paramSet *set, const char *names,
		FormatStatus (*controlFormat)(const char *),
		ContentStatus (*controlContent)(const char *),
		bool (*convert)(const char*, char*, unsigned)){
	parameter *tmp = NULL;
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

static bool paramSet_couldItBeTypo(const char *str, const paramSet *set){
	int numOfElements = 0;
	parameter **array = NULL;
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
static void paramSet_addRawParameter(const char *param, const char *arg, const char *source, paramSet *set, int priority){
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

void paramSet_readFromFile(const char *fname, paramSet *set, int priority){
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

void paramSet_readFromCMD(int argc, char **argv, paramSet *set, int priority){
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

bool paramSet_isFormatOK(const paramSet *set){
	parameter **array = NULL;
	parameter *pParam = NULL;
	int i = 0;
	int numOfElements = 0;
	paramValue *value = NULL;

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

static bool getValue(const paramSet *set, const char *name, unsigned at, bool controlFormat,
	void (*getter)(const paramSet*, const char*, unsigned, paramValue**),
	void (*convert)(char*, void**),
	void **value){

	paramValue *tmp = NULL;

	if(set == NULL || name == NULL || getter == NULL) return false;
	/*Get parameter->value at position*/
	getter(set, name, at, &tmp);
	if (tmp == NULL) return false;
	if (tmp->formatStatus != FORMAT_OK && controlFormat) return false;
	/*Convert value->string into format*/
	if(convert && value) convert(tmp->cstr_value, value);
	return true;
}

static void wrapper_getHighestPriorityValueByName(const paramSet *set, const char *name, unsigned at, paramValue **value){
	paramSet_getHighestPriorityValueByName(set, name, value);
}

void wrapper_returnStr(char* str, void** obj){
	*obj = (void*)str;
}

typedef struct INT_st {int value;} INT;

void wrapper_returnInt(char* str,  void** obj){
	((INT*)obj)->value = atoi(str);
}

bool paramSet_getIntValueByNameAt(const paramSet *set, const char *name, unsigned at, int *value){
	INT val;
	bool stat;
	stat = getValue(set, name, at, true, paramSet_getValueByNameAt, wrapper_returnInt, (void**)&val);
	*value = val.value;
	return stat;
}

bool paramSet_getStrValueByNameAt(const paramSet *set, const char *name, unsigned at, char **value){
	return getValue(set, name, at, true, paramSet_getValueByNameAt, wrapper_returnStr, (void**)value);
}

bool paramSet_getHighestPriorityIntValueByName(const paramSet *set, const char *name, int *value){
	INT val;
	bool stat;
	stat = getValue(set, name, 0xFFFF, true, wrapper_getHighestPriorityValueByName, wrapper_returnInt, (void**)&val);
	*value = val.value;
	return stat;
}

bool paramSet_getHighestPriorityStrValueByName(const paramSet *set, const char *name, char **value){
	return getValue(set, name, 0xFFFF, true, wrapper_getHighestPriorityValueByName, wrapper_returnStr, (void**)value);
}

bool paramSet_getValueCountByName(const paramSet *set, const char *name, unsigned *count){
	parameter *param = NULL;
	if(set == NULL || name == NULL) return false;

	paramSet_getParameterByName(set, name, &param);

	if(param == NULL) return false;
	*count = param->argCount;

	return true;
}

bool paramSet_isSetByName(const paramSet *set, const char *name){
	return getValue(set, name, 0, false, paramSet_getValueByNameAt, NULL, NULL);
}

void paramSet_Print(const paramSet *set){
	int numOfElements = 0;
	parameter **array = NULL;
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

void paramSet_PrintErrorMessages(const paramSet *set){
	parameter **array = NULL;
	parameter *pParam = NULL;
	int i = 0;
	int numOfElements = 0;
	paramValue *value = NULL;

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
				parameter_Print(pParam, set->printError);
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

void paramSet_printUnknownParameterWarnings(const paramSet *set){
	unsigned i = 0;
	unsigned count = 0;

	if(set == NULL) return;


	if(paramSet_getValueCountByName(set, UNKNOWN_PARAMETER_NAME, &count)){
		for(i = 0; i < count; i++){
			paramValue *value = NULL;
			paramSet_getValueByNameAt(set, UNKNOWN_PARAMETER_NAME, i, &value);
			if(value){
				set->printWarning("Warning: Unknown parameter '%s'", value->cstr_value);
				if(value->source) set->printWarning(" from '%s'", value->source);
				set->printWarning(".\n");
			}
		}
	}
}



static void paramSet_PrintSimilar(const char *str, const paramSet *set){
	int numOfElements = 0;
	parameter **array = NULL;
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

void paramSet_printIgnoredLowerPriorityWarnings(const paramSet *set){
	int numOfElements = 0;
	parameter **array = NULL;
	int i = 0;
	int highestPriority;
	if (set == NULL) return;

	numOfElements = set->count;
	array = set->parameter;

	for (i = 0; i < numOfElements; i++){
		paramValue *pValue = array[i]->arg;
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

void paramSet_printTypoWarnings(const paramSet *set){
	unsigned i = 0;
	unsigned count = 0;

	if(set == NULL) return;

	if(paramSet_getValueCountByName(set, TYPO_PARAMETER_NAME, &count)){
		for(i=0; i<count; i++){
			paramValue *value = NULL;
			paramSet_getValueByNameAt(set, TYPO_PARAMETER_NAME, i, &value);
			if(value)
				paramSet_PrintSimilar(value->cstr_value, set);
		}
	}
}

bool paramSet_isTypos(const paramSet *set){
	unsigned count = 0;
	paramSet_getValueCountByName(set, TYPO_PARAMETER_NAME, &count);
	return count > 0 ? true : false;
}