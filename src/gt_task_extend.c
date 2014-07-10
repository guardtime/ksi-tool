#include "gt_task.h"


int GT_extendTask(GT_CmdParameters *cmdparam) {
	KSI_CTX *ksi = NULL;
	int res;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;


        res = KSI_global_init();
        ERROR_HANDLING("Unable to init KSI global resources.\n");
        res = KSI_CTX_new(&ksi);
        ERROR_HANDLING("Unable to init KSI context.\n");
        res = configureNetworkProvider(cmdparam, ksi);
        ERROR_HANDLING("Unable to configure network provider.\n");

        
	/* Read the signature. */
        printf("Reading signature...");
	res = KSI_Signature_fromFile(ksi, cmdparam->inSigFileName, &sig);
        ERROR_HANDLING_STATUS_DUMP("failed!\n Unable to read signature from '%s'\n", cmdparam->inSigFileName);
        printf("ok.\n");

	/* Make sure the signature is ok. */
        printf("Verifying old signature...");
        MEASURE_TIME(
            res = KSI_verifySignature(ksi, sig);,
            "\n  -Verifing old signature ",
            cmdparam->t);
        ERROR_HANDLING_STATUS_DUMP("failed!\nUnable to verify signature.\n");
        printf("ok.\n");

	/* Extend the signature. */
        printf("Extending old signature...");
	MEASURE_TIME(
            res = KSI_extendSignature(ksi, sig, &ext);,
            "\n  -Extending old signature ",
            cmdparam->t);
	if (res != KSI_OK) {
		if (res == KSI_EXTEND_NO_SUITABLE_PUBLICATION) {
			printf("failed!\nNo suitable publication to extend to.\n");
			goto cleanup;
		}
		fprintf(stderr, "failed¤\nUnable to extend signature.\n");
		KSI_ERR_statusDump(ksi, stderr);
		goto cleanup;
	}
        printf("ok.\n");
        
	/* To be extra sure, lets verify the extended signature. */
        /*
        printf("Verifying extended signature...");
	res = KSI_verifySignature(ksi, ext);
        ERROR_HANDLING_STATUS_DUMP("failed\n Unable to verify the extended signature.\n");
        printf("ok.\n");       
        */
        res = saveSignatureFile(ext, cmdparam->outSigFileName);
        ERROR_HANDLING("Unable to save signature"); 
        
	

	printf("Signature extended.\n");

cleanup:

	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
	KSI_CTX_free(ksi);
	KSI_global_cleanup();

	return res;
}