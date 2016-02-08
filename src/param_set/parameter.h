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

#include "param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

	
enum PARAM_CONSTRAINTS_enum {
	/** Only a single value is allowed. */
	PARAM_SINGLE_VALUE = 0x0002,
	/** Only a single value is allowed at each priority level. */
	PARAM_SINGLE_VALUE_FOR_PRIORITY_LEVEL = 0x0004,
	/**/
	PARAM_INVALID_CONSTRAINT = 0x8000,
};
	
	
/**
 * Create a new empty parameter with constraints <flags> (see \c PARAM_FLAGS).
 *  
 * \param	flagName	The parameters name.
 * \param	flagAlias	The alias for the name. Can be NULL.
 * \param	constraint	Constraints for the parameter and its values (see \c PARAM_FLAGS).
 * \param	newObj		Pointer to receiving pointer to new object.
 * \return \c PST_OK when successful, error code otherwise.
 * 
 * \note It must be noted that it is possible to add multiple parameters to the
 * list using function \ref PARAM_addArgument no matter what the constraints are.
 * When parameter constraints are broken its validity check fails - see 
 * \ref PARAM_checkConstraints.
 */
int PARAM_new(const char *flagName,const char *flagAlias, int constraint, PARAM **newObj);

/**
 * Function to free PARAM object.
 * \param obj	Pointer to Object to be freed.
 */
void PARAM_free(PARAM *obj);

/**
 * Add control functions to the PARAM. If content or format status is invalid
 * it is still possible to extract PARAM_VAL objects (see \ref PARAM_getValue and
 * \ref PARAM_getInvalid). Value that has invalid status can not be extracted as
 * object (see \ref PARAM_getObject).
 * 
 * Function \c controlFormat takes input as raw parameter and returns 0 if
 * successful, status indicating the failure otherwise.
 * 
 * Function controlContent takes input as raw parameter and returns 0 if
 * successful, status indicating the failure otherwise.
 * 
 * Function convert takes input as raw parameter, followed by a buffer and its size.
 * Returns 1 if conversion successful, 0 otherwise.
 * 
 * \param	obj				Parameter object.
 * \param	controlFormat	Function pointer to control the format. Can be NULL.
 * \param	controlContent	Function pointer to control the content. Can be NULL.
 * \param	convert			Function pointer to convert the value before parsing. Can be NULL.
 * \return \c PST_OK when successful, error code otherwise.
 */
int PARAM_addControl(PARAM *obj, int (*controlFormat)(const char *), int (*controlContent)(const char *), int (*convert)(const char*, char*, unsigned));

/**
 * Set object extractor to the parameter that implements \ref PARAM_getObject. If
 * extractor allocates memory it must be freed by the user. As PARAM_getObject value
 * pointer is given directly to the extractor it is possible to initialize existing
 * objects or create a new ones. It all depends how the extractor method is implemented.
 * 
 * int extractObject(void *extra, const char *str, void **obj)
 * extra - optional pointer to data structure.
 * str - c-string value that belongs to PARAM_VAL object.
 * obj - pointer to receiving pointer to desired object.
 * Returns PST_OK if successful, error code otherwise.
 * 
 * \param	obj				Parameter object.
 * \param	extractObject	Object extractor.
 * \return \c PST_OK when successful, error code otherwise.
 */
int PARAM_setObjectExtractor(PARAM *obj, int (*extractObject)(void *, const char *, void**));

/**
 * Add new argument to the linked list. See \ref PARAM_addControl, \ref PARAM_getValue
 * \ref PARAM_getInvalid, \ref PARAM_getValueCount and \ref PARAM_getInvalidCount.
 * 
 * \param	param		Parameter object.
 * \param	argument	Parameters value as c-string. Can be NULL.
 * \param	source		Source description as c-string. Can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \return \c PST_OK when successful, error code otherwise.
 */
int PARAM_addValue(PARAM *param, const char *argument, const char* source, int priority);

/**
 * Extracts element from PARAM internal linked list. Element is extracted from given
 * priority level at the given index. If <at> is \c PST_INDEX_LAST the last element is
 * extracted. If priority is \c PST_PRIORITY_NONE every element is counted and returned 
 * at given priority level. If priority is \c PST_PRIORITY_VALID_BASE (0) or higher, only elements
 * at the given priority are counted and extracted. For example if list contains 
 * 2 lower priority values followed by higher priority value at position 3 and <at>
 * is 0, function returns the last value (but the  first value matching the priority).
 * Priority \c PST_PRIORITY_LOWEST and \c PST_PRIORITY_HIGHEST are used to extract only
 * the lowest and the highest priority values.
 * 
 * \param	param		Parameter object.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Constraint for the priority, can be \c PST_PRIORITY_NONE, \c PST_PRIORITY_LOWEST, \c PST_PRIORITY_HIGHEST or greater than 0.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	value		Pointer to receiving pointer to \c PARAM_VAL object.
 * 
 * \return \c PST_OK when successful, error code otherwise. When value is not found
 * \c PST_PARAMETER_VALUE_NOT_FOUND is returned. If parameter is empty \c PST_PARAMETER_EMPTY
 * is returned.
 * \note See \ref 
 */
int PARAM_getValue(PARAM *param, const char *source, int prio, unsigned at, PARAM_VAL **value);

/**
 * Same as get value, but values that have been controlled and failed by the
 * format and control functions (see \ref PARAM_addControl) will be returned.
 * 
 * \param	param		Parameter object.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Constraint for the priority, can be \c PST_PRIORITY_NONE, \c PST_PRIORITY_LOWEST, \c PST_PRIORITY_HIGHEST or greater than 0.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	value		Pointer to receiving pointer to \c PARAM_VAL object. 
 * 
 * \return \c PST_OK when successful, error code otherwise. When value is not found
 * \c PST_PARAMETER_VALUE_NOT_FOUND is returned. If parameter is empty \c PST_PARAMETER_EMPTY
 * is returned.
 */


/**
 * Like function \ref PARAM_getValue but is used to extract a specific object from
 * the c-string value. It must be noted that if format or content status is invalid,
 * it is not possible to extract the object. The functionality must be implemented
 * by setting object extractor function.  <value> is given to the extractor function
 * directly! See \ref PARAM_setObjectExtractor for more details.
 * 
 * \param	param		Parameter object.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Constraint for the priority, can be \c PST_PRIORITY_NONE, \c PST_PRIORITY_LOWEST, \c PST_PRIORITY_HIGHEST or greater than 0.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	extra		Pointer to optional extra data.
 * \param	value		Pointer to the receiving pointer to the value.
 * 
 * \return \c PST_OK when successful, error code otherwise.
 * \note Returned value must be freed by the user if the implementation needs it.
 */
int PARAM_getObject(PARAM *param, const char *source, int prio, unsigned at, void *extra, void **value);

int PARAM_getInvalid(PARAM *param, const char *source, int prio, unsigned at, PARAM_VAL **value);

/**
 * Return parameters value count according to the constraints set. 
 * \param	param		Parameter object.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Constraint for the priority, can be \c PST_PRIORITY_NONE, \c PST_PRIORITY_LOWEST, \c PST_PRIORITY_HIGHEST or greater than 0.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	count		Pointer to receiving pointer to \c PARAM_VAL object. 
 * \return \c PST_OK when successful, error code otherwise.
 */
int PARAM_getValueCount(PARAM *param, const char *source, int prio, int *count);

/**
 * Return the invalid parameters value count according to the constraints set. 
 * See \ref PARAM_addControl. 
 * \param	param		Parameter object.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Constraint for the priority, can be \c PST_PRIORITY_NONE, \c PST_PRIORITY_LOWEST, \c PST_PRIORITY_HIGHEST or greater than 0.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	count		Pointer to receiving pointer to \c PARAM_VAL object. 
 * \return \c PST_OK when successful, error code otherwise.
 */
int PARAM_getInvalidCount(PARAM *param, const char *source, int prio, int *count);

/**
 * This function checks parameter and if all the constraints set (see \ref PARAM_new
 * and \c PARAM_CONSTRAINTS) are satisfied. Constraint values can be concatenated
 * using |. Returned value contains all failed constraints concatenated (|).
 * 
 * \param	param			Parameter object.
 * \param	constraints		Concatenated constraints to be checked.
 * \return 0 if all constraints are satisfied, concatenated constraints otherwise.
 * If failure due to null pointer .etc \c PARAM_INVALID_CONSTRAINT is returned.
 */
int PARAM_checkConstraints(PARAM *param, int constraints);

int PARAM_clearAll(PARAM *param);

int PARAM_clearValue(PARAM *param, const char *source, int priority, int at);

int PARAM_SET_clearValue(PARAM_SET *set, const char *names, const char *source, int priority, int at);

#ifdef	__cplusplus
}
#endif

#endif	/* SET_PARAMETER_H */