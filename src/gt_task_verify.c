#include <io.h>

#include "gt_task_support.h"
#include "try-catch.h"
bool GT_verifyTask(GT_CmdParameters *cmdparam)
{
    KSI_CTX *ksi = NULL;
    KSI_Signature *sig = NULL;
    KSI_DataHash *hsh = NULL;
    KSI_DataHasher *hsr = NULL;
    KSI_PublicationsFile *publicationsFile = NULL;
    bool state = true;
    
    ResetExeptionHandler();
    try
        CODE{
            /*Initalization of KSI */
            InitTask_throws(cmdparam, &ksi);
            
            if (cmdparam->task == verifyPublicationsFile) {
                printf("Reading publications file... ");
                MEASURE_TIME(KSI_PublicationsFile_fromFile_throws(ksi, cmdparam->inPubFileName, &publicationsFile);)
                printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
                
                printf("Verifying  publications file... ");
                KSI_verifyPublicationsFile_throws(ksi, publicationsFile);
                printf("ok.\n");
                }        /* Verification of signature*/
            else {
                /* Reading signature file for verification. */
                printf("Reading signature... ");
                KSI_Signature_fromFile_throws(ksi, cmdparam->inSigFileName, &sig);
                printf("ok.\n");

                if (cmdparam->n) printSignerIdentity_throws(sig);
                if (cmdparam->r) printSignaturePublicationReference_throws(sig);

                /* Choosing between online and publications file signature verification */
                if (cmdparam->task == verifyTimestamp_online) {
                    printf("Verifying signature online... ");
                    MEASURE_TIME(KSI_Signature_verify_throws(sig, ksi);)
                    printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
                } else if (cmdparam->task == verifyTimestamp_locally) {
                    printf("Verifying signature ... ");
                    THROW_MSG(KSI_EXEPTION, "Error: Not implemeneted.\n");
                } else {
                    THROW_MSG(KSI_EXEPTION, "Error: Unexpected error Unknown task.\n ");
                        }
                        /* If datafile is present comparing hash of a datafile and timestamp */
                if (cmdparam->f) {
                    /* Create hasher. */
                    printf("Verifying file's %s hash...", cmdparam->inDataFileName);
                    KSI_Signature_createDataHasher_throws(sig, &hsr);
                    getFilesHash_throws(hsr, cmdparam->inDataFileName, &hsh);
                    printf("ok.\n");
                    printf("Verifying document hash... ");
                    KSI_Signature_verifyDataHash_throws(sig, hsh);
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
            fprintf(stderr , _EXP.expMsg);
            state = false;
            goto cleanup;
            }
    end_try

cleanup:

    KSI_Signature_free(sig);
    KSI_DataHasher_free(hsr);
    KSI_DataHash_free(hsh);
    KSI_CTX_free(ksi);
    KSI_global_cleanup();
    return state;
}