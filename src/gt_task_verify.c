#include "gt_task.h"

#include <stdio.h>
#include <string.h>
#include <ksi.h>
#include <net_curl.h>

int GT_verifyTask(GT_CmdParameters *cmdparam, GT_Tasks task) {
	int res = KSI_UNKNOWN_ERROR;
	KSI_CTX *ksi = NULL;
	KSI_Signature *sig = NULL;
	KSI_DataHash *hsh = NULL;
	KSI_DataHasher *hsr = NULL;
        KSI_PublicationsFile *publicationsFile = NULL;

	/* Init global resources. */
	res = KSI_global_init();
        ERROR_HANDLING("Unable to init KSI global resources.\n");
	res = KSI_CTX_new(&ksi);
        ERROR_HANDLING("Unable to init KSI context.\n");
	res = configureNetworkProvider(cmdparam, ksi);
        ERROR_HANDLING("Unable to configure network provider.\n");
        
        //if(GT_getTask() == verifyTimestamp_online)
        
        /* Verification on publications file or signature*/
        if(GT_getTask() == verifyPublicationsFile){
            printf("Reading publications file... ");
            res = KSI_PublicationsFile_fromFile(ksi, cmdparam->inPubFileName, &publicationsFile);
            ERROR_HANDLING_STATUS_DUMP("failed!\nUnable to read publications file %s.\n", cmdparam->inPubFileName);
            printf("ok\n");
            printf("Verifying  publications file... ");
            res = KSI_verifyPublicationsFile(ksi, publicationsFile);
            ERROR_HANDLING_STATUS_DUMP("failed\n")
            printf("ok\n");
            }
        else{
            /* Reading signature file for verification. */
            printf("Reading signature... ");
            res = KSI_Signature_fromFile(ksi, cmdparam->inSigFileName, &sig);
            ERROR_HANDLING("failed (%s)\n", KSI_getErrorString(res));
            printf("ok\n");
            
            /* Choosing between online and publications file signature verification */
            if((GT_getTask() == verifyTimestamp_and_file_online) || (GT_getTask() == verifyTimestamp_online)){
                printf("Verifying signature online... ");
                res = KSI_Signature_verify(sig, ksi);
                ERROR_HANDLING("failed (%s)\n", KSI_getErrorString(res));
                printf("ok\n");
                }
            else if((GT_getTask() == verifyTimestamp_and_file_use_pubfile) || (GT_getTask() == verifyTimestamp_use_pubfile)){
                printf("TODO verification via publications file.\n");
                goto cleanup;
                }
            else{
                fprintf(stderr, "Unexpected error. Unknown verification task.\n ");
                goto cleanup;
                }
            
            /* Comparing hash of a datafile and timestamp */
            if((GT_getTask() == verifyTimestamp_and_file_online) || (GT_getTask() == verifyTimestamp_and_file_use_pubfile)){
                /* Create hasher. */
                res = KSI_Signature_createDataHasher(sig, &hsr);
                ERROR_HANDLING("Unable to create data hasher.\n");
                res = calculateHashOfAFile(hsr, &hsh ,cmdparam->inDataFileName);
                ERROR_HANDLING("Unable to hash data.\n");
                printf("Verifying document hash... ");
                res = KSI_Signature_verifyDataHash(sig, hsh);
                ERROR_HANDLING("failed (%s)\nWrong document or signature.\n", KSI_getErrorString(res));
                printf("ok\n");
                }
        }

	printf("Verification of %s %s successful.\n", 
                (GT_getTask() == verifyPublicationsFile) ? "publications file" : "signature file",
                (GT_getTask() == verifyPublicationsFile) ? cmdparam->inPubFileName : cmdparam->inSigFileName
                );
	res = KSI_OK;

cleanup:

	KSI_Signature_free(sig);
	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(hsh);
	KSI_CTX_free(ksi);
	KSI_global_cleanup();
	return res;
}