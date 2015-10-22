/* 
 * File:   obj_printer.h
 * Author: Taavi
 *
 * Created on October 22, 2015, 9:05 AM
 */

#ifndef OBJ_PRINTER_H
#define	OBJ_PRINTER_H

#include <ksi/ksi.h>

#ifdef	__cplusplus
extern "C" {
#endif

void OBJPRINT_signerIdentity(KSI_Signature *sig);
void OBJPRINT_publicationsFileReferences(const KSI_PublicationsFile *pubFile);
void OBJPRINT_signaturePublicationReference(KSI_Signature *sig);
void OBJPRINT_signatureVerificationInfo(KSI_Signature *sig);
void OBJPRINT_publicationsFileCertificates(const KSI_PublicationsFile *pubfile);
void OBJPRINT_signatureSigningTime(const KSI_Signature *sig);


#ifdef	__cplusplus
}
#endif

#endif	/* OBJ_PRINTER_H */

