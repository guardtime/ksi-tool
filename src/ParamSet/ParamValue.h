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

#ifndef PARAM_VALUE_H
#define	PARAM_VALUE_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

int PARAM_VAL_new(const char *value, const char* source, int priority, PARAM_VAL **newObj);
void PARAM_VAL_free(PARAM_VAL *rootValue);
PARAM_VAL* PARAM_VAL_getElementAt(PARAM_VAL *rootValue, unsigned at);
PARAM_VAL* PARAM_VAL_getFirstHighestPriorityValue(PARAM_VAL *rootValue);

#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_VALUE_H */