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

static int  getHash(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hsh);

int GT_signTask(Task *task) {
	int res;
	paramSet *set = NULL;
	bool t, n, d, tlv;
	ERR_TRCKR *err = NULL;
	KSI_CTX *ksi = NULL;
	KSI_DataHash *hash = NULL;
	KSI_Signature *sign = NULL;
	int retval = EXIT_SUCCESS;
	char *outSigFileName = NULL;


	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "o", 0,&outSigFileName);
	n = paramSet_isSetByName(set, "n");
	t = paramSet_isSetByName(set, "t");
	d = paramSet_isSetByName(set, "d");
	tlv = paramSet_isSetByName(set, "tlv");


	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;


	/*Get Hash*/
	res = getHash(task, ksi, err, &hash);
	if (res != KT_OK) goto cleanup;

	/* Sign the data hash. */
	print_progressDesc(t, "Creating signature from hash... ");
	res = KSI_createSignature(ksi, hash, &sign);
	ERR_APPEND_KSI_BASE_ERR(err, res, ksi);
	ERR_CATCH_MSG(err, res, "Error: Unable to create signature.");
	print_progressResult(res);

	/* Save signature file */
	res = saveSignatureFile(err, ksi, sign, outSigFileName);
	if (res != KT_OK) goto cleanup;
	print_info("Signature saved.\n");

	/*Print info*/
	if(n || d || tlv) print_info("\n");
	if (n) printSignerIdentity(sign);
	if (d) printSignatureSigningTime(sign);
	if (tlv) printSignatureStructure(ksi, sign);


cleanup:
	print_progressResult(res);

	if (res != KT_OK) {
		ERR_TRCKR_printErrors(err);
		if(d && ksi) KSI_ERR_statusDump(ksi, stderr);
		retval = errToExitCode(res);
	}

	KSI_Signature_free(sign);
	KSI_DataHash_free(hash);
	closeTask(ksi);
	ERR_TRCKR_free(err);

	return retval;
}

static int  getHash(Task *task, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hsh) {
	int res;
	paramSet *set = NULL;
	bool H;
	char *hashAlg;
	int hasAlgID;
	KSI_DataHasher *hsr = NULL;
	KSI_DataHash *tmp = NULL;
	char *inDataFileName = NULL;
	char *imprint = NULL;


	set = Task_getSet(task);
	paramSet_getStrValueByNameAt(set, "f", 0,&inDataFileName);
	paramSet_getStrValueByNameAt(set, "F", 0,&imprint);
	H = paramSet_getStrValueByNameAt(set, "H", 0,&hashAlg);


	if(Task_getID(task) == signDataFile){
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

		res = getFilesHash(err, ksi, hsr, inDataFileName, &tmp);
		if (res != KT_OK) {
			ERR_TRCKR_ADD(err, res, "Error: Unable to hash file. (%s)", errToString(res));
			goto cleanup;
		}
	}
	else if(Task_getID(task) == signHash){
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
