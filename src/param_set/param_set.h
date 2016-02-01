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
	PST_PARAMETER_NOT_FOUND,
	PST_PARAMETER_VALUE_NOT_FOUND,
	PST_PARAMETER_EMPTY,
	PST_PARAMETER_IS_TYPO,
	PST_PARAMETER_IS_UNKNOWN,
	PST_PARAMETER_UNIMPLEMENTED_OBJ,
	PST_OUT_OF_MEMORY,
	PST_NEGATIVE_PRIORITY,
	PST_PARAMETER_INVALID_FORMAT,
	PST_TASK_ZERO_CONSISTENT_TASKS,
	PST_TASK_MULTIPLE_CONSISTENT_TASKS,
	PST_TASK_SET_HAS_NOD_DEFINITIONS,
	PST_TASK_SET_NOT_ANALYZED,
	PST_TASK_DEFINITION_NOT_DEFINED,
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
 * Creates new #paramSet object using parameters names definition.
 * Parameters names are defined using string "{name|alias}*{name|alias}*..." where
 * name - parameters name,
 * alias - alias for the name,
 * '*' - can have multiple values.
 * Exampl: "{h|help}{file}*{o}*{n}".
 * @param[in]	names			pointer to parameter names.
 * @param[out]	set				pointer to recieving pointer to paramSet obj.
 * @return true if successful false otherwise.
 */
int PARAM_SET_new(const char *names, PARAM_SET **set);

void PARAM_SET_free(PARAM_SET *set);

/**
 * Adds format and content control to a set of parameters. Also parameter conversion
 * function can be added. The set is defined using strings "{name}".
 * For example {a}{h}{file}.
 * Function pointers:
 * FormatStatus controlFormat(const char *value) - takes parameters value and
 * returns its format status.
 * ContentStatus controlContent(const char *value) - takes parameters value and
 * returns its content status.
 * bool control(const char* value_in, char* buf, unsigned buf_len) - takes value
 * and puts its converted value into buf. Returns true if successful, false otherwise.
 * @param[in]	set				pointer to parameter set.
 * @param[in]	names			parameters names.
 * @param[in]	controlFormat	function for format control.
 * @param[in]	controlContent	function for content control.
 * @param[in]	convert			function for argument conversion.
 */
int PARAM_SET_addControl(PARAM_SET *set, const char *names,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		int (*convert)(const char*, char*, unsigned),
		int (*extractObject)(const char *, void**));

/**
 * Appends parameter to the set. Invalid value format or content is not handled
 * as error if it is possible to append it. If parameter can have only one value,
 * it is still possible to add more. Internal format, content or count errors can
 * be detected - see @ref #paramSet_isFormatOK, @ref #paramSet_isTypos.
 * If parameters name dose not exist, function will fail. Only parameters defined
 * in set can be used. When parameter is not found it is examined as a typo or unknown
 * flag and pushed to the list according to the examination result. See @ref PARAM_SET_isTypos. 
 * @param[in]	name		parameters value.
 * @param[in]	value	parameters value. Can be NULL.
 * @param[in]	source		description of the parameters source e.g. file name. Can be NULL.
 * @param[in]	set	pointer to parameter set.
 * @return PST_OK if successful, error code otherwise. When parameters is not 
 * part of the set it is pushed to the unknown or typo list and error code 
 * \c PST_PARAMETER_IS_UNKNOWN or \c PST_PARAMETER_IS_TYPO is returned.
 */
int PARAM_SET_add(PARAM_SET *set, const char *name, const char *value, const char *source, int priority);

int PARAM_SET_getObj(PARAM_SET *set, const char *name, const char *source, int priority, int at, void **obj);

/**
 * Removes all values from the specified parameter.
 * @param[in]	set	pointer to parameter set.
 * @param[in] name
 */
int PARAM_SET_clearParameter(PARAM_SET *set, const char *names);

int PARAM_SET_clearValue(PARAM_SET *set, const char *names, const char *source, int priority, int at);

int PARAM_SET_getValueCount(PARAM_SET *set, const char *names, const char *source, int priority, int *count);

/**
 * Searches for a parameter by name and checks if its value is present. Even if
 * the values format or content is invalid, true is returned.
 * @param[in]	set		pointer to parameter set.
 * @param[in]	name	pointer to parameters name.
 * @return true if parameter and its value exists, false otherwise.
 */
int PARAM_SET_isSetByName(const PARAM_SET *set, const char *name);

/**
 * Controls if the format and content of the parameters is OK.
 * @param[in]	set	pointer to parameter set.
 * @return true if format and content is OK, false otherwise.
 */
int PARAM_SET_isFormatOK(const PARAM_SET *set);

/**
 * Controls if there are some undefined parameters red from command-line or
 * file, similar to the defined ones.
 * @param[in]	set	pointer to parameter set.
 * @return true if there are some possible typos, false otherwise.
 */
int PARAM_SET_isTypoFailure(const PARAM_SET *set);

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

char* PARAM_SET_typosToString(PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);

char* PARAM_SET_unknownsToString(const PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);

char* PARAM_SET_invalidParametersToString(const PARAM_SET *set, const char *prefix, const char* (*getErrString)(int), char *buf, size_t buf_len);

int PARAM_SET_getValueCount(PARAM_SET *set, const char *names, const char *source, int priority, int *count);


#ifdef	__cplusplus
}
#endif

#endif

