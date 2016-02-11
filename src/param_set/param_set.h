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

#ifndef PARAM_SET_H
#define	PARAM_SET_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef PARAM_SET_ERROR_BASE 
#define		PARAM_SET_ERROR_BASE 0x30001 
#endif	
	
enum param_set_err {
	PST_OK = 0,
	PST_INVALID_ARGUMENT = PARAM_SET_ERROR_BASE,
	PST_INVALID_FORMAT,
	PST_INDEX_OVF,
	/** Parameter with the given name does not exist it the given set. */
	PST_PARAMETER_NOT_FOUND,
	/** Parameters value with the given constraints does not exist. */
	PST_PARAMETER_VALUE_NOT_FOUND,
	/** The parameters value count is zero.*/
	PST_PARAMETER_EMPTY,
	/** Parameter added to the set is possible typo. */
	PST_PARAMETER_IS_TYPO,
	/** Parameter added to the set is unknown. */
	PST_PARAMETER_IS_UNKNOWN,
	/** Object extractor function is not implemented. */
	PST_PARAMETER_UNIMPLEMENTED_OBJ,
	PST_OUT_OF_MEMORY,
	PST_NEGATIVE_PRIORITY,
	PST_PARAMETER_INVALID_FORMAT,
	PST_TASK_ZERO_CONSISTENT_TASKS,
	PST_TASK_MULTIPLE_CONSISTENT_TASKS,
	PST_TASK_SET_HAS_NO_DEFINITIONS,
	PST_TASK_SET_NOT_ANALYZED,
	PST_TASK_UNABLE_TO_ANALYZE_PARAM_SET_CHANGED,
	PST_UNDEFINED_BEHAVIOUR,
	PST_UNKNOWN_ERROR,
};	

enum enum_priority {
	PST_PRIORITY_NOTDEFINED = -4,
	PST_PRIORITY_HIGHEST = -3,
	PST_PRIORITY_LOWEST = -2,
	PST_PRIORITY_NONE = -1,
	PST_PRIORITY_VALID_BASE = 0
}; 

enum enum_status {
	PST_VALUE_OK = 0,
	PST_VALUE_INVALID = 1,
};

enum enum_value_index {
	PST_INDEX_LAST = -1,
	PST_INDEX_FIRST = 0,
}; 

typedef struct PARAM_VAL_st PARAM_VAL;	
typedef struct PARAM_st PARAM;
typedef struct PARAM_SET_st PARAM_SET;
typedef enum PARAM_CONSTRAINTS_enum PARAM_CONSTRAINTS;	

typedef struct TASK_st TASK;
typedef struct TASK_DEFINITION_st TASK_DEFINITION;
typedef struct TASK_SET_st TASK_SET;

#define TASK_DEFINITION_MAX_COUNT 32
	
/**
 * This files deals with command-line parameters. An object @ref #paramSet used
 * in functions is set containing all parameters and its values (arguments).
 * The flow of using parameter set is as follows:
 * 1) Create new paramSet. See @ref #paramSet_new.
 * 2) Add control/repair functions. See @ref #paramSet_addControl, @ref #paramSet_isFormatOK and @ref #paramSet_isTypos.
 * 3) Read parameter values. See @ref #paramSet_readFromCMD and @ref #paramSet_readFromFile
 * 4) Work with parameters. See @ref #paramSet_appendParameterByName, @ref #paramSet_removeParameterByName, @ref #paramSet_isSetByName, @ref #paramSet_getIntValueByNameAt, @ref #paramSet_getStrValueByNameAt, @ref #paramSet_getValueCountByName.
 * 5) Free @ref #paramSet. See @ref #paramSet_free.
 */




/**
 * Creates new PARAM_SET object using parameters names.
 * Parameters names are defined using string "{name|alias}*{name|alias}*..." where
 * name - parameters name,
 * alias - alias for the name,
 * '*' - can have multiple values.
 * Exampl: "{h|help}{file}*{o}*{n}".
 * \param	names	pointer to parameter names.
 * \param	set		pointer to recieving pointer to paramSet obj.
 * \return \c PST_OK if successful, error code otherwise.
 */
int PARAM_SET_new(const char *names, PARAM_SET **set);

/**
 * Function to free entire parameter set.
 * \param	set	parameter set object.
 */
void PARAM_SET_free(PARAM_SET *set);

/**
 * Adds several optional functions to a set of parameters. Each function takes
 * the first parameters as c-string value (must not fail if is NULL). See
 * \ref PARAM_addControl, \ref PARAM_getObject and \ref PARAM_setObjectExtractor
 * for more details.
 * 
 * Function \c controlFormat is to control the format. Return 0 if format is ok,
 * error code otherwise.
 * int (*controlFormat)(const char *)
 * 
 * Function \c controlContent is to control the content. Return 0 if content is
 * ok, error code otherwise.
 * int (*controlContent)(const char *)
 * 
 * Function \c convert is used to repair / convert the c-string value before any
 * content or format check is done. Takes two extra parameters for buffer and its
 * size.
 * int (*convert)(const char*, char*, unsigned) extra parameters 
 * 
 * Function \c extractObject used to extract an object from the parameters value.
 * The value set affects the functions \ref PARAM_SET_getObj behaviour. If not
 * set, the default value extracted is the c-string. Extra value for extractor
 * is set as pointer to pointers (void**) and the size of the array is 2. When
 * calling \ref PARAM_SET_getObj both pointers in array are pointing to PARAM_SET
 * itself, when calling \ref PARAM_SET_getObjExtended, the second value is determined
 * by the function call and extra parameter given. Format void **a = {set, extra}.
 * int (*extractObject)(void *, const char *, void**))
 * 
 * \param	set				PARAM_SET object.
 * \param	names			list of names to add the functions.
 * \param	controlFormat	function for format control.
 * \param	controlContent	function for content control.
 * \param	convert			function for argument conversion.
 * \param	extractObject	function for object extraction.
 * \return \c PST_OK if successful, error code otherwise.
 * \note To display error messages look over \c PARAM_SET \c toString functions.
 */
int PARAM_SET_addControl(PARAM_SET *set, const char *names,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		int (*convert)(const char*, char*, unsigned),
		int (*extractObject)(void *, const char *, void**));

/**
 * Appends parameter to the set. Invalid value format or content is not handled
 * as error if it is possible to append it. If parameter can have only one value,
 * it is still possible to add more. Internal format, content or count errors can
 * be detected - see \ref PARAM_SET_isFormatOK. If parameters name dose not exist,
 * function will fail. When parameter is not found it is examined as a typo or
 * unknown (see \ref PARAM_SET_isTypoFailure and \ref PARAM_SET_isUnknown).
 * 
 * See \rfe PARAM_SET_unknownsToString, \ref PARAM_SET_typosToString and 
 * \ref PARAM_SET_invalidParametersToString to display errors.
 * 
 * \param	set			PARAM_SET object.
 * \param	name		parameters name.
 * \param	argument	Parameters value as c-string. Can be NULL.
 * \param	source		Source description as c-string. Can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \return PST_OK if successful, error code otherwise. When parameters is not 
 * part of the set it is pushed to the unknown or typo list and error code 
 * \c PST_PARAMETER_IS_UNKNOWN or \c PST_PARAMETER_IS_TYPO is returned.
 */
int PARAM_SET_add(PARAM_SET *set, const char *name, const char *value, const char *source, int priority);

/**
 * Extracts string from the \c PARAM_SET (see \ref PARAM_SET_add). If object
 * extractor is set, a string value is is always extracted. The user MUST not free
 * the returned string.
 *
 * \param	set			PARAM_SET object.
 * \param	name		parameters name.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	obj			Pointer to receiving pointer to string returned.
 * \return \c PST_OK when successful, error code otherwise. Some more common error
 * codes: 	\c PST_INVALID_ARGUMENT,  \c PST_PARAMETER_NOT_FOUND \c PST_PARAMETER_EMPTY
 * \c PST_PARAMETER_INVALID_FORMAT.
 */
int PARAM_SET_getStr(PARAM_SET *set, const char *name, const char *source, int priority, int at, char **value);

/**
 * Extracts object from the \c PARAM_SET (see \ref PARAM_SET_add,
 * \ref PARAM_SET_addControl). If no object extractor is set, a string value is 
 * returned. By default the user MUST not free the returned object. If a custom
 * object extractor function is used, object must be freed if implementation
 * requires it. 
 * 
 * \param	set			PARAM_SET object.
 * \param	name		parameters name.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	obj			Pointer to receiving pointer to \c object returned.
 * \return \c PST_OK when successful, error code otherwise. Some more common error 
 * codes: 	\c PST_INVALID_ARGUMENT,  \c PST_PARAMETER_NOT_FOUND \c PST_PARAMETER_EMPTY
 * \c PST_PARAMETER_INVALID_FORMAT \c PST_PARAMETER_UNIMPLEMENTED_OBJ.
 */
int PARAM_SET_getObj(PARAM_SET *set, const char *name, const char *source, int priority, int at, void **obj);

/**
 * Same as PARAM_SET_getObj, but the ctxt feed to object extractor contains two 
 * pointers {set, ctxt}.
 * \param	set			PARAM_SET object.
 * \param	name		parameters name.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	ctxt		Pointer to extra context.
 * \param	obj			Pointer to receiving pointer to \c object returned.
 * \return \c PST_OK when successful, error code otherwise. Some more common error 
 * codes: 	\c PST_INVALID_ARGUMENT,  \c PST_PARAMETER_NOT_FOUND \c PST_PARAMETER_EMPTY
 * \c PST_PARAMETER_INVALID_FORMAT \c PST_PARAMETER_UNIMPLEMENTED_OBJ.
 */
int PARAM_SET_getObjExtended(PARAM_SET *set, const char *name, const char *source, int priority, int at, void *ctxt, void **obj);
/**
 * Removes all values from the specified parameter list. Parameter list is defined
 * as "p1,p2,p3 ...".
 * \param set	pointer to parameter set.
 * \param names	parameter name list.
 * \return PST_OK if successful, error code otherwise.
 */
int PARAM_SET_clearParameter(PARAM_SET *set, const char *names);

/**
 * Removes a values specifeid by the constraints from the specified parameter list.
 * Parameter list is defined as "p1,p2,p3 ...".
 * 
 * \param	obj			Pointer to receiving pointer to \c object returned.
 * 
 * \param	set			PARAM_SET object.
 * \param	names		parameters name list.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \return PST_OK if successful, error code otherwise.
 */
int PARAM_SET_clearValue(PARAM_SET *set, const char *names, const char *source, int priority, int at);


/**
 * Counts all the existing parameter values in the list composed by the parameter
 * list and constraints specified.
 * Parameter list is defined as "p1,p2,p3 ...".
 * 
 * \param	set			PARAM_SET object.
 * \param	names		parameters name list.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \param	count		Pointer to integer that are loaded with value count.
 * \return PST_OK if successful, error code otherwise.
 */
int PARAM_SET_getValueCount(PARAM_SET *set, const char *names, const char *source, int priority, int *count);

/**
 * Searches for a parameter by name and checks if its value is present. Even if
 * the values format or content is invalid, true is returned.
 * 
 * \param	set		PARAM_SET object.
 * \param	name	parameter name.
 * \return PST_OK if successful, error code otherwise.
 */
int PARAM_SET_isSetByName(const PARAM_SET *set, const char *name);

/**
 * Controls if the format and content of the parameters is OK.
 * \param	set		PARAM_SET object.
 * \return 0 if format is invalid, greater than 0 otherwise.
 */
int PARAM_SET_isFormatOK(const PARAM_SET *set);

/**
 * Controls if there are some undefined parameters red from command-line or
 * file, similar to the defined ones.
 * \param	set		PARAM_SET object.
 * \return 0 if set contains possible typos, greater than 0 otherwise.
 */
int PARAM_SET_isTypoFailure(const PARAM_SET *set);

/**
 * Controls if there are some undefined parameters red from command-line or file
 * etc.. 
 * \param	set		PARAM_SET object.
 * \return 0 if set contains unknown parameters, greater than 0 otherwise.
 */
int PARAM_SET_isUnknown(const PARAM_SET *set);

/**
 * Reads parameter values from file into predefined parameter set. File must be
 * formatted one parameter (and its possible value) per line.
 * Format of parameters
 * --long		- long parameter without argument.
 * --long <arg>	- long parameter with argument.
 * -i <arg>		- short parameter with argument.
 * -vxn			- bunch of flags.
 * @param[in]		fname	files path.
 * @param[in/out]	set		parameter set.
 */
void PARAM_SET_readFromFile(const char *fname, PARAM_SET *set,  int priority);

/**
 * Reads parameter values from command-line into predefined parameter set.
 * Possible format of parameters.
 * --long		- long parameter without argument.
 * --long <arg>	- long parameter with argument.
 * -i <arg>		- short parameter with argument.
 * -vxn			- bunch of flags.
 * @param[in]		argc	count of command line objects.
 * @param[in]		argv	array of command line objects.
 * @param[in/out]	set		parameter set.
 */
void PARAM_SET_readFromCMD(int argc, char **argv, PARAM_SET *set, int priority);

/**
 * Generates typo failure string
 * \param	set		PARAM_SET object.
 * \param	prefix	prefix to each typo failure string. Can be NULL.
 * \param	buf		receiving buffer.
 * \param	buf_len	receiving buffer size.
 * \return buf if successful, NULL otherwise.
 */
char* PARAM_SET_typosToString(PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);

/**
 * Generates unknown parameter error string.
 * \param	set		PARAM_SET object.
 * \param prefix	prefix to each unknown failure string. Can be NULL.
 * \param buf		receiving buffer.
 * \param buf_len	receiving buffer size.
 * \return buf if successful, NULL otherwise.
 */
char* PARAM_SET_unknownsToString(const PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);

/**
 * Generates a string from invalid parameter list (\see PARAM_SET_addControl). 
 * By default error strings generated contain only error code. To make the messages
 * more human-readable define function \c getErrString that takes error code as 
 * input and returns \c const \c string describing the falure.
 * 
 * \param	set				PARAM_SET object.
 * \param	prefix			prefix for each failure string. Can be NULL.
 * \param	getErrString	Function pointer to make error codes to string. Can be NULL.
 * \param	buf				receiving buffer.
 * \param	buf_len			receiving buffer size.
 * \return buf if successful, NULL otherwise.
 */
char* PARAM_SET_invalidParametersToString(const PARAM_SET *set, const char *prefix, const char* (*getErrString)(int), char *buf, size_t buf_len);

/**
 * Converts PST_ Error codes to string.
 * \param err
 * \return Error string.
 */
const char* PARAM_SET_errorToString(int err);
/**
 * Function that can be used to separate names from string. A function \c isValidNameChar
 * must be defined to separate valid name characters from all kind of separators.
 * Function returns a pointer inside <name_string> that can be used in next iteration
 * to extract next name.
 *
 * \param	name_string		A string full of names that are separated from each other.
 * \param	isValidNameChar	Function that defines valid name characters.
 * \param	buf				Buffer for storing extracted name.
 * \param	len				Buffer size.
 * \param	flags			Can be left to NULL.
 * \return Pointer inside <name_string> that points to next character after name
 * extracted or NULL if end if string reached or no name can be extracted.
 */
const char* extract_next_name(const char* name_string, int (*isValidNameChar)(int), char *buf, short len, int *flags);

#ifdef	__cplusplus
}
#endif

#endif

