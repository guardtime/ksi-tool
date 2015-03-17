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

int GT_extendTask(Task *task) {
	paramSet *set = NULL;
	KSI_CTX *ksi = NULL;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;
	int retval = EXIT_SUCCESS;
	
	KSI_Integer *pubTime = NULL;
	
	bool T,t, n, r, d;
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
	
	resetExceptionHandler();
	try
		CODE{
			/*Initialization of KSI */
			initTask_throws(task, &ksi);
			/* Read the signature. */
			printf("Reading signature... ");
			KSI_Signature_fromFile_throws(ksi, inSigFileName, &sig);
			printf("ok.\n");

			/* Make sure the signature is ok. */
			printf("Verifying old signature... ");
			MEASURE_TIME(KSI_verifySignature(ksi, sig));
			printf("ok. %s\n",t ? str_measuredTime() : "");

			/* Extend the signature. */
			if(T){
				printf("Extending old signature to %i... ", publicationTime);
				KSI_Integer_new_throws(ksi, publicationTime, &pubTime);
				MEASURE_TIME(KSI_Signature_extendTo_throws(sig, ksi, pubTime, &ext));
			}
			else{
				printf("Extending old signature... ");
				MEASURE_TIME(KSI_extendSignature_throws(ksi, sig, &ext));
			}
			printf("ok. %s\n",t ? str_measuredTime() : "");

			printf("Verifying extended signature... ");
			MEASURE_TIME(KSI_Signature_verify_throws(ext, ksi));
			printf("ok. %s\n",t ? str_measuredTime() : "");
			
			/* Save signature. */
			saveSignatureFile_throws(ext, outSigFileName);
			printf("Extended signature saved.\n");
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
	if (n) printSignerIdentity(ext);
	if (r) printSignaturePublicationReference(ext);
	if (d) printSignatureVerificationInfo(ext);

	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
	KSI_Integer_free(pubTime);
	
	closeTask(ksi);
	return retval;
}


