#include "gt_task_support.h"
#include "try-catch.h"
bool GT_verifyTask(GT_CmdParameters *cmdparam){
	KSI_CTX *ksi = NULL;
	KSI_Signature *sig = NULL;
	KSI_DataHash *hsh = NULL;
	KSI_DataHasher *hsr = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	bool state = true;

	resetExeptionHandler();
	try
		CODE{
			/*Initalization of KSI */
			initTask_throws(cmdparam, &ksi);

			if (cmdparam->task == verifyPublicationsFile) {
				printf("Reading publications file... ");
				MEASURE_TIME(KSI_PublicationsFile_fromFile_throws(ksi, cmdparam->inPubFileName, &publicationsFile));
				printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");

				printf("Verifying  publications file... ");
				KSI_verifyPublicationsFile_throws(ksi, publicationsFile);
				printf("ok.\n");
			}
			/* Verification of signature*/
			else {
				/* Reading signature file for verification. */
				printf("Reading signature... ");
				KSI_Signature_fromFile_throws(ksi, cmdparam->inSigFileName, &sig);
				printf("ok.\n");
		
				/* Choosing between online and publications file signature verification */
				if (cmdparam->task == verifyTimestamp) {
					printf("Verifying signature %s ", cmdparam->b ? "using local publications file..." : "online...");
					MEASURE_TIME(KSI_Signature_verify_throws(sig, ksi));
					printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
				}
				else{
					THROW_MSG(KSI_EXEPTION, "Error: Unexpected error Unknown task.\n ");
				}
				/* If datafile is present compare hash of a datafile and timestamp */
				if (cmdparam->f) {
					/* Create hasher. */
					printf("Verifying file's %s hash...", cmdparam->inDataFileName);
					KSI_Signature_createDataHasher_throws(sig, &hsr);
					getFilesHash_throws(hsr, cmdparam->inDataFileName, &hsh);
					printf("ok.\n");
					printf("Verifying document hash... ");
					KSI_Signature_verifyDataHash_throws(sig, ksi, hsh);
					printf("ok.\n");
				}
			}

			printf("Verification of %s %s successful.\n",
					(cmdparam->task == verifyPublicationsFile) ? "publications file" : "signature file",
					(cmdparam->task == verifyPublicationsFile) ? cmdparam->inPubFileName : cmdparam->inSigFileName
					);
		}
		CATCH_ALL{
			printf("failed.\n");
			printErrorLocations();
			exeptionSolved();
			state = false;
		}
	end_try
	
	if(cmdparam->n || cmdparam->r || cmdparam->d) printf("\n");
	if (cmdparam->n) printSignerIdentity(sig);
	if (cmdparam->r) printSignaturePublicationReference(sig);
	if (cmdparam->d) printSignatureVerificationInfo(sig);
	
	KSI_Signature_free(sig);
	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(hsh);
	KSI_CTX_free(ksi);
	
	return state;
}