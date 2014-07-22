#include "gt_task_support.h"
#include "try-catch.h"

bool GT_extendTask(GT_CmdParameters *cmdparam) {
    KSI_CTX *ksi = NULL;
    KSI_Signature *sig = NULL;
    KSI_Signature *ext = NULL;
    bool state = true;

    ResetExeptionHandler();
    try
        CODE{
            /*Initalization of KSI */
            InitTask_throws(cmdparam, &ksi);

            /* Read the signature. */
            printf("Reading signature...");
            KSI_Signature_fromFile_throws(ksi, cmdparam->inSigFileName, &sig);
            printf("ok.\n");

            /* Make sure the signature is ok. */
            printf("Verifying old signature...");
            MEASURE_TIME(KSI_verifySignature(ksi, sig);)
            printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");

            /* Extend the signature. */
            printf("Extending old signature...");
            MEASURE_TIME(KSI_extendSignature_throws(ksi, sig, &ext););
            printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");

            saveSignatureFile_throws(ext, cmdparam->outSigFileName);
            printf("Signature extended.\n");
            }
        CATCH_ALL{
            fprintf(stderr , _EXP.expMsg);
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
    KSI_CTX_free(ksi);
    KSI_global_cleanup();
    return state;
}