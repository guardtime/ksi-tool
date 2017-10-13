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

#ifndef PARAM_SET_OBJ_IMPL_H
#define	PARAM_SET_OBJ_IMPL_H

#include "param_set.h"
#include "task_def.h"
#include "internal.h"

#ifdef	__cplusplus
extern "C" {
#endif

/** Maximum task count. */
#define TASK_DEFINITION_MAX_COUNT 64

/**
 * Parameter value data structure that contains the data and information about its
 * status, priority and source. Data is held as a linked list of values.
 */

struct PARAM_VAL_st{
	char *cstr_value;			/* c-string value for raw argument. */
	char *source;				/* Optional c-string source description, e.g. file, environment. */
	int priority;				/* Priority level constraint. */
	int formatStatus;			/* Format status. */
	int contentStatus;			/* Content status. */

	PARAM_VAL *previous;		/* Link to the previous value. */
	PARAM_VAL *next;			/* Link to the next value. */
};


/**
 * Parameter data structure that describes a parameter and its properties including
 * linked list of values.
 */

struct PARAM_st{
	char *flagName;					/* The name of the parameter. */
	char *flagAlias;				/* The alias for the parameter. */
	char *helpText;				/* The help text for a parameter. */
	int constraints;			/* Constraint If there is more than 1 parameter allowed. For validity check. */
	int highestPriority;			/* Highest priority of inserted values. */
	int parsing_options;			/* Some options used when parsing variables. */
	int argCount;					/* Count of all arguments in chain. */

	PARAM_VAL *last_element;	/* The last value in list. */
	PARAM_VAL *arg;		/* Linked list of parameter values. */
	ITERATOR *itr;

	/**
	 * A function to extract object from the parameter.
	 * int extractObject(void **extra, const char *str, void **obj)
	 * extra - optional pointer to array of pointers of size 2.
	 * str - c-string value that belongs to PARAM_VAL object.
	 * obj - pointer to receiving pointer to desired object.
	 * Returns PST_OK if successful, error code otherwise.
	 */
	int (*extractObject)(void **extra, const char *str, void **obj);

	/**
	 * Function convert takes input as raw parameter, followed by a buffer and its size.
	 * str - c-string value that belongs to PARAM_VAL object.
	 * buf - a buffer that will contain value after conversion.
	 * buf_len - the size of the buffer.
	 * Returns #PST_OK (0) if conversion successful, error code otherwise. If
	 * conversion is not performed, #PST_PARAM_CONVERT_NOT_PERFORMED must be used
	 * or \c str must be copied to \ buf.
	 */
	int (*convert)(const char *str, char *buf, unsigned buf_len);

	/**
	 * Function \c controlFormat takes input as raw parameter and performs format
	 * check.
	 * str - c-string value that belongs to PARAM_VAL object.
	 * Returns 0 if format ok, error code otherwise.
	 */
	int (*controlFormat)(const char *str);

	/**
	 * Function \c controlContent takes input as raw parameter and performs content
	 * check.
	 * str - c-string value that belongs to PARAM_VAL object.
	 * Returns 0 if content ok, error code otherwise.
	 */
	int (*controlContent)(const char *str);

	/**
	 * This buffer and its size is fed to the abstract function getPrintName.
	 * It is not meant to be used directly.
	 */
	char print_name_buf[256];
	char print_name_alias_buf[256];

	/**
	 * Function \c getPrintName is used to return string representation of the
	 * parameter.
	 */
	const char* (*getPrintName)(PARAM *param, char *buf, unsigned buf_len);
	const char* (*getPrintNameAlias)(PARAM *param, char *buf, unsigned buf_len);

	/**
	 * A function to expand tokens that contain wildcard character (WC) to array of
	 * new values. Characters '?' and '*' are WC. The first argument is the param_value
	 * that contains the WC. Argument ctx is for additional data structure used and
	 * value_shift is a return parameter that must contain how many values were extracted.
	 *
	 * expand_wildcard function must not remove param_value from the linked list
	 * as it is done by higher level functions. New values must be appended right
	 * after the param_value.
	 *
	 * Function must return PST_OK
	 *
	 * The function can be activated by manual call to PARAM_expandWildcard or by
	 * setting PST_PRSCMD_EXPAND_WILDCARD for the parameter and calling
	 * PARAM_SET_parseCMD.
	 */
	int (*expand_wildcard)(PARAM_VAL *param_value, void *ctx, int *value_shift);

	/**
	 * Additional context for expand_wildcard.
	 */
	void *expand_wildcard_ctx;

	/**
	 * An optional expanded wildcard ctx object.
	 */
	void (*expand_wildcard_free)(void *);
};


/**
 * Parameter set that contains a list of parameters and abstract functions to redirect
 * info, warning and error messages.
 */

struct PARAM_SET_st {
	/* Parameter count. */
	int count;

	/* List of parameters. */
	PARAM **parameter;
	PARAM *typos;
	PARAM *unknown;
	PARAM *syntax;
};

struct TASK_st{
	int id;
	TASK_DEFINITION *def;
	PARAM_SET *set;
};

struct TASK_DEFINITION_st{
	int id;

	char *name;
	char *mandatory;
	char *atleast_one;
	char *ignore;
	char *forbitten;

	char *toString;
	int isConsistent;
	int isAnalyzed;
	double consistency;
};

struct TASK_SET_st {
	TASK_DEFINITION *array[TASK_DEFINITION_MAX_COUNT];
	size_t count;

	size_t consistent_count;
	PARAM_SET *set_used;
	TASK *consistentTask;

	double cons[TASK_DEFINITION_MAX_COUNT];
	size_t index[TASK_DEFINITION_MAX_COUNT];

	int isAnalyzed;
};

struct ITERATOR_st {
	PARAM_VAL *root;
	PARAM_VAL *value;
	const char *source;
	int priority;
	int i;
};

#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_SET_OBJ_IMPL_H */

