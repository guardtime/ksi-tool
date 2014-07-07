#include "gt_task.h"


int GT_getPublicationsFileTask(GT_CmdParameters *cmdparam, GT_Tasks task){
	KSI_CTX *ksi = NULL;
	int res;
	KSI_PublicationsFile *publicationsFile = NULL;
	const char *fileName = NULL;


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

        res = KSI_PublicationsFile_toFile(ksi, publicationsFile, cmdparam->outPubFileName);
        ERROR_HANDLING_STATUS_DUMP("Unable to save publications file.\n");
        
        printf("Publications file %s saved.\n", cmdparam->outPubFileName);
        
    cleanup:
        KSI_CTX_free(ksi);
	KSI_global_cleanup();
        return res;
}