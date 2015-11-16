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

#include <stdlib.h>
#include "obj_printer.h"
#include "gt_task_support.h"

int GT_extendTask(Task *task) {
	int res;
	paramSet *set = NULL;
	KSI_CTX *ksi = NULL;
	ERR_TRCKR *err = NULL;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;
	KSI_Integer *pubTime = NULL;
	int retval = EXIT_SUCCESS;

	bool T, t, n, r, d;
	char *inSigFileName = NULL;
	char *outSigFileName = NULL;
	int publicationTime = 0;

	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "i", 0,&inSigFileName);
	paramSet_getStrValueByNameAt(set, "o", 0,&outSigFileName);
	T = paramSet_getIntValueByNameAt(set,"T",0,&publicationTime);
	n = paramSet_isSetByName(set, "n");
	t = paramSet_isSetByName(set, "t");
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");

	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;

	/* Read the signature. */
	print_progressDesc(t, "Reading signature... ");
	res = loadSignatureFile(err, ksi, inSigFileName, &sig);
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);

	/* Make sure the signature is ok. */
	print_progressDesc(t, "Verifying old signature... ");
	res = KSITOOL_Signature_verify(err, sig, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature.");

	print_progressResult(res);

	/* Extend the signature. */
	if(T){
		print_progressDesc(t, "Extending old signature to %i... ", publicationTime);
		res = KSI_Integer_new(ksi, publicationTime, &pubTime);
		ERR_CATCH_MSG(err, res, "Error: %s.", errToString(res));
		res = KSITOOL_Signature_extendTo(err, sig, ksi, pubTime, &ext);
		ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	}
	else{
		print_progressDesc(t, "Extending old signature... ");
		res = KSITOOL_extendSignature(err, ksi, sig, &ext);
		ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	}
	print_progressResult(res);

	print_progressDesc(t, "Verifying extended signature... ");
	res = KSITOOL_Signature_verify(err, ext, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	print_progressResult(res);

	/* Save signature. */
	res = saveSignatureFile(err, ksi, ext, outSigFileName);
	if (res != KT_OK) goto cleanup;
	print_info("Extended signature saved.\n");



cleanup:
	print_progressResult(res);

	if(n || r || d) print_info("\n");

	if(ext != NULL){
		if(n || d || r) print_info("Extended signature:\n");
		if (d) OBJPRINT_signatureVerificationInfo(ext);
		if (n) OBJPRINT_signerIdentity(ext);
		if (r) OBJPRINT_signaturePublicationReference(ext);
	}

	if (res != KT_OK) {
		print_errors("\n");
		ERR_TRCKR_printErrors(err);
		if(d && ksi) {
			print_errors("\n");
			KSI_ERR_statusDump(ksi, stderr);
		}

		if (sig != NULL && d){
			print_errors("\nOld signature:\n");
			OBJPRINT_signatureVerificationInfo(sig);
		}

		retval = errToExitCode(res);
	}



	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
	KSI_Integer_free(pubTime);
	ERR_TRCKR_free(err);
	closeTask(ksi);

	return retval;
}


