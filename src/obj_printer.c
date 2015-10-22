#include "obj_printer.h"
#include "printer.h"
#include <ksi/pkitruststore.h>
#include <string.h>

void printPublicationsFileReferences(const KSI_PublicationsFile *pubFile){
	int res = KSI_UNKNOWN_ERROR;
	KSI_LIST(KSI_PublicationRecord)* list_publicationRecord = NULL;
	KSI_PublicationRecord *publicationRecord = NULL;
	char buf[1024];
	unsigned int i, j;
	char *pLineBreak = NULL;
	char *pStart = NULL;

	if(pubFile == NULL) return;

	print_info("Publications file references:\n");

	res = KSI_PublicationsFile_getPublications(pubFile, &list_publicationRecord);
	if(res != KSI_OK) return;

	for (i = 0; i < KSI_PublicationRecordList_length(list_publicationRecord); i++) {
		int h=0;
		res = KSI_PublicationRecordList_elementAt(list_publicationRecord, i, &publicationRecord);
		if(res != KSI_OK) return;

		if(KSI_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;

		pStart = buf;
		j=1;
		h=0;
		if(i) print_info("\n");
		while((pLineBreak = strchr(pStart, '\n')) != NULL){
			*pLineBreak = 0;
			if(h++ < 3)
				print_info("%s %s\n", "  ", pStart);
			else
				print_info("%s %2i) %s\n", "    ", j++, pStart);
			pStart = pLineBreak+1;
		}

		if(h < 3)
			print_info("%s %s\n", "  ", pStart);
		else
			print_info("%s %2i) %s\n", "    ", j++, pStart);
	}
	print_info("\n");
	return;
}

void printSignaturePublicationReference(KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	KSI_PublicationRecord *publicationRecord;
	char buf[1024];
	char *pLineBreak = NULL;
	char *pStart = buf;
	int i=1;
	int h=0;
	if(sig == NULL) return;

	print_info("Signatures publication references:\n");
	res = KSI_Signature_getPublicationRecord(sig, &publicationRecord);
	if(res != KSI_OK)return ;

	if(publicationRecord == NULL) {
		print_info("  (No publication records available)\n\n");
		return;
	}

	if(KSI_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;
	pStart = buf;

	while((pLineBreak = strchr(pStart, '\n')) != NULL){
		*pLineBreak = 0;

		if(h < 3)
			print_info("%s %s\n", "  ", pStart);
		else
			print_info("%s %2i) %s\n", "    ", i++, pStart);

		pStart = pLineBreak+1;
	}

	if(h<3)
		print_info("%s %s\n", "  ", pStart);
	else
		print_info("%s %2i) %s\n", "    ", i++, pStart);
	print_info("\n");

	return;
}

void printSignerIdentity(KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	char *signerIdentity = NULL;

	if(sig == NULL) goto cleanup;

	print_info("Signer identity: ");
	res = KSI_Signature_getSignerIdentity(sig, &signerIdentity);
	if(res != KSI_OK){
		print_info("Unable to get signer identity.\n");
		goto cleanup;
	}

	print_info("'%s'\n", signerIdentity == NULL || strlen(signerIdentity) == 0 ? "Unknown" : signerIdentity);
	print_info("\n");
cleanup:

	KSI_free(signerIdentity);
	return;
}

void printSignatureVerificationInfo(KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	const KSI_VerificationResult *sigVerification = NULL;
	const KSI_VerificationStepResult *result = NULL;
	const char *desc;
	unsigned int i = 0;

	if(sig == NULL){
		return;
	}

	print_info("Verification steps:\n");
	res = KSI_Signature_getVerificationResult(sig, &sigVerification);
	if(res != KSI_OK){
		print_info("Unable to get verification steps\n\n");
		return;
	}

	if(sigVerification != NULL){
		for(i=0; i< KSI_VerificationResult_getStepResultCount(sigVerification); i++){
			res = KSI_VerificationResult_getStepResult(sigVerification, i, &result);
			if(res != KSI_OK){
				return;
			}
			print_info("  0x%03x:\t%s", KSI_VerificationStepResult_getStep(result), KSI_VerificationStepResult_isSuccess(result) ? "OK" : "FAIL");
			desc = KSI_VerificationStepResult_getDescription(result);
			if (desc && *desc) {
				print_info(" (%s)", desc);
			}
			print_info("\n");
		}
	}
	print_info("\n");
	return;
}

void printSignatureSigningTime(const KSI_Signature *sig) {
	int res;
	KSI_Integer *sigTime = NULL;
	unsigned long signingTime = 0;
	char date[1024];


	if (sig == NULL) {
		return;
	}


	res = KSI_Signature_getSigningTime(sig, &sigTime);
	if (res != KSI_OK) {
		return;
	}

	if (KSI_Integer_toDateString(sigTime, date, sizeof(date)) != date) {
		return;
	}

	signingTime = (unsigned long)KSI_Integer_getUInt64(sigTime);

	print_info("Signing time:\n"
			"UTC seconds:%i\n"
			"Date %s\n", signingTime, date);

	print_info("\n");
	return;
}

void printPublicationsFileCertificates(const KSI_PublicationsFile *pubfile){
	KSI_CertificateRecordList *certReclist = NULL;
	KSI_CertificateRecord *certRec = NULL;
	KSI_PKICertificate *cert = NULL;
	char buf[1024];
	unsigned int i=0;
	int res = 0;

	if(pubfile == NULL) goto cleanup;
	print_info("Publications file certificates::\n");

	res = KSI_PublicationsFile_getCertificates(pubfile, &certReclist);
	if(res != KSI_OK || certReclist == NULL) goto cleanup;

	for(i=0; i<KSI_CertificateRecordList_length(certReclist); i++){
		res = KSI_CertificateRecordList_elementAt(certReclist, i, &certRec);
		if(res != KSI_OK || certRec == NULL) goto cleanup;

		res = KSI_CertificateRecord_getCert(certRec, &cert);
		if(res != KSI_OK || cert == NULL) goto cleanup;

		if(KSI_PKICertificate_toString(cert, buf, sizeof(buf)) != NULL)
			print_info("%s\n", buf);
	}

cleanup:

	return;
	}

