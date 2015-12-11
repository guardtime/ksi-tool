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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ParamValue.h"
#include "param_set_obj_impl.h"


#define FORMAT_OK 0

static char *new_string(const char *str) {
	char *tmp = NULL;
	if(str == NULL) return NULL;
	tmp = (char*)malloc(strlen(str)*sizeof(char)+1);
	if(tmp == NULL) return NULL;
	return strcpy(tmp, str);
}

void parameter_free(PARAM *obj) {
	if(obj == NULL) return;
	free(obj->flagName);
	free(obj->flagAlias);
	if(obj->arg) PARAM_VAL_free(obj->arg);
	free(obj);
}

bool parameter_new(const char *flagName,const char *flagAlias, bool isMultipleAllowed, bool isSingleHighestPriority,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		bool (*convert)(const char*, char*, unsigned),
		PARAM **newObj){
	PARAM *tmp = NULL;

	if(newObj == NULL || flagName == NULL) return false;

	tmp = (PARAM*)malloc(sizeof(PARAM));
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
bool parameter_addArgument(PARAM *param, const char *argument, const char* source, int priority){
	PARAM_VAL *newValue = NULL;
	PARAM_VAL *pLastValue = NULL;
	bool status = false;
	const char *arg = NULL;
	char buf[1024];
	if(param == NULL) return false;

	/*If conversion function exists convert the argument*/
	if(param->convert)
		arg = param->convert(argument, buf, sizeof(buf)) ? buf : argument;
	else
		arg = argument;

	/*Create new object and control the format*/
	if(!PARAM_VAL_new(arg, source, priority, &newValue)) goto cleanup;

	if(param->controlFormat)
		newValue->formatStatus = param->controlFormat(arg);
	if(newValue->formatStatus == FORMAT_OK && param->controlContent)
		newValue->contentStatus = param->controlContent(arg);

	if(param->arg == NULL){
		param->arg = newValue;
	}
	else{
		pLastValue = (PARAM_VAL*)PARAM_VAL_getElementAt(param->arg, param->argCount-1);
		if(pLastValue == NULL || pLastValue->next != NULL) goto cleanup;
		pLastValue->next = newValue;
	}
	param->argCount++;

	if(param->highestPriority < priority)
		param->highestPriority = priority;

	newValue = NULL;
	status = true;

cleanup:

	PARAM_VAL_free(newValue);

	return status;
	}

void parameter_Print(const PARAM *param, int (*print)(const char*, ...)){
	PARAM_VAL *pValue = NULL;

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
bool parameter_isDuplicateConflict(const PARAM *param) {
	int highestPriority = 0;
	int count = 0;
	PARAM_VAL *value = NULL;

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