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

#include "types.h"

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
	
	PARAM_VAL *next;			/* Link to the next value. */
};	


/**
 * Parameter data structure that describes a parameter and its properties including
 * linked list of values.  
 */

struct PARAM_st{
	char *flagName;					/* The name of the parameter. */
	char *flagAlias;				/* The alias for the parameter. */
	int isMultipleAllowed;			/* Constraint If there is more than 1 parameters allowed. For validity check. */
	int isSingleHighestPriority;	/* Constraint to allow to have a single value at highest priority. */
	int highestPriority;			/* Highest priority of inserted values. */
	int argCount;					/* Count of all arguments in chain. */

	PARAM_VAL *arg;		/* Linked list of parameter values. */

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
	
	/* Function pointers to redirect info, warning and error messages. */
	int (*printInfo)(const char*, ...);
	int (*printWarning)(const char*, ...);
	int (*printError)(const char*, ...);
};

#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_SET_OBJ_IMPL_H */

