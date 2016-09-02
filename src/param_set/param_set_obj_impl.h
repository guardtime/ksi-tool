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

#ifndef PARAM_SET_OBJ_IMPL_H
#define	PARAM_SET_OBJ_IMPL_H

#include "param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Parameter value data structure that contains the data and information about its
 * status, priority and source. Data is hold as a linked list of values. 
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
	int constraints;			/* Constraint If there is more than 1 parameters allowed. For validity check. */
	int highestPriority;			/* Highest priority of inserted values. */
	int parsing_options;			/* Some options used when parsing variables. */
	int argCount;					/* Count of all arguments in chain. */

	PARAM_VAL *arg;		/* Linked list of parameter values. */

	int (*extractObject)(void *extra, const char*, void**);		/* Function pointer to convert or repair the value before format and content check. */
	int (*convert)(const char*, char*, unsigned);	/* Function pointer to convert or repair the value before format and content check. */
	int (*controlFormat)(const char*);				/* Function pointer for format control. */
	int (*controlContent)(const char*);				/* Function pointer for content control. */
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
	int index[TASK_DEFINITION_MAX_COUNT];

	int isAnalyzed;
};

#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_SET_OBJ_IMPL_H */

