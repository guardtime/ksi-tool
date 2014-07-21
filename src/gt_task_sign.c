#include <ctype.h>
#include "gt_task_support.h"

static int getHashFromCommandLine(GT_CmdParameters *cmdparam,KSI_CTX *ksi, KSI_DataHash **hash);
static int getHashAlgorithm(const char *hashAlg, int *id);

bool GT_signTask(GT_CmdParameters *cmdparam) {
    KSI_CTX *ksi = NULL;
    int res = KSI_UNKNOWN_ERROR;
    KSI_DataHasher *hsr = NULL;
    KSI_DataHash *hash = NULL;
    KSI_Signature *sign = NULL;
    
    
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
        /*Getting the hash for signing process*/
        if(cmdparam->task == signDataFile){
            char *hashAlg;
            int hasAlgID=-1;
            /*Choosing of hash algorithm and creation of data hasher*/
            printf("Getting hash from file for signing process...");
            hashAlg = cmdparam->H ? (cmdparam->hashAlgName_H) : ("default");
            _DO_TEST_COMPLAIN(getHashAlgorithm(hashAlg, &hasAlgID), "Error: The hash algorithm \"%s\"is unknown\n", hashAlg);
            _DO_TEST_COMPLAIN(KSI_DataHasher_open(ksi,hasAlgID , &hsr),"Error: Unable to create hasher.\n");
            _DO_TEST_COMPLAIN(calculateHashOfAFile(hsr, cmdparam->inDataFileName, &hash ), "Error: Unable to hash data.\n");
            printf("ok.\n");
            }
        else if(cmdparam->task == signHash){
            printf("Getting hash from input string for signing process...");
            _DO_TEST_COMPLAIN(getHashFromCommandLine(cmdparam,ksi, &hash), "Error: Unable to create hash from digest.\n");
            printf("ok.\n");
            }
        else{
            goto cleanup;
            }

        /* Sign the data hash. */
            printf("Creating signature from hash...");
            MEASURE_TIME(_res = KSI_createSignature(ksi, hash, &sign);)
            _DO_TEST_COMPLAIN(_res, "Error: Unable to sign.\n");
            printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");

            /* Save signature file */
            _DO_TEST_COMPLAIN(saveSignatureFile(sign, cmdparam->outSigFileName),"Error: Unable to save signature file %s.\n", cmdparam->outSigFileName);
            printf("Signature saved.\n");
    }_CATCH{
        printf("failed.\n");
        fprintf(stderr , __msg );
        res = _res;
        goto cleanup;
        }};
    
    /*
    printf("Verifying freshly created signature...");
    res = KSI_Signature_verify(sign, ksi);
    ERROR_HANDLING_STATUS_DUMP("\nVerifying failed (%s)\n", KSI_getErrorString(res));
    printf("ok.\n");
    */
    
    if(cmdparam->n)
        printSignerIdentity(sign);
    
    res = KSI_OK;

cleanup:
    KSI_Signature_free(sign);
    KSI_DataHash_free(hash);
    KSI_DataHasher_free(hsr);
    KSI_CTX_free(ksi);
    KSI_global_cleanup();
    return (res==KSI_OK) ? true : false;
}



// helpers for hex decoding
static int x(char c){
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	abort(); // isxdigit lies.
	return -1; // makes compiler happy
}

static int xx(char c1, char c2)
{
	if(!isxdigit(c1) || !isxdigit(c2))
		return -1;
	return x(c1) * 16 + x(c2);
}

static int getBinaryFromHexString(KSI_CTX *ksi, const char *hexin, unsigned char **binout, size_t *lenout){
    size_t len = strlen(hexin);
    unsigned char *tempBin=NULL;
    size_t arraySize = len/2;
    int i,j;
    
    if(len%2 != 0){
        KSI_LOG_debug(ksi, "The hash lenght is not even number!\n");
        return KSI_INVALID_FORMAT ;
        }
    
    tempBin = KSI_calloc(arraySize, sizeof(unsigned char));
    if(tempBin == NULL){
        KSI_LOG_debug(ksi, "Unable to get memory for parsing hex to binary.\n");
        return KSI_OUT_OF_MEMORY ;
        }
    
    for(i=0,j=0; i<arraySize; i++, j+=2){
        int res = xx(hexin[j], hexin[j+1]);
        if(res == -1){
            KSI_LOG_debug(ksi, "The hex number is invalid: %c%c!\n", hexin[j], hexin[j+1]);
            KSI_free(tempBin);
            tempBin = NULL;
            return KSI_INVALID_FORMAT ;
            }
        tempBin[i] = res;
        //printf("%c%c -> %i\n", hexin[j], hexin[j+1], tempBin[i]);
        }
    
    *lenout = arraySize;
    *binout = tempBin;
    
    return KSI_OK;
}


static int getHashFromCommandLine(GT_CmdParameters *cmdparam,KSI_CTX *ksi, KSI_DataHash **hash){
    unsigned char *data;
    size_t len;
    int res = KSI_UNKNOWN_ERROR;
    int hasAlg = -1;
   // KSI_DataHash *temp_hash;
    
    res = getBinaryFromHexString(ksi, cmdparam->inputHashStrn, &data, &len);
    ERROR_HANDLING("Unable to extract Hash value from command line input.\n");
    
    hasAlg = KSI_getHashAlgorithmByName (cmdparam->hashAlgName_F);
    if(hasAlg == -1){
      //  fprintf(stderr, "\nThe hash algorithm \"%s\"is unknown\n", cmdparam->hashAlgName_F);
        res = KSI_INVALID_ARGUMENT;
        goto cleanup;
        }
    
    res = KSI_DataHash_fromDigest(ksi, hasAlg, data, len, hash);
    ERROR_HANDLING("\nUnable to create hash from digest.\n");
    cleanup:
    
    return res;
    }

static int getHashAlgorithm(const char *hashAlg, int *id){
    int hasAlgID = KSI_getHashAlgorithmByName(hashAlg);
    *id = hasAlgID;
    return (hasAlgID == -1) ? KSI_INVALID_ARGUMENT : KSI_OK;
    }
