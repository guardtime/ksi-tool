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

void DEBUG_verifySignature(KSI_CTX *ksi, int res, KSI_Signature *sig, KSI_PolicyVerificationResult *result, KSI_DataHash *hsh) {
	KSI_PublicationsFile *pubFile = NULL;
	KSI_DataHash *input_hash = NULL;


	if (ksi == NULL || sig == NULL || result == NULL) return;

	print_debug("KSI Signature Debug info:\n\n");
	OBJPRINT_signatureDump(sig, print_debug);

	print_debug("\n");
	OBJPRINT_signatureVerificationResultDump(result, print_debug);
	print_debug("\n");

	if (hsh != NULL) {
		res = KSI_Signature_getDocumentHash(sig, &input_hash);
		if (res != KSI_OK) {
			print_errors("Error: Unable to extract signatures input hash.\n");
			return;
		}

		if (!KSI_DataHash_equals(hsh, input_hash)) {
			OBJPRINT_Hash(hsh,        "Document hash:       ", print_debug);
			OBJPRINT_Hash(input_hash, "Expected Input hash: ", print_debug);
		}
	}

	if (!strcmp(result->finalResult.ruleName, "KSI_VerificationRule_PublicationsFilePublicationTimeMatchesExtenderResponse") ||
		!strcmp(result->finalResult.ruleName, "KSI_VerificationRule_PublicationsFilePublicationHashMatchesExtenderResponse") ||
		!strcmp(result->finalResult.ruleName, "KSI_VerificationRule_PublicationsFileExtendedSignatureInputHash")) {
		res = KSI_CTX_getPublicationsFile(ksi, &pubFile);
		if (res == KSI_OK && pubFile != NULL) {
			OBJPRINT_publicationsFileCertificates(pubFile, print_debug);
		}
	}
	print_debug("\n");
}

void DEBUG_verifyPubfile(KSI_CTX *ksi, PARAM_SET *set, int res, KSI_PublicationsFile *pub) {
	char *constraint = NULL;
	unsigned i = 0;

	if (ksi == NULL || pub == NULL) return;


	if (res == KSI_PKI_CERTIFICATE_NOT_TRUSTED || res == KSI_INVALID_PKI_SIGNATURE) {
		OBJPRINT_publicationsFileSigningCert(pub, print_debug);

		if (PARAM_SET_isSetByName(set, "cnstr")) {
			print_debug("Expected publications file PKI certificate constraints:\n");
		}

		while (PARAM_SET_getObj(set, "cnstr", NULL, PST_PRIORITY_HIGHEST, i++, (void**)&constraint) == PST_OK) {
			char OID[1204];
			char value[1204];
			char *ret = NULL;

			ret = STRING_extractRmWhite(constraint, NULL, "=", OID, sizeof(OID));
			if(ret != OID) continue;

			ret = STRING_extractRmWhite(constraint, "=", NULL, value, sizeof(value));
			if(ret != value) continue;

			print_debug("  * %s = '%s'\n", OID_getShortDescriptionString(OID), value);
		}

	}
}
