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
#include "param_set/param_value.h"

static int getHash(TASK *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hsh);

int GT_signTask(TASK *task) {
	int res;
	PARAM_SET *set = NULL;
	bool t, n, d;
	ERR_TRCKR *err = NULL;
	KSI_CTX *ksi = NULL;
	KSI_DataHash *hash = NULL;
	KSI_Signature *sign = NULL;
	int retval = EXIT_SUCCESS;
	char *outSigFileName = NULL;


	set = TASK_getSet(task);
	PARAM_SET_getStrValue(set, "o", NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, &outSigFileName);
	n = PARAM_SET_isSetByName(set, "n");
	t = PARAM_SET_isSetByName(set, "t");
	d = PARAM_SET_isSetByName(set, "d");


	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;


	/*Get Hash*/
	res = getHash(task, ksi, err, &hash);
	if (res != KT_OK) goto cleanup;

	/* Sign the data hash. */
	print_progressDesc(t, "Creating signature from hash... ");
	res = KSITOOL_createSignature(err, ksi, hash, &sign);
	ERR_CATCH_MSG(err, res, "Error: Unable to create signature.");
	print_progressResult(res);

	/* Save signature file */
	res = saveSignatureFile(err, ksi, sign, outSigFileName);
	if (res != KT_OK) goto cleanup;
	print_info("Signature saved.\n");

	/*Print info*/
	if(n || d) print_info("\n");
	if (n) OBJPRINT_signerIdentity(sign);
	if (d) OBJPRINT_signatureSigningTime(sign);


cleanup:
	print_progressResult(res);
	KSITOOL_KSI_ERRTrace_save(ksi);

	if (res != KT_OK) {
		ERR_TRCKR_printErrors(err);
		retval = errToExitCode(res);
	}

	KSI_Signature_free(sign);
	KSI_DataHash_free(hash);
	closeTask(ksi);
	ERR_TRCKR_free(err);

	return retval;
}

static int  getHash(TASK *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hsh) {
	int res;
	PARAM_SET *set = NULL;
	bool H;
	char *hashAlg;
	KSI_HashAlgorithm hasAlgID;
	KSI_DataHasher *hsr = NULL;
	KSI_DataHash *tmp = NULL;
	char *inDataFileName = NULL;
	char *outDataFileName = NULL;
	char *imprint = NULL;


	set = TASK_getSet(task);
	PARAM_SET_getStrValue(set, "f", NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, &inDataFileName);
	PARAM_SET_getStrValue(set, "D", NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, &outDataFileName);
	PARAM_SET_getStrValue(set, "F", NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, &imprint);
	H = PARAM_SET_getStrValue(set, "H", NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, &hashAlg) == PST_OK ? true : false;


	if(TASK_getID(task) == signDataFile){
		print_progressDesc(false, "Getting hash from file for signing process... ");
		hashAlg = H ? (hashAlg) : ("default");
		hasAlgID = KSI_getHashAlgorithmByName(hashAlg);
		if (hasAlgID == -1) {
			res = KT_UNKNOWN_HASH_ALG;
			ERR_TRCKR_ADD(err, res, "Error: The hash algorithm \"%s\" is unknown.", hashAlg);
			goto cleanup;
		}

		res = KSI_DataHasher_open(ksi, hasAlgID, &hsr);
		ERR_APPEND_KSI_ERR(err, res, KSI_UNAVAILABLE_HASH_ALGORITHM);
		ERR_CATCH_MSG(err, res, "Error: Unable to open hasher.");

		res = getFilesHash(err, hsr, inDataFileName, outDataFileName, &tmp);
		if (res != KT_OK) {
			ERR_TRCKR_ADD(err, res, "Error: Unable to hash file. (%s)", errToString(res));
			goto cleanup;
		}
	}
	else if(TASK_getID(task) == signHash){
		print_progressDesc(false, "Getting hash from input string for signing process... ");
		res = getHashFromCommandLine(imprint, ksi, err, &tmp);
		if (res != KT_OK) goto cleanup;
	}

	*hsh = tmp;
	tmp = NULL;

	res = KT_OK;
	print_progressResult(res);


cleanup:
	print_progressResult(res);

	KSI_DataHash_free(tmp);
	KSI_DataHasher_free(hsr);

	return res;
}
