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

#include "gt_cmd_common.h"
#include "param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct taskdef_st TaskDefinition;
typedef struct task_st Task;


/**
 * Defines a new task. Task is defined using multiple strings called categories
 * containing parameters that have special meaning. String format is 
 * "<name><ndn><name><ndn>" where name is parameters name and <ndn> is character
 * that is not a digit or number. For example string "p,example" under category
 * defined defines that task must have parameters 'p' and 'example' prsent.
 * 
 * The meaning of categories is as follows:
 * defined - the base set of parameters that consistently define the task.
 * mandatory - additional mandatory parameters that a task must have.
 * ignore - parameters that are ignored.
 * optional - optional parameters (has no effect).
 * forbidden - forbidden flags that a task must not have defined.
 * 
 * @param[in]	id		the id of the task.
 * @param[in]	name	name or description of the task.
 * @param[in]	def		defined category.
 * @param[in]	man		mandatory category.
 * @param[in]	ignore	ignore category.
 * @param[in]	opt		optional category.
 * @param[in]	forb	forbidden category. 
 * @param[out]	new		New task.
 * @note TaskDefinition must be freed by the caller.
 */
void TaskDefinition_new(int id, const char *name, const char *man, const char *ignore, const char *opt, const char *forb, TaskDefinition **new);

void TaskDefinition_free(TaskDefinition *obj);


/**
 * Analysis task definitions against defined parameter set and determins if
 * a consistent task exists. During analysis some data is updated in def 
 * elements. It is used by @ref Task_getConsistentTask and @ref Task_printError.
 * @param[in]	def		task definition array.
 * @param[in]	count	size of the array.
 * @param[in]	set		parameter set.
 * @return True if consistent task exists, false otherwise.
 */
bool Task_analyse(TaskDefinition **def, int count, paramSet *set);


/**
 * Using predefined task definition array and parameter set, extract consistent
 * task. On errors and warnings messages will be printed into stderr.
 * @param[in]	def		task definition array.
 * @param[in]	count	size of the array.
 * @param[in]	set		parameter set.
 * @return consistent task if it exists, NULL otherwise.
 * @note Task must be freed by the caller.
 */
Task* Task_getConsistentTask(TaskDefinition **def, int count, paramSet *set);


/**
 * Can be used to print errors related to invalid task definitions.
 * @param[in]	def		task definition array.
 * @param[in]	count	size of the array.
 * @param[in]	set		parameter set.
 */
void Task_printError(TaskDefinition **def, int count, paramSet *set);


void Task_free(Task *obj);
int Task_getID(Task *task);
paramSet *Task_getSet(Task *task);

#ifdef	__cplusplus
}
#endif

#endif

