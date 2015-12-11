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

#ifndef SET_PARAMETER_H
#define	SET_PARAMETER_H


#ifdef	__cplusplus
extern "C" {
#endif

void parameter_free(PARAM *obj);

bool parameter_new(const char *flagName,const char *flagAlias, bool isMultipleAllowed, bool isSingleHighestPriority,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		bool (*convert)(const char*, char*, unsigned),
		PARAM **newObj);

bool parameter_addArgument(PARAM *param, const char *argument, const char* source, int priority);

bool parameter_isDuplicateConflict(const PARAM *param);

void parameter_Print(const PARAM *param, int (*print)(const char*, ...));
#ifdef	__cplusplus
}
#endif

#endif	/* SET_PARAMETER_H */