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

#ifndef DEBUG_PRINT_H
#define	DEBUG_PRINT_H

#include <ksi/ksi.h>
#include "param_set/param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

void DEBUG_verifySignature(KSI_CTX *ksi, int res, KSI_Signature *sig, KSI_DataHash *hsh);
void DEBUG_verifyPubfile(KSI_CTX *ksi, PARAM_SET *set, int res, KSI_PublicationsFile *pub);


#ifdef	__cplusplus
}
#endif

#endif	/* DEBUG_PRINT_H */

