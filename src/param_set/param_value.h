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

#ifndef SET_PARAM_VALUE_H
#define	SET_PARAM_VALUE_H

#include "param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Creates a new parameter value object. If \c newObj is pointer to NULL, a new
 * object is created. If \c newObj points to existing \c PARAM_VAL object, a new
 * value is created and appended to the end of the linked list. After use, the 
 * root value must be freed by calling \c PARAM_VAL_free.
 * \param value		value as c-string. Can be NULL.
 * \param source	describes the source e.g. file name or environment variable. Can be NULL.
 * \param priority	priority of the parameter, must be positive.
 * \param newObj	receiving pointer.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_new(const char *value, const char* source, int priority, PARAM_VAL **newObj);

/**
 * Extracts element from PARAM_VAL linked list (see \c PARAM_VAL_getElement) and
 * appends a value right after the value found.
 *
 * \param	target		The first PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	obj			New obj to be inserted.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_insert(PARAM_VAL *target, const char* source, int priority, int at, PARAM_VAL *obj);

/**
 * Free parameter value object.
 * \param rootValue	Object to be freed.
 */
void PARAM_VAL_free(PARAM_VAL *rootValue);

/**
 * Extracts element from PARAM_VAL linked list. Element is extracted from given
 * priority level at the given index. If at is \PST_INDEX_LAST the last element is
 * extracted. If priority is \PST_PRIORITY_NONE every element is counted and returned 
 * at given index. If priority is \PST_PRIORITY_VALID_BASE (0) or higher, only elements
 * at the given priority are counted and extracted. For example if list contains 
 * 2 lower priority values followed by higher priority value at position 3 and at
 * is 0, function returns the last value (but the  first value matching the priority).
 * Priority \PST_PRIORITY_LOWEST and \PST_PRIORITY_HIGHEST are used to extract only
 * the lowest and the highest priority values. Use \PST_PRIORITY_HIGHER_THAN + <prio>
 * or \PST_PRIORITY_LOWER_THAN + <prio> to extract values with higher or lower
 * priority accordingly.
 * 
 * \param	rootValue	The first PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	val			Pointer to receiving pointer.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_getElement(PARAM_VAL *rootValue, const char* source, int priority, int at, PARAM_VAL** val);

/**
 * This function pops a element out of the PARAM_VAL linked list. After successful
 * poping rootValue is changed to the first element in the linked list.
 * THIS IS SOMEHOW ODD BEHAVIOUR. TODO FIX.
 * \param	rootValue	The pointer to pointer of the first PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	val			Pointer to receiving pointer of poped element.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_popElement(PARAM_VAL **rootValue, const char* source, int priority, int at, PARAM_VAL** val);

int PARAM_VAL_extract(PARAM_VAL *rootValue, const char **value, const char **source, int *priority);

/**
 * Count the values with given constraints. If there are no values matching the
 * constraints 0 is returned.
 * \param	rootValue	The first PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	prio		Constraint for the priority.
 * \param	count		Pointer to int receiving the count value.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_getElementCount(PARAM_VAL *rootValue, const char *source, int prio, int *count);

/**
 * Count the invalid values with given constraints. If there are no values matching the
 * constraints 0 is returned.
 * \param	rootValue	The first PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	prio		Constraint for the priority.
 * \param	count		Pointer to int receiving the count value.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_getInvalidCount(PARAM_VAL *rootValue, const char *source, int prio, int *count);

/**
 * Extract the next priority level <nextPrio> according to the <current>. If
 * <current> is set to \c PST_PRIORITY_HIGHEST or \c PST_PRIORITY_LOWEST the lowest
 * or highest value is returned. If <current> is set to other values lower than
 * \c PST_PRIORITY_VALID_BASE an error is returned. If current is set to a value
 * that is exactly the priority of the highest value error code
 * \c PST_PARAMETER_VALUE_NOT_FOUND is returned.
 *
 * \param	rootValue	The first PARAM_VAL link in the linked list.
 * \param	current		Priority value or (\c PST_PRIORITY_LOWEST or \c PST_PRIORITY_HIGHEST)
 * \param	nextPrio	Next highest priority or the lowest or highest.
 * 
 * \return \c PST_OK when successful, error code otherwise.
 */
int PARAM_VAL_getPriority(PARAM_VAL *rootValue, int current, int *nextPrio);

int PARAM_VAL_getErrors(PARAM_VAL *rootValue, int *format, int* content);

int PARAM_VAL_getInvalid(PARAM_VAL *rootValue, const char* source, int priority, int at, PARAM_VAL** val);

char* PARAM_VAL_toString(const PARAM_VAL *value, char *buf, size_t buf_len);
#ifdef	__cplusplus
}
#endif

#endif	/* SET_PARAM_VALUE_H */