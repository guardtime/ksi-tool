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

#ifndef SET_PARAM_VALUE_H
#define	SET_PARAM_VALUE_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

enum enum_priority {
	PST_PRIORITY_NOTDEFINED = -4,
	PST_PRIORITY_HIGHEST = -3,
	PST_PRIORITY_LOWEST = -2,
	PST_PRIORITY_NONE = -1,
	PST_PRIORITY_VALID_BASE = 0
}; 

enum enum_value_index {
	PST_INDEX_LAST = -1,
	PST_INDEX_FIRST = PST_PRIORITY_VALID_BASE,
}; 

/**
 * Creates a new parameter value object.
 * \param value		value as c-string. Can be NULL.
 * \param source	describes the source e.g. file name or environment variable. Can be NULL.
 * \param priority	priority of the parameter, must be positive.
 * \param newObj	receiving pointer.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_new(const char *value, const char* source, int priority, PARAM_VAL **newObj);

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
 * the lowest and the highest priority values.
 * 
 * \param	rootValue	The first PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	val			Pointer to receiving pointer.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_getElement(PARAM_VAL *rootValue, const char* source, int priority, int at, PARAM_VAL** val);


#ifdef	__cplusplus
}
#endif

#endif	/* SET_PARAM_VALUE_H */