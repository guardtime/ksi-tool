#include "gt_task.h"




int GT_signTask(GT_CmdParameters *cmdparam, GT_Tasks task) {
    KSI_CTX *ksi = NULL;
    int res = KSI_UNKNOWN_ERROR;
    KSI_DataHasher *hsr = NULL;
    KSI_DataHash *hash = NULL;
    KSI_Signature *sign = NULL;


    /*Initalization of KSI */
    res = KSI_global_init();
    ERROR_HANDLING("Unable to init KSI global resources.\n");
    res = KSI_CTX_new(&ksi);
    ERROR_HANDLING("Unable to init KSI context.\n");
    res = configureNetworkProvider(cmdparam, ksi);
    ERROR_HANDLING("Unable to configure network provider.\n");

        
    /*Getting the hash for signing process*/
    printf("Getting hash for signing process...");
    if(GT_getTask() == signDataFile){
        /*Choosing of hash algorithm and creation of data hasher*/
        char *hashAlg = cmdparam->H ? (cmdparam->rawHashString) : ("default");
        res = KSI_DataHasher_open(ksi, KSI_getHashAlgorithmByName(hashAlg), &hsr);
        ERROR_HANDLING("\nUnable to create hasher.\n");
        res = calculateHashOfAFile(hsr, &hash ,cmdparam->inDataFileName);
        ERROR_HANDLING("\nUnable to hash data.\n");
        }
    else if(GT_getTask() == signHash){
        printf("\ntodo hash signing.\n");
        goto cleanup;
        }
    else{
        goto cleanup;
        }
    printf("ok\n");
    
    /* Sign the data hash. */
    printf("Creating signature from hash...");
    res = KSI_createSignature(ksi, hash, &sign);
    ERROR_HANDLING_STATUS_DUMP("\nUnable to sign %d.\n", res);
    printf("ok\n");
    printf("Verifying freshly created signature...");
    res = KSI_Signature_verify(sign, ksi);
    ERROR_HANDLING("\nVerifying failed (%s)\n", KSI_getErrorString(res));
    printf("ok\n");
    /* Save signature file */
    res = saveSignatureFile(sign, cmdparam->outSigFileName);
    ERROR_HANDLING("Unable to save signature file %s", cmdparam->outSigFileName);    
    printf("Signature saved.\n");
    res = KSI_OK;

cleanup:
    KSI_Signature_free(sign);
    KSI_DataHash_free(hash);
    KSI_DataHasher_free(hsr);
    KSI_CTX_free(ksi);
    KSI_global_cleanup();

    return res;
}