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

#include "gt_task_support.h"
#include "obj_printer.h"
#include "debug_print.h"

static int GT_verifyTask_verifySigOnline(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig);
static int GT_verifyTask_verifyWithPublication(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig, KSI_Signature **out);
static int GT_verifyTask_verify(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig);
static int GT_verifyTask_verifyData(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig);

int GT_verifySignatureTask(Task *task){
	int res;
	KSI_CTX *ksi = NULL;
	ERR_TRCKR *err = NULL;
	paramSet *set = NULL;
	KSI_Signature *sig = NULL;
	KSI_Signature *tmp_ext = NULL;
	int retval = EXIT_SUCCESS;


	bool n, r, d, f, F, ref;
	char *inSigFileName = NULL;

	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "i",0, &inSigFileName);

	ref = paramSet_isSetByName(set, "ref");
	f = paramSet_isSetByName(set, "f");
	F = paramSet_isSetByName(set, "F");
	n = paramSet_isSetByName(set, "n");
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");

	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;


	/* Reading signature file for verification. */
	print_progressDesc(false, "Reading signature... ");
	res = loadSignatureFile(err, ksi, inSigFileName, &sig);
	if (res != KT_OK) goto cleanup;
	print_progressResult(res);


	if (Task_getID(task) == verifyTimestampOnline) {
		res = GT_verifyTask_verifySigOnline(task, ksi, err, sig);
		if (res != KT_OK) goto cleanup;
	} else if(Task_getID(task) == verifyTimestamp) {
		if (ref) {
			res = GT_verifyTask_verifyWithPublication(task, ksi, err, sig, &tmp_ext);
			if (res != KT_OK) goto cleanup;
		} else {
			res = GT_verifyTask_verify(task, ksi, err, sig);
			if (res != KT_OK) goto cleanup;
		}
	}

	if (f || F) {
		res = GT_verifyTask_verifyData(task, ksi, err, sig);
		if (res != KT_OK) goto cleanup;
	}

	print_info("Verification of signature %s successful.\n", inSigFileName);


	if (n || r || d) print_info("\n");

	if (((n || r || d) &&  (Task_getID(task) == verifyTimestamp)) || Task_getID(task) == verifyTimestampOnline){
		if (n) OBJPRINT_signerIdentity(tmp_ext != NULL ? tmp_ext : sig);
		if (r) OBJPRINT_signaturePublicationReference(tmp_ext != NULL ? tmp_ext : sig);
		if (d) {
			OBJPRINT_signatureVerificationInfo(tmp_ext != NULL ? tmp_ext : sig);
			OBJPRINT_signatureSigningTime(tmp_ext != NULL ? tmp_ext : sig);
		}
	}



cleanup:
	print_progressResult(res);

	if (res != KT_OK) {
		DEBUG_verifySignature(ksi, task, res, sig);
		ERR_TRCKR_printErrors(err);
		retval = errToExitCode(res);
	}


	KSI_Signature_free(sig);
	KSI_Signature_free(tmp_ext);
	ERR_TRCKR_free(err);
	closeTask(ksi);

	return retval;
}

int GT_verifyPublicationFileTask(Task *task){
	int res;
	ERR_TRCKR *err = NULL;
	KSI_CTX *ksi = NULL;
	paramSet *set = NULL;
	bool d, t;
	char *inPubFileName = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	KSI_PublicationRecord *pubRec = NULL;
	KSI_PublicationData *pubData = NULL;
	KSI_Integer *pubTime = NULL;
	char pubTimeBuf[1024];
	int retval = EXIT_SUCCESS;


	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "b", 0, &inPubFileName);
	d = paramSet_isSetByName(set, "d");
	t = paramSet_isSetByName(set, "t");

	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;

	print_progressDesc(t, "Reading publications file... ");
	res = loadPublicationFile(err, ksi, inPubFileName, &publicationsFile);
	print_progressResult(res);

	print_progressDesc(t, "Verifying publications file... ");
	res = KSITOOL_verifyPublicationsFile(err, ksi, publicationsFile);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify publication file.");
	print_progressResult(res);

	print_progressDesc(t, "Extracting latest publication time... ");
	res = KSI_PublicationsFile_getLatestPublication(publicationsFile, NULL, &pubRec);
	ERR_CATCH_MSG(err, res, "Error: Unable to extract publication record.");
	res = KSI_PublicationRecord_getPublishedData(pubRec, &pubData);
	ERR_CATCH_MSG(err, res, "Error: Unable to extract publication data.");
	res = KSI_PublicationData_getTime(pubData, &pubTime);
	ERR_CATCH_MSG(err, res, "Error: Unable to extract publication time.");
	print_progressResult(res);

	KSI_Integer_toDateString(pubTime, pubTimeBuf, sizeof(pubTimeBuf));
	print_info("Verification of publication file %s successful.\n", inPubFileName);
	print_info("Latest publication %s.\n", pubTimeBuf);


	if(d && Task_getID(task) == verifyPublicationsFile){
		OBJPRINT_publicationsFileReferences(publicationsFile);
		OBJPRINT_publicationsFileCertificates(publicationsFile);
	}

cleanup:
	print_progressResult(res);

	if (res != KT_OK) {
		DEBUG_verifyPubfile(ksi, task, res, publicationsFile);
		print_info("\n");
		ERR_TRCKR_printErrors(err);
		retval = errToExitCode(res);
	}

	KSI_Integer_free(pubTime);
	KSI_PublicationsFile_free(publicationsFile);
	ERR_TRCKR_free(err);
	closeTask(ksi);

	return retval;
}


static int GT_verifyTask_verifySigOnline(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig) {
	int res;
	paramSet *set = NULL;
	bool t;

	if (task == NULL || ksi == NULL || err == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	set = Task_getSet(task);
	t = paramSet_isSetByName(set, "t");

	print_progressDesc(t, "Verifying online... ");
	res = KSITOOL_Signature_verifyOnline(err, sig, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature online.");
	print_progressResult(res);

	res = KT_OK;

cleanup:
	print_progressResult(res);

	return res;
}

static int GT_verifyTask_verifyWithPublication(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig, KSI_Signature **out) {
	int res;
	paramSet *set = NULL;
	KSI_PublicationData *publication = NULL;
	KSI_PublicationRecord *extendTo = NULL;
	KSI_Signature *tmp_ext = NULL;
	KSI_PublicationRecord *pubRec = NULL;
	KSI_PublicationData *pubData = NULL;
	KSI_PublicationsFile *pubFile = NULL;
	KSI_Integer *timeA = NULL;
	KSI_Integer *timeB = NULL;
	bool t, isExtended;
	char *refStrn = NULL;

	if (task == NULL || ksi == NULL || err == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "ref",0, &refStrn);

	t = paramSet_isSetByName(set, "t");


	isExtended = isSignatureExtended(sig);
	res = KSI_PublicationData_fromBase32(ksi, refStrn, &publication);
	ERR_CATCH_MSG(err, res, "Error: Unable parse publication string.");
	res = KSI_PublicationData_getTime(publication, &timeB);
	ERR_CATCH_MSG(err, res, "Error: Unable to get publication time from publication string.");

	if (isExtended) {
		res = KSI_Signature_getPublicationRecord(sig, &pubRec);
		ERR_CATCH_MSG(err, res, "Error: Unable to extract publication record from signature.");
		res = KSI_PublicationRecord_getPublishedData(pubRec, &pubData);
		ERR_CATCH_MSG(err, res, "Error: Unable to get publication data from signatures publication record.");
		res = KSI_PublicationData_getTime(pubData, &timeA);
		ERR_CATCH_MSG(err, res, "Error: Unable to get publication time from signatures publication record.");
	}

	if (isExtended && KSI_Integer_equals(timeA, timeB)) {
		print_progressDesc(t, "Verifying signature using user publication... ");
		res = KSI_Signature_verifyWithPublication(sig, ksi, publication);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify signature with user publication.");
		print_progressResult(res);
	} else {
		if (isExtended)
			print_warnings("Warning: Publication time of publication string is not matching with signatures publication.\n");
		else
			print_warnings("Warning: Signature is not extended.\n");

		res = KSI_receivePublicationsFile(ksi, &pubFile);
		ERR_APPEND_KSI_ERR(err, res, KSI_PUBLICATIONS_FILE_NOT_CONFIGURED);
		ERR_CATCH_MSG(err, res, "Error: Unable to receive publication file.");
		res = KSI_PublicationsFile_getPublicationDataByPublicationString(pubFile, refStrn, &pubRec);
		ERR_CATCH_MSG(err, res, "Error: Unable to get publication from publication file.");

		if (pubRec == NULL) {
			res = KSI_PublicationRecord_new(ksi, &extendTo);
			ERR_CATCH_MSG(err, res, "Error: Unable to create new publication record.");
			res = KSI_PublicationRecord_setPublishedData(extendTo, publication);
			ERR_CATCH_MSG(err, res, "Error: Unable to set published data.");
			publication = NULL;
			res = KSI_PublicationData_fromBase32(ksi, refStrn, &publication);
			ERR_CATCH_MSG(err, res, "Error: Unable to parse publication string.");
		} else {
			res = KSI_PublicationRecord_clone(pubRec, &extendTo);
			ERR_CATCH_MSG(err, res, "Error: Unable to clone publication record.");
		}

		print_progressDesc(t, "Extending signature to publication time of publication string... ");
		res = KSI_Signature_extend(sig, ksi, extendTo, &tmp_ext);
		ERR_APPEND_KSI_ERR(err, res, KSI_EXTEND_NO_SUITABLE_PUBLICATION);
		ERR_APPEND_KSI_ERR(err, res, KSI_PUBLICATIONS_FILE_NOT_CONFIGURED);
		ERR_APPEND_KSI_BASE_ERR(err, res, ksi);
		ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
		print_progressResult(res);

		print_progressDesc(t, "Verifying signature using user publication... ");
		res = KSI_Signature_verifyWithPublication(tmp_ext, ksi, publication);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify signature with user publication.");
		print_progressResult(res);
	}

	if (out != NULL) {
		*out = tmp_ext;
		tmp_ext = NULL;
	}

	res = KT_OK;

cleanup:
	print_progressResult(res);

	KSI_PublicationData_free(publication);
	KSI_PublicationRecord_free(extendTo);
	KSI_Signature_free(tmp_ext);

	return res;
}

static int GT_verifyTask_verify(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig) {
	int res;
	paramSet *set = NULL;
	bool b, t;
	bool isExtended = false;


	if (task == NULL || ksi == NULL || err == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	set = Task_getSet(task);
	t = paramSet_isSetByName(set, "t");
	b = paramSet_isSetByName(set, "b");

	isExtended = isSignatureExtended(sig);


	print_progressDesc(t, "Verifying signature%s ", b && isExtended ? " using local publications file... " : "... ");
	res = KSITOOL_Signature_verify(err, sig, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature.");
	print_progressResult(res);

	res = KT_OK;

cleanup:
	print_progressResult(res);

	return res;
}

static int GT_verifyTask_verifyData(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig) {
	int res;
	paramSet *set = NULL;
	bool F, f;
	char *imprint = NULL;
	KSI_DataHash *file_hsh = NULL;
	KSI_DataHash *raw_hsh = NULL;
	KSI_DataHasher *hsr = NULL;
	char *inDataFileName = NULL;

	if (task == NULL || ksi == NULL || err == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	set = Task_getSet(task);
	f = paramSet_getStrValueByNameAt(set, "f",0, &inDataFileName);
	F = paramSet_getStrValueByNameAt(set, "F",0, &imprint);


	if(f){
		print_progressDesc(false, "Verifying file's %s hash... ", inDataFileName);
		res = KSI_Signature_createDataHasher(sig, &hsr);
		ERR_CATCH_MSG(err, res, "Error: Unable to create data hasher.");
		res = getFilesHash(err, hsr, inDataFileName, &file_hsh);
		if (res != KT_OK) {
			ERR_TRCKR_ADD(err, res, "Error: Unable to hash file. (%s)", errToString(res));
			goto cleanup;
		}
		res = KSITOOL_Signature_verifyDataHash(err, sig, ksi, file_hsh);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify files hash.");
		print_progressResult(res);
	}
	if(F){
		print_progressDesc(false, "Verifying imprint... ");
		res = getHashFromCommandLine(imprint, ksi, err, &raw_hsh);
		if (res != KT_OK) goto cleanup;
		res = KSITOOL_Signature_verifyDataHash(err, sig, ksi, raw_hsh);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify hash.");
		print_progressResult(res);
	}

	res = KT_OK;

cleanup:
	print_progressResult(res);

	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(raw_hsh);
	KSI_DataHash_free(file_hsh);

	return res;
}
