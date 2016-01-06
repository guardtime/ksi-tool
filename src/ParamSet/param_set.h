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

//#include "gt_cmd_control.h"
#include "types.h"
#ifdef	__cplusplus
extern "C" {
#endif

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
 * @param [in] printInfo		Function pointer to handle info messages.
 * @param [in] printWarnings	Function pointer to handle warning messages.
 * @param [in] printErrors		Function pointer to handle error messages.
 * @param[out]	set				pointer to recieving pointer to paramSet obj.
 * @return true if successful false otherwise.
 */
int PARAM_SET_new(const char *names,
		int (*printInfo)(const char*, ...), int (*printWarnings)(const char*, ...), int (*printErrors)(const char*, ...),
		PARAM_SET **set);

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
void PARAM_SET_addControl(PARAM_SET *set, const char *names,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		int (*convert)(const char*, char*, unsigned));

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
 * Controls if the format and content of the parameters is OK.
 * @param[in]	set	pointer to parameter set.
 * @return true if format and content is OK, false otherwise.
 */
int PARAM_SET_isFormatOK(const PARAM_SET *set);

/**
 * Appends parameter to the set. Invalid value format or content is not handled
 * as error if it is possible to append it. If parameter can have only one value,
 * it is still possible to add more. Internal format, content or count errors can
 * be detected - see @ref #paramSet_isFormatOK, @ref #paramSet_isTypos.
 * If parameters name dose not exist, function will fail. Only parameters defined
 * in set can be used.
 * @param[in]	name		parameters value.
 * @param[in]	value	parameters value. Can be NULL.
 * @param[in]	source		description of the parameters source e.g. file name. Can be NULL.
 * @param[in]	set	pointer to parameter set.
 * @return true if successful, false otherwise.
 */
int PARAM_SET_add(PARAM_SET *set, const char *name, const char *value, const char *source, int priority);

/**
 * Removes all values from the specified parameter.
 * @param[in]	set	pointer to parameter set.
 * @param[in] name
 */
int PARAM_SET_clear(PARAM_SET *set, const char *name);

/**
 * Controls if there are some undefined parameters red from command-line or
 * file, similar to the defined ones.
 * @param[in]	set	pointer to parameter set.
 * @return true if there are some possible typos, false otherwise.
 */
int PARAM_SET_isTypos(const PARAM_SET *set);

/**
 * Searches for a parameter by name and gives its value count. Even the
 * invalid values are counted.
 *
 * @param[in]	set		pointer to parameter set.
 * @param[in]	name	pointer to parameters name.
 * @param[out]	count	pointer to receiving pointer to variable.
 * @return true if successful false otherwise.
 */
int PARAM_SET_getValueCountByName(const PARAM_SET *set, const char *name, unsigned *count);

/**
 * Searches for a parameter by name and checks if its value is present. Even if
 * the values format or content is invalid, true is returned.
 * @param[in]	set		pointer to parameter set.
 * @param[in]	name	pointer to parameters name.
 * @return true if parameter and its value exists, false otherwise.
 */
int PARAM_SET_isSetByName(const PARAM_SET *set, const char *name);

/**
 * Search for a integer value from parameter set by name at index. If parameter
 * has format or content error, function fails even if some kind of information
 * is present.
 * @param[in]	set		pointer to parameter set.
 * @param[in]	name	pointer to parameters name.
 * @param[in]	at		parameters index.
 * @param[out]	value	pointer to receiving variable.
 * @return true if successful false otherwise.
 * @note value must not be freed.
 **/
bool PARAM_SET_getIntValueByNameAt(const PARAM_SET *set, const char *name, unsigned at, int *value);

int PARAM_SET_getIntValue(const PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, int *value);
int PARAM_SET_getStrValue(const PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, char **value);
/**
 * Searches for a c string value from parameter set by name at index. If parameter
 * has format or content error, function fails even if some kind of information
 * is present.
 * @param[in] set pointer to parameter set.
 * @param[in] name pointer to parameters name.
 * @param[in] at parameters index.
 * @param[out] value pointer to receiving pointer to variable.
 * @return true if successful false otherwise.
 * @note value must not be freed.
 **/
//bool PARAM_SET_getStrValueByNameAt(const PARAM_SET *set, const char *name, unsigned at, char **value);

//bool paramSet_getHighestPriorityStrValueByName(const PARAM_SET *set, const char *name, char **value);
//bool paramSet_getHighestPriorityIntValueByName(const PARAM_SET *set, const char *name, int *value);

void PARAM_SET_Print(const PARAM_SET *set);
void PARAM_SET_PrintErrorMessages(const PARAM_SET *set);
void PARAM_SET_printUnknownParameterWarnings(const PARAM_SET *set);
void PARAM_SET_printTypoWarnings(const PARAM_SET *set);
void PARAM_SET_printIgnoredLowerPriorityWarnings(const PARAM_SET *set);

int (*PARAM_SET_getErrorPrinter(PARAM_SET *set))(const char*, ...);
int (*PARAM_SET_getWarningPrinter(PARAM_SET *set))(const char*, ...);

#ifdef	__cplusplus
}
#endif

#endif

