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

	bool T, t, n, r, d, tlv;
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
	tlv = paramSet_isSetByName(set, "tlv");

	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;

	/* Read the signature. */
	print_progressDesc(t, "Reading signature... ");
	res = loadSignatureFile(err, ksi, inSigFileName, &sig);
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);

	/* Make sure the signature is ok. */
	print_progressDesc(t, "Verifying old signature... ");
	res = KSI_Signature_verify(sig, ksi);

	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature.");
	print_progressResult(res);

	/* Extend the signature. */
	if(T){
		print_progressDesc(t, "Extending old signature to %i... ", publicationTime);
		res = KSI_Integer_new(ksi, publicationTime, &pubTime);
		ERR_CATCH_MSG(err, res, "Error: %s.", errToString(res));
		res = KSI_Signature_extendTo(sig, ksi, pubTime, &ext);
		ERR_APPEND_KSI_ERR(err, res, KSI_EXTEND_NO_SUITABLE_PUBLICATION);
		ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_DATABASE_CORRUPT);
		ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_DATABASE_MISSING);
		ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_REQUEST_TIME_IN_FUTURE);
		ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_REQUEST_TIME_TOO_NEW);
		ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_REQUEST_TIME_TOO_OLD);
		ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_INVALID_TIME_RANGE);
		ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	}
	else{
		print_progressDesc(t, "Extending old signature... ");
		res = KSI_extendSignature(ksi, sig, &ext);
		ERR_APPEND_KSI_ERR(err, res, KSI_EXTEND_NO_SUITABLE_PUBLICATION);
		ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_DATABASE_CORRUPT);
		ERR_APPEND_KSI_ERR(err, res, KSI_SERVICE_EXTENDER_DATABASE_MISSING);
		ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
	}
	print_progressResult(res);

	print_progressDesc(t, "Verifying extended signature... ");
	res = KSI_Signature_verify(ext, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify extended signature.");
	print_progressResult(res);

	/* Save signature. */
	res = saveSignatureFile(err, ksi, ext, outSigFileName);
	if (res != KT_OK) goto cleanup;
	print_info("Extended signature saved.\n");



cleanup:
	print_progressResult(res);

	if(n || r || d || tlv) print_info("\n");
	if(res != KT_OK && sig != NULL && d){
		print_info("Old signature:\n");
		printSignatureVerificationInfo(sig);
		printSignatureStructure(ksi, sig);
	}
	if(ext != NULL){
		if(n || d || r || tlv) print_info("Extended signature:\n");
		if (d) printSignatureVerificationInfo(ext);
		if (n) printSignerIdentity(ext);
		if (r) printSignaturePublicationReference(ext);
		if (tlv) printSignatureStructure(ksi, ext);
	}

	if (res != KT_OK) {
		ERR_TRCKR_printErrors(err);
		if(d && ksi) KSI_ERR_statusDump(ksi, stderr);
		retval = errToExitCode(res);
	}

	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
	KSI_Integer_free(pubTime);
	ERR_TRCKR_free(err);
	closeTask(ksi);

	return retval;
}


