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
#include "param_set_obj_impl.h"
#include "ParamValue.h"
#include "Parameter.h"


#define FORMAT_OK 0

static char *new_string(const char *str) {
	char *tmp = NULL;
	if(str == NULL) return NULL;
	tmp = (char*)malloc(strlen(str)*sizeof(char)+1);
	if(tmp == NULL) return NULL;
	return strcpy(tmp, str);
}

int PARAM_new(const char *flagName,const char *flagAlias, int isMultipleAllowed, int isSingleHighestPriority,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		int (*convert)(const char*, char*, unsigned),
		PARAM **newObj){
	int res;
	PARAM *tmp = NULL;
	char *tmpFlagName = NULL;
	char *tmpAlias = NULL;

	if (flagName == NULL || newObj == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	tmp = (PARAM*)malloc(sizeof(PARAM));
	if (tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->flagName = NULL;
	tmp->flagAlias = NULL;
	tmp->arg = NULL;
	tmp->highestPriority = 0;
	tmp->isMultipleAllowed = isMultipleAllowed;
	tmp->isSingleHighestPriority = isSingleHighestPriority;
	tmp->argCount = 0;
	tmp->controlFormat = controlFormat;
	tmp->controlContent = controlContent;
	tmp->convert = convert;


	tmpFlagName = new_string(flagName);
	if (tmpFlagName == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	if (flagAlias) {
		tmpAlias = new_string(flagAlias);
		if(tmpAlias == NULL) {
			res = PST_OUT_OF_MEMORY;
			goto cleanup;
		}
	}

	tmp->flagAlias = tmpAlias;
	tmp->flagName = tmpFlagName;
	*newObj = tmp;

	tmpAlias = NULL;
	tmpFlagName = NULL;
	tmp = NULL;

	res = PST_OK;

cleanup:

	free(tmpFlagName);
	free(tmpAlias);
	PARAM_free(tmp);
	return res;
}


void PARAM_free(PARAM *obj) {
	if(obj == NULL) return;
	free(obj->flagName);
	free(obj->flagAlias);
	if(obj->arg) PARAM_VAL_free(obj->arg);
	free(obj);
}

int PARAM_addArgument(PARAM *param, const char *argument, const char* source, int priority){
	int res;
	PARAM_VAL *newValue = NULL;
	PARAM_VAL *pLastValue = NULL;
	const char *arg = NULL;
	char buf[1024];

	if(param == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/*If conversion function exists convert the argument*/
	if(param->convert)
		arg = param->convert(argument, buf, sizeof(buf)) ? buf : argument;
	else
		arg = argument;

	/*Create new object and control the format*/
	res = PARAM_VAL_new(arg, source, priority, &newValue);
	if(res != PST_OK) goto cleanup;

	if (param->controlFormat)
		newValue->formatStatus = param->controlFormat(arg);
	if (newValue->formatStatus == FORMAT_OK && param->controlContent)
		newValue->contentStatus = param->controlContent(arg);

	if (param->arg == NULL) {
		param->arg = newValue;
	} else{
		res = PARAM_VAL_getElement(param->arg, NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, &pLastValue);
		if(res != PST_OK) goto cleanup;

		/* The last element must exists and its next value must be NULL. */
		if(pLastValue == NULL || pLastValue->next != NULL) {
			res = PST_UNDEFINED_BEHAVIOUR;
			goto cleanup;
		}
		pLastValue->next = newValue;
	}
	param->argCount++;

	if(param->highestPriority < priority)
		param->highestPriority = priority;

	newValue = NULL;
	res = PST_OK;

cleanup:

	PARAM_VAL_free(newValue);

	return res;
}

int PARAM_getValue(PARAM *param, const char *name, const char *source, int prio, unsigned at, PARAM_VAL **value) {
	int res;
	PARAM_VAL *tmp = NULL;

	if (param == NULL || name == NULL || value == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (param->arg == NULL) {
		res = PST_PARAMETER_EMPTY;
		goto cleanup;
	}

	res = PARAM_VAL_getElement(param->arg, source, prio, at, &tmp);
	if (res != PST_OK) goto cleanup;

	*value = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	return res;
}

int PARAM_isDuplicateConflict(const PARAM *param) {
	int highestPriority = 0;
	int count = 0;
	PARAM_VAL *value = NULL;

	if(param->argCount > 1 && !(param->isMultipleAllowed || param->isSingleHighestPriority)){
		return 1;
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
			return 1;
	}
	return 0;
}

void PARAM_print(const PARAM *param, int (*print)(const char*, ...)){
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