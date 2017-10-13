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

#ifndef OBJ_PRINTER_H
#define	OBJ_PRINTER_H

#include <ksi/ksi.h>
#include <ksi/policy.h>

#ifdef	__cplusplus
extern "C" {
#endif

void OBJPRINT_IdentityMetadata(KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signaturePublicationReference(KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signatureVerificationInfo(KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signatureSigningTime(const KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signatureCertificate(KSI_CTX *ctx, const KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signatureDump(KSI_CTX *ctx, KSI_Signature *sig, int (*print)(const char *format, ... ));

void OBJPRINT_Hash(KSI_DataHash *hsh, const char *prefix, int (*print)(const char *format, ... ));

void OBJPRINT_publicationsFileReferences(const KSI_PublicationsFile *pubFile, int (*print)(const char *format, ... ));
void OBJPRINT_publicationsFileCertificates(const KSI_PublicationsFile *pubfile, int (*print)(const char *format, ... ));
void OBJPRINT_publicationsFileSigningCert(KSI_PublicationsFile *pubfile, int (*print)(const char *format, ... ));
void OBJPRINT_publicationsFileDump(KSI_PublicationsFile *pubfile, int (*print)(const char *format, ... ));

const char *OBJPRINT_getVerificationErrorCode(KSI_VerificationErrorCode code);
const char *OBJPRINT_getVerificationErrorDescription(KSI_VerificationErrorCode code);

void OBJPRINT_signatureVerificationResultDump(KSI_PolicyVerificationResult *res, int (*print)(const char *format, ... ));

void OBJPRINT_aggregatorConfDump(KSI_Config *config, int (*print)(const char *format, ... ));
void OBJPRINT_extenderConfDump(KSI_Config *config, int (*print)(const char *format, ... ));

#ifdef	__cplusplus
}
#endif

#endif	/* OBJ_PRINTER_H */

