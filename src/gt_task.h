/* 
 * File:   gt_task.h
 * Author: Taavi
 *
 * Created on July 22, 2014, 1:31 PM
 */

#ifndef GT_TASK_H
#define	GT_TASK_H

#include "task_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Task that deals with signing operations.
 * @param [in] task Pointer to task object.
 * @return True if successful, false otherwise.
 */
bool GT_signTask(Task *task);

/**
 * Task that deals with verifying operations.
 * @param [in] task Pointer to task object.
 * @return True if successful, false otherwise.
 */
bool GT_verifyTask(Task *task);

/**
 * Task that deals with extending operations.
 * @param [in] task Pointer to task object.
 * @return True if successful, false otherwise.
 */
bool GT_extendTask(Task *task);

/**
 * Task that deals with extending operations.
 * @param [in] task Pointer to task object.
 * @return True if successful, false otherwise.
 */
bool GT_publicationsFileTask(Task *task);

bool GT_other(Task *task);

#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

