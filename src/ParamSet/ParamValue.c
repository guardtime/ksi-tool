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
#include "../gt_cmd_common.h"		//Temp usage for bool
#include "param_set_obj_impl.h"

static char *new_string(const char *str) {
	char *tmp = NULL;
	if(str == NULL) return NULL;
	tmp = (char*)malloc(strlen(str)*sizeof(char)+1);
	if(tmp == NULL) return NULL;
	return strcpy(tmp, str);
}

#define UNKNOW_FORMAT_STATUS -1
#define UNKNOW_CONTENT_STATUS -1

/**
 * Creates a new parameter value object.
 * @param value - value as c-string. Can be NULL.
 * @param source - describes the source e.g. file name or environment variable. Can be NULL.
 * @param priority - priority of the parameter.
 * @param newObj - receiving pointer.
 * @return true on success, false otherwise.
 */
int PARAM_VAL_new(const char *value, const char* source, int priority, PARAM_VAL **newObj) {
	int res;
	PARAM_VAL *tmp = NULL;
	char *tmp_value = NULL;
	char *tmp_source = NULL;

	if(newObj == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/*Create obj itself*/
	tmp = (PARAM_VAL*)malloc(sizeof(PARAM_VAL));
	if(tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->cstr_value = NULL;
	tmp->source = NULL;
	tmp->formatStatus= UNKNOW_FORMAT_STATUS;
	tmp->contentStatus = UNKNOW_CONTENT_STATUS;
	tmp->next = NULL;
	tmp->priority = priority;

	if(value != NULL){
		tmp_value = new_string(value);
		if(tmp_value == NULL) {
			res = PST_OUT_OF_MEMORY;
			goto cleanup;
		}
	}

	if(source != NULL){
		tmp_source = new_string(source);
		if(tmp_source == NULL) {
			res = PST_OUT_OF_MEMORY;
			goto cleanup;
		}
	}

	tmp->cstr_value = tmp_value;
	tmp->source = tmp_source;
	*newObj = tmp;

	tmp = NULL;
	tmp_value = NULL;
	tmp_source = NULL;

	res = PST_OK;

cleanup:

	free(tmp_value);
	free(tmp_source);
	PARAM_VAL_free(tmp);
	return res;
}

void PARAM_VAL_free(PARAM_VAL *rootValue) {
	if(rootValue == NULL) return;

	if(rootValue->next != NULL)
		PARAM_VAL_free(rootValue->next);

	free(rootValue->cstr_value);
	free(rootValue->source);
	free(rootValue);
}

PARAM_VAL* PARAM_VAL_getElementAt(PARAM_VAL *rootValue, unsigned at) {
	PARAM_VAL *tmp = NULL;
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


PARAM_VAL* PARAM_VAL_getFirstHighestPriorityValue(PARAM_VAL *rootValue) {
	PARAM_VAL *pValue = NULL;
	PARAM_VAL *master = NULL;

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