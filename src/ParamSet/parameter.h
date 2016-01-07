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

#ifndef SET_PARAMETER_H
#define	SET_PARAMETER_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

	
	
/**
 * Create a new empty parameter with constraints. When flag \c isMultipleAllowed
 * is set it means that only a single parameter can be present. If flag
 * \c isSingleHighestPriority is set, it means that there can be multiple values
 * but different priorities.
 * \param	flagName				The parameters name.
 * \param	flagAlias				The alias for the name. Can be NULL.
 * \param	isMultipleAllowed		Set as 1 to allow multiple values.
 * \param	isSingleHighestPriority	Set as 1 to allow just one value at given priority level.
 * \param	controlFormat			Function pointer to control the format. Can be NULL.
 * \param	controlContent			Function pointer to control the content. Can be NULL.
 * \param	convert					Function pointer to convert the value before parsing. Can be NULL.
 * \param	newObj					Pointer to receiving pointer to new object.
 * \return \c PST_OK when successful, error code otherwise..
 * \note It must be noted that it is possible to add multiple parameters to the
 * list using function \ref PARAM_addArgument no matter what the constraints are.
 * When parameter constraints are broken its validity check fails.
 */
int PARAM_new(const char *flagName,const char *flagAlias, int isMultipleAllowed, int isSingleHighestPriority,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		int (*convert)(const char*, char*, unsigned),
		PARAM **newObj);

/**
 * Function to free PARAM object.
 * \param obj	Pointer to Object to be freed.
 */
void PARAM_free(PARAM *obj);

/**
 * Add new argument to the linked list.
 * \param	param		Parameter object.
 * \param	argument	Parameters value as c-string. Can be NULL.
 * \param	source		Source description as c-string. Can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_NONE \c PST_PRIORITY_LOWEST \c PST_PRIORITY_HIGHEST \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \return \c PST_OK when successful, error code otherwise.
 */
int PARAM_addArgument(PARAM *param, const char *argument, const char* source, int priority);


/**
 * Extracts element from PARAM internal linked list. Element is extracted from given
 * priority level at the given index. If at is \c PST_INDEX_LAST the last element is
 * extracted. If priority is \c PST_PRIORITY_NONE every element is counted and returned 
 * at given index. If priority is \c PST_PRIORITY_VALID_BASE (0) or higher, only elements
 * at the given priority are counted and extracted. For example if list contains 
 * 2 lower priority values followed by higher priority value at position 3 and at
 * is 0, function returns the last value (but the  first value matching the priority).
 * Priority \c PST_PRIORITY_LOWEST and \c PST_PRIORITY_HIGHEST are used to extract only
 * the lowest and the highest priority values.
 * \param	param		Parameter object.
 * \param	name		Parameters name or alias if available.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	value		Pointer to receiving pointer to \c PARAM_VAL object. 
 * \return \c PST_OK when successful, error code otherwise. When value is not found
 * \c PST_PARAMETER_VALUE_NOT_FOUND is returned. If parameter is empty \c PST_PARAMETER_EMPTY
 * is returned.
 */
int PARAM_getValue(PARAM *param, const char *name, const char *source, int prio, unsigned at, PARAM_VAL **value);

/**
 * Function that examines if parameter has valid count of parameters including
 * priority level checks. See \ref PARAM_new.
 * \param	param		Parameter object.
 * \return 0 if there are no conflicts, greater than 0 otherwise. 
 */
int PARAM_isDuplicateConflict(const PARAM *param);


void PARAM_print(const PARAM *param, int (*print)(const char*, ...));



#ifdef	__cplusplus
}
#endif

#endif	/* SET_PARAMETER_H */