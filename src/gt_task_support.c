#include <string.h>
#include <time.h>
#include <ksi/net_http.h>
#include <stdio.h>
#include "gt_task_support.h"

#ifdef _WIN32 
#   define snprintf _snprintf
#endif

#define ERROR_MESSAGE_SIZE 256
static char errorMessage[ERROR_MESSAGE_SIZE];

#define ERROR_HANDLING_SILENT(...) \
    if (res != KSI_OK){  \
        snprintf(errorMessage,ERROR_MESSAGE_SIZE,__VA_ARGS__);\
	goto cleanup; \
	}

const char *getLastSupportFunctionErrorMessage(void){
    return errorMessage;
    }

int configureNetworkProvider(KSI_CTX *ksi, GT_CmdParameters *cmdparam)
{
    int res = KSI_OK;
    KSI_NetworkClient *net = NULL;
    /* Check if uri's are specified. */
    if (cmdparam->S || cmdparam->P || cmdparam->X || cmdparam->C || cmdparam->c) {
        res = KSI_UNKNOWN_ERROR;
        res = KSI_HttpClient_new(ksi, &net);
        ERROR_HANDLING_SILENT("Unable to create new network provider.\n");

        /* Check aggregator url */
        if (cmdparam->S) {
            res = KSI_HttpClient_setSignerUrl(net, cmdparam->signingService_url);
            ERROR_HANDLING_SILENT("Unable to set aggregator url %s.\n", cmdparam->signingService_url);
        }

        /* Check publications file url. */
        if (cmdparam->P) {
            res = KSI_HttpClient_setPublicationUrl(net, cmdparam->publicationsFile_url);
            ERROR_HANDLING_SILENT("Unable to set publications file url %s.\n", cmdparam->publicationsFile_url);
        }

        /* Check extending/verification service url. */
        if (cmdparam->X) {
            res = KSI_HttpClient_setExtenderUrl(net, cmdparam->verificationService_url);
            ERROR_HANDLING_SILENT("Unable to set extender/verifier url %s.\n", cmdparam->verificationService_url);
        }


        /* Check Network connection timeout. */
        if (cmdparam->C) {
            res = KSI_HttpClient_setConnectTimeoutSeconds(net, cmdparam->networkConnectionTimeout);
            ERROR_HANDLING_SILENT("Unable to set network connection timeout %i.\n", cmdparam->networkConnectionTimeout);
        }

        /* Check Network transfer timeout. */
        if (cmdparam->c) {
            res = KSI_HttpClient_setReadTimeoutSeconds(net, cmdparam->networkTransferTimeout);
            ERROR_HANDLING_SILENT("Unable to set network transfer timeout %i.\n", cmdparam->networkTransferTimeout);
        }

        /* Set the new network provider. */
        res = KSI_setNetworkProvider(ksi, net);
        ERROR_HANDLING_SILENT("Unable to set network provider.\n");
    }

cleanup:
    return res;

}

int calculateHashOfAFile(KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash)
{
    FILE *in = NULL;
    int res = KSI_UNKNOWN_ERROR;
    unsigned char buf[1024];
    int buf_len;

    /* Open Input file */
    in = fopen(fname, "rb");
    if (in == NULL) {
        snprintf(errorMessage,ERROR_MESSAGE_SIZE,"Unable to open input file '%s'\n", fname);
        res = KSI_IO_ERROR;
        goto cleanup;
    }

    /* Read the input file and calculate the hash of its contents. */
    while (!feof(in)) {
        buf_len = fread(buf, 1, sizeof (buf), in);
        /* Add  next block to the calculation. */
        res = KSI_DataHasher_add(hsr, buf, buf_len);
        ERROR_HANDLING_SILENT("Unable to add data to hasher.\n");
    }

    /* Close the data hasher and retreive the data hash. */
    res = KSI_DataHasher_close(hsr, hash);
    ERROR_HANDLING_SILENT("Unable to create hash.\n");


cleanup:
    if (in != NULL) fclose(in);
    return res;

}

int saveSignatureFile(KSI_Signature *sign, const char *fname)
{
    int res = KSI_UNKNOWN_ERROR;
    unsigned char *raw = NULL;
    int raw_len = 0;
    int count = 0;
    FILE *out = NULL;
    /* Serialize the extended signature. */
    res = KSI_Signature_serialize(sign, &raw, &raw_len);
    ERROR_HANDLING_SILENT("Unable to serialize signature.\n");
    
    /* Open output file. */
    out = fopen(fname, "wb");
    if (out == NULL) {
        snprintf(errorMessage,ERROR_MESSAGE_SIZE,"Unable to open output file '%s'\n", fname);
        res = KSI_IO_ERROR;
        goto cleanup;
    }

    count = fwrite(raw, 1, raw_len, out);
    if (count != raw_len) {
        snprintf(errorMessage,ERROR_MESSAGE_SIZE,"Failed to write output file.\n");
        res = KSI_IO_ERROR;
        goto cleanup;
    }

cleanup:
    if (out != NULL) fclose(out);
    KSI_free(raw);
    return res;
}



static void printPublicationRecordReferences(KSI_PublicationRecord *publicationRecord)
{
    KSI_LIST(KSI_Utf8String) *list_publicationReferences = NULL;
    int res = KSI_UNKNOWN_ERROR;
    int j = 0;

    res = KSI_PublicationRecord_getPublicationRef(publicationRecord, &list_publicationReferences);

    for (j = 0; j < KSI_Utf8StringList_length(list_publicationReferences); j++) {
        KSI_Utf8String *reference = NULL;
        res = KSI_Utf8StringList_elementAt(list_publicationReferences, j, &reference);
        ERROR_HANDLING_SILENT("Unable to get publication ref.\n");
        printf("*  %s\n", KSI_Utf8String_cstr(reference));
    }
cleanup:


    //KSI_Utf8StringList_free(list_publicationReferences);
    return;
}

static void printfPublicationRecordTime(KSI_PublicationRecord *publicationRecord)
{
    KSI_PublicationData *publicationData = NULL;
    KSI_Integer *rawTime = NULL;
    time_t pubTime;
    struct tm *publicationTime = NULL;
    int res = KSI_UNKNOWN_ERROR;
    
    res = KSI_PublicationRecord_getPublishedData(publicationRecord, &publicationData);
    ERROR_HANDLING_SILENT("Unable to get pulication data\n");
    res = KSI_PublicationData_getTime(publicationData, &rawTime);
    if (res != KSI_OK || rawTime == NULL) {
        snprintf(errorMessage,ERROR_MESSAGE_SIZE,"Failed to get publication time\n");
        goto cleanup;
    }
    pubTime = (time_t) KSI_Integer_getUInt64(rawTime);
    publicationTime = gmtime(&pubTime);
    printf("[%d-%d-%d]\n", 1900 + publicationTime->tm_year, publicationTime->tm_mon + 1, publicationTime->tm_mday);

cleanup:
//    KSI_PublicationData_free(publicationData);
    //KSI_Integer_free(rawTime);
   // KSI_free(publicationTime);
    return;

}


//the bibliographic reference to a media outlet where the publication appeared
int printPublicationReferences(const KSI_PublicationsFile *pubFile)
{
    int res = KSI_UNKNOWN_ERROR;
    KSI_LIST(KSI_PublicationRecord)* list_publicationRecord = NULL;
    KSI_PublicationRecord *publicationRecord = NULL;
    int i;

    res = KSI_PublicationsFile_getPublications(pubFile, &list_publicationRecord);
    ERROR_HANDLING_SILENT("Unable to get publications records.\n");

    for (i = 0; i < KSI_PublicationRecordList_length(list_publicationRecord); i++) {


        res = KSI_PublicationRecordList_elementAt(list_publicationRecord, i, &publicationRecord);
        ERROR_HANDLING_SILENT("Failed to get publications record object.\n");

        printfPublicationRecordTime(publicationRecord);
        printPublicationRecordReferences(publicationRecord);
    }

cleanup:
    //KSI_PublicationRecordList_free(list_publicationRecord);
   // KSI_PublicationRecord_free(publicationRecord);
    return res;
}

int printSignaturePublicationReference(const KSI_Signature *sig)
{
    int res = KSI_UNKNOWN_ERROR;
    KSI_PublicationRecord *publicationRecord;

    res = KSI_Signature_getPublicationRecord(sig, &publicationRecord);
    ERROR_HANDLING_SILENT("Failed to get publications reference list from publication record object.\n");

    if (publicationRecord == NULL) {
        snprintf(errorMessage,ERROR_MESSAGE_SIZE,"No publication Record avilable.\n");
        res = KSI_UNKNOWN_ERROR;
        goto cleanup;
    }

    printfPublicationRecordTime(publicationRecord);
    printPublicationRecordReferences(publicationRecord);
    
cleanup:
    //KSI_PublicationRecord_free(publicationRecord);
    return res;
}

int printSignerIdentity(KSI_Signature *sign)
{
    int res = KSI_UNKNOWN_ERROR;
    char *signerIdentity = NULL;

    res = KSI_Signature_getSignerIdentity(sign, &signerIdentity);
    ERROR_HANDLING_SILENT("Unable to read signer identity.\n");
    printf("Signer identity: '%s'\n", signerIdentity == NULL ? "Unknown" : signerIdentity);

cleanup:
    KSI_free(signerIdentity);
    return res;

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