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

#ifndef TASK_INITIALIZER_H
#define	TASK_INITIALIZER_H

#include <ksi/ksi.h>
#include "param_set/param_set.h"
#include "param_set/task_def.h"

#ifdef	__cplusplus
extern "C" {
#endif
	
/**
 * 1) Check if parameter set contains invalid values (format / content).
 * 2) Check for typos report errors.
 * 3) Check for unknown, report errors.
 * 4) Analyze task set against the given parameter set.
 * 5) Extract the consistent task.
 * 6) If there is no consistent task, report errors.
 *
 * \param set				PARAM_SET obj
 * \param task_set			TASK_SET obj
 * \param task_set_sens		TASK_SET analyze sensitivity. Reccommended value 0.2.
 * \param task_dif			TASK_SET similarity check coeficent. Reccommended value 0.1.
 * \param task
 * \return KT_OK if successful, error code otherwise.
 */
int TASK_INITIALIZER_check_analyze_report(PARAM_SET *set, TASK_SET *task_set, double task_set_sens, double task_dif, TASK **task);

int TASK_INITIALIZER_getServiceInfo(PARAM_SET *set, int argc, char **argv, char **envp);

#ifdef	__cplusplus
}
#endif

#endif	/* TASK_INITIALIZER_H */

