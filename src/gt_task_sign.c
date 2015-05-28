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
#include "try-catch.h"

static void printSignaturesRootHash_and_Time(const KSI_Signature *sig);

int GT_signTask(Task *task) {
	paramSet *set = NULL;
	KSI_CTX *ksi = NULL;
	KSI_DataHasher *hsr = NULL;
	KSI_DataHash *hash = NULL;
	KSI_Signature *sign = NULL;
	int retval = EXIT_SUCCESS;

	bool H, t, n, d, tlv;
	char *hashAlgName_H = NULL;
	char *inDataFileName = NULL;
	char *outSigFileName = NULL;
	char *imprint = NULL;

	set = Task_getSet(task);
	H = paramSet_getStrValueByNameAt(set, "H", 0,&hashAlgName_H);
	paramSet_getStrValueByNameAt(set, "f", 0,&inDataFileName);
	paramSet_getStrValueByNameAt(set, "o", 0,&outSigFileName);
	paramSet_getStrValueByNameAt(set, "F", 0,&imprint);
	n = paramSet_isSetByName(set, "n");
	t = paramSet_isSetByName(set, "t");
	d = paramSet_isSetByName(set, "d");
	tlv = paramSet_isSetByName(set, "tlv");

	resetExceptionHandler();
	try
		CODE{
			/*Initalization of KSI */
			initTask_throws(task, &ksi);
			/*Getting the hash for signing process*/
			if(Task_getID(task) == signDataFile){
				char *hashAlg;
				int hasAlgID=-1;
				/*Choosing of hash algorithm and creation of data hasher*/
				print_info("Getting hash from file for signing process... ");
				hashAlg = H ? (hashAlgName_H) : ("default");
				hasAlgID = getHashAlgorithm_throws(hashAlg);
				KSI_DataHasher_open_throws(ksi,hasAlgID , &hsr);
				getFilesHash_throws(ksi, hsr, inDataFileName, &hash );
				print_info("ok.\n");
			}
			else if(Task_getID(task) == signHash){
				print_info("Getting hash from input string for signing process... ");
				getHashFromCommandLine_throws(imprint,ksi, &hash);
				print_info("ok.\n");
			}

			/* Sign the data hash. */
			print_info("Creating signature from hash... ");
			MEASURE_TIME(KSI_createSignature_throws(ksi, hash, &sign));
			print_info("ok. %s\n",t ? str_measuredTime() : "");

			/* Save signature file */
			saveSignatureFile_throws(sign, outSigFileName);
			print_info("Signature saved.\n");
		}
		CATCH_ALL{
			if(ksi)
				print_errors("failed.\n");
			printErrorMessage();
			retval = _EXP.exep.ret;
			exceptionSolved();
		}
	end_try

	if(n || d || tlv) print_info("\n");
	if (n) printSignerIdentity(sign);
	if (d) printSignatureSigningTime(sign);
	if (tlv) printSignatureStructure(ksi, sign);

	KSI_Signature_free(sign);
	KSI_DataHash_free(hash);
	KSI_DataHasher_free(hsr);
	closeTask(ksi);

	return retval;
}






