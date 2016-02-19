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
#include "param_value.h"
#include "param_set.h"

static char *new_string(const char *str) {
	char *tmp = NULL;
	if(str == NULL) return NULL;
	tmp = (char*)malloc(strlen(str)*sizeof(char)+1);
	if(tmp == NULL) return NULL;
	return strcpy(tmp, str);
}

#define PST_FORMAT_STATUS_OK 0
#define PST_CONTENT_STATUS_OK 0


int PARAM_VAL_new(const char *value, const char* source, int priority, PARAM_VAL **newObj) {
	int res;
	PARAM_VAL *tmp = NULL;
	char *tmp_value = NULL;
	char *tmp_source = NULL;

	if(newObj == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (priority < PST_PRIORITY_VALID_BASE) {
		res = PST_NEGATIVE_PRIORITY;
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
	tmp->formatStatus= PST_FORMAT_STATUS_OK;
	tmp->contentStatus = PST_CONTENT_STATUS_OK;
	tmp->next = NULL;
	tmp->previous = NULL;
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

	/**
	 * If receiving pointer is NULL, initialize it. If receiving pointer is not
	 * NULL iterate through linked list and append the value to the end.
     */
	if (*newObj == NULL) {
		tmp->previous = *newObj;
		*newObj = tmp;
	} else {
		PARAM_VAL *current = *newObj;

		while (current->next != NULL) {
			current = current->next;
		}

		tmp->previous = current;
		current->next = tmp;
	}

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

static int param_val_getPriority(PARAM_VAL *rootValue, int type, int *prio) {
	int res;
	PARAM_VAL *nxt = NULL;
	int tmp = 0;

	if (rootValue == NULL || prio == NULL || type <= PST_PRIORITY_NOTDEFINED) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * If the priority is not counted (\PST_PRIORITY_NONE) or in valid REAL
	 * priority range return the type.
	 */
	if (type == PST_PRIORITY_NONE || type >= PST_PRIORITY_VALID_BASE) {
		tmp = type;
	} else {
		/* If priority asked is the highest or the lowest extract it. */
		nxt = rootValue;
		tmp = nxt->priority;
		while (nxt != NULL) {
			if ((type == PST_PRIORITY_HIGHEST && tmp < nxt->priority)
				|| (type == PST_PRIORITY_LOWEST && tmp > nxt->priority)) {
				tmp = nxt->priority;
			}

			nxt = nxt->next;
		}
	}


	*prio = tmp;
	res = PST_OK;

cleanup:

	return res;
}

static int param_val_get_element(PARAM_VAL *rootValue, const char* source, int priority, int at, int onlyInvalid, PARAM_VAL** val) {
	int res;
	PARAM_VAL *current = NULL;
	PARAM_VAL *tmp = NULL;
	PARAM_VAL *last_match = NULL;
	int i = 0;
	int prio = 0;

	if (rootValue == NULL || priority <= PST_PRIORITY_NOTDEFINED || at < PST_INDEX_LAST || val == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	 /* Extract the real priority value to be used. */
	res = param_val_getPriority(rootValue, priority, &prio);
	if (res != PST_OK) goto cleanup;

	/**
	 * If at == i return value at the given position.
	 * If at is \PST_INDEX_LAST return the last.
	 */
	current = rootValue;
	while (current != NULL) {
		/* Increase count if (priority matches AND source matches). */
		if ((prio == PST_PRIORITY_NONE || prio == current->priority)
				&& (source == NULL || (source != NULL && current->source != NULL && strcmp(source, current->source) == 0))
				&& (onlyInvalid == 0 || onlyInvalid && (current->contentStatus != 0 || current->formatStatus != 0))) {

			last_match = current;

			if (at >= PST_INDEX_FIRST && i == at) {
				tmp =  current;
				break;
			}

			i++;
		}

		current = current->next;
	}

	if (tmp == NULL && last_match != NULL && at == PST_INDEX_LAST) {
		tmp = last_match;
	}

	if (tmp == NULL) {
		res = PST_PARAMETER_VALUE_NOT_FOUND;
		goto cleanup;
	}

	*val = tmp;
	res = PST_OK;

cleanup:

	return res;
}

int PARAM_VAL_popElement(PARAM_VAL **rootValue, const char* source, int priority, int at, PARAM_VAL** val) {
	int res;
	PARAM_VAL *previous = NULL;
	PARAM_VAL *next = NULL;
	PARAM_VAL *tmp = NULL;
	PARAM_VAL *newRoot = NULL;

	/**
	 * Extract a value from the chain.
     */
	res = param_val_get_element(*rootValue, source, priority, at, 0, &tmp);
	if (res != PST_OK) goto cleanup;

	previous = tmp->previous;
	next = tmp->next;

	/**
	 * Repair the chain and root value.
     */
	if (previous != NULL) {
		previous->next = next;
		newRoot = previous;
	}

	if (next != NULL) {
		newRoot = (newRoot != NULL) ? newRoot : next;
		next->previous = previous;
	}

	while (newRoot != NULL && newRoot->previous != NULL) {
		newRoot = newRoot->previous;
	}


	*rootValue = newRoot;
	tmp->previous = NULL;
	tmp->next = NULL;
	*val = tmp;
	res = PST_OK;

cleanup:
	return res;
}

int PARAM_VAL_getElement(PARAM_VAL *rootValue, const char* source, int priority, int at, PARAM_VAL** val) {
	return param_val_get_element(rootValue, source, priority, at, 0, val);
}

int PARAM_VAL_getInvalid(PARAM_VAL *rootValue, const char* source, int priority, int at, PARAM_VAL** val) {
	return param_val_get_element(rootValue, source, priority, at, 1, val);
}

static int param_val_get_element_count(PARAM_VAL *rootValue, const char *source, int prio,
		int (*value_getter)(PARAM_VAL *, const char*, int, int, PARAM_VAL**),
		int *count) {
	int res;
	PARAM_VAL *tmp = NULL;
	int i = 0;


	if (rootValue == NULL || prio <= PST_PRIORITY_NOTDEFINED || value_getter == NULL || count == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	while (1) {
		res = value_getter(rootValue, source, prio, i, &tmp);
		if (res != PST_OK && res != PST_PARAMETER_VALUE_NOT_FOUND) {
			goto cleanup;
		}

		if (res == PST_PARAMETER_VALUE_NOT_FOUND) {
			break;
		}

		i++;
	}


	*count = i;
	res = PST_OK;

cleanup:

	return res;
}

int PARAM_VAL_getElementCount(PARAM_VAL *rootValue, const char *source, int prio, int *count) {
	return param_val_get_element_count(rootValue, source, prio, PARAM_VAL_getElement, count);
}

int PARAM_VAL_getInvalidCount(PARAM_VAL *rootValue, const char *source, int prio, int *count) {
	return param_val_get_element_count(rootValue, source, prio, PARAM_VAL_getInvalid, count);
}

int PARAM_VAL_extract(PARAM_VAL *rootValue, const char **value, const char **source, int *priority) {
	int res;

	if (rootValue == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Extract parameters;
     */
	if (value != NULL) {
		*value = rootValue->cstr_value;
	}

	if (source != NULL) {
		*source = rootValue->source;
	}

	if (priority != NULL) {
		*priority = rootValue->priority;
	}

	res = PST_OK;

cleanup:

	return res;
}

int PARAM_VAL_getErrors(PARAM_VAL *rootValue, int *format, int* content) {
	int res;

	if (rootValue == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Extract error values;
     */
	if (format != NULL) {
		*format = rootValue->formatStatus;
	}

	if (content != NULL) {
		*content = rootValue->contentStatus;
	}

	res = PST_OK;

cleanup:

	return res;
}

int PARAM_VAL_getPriority(PARAM_VAL *rootValue, int current, int *nextPrio) {
	int res;
	PARAM_VAL *nxt = NULL;
	int tmp = 0;

	if (rootValue == NULL || current <= PST_PRIORITY_NOTDEFINED
			|| current == PST_PRIORITY_NONE || nextPrio == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * If highest or lowest priority is asked.
     */
	if (current == PST_PRIORITY_LOWEST || current == PST_PRIORITY_HIGHEST) {
		res = param_val_getPriority(rootValue, current, &tmp);
		if (res != PST_OK) goto cleanup;

		*nextPrio = tmp;
		res = PST_OK;
		goto cleanup;
	}


	/**
	 * If the next priority is asked, extract it.
     */
	nxt = rootValue;

	res = param_val_getPriority(rootValue, PST_PRIORITY_HIGHEST, &tmp);
	if (res != PST_OK) goto cleanup;

	while (nxt != NULL) {
		if (nxt->priority > current) {
			if (tmp > nxt->priority) {
				tmp = nxt->priority;
			}
		}

		nxt = nxt->next;
	}

	if (tmp == current) {
		res = PST_PARAMETER_VALUE_NOT_FOUND;
		goto cleanup;
	}

	*nextPrio = tmp;
	res = PST_OK;

cleanup:

	return res;
}
