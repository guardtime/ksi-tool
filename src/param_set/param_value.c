/*
 * Copyright 2013-2016 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "param_set_obj_impl.h"
#include "param_value.h"
#include "param_set.h"
#include "strn.h"

static char *new_string(const char *str) {
	char *tmp = NULL;
	if (str == NULL) return NULL;
	tmp = (char*)malloc(strlen(str)*sizeof(char)+1);
	if (tmp == NULL) return NULL;
	return strcpy(tmp, str);
}

int PARAM_VAL_insert(PARAM_VAL *target, const char* source, int priority, int at, PARAM_VAL *obj) {
	int res;
	PARAM_VAL *insert_to = NULL;

	if (target == NULL || obj == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = PARAM_VAL_getElement(target, source, priority, at, &insert_to);
	if (res != PST_OK) goto cleanup;

	/**
	 * If a value is found, new obj is appended to the list after the value found.
	 */

	/* Initialize the new value. */
	obj->previous = insert_to;
	obj->next = insert_to->next;

	/* Fix the old previous value. */
	if (insert_to->next != NULL) {
		insert_to->next->previous = obj;
	}

	/* Fix the insert_to value. */
	insert_to->next = obj;



cleanup:
	return res;
}

int PARAM_VAL_new(const char *value, const char* source, int priority, PARAM_VAL **newObj) {
	int res;
	PARAM_VAL *tmp = NULL;
	char *tmp_value = NULL;
	char *tmp_source = NULL;

	if (newObj == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (priority < PST_PRIORITY_VALID_BASE) {
		res = PST_PRIORITY_NEGATIVE;
		goto cleanup;
	}

	if (priority > PST_PRIORITY_VALID_ROOF) {
		res = PST_PRIORITY_TOO_LARGE;
		goto cleanup;
	}

	/*Create obj itself*/
	tmp = (PARAM_VAL*)malloc(sizeof(PARAM_VAL));
	if (tmp == NULL) {
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

	if (value != NULL){
		tmp_value = new_string(value);
		if (tmp_value == NULL) {
			res = PST_OUT_OF_MEMORY;
			goto cleanup;
		}
	}

	if (source != NULL){
		tmp_source = new_string(source);
		if (tmp_source == NULL) {
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

		res = PARAM_VAL_insert(current, NULL, PST_PRIORITY_NONE, PST_INDEX_LAST, tmp);
		if (res != PST_OK) goto cleanup;
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
	PARAM_VAL *next = NULL;
	PARAM_VAL *to_be_freed = NULL;

	if (rootValue == NULL) return;

	to_be_freed = rootValue;
	next = rootValue;

	do {
		to_be_freed = next;
		next = next->next;
		free(to_be_freed->cstr_value);
		free(to_be_freed->source);
		free(to_be_freed);
	} while (next != NULL);

	return;
}

static int param_val_getPriority(PARAM_VAL *rootValue, int type, int *prio) {
	int res;
	PARAM_VAL *nxt = NULL;
	int tmp = 0;

	if (rootValue == NULL || prio == NULL || type <= PST_PRIORITY_NOTDEFINED
			|| type >= PST_PRIORITY_FIELD_OUT_OF_RANGE) {
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

static int prio_compare_if_match(int prio, int current) {
	int virtual_prio = PST_PRIORITY_NOTDEFINED;

	if (prio <= PST_PRIORITY_VALID_ROOF) {
		return prio == current;
	} else if (prio >= PST_PRIORITY_HIGHER_THAN && prio < PST_PRIORITY_LOWER_THAN) {
		virtual_prio = prio - PST_PRIORITY_HIGHER_THAN;
		return current > virtual_prio;
	} else if (prio >= PST_PRIORITY_LOWER_THAN && prio < PST_PRIORITY_FIELD_OUT_OF_RANGE) {
		virtual_prio = prio - PST_PRIORITY_LOWER_THAN;
		return current < virtual_prio;
	}

	return 0;
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
		if ((prio == PST_PRIORITY_NONE || (prio_compare_if_match(prio, current->priority)))
				&& (source == NULL || (source != NULL && current->source != NULL && strcmp(source, current->source) == 0))
				&& (onlyInvalid == 0 || (onlyInvalid && (current->contentStatus != 0 || current->formatStatus != 0)))) {

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
	 * If the previous element existed, repair its next value, as it is removed
	 * from the list. Set the new root as previous.
	 */
	if (previous != NULL) {
		previous->next = next;
		newRoot = previous;
	}

	/**
	* If the next element existed, fix its previous link as it is removed from
	* the list.
	*/
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

	for (;;) {
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

char* PARAM_VAL_toString(const PARAM_VAL *value, char *buf, size_t buf_len) {
	const PARAM_VAL *root = value;
	size_t count = 0;

	if (value == NULL || buf == NULL || buf_len == 0) return NULL;

	while (root->previous != NULL) {
		root = root->previous;
	}

	while (root != NULL) {
		count += PST_snprintf(buf + count, buf_len - count, "%s ->", root->cstr_value);
		root = root->next;
	}

	return buf;
}

void ITERATOR_free(ITERATOR *itr) {
	if (itr == NULL) return;
	free(itr);
}

int ITERATOR_reset(ITERATOR *itr) {
	if (itr == NULL) return PST_INVALID_ARGUMENT;
	itr->value = itr->root;
	itr->i = 0;
	return PST_OK;
}

int ITERATOR_set(ITERATOR *itr, PARAM_VAL *new_root, const char* source, int priority, int at) {
	int res;
	PARAM_VAL *tmp = NULL;

	if (itr == NULL) return PST_INVALID_ARGUMENT;

	res = ITERATOR_reset(itr);
	if (res != PST_OK) goto cleanup;

	if (new_root != NULL) {
		itr->root = new_root;
	}

	itr->source = source;
	itr->priority = priority;

	res = PARAM_VAL_getElement(itr->root, itr->source, itr->priority, at, &tmp);
	if (res != PST_OK && res != PST_PARAMETER_VALUE_NOT_FOUND)  goto cleanup;

	itr->value = tmp;
	itr->i = at;
	res = PST_OK;


cleanup:

	return PST_OK;
}

int ITERATOR_new(PARAM_VAL *root, ITERATOR **itr) {
	ITERATOR *tmp = NULL;
	int res = PST_UNKNOWN_ERROR;

	if (root == NULL || itr == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	tmp = (ITERATOR*)malloc(sizeof(ITERATOR));
	if (tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}
	tmp->root = root;
	tmp->value = tmp->root;
	tmp->i = 0;
	tmp->source = NULL;
	tmp->priority = PST_PRIORITY_NONE;

	*itr = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	ITERATOR_free(tmp);
	return res;
}

int ITERATOR_canBeUsedToFetch(ITERATOR *itr, const char* source, int priority, int at) {
	if (itr == NULL) return 0;
	if ((source == NULL && itr->source != NULL) && (source != NULL && itr->source == NULL)) return 0;
	if (itr->priority != priority) return 0;
	if (at < itr->i) return 0;
	return 1;
}

int ITERATOR_fetch(ITERATOR *itr, const char* source, int priority, int at, PARAM_VAL **item) {
	int res = PST_UNKNOWN_ERROR;
	PARAM_VAL *tmp = NULL;
	int virtual_at = 0;

	if (itr == NULL || item == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}


	/* If iterator is not suitable, reset its "pointer". */
	if (!ITERATOR_canBeUsedToFetch(itr, source, priority, at)) {
		res = ITERATOR_set(itr, NULL, source, priority, at);
		if (res != PST_OK) goto cleanup;
	}

	virtual_at = at - itr->i;
	res = PARAM_VAL_getElement(itr->value, itr->source, itr->priority, virtual_at, &tmp);
	if (res != PST_OK)  goto cleanup;


	itr->i = at;
	itr->value = tmp;
	*item = tmp;
	tmp = NULL;


cleanup:

	return res;

}