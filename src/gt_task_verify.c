#include "gt_task.h"

int GT_verifyTask(GT_CmdParameters *cmdparam, GT_Tasks task)
{
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

    //if(task == verifyTimestamp_online)

    /* Verification of publications file */
    if (task == verifyPublicationsFile) {
        printf("Reading publications file... ");
        res = KSI_PublicationsFile_fromFile(ksi, cmdparam->inPubFileName, &publicationsFile);
        ERROR_HANDLING_STATUS_DUMP("failed!\nUnable to read publications file %s.\n", cmdparam->inPubFileName);
        printf("ok\n");
        printf("Verifying  publications file... ");
        res = KSI_verifyPublicationsFile(ksi, publicationsFile);
        ERROR_HANDLING_STATUS_DUMP("failed\n")
        printf("ok\n");
    }        /* Verification of signature*/
    else {
        /* Reading signature file for verification. */
        printf("Reading signature... ");
        res = KSI_Signature_fromFile(ksi, cmdparam->inSigFileName, &sig);
        ERROR_HANDLING_STATUS_DUMP("failed (%s)\n", KSI_getErrorString(res));
        printf("ok\n");

        if (cmdparam->n) {
            printSignerIdentity(sig);
            }

        if (cmdparam->r) {
            printSignaturePublicationReference(sig);
        }

        /* Choosing between online and publications file signature verification */
        if (task == verifyTimestamp_online) {
            printf("Verifying signature online... ");
            res = KSI_Signature_verify(sig, ksi);
            ERROR_HANDLING_STATUS_DUMP("failed (%s)\n", KSI_getErrorString(res));
            printf("ok\n");
        } else if (task == verifyTimestamp_locally) {
            printf("TODO verification via publications file.\n");
            goto cleanup;
        } else {
            fprintf(stderr, "Unexpected error. Unknown verification task.\n ");
            goto cleanup;
        }

        /* If datafile is present comparing hash of a datafile and timestamp */
        if (cmdparam->f) {
            /* Create hasher. */
            printf("Verifying file's %s hash...", cmdparam->inDataFileName);
            res = KSI_Signature_createDataHasher(sig, &hsr);
            ERROR_HANDLING(" failed!\nUnable to create data hasher.\n");
            res = calculateHashOfAFile(hsr, &hsh, cmdparam->inDataFileName);
            ERROR_HANDLING(" failed!\nUnable to hash data.\n");
            printf("ok\n");
            printf("Verifying document hash... ");
            res = KSI_Signature_verifyDataHash(sig, hsh);
            ERROR_HANDLING("failed (%s)\nWrong document or signature.\n", KSI_getErrorString(res));
            printf("ok\n");
        }
    }

    printf("Verification of %s %s successful.\n",
            (task == verifyPublicationsFile) ? "publications file" : "signature file",
            (task == verifyPublicationsFile) ? cmdparam->inPubFileName : cmdparam->inSigFileName
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