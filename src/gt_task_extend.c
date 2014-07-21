#include "gt_task_support.h"


bool GT_extendTask(GT_CmdParameters *cmdparam) {
	KSI_CTX *ksi = NULL;
	int res;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;


    /*Initalization of KSI */
    _TRY{
        _DO_TEST_COMPLAIN(KSI_global_init(), "Error: Unable to init KSI global resources.\n");
        _DO_TEST_COMPLAIN(KSI_CTX_new(&ksi), "Error: Unable to init KSI context.\n");
        _DO_TEST_COMPLAIN(configureNetworkProvider(ksi, cmdparam), "Error: Unable to configure network provider.\n")
    }_CATCH{
        fprintf(stderr , __msg );
        res = _res;
        goto cleanup;
        }};

    _TRY{
	/* Read the signature. */
        printf("Reading signature...");
        _DO_TEST_COMPLAIN(KSI_Signature_fromFile(ksi, cmdparam->inSigFileName, &sig), "Error: Unable to read signature from '%s'\n", cmdparam->inSigFileName);
        printf("ok.\n");

	/* Make sure the signature is ok. */
        printf("Verifying old signature...");
        MEASURE_TIME(_res = KSI_verifySignature(ksi, sig);)
        _TEST_COMPLAIN(_res != KSI_OK, "Error: Unable to verify signature.\n");
        printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");

	/* Extend the signature. */
        printf("Extending old signature...");
	MEASURE_TIME(_res = KSI_extendSignature(ksi, sig, &ext););
        _TEST_COMPLAIN(_res != KSI_OK, "Error: Unable to extend signature.\n");
        printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
        
        _TEST_COMPLAIN(saveSignatureFile(ext, cmdparam->outSigFileName), "Error: Unable to save signature.\n"); 
	printf("Signature extended.\n");
        }_CATCH{
            printf("failed.\n");
            fprintf(stderr , __msg );
            if (_res == KSI_EXTEND_NO_SUITABLE_PUBLICATION) fprintf(stderr, "Error: No suitable publication to extend to.\n");
        res = _res;
        goto cleanup;
        }};
        
	/* To be extra sure, lets verify the extended signature. */
        /*
        printf("Verifying extended signature...");
	res = KSI_verifySignature(ksi, ext);
        ERROR_HANDLING_STATUS_DUMP("failed\n Unable to verify the extended signature.\n");
        printf("ok.\n");       
        */
        res = KSI_OK;
cleanup:

	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
	KSI_CTX_free(ksi);
	KSI_global_cleanup();
	return (res==KSI_OK) ? true : false;
}