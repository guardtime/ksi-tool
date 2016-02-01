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

#include "param_set/task_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Task that deals with signing files and hash.
 * @param [in] task Pointer to task object.
 * @return exit code.
 */
int GT_signTask(TASK *task);

/**
 * Task that deals with signature verification.
 * @param [in] task Pointer to task object.
 * @return exit code.
 */
int GT_verifySignatureTask(TASK *task);

/**
 * Task that deals with publication file verification.
 * @param [in] task Pointer to task object.
 * @return exit code.
 */
int GT_verifyPublicationFileTask(TASK *task);

/**
 * Task that deals with signature extending.
 * @param [in] task Pointer to task object.
 * @return exit code.
 */
int GT_extendTask(TASK *task);

/**
 * Task that deals with publications file.
 * @param [in] task Pointer to task object.
 * @return exit code.
 */
int GT_publicationsFileTask(TASK *task);

/**
 * Other tasks..
 * @param [in] task Pointer to task object.
 * @return exit code.
 */
int GT_other(TASK *task);

#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

