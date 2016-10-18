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

#ifndef DEBUG_PRINT_H
#define	DEBUG_PRINT_H

#include <ksi/ksi.h>
#include <ksi/policy.h>
#include "param_set/param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

void DEBUG_verifySignature(KSI_CTX *ksi, int res, KSI_Signature *sig, KSI_PolicyVerificationResult *result, KSI_DataHash *hsh);
void DEBUG_verifyPubfile(KSI_CTX *ksi, PARAM_SET *set, int res, KSI_PublicationsFile *pub);
void print_progressDesc(int showTiming, const char *msg, ...);
void print_progressResult(int res);
int PROGRESS_BAR_display(int progress);

#ifdef	__cplusplus
}
#endif

#endif	/* DEBUG_PRINT_H */

