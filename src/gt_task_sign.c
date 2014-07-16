#include <ctype.h>
#include "gt_task.h"

static int getHashFromCommandLine(GT_CmdParameters *cmdparam,KSI_CTX *ksi, KSI_DataHash **hash);


int GT_signTask(GT_CmdParameters *cmdparam) {
    KSI_CTX *ksi = NULL;
    int res = KSI_UNKNOWN_ERROR;
    KSI_DataHasher *hsr = NULL;
    KSI_DataHash *hash = NULL;
    KSI_Signature *sign = NULL;
    
    
    /*Initalization of KSI */
    res = KSI_global_init();
    ERROR_HANDLING("Unable to init KSI global resources.\n");
    res = KSI_CTX_new(&ksi);
    ERROR_HANDLING("Unable to init KSI context. %s\n", KSI_getErrorString(res));
    res = configureNetworkProvider(cmdparam, ksi);
    ERROR_HANDLING("Unable to configure network provider.\n");

        
    /*Getting the hash for signing process*/
    if(cmdparam->task == signDataFile){
        char *hashAlg;
        int hasAlgID=-1;
        /*Choosing of hash algorithm and creation of data hasher*/
        printf("Getting hash from file for signing process...");
        hashAlg = cmdparam->H ? (cmdparam->hashAlgName_H) : ("default");
        hasAlgID = KSI_getHashAlgorithmByName(hashAlg);
        if(hasAlgID == -1){
            fprintf(stderr, "The hash algorithm \"%s\"is unknown\n", cmdparam->hashAlgName_F);
            res = KSI_INVALID_ARGUMENT;
            goto cleanup;
        }
        res = KSI_DataHasher_open(ksi,hasAlgID , &hsr);
        ERROR_HANDLING("\nUnable to create hasher.\n");
        res = calculateHashOfAFile(hsr, &hash ,cmdparam->inDataFileName);
        ERROR_HANDLING("\nUnable to hash data.\n");
        printf("ok.\n");
        }
    else if(cmdparam->task == signHash){
        printf("Getting hash from input string for signing process...");
        res = getHashFromCommandLine(cmdparam,ksi, &hash);
        ERROR_HANDLING_STATUS_DUMP("\nUnable to create hash from digest.\n");
        printf("ok.\n");
        }
    else{
        goto cleanup;
        }
    
    /* Sign the data hash. */
    printf("Creating signature from hash...");
    MEASURE_TIME(res = KSI_createSignature(ksi, hash, &sign);)
    ERROR_HANDLING_STATUS_DUMP("\nUnable to sign %d.\n", res);
    printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
    /*
    printf("Verifying freshly created signature...");
    res = KSI_Signature_verify(sign, ksi);
    ERROR_HANDLING_STATUS_DUMP("\nVerifying failed (%s)\n", KSI_getErrorString(res));
    printf("ok.\n");
    */
    
    if(cmdparam->n)
        printSignerIdentity(sign);
    
    /* Save signature file */
    res = saveSignatureFile(sign, cmdparam->outSigFileName);
    ERROR_HANDLING("Unable to save signature file %s", cmdparam->outSigFileName);    
    printf("Signature saved.\n");
    res = KSI_OK;

cleanup:
    KSI_Signature_free(sign);
    KSI_DataHash_free(hash);
    KSI_DataHasher_free(hsr);
    KSI_CTX_free(ksi);
    KSI_global_cleanup();
    return res;
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
    ERROR_HANDLING("\nUnable to extract Hash value from command line input.\n");
    
    hasAlg = KSI_getHashAlgorithmByName (cmdparam->hashAlgName_F);
    if(hasAlg == -1){
        fprintf(stderr, "\nThe hash algorithm \"%s\"is unknown\n", cmdparam->hashAlgName_F);
        res = KSI_INVALID_ARGUMENT;
        goto cleanup;
        }
    
    res = KSI_DataHash_fromDigest(ksi, hasAlg, data, len, hash);
    ERROR_HANDLING("\nUnable to create hash from digest.\n");
    cleanup:
    
    return res;
    }


