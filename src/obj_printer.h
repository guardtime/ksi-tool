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

#ifdef	__cplusplus
extern "C" {
#endif

void OBJPRINT_signerIdentity(KSI_Signature *sig);
void OBJPRINT_signaturePublicationReference(KSI_Signature *sig);
void OBJPRINT_signatureVerificationInfo(KSI_Signature *sig);
void OBJPRINT_signatureSigningTime(const KSI_Signature *sig);
void OBJPRINT_signatureCertificate(const KSI_Signature *sig);

void OBJPRINT_publicationsFileReferences(const KSI_PublicationsFile *pubFile);
void OBJPRINT_publicationsFileCertificates(const KSI_PublicationsFile *pubfile);
void OBJPRINT_publicationsFileSigningCert(KSI_PublicationsFile *pubfile);


#ifdef	__cplusplus
}
#endif

#endif	/* OBJ_PRINTER_H */

