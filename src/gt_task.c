#include "gt_task.h"
#include <net_curl.h>
#include <string.h>
#include <ctype.h>

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
    int raw_len=0;
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

// helpers for hex decoding
static int x(char c)
{
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

int getBinaryFromHexString(KSI_CTX *ksi, const char *hexin, unsigned char **binout, size_t *lenout){
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


int getHashFromCommandLine(GT_CmdParameters *cmdparam,KSI_CTX *ksi, KSI_DataHash **hash){
    unsigned char *data;
    size_t len;
    int res = KSI_UNKNOWN_ERROR;
    int hasAlg = -1;
   // KSI_DataHash *temp_hash;
    
    res = getBinaryFromHexString(ksi, cmdparam->inputHashStrn, &data, &len);
    ERROR_HANDLING("Unable to extract Hash value from command line input.\n");
    
    hasAlg = KSI_getHashAlgorithmByName (cmdparam->hashAlgName_F);
    if(hasAlg == -1){
        fprintf(stderr, "The hash algorithm \"%s\"is unknown\n", cmdparam->hashAlgName_F);
        res = KSI_INVALID_ARGUMENT;
        goto cleanup;
        }
    
    res = KSI_DataHash_fromDigest(ksi, hasAlg, data, len, hash);
    ERROR_HANDLING("Unable to create hash from digest.\n");
    cleanup:
    
    return res;
    }


/*
// parse specified hash value, formatted like ALG:hexencodedhash
int parse_digest(char *arg, KSI_DataHash **hash) {
	int res = KSI_UNKNOWN_ERROR;

        

        int *rawAlgLen;
        
        int *rawHasLen;
        
	int alg_id = KSI_HASHALG_SHA2_256 ;
	const EVP_MD *evp_md;
	int len;
	KSI_DataHash *tmp_data_hash = NULL;
	unsigned char* tmp_hash = NULL;
	size_t tmp_length;
	
	colon = strchr(arg, ':');
        if (colon == NULL) {
            alg_id = KSI_HASHALG_SHA2_256 ;
            pos = arg;
            }
        else{
            
            }
        
        
        
	} else {  // separator : found
		if (defaultalg != GT_HASHALG_DEFAULT)
			fprintf(stderr, "Warning, ignoring -H <hashalgorithm> as hash value is prefixed with algorithm name.\n");
		pos[0] = '\0';			// is modifying optarg safe?
		pos++;
		evp_md = EVP_get_digestbyname(arg);
		if (evp_md == NULL) {
			fprintf(stderr, "Invalid hash algorithm name %s.\n", arg);
			goto e;
		}
		alg_id = GT_EVPToHashChainID(evp_md);
		if (alg_id < 0) {
			fprintf(stderr, "Untrusted hash algorithm %s.\n", arg);
			goto e;
		}
	}
	
	tmp_data_hash = (GTDataHash *) GT_malloc(sizeof(GTDataHash));
	if (tmp_data_hash == NULL) {
		res = GT_OUT_OF_MEMORY;
		goto e;
	}
	tmp_data_hash->digest = NULL;
	tmp_data_hash->context = NULL;
	tmp_length = EVP_MD_size(evp_md);
	tmp_hash = (unsigned char *) GT_malloc(tmp_length);
	if (tmp_hash == NULL) {
		res = GT_OUT_OF_MEMORY;
		goto e;
	}
	
	len = strlen(pos);
	if (len != tmp_length*2) {
		fprintf(stderr, "Invalid hash value length, must be %lu characters.\n", tmp_length*2);
		goto e;
	}
	{
		int i, j;
		for (i = 1, j = 0; i < len; i += 2, j++) {
			int ch = xx((pos[i - 1]), (pos[i]));
			if (ch == -1) {
				fprintf(stderr, "Invalid hexadecimal character.\n");
				goto e;
			}
			tmp_hash[j] = ch;
		}
	}
	tmp_data_hash->digest = tmp_hash;
	tmp_hash = NULL;
	tmp_data_hash->digest_length = tmp_length;
	tmp_data_hash->algorithm = alg_id;
	*data_hash = tmp_data_hash;
	tmp_data_hash = NULL;
	
	res = GT_OK;
	
e:
	if (res == GT_OUT_OF_MEMORY)
		fprintf(stderr, "%s\n", GT_getErrorString(res));
	GT_free(tmp_hash);
	GTDataHash_free(tmp_data_hash);
	return res;
}
        
        
        }
*/