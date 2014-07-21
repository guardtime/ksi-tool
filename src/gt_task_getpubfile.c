#include "gt_task_support.h"

bool GT_getPublicationsFileTask(GT_CmdParameters *cmdparam)
{
    KSI_CTX *ksi = NULL;
    int res;
    KSI_PublicationsFile *publicationsFile = NULL;
    unsigned int count;
    char *raw = NULL;
    int raw_len = 0;
    FILE *out = NULL;

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
        printf("Downloading publications file...");
        MEASURE_TIME(_res = KSI_receivePublicationsFile(ksi, &publicationsFile);)
        _DO_TEST_COMPLAIN(_res, "Error: Unable to read publications file.\n");
        printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
        
        printf("Verifying publications file...");
        
        MEASURE_TIME(_res = KSI_verifyPublicationsFile(ksi, publicationsFile);)
        _DO_TEST_COMPLAIN(_res, "Unable to verify publications file.\n");
        printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
        
        _DO_TEST_COMPLAIN(KSI_PublicationsFile_serialize(ksi, publicationsFile, &raw, &raw_len), "Error: Unable serialize publications file.\n");
                
        /* Open output file. */
        out = fopen(cmdparam->outPubFileName, "wb");
        _TEST_COMPLAIN(out == NULL, "Unable to ope publications file '%s' for writing.\n", cmdparam->outPubFileName);
        
        /* Write output file */
        count = fwrite(raw, 1, raw_len, out);
        _TEST_COMPLAIN(count != raw_len, "Error: Unable to write publications file '%s'.\n", cmdparam->outPubFileName);
    }
    _CATCH{
        printf("failed.\n");
        if(_res == KSI_OK) _res = KSI_IO_ERROR;
        fprintf(stderr , __msg );
        res = _res;
        goto cleanup;
        }}
    

    printf("Publications file '%s' saved.\n", cmdparam->outPubFileName);
    res = KSI_OK;
cleanup:
    if (out != NULL) fclose(out);
    KSI_CTX_free(ksi);
    KSI_global_cleanup();
    return (res==KSI_OK) ? true : false;
}