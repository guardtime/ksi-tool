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

#include "param_value.h"


#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Parameter object that has a name and list of values. Accessing values in
 * sequence (index is increased) is optimized. Contains optional functionality
 * for format and content checking.
 */
typedef struct PARAM_st PARAM;

typedef enum PARAM_PARS_OPTIONS_enum PARAM_PARS_OPTIONS;

typedef enum PARAM_CONSTRAINTS_enum PARAM_CONSTRAINTS;

typedef struct PARAM_ATR_st PARAM_ATR;


enum PARAM_CONSTRAINTS_enum {
	/** Only a single value is allowed. */
	PARAM_SINGLE_VALUE = 0x0002,

	/** Only a single value is allowed at each priority level. */
	PARAM_SINGLE_VALUE_FOR_PRIORITY_LEVEL = 0x0004,

	/**/
	PARAM_INVALID_CONSTRAINT = 0x8000,
};

/**
 * Object that is meant to be used with function #PARAM_SET_getAtr to get details
 * about parameter value.
 */
struct PARAM_ATR_st {
	/** Name. */
	char *name;

	/** Alias. */
	char *alias;

	/** c-string value. */
	char *cstr_value;

	/** Optional c-string source description, e.g. file, environment. */
	char *source;

	/** Priority level constraint. */
	int priority;

	/** Format status. */
	int formatStatus;

	/** Content status. */
	int contentStatus;
};

/**
 * This is the list of #PARAM parsing options that will affect how the parameters
 * are parsed from the command-line. Parsing options will guide how function
 * #PARAM_SET_parseCMD fills #PARAM_SET internal #PARAM values.
 *
 * (Potential) parameter for a command-line parser is a token that has dash in prefix
 * (e.q. "-x", "-xy", "--zzz", "---"). Unknown parameter is parameter that is not
 * specified in #PARAM_SET parameter list. Note that by using parsing options it is
 * still possible to interpret "parameters" as values (e.g. parameter "--conf"
 * takes configuration file with odd name "--conf" as input "--conf --conf").
 *
 * Terms used in parsing option descriptions:
 * - \c curr - Current parameter whose parsing options are active.
 * - \c known or \c unknown - A known and unknown parameter accordingly.
 * - \c param - Both \c known and \c unknown parameter.
 * - \c bindv - A value that belongs to \c curr and is bound with it.
 * - <TT>loose value</tt> or \c parameter - A value that is not bound with \c param or \c unknown parameter accordingly.
 * - \c token - Can be interpreted as anything.
 *
 * Usage examples:
 * \code{.txt}
 *
 * 1) Default behaviour.
 *   cmd: "-a -a x z -b y"
 *   PARAM_SET = {a, b}
 *     a = PST_PRSCMD_DEFAULT
 *     b = PST_PRSCMD_DEFAULT
 *   PARSE cmd:
 *     a = {NULL, x}
 *     b = {y}
 *     unknown/typo = {z}
 *
 * 2) Has a value and has not a value.
 *   cmd = "-a -a -b -a -b -b -b v -b"
 *   PARAM_SET = {a, b, c}
 *     a = PST_PRSCMD_HAS_NO_VALUE
 *     b = PST_PRSCMD_HAS_VALUE
 *     c = PST_PRSCMD_DEFAULT
 *   PARSE cmd:
 *     a = {NULL, NULL}
 *     b = {"-a", "-b", "v", NULL}
 *     c = {}
 *
 * 3) Has value with break.
 *   cmd = "-a y -a -b -x -b -a z"
 *   PARAM_SET = {a, b}
 *     a = PST_PRSCMD_HAS_VALUE | PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER
 *     b = PST_PRSCMD_HAS_VALUE | PST_PRSCMD_BREAK_WITH_EXISTING_PARAMETER_MATCH
 *   PARSE cmd:
 *     a = {"y", NULL, "z"}
 *     b = {"-x", NULL}
 *
 * 4) Value with and without typos.
 *   cmd = "xbc xde"
 *   PARAM_SET = {abc, bde}
 *     abc = PST_PRSCMD_HAS_NO_VALUE | PST_PRSCMD_NO_TYPOS
 *     bde = PST_PRSCMD_HAS_NO_VALUE
 *   PARSE cmd:
 *     unknown = {xbc}
 *     typo = {xde}
 *     abc = {}
 *     bde = {}
 *
 * 5) Has value sequence with break.
 *   cmd = "-a x y z -c v -a -a w -b -x -y -c w"
 *   PARAM_SET = {a, b, c}
 *     a = PST_PRSCMD_HAS_VALUE_SEQUENCE | PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER
 *     b = PST_PRSCMD_HAS_VALUE_SEQUENCE | PST_PRSCMD_BREAK_WITH_EXISTING_PARAMETER_MATCH
 *     c = PST_PRSCMD_DEFAULT
 *   PARSE cmd:
 *     a = {"x", "y", "z", NULL, "w"}
 *     b = {"-x", "-y"}
 *     c = {"v", "w"}
 *
 * 6) Collect values not bound with any parameter.
 *   cmd = "-i x -b y z -a w"
 *   PARAM_SET = {i, a, b}
 *     i = PST_PRSCMD_HAS_VALUE | PST_PRSCMD_COLLECT_LOOSE_VALUES
 *     a = PST_PRSCMD_HAS_VALUE
 *     b = PST_PRSCMD_HAS_NO_VALUE
 *   PARSE cmd:
 *     i = {"x", "y", "z"}
 *     a = {"w"}
 *     b = {NULL}
 *
 * 7) Collect all loose values and enable parameter "--" to close parsing and redirect all tokens to parameter "j".
 *   cmd = "x y z -i v -j w -i -- -- -- - -i -j"
 *   PARAM_SET = {i, j}
 *     i = PST_PRSCMD_HAS_VALUE | PST_PRSCMD_COLLECT_LOOSE_VALUES
 *     j = PST_PRSCMD_HAS_VALUE | PST_PRSCMD_CLOSE_PARSING | PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED
 *   PARSE cmd:
 *     i = {"x", "y", "z", "v", "--"}
 *     j = {"w", "--", "-", "-i", "-j"}
 *
 * 8) Create a hidden parameter (that can not be explicitly added from the command-line) to collect all tokens after "--".
 *   cmd = "-i x -a y -- z w v"
 *   PARAM_SET = {a, i}
 *     a = PST_PRSCMD_HAS_VALUE
 *     i = PST_PRSCMD_HAS_VALUE | PST_PRSCMD_CLOSE_PARSING | PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED | PST_PRSCMD_HAS_NO_FLAG | PST_PRSCMD_NO_TYPOS
 *   PARSE cmd:
 *     a = {"y"}
 *     i = {"z", "w", "v"}
 *     unknown/typo = {"-i", "x"}
 *
 * 9) Create a parameter that has no flag but can collect up to 3 values that are not bound with any parameter.
 *   cmd = "x -a v y -i z w"
 *   PARAM_SET = {a, i}
 *     a = PST_PRSCMD_HAS_VALUE
 *     i = PST_PRSCMD_HAS_VALUE | PST_PRSCMD_HAS_NO_FLAG | PST_PRSCMD_NO_TYPOS | PST_PRSCMD_COLLECT_LOOSE_VALUES | PST_PRSCMD_COLLECT_LIMITER_BREAK_ON | ((PST_PRSCMD_COLLECT_LIMITER_1X * 3) & PST_PRSCMD_COLLECT_LIMITER_MAX_MASK)
 *   PARSE cmd:
 *     a = {"v"}
 *     i = {"x", "y", "z"}
 *     unknown/typo = {"-i", "w"}
 *
 * 10) Implement and apply Wildcard expander that selects a value from the list:
 *   cmd = "-w qwe -w *c -w p* -w *e"
 *   list = {"abc", "qwe", "xbc", "aee"}
 *   PARAM_SET = {w}
 *     w = PST_PRSCMD_HAS_VALUE | PST_PRSCMD_EXPAND_WILDCARD
 *   PARSE cmd:
 *     w = {"qwe", ("abc", "xbc"),(),("qwe", "aee")}
 *
 * \endcode
 *
 * \see #PARAM_SET_new and #PARAM_SET_setParseOptions.
 */
enum PARAM_PARS_OPTIONS_enum {
	PST_PRSCMD_NONE = 0x0000,

	/**
	 * If set the parameter must not have a value and next token from the
	 * command-line is interpreted as next possible parameter (or unknown token).
	 * The value is set to \c NULL, that indicates the occurrence of the parameter.
	 * \code{.txt}
	 * curr token token ...
	 * \endcode
	 */
	PST_PRSCMD_HAS_NO_VALUE = 0x0002,

	/**
	 * If set the parameter takes next token as its value even if it equals
	 * known / unknown parameter. If there is no next value to be read or a
	 * break occurred (see \c notes), the value is set to \c NULL indicating that
	 * the parameter was specified but it was not possible to get its value.
	 *
	 * \code{.txt}
	 * // Without a break.
	 * curr bindv token ...
	 *
	 * // With potential parameter break.
	 * curr param token ...
	 *
	 * // With known parameter break.
	 * curr known token ...
	 * \endcode
	 *
	 * \note Using additional options #PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER
	 * or #PST_PRSCMD_BREAK_WITH_EXISTING_PARAMETER_MATCH it is possible to
	 * interpret the next token as parameter.
	 */
	PST_PRSCMD_HAS_VALUE = 0x0004,

	/**
	 * If set the parameter takes all the next tokens as its values even if some
	 * values equal known / unknown parameters. If there is no next value to
	 * be read or a break occurred (see \c notes), the parsing of current parameter
	 * is closed. If during the parsing there were no values bound with the current
	 * parameter, its value is set to \c NULL, otherwise the parsing is closed
	 * without adding any new data to the #PARAM.
	 *
	 * \code{.txt}
	 * // Without a break.
	 * curr bindv bindv ... bindv
	 *
	 * // With potential parameter break.
	 * curr bindv bindv ... param token ...
	 *
	 * // With known parameter break.
	 * curr bindv bindv ... known token ...
	 * \endcode
	 * \note Using additional options #PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER
	 * or #PST_PRSCMD_BREAK_WITH_EXISTING_PARAMETER_MATCH it is possible to break
	 * the parsing of the current parameter and interpret the next token as parameter.
	 */
	PST_PRSCMD_HAS_VALUE_SEQUENCE = 0x0008,

	/**
	 * If parameter has dash in prefix, it is a potential parameter that can even
	 * be unknown parameter not specified in #PARAM_SET (e.q. "-x", "-xy",
	 * "--zzz", "---").
	 *
	 * \note Note that  when #PST_PRSCMD_CLOSE_PARSING is set for <b>any
	 * parameter</b> (not just the current one) in the set, "--" is
	 * considered as legal known parameter that will perform the break.
	 */
	PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER = 0x0010,

	/**
	 * If set and the next token matches with <b>existing parameter</b>, the
	 * value is interpreted as next parameter.
	 * \todo: Check if bunch of known flags are triggering the break.
	 *
	 * \note Note that  when #PST_PRSCMD_CLOSE_PARSING is set for <b>any
	 * parameter</b> (not just the current one) in the set, "--" is
	 * considered as known parameter that will perform the break.
	 */
	PST_PRSCMD_BREAK_WITH_EXISTING_PARAMETER_MATCH = 0x0020,


	/**
	 * The default parsing options interpret each token as potential parameter
	 * which can bind next token as its single value.
	 *
	 * \code{.txt}
	 * curr bindv param param token ...
	 * \endcode
	 *
	 */
	PST_PRSCMD_DEFAULT = 0x0001 | PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER,

	/**
	 * Collect all tokens that are not bound with some parameter and that do not
	 * look like potential parameters. If used together with
	 * #PST_PRSCMD_COLLECT_LIMITER_BREAK_ON, collect maximum count is limited.
	 * \note Note that by default, if not bound with some parameter, "-" and "--"
	 * are considered as loose values. But in case when #PST_PRSCMD_CLOSE_PARSING
	 * is set for <b>any parameter</b> (not just the current one) on the set, "--"
	 * is considered as known parameter that is not collected as a loose value.
	 */
	PST_PRSCMD_COLLECT_LOOSE_VALUES = 0x0040,

	/**
	 * Collect all loose (unknown or misspelled) parameters (e.q. "-x", "-xy", "--zzz",
	 * "---"). If used together with #PST_PRSCMD_COLLECT_LIMITER_BREAK_ON, collect
	 * maximum count is limited.
	 * \attention No typos or unknowns are detected as all unknown or misspelled
	 * parameters are redirected to parameters with the this options set.
	 */
	PST_PRSCMD_COLLECT_LOOSE_FLAGS = 0x0080,

	/**
	 * Using with #PST_PRSCMD_CLOSE_PARSING collects \b everything after the
	 * parsing is closed. If used together with #PST_PRSCMD_COLLECT_LIMITER_BREAK_ON
	 * collect maximum count is limited.
	 */
	PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED = 0x0100,


	/**
	 * Enables parameter "--" that ends the parsing of the command line after
	 * what all tokens are redirected to parameters that have
	 * #PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED set. Value "--" can still be used
	 * when it is bound with a parameter (see #PST_PRSCMD_HAS_VALUE).
	 * \note This option affects #PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER,
	 * #PST_PRSCMD_BREAK_WITH_EXISTING_PARAMETER_MATCH and #PST_PRSCMD_COLLECT_LOOSE_VALUES.
	 */
	PST_PRSCMD_CLOSE_PARSING = 0x0200,

	/**
	 * Collect has one point lower priority.
	 */
	PST_PRSCMD_COLLECT_HAS_LOWER_PRIORITY = 0x0400,

	/**
	 * This option can be used to hide existing parameter from command-line parser.
	 * Parameter with this option can only be added from the code. Use together with
	 * #PST_PRSCMD_NO_TYPOS to completely hide the parameter from the user.
	 */
	PST_PRSCMD_HAS_NO_FLAG = 0x0800,

	/**
	 * With this parameter no typos are generated for a parameter.
	 */
	PST_PRSCMD_NO_TYPOS = 0x1000,

	/**
	 * With this parameter only the highest priority last parameter format is
	 * checked in functions #PARAM_SET_isFormatOK and #PARAM_SET_invalidParametersToString.
	 * The error status is set in spite of the given option.
	 */
	PST_PRSCMD_FORMAT_CONTROL_ONLY_FOR_LAST_HIGHST_PRIORITY_VALUE = 0x2000,

	/**
	 * If set, searches for the wildcard characters (<tt>WC</tt>) "*" and "?" in
	 * parameter values. If found, the value containing the \c WC is removed and
	 * replaced with the matching values. If no match was found, the removed
	 * value <b>is not</b> pushed back.
	 *
	 * The \c WC expanding is performed by abstract \c WC expander function that
	 * must be implemented (see #PARAM_SET_setWildcardExpander and
	 * #PARAM_setWildcardExpander). If not implemented, parsing fails.
	 * \see #PARAM_expandWildcard and #PARAM_SET_parseCMD.
	 */
	PST_PRSCMD_EXPAND_WILDCARD = 0x4000,

	/**
	 * If set, this option will affect the maximum possible count of collected
	 * values during entire command-line parsing. It is possible to create multiple
	 * parameters with different count limits that are computed separately. If
	 * the limit for all counters is exceeded, the value is redirected to typo
	 * or unknown parameter check.
	 *
	 * This must be used together with #PST_PRSCMD_COLLECT_LIMITER_1X * \c N,
	 * where \c N is the maximum count permitted. See #PST_PRSCMD_COLLECT_LIMITER_MAX_MASK
	 * to get the maximum count value supported for a parameter.
	 *
	 * Example:
	 * \code{.txt}
	 * PST_PRSCMD_COLLECT_LIMITER_BREAK_ON | ((PST_PRSCMD_COLLECT_LIMITER_1X * 5) & PST_PRSCMD_COLLECT_LIMITER_MAX_MASK)
	 * \endcode
	 * \attention Make sure that the count value specified does not exceed the maximum
	 * supported value or spoil some other parse options. Use #PST_PRSCMD_COLLECT_LIMITER_MAX_MASK
	 * to clean collect limiter parse option.
	 */
	PST_PRSCMD_COLLECT_LIMITER_BREAK_ON       = 0x00008000,

	/**
	 * One \c unit for collect limiter that is used together with #PST_PRSCMD_COLLECT_LIMITER_BREAK_ON.
	 */
	PST_PRSCMD_COLLECT_LIMITER_1X       = 0x00010000,

	/**
	 * Collector count limit mask.
	 */
	PST_PRSCMD_COLLECT_LIMITER_MAX_MASK = 0xffff0000
};

/**
 * Create a new and empty parameter.
 *
 * \param	flagName	The parameter name.
 * \param	flagAlias	The alias for the name. Can be <tt>NULL</tt>.
 * \param	constraint	Constraints for the parameter and its values (see #PARAM_CONSTRAINTS).
 * \param	pars_opt	Parsing options (see [PARAM_PARS_OPTIONS](@ref PARAM_PARS_OPTIONS_enum)). Can be <tt>0</tt>.
 * \param	newObj		Pointer to receiving pointer to new object.
 * \return #PST_OK when successful, error code otherwise.
 *
 * \note Note that it is possible to add multiple values to the list using
 * function #PARAM_addValue no matter what the \c constraint value is set to. To
 * check the constraints see #PARAM_checkConstraints.
 */
int PARAM_new(const char *flagName, const char *flagAlias, int constraint, int pars_opt, PARAM **newObj);

/**
 * Function to free #PARAM object.
 * \param param	Pointer to object to be freed.
 */
void PARAM_free(PARAM *param);

/**
 * Adds several optional functions to a parameter. Each function takes
 * the first parameter as c-string value (must not fail if is \c NULL). All the
 * functions are applied when adding the value to the #PARAM object.
 *
 * Function \c controlFormat is to check the format. Return \c 0 if format is ok,
 * error code otherwise.
 *
 * <tt>int (*controlFormat)(const char *str)</tt>
 *
 * Function \c controlContent is to check the content. Return 0 if content is
 * ok, error code otherwise.
 *
 * <tt>int (*controlContent)(const char *str)</tt>
 *
 * Function \c convert is used to repair / convert the c-string value before any
 * content or format check is performed. Takes two extra parameters for buffer and its
 * size. Return #PST_OK if conversion is successful or #PST_PARAM_CONVERT_NOT_PERFORMED
 * to skip conversion. Any other error code will break adding the value.
 *
 * <tt>int (*convert)(const char *value, char *buf, unsigned *buf_len)</tt>
 *
 *
 * \param	param			#PARAM object.
 * \param	controlFormat	Function for format checking.
 * \param	controlContent	Function for content checking.
 * \param	convert			Function for parameter value conversion.
 * \return #PST_OK if successful, error code otherwise.
 * \note Note that \c controlFormat and \c controlContent may return any error code
 * but \c convert function should return #PST_OK and #PST_PARAM_CONVERT_NOT_PERFORMED
 * as any other error code will break adding the simple value, parsing configuration
 * file or command-line.
 */
int PARAM_addControl(PARAM *param, int (*controlFormat)(const char *), int (*controlContent)(const char *), int (*convert)(const char*, char*, unsigned));

/**
 * Check if parse option or a group is set (can be concatenated together
 * with |).
 *
 * \param param		#PARAM object.
 * \param state		A state to be controlled.
 * \return #PST_OK if successful, error code otherwise.
 * \see [PARAM_PARS_OPTIONS](@ref PARAM_PARS_OPTIONS_enum), #PARAM_setParseOption and #PARAM_SET_parseCMD
 * for more information.
 */
int PARAM_isParseOptionSet(PARAM *param, int state);


/**
 * This function is used to specify parsing options ([PARAM_PARS_OPTIONS](@ref PARAM_PARS_OPTIONS_enum)) used
 * by #PARAM_SET_parseCMD.
 *
 * \param param		#PARAM object.
 * \param option	Parsing options.
 * \return #PST_OK if successful, error code otherwise. #PST_PRSCMD_INVALID_COMBINATION
 * is returned if conflicting parsing option combination is fed to the function.
 */
int PARAM_setParseOption(PARAM *param, int option);

/**
 * Set object extractor to the parameter that implements #PARAM_getObject.
 *
 * Function \c extractObject is used to extract an object from the parameter value.
 * Function \c extractObject affects #PARAM_getObject behaviour. If not specified,
 * the default function is used that extracts the c-string value. Parameter \c extra
 * is <tt>void**</tt> pointer to array with 2 elements, \c str is parameter value
 * and \c obj is pointer to receiving pointer. Calling #PARAM_getObject both array
 * elements in \c extra are pointing to #PARAM_SET itself (<tt>void **extra = {set, set}</tt>),
 * when calling #PARAM_SET_getObjExtended, the second value is determined by
 * the function call and extra parameter given (<tt>void **extra = {set, extra}</tt>).
 *
 * <tt>int (*extractObject)(void **extra, const char *str, void **obj)</tt>
 *
 * \param	param				#PARAM object.
 * \param	extractObject	Object extractor.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_setObjectExtractor(PARAM *param, int (*extractObject)(void **, const char *, void**));

/**
 * Appends value to the parameter. Invalid value format or content is not handled
 * as error, but the state is saved. Internal format or content errors can be
 * detected - see #PARAM_getInvalid.
 *
 * \param	param		#PARAM object.
 * \param	value		Parameter value as c-string. Can be \c NULL.
 * \param	source		Source description as c-string. Can be \c NULL.
 * \param	prio		Priority that can be \c PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher.
 * \return #PST_OK when successful, error code otherwise.
 * \see #PARAM_getValueCount, #PARAM_getInvalidCount, #PARAM_getValue and #PARAM_getInvalid.
 */
int PARAM_addValue(PARAM *param, const char *value, const char* source, int prio);

/**
 *
 * Values are filtered by constraints where parameter\c at is used to index over
 * values. If parameter is empty, #PST_PARAMETER_EMPTY is returned. If there is
 * no value matching the constraints error, #PST_PARAMETER_VALUE_NOT_FOUND is
 * returned.
 *
 * Use #PST_INDEX_LAST as \c at to extract the last value matching the constraints.
 * If there are values with different priority levels use #PST_PRIORITY_HIGHEST
 * (see #PST_PRIORITY_enum) to get the values with the highest priority. To extract
 * values that have source specified, set \c source as desired value or if there
 * is need to ignore the source, set \c source as \c NULL.
 *
 * \param	param		#PARAM object.
 * \param	source		Constraint for the source, can be \c NULL.
 * \param	prio		Priority that can be \c PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	value		Pointer to receiving pointer to \c PARAM_VAL object.
 *
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_getValue(PARAM *param, const char *source, int prio, int at, PARAM_VAL **value);

/**
 * Same as #PARAM_getValue, but instead of #PARAM_VAL object it fills output
 * parameter with #PARAM attributes. Note that \c name in #PARAM_ATR is the real
 * string representation of the parameter name and is \b NOT altered by function
 * #PARAM_setPrintName. See #PARAM_getPrintName to extract parameter print name.
 * \param	param		#PARAM object.
 * \param	source		Constraint for the source, can be \c NULL.
 * \param	prio		Priority that can be \c PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	atr			Pointer to #PARAM_ATR object to store the result.
 * \return #PST_OK when successful, error code otherwise. More common errors
 * #PST_INVALID_ARGUMENT, #PST_PARAMETER_EMPTY and #PST_PARAMETER_VALUE_NOT_FOUND.
 */
int PARAM_getAtr(PARAM *param, const char *source, int prio, int at, PARAM_ATR *atr);

/**
 * This function extracts real name of parameter and/or its alias.
 * \param	param		#PARAM object.
 * \param	name		Pointer to receiving pointer to name.
 * \param	alias		Pointer to receiving pointer to alias.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_getName(PARAM *param, const char **name, const char **alias);

/**
 * Like function #PARAM_getValue but is used to extract a specific object from
 * the c-string value. The functionality must be implemented by setting object
 * extractor function. See \ref #PARAM_setObjectExtractor for more details.
 *
 * \param	param		#PARAM object.
 * \param	source		Constraint for the source, can be \c NULL.
 * \param	prio		Priority that can be \c PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	extra		Pointer to optional extra data array.
 * \param	value		Pointer to the receiving pointer to the value.
 *
 * \return #PST_OK when successful, error code otherwise. More common errors
 * #PST_INVALID_ARGUMENT, #PST_PARAMETER_EMPTY, #PST_PARAMETER_VALUE_NOT_FOUND,
 * #PST_PARAMETER_INVALID_FORMAT and #PST_PARAMETER_UNIMPLEMENTED_OBJ.
 * \note Noted that if format or content status is invalid, it is not possible to extract
 * the object.
 * \attention Returned value must be freed by the user if the implementation requires it.
 */
int PARAM_getObject(PARAM *param, const char *source, int prio, int at, void **extra, void **value);

/**
 * Same as #PARAM_getValue but extracts only values that have invalid content or
 * error status set. See #PARAM_addControl how to apply value checking.
 * \param	param		#PARAM object.
 * \param	source		Constraint for the source, can be \c NULL.
 * \param	prio		Priority that can be \c PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	value		Pointer to receiving pointer to \c PARAM_VAL object.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_getInvalid(PARAM *param, const char *source, int prio, int at, PARAM_VAL **value);

/**
 * Return parameter value count matching the constraints.
 * \param	param		#PARAM object.
 * \param	source		Constraint for the source, can be \c NULL.
 * \param	prio		Priority that can be \c PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher.
 * \param	count		Pointer to to store the count value.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_getValueCount(PARAM *param, const char *source, int prio, int *count);

/**
 * Return parameter's invalid value count matching the constraints. See
 * #PARAM_addControl how to apply value checking.
 * \param	param		#PARAM object.
 * \param	source		Constraint for the source, can be \c NULL.
 * \param	prio		Priority that can be \c PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher.
 * \param	count		Pointer to to store the count value.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_getInvalidCount(PARAM *param, const char *source, int prio, int *count);

/**
 * This function is used to alter the way the parameter is represented in (error)
 * messages or returned by #PARAM_getPrintName. It does not change the parameters
 * real name that is used to get it from #PARAM_SET object.
 *
 * If \c constv  <b>is not</b> \c NULL, a user specified constant value is used.
 * If \c constv is \c NULL an abstract function \c getPrintName must be specified
 * that formats the string.
 * Default print format for long and short parameter is "--long-option" and "-a".
 *
 * <tt>const char* (*getPrintName)(PARAM *param, char *buf, unsigned buf_len)</tt>
 * param   - #PARAM object.
 * buf     - Internal buffer with constant size. May be left unused.
 * buf_len - The size of the internal buffer.
 * Returns string that is the string representation of the parameter.
 *
 *
 * \param param			#PARAM object.
 * \param constv		Constant string representation of the parameter. Value is copied. Can be \c NULL.
 * \param getPrintName	Abstract function's implementation. Has effect only when \c constv is \c NULL. Can be \c NULL.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_setPrintName(PARAM *param, const char *constv, const char* (*getPrintName)(PARAM *param, char *buf, unsigned buf_len));

/**
 * Same as #PARAM_setPrintName but works with the alias.
 * \param param			#PARAM object.
 * \param constv		Constant string representation of the parameter alias. Value is copied. Can be \c NULL.
 * \param getPrintNameAlias	Abstract function's implementation. Has effect only when \c constv is \c NULL. Can be \c NULL.
 * \return #PST_OK when successful, error code otherwise. Return #PST_ALIAS_NOT_SPECIFIED if parameter do not have alias.
 */
int PARAM_setPrintNameAlias(PARAM *param, const char *constv, const char* (*getPrintNameAlias)(PARAM *param, char *buf, unsigned buf_len));

/**
 * Returns the string representation of the parameter. See #PARAM_setPrintName to
 * alter behaviour of this function. Default print format for long and short
 * parameter is "--long-option" and "-a".
 *
 * \param obj			#PARAM object.
 * \return String that is the string representation of the parameter. Can return \c NULL.
 */
const char* PARAM_getPrintName(PARAM *obj);

/**
 * Same as #PARAM_getPrintName but works with alias.
 * \param obj			#PARAM object.
 * \return String that is the string representation of the parameter alias. If alias does not exist, \c NULL is returned.
 */
const char* PARAM_getPrintNameAlias(PARAM *obj);
/**
 * This function is used to set help text for a parameter. The input value is copied.
 * \param param			#PARAM object.
 * \param txt			Help text for a parameter. Value is copied.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_setHelpText(PARAM *param, const char *txt);

/**
 * Returns the help text of the parameter. See #PARAM_setHelpText to change the value.
 * \param obj			#PARAM object.
 * \return String that is the help text of the parameter. If value is not specified,
 * \c NULL is returned.
 */
const char* PARAM_getHelpText(PARAM *obj);

/**
 * This function checks if parameter constraints are satisfied (see #PARAM_new
 * and #PARAM_CONSTRAINTS). Constraint values can be concatenated using |.
 * Returned value contains all failed constraints concatenated (|).
 *
 * \param	param			#PARAM object.
 * \param	constraints		Concatenated constraints to be checked.
 * \return #PST_OK when successful, error code otherwise.
 * If failure due to \c NULL pointer \c PARAM_INVALID_CONSTRAINT is returned.
 */
int PARAM_checkConstraints(const PARAM *param, int constraints);

/**
 * Clears all the values.
 * \param	param			#PARAM object.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_clearAll(PARAM *param);

/**
 * Removes specified value.
 * \param	param		#PARAM object.
 * \param	source		Constraint for the source, can be \c NULL.
 * \param	prio		Priority that can be \c PST_PRIORITY_VALID_BASE (<tt>0</tt>) or higher.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \return #PST_OK when successful, error code otherwise.
 */
int PARAM_clearValue(PARAM *param, const char *source, int prio, int at);

/**
 * Specify a function to expand tokens that contain wildcard character (<tt>WC</tt>)
 * to array of new values. Characters '<tt>?</tt>' and '<tt>*</tt>' are \c WC.
 * Values containing \c WC are removed and replaced with the expanded values.
 *
 * <tt>int (*expand_wildcard)(PARAM_VAL *param_value, void *ctx, int *value_shift)</tt>
 *
 * \c param_value - The value of the current parameter that contains <tt>WC</tt>.
 * \c ctx - Additional data structure (same object as #PARAM_SET_setWildcardExpander input parameter <tt>ctx</tt>.
 * \c value_shift - Output parameter for the count of values expanded.
 *
 *
 * \param	param				#PARAM_SET object.
 * \param	ctx					Data structure used by wildcard expander. Can be \c NULL.
 * \param	ctx_free			Data structure release function. Can be \c NULL.
 * \param	expand_wildcard		Function pointer to wildcard expander function.
 * \return #PST_OK if successful, error code otherwise.
 * \note #PARAM_SET_parseCMD must be used and parsing option #PST_PRSCMD_EXPAND_WILDCARD
 * must be set using #PARAM_SET_setParseOptions.
 */
int PARAM_setWildcardExpander(PARAM *param, void *ctx, void (*ctx_free)(void*), int (*expand_wildcard)(PARAM_VAL *param_value, void *ctx, int *value_shift));

/**
 * Expand the values containing wildcard characters (<tt>WC</tt>). Before using
 * WC expander abstract function must be implemented (See #PARAM_setWildcardExpander).
 * \param param		#PARAM object.
 * \param count		The count of new values inserted.
 * \return #PST_OK if successful, error code otherwise.
 */
int PARAM_expandWildcard(PARAM *param, int *count);

/**
 * Generates #PARAM object description string. Useful for debugging.
 *
 * \param	param	#PARAM object.
 * \param	buf		Receiving buffer.
 * \param	buf_len	Receiving buffer size.
 * \return \c buf if successful, \c NULL otherwise.
 */
char* PARAM_toString(const PARAM *param, char *buf, size_t buf_len);

/**
 * Generates constraint failure report.
 * \param	param	#PARAM object.
 * \param	prefix	Prefix to each constraint failure string. Can be \c NULL.
 * \param	buf		Receiving buffer.
 * \param	buf_len	Receiving buffer size.
 * \return \c buf if successful, \c NULL otherwise.
 */
char* PARAM_constraintErrorToString(PARAM *param, const char *prefix, char *buf, size_t buf_len);

#ifdef	__cplusplus
}
#endif

#endif	/* SET_PARAMETER_H */
