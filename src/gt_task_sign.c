#include <ctype.h>
#include "gt_task_support.h"
#include "try-catch.h"

static void getHashFromCommandLine_throws(GT_CmdParameters *cmdparam,KSI_CTX *ksi, KSI_DataHash **hash);
static int getHashAlgorithm_throws(const char *hashAlg);

bool GT_signTask(GT_CmdParameters *cmdparam) {
    KSI_CTX *ksi = NULL;
    KSI_DataHasher *hsr = NULL;
    KSI_DataHash *hash = NULL;
    KSI_Signature *sign = NULL;
    bool state = true;
    resetExeptionHandler();
    try
        CODE{
        /*Initalization of KSI */
        initTask_throws(cmdparam, &ksi);
        
        /*Getting the hash for signing process*/
        if(cmdparam->task == signDataFile){
            char *hashAlg;
            int hasAlgID=-1;
            /*Choosing of hash algorithm and creation of data hasher*/
            printf("Getting hash from file for signing process...");
            hashAlg = cmdparam->H ? (cmdparam->hashAlgName_H) : ("default");
            hasAlgID = getHashAlgorithm_throws(hashAlg);
            KSI_DataHasher_open_throws(ksi,hasAlgID , &hsr);
            getFilesHash_throws(hsr, cmdparam->inDataFileName, &hash );
            printf("ok.\n");
            }
        else if(cmdparam->task == signHash){
            printf("Getting hash from input string for signing process...");
            getHashFromCommandLine_throws(cmdparam,ksi, &hash);
            printf("ok.\n");
            }
        else{
            goto cleanup;
            }

        /* Sign the data hash. */
        printf("Creating signature from hash...");
        MEASURE_TIME(KSI_createSignature_throws(ksi, hash, &sign));
        printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");
        
        if(cmdparam->n) printSignerIdentity_throws(sign);
        
        /* Save signature file */
        saveSignatureFile_throws(sign, cmdparam->outSigFileName);
        printf("Signature saved.\n");
        }
    CATCH_ALL{
        printf("failed.\n");
        printErrorLocations();
        exeptionSolved();
        state = false;
        goto cleanup;
        }
    end_try

    /*
    printf("Verifying freshly created signature...");
    res = KSI_Signature_verify(sign, ksi);
    ERROR_HANDLING_STATUS_DUMP("\nVerifying failed (%s)\n", KSI_getErrorString(res));
    printf("ok.\n");
    */
cleanup:
    KSI_Signature_free(sign);
    KSI_DataHash_free(hash);
    KSI_DataHasher_free(hsr);
    KSI_CTX_free(ksi);
    return state;
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

/**
 * Converts a string into binary array.
 * 
 * @param[in] ksi Pointer to KSI KSI_CTX object.
 * @param[in] hexin Pointer to string for conversion.
 * @param[out] binout Pointer to receiving pointer to binary array.  
 * @param[out] lenout Pointer to binary array length.
 * 
 * @throws INVALID_ARGUMENT_EXEPTION.
 */
static void getBinaryFromHexString(KSI_CTX *ksi, const char *hexin, unsigned char **binout, size_t *lenout){
    size_t len = strlen(hexin);
    unsigned char *tempBin=NULL;
    size_t arraySize = len/2;
    int i,j;
    
    if(len%2 != 0){
        THROW_MSG(INVALID_ARGUMENT_EXEPTION, "Error: The hash lenght is not even number!\n");
        }
    
    tempBin = KSI_calloc(arraySize, sizeof(unsigned char));
    if(tempBin == NULL){
        THROW_MSG(INVALID_ARGUMENT_EXEPTION, "Error: Unable to get memory for parsing hex to binary.\n");
        }
    
    for(i=0,j=0; i<arraySize; i++, j+=2){
        int res = xx(hexin[j], hexin[j+1]);
        if(res == -1){
            KSI_free(tempBin);
            tempBin = NULL;
            THROW_MSG(INVALID_ARGUMENT_EXEPTION, "Error: The hex number is invalid: %c%c!\n", hexin[j], hexin[j+1]);
            }
        tempBin[i] = res;
        //printf("%c%c -> %i\n", hexin[j], hexin[j+1], tempBin[i]);
        }
    
    *lenout = arraySize;
    *binout = tempBin;
    
    return;
}

/**
 * Reads hash from command line and creates the KSI_DataHash object.
 * 
 * @param[in] cmdparam Pointer to command line data object.
 * @param[in] ksi Pointer to ksi context object.
 * @param[out] hash Pointer to receiving pointer to KSI_DataHash object.
 * 
 * @throws INVALID_ARGUMENT_EXEPTION, KSI_EXEPTION.
 */
static void getHashFromCommandLine_throws(GT_CmdParameters *cmdparam,KSI_CTX *ksi, KSI_DataHash **hash){
    unsigned char *data;
    size_t len;
    int res = KSI_UNKNOWN_ERROR;
    int hasAlg = -1;

    try
        CODE{
            getBinaryFromHexString(ksi, cmdparam->inputHashStrn, &data, &len);
            hasAlg = getHashAlgorithm_throws(cmdparam->hashAlgName_F);
            KSI_DataHash_fromDigest_throws(ksi, hasAlg, data, len, hash);
            }
        CATCH_ALL{
            THROW_FORWARD_APPEND_MESSAGE("Error: Unable to get hash from command line input.\n");
            }
    end_try
    
    return;
    }

/**
 * Gives hash algorithm identifier by name.
 * 
 * @param[in] hashAlg Hash algorithm name.
 * @return Hash algorithm identifier.
 * 
 * @throws KSI_EXEPTION.
 */
static int getHashAlgorithm_throws(const char *hashAlg){
    int hasAlgID = KSI_getHashAlgorithmByName(hashAlg);
    if(hasAlgID == -1) THROW_MSG(KSI_EXEPTION, "Error: The hash algorithm \"%s\" is unknown\n", hashAlg);
    return hasAlgID;
    }
