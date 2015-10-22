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

void printSignerIdentity(KSI_Signature *sig);
void printPublicationsFileReferences(const KSI_PublicationsFile *pubFile);
void printSignaturePublicationReference(KSI_Signature *sig);
void printSignatureVerificationInfo(KSI_Signature *sig);
void printPublicationsFileCertificates(const KSI_PublicationsFile *pubfile);
void printSignaturesRootHash_n_Time(const KSI_Signature *sig);
void printSignatureSigningTime(const KSI_Signature *sig);
void printSignatureStructure(KSI_CTX *ksi, KSI_Signature *sig);


#ifdef	__cplusplus
}
#endif

#endif	/* OBJ_PRINTER_H */

