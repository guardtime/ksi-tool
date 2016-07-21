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

#ifndef TASK_DEF_H
#define	TASK_DEF_H

#include <stddef.h>
#include "param_set.h"


#ifdef	__cplusplus
extern "C" {
#endif

int TASK_DEFINITION_new(int id, const char *name, const char *man, const char *atleastone, const char *forb, const char *ignore, TASK_DEFINITION **new);
void TASK_DEFINITION_free(TASK_DEFINITION *obj);
int TASK_DEFINITION_analyzeConsistency(TASK_DEFINITION *def, PARAM_SET *set, double *cons);
int TASK_DEFINITION_getMoreConsistent(TASK_DEFINITION *A, TASK_DEFINITION *B, PARAM_SET *set, double sensitivity, TASK_DEFINITION **result);

char* TASK_DEFINITION_toString(TASK_DEFINITION *def, char *buf, size_t buf_len);
char *TASK_DEFINITION_howToRepiar_toString(TASK_DEFINITION *def, PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);
char* TASK_DEFINITION_ignoredParametersToString(TASK_DEFINITION *def, PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);


int TASK_SET_new(TASK_SET **new);
void TASK_SET_free(TASK_SET *obj);
int TASK_SET_add(TASK_SET *obj, int id, const char *name, const char *man, const char *atleastone, const char *forb, const char *ignore);
int TASK_SET_analyzeConsistency(TASK_SET *task_set, PARAM_SET *set, double sensitivity);
int TASK_SET_getConsistentTask(TASK_SET *task_set, TASK **task);
int TASK_SET_cleanIgnored(TASK_SET *task_set, TASK *task, int *removed);
int TASK_SET_isOneFromSetTheTarget(TASK_SET *task_set, double diff, int *ID);

char* TASK_SET_suggestions_toString(TASK_SET *task_set, int depth, char *buf, size_t buf_len);
char* TASK_SET_howToRepair_toString(TASK_SET *task_set, PARAM_SET *set, int ID, const char *prefix, char *buf, size_t buf_len);

int TASK_getID(TASK *task);
PARAM_SET *TASK_getSet(TASK *task);
TASK_DEFINITION *TASK_getDef(TASK *task);


#ifdef	__cplusplus
}
#endif

#endif

