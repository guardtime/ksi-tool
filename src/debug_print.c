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

#include "debug_print.h"
#include <string.h>
#include "printer.h"
#include "obj_printer.h"
#include "param_set/param_set.h"
#include "tool_box/tool_box.h"

static int debug_getVerificationStepFailed(KSI_Signature *sig) {
	int stat;
	const KSI_VerificationResult *ver = NULL;
	const KSI_VerificationStepResult *step;
	size_t count;
	size_t i;

	if (sig == NULL) return 0;

	stat = KSI_Signature_getVerificationResult(sig, &ver);
	if (stat != KSI_OK) return 0;

	count = KSI_VerificationResult_getStepResultCount(ver);
	if (count == 0) return 0;

	for(i = 0; i < count; i++) {
		KSI_VerificationResult_getStepResult(ver, i, &step);

		/**
		 *	If is verification failure.
		 */
		if (KSI_VerificationStepResult_isSuccess(step) == 0) {
			return KSI_VerificationStepResult_getStep(step);
		}
	}

	return 0;
}

void DEBUG_verifySignature(KSI_CTX *ksi, int res, KSI_Signature *sig, KSI_DataHash *hsh) {
	int stepFailed;
	KSI_PublicationsFile *pubFile = NULL;
	KSI_DataHash *input_hash = NULL;


	if (ksi == NULL || sig == NULL || res != KSI_VERIFICATION_FAILURE) return;

	stepFailed = debug_getVerificationStepFailed(sig);
	print_info("KSI Signature Debug info:\n\n");
	OBJPRINT_signatureDump(sig);

	print_info("\n");
	OBJPRINT_signatureVerificationInfo(sig);

	if (hsh != NULL) {
		res = KSI_Signature_getDocumentHash(sig, &input_hash);
		if (res != KSI_OK) {
			print_errors("Error: Unable to extract signatures input hash.\n");
			return;
		}

		if (!KSI_DataHash_equals(hsh, input_hash)) {
			OBJPRINT_Hash(hsh,        "Document hash:       ");
			OBJPRINT_Hash(input_hash, "Expected Input hash: ");
		}
	}

	if (stepFailed == KSI_VERIFY_CALAUTHREC_WITH_SIGNATURE) {
		res = KSI_CTX_getPublicationsFile(ksi, &pubFile);
		if (res == KSI_OK && pubFile != NULL) {
			OBJPRINT_publicationsFileCertificates(pubFile);
		}
	}




	print_info("\n");
}

void DEBUG_verifyPubfile(KSI_CTX *ksi, PARAM_SET *set, int res, KSI_PublicationsFile *pub) {
	char *constraint = NULL;
	unsigned i = 0;

	if (ksi == NULL || pub == NULL) return;


	if (res == KSI_PKI_CERTIFICATE_NOT_TRUSTED || res == KSI_INVALID_PKI_SIGNATURE) {
		print_info("\n");
		OBJPRINT_publicationsFileSigningCert(pub);

		if (PARAM_SET_isSetByName(set, "cnstr")) {
			print_info("Expected publications file PKI certificate constraints:\n");
		}

		while (PARAM_SET_getObj(set, "cnstr", NULL, PST_PRIORITY_HIGHEST, i++, (void**)&constraint) == PST_OK) {
			char OID[1204];
			char value[1204];
			char *ret = NULL;

			ret = STRING_extractRmWhite(constraint, NULL, "=", OID, sizeof(OID));
			if(ret != OID) continue;

			ret = STRING_extractRmWhite(constraint, "=", NULL, value, sizeof(value));
			if(ret != value) continue;

			print_info("  * %s = '%s'\n", OID_getShortDescriptionString(OID), value);
		}

	}
}
