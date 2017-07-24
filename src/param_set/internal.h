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

#ifndef INTERNAL_H
#define INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>
#include <stddef.h>
#include "param_set.h"

typedef struct TASK_DEFINITION_st TASK_DEFINITION;
typedef struct ITERATOR_st ITERATOR;

int TASK_DEFINITION_new(int id, const char *name, const char *man, const char *atleastone, const char *forb, const char *ignore, TASK_DEFINITION **new);
void TASK_DEFINITION_free(TASK_DEFINITION *obj);
int TASK_DEFINITION_analyzeConsistency(TASK_DEFINITION *def, PARAM_SET *set, double *cons);
int TASK_DEFINITION_getMoreConsistent(TASK_DEFINITION *A, TASK_DEFINITION *B, PARAM_SET *set, double sensitivity, TASK_DEFINITION **result);
char* TASK_DEFINITION_toString(TASK_DEFINITION *def, char *buf, size_t buf_len);
char *TASK_DEFINITION_howToRepiar_toString(TASK_DEFINITION *def, PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);
char* TASK_DEFINITION_ignoredParametersToString(TASK_DEFINITION *def, PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);


/**
 * Helper data structure to optimize sequential access to the #PARAM_VAL links.
 * \param root	The first #PARAM_VAL link in the linked list.
 * \param itr	Pointer to receiving pointer to #ITERATOR object.
 * \return #PST_OK if successful, error code otherwise.
 */
int ITERATOR_new(PARAM_VAL *root, ITERATOR **itr);

/**
 * Free #ITERATOR object.
 * \param itr	#ITERATOR object to be freed.
 */
void ITERATOR_free(ITERATOR *itr);

/**
 * Set iterator to specified state.
 * \param itr		- #ITERATOR object.
 * \param root		- Set new root value (e.g. the old one is not used) or NULL to let the base value remain the same.
 * \param source	- The source constraint.
 * \param priority	- The priority constraint.
 * \param at		- Index constraint.
 * \return #PST_OK if successful error code otherwise.
 */
int ITERATOR_set(ITERATOR *itr, PARAM_VAL *new_root, const char* source, int priority, int at);

/**
 * Fetch a parameter value from the configured #ITERATOR (see #ITERATOR_set).
 * \param	itr			#ITERATOR object.
 * \param	source		Constraint for the source.
 * \param	priority	Constraint for the priority.
 * \param	at			Parameter index in the matching set composed with the constraints.
 * \param	item		Pointer to receiving pointer.
 * \return #PST_OK if successful error code otherwise.
 */
int ITERATOR_fetch(ITERATOR *itr, const char* source, int priority, int at, PARAM_VAL **item);

#ifdef __cplusplus
}
#endif

#endif /* INTERNAL_H */

