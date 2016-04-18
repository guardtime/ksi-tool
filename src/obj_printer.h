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

#ifndef OBJ_PRINTER_H
#define	OBJ_PRINTER_H

#include <ksi/ksi.h>
#include <ksi/policy.h>

#ifdef	__cplusplus
extern "C" {
#endif

void OBJPRINT_signerIdentity(KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signaturePublicationReference(KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signatureVerificationInfo(KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signatureSigningTime(const KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signatureCertificate(const KSI_Signature *sig, int (*print)(const char *format, ... ));
void OBJPRINT_signatureDump(KSI_Signature *sig, int (*print)(const char *format, ... ));

void OBJPRINT_Hash(KSI_DataHash *hsh, const char *prefix, int (*print)(const char *format, ... ));

void OBJPRINT_publicationsFileReferences(const KSI_PublicationsFile *pubFile, int (*print)(const char *format, ... ));
void OBJPRINT_publicationsFileCertificates(const KSI_PublicationsFile *pubfile, int (*print)(const char *format, ... ));
void OBJPRINT_publicationsFileSigningCert(KSI_PublicationsFile *pubfile, int (*print)(const char *format, ... ));
void OBJPRINT_publicationsFileDump(KSI_PublicationsFile *pubfile, int (*print)(const char *format, ... ));

const char *OBJPRINT_getVerificationErrorCode(VerificationErrorCode code);
void OBJPRINT_signatureVerificationResultDump(KSI_PolicyVerificationResult *res, int (*print)(const char *format, ... ));

#ifdef	__cplusplus
}
#endif

#endif	/* OBJ_PRINTER_H */

