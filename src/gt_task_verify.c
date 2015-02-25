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
int GT_verifyTask(Task *task){
	paramSet *set = NULL;
	KSI_CTX *ksi = NULL;
	KSI_Signature *sig = NULL;
	KSI_DataHash *file_hsh = NULL;
	KSI_DataHash *raw_hsh = NULL;
	KSI_DataHasher *hsr = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	char *imprint = NULL;
	int retval = EXIT_SUCCESS;
	
	bool n,r, d,t, b, f, F;
	char *inPubFileName = NULL;
	char *inSigFileName = NULL;
	char *inDataFileName = NULL;
	
	set = Task_getSet(task);
	b = paramSet_getStrValueByNameAt(set, "b",0, &inPubFileName);
	paramSet_getStrValueByNameAt(set, "i",0, &inSigFileName);
	f = paramSet_getStrValueByNameAt(set, "f",0, &inDataFileName);
	F = paramSet_getStrValueByNameAt(set, "F",0, &imprint);

	n = paramSet_isSetByName(set, "n");
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");
	t = paramSet_isSetByName(set, "t");
	
	resetExceptionHandler();
	try
		CODE{
			/*Initalization of KSI */
			initTask_throws(task, &ksi);

			if (Task_getID(task) == verifyPublicationsFile) {
				printf("Reading publications file... ");
				MEASURE_TIME(KSI_PublicationsFile_fromFile_throws(ksi, inPubFileName, &publicationsFile));
				printf("ok. %s\n",t ? str_measuredTime() : "");

				printf("Verifying  publications file... ");
				KSI_verifyPublicationsFile_throws(ksi, publicationsFile);
				printf("ok.\n");
			}
			/* Verification of signature*/
			else {
				/* Reading signature file for verification. */
				printf("Reading signature... ");
				KSI_Signature_fromFile_throws(ksi, inSigFileName, &sig);
				printf("ok.\n");
		
				/* Choosing between online and publications file signature verification */
				if (Task_getID(task) == verifyTimestamp) {
					printf("Verifying signature %s ", b ? "using local publications file... " : "online... ");
					MEASURE_TIME(KSI_Signature_verify_throws(sig, ksi));
					printf("ok. %s\n",t ? str_measuredTime() : "");
				}

				/* If datafile or imprint is present compare hash and timestamp */
				if(f){
					printf("Verifying file's %s hash... ", inDataFileName);
					KSI_Signature_createDataHasher_throws(ksi, sig, &hsr);
					getFilesHash_throws(ksi, hsr, inDataFileName, &file_hsh);
					KSI_Signature_verifyDataHash_throws(sig, ksi, file_hsh);
					printf("ok.\n");
				}
				if(F){
					printf("Verifying imprint... ");
					getHashFromCommandLine_throws(imprint, ksi, &raw_hsh);
					KSI_Signature_verifyDataHash_throws(sig, ksi, raw_hsh);
					printf("ok.\n");
				}
			}

			printf("Verification of %s %s successful.\n",
					(Task_getID(task) == verifyPublicationsFile) ? "publications file" : "signature file",
					(Task_getID(task) == verifyPublicationsFile) ? inPubFileName : inSigFileName
					);
		}
		CATCH_ALL{
			if(ksi)
				printf("failed.\n");
			printErrorMessage();
			retval = _EXP.exep.ret;
			exceptionSolved();
		}
	end_try
	
	if(n || r || d) printf("\n");
	if (n) printSignerIdentity(sig);
	if (r) printSignaturePublicationReference(sig);
	if (d) printSignatureVerificationInfo(sig);
	
	KSI_Signature_free(sig);
	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(raw_hsh);
	KSI_DataHash_free(file_hsh);
	closeTask(ksi);
	
	return retval;
}