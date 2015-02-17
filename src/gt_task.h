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
int GT_signTask(Task *task);

/**
 * Task that deals with verifying operations.
 * @param [in] task Pointer to task object.
 * @return True if successful, false otherwise.
 */
int GT_verifyTask(Task *task);

/**
 * Task that deals with extending operations.
 * @param [in] task Pointer to task object.
 * @return True if successful, false otherwise.
 */
int GT_extendTask(Task *task);

/**
 * Task that deals with extending operations.
 * @param [in] task Pointer to task object.
 * @return True if successful, false otherwise.
 */
int GT_publicationsFileTask(Task *task);

int GT_other(Task *task);

#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

