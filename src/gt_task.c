#include "gt_task.h"
#include "gt_cmdparameters.h"

#include <stdio.h>
#include <string.h>




int configureNetworkProvider(GT_CmdParameters *cmdparam, KSI_CTX *ksi){
    int res = KSI_OK;
    KSI_NetProvider *net = NULL;
    /* Check if uri's are specified. */
    if (cmdparam->S || cmdparam->P || cmdparam->X || cmdparam->C || cmdparam->c) {
        res = KSI_UNKNOWN_ERROR;
        res = KSI_CurlNetProvider_new(ksi, &net);
        ERROR_HANDLING("Unable to create new network provider.\n");

        /* Check aggregator url */
        if (cmdparam->S) {
            res = KSI_CurlNetProvider_setSignerUrl(net, cmdparam->signingService_url);
            ERROR_HANDLING("Unable to set aggregator url %s.\n", cmdparam->signingService_url);
            }

        /* Check publications file url. */
        if (cmdparam->P) {
            res = KSI_CurlNetProvider_setPublicationUrl(net, cmdparam->publicationsFile_url);
            ERROR_HANDLING("Unable to set publications file url %s.\n", cmdparam->publicationsFile_url);
            }

        /* Check extending/verification service url. */
        if (cmdparam->X) {
            res = KSI_CurlNetProvider_setExtenderUrl(net, cmdparam->verificationService_url);
            ERROR_HANDLING("Unable to set extender/verifier url %s.\n", cmdparam->verificationService_url);
            }            


        /* Check Network connection timeout. */
        if (cmdparam->C) {
            res = KSI_CurlNetProvider_setConnectTimeoutSeconds(net, cmdparam->networkConnectionTimeout);
            ERROR_HANDLING("Unable to set network connection timeout %i.\n", cmdparam->networkConnectionTimeout);
            }

            /* Check Network transfer timeout. */
        if (cmdparam->c) {
            res = KSI_CurlNetProvider_setReadTimeoutSeconds(net, cmdparam->networkTransferTimeout);
            ERROR_HANDLING("Unable to set network transfer timeout %i.\n", cmdparam->networkTransferTimeout);
            }            

        /* Set the new network provider. */
        res = KSI_setNetworkProvider(ksi, net);
        ERROR_HANDLING("Unable to set network provider.\n");
	}
    
    cleanup:
    //KSI_NetProvider_free(net);
    return res;

}

int calculateHashOfAFile(KSI_DataHasher *hsr, KSI_DataHash **hash ,const char *fname){
    FILE *in = NULL;
    int res = KSI_UNKNOWN_ERROR;
    unsigned char buf[1024];
    int buf_len;
        
    /* Open Input file */
    in = fopen(fname, "rb");
        if (in == NULL){
            fprintf(stderr, "Unable to open input file '%s'\n", fname);
            res = KSI_IO_ERROR;
            goto cleanup;
            }
            
    /* Read the input file and calculate the hash of its contents. */
    while (!feof(in)){
        buf_len = fread(buf, 1, sizeof(buf), in);
        /* Add  next block to the calculation. */
        res = KSI_DataHasher_add(hsr, buf, buf_len);
        ERROR_HANDLING("Unable to add data to hasher.\n");
        }

    /* Close the data hasher and retreive the data hash. */
    res = KSI_DataHasher_close(hsr, hash);
    ERROR_HANDLING("Unable to create hash.\n");

    
    cleanup:
    if (in != NULL) fclose(in);
    return res;

}

int saveSignatureFile(KSI_Signature *sign, const char *fname){
    int res = KSI_UNKNOWN_ERROR;
    unsigned char *raw = NULL;
    int raw_len;
    int count =0;
    FILE *out = NULL;
    /* Serialize the extended signature. */
    
    res = KSI_Signature_serialize(sign, &raw, &raw_len);
    ERROR_HANDLING("Unable to serialize signature.\n");


    /* Open output file. */
    out = fopen(fname, "wb");
	if (out == NULL) {
            fprintf(stderr, "Unable to open output file '%s'\n", fname);
            res = KSI_IO_ERROR;
            goto cleanup;
	}

	count = fwrite(raw, 1, raw_len, out);
	if (count != raw_len) {
            fprintf(stderr, "Failed to write output file.\n");
            res = KSI_IO_ERROR;
            goto cleanup;
	}
        
    cleanup:
    if (out != NULL) fclose(out);
    KSI_free(raw);
    return res;
    }

        
        
        
