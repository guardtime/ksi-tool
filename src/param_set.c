#include <stdio.h>		//input output
#include <string.h>		
#include <stdlib.h>		//malloc, random, int ja strn 
#include <ctype.h>
#include <stdio.h>
#include "gt_cmd_control.h"
#include "param_set.h"
#include "task_def.h"

#define SINGLE false
#define MULTIPLE true 

#define UNKNOWN_PARAMETER_NAME "_UN_KNOWN_"

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



typedef struct param_st{
	char *flagName;
	bool flag;									//if the varible is set
	bool isMultipleAllowed;						//If there is more than 1 parameters allowed
	int arcCount;								//count of arguments in chain
	paramValue *arg;							//argument(s)
	
	FormatStatus (*controlFormat)(const char *);	//function pointer for format control
	ContentStatus (*controlContent)(const char *);	//function pointer for content control
} parameter;

static void parameter_free(parameter *obj){
	if(obj == NULL) return;
	
	free(obj->flagName);
	if(obj->arg) paramValue_recursiveFree(obj->arg);
	free(obj);
}

static bool parameter_new(const char *flagName, bool isMultipleAllowed,FormatStatus (*controlFormat)(const char *),ContentStatus (*controlContent)(const char *), parameter **newObj){
	parameter *tmp = NULL;
	
	if(newObj == NULL || flagName == NULL) return false;

	tmp = (parameter*)malloc(sizeof(parameter));
	if(tmp == NULL) goto cleanup;
	
	tmp->flagName = (char*)malloc(sizeof(char)*strlen(flagName)+1);
	if(tmp->flagName == NULL) goto cleanup;
	
	strcpy(tmp->flagName, flagName);
	
	tmp->flag = false;
	tmp->isMultipleAllowed = isMultipleAllowed;
	tmp->arcCount = 0;
	tmp->arg = NULL;
	
	tmp->controlFormat = controlFormat;
	tmp->controlContent = controlContent;
	*newObj = tmp;
	tmp = NULL;

cleanup:

	parameter_free(tmp);
	return true;
}

/**
 * Appends a argument to the parameter and performs a format check. Parameter
 * is copied.
 * @param param - parameter where the argument is inserted
 * @param argument - argument
 * @return 
 */
static bool parameter_addArgument(parameter *param, const char *argument){
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

static void parameter_Print(parameter *param){
	paramValue *pValue = NULL;
	
	if(param == NULL) return;
	
	printf("%s\n", param->flagName);
	pValue = param->arg;
	do{
		if(pValue != NULL){
			printf("  '%s'  err: %2x %2x\n", pValue->cstr_value, pValue->formatStatus, pValue->contentStatus);
			pValue = pValue->next;
			}
		else
			printf("  <null>\n");
	}while(pValue);
	
}


struct paramSet_st {
	parameter **parameter;
	int count;
};

char *getParametersName(const char* names, char *buf, short len, bool *isMultiple){
	char *pName = NULL;
	int i = 0;
	
	if(names == NULL || buf == NULL) return NULL;
	if(names[0] == 0) return NULL;
	
	pName=strchr(names, '{');
	if(pName == NULL) return NULL;
	pName++;
	while(pName[i] != '}' && pName[i] != 0){
		if(len-1 <= i){
			return NULL;
		}
		buf[i] = pName[i];
		i++;
	}
	if(isMultiple != NULL && pName[i] == '}'){
		if(pName[i+1] == '*')
			*isMultiple = true;
		else
			*isMultiple = false;
	}
	
	
	buf[i] = 0;
	return &pName[i];
}

bool paramSet_new(const char *names, paramSet **set){
	paramSet *tmp = NULL;
	bool status = false;
	char *copyNames = NULL;
	const char *pName = NULL;
	int paramCount = 0;
	int i = 0;
	int iter = 0;
	char mem = 0;
	char buf[256];
	short len = 0;
	bool isMultiple = false; 
	
	if(set == NULL || names == NULL) return false;
	
	while(names[i]){
		if(names[i] == '{') mem = names[i]; 
		else if(mem == '{' && names[i] == '}'){
			paramCount++; 
			mem = 0;
		}
		i++;
	}

	tmp = (paramSet*)calloc(sizeof(paramSet), 1);
	if(tmp == NULL) goto cleanup;
	paramCount++;
	tmp->parameter = (parameter**)calloc(paramCount, sizeof(parameter*));
	if(tmp->parameter == NULL) goto cleanup;

	tmp->count = paramCount;

	len = sizeof(buf);
	pName = names;
	
	while((pName = getParametersName(pName,buf, len, &isMultiple)) != NULL){
//		printf(">>> '%s' : %i\n", buf, isMultiple);
		if(!parameter_new(buf,isMultiple,NULL,NULL, &tmp->parameter[iter]))
			goto cleanup;
		iter++;
	}
	
	parameter_new(UNKNOWN_PARAMETER_NAME,MULTIPLE,NULL,NULL, &tmp->parameter[iter]);
	
	*set = tmp;
	tmp = NULL;	
	status = true;
	
cleanup:
				
	paramSet_free(tmp);	
	return status;
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
	
	free(set);
	return;
	}

static void paramSet_getParameterByName(paramSet *set, char *name, parameter **param){
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
			if(strcmp(tmp->flagName, name) == 0){
				*param = tmp;
				return;
			}
		}
	}
	return;
}

static bool paramSet_appendParameterByName(const char *argument, char *name, paramSet *set){
	parameter *param = NULL;
	
	if(set == NULL || name == NULL) return false;
	
	paramSet_getParameterByName(set, name, &param);
	if(param == NULL) return false;
	
	if(parameter_addArgument(param, argument))
		return true;
	else
		return false;
}

void paramSet_removeParameterByName(paramSet *set, char *name){
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
			if(strcmp(tmp->flagName, name) == 0){
				parameter_free(tmp);
				array[i] = NULL;
				return;
			}
		}
	}
	return;
	
}

static void paramSet_getFirstValueByName(paramSet *set, char *name, paramValue **value){
	parameter *tmp = NULL;

	if(set == NULL || value == NULL || name == NULL) return;
	*value = NULL;
	
	paramSet_getParameterByName(set, name, &tmp);
	if(tmp == NULL) return;
	
	*value = tmp->arg;
	
	return;	
}

static void paramSet_getValueByNameAt(paramSet *set, char *name,int at, paramValue **value){
	paramValue *tmp = NULL;

	if(set == NULL || value == NULL || name == NULL) return;
	*value = NULL;
	
	paramSet_getFirstValueByName(set,name, &tmp);
	if(tmp == NULL) return;
	
	tmp = paramValue_getElementAt(tmp, at);
	if(tmp == NULL) return;
	
	*value = tmp;

	return;
}



void paramSet_addControl(paramSet *set, const char *names, FormatStatus (*controlFormat)(const char *), ContentStatus (*controlContent)(const char *)){
	parameter *tmp = NULL;
	const char *pName = NULL;
	char buf[256];
	
	pName = names;		
	while((pName = getParametersName(pName,buf, sizeof(buf), NULL)) != NULL){
		paramSet_getParameterByName(set, buf, &tmp);
		if(tmp != NULL){
			tmp->controlFormat = controlFormat;
			tmp->controlContent = controlContent;
		}
	}
}


void paramSet_readFromFile(const char *fname, paramSet *set){
	FILE *file = NULL;
	char *ln = NULL;
	char line[1024];
	char flag[1024];
	char arg[1024];
	
	if(fname == NULL || set == NULL) goto cleanup;
	
	printf("File %s\n", fname);
	file = fopen(fname, "r");
	if(file == NULL) goto cleanup;
	
	
	while(fgets(line, sizeof(line),file)){
		ln = strchr(line, '\n');
		if(ln != NULL) *ln = 0;
		
		if(sscanf(line, "%s %s", flag, arg) == 2){
			if(paramSet_appendParameterByName(arg,flag[0] == '-' ? flag+1 : flag,set)==false){
				paramSet_appendParameterByName(flag[0] == '-' ? flag+1 : flag, UNKNOWN_PARAMETER_NAME,set);
				paramSet_appendParameterByName(arg,UNKNOWN_PARAMETER_NAME,set);
			}
		}	
		else{
			if(paramSet_appendParameterByName(NULL,line[0] == '-' ? line+1 : line,set)==false){
				paramSet_appendParameterByName(line[0] == '-' ? line+1 : line,UNKNOWN_PARAMETER_NAME,set);
			}
		}
		
	}
	
cleanup:
	
	if(file) fclose(file);
	return;
	
}
void paramSet_readFromCMD(int argc, char **argv, paramSet *set){
	int c = 0;
	paramValue *test = NULL;
	int itest = 0;
	char *stest = NULL;
	char str[2]={0,0};
	int i=0;
	char *tmp = NULL;
	char *flag = NULL;
	char *arg = NULL;

	if(set == NULL) return;
	
	for(i=1; i<argc;i++){
		tmp = argv[i];
		arg = NULL;
		flag = NULL;
		
		if(tmp[0] == '-'){
			flag = tmp+1;
			if(i+1<argc){
				if(argv[i+1][0] != '-')
					arg = argv[++i]; 
			}
			
//			printf("%i) -%s '%s'\n", i,flag, arg);
			if(paramSet_appendParameterByName(arg,flag,set)==false){
				paramSet_appendParameterByName(flag,UNKNOWN_PARAMETER_NAME,set);
				if(arg)
					paramSet_appendParameterByName(arg,UNKNOWN_PARAMETER_NAME,set);
				
			}
		}
		else{
			paramSet_appendParameterByName(tmp,UNKNOWN_PARAMETER_NAME,set);
		}
	}
	
	return;
}

bool paramSet_isFormatOK(paramSet *set){
	parameter **array = NULL;
	parameter *pParam = NULL;
	int i = 0;
	int numOfElements = 0;
	paramValue *value = NULL;
	bool status = true;
	
	if(set == NULL) return false;
	numOfElements = set->count;
	array = set->parameter;
	
	for(i=0; i<numOfElements;i++){
		if((pParam = array[i]) != NULL){
			if(strcmp(pParam->flagName, UNKNOWN_PARAMETER_NAME)==0) continue;
			if(pParam->arcCount > 1 && !(pParam->isMultipleAllowed)){
				printf("Error: Duplicate values '%s'!\n", pParam->flagName);
				status = false;
			}
			
			value = pParam->arg;
			while(value){
				if(value->formatStatus == FORMAT_OK){
					if(value->contentStatus != PARAM_OK){
						printf("Content error: -%s '%s' err: %s\n",pParam->flagName, value->cstr_value, getParameterContentErrorString(value->contentStatus));
						status = false;
					}
				}
				else{
					printf("Format error: -%s '%s' err: %s\n",pParam->flagName, value->cstr_value, getFormatErrorString(value->formatStatus));
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
	parameter **array = NULL;
	int i =0;
	
	if(set == NULL) return;
	numOfElements = set->count;
	printf("Raw param set (%i)::\n", numOfElements);
	array = set->parameter;
	
	for(i=0; i<numOfElements;i++){
		if(array[i]->flag){
			parameter_Print(array[i]);
		}
	}
	
	return;
	}


bool paramSet_getIntValueByNameAt(paramSet *set, char *name,int at, int *value){
	paramValue *tmp = NULL;
	
	if(set == NULL || value == NULL || name == NULL) return false;
	*value = 0;
	
	paramSet_getValueByNameAt(set, name, at, &tmp);
	if(tmp == NULL) return false;
	if(tmp->formatStatus != FORMAT_OK) return false;
		
	*value = atoi(tmp->cstr_value);
	return true;
}

bool paramSet_getStrValueByNameAt(paramSet *set, char *name,int at, char **value){
	paramValue *tmp = NULL;
	
	if(set == NULL || value == NULL || name == NULL) return false;
	*value = NULL;
	
	paramSet_getValueByNameAt(set, name, at, &tmp);
	if(tmp == NULL) return false;
	if(tmp->formatStatus != FORMAT_OK) return false;
		
	*value = tmp->cstr_value;
	return true;
}

bool paramSet_getValueCountByName(paramSet *set, char *name, int *count){
	parameter *param = NULL;
	
	if(set == NULL || name == NULL) return false;
	
	paramSet_getParameterByName(set, name, &param);
	
	if(param == NULL) return false;
	*count = param->arcCount;
	
	return true;
}

bool paramSet_isSetByName(paramSet *set, char *name){
	parameter *param = NULL;
	
	if(set == NULL || name == NULL) return false;
	
	paramSet_getParameterByName(set, name, &param);
	
	if(param == NULL) return false;
	return param->flag;
}

void paramSet_printUnknownParameterWarnings(paramSet *set){
	int i = 0;
	int count = 0;
	char *tmp = NULL;
	
	if(set == NULL) return;
	
	
	if(paramSet_getValueCountByName(set, UNKNOWN_PARAMETER_NAME,&count)){
		for(i=0; i<count; i++){
			paramValue *value = NULL;
			paramSet_getValueByNameAt(set, UNKNOWN_PARAMETER_NAME, i, &value);
			if(value)
				printf("Warning: Unknown parameter '%s'\n", value->cstr_value);
		}
	}
}