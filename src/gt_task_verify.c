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


	bool n, r, d, f, F, ref, tlv;
	char *inSigFileName = NULL;

	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "i",0, &inSigFileName);

	ref = paramSet_isSetByName(set, "ref");
	f = paramSet_isSetByName(set, "f");
	F = paramSet_isSetByName(set, "F");
	n = paramSet_isSetByName(set, "n");
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");
	tlv = paramSet_isSetByName(set, "tlv");

	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;


	/* Reading signature file for verification. */
	print_info("Reading signature... ");
	res = loadSignatureFile(err, ksi, inSigFileName, &sig);
	if (res != KT_OK) goto cleanup;
	print_info("ok.\n");


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


	if (n || r || d || tlv) print_info("\n");

	if ((n || r || d || tlv) &&  Task_getID(task) == verifyTimestamp || Task_getID(task) == verifyTimestampOnline){
		if (n) printSignerIdentity(tmp_ext != NULL ? tmp_ext : sig);
		if (r) printSignaturePublicationReference(tmp_ext != NULL ? tmp_ext : sig);
		if (d) {
			printSignatureVerificationInfo(tmp_ext != NULL ? tmp_ext : sig);
			printSignatureSigningTime(tmp_ext != NULL ? tmp_ext : sig);
		}
		if (tlv) {
			printSignatureStructure(ksi, tmp_ext != NULL ? tmp_ext : sig);
		}
	}



cleanup:

	if (res != KT_OK) {
		print_errors("failed.\n");
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
	bool b, d, t;
	char *inPubFileName = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	int retval = EXIT_SUCCESS;


	set = Task_getSet(task);
	b = paramSet_getStrValueByNameAt(set, "b",0, &inPubFileName);
	d = paramSet_isSetByName(set, "d");
	t = paramSet_isSetByName(set, "t");

	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;

	print_info("Reading publications file... ");
	MEASURE_TIME(res = loadPublicationFile(err, ksi, inPubFileName, &publicationsFile));
	print_info("ok. %s\n",t ? str_measuredTime() : "");

	print_info("Verifying  publications file... ");
	res = KSI_verifyPublicationsFile(ksi, publicationsFile);
	ERR_CATCH_MSG(err, res, "Error: Unable to verify publication file.");
	print_info("ok.\n");

	print_info("Verification of publication file %s successful.\n", inPubFileName);


	if(d && Task_getID(task) == verifyPublicationsFile){
		printPublicationsFileReferences(publicationsFile);
		printPublicationsFileCertificates(publicationsFile);
	}

cleanup:

	if (res != KT_OK) {
		print_errors("failed.\n");
		ERR_TRCKR_printErrors(err);
		retval = errToExitCode(res);
	}

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

	print_info("Verifying online... ");
	MEASURE_TIME(res = KSI_Signature_verifyOnline(sig, ksi));
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature online.");
	print_info("ok. %s\n", t ? str_measuredTime() : "");

	res = KT_OK;

cleanup:

	return res;
}

static int GT_verifyTask_verifyWithPublication(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig, KSI_Signature **out) {
	int res;
	paramSet *set = NULL;
	KSI_PublicationData *publication = NULL;
	KSI_PublicationRecord *extendTo = NULL;
	KSI_Signature *tmp_ext = NULL;
	int retval = EXIT_SUCCESS;
	KSI_PublicationRecord *pubRec = NULL;
	KSI_PublicationData *pubData = NULL;
	KSI_PublicationsFile *pubFile = NULL;
	KSI_Integer *timeA = NULL;
	KSI_Integer *timeB = NULL;
	bool n, r, d, t, ref, tlv, isExtended;
	char *refStrn = NULL;
	bool onErrorPrintFail = false;

	if (task == NULL || ksi == NULL || err == NULL || sig == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	set = Task_getSet(task);
	ref = paramSet_getStrValueByNameAt(set, "ref",0, &refStrn);

	n = paramSet_isSetByName(set, "n");
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");
	t = paramSet_isSetByName(set, "t");
	tlv = paramSet_isSetByName(set, "tlv");


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
		print_info("Verifying signature using user publication... ");
		MEASURE_TIME(res = KSI_Signature_verifyWithPublication(sig, ksi, publication));
		ERR_CATCH_MSG(err, res, "Error: Unable to verify signature with user publication.");
		print_info("ok. %s\n", t ? str_measuredTime() : "");
	} else {
		if (isExtended)
			print_warnings("Warning: Publication time of publication string is not matching with signatures publication.\n");
		else
			print_warnings("Warning: Signature is not extended.\n");

		res = KSI_receivePublicationsFile(ksi, &pubFile);
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

		print_info("Extending signature to publication time of publication string... ");
		MEASURE_TIME(res = KSI_Signature_extend(sig, ksi, extendTo, &tmp_ext));
		ERR_CATCH_MSG(err, res, "Error: Unable to extend signature.");
		print_info("ok. %s\n", t ? str_measuredTime() : "");

		print_info("Verifying signature using user publication... ");
		MEASURE_TIME(res = KSI_Signature_verifyWithPublication(tmp_ext, ksi, publication));
		ERR_CATCH_MSG(err, res, "Error: Unable to verify signature with user publication.");
		print_info("ok. %s\n", t ? str_measuredTime() : "");
	}

	if (out != NULL) {
		*out = tmp_ext;
		tmp_ext = NULL;
	}

	res = KT_OK;

cleanup:

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


	print_info("Verifying signature%s ", b && isExtended ? " using local publications file... " : "... ");
	MEASURE_TIME(res = KSI_Signature_verify(sig, ksi));
	ERR_CATCH_MSG(err, res, "Error: Unable to verify signature.");
	print_info("ok. %s\n",t ? str_measuredTime() : "");

	res = KT_OK;

cleanup:

	return res;
}

static int GT_verifyTask_verifyData(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_Signature *sig) {
	int res;
	paramSet *set = NULL;
	bool F, f, t;
	bool isExtended = false;
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
	t = paramSet_isSetByName(set, "t");
	f = paramSet_getStrValueByNameAt(set, "f",0, &inDataFileName);
	F = paramSet_getStrValueByNameAt(set, "F",0, &imprint);


	if(f){
		print_info("Verifying file's %s hash... ", inDataFileName);
		res = KSI_Signature_createDataHasher(sig, &hsr);
		ERR_CATCH_MSG(err, res, "Error: Unable to create data hasher.");
		res = getFilesHash(err, ksi, hsr, inDataFileName, &file_hsh);
		if (res != KT_OK) {
			ERR_TRCKR_ADD(err, res, "Error: Unable to hash file. (%s)", errToString(res));
			goto cleanup;
		}
		res = KSI_Signature_verifyDataHash(sig, ksi, file_hsh);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify files hash.");
		print_info("ok.\n");
	}
	if(F){
		print_info("Verifying imprint... ");
		res = getHashFromCommandLine(imprint, ksi, err, &raw_hsh);
		if (res != KT_OK) goto cleanup;
		res = KSI_Signature_verifyDataHash(sig, ksi, raw_hsh);
		ERR_CATCH_MSG(err, res, "Error: Unable to verify hash.");
		print_info("ok.\n");
	}

	res = KT_OK;

cleanup:

	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(raw_hsh);
	KSI_DataHash_free(file_hsh);

	return res;
}