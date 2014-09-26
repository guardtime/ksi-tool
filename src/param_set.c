#include <stdio.h>		//input output
#include <string.h>		
#include <stdlib.h>		//malloc, random, int ja strn 
#include <ctype.h>
#include "getopt.h"
#include "gt_cmd_control.h"
#include "param_set.h"

#define SINGLE false
#define MULTIPLE true 

struct paramValu_st{
	char *cstr_value;		//c-string value for raw argument
	FormatStatus formatStatus;	//parameters status
	ContentStatus contentStatus;
	paramValue *next;		//link to the next value
};

/**
 * Creates a new parameter value object.
 * @param value - value as c-string. Can be NULL.
 * @param newObj - reciving pointer.
 * @return
 */
static bool paramValue_new(const char *value, paramValue **newObj){
	paramValue *tmp = NULL;
	char *tmp_cstr = NULL;
	bool status = false;
	
	if(newObj == NULL) return false;
	
	/*Create obj itself*/
	tmp = (paramValue*)malloc(sizeof(paramValue));
	if(tmp == NULL) goto cleanup;
	
	tmp->cstr_value = NULL;
	tmp->formatStatus= FORMAT_UNKNOWN_ERROR;
	tmp->contentStatus = PARAM_UNKNOWN_ERROR;
	tmp->next = NULL;
	
	/*Make a buffer for a value and copy*/
	if(value != NULL){
		tmp_cstr = (char*)malloc(strlen(value)*sizeof(char)+1);
		if(tmp_cstr == NULL) goto cleanup;
		strcpy(tmp_cstr, value);
	}
	else{
		tmp_cstr = NULL;
	}
	
	tmp->cstr_value = tmp_cstr;
	*newObj = tmp;

	tmp = NULL;
	tmp_cstr = NULL;
	status = true;
	
cleanup:

	free(tmp_cstr);
	free(tmp);	
	return status;
}

static void paramValue_recursiveFree(paramValue *rootValue){
	if(rootValue == NULL) return;
	
	if(rootValue->next != NULL){
		paramValue_recursiveFree(rootValue->next);
	}
	free(rootValue->cstr_value);
	free(rootValue);
	return;
}

static paramValue* paramValue_getElementAt(paramValue *rootValue, int at){
	paramValue *tmp = NULL;
	int i=0;
	
	if(at < 0) return NULL;
	if(rootValue == NULL) return NULL;
	
	if(at == 0) return rootValue;
	
	tmp = rootValue;
	for(i=0; i<at;i++){
		if(tmp->next == NULL) return NULL;
		tmp = tmp->next;
	}
	
	return tmp;
}


struct rawParam_st{
	char flagName;
	bool flag;									//if the varible is set
	bool isMultipleAllowed;						//If there is more than 1 parameters allowed
	int arcCount;								//count of arguments in chain
	paramValue *arg;							//argument(s)
	
	FormatStatus (*controlFormat)(const char *);	//function pointer for format control
	ContentStatus (*controlContent)(const char *);	//function pointer for content control
};

static bool rawParam_new(const char flagName, bool isMultipleAllowed,FormatStatus (*controlFormat)(const char *),ContentStatus (*controlContent)(const char *), rawParam **newObj){
	rawParam *tmp = NULL;
	
	if(newObj == NULL) return false;
	//if(controlFormat == NULL) return false;

	tmp = (rawParam*)malloc(sizeof(rawParam));
	if(tmp == NULL) return false;
	
	tmp->flagName = flagName;
	tmp->flag = false;
	tmp->isMultipleAllowed = isMultipleAllowed;
	tmp->arcCount = 0;
	tmp->arg = NULL;
	
	tmp->controlFormat = controlFormat;
	tmp->controlContent = controlContent;
	*newObj = tmp;
	tmp = NULL;
	
	return true;
}

static void rawParam_free(rawParam *obj){
	if(obj == NULL) return;
	
	if(obj->arg) paramValue_recursiveFree(obj->arg);
	free(obj);
}

/**
 * Appends a argument to the parameter and performs a format check. Parameter
 * is copied.
 * @param param - parameter where the argument is inserted
 * @param argument - argument
 * @return 
 */
static bool rawParam_addArgument(rawParam *param, const char *argument){
	paramValue *newValue = NULL;
	paramValue *pNextValue = NULL;
	bool status = false;
	
	if(param == NULL) return false;
	
	/*Create new object and control the format*/
	if(!paramValue_new(argument, &newValue)) goto cleanup;
	
	if(param->controlFormat)
		newValue->formatStatus = param->controlFormat(argument);
	if(newValue->formatStatus == FORMAT_OK && param->controlContent)
		newValue->contentStatus = param->controlContent(argument);
	
	if(param->arg == NULL){
		param->arg = newValue;
		param->arcCount = 1;
		param->flag = true;
	}
	else{
		pNextValue = param->arg;
		
		do{
			if(pNextValue->next == NULL){
				pNextValue->next = newValue;
				param->arcCount++;
				break;
			}
			pNextValue = pNextValue->next;
		}
		while(pNextValue);
	}
		
	newValue = NULL;
	status = true;
	
cleanup:
		
	paramValue_recursiveFree(newValue);

	return status;
	}

static void rawParamPrint(rawParam *param){
	paramValue *pValue = NULL;
	
	if(param == NULL) return;
	
	printf("%c == %s\n",param->flagName, param->flag ? "true" : "false");
	printf("Multi: %s\n", param->isMultipleAllowed ? "true" : "false");
	printf("N %i\n", param->arcCount);
	printf("Controll form %x cont %x\n", param->controlFormat, param->controlContent);
	
	
	printf("values:\n");
	pValue = param->arg;
	do{
		if(pValue != NULL){
			printf("p-->%6x n-->%6x:'%s' err: %2x %2x\n", (int)pValue, (int)pValue->next, pValue->cstr_value, pValue->formatStatus, pValue->contentStatus);
			pValue = pValue->next;
			}
		else
			printf("<null>\n");
	}while(pValue);
	
}


struct paramSet_st {
	rawParam **parameter;
	int count;
};

bool paramSet_new(const char *names, paramSet **set){
	paramSet *tmp = NULL;
	bool status = false;
	int paramCount = 0;
	int i=0;
	int iter=0;
	if(set == NULL) return false;
	
	while(names[i]){
		if(isalpha(names[i]))
			paramCount++;
		i++;
	}

	tmp = (paramSet*)calloc(sizeof(paramSet), 1);
	if(tmp == NULL) goto cleanup;
	
	tmp->parameter = (rawParam**)calloc(paramCount, sizeof(rawParam*));
	if(tmp->parameter == NULL) goto cleanup;

	tmp->count = paramCount;
	
	i=0;
 	while(names[i]){
		if(isalpha(names[i])){
			if(!rawParam_new(names[i],(names[i+1] == '*') ? MULTIPLE : SINGLE,NULL,NULL, &tmp->parameter[iter])) goto cleanup;
			if(tmp->parameter[iter] == NULL) goto cleanup;
			iter++;
		}
		i++;
	}

	*set = tmp;
	tmp = NULL;	
	status = true;
	
cleanup:
		
	paramSet_free(tmp);	
	return status;
}

void paramSet_free(paramSet *set){
	int numOfElements = 0;
	rawParam **array = NULL;
	int i =0;

	if(set == NULL) return;
	numOfElements = set->count;
	array = set->parameter;
	
	for(i=0; i<numOfElements;i++)
		rawParam_free(array[i]);
	
	free(set);
	return;
	}

static void rawParamSet_getParameterByName(paramSet *set, char name, rawParam **param){
	int numOfElements = 0;
	rawParam **array = NULL;
	rawParam *tmp = NULL;
	int i =0;
	
	if(set == NULL || param == NULL) return;
	*param = NULL;
	numOfElements = set->count;
	
	array = set->parameter;
	
	for(i=0; i<numOfElements; i++){
		tmp = array[i];
		if(tmp != NULL){
			if(tmp->flagName == name){
				*param = tmp;
				return;
			}
		}
	}
	return;
}

static bool rawParamSet_setParameterByName(const char *argument, char name, paramSet *set){
	rawParam *param = NULL;
	
	if(set == NULL) return false;
	
	rawParamSet_getParameterByName(set, name, &param);
	if(param == NULL) return false;
	
	if(rawParam_addArgument(param, argument))
		return true;
	else
		return false;
}

static void rawParamSet_getValueByName(paramSet *set, char name, paramValue **value){
	rawParam *tmp = NULL;

	if(set == NULL || value == NULL) return;
	*value = NULL;
	
	rawParamSet_getParameterByName(set, name, &tmp);
	if(tmp == NULL) return;
	
	*value = tmp->arg;
	
	return;	
}

static void rawParamSet_getValueByNameAt(paramSet *set, char name,int at, paramValue **value){
	paramValue *tmp = NULL;
	
	if(set == NULL || value == NULL) return;
	*value = NULL;
	
	rawParamSet_getValueByName(set,name, &tmp);
	if(tmp == NULL) return;
	
	tmp = paramValue_getElementAt(tmp, at);
	if(tmp == NULL) return;
	
	*value = tmp;

	return;
}

void paramSet_addControl(paramSet *set, char *names, FormatStatus (*controlFormat)(const char *), ContentStatus (*controlContent)(const char *)){
	int i=0;
	rawParam *tmp = NULL;
	
	while(names[i]){
		if(isalpha(names[i])){
			rawParamSet_getParameterByName(set, names[i], &tmp);
			if(tmp != NULL){
				tmp->controlFormat = controlFormat;
				tmp->controlContent = controlContent;
			}
		}
		i++;
	}
}


void paramSet_readFromcCMD(int argc, char **argv, char *definition, paramSet *rawParam){
	int c = 0;
	paramValue *test = NULL;
	int itest = 0;
	char *stest = NULL;
	
	if(rawParam == NULL) return;
	
	while (1) {
		c = getopt(argc, argv, definition);
		if (c == -1) {
			break;
		}
		rawParamSet_setParameterByName(optarg,c,rawParam);
	}
	
	return;
}

bool paramSet_isFormatOK(paramSet *set){
	rawParam **array = NULL;
	rawParam *pParam = NULL;
	int i = 0;
	int numOfElements = 0;
	paramValue *value = NULL;
	bool status = true;
	
	if(set == NULL) return false;
	numOfElements = set->count;
	array = set->parameter;
	
	for(i=0; i<numOfElements;i++){
		if((pParam = array[i]) != NULL){
			if(pParam->arcCount > 1 && !(pParam->isMultipleAllowed)){
				printf("Error: Duplicate values '%c'!\n", pParam->flagName);
				status = false;
			}
			
			value = pParam->arg;
			while(value){
				if(value->formatStatus == FORMAT_OK){
					if(value->contentStatus != PARAM_OK){
						printf("Content error: -%c '%s' err: %s\n",pParam->flagName, value->cstr_value, getParameterContentErrorString(value->contentStatus));
						status = false;
					}
				}
				else{
					printf("Format error: -%c '%s' err: %s\n",pParam->flagName, value->cstr_value, getFormatErrorString(value->formatStatus));
					status = false;
				}
				
				value = value->next;
			}
		}
	}
	
	return status;
}

void paramSet_Print(paramSet *set){
	int numOfElements = 0;
	rawParam **array = NULL;
	int i =0;
	
	if(set == NULL) return;
	numOfElements = set->count;
	printf("Raw param set (%i)::\n", numOfElements);
	array = set->parameter;
	
	for(i=0; i<numOfElements;i++){
		if(array[i]->flag){
			printf("-------\n");
			rawParamPrint(array[i]);
		}
	}
	
	return;
	}


bool paramSet_getIntValueByNameAt(paramSet *set, char name,int at, int *value){
	paramValue *tmp = NULL;
	
	if(set == NULL || value == NULL) return false;
	*value = 0;
	
	rawParamSet_getValueByNameAt(set, name, at, &tmp);
	if(tmp == NULL) return false;
	if(tmp->formatStatus != FORMAT_OK) return false;
		
	*value = atoi(tmp->cstr_value);
	return true;
}

bool paramSet_getStrValueByNameAt(paramSet *set, char name,int at, char **value){
	paramValue *tmp = NULL;
	
	if(set == NULL || value == NULL) return false;
	*value = NULL;
	
	rawParamSet_getValueByNameAt(set, name, at, &tmp);
	if(tmp == NULL) return false;
	if(tmp->formatStatus != FORMAT_OK) return false;
		
	*value = tmp->cstr_value;
	return true;
}

bool paramSet_getValueCountByName(paramSet *set, char name, int *count){
	rawParam *param = NULL;
	
	if(set == NULL) return false;
	
	rawParamSet_getParameterByName(set, name, &param);
	
	if(param == NULL) return false;
	*count = param->arcCount;
	
	return true;
}

bool paramSet_isSetByName(paramSet *set, char name){
	rawParam *param = NULL;
	
	if(set == NULL) return false;
	
	rawParamSet_getParameterByName(set, name, &param);
	
	if(param == NULL) return false;
	return param->flag;
}
