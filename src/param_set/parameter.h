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
 * List of command-line parsing options that can be attached to parameters. If
 * command line is parsed:
 * 1) Token is found from command-line that matches parameter (this param) in set.
 * 2) Parameter has a parsing option set.
 * 3) Parser is prepared accordingly (by this param) to parse next token from command-line.
 */
enum PARAM_PARS_OPTIONS_enum {
	PST_PRSCMD_NONE = 0x0000,

	/**
	 * <this param> <next param>
	 * If set the parameter must not have a value and next token from the
	 * command-line is interpreted as next possible argument.
	 */
	PST_PRSCMD_HAS_NO_VALUE = 0x0002,

	/**
	 * <this param> <value> <next param>
	 * If set the parameter must have a single value after its definition.
	 * The next token read from the command-line is interpreted as its value.
	 * See flags below to set some constraints for parsing the values:
	 * \ref PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER and
	 * \ref PST_PRSCMD_BREAK_VALUE_WITH_EXISTING_PARAMETER_MATCH
	 */
	PST_PRSCMD_HAS_VALUE = 0x0004,

	/**
	 * <this param> <value 1> <value 2> .. <value n> <break> [<arg | param>]
	 * If set the parameter must have at least one value. Next tokens read are
	 * interpreted as values. Values are read as long as the break is reached.
	 * By default the break is reached by default next possible command-line
	 * argument (prefix "-" or "--") or end of tokens.
	 * See flags below to set some constraints for parsing the values:
	 * \ref PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER and
	 * \ref PST_PRSCMD_BREAK_VALUE_WITH_EXISTING_PARAMETER_MATCH
	 */
	PST_PRSCMD_HAS_MULTIPLE_INSTANCES = 0x0008,

	/**
	 * If set and the next token has dash in prefix (is a potential parameter
	 * -e.q.-x, -xy, --zzz, ---), the value is interpreted as
	 * next parameter. If used with \ref PST_PRSCMD_HAS_VALUE, the last parameter
	 * is closed and value is set to NULL.
	 * If Used with \ref PST_PRSCMD_HAS_MULTIPLE_INSTANCES and some values are
	 * found parameter is closed, if not NULL is set.
	 * If PST_PRSCMD_COLLECT_LOOSE_PERMIT_END_OF_COMMANDS is set -- will end the
	 * parsing as is potential parameter.
	 */
	PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER = 0x0010,

	/**
	 * If set and the next token matches with existing parameter, the value is
	 * interpreted as next parameter. If used with \ref PST_PRSCMD_HAS_VALUE, the
	 * las parameter is closed and value is set to NULL.
	 * If Used with \ref PST_PRSCMD_HAS_MULTIPLE_INSTANCES and some values are
	 * found parameter is closed, if not NULL is set.
	 */
	PST_PRSCMD_BREAK_VALUE_WITH_EXISTING_PARAMETER_MATCH = 0x0020,


	/**
	 * <this param> <arg | param>
	 * If no parsing options are set, all tokens with prefixes "-" and "--"
	 * are interpreted as command-line parameters. Tokens without the prefixes
	 * are interpreted as values. One parameter can be followed by a single value only.
	 * To have multiple instances multiple parameters must be defined (-a 1 -a 2 ... -a n).
	 * See \ref PST_PRSCMD_HAS_MULTIPLE_INSTANCES for more information to learn
	 * how to define one argument with multiple values.
	 */
	PST_PRSCMD_DEFAULT = 0x0001 | PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER,
	
	/**
	 * Collect all elements that are not bind with another flag and that do not
	 * look like potential parameters. It must be noted that - and -- are still
	 * interpreted as values. To collect everything that do not match with existing
	 * parameter use it together with PST_PRSCMD_COLLECT_LOOSE_FLAGS;
	 */
	PST_PRSCMD_COLLECT_LOOSE_VALUES = 0x0040,
	
	/**
	 * Collect all loose (No typos or unknowns are detected) flags.
	 */
	PST_PRSCMD_COLLECT_LOOSE_FLAGS = 0x0080,
	
	/**
	 * Collect everything after the parsing is closed. See \c PST_PRSCMD_CLOSE_PARSING
	 * to see how to enable the closing of command line parsing. 
	 */
	PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED = 0x0100,
	
	
	/**
	 * Enable loose element collector to stop command line parsing and redirect
	 * all tokens to parameter pointed with loose element flag. Parameter -- is used
	 * to mark the end of commands. It must be used with PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED.
	 */
	PST_PRSCMD_CLOSE_PARSING = 0x0200,
	
	/**
	 * Collect has lower priority.
	 */
	PST_PRSCMD_COLLECT_HAS_LOWER_PRIORITY = 0x0400,
	
	/**
	 * New flag to ignore a parameter from command line to be parsed.
	 * Parameter can be added from code.
	 */
	PST_PRSCMD_HAS_NO_FLAG = 0x0800,
	
	/**
	 * With this parameter no typos are generated for a parameter.
	 */
	PST_PRSCMD_NO_TYPOS = 0x1000,

	/**
	 * With this parameter only the highest priority last parameters format is
	 * checked in functions \c PARAM_SET_isFormatOK and
	 * \c PARAM_SET_invalidParametersToString. The error status is set in spite
	 * of the given flag flag.
	 */
	PST_PRSCMD_FORMAT_CONTROL_ONLY_FOR_LAST_HIGHST_PRIORITY_VALUE = 0x2000,

	/**
	 * If set, searches for the wildcard characters (WC) * and ?. If found, the
	 * token containing the WC is removed and replaced with the matching tokens.
	 * The WC expanding is performed by WC expander function that MUST BE configured.
	 * See \ref PARAM_expandWildcard, \ref PARAM_setWildcardExpander and \ref
	 * PARAM_SET_wildcardExpander for more details how to specify the implementation.
	 * If not implemented parsing fails.
	 */
	PST_PRSCMD_EXPAND_WILDCARD = 0x4000,
	
	PST_PRSCMD_COLLECT_LIMITER_ON       = 0x00008000,
	PST_PRSCMD_COLLECT_LIMITER_1X       = 0x00010000,
	PST_PRSCMD_COLLECT_LIMITER_MAX_MASK = 0xffff0000
};

/**
 * Create a new empty parameter with constraints <flags> (see \c PARAM_FLAGS).
 *  
 * \param	flagName	The parameters name.
 * \param	flagAlias	The alias for the name. Can be NULL.
 * \param	constraint	Constraints for the parameter and its values (see \c PARAM_FLAGS).
 * \param	pars_opt	Parsing Options.
 * \param	newObj		Pointer to receiving pointer to new object.
 * \return \c PST_OK when successful, error code otherwise.
 * 
 * \note It must be noted that it is possible to add multiple parameters to the
 * list using function \ref PARAM_addArgument no matter what the constraints are.
 * When parameter constraints are broken its validity check fails - see
 * \ref PARAM_checkConstraints.
 */
int PARAM_new(const char *flagName, const char *flagAlias, int constraint, int pars_opt, PARAM **newObj);

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
 * Check if parsing parameter or a group is set (can be concatenated together
 * with |). See See \ref PARAM_PARS_OPTIONS_enum and \ref PARAM_setParseOption
 * and \ref PARAM_SET_parseCMD for more information.
 *
 * \param param		A parameter object.
 * \param state		A state to be controlled.
 * \return \c PST_OK when successful, error code otherwise.
 */
int PARAM_isParsOptionSet(PARAM *param, int state);

/**
 * Add parsing options to parameter. See \ref PARAM_PARS_OPTIONS_enum and \ref
 * PARAM_SET_parseCMD for more information.
 *
 * \param param		A parameter object.
 * \param state		A state to be sett.
 * \return \c PST_OK when successful, error code otherwise.
 * Error \c PST_PRSCMD_INVALID_COMBINATION is returned if conflicting parsing flag
 * combination is feed to the function.
 */
int PARAM_setParseOption(PARAM *obj, int option);

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
 */
int PARAM_getValue(PARAM *param, const char *source, int prio, int at, PARAM_VAL **value);

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
int PARAM_getObject(PARAM *param, const char *source, int prio, int at, void *extra, void **value);

int PARAM_getInvalid(PARAM *param, const char *source, int prio, int at, PARAM_VAL **value);

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
int PARAM_checkConstraints(const PARAM *param, int constraints);

int PARAM_clearAll(PARAM *param);

int PARAM_clearValue(PARAM *param, const char *source, int priority, int at);

int PARAM_SET_clearValue(PARAM_SET *set, const char *names, const char *source, int priority, int at);

char* PARAM_constraintErrorToString(const PARAM *param, const char *prefix, char *buf, size_t buf_len);

/**
 * A function to expand tokens that contain wildcard character (WC) to array of
 * new values. Characters '?' and '*' are WC. Values containing WC are removed and
 * replaced with the expanded values.
 *
 *
 * int expand_wildcard(PARAM_VAL *param_value, void *ctx, int *value_shift)
 *   param-value - Parameter that contains value string with WC in it.
 *   ctx - additional context for the function (e.g. some string massive or database)
 *   value_shift - exact count of values extracted.
 *   returns: PST_OK if successful, error code otherwise.
 *   expand_wildcard function must not remove param_value from the linked list
 *   as it is done by higher level functions. New values must be appended right
 *   after the param_value.
 *
 * The function can be activated by manual call to PARAM_expandWildcard or by
 * setting PST_PRSCMD_EXPAND_WILDCARD for the parameter and calling
 * PARAM_SET_parseCMD.
 *
 * \param obj - Parameter OBJ where all values are examined for WC and if found replaced with expanded values.
 * \param ctx - Additional context for expanding the WC.
 * \param expand_wildcard - A function that expands the strings containing WC.
 * \return PST_OK if successful, error code otherwise.
 */
int PARAM_setWildcardExpander(PARAM *obj, void *ctx, int (*expand_wildcard)(PARAM_VAL *param_value, void *ctx, int *value_shift));

/**
 * Expand the values containing wildcard characters (WC). Before using WC expander
 * function must be configured. See \ref PARAM_setWildcardExpander.
 * \param obj - Parameter OBJ where all values are examined for WC and if found replaced with expanded values.
 * \param The count of new values inserted.
 * \return PST_OK if successful, error code otherwise.
 */
int PARAM_expandWildcard(PARAM *param, int *count);

char* PARAM_toString(const PARAM *param, char *buf, size_t buf_len);

#ifdef	__cplusplus
}
#endif

#endif	/* SET_PARAMETER_H */