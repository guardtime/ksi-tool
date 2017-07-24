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

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Parameter value object. It holds values in list in order of insertion. Values
 * can be filtered with constraints - source, priority and index. Values are held
 * in linked list.
 */
typedef struct PARAM_VAL_st PARAM_VAL;

/**
 * List of priority constraints used for filtering the parameter values.
 */
enum PST_PRIORITY_enum {
	/** Priority equal or less than this value is undefined.*/
	PST_PRIORITY_NOTDEFINED = -4,

	/** Priority for the most significant priority level. */
	PST_PRIORITY_HIGHEST = -3,

	/** Priority for the least significant priority level. */
	PST_PRIORITY_LOWEST = -2,

	/** Priority level is not used when filtering elements. */
	PST_PRIORITY_NONE = -1,

	/** Count of possible priority values beginning from #PST_PRIORITY_VALID_BASE. */
	PST_PRIORITY_COUNT = 0xffff,

	/** The valid base for priority level. */
	PST_PRIORITY_VALID_BASE = 0,

	/** The valid highest priority level.*/
	PST_PRIORITY_VALID_ROOF = PST_PRIORITY_VALID_BASE + PST_PRIORITY_COUNT - 1 ,

	/** To extract values higher than A, use priority A + #PST_PRIORITY_HIGHER_THAN*/
	PST_PRIORITY_HIGHER_THAN = PST_PRIORITY_VALID_ROOF + 1,

	/** To extract values lower than A, use priority A + #PST_PRIORITY_LOWER_THAN*/
	PST_PRIORITY_LOWER_THAN = PST_PRIORITY_HIGHER_THAN + PST_PRIORITY_COUNT,

	 /** Priorities greater than that are all invalid. */
	PST_PRIORITY_FIELD_OUT_OF_RANGE = PST_PRIORITY_LOWER_THAN + PST_PRIORITY_COUNT
};

/**
 * List of special index macros used for filtering the parameter values.
 */
enum PST_INDEX_enum {
	/** Return the last value found with specified constraints. */
	PST_INDEX_LAST = -1,

	/** Return the first value found with specified constraints. */
	PST_INDEX_FIRST = 0,
};

/**
 * Creates a new #PARAM_VAL object. If \c new is pointer to \c NULL, a new
 * object is created. If \c new points to existing #PARAM_VAL object, a new
 * value is created and appended to the end list.
 * \param value		Value as c-string. Can be \c NULL.
 * \param source	Describes the source e.g. file name or environment variable. Can be \c NULL.
 * \param priority	Priority of the parameter, must be positive.
 * \param new		Pointer to receiving pointer or pointer to existing value.
 * \return #PST_OK when successful, error code otherwise.
 * \note The root value must be freed by calling #PARAM_VAL_free.
 */
int PARAM_VAL_new(const char *value, const char* source, int priority, PARAM_VAL **new);

/**
 * Extracts element from \c target list (see #PARAM_VAL_getElement) and
 * inserts \c obj right after the specified value.
 *
 * \param	target		The first #PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	obj			New obj to be inserted.
 * \return \c PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_insert(PARAM_VAL *target, const char* source, int priority, int at, PARAM_VAL *obj);

/**
 * Free parameter value object.
 * \param rootValue		The first #PARAM_VAL link in the linked list.
 */
void PARAM_VAL_free(PARAM_VAL *rootValue);

/**
 * Extracts element from #PARAM_VAL list. Element is extracted from given
 * priority level at the given index. If \c at is #PST_INDEX_LAST, the last element is
 * extracted. If priority is #PST_PRIORITY_NONE every element is counted and returned
 * at given index. If priority is #PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher, only
 * elements at the given priority are counted and extracted. For example if list contains
 * 2 lower priority values followed by higher priority value at position 3 and at
 * is 0, function returns the last value (but the  first value matching the priority).
 * Priority #PST_PRIORITY_LOWEST and #PST_PRIORITY_HIGHEST are used to extract only
 * the lowest and the highest priority values. Use #PST_PRIORITY_HIGHER_THAN + \c priority
 * or #PST_PRIORITY_LOWER_THAN + \c priority to extract values with higher or lower
 * priority accordingly.
 *
 * \param	rootValue	The first #PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	val			Pointer to receiving pointer.
 * \return #PST_OK when successful, error code otherwise. Some more common error
 * codes: #PST_INVALID_ARGUMENT and #PST_PARAMETER_VALUE_NOT_FOUND.
 */
int PARAM_VAL_getElement(PARAM_VAL *rootValue, const char* source, int priority, int at, PARAM_VAL** val);

/**
 * This function pops an element out of the \c rootValue and outputs is via \c val.
 *
 * \param	rootValue	The pointer to pointer to the first #PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	val			Pointer to receiving pointer of poped element.
 * \return #PST_OK when successful, error code otherwise.
 * \attention After successful poping \c rootValue will point to the first element
 * in the list. Thats is, if not the \b root value link is feed to the function
 * it will be changed to the first element in list.
 * \attention User is responsible to free the object pointed by \c val.
 */
int PARAM_VAL_popElement(PARAM_VAL **rootValue, const char* source, int priority, int at, PARAM_VAL** val);

/**
 * Getter method for #PARAM_VAL data fields.
 * \param	rootValue	#PARAM_VAL object.
 * \param	value		Pointer to receiving pointer to value. Can be \c NULL.
 * \param	source		Pointer to receiving pointer to source. Can be \c NULL.
 * \param	priority	Pointer to integer where the priority is written.  Can be \c NULL.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_VAL_extract(PARAM_VAL *rootValue, const char **value, const char **source, int *priority);

/**
 * Count the values with given constraints. If there are no values matching the
 * constraints 0 is returned.
 * \param	rootValue	The first #PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	prio		Constraint for the priority.
 * \param	count		Pointer to integer receiving the count value.
 * \return #PST_OK when successful, error code otherwise..
 */
int PARAM_VAL_getElementCount(PARAM_VAL *rootValue, const char *source, int prio, int *count);

/**
 * Count the invalid values with given constraints. If there are no values matching the
 * constraints 0 is returned.
 * \param	rootValue	The first #PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	prio		Constraint for the priority.
 * \param	count		Pointer to integer receiving the count value.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_VAL_getInvalidCount(PARAM_VAL *rootValue, const char *source, int prio, int *count);

/**
 * Extract the next priority level via \c nextPrio compared with \c current priority.
 * If \c current is set to #PST_PRIORITY_HIGHEST or #PST_PRIORITY_LOWEST the lowest
 * or the highest value is returned. If \c current is set to other values lower than
 * #PST_PRIORITY_VALID_BASE an error is returned. If current is set to a value
 * that is exactly the priority of the highest value error code
 * #PST_PARAMETER_VALUE_NOT_FOUND is returned, as there is no parameter that has
 * priority level higher that \c current.
 *
 * \param	rootValue	The first #PARAM_VAL link in the linked list.
 * \param	current		Priority value.
 * \param	nextPrio	Next highest priority or the lowest or highest.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_VAL_getPriority(PARAM_VAL *rootValue, int current, int *nextPrio);

/**
 * Getter method for format and content status. #PARAM_addValue
 * \param	rootValue	#PARAM_VAL object.
 * \param	format		Pointer to integer to store the format status.
 * \param	content		Pointer to integer to store the content status.
 * \return #PST_OK when successful, error code otherwise.
 * \see #PARAM_addValue and #PARAM_addControl.
 */
int PARAM_VAL_getErrors(PARAM_VAL *rootValue, int *format, int* content);

/**
 * Same as #PARAM_VAL_getElement but returns only values with invalid format or
 * content status.
 * \param	rootValue	The first #PARAM_VAL link in the linked list.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	val			Pointer to receiving pointer.
 * \return #PST_OK when successful, error code otherwise. Some more common error
 * codes: #PST_INVALID_ARGUMENT and #PST_PARAMETER_VALUE_NOT_FOUND.
 */
int PARAM_VAL_getInvalid(PARAM_VAL *rootValue, const char* source, int priority, int at, PARAM_VAL** val);

/**
 * Generates #PARAM_VAL object description string. Useful for debugging.
 * \param	value	#PARAM_VAL object.
 * \param	buf		Receiving buffer.
 * \param	buf_len	Receiving buffer size.
 * \return \c buf if successful, \c NULL otherwise.
 */
char* PARAM_VAL_toString(const PARAM_VAL *value, char *buf, size_t buf_len);



#ifdef	__cplusplus
}
#endif

#endif	/* SET_PARAM_VALUE_H */