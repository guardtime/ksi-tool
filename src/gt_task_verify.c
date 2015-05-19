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
	KSI_PublicationData *publication = NULL;
	KSI_PublicationRecord *extendTo = NULL;
	KSI_Signature *tmp_ext = NULL;
	KSI_Signature *dumpBuf = NULL;
	char *imprint = NULL;
	int retval = EXIT_SUCCESS;

	bool n, r, d, t, b, f, F, x, ref;
	char *inPubFileName = NULL;
	char *inSigFileName = NULL;
	char *inDataFileName = NULL;
	char *refStrn = NULL;

	set = Task_getSet(task);
	b = paramSet_getStrValueByNameAt(set, "b",0, &inPubFileName);
	paramSet_getStrValueByNameAt(set, "i",0, &inSigFileName);
	f = paramSet_getStrValueByNameAt(set, "f",0, &inDataFileName);
	F = paramSet_getStrValueByNameAt(set, "F",0, &imprint);
	ref = paramSet_getStrValueByNameAt(set, "ref",0, &refStrn);

	n = paramSet_isSetByName(set, "n");
	r = paramSet_isSetByName(set, "r");
	d = paramSet_isSetByName(set, "d");
	t = paramSet_isSetByName(set, "t");
	x = paramSet_isSetByName(set, "x");

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
				bool isExtended = false;
				/* Reading signature file for verification. */
				printf("Reading signature... ");
				KSI_Signature_fromFile_throws(ksi, inSigFileName, &sig);
				printf("ok.\n");

				isExtended = isSignatureExtended(sig);

				/* Choosing between online and publications file signature verification */
				if (Task_getID(task) == verifyTimestampOnline) {
					printf("Verifying online... ");
					MEASURE_TIME(KSI_Signature_verifyOnline_throws(ksi, sig));
					printf("ok. %s\n",t ? str_measuredTime() : "");
				}
				else if(Task_getID(task) == verifyTimestamp) {
					if (ref) {
						KSI_PublicationRecord *pubRec = NULL;
						KSI_PublicationData *pubData = NULL;
						KSI_Integer *timeA = NULL;
						KSI_Integer *timeB = NULL;

						KSI_PublicationData_fromBase32_throws(ksi, refStrn, &publication);
						KSI_PublicationData_getTime_throws(ksi, publication, &timeB);

						if (isExtended) {
							KSI_Signature_getPublicationRecord_throws(ksi, sig, &pubRec);
							KSI_PublicationRecord_getPublishedData_throws(ksi, pubRec, &pubData);
							KSI_PublicationData_getTime_throws(ksi, pubData, &timeA);
						}

						if (isExtended && KSI_Integer_equals(timeA, timeB)) {
							printf("Verifying signature using user publication... ");
							MEASURE_TIME(KSI_Signature_verifyWithPublication_throws(sig, ksi, publication));
						} else {
							KSI_PublicationRecord *pubRec = NULL;
							KSI_PublicationsFile *pubFile = NULL;
							if (isExtended)
								printf("Warning: Publication time of publication string is not matching with signatures publication.\n");
							else
								printf("Warning: Signature is not extended.\n");

							KSI_receivePublicationsFile_throws(ksi, &pubFile);
							KSI_PublicationsFile_getPublicationDataByPublicationString_throws(ksi, pubFile, refStrn, &pubRec);

							if (pubRec == NULL) {
								KSI_PublicationRecord_new_throws(ksi, &extendTo);
								KSI_PublicationRecord_setPublishedData_throws(ksi, extendTo, publication);
								publication = NULL;
								KSI_PublicationData_fromBase32_throws(ksi, refStrn, &publication);
							} else {
								KSI_PublicationRecord_clone_throws(ksi, pubRec, &extendTo);
							}

							printf("Extending signature to publication time of publication string... ");
							KSI_Signature_extend_throws(sig, ksi, extendTo, &tmp_ext);
							printf("ok.\n");

							printf("Verifying signature using user publication... ");
							MEASURE_TIME(KSI_Signature_verifyWithPublication_throws(tmp_ext, ksi, publication));
						}

					} else {
						if (ref) {
							printf("Warning: Signature is not extended.\n");
						}
						printf("Verifying signature%s ", b && isExtended ? " using local publications file... " : "... ");
						MEASURE_TIME(KSI_Signature_verify_throws(sig, ksi));
					}
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

	if (n || r || d) printf("\n");

	if ((n || r || d) &&  Task_getID(task) == verifyTimestamp || Task_getID(task) == verifyTimestampOnline){
		if (n) printSignerIdentity(tmp_ext != NULL ? tmp_ext : sig);
		if (r) printSignaturePublicationReference(tmp_ext != NULL ? tmp_ext : sig);
		if (d) {
			printSignatureVerificationInfo(tmp_ext != NULL ? tmp_ext : sig);
			printSignatureSigningTime(tmp_ext != NULL ? tmp_ext : sig);
		}
	}

	if(d && Task_getID(task) == verifyPublicationsFile){
		printPublicationsFileReferences(publicationsFile);
		printPublicationsFileCertificates(publicationsFile);
	}

	KSI_Signature_free(sig);
	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(raw_hsh);
	KSI_DataHash_free(file_hsh);
	KSI_PublicationData_free(publication);
	KSI_PublicationRecord_free(extendTo);
	KSI_Signature_free(tmp_ext);
	KSI_PublicationsFile_free(publicationsFile);
	closeTask(ksi);

	return retval;
}