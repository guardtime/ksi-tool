#include <io.h>

#include "gt_task_support.h"

bool GT_verifyTask(GT_CmdParameters *cmdparam)
{
    int res = KSI_UNKNOWN_ERROR;
    KSI_CTX *ksi = NULL;
    KSI_Signature *sig = NULL;
    KSI_DataHash *hsh = NULL;
    KSI_DataHasher *hsr = NULL;
    KSI_PublicationsFile *publicationsFile = NULL;
    
    /* Init global resources. */
    _TRY{
        _DO_TEST_COMPLAIN(KSI_global_init(), "Unable to init KSI global resources.\n");
        _DO_TEST_COMPLAIN(KSI_CTX_new(&ksi), "Unable to init KSI context. %s\n");
        _DO_TEST_COMPLAIN(configureNetworkProvider(ksi, cmdparam), "Unable to configure network provider.\n")
    }_CATCH{
        fprintf(stderr , __msg );
        res = _res;
        goto cleanup;
        }};

    //if(task == verifyTimestamp_online)

    /* Verification of publications file */
    if (cmdparam->task == verifyPublicationsFile) {
        _TRY{
            printf("Reading publications file... ");
            MEASURE_TIME(_res = KSI_PublicationsFile_fromFile(ksi, cmdparam->inPubFileName, &publicationsFile);)
            _DO_TEST_COMPLAIN(_res, "Error: Unable to read publications file %s.\n", cmdparam->inPubFileName);
            printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
            printf("Verifying  publications file... ");
            _DO_TEST_COMPLAIN(KSI_verifyPublicationsFile(ksi, publicationsFile),"Error: Unable to verify publications file %s.\n", cmdparam->inPubFileName);
            printf("ok.\n");
        }
        _CATCH{
            printf("failed.\n");
            fprintf(stderr , __msg );
            res = _res;
            goto cleanup;
        }};
        
        
    }        /* Verification of signature*/
    else {
        
        _TRY{
        /* Reading signature file for verification. */
            printf("Reading signature... ");
            _DO_TEST_COMPLAIN(KSI_Signature_fromFile(ksi, cmdparam->inSigFileName, &sig), "Error: %s\n", KSI_getErrorString(res));
            printf("ok.\n");
            
            if (cmdparam->n) printSignerIdentity(sig);
            if (cmdparam->r) printSignaturePublicationReference(sig);
            
            /* Choosing between online and publications file signature verification */
            if (cmdparam->task == verifyTimestamp_online) {
                printf("Verifying signature online... ");
                MEASURE_TIME(res = KSI_Signature_verify(sig, ksi);)
                _TEST_COMPLAIN(res != KSI_OK, "Error: %s\n", KSI_getErrorString(res));
                printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
            } else if (cmdparam->task == verifyTimestamp_locally) {
                printf("Verifying signature ... ");
                _TEST_COMPLAIN(false, "Error: Not implemeneted.\n");
            } else {
                _TEST_COMPLAIN(false, "Error: Unexpected error Unknown task.\n ");
            }
            
                    /* If datafile is present comparing hash of a datafile and timestamp */
            if (cmdparam->f) {
                /* Create hasher. */
                printf("Verifying file's %s hash...", cmdparam->inDataFileName);
                _DO_TEST_COMPLAIN(KSI_Signature_createDataHasher(sig, &hsr), "Error: Unable to create data hasher.\n");
                _DO_TEST_COMPLAIN(calculateHashOfAFile(hsr, cmdparam->inDataFileName, &hsh), "Error: Unable to hash data.\n");
                printf("ok.\n");
                printf("Verifying document hash... ");
                _DO_TEST_COMPLAIN(KSI_Signature_verifyDataHash(sig, hsh), "Error: Wrong document or signature.\n", KSI_getErrorString(res));
                printf("ok.\n");
            }
            
        }_CATCH{
            printf("failed.\n");
            fprintf(stderr , __msg );
            res = _res;
            goto cleanup;
        }};
        
    }

    printf("Verification of %s %s successful.\n",
            (cmdparam->task == verifyPublicationsFile) ? "publications file" : "signature file",
            (cmdparam->task == verifyPublicationsFile) ? cmdparam->inPubFileName : cmdparam->inSigFileName
            );
    res = KSI_OK;

cleanup:

    KSI_Signature_free(sig);
    KSI_DataHasher_free(hsr);
    KSI_DataHash_free(hsh);
    KSI_CTX_free(ksi);
    KSI_global_cleanup();
    return (res==KSI_OK) ? true : false;
}