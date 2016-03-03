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

#include <ksi/pkitruststore.h>
#include <string.h>
#include <stdlib.h>
#include "obj_printer.h"
#include "printer.h"
#include "api_wrapper.h"

void OBJPRINT_publicationsFileReferences(const KSI_PublicationsFile *pubFile){
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

		if(KSITOOL_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;

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

void OBJPRINT_signaturePublicationReference(KSI_Signature *sig){
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

	if(KSITOOL_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;
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

void OBJPRINT_Hash(KSI_DataHash *hsh, const char *prefix) {
	char buf[1024];

	if (hsh == NULL) return;

	if (KSITOOL_DataHash_toString(hsh, buf, sizeof(buf)) == NULL) return;

	print_info("%s%s\n",
			prefix == NULL ? "" : prefix,
			buf
			);

	return;
}
void OBJPRINT_signatureInputHash(KSI_Signature *sig) {
	int res;
	KSI_DataHash *hsh = NULL;

	if (sig == NULL) return;

	res = KSI_Signature_getDocumentHash(sig, &hsh);
	if (res != KSI_OK) {
		print_info("Input hash: Unable to extract from the signature.\n");
		return;
	}

	OBJPRINT_Hash(hsh, "Input hash: ");

	return;
}

void OBJPRINT_signerIdentity(KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	char *signerIdentity = NULL;

	if(sig == NULL) goto cleanup;

	print_info("Signer identity: ");
	res = KSI_Signature_getSignerIdentity(sig, &signerIdentity);
	if(res != KSI_OK){
		print_info("Unable to get signer identity.\n");
		goto cleanup;
	}

	print_info("'%s'.\n", signerIdentity == NULL || strlen(signerIdentity) == 0 ? "Unknown" : signerIdentity);
cleanup:

	KSI_free(signerIdentity);
	return;
}

void OBJPRINT_signatureVerificationInfo(KSI_Signature *sig){
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

void OBJPRINT_signatureSigningTime(const KSI_Signature *sig) {
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

	print_info("Signing time: (%i) %s+00:00\n",
			signingTime, date);

	return;
}

void OBJPRINT_signatureCertificate(const KSI_Signature *sig) {
	int res;
	KSI_CalendarAuthRec *calAuthRec = NULL;
	KSI_PKISignedData *pki_data = NULL;
	KSI_OctetString *ID = NULL;
	KSI_Utf8String *sig_type = NULL;

	char str_id[1024];

	char *ret;

	if (sig == NULL) goto cleanup;

	res = KSI_Signature_getCalendarAuthRec(sig, &calAuthRec);
	if (res != KSI_OK || calAuthRec == NULL) goto cleanup;

	res = KSI_CalendarAuthRec_getSignatureData(calAuthRec, &pki_data);
	if (res != KSI_OK || pki_data == NULL) goto cleanup;

	res = KSI_PKISignedData_getCertId(pki_data, &ID);
	if (res != KSI_OK || ID == NULL) goto cleanup;

	res = KSI_PKISignedData_getSigType(pki_data, &sig_type);
	if (res != KSI_OK || sig_type == NULL) goto cleanup;

	ret = KSI_OctetString_toString(ID, ':', str_id, sizeof(str_id));
	if (ret != str_id) goto cleanup;

	print_info("KSI Signatures Calendar authentication record PKI signature:\n");
	print_info("  Signing certificate ID: %s\n", str_id);
	print_info("  Signature type: %s\n", KSI_Utf8String_cstr(sig_type));

cleanup:


	return;
}

void OBJPRINT_publicationsFileCertificates(const KSI_PublicationsFile *pubfile){
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

	for (i = 0; i < KSI_CertificateRecordList_length(certReclist); i++){
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

void OBJPRINT_publicationsFileSigningCert(KSI_PublicationsFile *pubfile) {
	int res;
	KSI_PKISignature *sig = NULL;
	KSI_PKICertificate *cert = NULL;
	char buf[2048];
	char *tmp = NULL;

	res = KSI_PublicationsFile_getSignature(pubfile, &sig);
	if (res != KSI_OK) goto cleanup;

	res = KSI_PKISignature_extractCertificate(sig, &cert);
	if (res != KSI_OK) goto cleanup;

	tmp = KSI_PKICertificate_toString(cert, buf, sizeof(buf));
	if (tmp != buf) goto cleanup;

	print_info("Publications file signing %s", buf);
	print_info("\n", buf);

cleanup:

	KSI_PKICertificate_free(cert);

	return;
}

void OBJPRINT_signatureDump(KSI_Signature *sig) {

	print_info("KSI Signature dump:\n");

	if (sig == NULL) {
		print_info("(null)\n");
		return;
	}

	print_info("  ");
	OBJPRINT_signatureInputHash(sig);
	print_info("  ");
	OBJPRINT_signatureSigningTime(sig);
	print_info("  ");
	OBJPRINT_signerIdentity(sig);
	print_info("  Trust anchor: ");

	if (KSITOOL_Signature_isCalendarAuthRecPresent(sig)) {
		print_info("Calendar Authentication Record.\n\n");
		OBJPRINT_signatureCertificate(sig);
	} else if (KSITOOL_Signature_isPublicationRecordPresent(sig)) {
		print_info("Publication Record.\n\n");
		OBJPRINT_signaturePublicationReference(sig);
	} else {
		print_info("missing.\n\n");
	}

	return;
}

void OBJPRINT_publicationsFileDump(KSI_PublicationsFile *pubfile) {

	print_info("KSI Signature dump:\n");

	if (pubfile == NULL) {
		print_info("(null)\n");
		return;
	}

	OBJPRINT_publicationsFileReferences(pubfile);
	OBJPRINT_publicationsFileCertificates(pubfile);
	OBJPRINT_publicationsFileSigningCert(pubfile);

	return;
}