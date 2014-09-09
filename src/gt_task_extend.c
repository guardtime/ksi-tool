#include "gt_task_support.h"
#include "try-catch.h"

bool GT_extendTask(GT_CmdParameters *cmdparam) {
    KSI_CTX *ksi = NULL;
    KSI_Signature *sig = NULL;
    KSI_Signature *ext = NULL;
    bool state = true;
	KSI_PublicationRecord *pPubRec = NULL;
	KSI_PublicationData *pPublicationData = NULL;
	KSI_Integer *pubTime = NULL;
	KSI_DataHash *dummyHash = NULL;
	
	
	
    resetExeptionHandler();
    try
        CODE{
            /*Initalization of KSI */
            initTask_throws(cmdparam, &ksi);
			KSI_CTX_setLogLevel(ksi, KSI_LOG_DEBUG);
            /* Read the signature. */
            printf("Reading signature...");
            KSI_Signature_fromFile_throws(ksi, cmdparam->inSigFileName, &sig);
            printf("ok.\n");

            /* Make sure the signature is ok. */
            printf("Verifying old signature...");
            MEASURE_TIME(KSI_verifySignature(ksi, sig));
            printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");

			
            /* Extend the signature. */
            if(cmdparam->T){
				printf("Extending old signature to %i...", cmdparam->publicationTime);
				
				KSI_PublicationRecord_new(ksi, &pPubRec);
				KSI_PublicationData_new(ksi, &pPublicationData);
				KSI_Integer_new(ksi, cmdparam->publicationTime, &pubTime);
				KSI_Signature_getDocumentHash(sig, &dummyHash);
				KSI_PublicationData_setTime(pPublicationData, pubTime);
				KSI_PublicationData_setImprint(pPublicationData, dummyHash);
				KSI_PublicationRecord_setPublishedData(pPubRec, pPublicationData);
				
				MEASURE_TIME(KSI_Signature_extend_throws(sig, ksi, pPubRec, &ext));
			}
			else{
				printf("Extending old signature...");
				MEASURE_TIME(KSI_extendSignature_throws(ksi, sig, &ext));
			}
			
			
            printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
            try
				CODE{
					if (cmdparam->r) printSignaturePublicationReference_throws(ext);
				}
				CATCH_ALL{
					printErrorLocations();
					exeptionSolved();
				}
			end_try
            /* Save signature. */
            saveSignatureFile_throws(ext, cmdparam->outSigFileName);
            printf("Signature extended.\n");
            }
        CATCH_ALL{
			printf("failed.\n");
            printErrorLocations();
            exeptionSolved();
            state = false;
            goto cleanup;
            }
    end_try
        
    /* To be extra sure, lets verify the extended signature. */
    /*
    printf("Verifying extended signature...");
    res = KSI_verifySignature(ksi, ext);
    ERROR_HANDLING_STATUS_DUMP("failed\n Unable to verify the extended signature.\n");
    printf("ok.\n");       
    */
        
cleanup:

    KSI_Signature_free(sig);
    KSI_Signature_free(ext);
    //KSI_PublicationRecord_free(pPubRec);
	//KSI_PublicationData_free(pPublicationData);
	//KSI_Integer_free(pubTime);
	KSI_CTX_free(ksi);
    return state;
}