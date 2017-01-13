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

#ifndef PARAM_SET_H
#define	PARAM_SET_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>

#ifndef PARAM_SET_ERROR_BASE
#define		PARAM_SET_ERROR_BASE 0x30001
#endif

enum param_set_err {
	PST_OK = 0,
	PST_INVALID_ARGUMENT = PARAM_SET_ERROR_BASE,
	PST_INVALID_FORMAT,
	/* Currently used only if TASK_DEFINITION_MAX_COUNT exceeded. */
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
	/* The function for extracting wildcard is not implemented. */
	PST_PARAMETER_UNIMPLEMENTED_WILDCARD,
	PST_OUT_OF_MEMORY,
	PST_IO_ERROR,
	PST_PRIORITY_NEGATIVE,
	PST_PRIORITY_TOO_LARGE,
	PST_PARAMETER_INVALID_FORMAT,
	PST_TASK_ZERO_CONSISTENT_TASKS,
	PST_TASK_MULTIPLE_CONSISTENT_TASKS,
	PST_TASK_SET_HAS_NO_DEFINITIONS,
	PST_TASK_SET_NOT_ANALYZED,
	PST_TASK_UNABLE_TO_ANALYZE_PARAM_SET_CHANGED,
	PST_WILDCARD_ERROR,
	PST_UNDEFINED_BEHAVIOUR,
	/* Unable to set the combination of command-line parser options. */
	PST_PRSCMD_INVALID_COMBINATION,
	PST_UNKNOWN_ERROR,
};

enum enum_priority {
	PST_PRIORITY_NOTDEFINED = -4,
	
	/* Priority for the most significant priority level. */
	PST_PRIORITY_HIGHEST = -3,
	
	/* Priority for the least significant priority level. */
	PST_PRIORITY_LOWEST = -2,
	
	/* Priority level is not used when filtering elements. */
	PST_PRIORITY_NONE = -1,
	
	/* Count of possible priority values beginning from PST_PRIORITY_VALID_BASE. */
	PST_PRIORITY_COUNT = 0xffff,
	
	/* The valid base for priority level. */
	PST_PRIORITY_VALID_BASE = 0,
		
	/* The valid highest priority level.*/
	PST_PRIORITY_VALID_ROOF = PST_PRIORITY_VALID_BASE + PST_PRIORITY_COUNT - 1 ,
	
	/* To extract values higher than A, use priority A + PST_PRIORITY_HIGHER_THAN*/
	PST_PRIORITY_HIGHER_THAN = PST_PRIORITY_VALID_ROOF + 1,
	
	/* To extract values lower than A, use priority A + PST_PRIORITY_LOWER_THAN*/
	PST_PRIORITY_LOWER_THAN = PST_PRIORITY_HIGHER_THAN + PST_PRIORITY_COUNT,
	
	 /* Priorities greater than that are all invalid. */
	PST_PRIORITY_FIELD_OUT_OF_RANGE = PST_PRIORITY_LOWER_THAN + PST_PRIORITY_COUNT
};

enum enum_status {
	PST_VALUE_OK = 0,
	PST_VALUE_INVALID = 1,
};

enum enum_value_index {
	PST_INDEX_LAST = -1,
	PST_INDEX_FIRST = 0,
};

enum enum_to_str_artifacts {
	PST_TOSTR_NONE = 0x0,
	PST_TOSTR_HYPHEN = 0x02,
	PST_TOSTR_DOUBLE_HYPHEN = 0x04 ,
};

struct PARAM_ATR_st {
	/* Name. */
	char *name;

	/* Alias. */
	char *alias;

	/* c-string value for raw argument. */
	char *cstr_value;

	/* Optional c-string source description, e.g. file, environment. */
	char *source;

	/* Priority level constraint. */
	int priority;

	/* Format status. */
	int formatStatus;

	/* Content status. */
	int contentStatus;
};

#define PST_FORMAT_STATUS_OK 0
#define PST_CONTENT_STATUS_OK 0

typedef struct PARAM_VAL_st PARAM_VAL;
typedef struct PARAM_st PARAM;
typedef struct PARAM_SET_st PARAM_SET;
typedef enum PARAM_CONSTRAINTS_enum PARAM_CONSTRAINTS;
typedef struct PARAM_ATR_st PARAM_ATR;
typedef struct ITERATOR_st ITERATOR;

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
 * Extracts strings from the \c PARAM_SET (see \ref PARAM_SET_add). If object
 * extractor is set, a string value is always extracted. The user MUST not free
 * the returned string.
 *
 * Values are filtered by constraints. If multiple names are
 * specified (e.g. name1,name2,name3) all the parameters are examined. If all the
 * parameters do not contain any values \c PST_PARAMETER_EMPTY is returned, if some
 * values are found and the end of the list is reached \c PST_PARAMETER_VALUE_NOT_FOUND
 * is returned.
 *
 * \param	set			PARAM_SET object.
 * \param	name		parameters name.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	obj			Pointer to receiving pointer to string returned.
 * \return \c PST_OK when successful, error code otherwise. Some more common error
 * codes: 	\c PST_INVALID_ARGUMENT,  \c PST_PARAMETER_NOT_FOUND \c PST_PARAMETER_EMPTY
 * \c PST_PARAMETER_INVALID_FORMAT \c PST_PARAMETER_VALUE_NOT_FOUND.
 */
int PARAM_SET_getStr(PARAM_SET *set, const char *name, const char *source, int priority, int at, char **value);

/**
 * Extracts object from the \c PARAM_SET (see \ref PARAM_SET_add,
 * \ref PARAM_SET_addControl). If no object extractor is set, a string value is
 * returned. By default the user MUST not free the returned object. If a custom
 * object extractor function is used, object must be freed if implementation
 * requires it.
 *
 * Values are filtered by constraints. If multiple names are
 * specified (e.g. name1,name2,name3) all the parameters are examined. If all the
 * parameters do not contain any values \c PST_PARAMETER_EMPTY is returned, if some
 * values are found and the end of the list is reached \c PST_PARAMETER_VALUE_NOT_FOUND
 * is returned.
 *
 * \param	set			PARAM_SET object.
 * \param	name		parameters name.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	obj			Pointer to receiving pointer to \c object returned.
 * \return \c PST_OK when successful, error code otherwise. Some more common error
 * codes: 	\c PST_INVALID_ARGUMENT,  \c PST_PARAMETER_NOT_FOUND \c PST_PARAMETER_EMPTY
 * \c PST_PARAMETER_INVALID_FORMAT \c PST_PARAMETER_UNIMPLEMENTED_OBJ \c PST_PARAMETER_VALUE_NOT_FOUND.
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
 * \c PST_PARAMETER_INVALID_FORMAT \c PST_PARAMETER_UNIMPLEMENTED_OBJ \c PST_PARAMETER_VALUE_NOT_FOUND.
 */
int PARAM_SET_getObjExtended(PARAM_SET *set, const char *name, const char *source, int priority, int at, void *ctxt, void **obj);

/**
 * Extract a values attributes with the given constraints.
 * \param	set			PARAM_SET object.
 * \param	name		parameters name.
 * \param	source		Constraint for the source, can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	atr
 * \return \c PST_OK when successful, error code otherwise. Some more common error
 * codes: 	\c PST_INVALID_ARGUMENT,  \c PST_PARAMETER_NOT_FOUND \c PST_PARAMETER_EMPTY
 * \c PST_PARAMETER_VALUE_NOT_FOUND
 */
int PARAM_SET_getAtr(PARAM_SET *set, const char *name, const char *source, int priority, int at, PARAM_ATR *atr);

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
 * \return PST_OK if successful, error code otherwise. If parameter foes not exists
 * \c PST_PARAMETER_NOT_FOUND is returned.
 */
int PARAM_SET_getValueCount(PARAM_SET *set, const char *names, const char *source, int priority, int *count);

/**
 * Searches for a parameter defined in the list by name and checks if all the
 * parameters have values set. Parameter list is defined as "p1,p2,p3 ...".
 * Even if the values format or content is invalid, true is returned.
 *
 * \param	set		PARAM_SET object.
 * \param	names	parameter name list.
 * \return 1 if yes, 0 otherwise.
 */
int PARAM_SET_isSetByName(const PARAM_SET *set, const char *names);

/**
 * Searches for a parameter defined in the list by name and checks if one of the
 * parameters have values set. Parameter list is defined as "p1,p2,p3 ...".
 * Even if the values format or content is invalid, true is returned.
 *
 * \param	set		PARAM_SET object.
 * \param	names	parameter name list.
 * \return 1 if yes, 0 otherwise.
 */
int PARAM_SET_isOneOfSetByName(const PARAM_SET *set, const char *names);

/**
 * Controls if the format and content of the parameters is OK.
 * \param	set		PARAM_SET object.
 * \return 0 if format is invalid, greater than 0 otherwise.
 */
int PARAM_SET_isFormatOK(const PARAM_SET *set);

/**
 * Controls if the the constraints are violated.
 * \param	set		PARAM_SET object.
 * \return 0 if constraints are OK, 1 otherwise. -1 if an error occurred.
 */
int PARAM_SET_isConstraintViolation(const PARAM_SET *set);
/**
 * Controls if there are some undefined parameters red from command-line or
 * file, similar to the defined ones.
 * \param	set		PARAM_SET object.
 * \return 0 if set contains possible typos, greater than 0 otherwise.
 */
int PARAM_SET_isTypoFailure(const PARAM_SET *set);

int PARAM_SET_isSyntaxError(const PARAM_SET *set);
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
 * Format of parameters.
 * --long		- long parameter without argument.
 * --long <arg>	- long parameter with argument.
 * -i <arg>		- short parameter with argument.
 * -vxn			- bunch of flags.
 * \param	set			parameter set.
 * \param	fname		files path.
 * \param	source		Source description as c-string. Can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \return PST_OK if successful, error code otherwise. If file format is invalid,
 * \c PST_INVALID_FORMAT is returned.
 */
int PARAM_SET_readFromFile(PARAM_SET *set, const char *fname, const char* source, int priority);

/**
 * Reads parameter values from command-line into predefined parameter set.
 * Possible format of parameters.
 * --long		- long parameter without argument.
 * --long <arg>	- long parameter with argument.
 * -i <arg>		- short parameter with argument.
 * -vxn			- bunch of flags.
 * \param	set			parameter set.
 * \param	argc		count of command line strings.
 * \param	argv		array of command line strings.
 * \param	source		Source description as c-string. Can be NULL.
 * \param	priority	Priority that can be \c PST_PRIORITY_VALID_BASE (0) or higher.
 * \return PST_OK if successful, error code otherwise.
 */
int PARAM_SET_readFromCMD(PARAM_SET *set, int argc, char **argv, const char *source, int priority);


int PARAM_SET_setParseOptions(PARAM_SET *set, const char *names, int options);

int PARAM_SET_parseCMD(PARAM_SET *set, int argc, char **argv, const char *source, int priority);

/**
 * Extracts all parameters from \c src known to \c target and appends all the
 * values to the target set. Values are added via \ref PARAM_SET_add and all
 * control and extract functions that are used are from the target set. After
 * successful operation both sets must be freed separately and operations applied
 * to each set does not affect the other one.
 *
 * \param	target			target parameter set.
 * \param	srct			source parameter set.
 * \return PST_OK if successful, error code otherwise.
 */
int PARAM_SET_IncludeSet(PARAM_SET *target, PARAM_SET *src);

char* PARAM_SET_toString(PARAM_SET *set, char *buf, size_t buf_len);

/**
 * Generates typo failure string
 * \param	set		PARAM_SET object.
 * \param	flags	Optional flags to modify output. Use PST_TOSTR_x values.
 * \param	prefix	prefix to each typo failure string. Can be NULL.
 * \param	buf		receiving buffer.
 * \param	buf_len	receiving buffer size.
 * \return buf if successful, NULL otherwise.
 */
char* PARAM_SET_typosToString(PARAM_SET *set, int flags, const char *prefix, char *buf, size_t buf_len);

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

char* PARAM_SET_constraintErrorToString(const PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);

/**
 * Converts PST_ Error codes to string.
 * \param err
 * \return Error string.
 */
const char* PARAM_SET_errorToString(int err);


char* PARAM_SET_syntaxErrorsToString(const PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);

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

/**
 * Extract a key value pair from the line. Supported formats, where <ws> is white space:
 * <wh><key><wh>=<wh><value><wh>
 * <wh><key><wh><value><wh>
 * <wh><key><wh>=<wh>"<value><wh><value>"
 * <wh><key><wh>"<value><wh><value>"
 * <wh><key><wh>"<value><wh><value>"
 *
 * To have <wh> inside value an escape character \ must be used.
 * Some examples:
 *
 * ' key = "a b c d"' result: 'key' 'a b c d'
 * ' key  "a \"b\" c d"' result 'key' 'a "b" c d'
 *
 * \param line
 * \param key
 * \param value
 * \param buf_len
 * \return
 */
int parse_key_value_pair(const char *line, char *key, char *value, size_t buf_len);

/**
 * Read a line from a file (opend with fopen in mode r) ant track the lines.
 * Supported line endings:
 * MAC  \r		CR		0x0d
 * Unix \n		LF		0x0a
 * Win  \r\n	CRLF	0x0d0a
 * \param	file		a file pointer that is us used for reading from.
 * \param	buf			a buffer to store the line.
 * \param	len			size of the buffer.
 * \param	row_pointer	pointer to the row counting value. Initial value pointed to must be 0. If not used can be NULL.
 * \param	read_count	pointer to character counting value. If not used can be NULL.
 * \return Return 0 if successful, EOF if end of file.
 */
int read_line(FILE *file, char *buf, size_t len, size_t *row_pointer, size_t *read_count);

/**
 * Specify a function to expand tokens that contain wildcard character (WC) to array of
 * new values. Characters '?' and '*' are WC. Values containing WC are removed and
 * replaced with the expanded values. See \ref PARAM_expandWildcard,
 * \ref PARAM_setWildcardExpander, \ref PARAM_SET_wildcardExpander and \ref
 * PST_PRSCMD_EXPAND_WILDCARD for more details.
 */
int PARAM_SET_wildcardExpander(PARAM_SET *set, const char *names,
		void *ctx,
		int (*expand_wildcard)(PARAM_VAL *param_value, void *ctx, int *value_shift));

#ifdef	__cplusplus
}
#endif

#endif

