#include "gt_task_support.h"
#include "try-catch.h"
bool GT_verifyTask(Task *task){
	KSI_CTX *ksi = NULL;
	KSI_Signature *sig = NULL;
	KSI_DataHash *hsh = NULL;
	KSI_DataHasher *hsr = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	bool state = true;
	
	bool n,r, d,t, b, f;
	char *inPubFileName = NULL;
	char *inSigFileName = NULL;
	char *inDataFileName = NULL;
	
	b = paramSet_getStrValueByNameAt(task->set, "b",0, &inPubFileName);
	paramSet_getStrValueByNameAt(task->set, "i",0, &inSigFileName);
	f = paramSet_getStrValueByNameAt(task->set, "f",0, &inDataFileName);
	
	n = paramSet_isSetByName(task->set, "n");
	r = paramSet_isSetByName(task->set, "r");
	d = paramSet_isSetByName(task->set, "d");
	t = paramSet_isSetByName(task->set, "t");
	
	resetExeptionHandler();
	try
		CODE{
			/*Initalization of KSI */
			initTask_throws(task, &ksi);

			if (task->id == verifyPublicationsFile) {
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
				if (task->id == verifyTimestamp) {
					printf("Verifying signature %s ", b ? "using local publications file..." : "online...");
					MEASURE_TIME(KSI_Signature_verify_throws(sig, ksi));
					printf("ok. %s\n",t ? str_measuredTime() : "");
				}
				else{
					THROW_MSG(KSI_EXEPTION, "Error: Unexpected error Unknown task.\n ");
				}
				/* If datafile is present compare hash of a datafile and timestamp */
				if (f) {
					/* Create hasher. */
					printf("Verifying file's %s hash...", inDataFileName);
					KSI_Signature_createDataHasher_throws(sig, &hsr);
					getFilesHash_throws(hsr, inDataFileName, &hsh);
					printf("ok.\n");
					printf("Verifying document hash... ");
					KSI_Signature_verifyDataHash_throws(sig, ksi, hsh);
					printf("ok.\n");
				}
			}

			printf("Verification of %s %s successful.\n",
					(task->id == verifyPublicationsFile) ? "publications file" : "signature file",
					(task->id == verifyPublicationsFile) ? inPubFileName : inSigFileName
					);
		}
		CATCH_ALL{
			printf("failed.\n");
			printErrorMessage();
			exeptionSolved();
			state = false;
		}
	end_try
	
	if(n || r || d) printf("\n");
	if (n) printSignerIdentity(sig);
	if (r) printSignaturePublicationReference(sig);
	if (d) printSignatureVerificationInfo(sig);
	
	KSI_Signature_free(sig);
	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(hsh);
	KSI_CTX_free(ksi);
	
	return state;
}