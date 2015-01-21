#include <stdio.h>		//input output
#include <string.h>		
#include <stdlib.h>		//malloc, random, int ja strn 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "gt_cmd_control.h"
#include "param_set.h"
#include "task_def.h"

#define SINGLE false
#define MULTIPLE true 

#define UNKNOWN_PARAMETER_NAME "_UN_KNOWN_"
#define TYPO_PARAMETER_NAME "_TYPO_"

typedef struct paramValu_st paramValue;

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
	char *flagAlias;
	bool flag;									//if the varible is set
	bool isMultipleAllowed;						//If there is more than 1 parameters allowed
	int arcCount;								//count of arguments in chain
	paramValue *arg;							//argument(s)
	
	FormatStatus (*controlFormat)(const char*);	//function pointer for format control
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

static bool parameter_new(const char *flagName,const char *flagAlias, bool isMultipleAllowed,
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
	
	tmp->flagName = (char*)malloc(sizeof(char)*strlen(flagName)+1);
	if(tmp->flagName == NULL) goto cleanup;
	strcpy(tmp->flagName, flagName);
	
	if(flagAlias){
		tmp->flagAlias = (char*)malloc(sizeof(char)*strlen(flagAlias)+1);
		if(tmp->flagAlias == NULL) goto cleanup;
		strcpy(tmp->flagAlias, flagAlias);
	}
	
	tmp->flag = false;
	tmp->isMultipleAllowed = isMultipleAllowed;
	tmp->arcCount = 0;
	tmp->arg = NULL;
	
	tmp->controlFormat = controlFormat;
	tmp->controlContent = controlContent;
	tmp->convert = convert;
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
	char *arg = NULL;
	char buf[1024];
	if(param == NULL) return false;

	/*If conversion function exists convert the argument*/
	if(param->convert){
		if(param->convert(argument, buf, sizeof(buf)))
			arg = buf;
		else
			arg = (char *)argument;
	}
	else{
		arg = (char *)argument;
	}
	
	/*Create new object and control the format*/
	if(!paramValue_new(arg, &newValue)) goto cleanup;
	
	if(param->controlFormat)
		newValue->formatStatus = param->controlFormat(arg);
	if(newValue->formatStatus == FORMAT_OK && param->controlContent)
		newValue->contentStatus = param->controlContent(arg);
	
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

static char *getParametersName(const char* names, char *name,char *alias, short len, bool *isMultiple){
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
	
	
	name[i_name] = 0;
	if(alias)
		alias[i_alias] = 0;
	
	return &pName[i];
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
			if(strcmp(tmp->flagName, name) == 0 || (tmp->flagAlias && strcmp(tmp->flagAlias, name) == 0)){
				*param = tmp;
				return;
			}
		}
	}
	return;
}

bool paramSet_appendParameterByName(const char *argument, char *name, paramSet *set){
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
			if(strcmp(tmp->flagName, name) == 0 || (tmp->flagAlias && strcmp(tmp->flagAlias, name) == 0)){
				tmp->arcCount = 0;
				tmp->flag = false;
				if(tmp->arg) paramValue_recursiveFree(tmp->arg);
				tmp->arg = NULL;
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
	char alias[256];
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
	paramCount+=2;

	tmp = (paramSet*)calloc(sizeof(paramSet), 1);
	if(tmp == NULL) goto cleanup;
	tmp->parameter = (parameter**)calloc(paramCount, sizeof(parameter*));
	if(tmp->parameter == NULL) goto cleanup;

	tmp->count = paramCount;

	len = sizeof(buf);
	pName = names;
	
	while((pName = getParametersName(pName,buf,alias, len, &isMultiple)) != NULL){
//		printf(">>> '%s'/'%s' : %i\n", buf,alias, isMultiple);
		if(!parameter_new(buf,alias[0] ? alias : NULL,isMultiple, NULL, NULL, NULL, &tmp->parameter[iter]))
			goto cleanup;
		iter++;
	}
	
	parameter_new(UNKNOWN_PARAMETER_NAME, NULL,MULTIPLE, NULL, NULL, NULL, &tmp->parameter[iter++]);
	parameter_new(TYPO_PARAMETER_NAME, NULL,MULTIPLE, NULL, NULL, NULL, &tmp->parameter[iter]);
	
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
	while((pName = getParametersName(pName,buf, NULL, sizeof(buf), NULL)) != NULL){
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

static int editDistance_levenshtein(const char *A, const char *B){
	unsigned lenA, lenB;
	unsigned M_H, M_W;
	unsigned i=0, j=0;
	char **m;
	unsigned rows_created = 0;
	unsigned edit_distance = -1;
	
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
		m[i][0] = i;
		rows_created++;
	}
	
	for(j=0; j<M_W; j++) m[0][j] = j;
	
	for(j=1; j<M_W; j++){
		for(i=1; i<M_H; i++){
			if(A[i-1] == B[j-1]) m[i][j] = DIAG(m,i,j);
			else m[i][j] = min_of_3(UP(m,i,j), LEFT(m,i,j), DIAG(m,i,j))+1;
		}
	}
	edit_distance = m[i-1][j-1];
	
//	printf("Matrix::\n");
//	for(i=0; i<M_H; i++){
//		for(j=0; j<M_W; j++){
//			printf("%2i", m[i][j]);
//		}
//		printf("\n");
//	}
	
cleanup:
	if(m)					
		for(i=0; i<rows_created; i++) free(m[i]);
	free(m);
	
	return edit_distance;
}

static bool paramSet_couldItBeTypo(const char *str, paramSet *set){
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
static void paramSet_addRawParameter(char *param, char *arg, paramSet *set){
	int i=0;
	char *flag = NULL;
	unsigned len;
	
	len = (unsigned)strlen(param);
	if(param[0] == '-' && param[1] != 0){
		flag = param+(param[1] == '-' ? 2 : 1);

		/*It is long parameter*/
		if(strncmp("--", param, 2)==0 && len >= 3){
			if(param[3] == 0){
				paramSet_appendParameterByName(flag,TYPO_PARAMETER_NAME,set);
				if(arg)
					paramSet_appendParameterByName(arg,paramSet_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME,set);
				return;
			}
				
			if(paramSet_appendParameterByName(arg,flag,set)==false){
				paramSet_appendParameterByName(flag,paramSet_couldItBeTypo(flag, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME,set);
				if(arg)
					paramSet_appendParameterByName(arg,paramSet_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME,set);
			}
		}
		/*It is short parameter*/
		else if(param[0] == '-' && len == 2){
			if(paramSet_appendParameterByName(arg,flag,set)==false){
				paramSet_appendParameterByName(flag,UNKNOWN_PARAMETER_NAME,set);
				if(arg)
					paramSet_appendParameterByName(arg,paramSet_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME,set);
			}
		}
		/*It is bunch of flags*/
		else{
			//paramSet_appendParameterByName(arg,flag,set)==false
			char str_flg[2] = {255,0};
			int itr = 0;
			
			if(arg)
				paramSet_appendParameterByName(arg,paramSet_couldItBeTypo(arg, set) ? TYPO_PARAMETER_NAME : UNKNOWN_PARAMETER_NAME,set);
			
			while(str_flg[0] = flag[itr++]){
				if(paramSet_appendParameterByName(NULL,str_flg,set)==false){
					if(paramSet_couldItBeTypo(flag, set)){
						paramSet_appendParameterByName(flag,TYPO_PARAMETER_NAME,set);
						break;
					}
					else
						paramSet_appendParameterByName(str_flg,UNKNOWN_PARAMETER_NAME,set);
				}
			}

		}
	}
	else{
		paramSet_appendParameterByName(param,UNKNOWN_PARAMETER_NAME,set);
	}
} 

void paramSet_readFromFile(const char *fname, paramSet *set){
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

		if(sscanf(line, "%s %s", flag, arg) == 2){
			paramSet_addRawParameter(flag, arg, set);
		}	
		else{
			paramSet_addRawParameter(line,NULL, set);
		}
	}
	
cleanup:
	
	if(file) fclose(file);
	return;
}

void paramSet_readFromCMD(int argc, char **argv, paramSet *set){
	int i=0;
	char *tmp = NULL;
	char *flag = NULL;
	char *arg = NULL;

	if(set == NULL) return;
	
	for(i=1; i<argc;i++){
		tmp = argv[i];
		arg = NULL;
		flag = NULL;
		
		if(i+1<argc){
			if(argv[i+1][0] != '-')
				arg = argv[++i]; 
		}
		
		paramSet_addRawParameter(tmp, arg, set);
	}
	
	return;
}

bool paramSet_isFormatOK(paramSet *set){
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

			if(pParam->arcCount > 1 && !(pParam->isMultipleAllowed)){
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

void paramSet_PrintErrorMessages(paramSet *set){
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
			
			if(pParam->arcCount > 1 && !(pParam->isMultipleAllowed)){
				printf("Error: Duplicate values '%s'!\n", pParam->flagName);
			}
			
			value = pParam->arg;
			while(value){
				if(value->formatStatus == FORMAT_OK){
					if(value->contentStatus != PARAM_OK){
						printf("Content error: %s%s '%s' err: %s\n",strlen(pParam->flagName)>1 ? "--" : "-", pParam->flagName, value->cstr_value, getParameterContentErrorString(value->contentStatus));
					}
				}
				else{
					printf("Format error: %s%s '%s' err: %s\n",strlen(pParam->flagName)>1 ? "--" : "-", pParam->flagName, value->cstr_value, getFormatErrorString(value->formatStatus));
				}
				
				value = value->next;
			}
		}
	}
	
	return;
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



static void paramSet_PrintSimilar(const char *str, paramSet *set){
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
			printf("Typo: Did You mean '%s%s'.\n",lenName>1 ? "--" : "-", array[i]->flagName);
		
		if(array[i]->flagAlias){
			editDist = editDistance_levenshtein(array[i]->flagAlias, str); 
			lenName = (unsigned)strlen(array[i]->flagAlias);

			if((editDist*100)/lenName <= 33)
				printf("Typo: Did You mean '%s%s'.\n",lenName>1 ? "--" : "-", array[i]->flagAlias);
		}
	}
	
	return;
}

void paramSet_printTypoWarnings(paramSet *set){
	int i = 0;
	int count = 0;
	char *tmp = NULL;
	
	if(set == NULL) return;
	
	if(paramSet_getValueCountByName(set, TYPO_PARAMETER_NAME,&count)){
		for(i=0; i<count; i++){
			paramValue *value = NULL;
			paramSet_getValueByNameAt(set, TYPO_PARAMETER_NAME, i, &value);
			if(value)
				paramSet_PrintSimilar(value->cstr_value, set);
		}
	}
}

bool paramSet_isTypos(paramSet *set){
	int count = 0;
	paramSet_getValueCountByName(set, TYPO_PARAMETER_NAME,&count);
	return count > 0 ? true : false; 
}