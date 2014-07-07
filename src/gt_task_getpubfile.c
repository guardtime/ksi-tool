#include "gt_task.h"


int GT_getPublicationsFileTask(GT_CmdParameters *cmdparam, GT_Tasks task){
	KSI_CTX *ksi = NULL;
	int res;
	KSI_PublicationsFile *publicationsFile = NULL;
        unsigned int count;
        char *raw=NULL;
        int raw_len=0;
	FILE *out = NULL;

        res = KSI_global_init();
        ERROR_HANDLING("Unable to init KSI global resources.\n");
        res = KSI_CTX_new(&ksi);
        ERROR_HANDLING("Unable to init KSI context.\n");
        res = configureNetworkProvider(cmdparam, ksi);
        ERROR_HANDLING("Unable to configure network provider.\n");

        printf("Downloading publications file...");
        res = KSI_receivePublicationsFile(ksi, &publicationsFile);
        ERROR_HANDLING_STATUS_DUMP("failed!\n Unable to read publications file.\n");
	res = KSI_verifyPublicationsFile(ksi, publicationsFile);
        ERROR_HANDLING_STATUS_DUMP("failed! Unable to verify publications file.\n");
	printf("ok.\n");
        res = KSI_PublicationsFile_serialize(ksi, publicationsFile, &raw, &raw_len);
        ERROR_HANDLING_STATUS_DUMP("Unable serialize publications file.\n");

       /* Open output file. */
        out = fopen(cmdparam->outPubFileName, "wb");
	if (out == NULL) {
            fprintf(stderr, "Unable to ope publications file '%s' for writing.\n", cmdparam->outPubFileName);
            res = KSI_IO_ERROR;
            goto cleanup;
	}
        
        /* Write output file */
        count = fwrite(raw, 1, raw_len, out);
	if (count != raw_len) {
            fprintf(stderr, "Unable to write publications file '%s'.\n", cmdparam->outPubFileName);
            res = KSI_IO_ERROR;
            goto cleanup;
	}
        
        printf("Publications file '%s' saved.\n", cmdparam->outPubFileName);
        
    cleanup:
    	if (out != NULL) fclose(out);
        KSI_CTX_free(ksi);
	KSI_global_cleanup();
        return res;
}