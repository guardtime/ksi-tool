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

#ifndef TASK_DEF_H
#define	TASK_DEF_H

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

char* TASK_SET_suggestions_toString(TASK_SET *task_set, int depth, char *buf, size_t buf_len);

int TASK_getID(TASK *task);
PARAM_SET *TASK_getSet(TASK *task);
TASK_DEFINITION *TASK_getDef(TASK *task);


#ifdef	__cplusplus
}
#endif

#endif

