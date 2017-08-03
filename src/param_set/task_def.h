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

/**
 * A set of task definitions that can be analyzed against #PARAM_SET object to
 * extract a single consistent task.
 */
typedef struct TASK_SET_st TASK_SET;

/**
 * Consistent task object that is extracted from the #TASK_SET.
 */
typedef struct TASK_st TASK;

/**
 * Create an empty #TASK_SET object.
 * \param new	Pointer to receiving pointer to #TASK_SET object.
 * \return #PST_OK if successful, error code otherwise.
 */
int TASK_SET_new(TASK_SET **new);

/**
 * Function to free #TASK_SET object.
 * \param	obj	#TASK_SET object.
 */
void TASK_SET_free(TASK_SET *obj);

/**
 * This function is used to add tasks to the task set. Task is composed from
 * \c id, \c name and different groups of command line parameters. Command-line
 * parameter is said to be set when #PARAM object from #PARAM_SET is not empty (is
 * specified on command-line, with or without a value). A group is specified by
 * string containing command-line parameter names separated with comma
 * (e.g. "input,output,h"). Note that the names do not contain prefix "-" and "--"
 * where parameters are parsed from command-line (see #PARAM_SET_parseCMD).
 *
 * Command-line parameter groups:
 * - \c mandatory - All parameters specified must be set.
 * - <tt> at least one </tt> -	At least one parameter from group must be set.
 * - \c forbidden - Any parameter from this group <b>must not</b> be set.
 * - \c ignored - All parameters from this set can be ignored or even removed (see #TASK_SET_cleanIgnored).
 *
 * \param task_set		#TASK_SET object.
 * \param id			The ID of the task.
 * \param name			Name or description of the task.
 * \param man			Mandatory parameters.
 * \param atleastone	Parameter where at least one must be set. Can be \c NULL.
 * \param forb			Forbidden parameters. Can be \c NULL.
 * \param ignore		Ignored parameters. Can be \c NULL.
 * \return #PST_OK if successful, error code otherwise.
 */
int TASK_SET_add(TASK_SET *task_set, int id, const char *name, const char *man, const char *atleastone, const char *forb, const char *ignore);

/**
 * Performs analysis on the #TASK_SET with parameters from #PARAM_SET. After analysis
 * it is possible to extract one consistent #TASK from the #TASK_SET (see
 * #TASK_SET_getConsistentTask) or in case of an error, check if one task from the
 * #TASK_SET stands out as the possible target and give user some hints how to fix.
 * If it is not possible to assume what task the user is trying to accomplish, show
 * user the tasks that have the most similar patterns compared to the given input.
 * (see #TASK_SET_isOneFromSetTheTarget, #TASK_SET_howToRepair_toString and
 * #TASK_SET_suggestions_toString).
 *
 * The analysis and error handling is based on the task \c consistency value. Consistency
 * \c 1.0 means that it is possible to perform the task with given parameters and
 * \b less than \c 1.0 means it is not. The consistency of a task is calculated
 * from command-line parameter groups (see #TASK_SET_add). To get a consistent
 * task, all mandatory parameters must be set and there must not be a single
 * forbidden parameter set. The consistency of a task is calculated as follows:
 *
 * \code{.txt}
 *                    man_mis + atl_mis + forbidden
 * consistency = 1 -  -----------------------------  , where
 *                        man_count + atl_count
 *
 *   atl_count - Count of parameters that are set from group "at least one".
 *   atl_mis   - 0 if at least one parameter from group "at least one" is set, 1 otherwise.
 *   forbidden - Count of forbidden parameters that are set.
 *   man_count - Count of all mandatory parameters.
 *   man_mis   - Count of missing mandatory parameters.
 * \endcode
 *
 *
 * During analysis \c consistency of all parameters is calculated and values are
 * sorted starting from the most consistent and ending with least consistent. If
 * two tasks have very similar \c consistency, the weighted comparison is used
 * (the difference is specified with \c sensitivity). If weighted comparison does
 * not give better results, two tasks are considered as equal. Weighted comparison
 * is calculated as described below:
 *
 * \code{.txt}
 * Task A is more consistent when ConsitencyA * WA > ConsistencyB * WB
 *
 * Wx = atl_count_x + man_count_x - set_in_both  , where
 *
 *   atl_count_x - Count of parameters that are set from group "at least one".
 *   man_count_x - Count of parameters that are set from group "mandatory".
 *   set_in_both - All parameter that are set in both A nd B.
 * \endcode
 *
 * \param task_set		#TASK_SET object.
 * \param set			#PARAM_SET object.
 * \param sensitivity	Analyze sensitivity .
 * \return #PST_OK if successful, error code otherwise. Some more common error
 * codes: #PST_TASK_SET_HAS_NO_DEFINITIONS and #PST_TASK_UNABLE_TO_ANALYZE_PARAM_SET_CHANGED.
 */
int TASK_SET_analyzeConsistency(TASK_SET *task_set, PARAM_SET *set, double sensitivity);

/**
 * This function is used to extract one single consistent task from the #TASK_SET
 * that is the task user wants to execute. Before this #TASK_SET_analyzeConsistency
 * must be called.
 * \param task_set		#TASK_SET object.
 * \param task			Pointer to receiving pointer to #TASK object.
 * \return #PST_OK if successful, error code otherwise. Some more common error
 * codes: #PST_TASK_SET_HAS_NO_DEFINITIONS, #PST_TASK_SET_NOT_ANALYZED,
 * #PST_TASK_ZERO_CONSISTENT_TASKS and #PST_TASK_MULTIPLE_CONSISTENT_TASKS.
 */
int TASK_SET_getConsistentTask(TASK_SET *task_set, TASK **task);

/**
 * If there is no single consistent task in #TASK_SET, it is possible that user
 * has made an error by specifying too few or too many parameters. In this case,
 * it may be possible to extract the task that is assumed to be the one that user
 * is trying to accomplish. Task is extracted if there is a single task that has
 * its consistency higher than any other task by the value of \c diff.
 * \param task_set		#TASK_SET object.
 * \param diff			Task consistency difference.
 * \param ID			Output parameter for task ID.
 * \return #PST_OK if successful, error code otherwise.
 */
int TASK_SET_isOneFromSetTheTarget(TASK_SET *task_set, double diff, int *ID);

/**
 * This function removes all ignored parameters (see #TASK_SET_add) from the
 * #PARAM_SET object used to perform analysis (see #TASK_SET_analyzeConsistency).
 *
 * \param task_set		#TASK_SET object.
 * \param task			#TASK objet.
 * \param removed		Output parameter for the count of removed values.
 * \return #PST_OK if successful, error code otherwise.
 */
int TASK_SET_cleanIgnored(TASK_SET *task_set, TASK *task, int *removed);

/**
 * Generates suggestion message from pre analyzed #TASK_SET to help user figure
 * out what task he or she wants to accomplish. The maximum number of tasks
 * suggested is specified by \c depth. Suggested tasks are ordered by the
 * consistency value starting from the highest (see #TASK_SET_analyzeConsistency).
 * \param	task_set		#TASK_SET object.
 * \param	depth			Maximum count of tasks displayed.
 * \param	buf				Receiving buffer.
 * \param	buf_len			Receiving buffer size.
 * \return \c buf if successful, \c NULL otherwise.
 */
char* TASK_SET_suggestions_toString(TASK_SET *task_set, int depth, char *buf, size_t buf_len);

/**
 * Generates suggestion message from pre analyzed (see #TASK_SET_analyzeConsistency)
 * #TASK_SET to help user figure out how to fix the task with \c ID
 * (see #TASK_SET_isOneFromSetTheTarget to get ID of the task that user is trying
 * to accomplish).
 * \param	task_set		#TASK_SET object.
 * \param	set				#PARAM_SET object.
 * \param	ID				Tasks ID.
 * \param	prefix			Prefix for the message. Can be \c NULL.
 * \param	buf				Receiving buffer.
 * \param	buf_len			Receiving buffer size.
 * \return \c buf if successful, \c NULL otherwise.
 */
char* TASK_SET_howToRepair_toString(TASK_SET *task_set, PARAM_SET *set, int ID, const char *prefix, char *buf, size_t buf_len);

/**
 * Getter method to extract #TASK id.
 * \param task	#TASK object.
 * \return Task ID.
 */
int TASK_getID(TASK *task);

/**
 * Getter method to extract #TASK name.
 * \param task	#TASK object.
 * \return Task name.
 */
const char* TASK_getName(TASK *task);

/**
 * Getter method to extract #PARAM_SET object used to analyze \c task consistancy
 * (see #TASK_SET_analyzeConsistency).
 * \param task	#TASK object.
 * \return #PARAM_SET object.
 */
PARAM_SET *TASK_getSet(TASK *task);


#ifdef	__cplusplus
}
#endif

#endif

