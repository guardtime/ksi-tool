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

#include "gt_cmd_control.h"

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


typedef struct paramSet_st paramSet;

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
bool paramSet_new(const char *names,
		int (*printInfo)(const char*, ...), int (*printWarnings)(const char*, ...), int (*printErrors)(const char*, ...),
		paramSet **set);

void paramSet_free(paramSet *set);

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
void paramSet_addControl(paramSet *set, const char *names, FormatStatus (*controlFormat)(const char *), ContentStatus (*controlContent)(const char *), bool (*convert)(const char*, char*, unsigned));

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
void paramSet_readFromCMD(int argc, char **argv, paramSet *set, int priority);

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
void paramSet_readFromFile(const char *fname, paramSet *set,  int priority);

/**
 * Controls if the format and content of the parameters is OK.
 * @param[in]	set	pointer to parameter set.
 * @return true if format and content is OK, false otherwise.
 */
bool paramSet_isFormatOK(const paramSet *set);

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
bool paramSet_appendParameterByName(const char *name, const char *value, const char *source, paramSet *set);

bool paramSet_priorityAppendParameterByName(const char *name, const char *value, const char *source, int priority, paramSet *set);

/**
 * Removes all values from the specified parameter.
 * @param[in]	set	pointer to parameter set.
 * @param[in] name
 */
void paramSet_removeParameterByName(paramSet *set, const char *name);

/**
 * Controls if there are some undefined parameters red from command-line or
 * file, similar to the defined ones.
 * @param[in]	set	pointer to parameter set.
 * @return true if there are some possible typos, false otherwise.
 */
bool paramSet_isTypos(const paramSet *set);

/**
 * Searches for a parameter by name and gives its value count. Even the
 * invalid values are counted.
 *
 * @param[in]	set		pointer to parameter set.
 * @param[in]	name	pointer to parameters name.
 * @param[out]	count	pointer to receiving pointer to variable.
 * @return true if successful false otherwise.
 */
bool paramSet_getValueCountByName(const paramSet *set, const char *name, unsigned *count);

/**
 * Searches for a parameter by name and checks if its value is present. Even if
 * the values format or content is invalid, true is returned.
 * @param[in]	set		pointer to parameter set.
 * @param[in]	name	pointer to parameters name.
 * @return true if parameter and its value exists, false otherwise.
 */
bool paramSet_isSetByName(const paramSet *set, const char *name);

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
bool paramSet_getIntValueByNameAt(const paramSet *set, const char *name, unsigned at, int *value);

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
bool paramSet_getStrValueByNameAt(const paramSet *set, const char *name, unsigned at, char **value);

bool paramSet_getHighestPriorityStrValueByName(const paramSet *set, const char *name, char **value);
bool paramSet_getHighestPriorityIntValueByName(const paramSet *set, const char *name, int *value);

void paramSet_Print(const paramSet *set);
void paramSet_PrintErrorMessages(const paramSet *set);
void paramSet_printUnknownParameterWarnings(const paramSet *set);
void paramSet_printTypoWarnings(const paramSet *set);
void paramSet_printIgnoredLowerPriorityWarnings(const paramSet *set);

int (*paramSet_getErrorPrinter(paramSet *set))(const char*, ...);
int (*paramSet_getWarningPrinter(paramSet *set))(const char*, ...);

#ifdef	__cplusplus
}
#endif

#endif

