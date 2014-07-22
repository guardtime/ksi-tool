#include <string.h>
#include <time.h>
#include <ksi/net_http.h>
#include <stdio.h>
#include "gt_task_support.h"
#include "try-catch.h"


#define ON_ERROR_THROW_MSG(_exeption, ...) \
    if (res != KSI_OK){  \
        THROW_MSG(_exeption,__VA_ARGS__); \
	}

/**
 * Configures NetworkProvider using info from commandline.
 * Sets urls and timeouts.
 * @param[in] cmdparam pointer to command-line parameters.
 * @param[in] ksi pointer to KSI context.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 * 
 * @Throws KSI_EXEPTION
 */
static int configureNetworkProvider_throws(KSI_CTX *ksi, GT_CmdParameters *cmdparam)
{
    int res = KSI_OK;
    KSI_NetworkClient *net = NULL;
    /* Check if uri's are specified. */
    if (cmdparam->S || cmdparam->P || cmdparam->X || cmdparam->C || cmdparam->c) {
        res = KSI_UNKNOWN_ERROR;
        res = KSI_HttpClient_new(ksi, &net);
        ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to configure network provider.\nError: Unable to create new network provider.\n");

        /* Check aggregator url */
        if (cmdparam->S) {
            res = KSI_HttpClient_setSignerUrl(net, cmdparam->signingService_url);
            ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to configure network provider.\nError: Unable to set aggregator url '%s'.\n", cmdparam->signingService_url);
            }

        /* Check publications file url. */
        if (cmdparam->P) {
            res = KSI_HttpClient_setPublicationUrl(net, cmdparam->publicationsFile_url);
            ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to configure network provider.\nError: Unable to set publications file url '%s'.\n", cmdparam->publicationsFile_url);
            }

        /* Check extending/verification service url. */
        if (cmdparam->X) {
            res = KSI_HttpClient_setExtenderUrl(net, cmdparam->verificationService_url);
            ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to configure network provider.\nError: Unable to set extender/verifier url '%s'.\n", cmdparam->verificationService_url);
            }

        /* Check Network connection timeout. */
        if (cmdparam->C) {
            res = KSI_HttpClient_setConnectTimeoutSeconds(net, cmdparam->networkConnectionTimeout);
            ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to configure network provider.\nError: Unable to set network connection timeout %i.\n", cmdparam->networkConnectionTimeout);
            }

        /* Check Network transfer timeout. */
        if (cmdparam->c) {
            res = KSI_HttpClient_setReadTimeoutSeconds(net, cmdparam->networkTransferTimeout);
            ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to configure network provider.\nError: Unable to set network transfer timeout %i.\n", cmdparam->networkTransferTimeout);
            }

        /* Set the new network provider. */
        res = KSI_setNetworkProvider(ksi, net);
        ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to configure network provider.\nError: Unable to set network provider.\n");
    }

    return res;

}

void InitTask_throws(GT_CmdParameters *cmdparam ,KSI_CTX **ksi){
    int res = KSI_UNKNOWN_ERROR;
    res = KSI_global_init();
    ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to init KSI global resources.\n");
    res = KSI_CTX_new(ksi);
    ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to init KSI context.\n");
    
    try
        CODE{
            res = configureNetworkProvider_throws(*ksi, cmdparam);
            }
        CATCH(KSI_EXEPTION){
            THROW_FORWARD();
            }
    end_try
    
    return;
}

void getFilesHash_throws(KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash)
{
    FILE *in = NULL;
    int res = KSI_UNKNOWN_ERROR;
    unsigned char buf[1024];
    int buf_len;

    /* Open Input file */
    in = fopen(fname, "rb");
    if (in == NULL) 
        THROW_MSG(IO_EXEPTION, "Error: Unable to hash file '%s'.\nError: Unable to open input file '%s'\n", fname,fname);
        
    /* Read the input file and calculate the hash of its contents. */
    while (!feof(in)) {
        buf_len = fread(buf, 1, sizeof (buf), in);
        /* Add  next block to the calculation. */
        res = KSI_DataHasher_add(hsr, buf, buf_len);
        if(res != KSI_OK){
            fclose(in);
            ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to hash file '%s'.\nError: Unable to add data to hasher.\n",fname);
            }
    }
    if (in != NULL) fclose(in);
    /* Close the data hasher and retreive the data hash. */
    res = KSI_DataHasher_close(hsr, hash);
    ON_ERROR_THROW_MSG(KSI_EXEPTION, "Error: Unable to hash file '%s'.\nError: Unable to create hash.\n",fname);

    return;
}

void saveSignatureFile_throws(KSI_Signature *sign, const char *fname)
{
    int res = KSI_UNKNOWN_ERROR;
    unsigned char *raw = NULL;
    int raw_len = 0;
    int count = 0;
    FILE *out = NULL;
    
    /* Serialize the extended signature. */
    res = KSI_Signature_serialize(sign, &raw, &raw_len);
    ON_ERROR_THROW_MSG(KSI_EXEPTION,"Error: Unable to save signature '%s'.\n Error: Unable to serialize signature.\n",fname);
    
    /* Open output file. */
    out = fopen(fname, "wb");
    if (out == NULL) {
        KSI_free(raw);
        THROW_MSG(IO_EXEPTION, "Error: Unable to save signature '%s'.\n Error: Unable to open output file '%s'\n", fname,fname);
        }

    count = fwrite(raw, 1, raw_len, out);
    if (count != raw_len) {
        fclose(out);
        KSI_free(raw);
        THROW_MSG(KSI_EXEPTION, "Error: Unable to save signature '%s'.\n Error: Failed to write output file.\n",fname);
        }

    fclose(out);
    KSI_free(raw);
    return;
}

/**
 * Print publication record references.
 * 
 * @param[in] publicationRecord Pointer to KSI_PublicationRecord object.
 * 
 * @throws KSI_EXEPTION.
 */
static void printPublicationRecordReferences_throws(KSI_PublicationRecord *publicationRecord)
{
    KSI_LIST(KSI_Utf8String) *list_publicationReferences = NULL;
    int res = KSI_UNKNOWN_ERROR;
    int j = 0;

    res = KSI_PublicationRecord_getPublicationRef(publicationRecord, &list_publicationReferences);

    for (j = 0; j < KSI_Utf8StringList_length(list_publicationReferences); j++) {
        KSI_Utf8String *reference = NULL;
        res = KSI_Utf8StringList_elementAt(list_publicationReferences, j, &reference);
        ON_ERROR_THROW_MSG(KSI_EXEPTION,"Error: Unable to print publication record reference.\nError: Unable to get publication ref.\n");
        printf("*  %s\n", KSI_Utf8String_cstr(reference));
    }
    return;
}

/**
 * Print publication record time as [yy-mm-dd].
 * 
 * @param[in] publicationRecord Pointer to KSI_PublicationRecord object.
 * 
 * @throws KSI_EXEPTION.
 */
static void printfPublicationRecordTime_throws(KSI_PublicationRecord *publicationRecord)
{
    KSI_PublicationData *publicationData = NULL;
    KSI_Integer *rawTime = NULL;
    time_t pubTime;
    struct tm *publicationTime = NULL;
    int res = KSI_UNKNOWN_ERROR;
    
    res = KSI_PublicationRecord_getPublishedData(publicationRecord, &publicationData);
    ON_ERROR_THROW_MSG(KSI_EXEPTION,"Error: Unable print publication record time.\nError: Unable to get pulication data\n");
    
    res = KSI_PublicationData_getTime(publicationData, &rawTime);
    if (res != KSI_OK || rawTime == NULL) {
        THROW_MSG(KSI_EXEPTION, "Error: Unable print publication record time.\nError: Failed to get publication time\n");
    }
    pubTime = (time_t) KSI_Integer_getUInt64(rawTime);
    publicationTime = gmtime(&pubTime);
    printf("[%d-%d-%d]\n", 1900 + publicationTime->tm_year, publicationTime->tm_mon + 1, publicationTime->tm_mday);

    return;
}



void printPublicationReferences_throws(const KSI_PublicationsFile *pubFile)
{
    int res = KSI_UNKNOWN_ERROR;
    KSI_LIST(KSI_PublicationRecord)* list_publicationRecord = NULL;
    KSI_PublicationRecord *publicationRecord = NULL;
    int i;

    res = KSI_PublicationsFile_getPublications(pubFile, &list_publicationRecord);
    ON_ERROR_THROW_MSG(KSI_EXEPTION,"Error: Unable to print references. \nError: Unable to get publications records.\n");

    for (i = 0; i < KSI_PublicationRecordList_length(list_publicationRecord); i++) {
        res = KSI_PublicationRecordList_elementAt(list_publicationRecord, i, &publicationRecord);
        ON_ERROR_THROW_MSG(KSI_EXEPTION,"Error: Unable to print references. \nError: Unable to get publications record object.\n");
        
        try
           CODE{
                printfPublicationRecordTime_throws(publicationRecord);
                printPublicationRecordReferences_throws(publicationRecord);
                }
            CATCH_ALL{
                THROW_FORWARD();
                }
        end_try
        
    }
    return;
}

void printSignaturePublicationReference_throws(const KSI_Signature *sig)
{
    int res = KSI_UNKNOWN_ERROR;
    KSI_PublicationRecord *publicationRecord;

    res = KSI_Signature_getPublicationRecord(sig, &publicationRecord);
    ON_ERROR_THROW_MSG(KSI_EXEPTION,"Error: Unable to print signatures publication reference.\nError: Unable to get publications reference list from publication record object.\n");

    if (publicationRecord == NULL) {
        THROW_MSG(KSI_EXEPTION, "Error: Unable to print signatures publication reference.\nError: No publication Record avilable.\n");
    }
    
    try
       CODE{
            printfPublicationRecordTime_throws(publicationRecord);
            printPublicationRecordReferences_throws(publicationRecord);
            }
        CATCH_ALL{
            THROW_FORWARD();
            }
    end_try
    
    return;
}

void printSignerIdentity_throws(KSI_Signature *sign)
{
    int res = KSI_UNKNOWN_ERROR;
    char *signerIdentity = NULL;

    res = KSI_Signature_getSignerIdentity(sign, &signerIdentity);
    if(res != KSI_OK){
        KSI_free(signerIdentity);
        ON_ERROR_THROW_MSG(KSI_EXEPTION,"Error: Unable to print signer identity.\nError: Unable to read signer identity.\n");
        }
    printf("Signer identity: '%s'\n", signerIdentity == NULL ? "Unknown" : signerIdentity);

    KSI_free(signerIdentity);
    return;
}


static unsigned int elapsed_time_ms;

unsigned int measureLastCall(void){
    static clock_t thisCall = 0;
    static clock_t lastCall = 0;

    thisCall = clock();
    elapsed_time_ms = 1000*(thisCall - lastCall) / CLOCKS_PER_SEC;
    lastCall = thisCall;
    return elapsed_time_ms;
}

unsigned int measuredTime(void){
    return elapsed_time_ms;
    }

char* str_measuredTime(void){
    static char buf[32];
    snprintf(buf,32,"(%i ms)", elapsed_time_ms);
    return buf;
    }



#define THROWABLE(func, ...) \
    int res = KSI_UNKNOWN_ERROR; \
    res = func;  \
    if(res != KSI_OK) {THROW_MSG(KSI_EXEPTION, __VA_ARGS__);} \
    return res;
    
int KSI_receivePublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile **publicationsFile){
    THROWABLE(KSI_receivePublicationsFile(ksi, publicationsFile), "Error: Unable to read publications file. (%s)\n", KSI_getErrorString(res));
    }

int KSI_verifyPublicationsFile_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile){
    THROWABLE(KSI_verifyPublicationsFile(ksi, publicationsFile), "Error: Unable to verify publications file. (%s)\n", KSI_getErrorString(res));
    }

int KSI_PublicationsFile_serialize_throws(KSI_CTX *ksi, KSI_PublicationsFile *publicationsFile, char **raw, int *raw_len){
    THROWABLE(KSI_PublicationsFile_serialize(ksi, publicationsFile, raw, raw_len), "Error: Unable serialize publications file. (%s)\n", KSI_getErrorString(res));
}

int KSI_DataHasher_open_throws(KSI_CTX *ksi,int hasAlgID ,KSI_DataHasher **hsr){
    THROWABLE(KSI_DataHasher_open(ksi, hasAlgID, hsr), "Error: Unable to create hasher. (%s)\n", KSI_getErrorString(res));
}

int KSI_createSignature_throws(KSI_CTX *ksi, const KSI_DataHash *hash, KSI_Signature **sign){
     THROWABLE(KSI_createSignature(ksi, hash, sign), "Error: Unable to sign. (%s)\n", KSI_getErrorString(res));
    }

int KSI_DataHash_fromDigest_throws(KSI_CTX *ksi, int hasAlg, char *data, unsigned int len, KSI_DataHash **hash){
    THROWABLE(KSI_DataHash_fromDigest(ksi, hasAlg, data, len, hash), "Error: Unable to create hash from digest. (%s)\n", KSI_getErrorString(res));
    }

int KSI_PublicationsFile_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_PublicationsFile **pubFile){
    THROWABLE(KSI_PublicationsFile_fromFile(ksi, fileName, pubFile), "Error: Unable to read publications file '%s'.\n", fileName)
    }

int KSI_Signature_fromFile_throws(KSI_CTX *ksi, const char *fileName, KSI_Signature **sig){
    THROWABLE(KSI_Signature_fromFile(ksi, fileName, sig), "Error: Unable to read signature from file. (%s)\n", KSI_getErrorString(res));
}

int KSI_Signature_verify_throws(KSI_Signature *sig, KSI_CTX *ksi){
    THROWABLE(KSI_Signature_verify(sig, ksi), "Error: Unable verify signature. (%s)\n", KSI_getErrorString(res));
    }

int KSI_Signature_createDataHasher_throws(KSI_Signature *sig, KSI_DataHasher **hsr){
    THROWABLE(KSI_Signature_createDataHasher(sig, hsr), "Error: Unable to create data hasher. (%s)\n", KSI_getErrorString(res));
    }

int KSI_Signature_verifyDataHash_throws(KSI_Signature *sig, KSI_DataHash *hash){
    THROWABLE(KSI_Signature_verifyDataHash(sig, hash), "Error: Wrong document or signature. (%s)\n", KSI_getErrorString(res));
}

int KSI_extendSignature_throws(KSI_CTX *ksi, KSI_Signature *sig, KSI_Signature **ext){
    THROWABLE(KSI_extendSignature(ksi, sig, ext),"Error: Unable to extend signature. (%s)\n", KSI_getErrorString(res));
}


