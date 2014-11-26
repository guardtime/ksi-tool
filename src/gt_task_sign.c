#include <ctype.h>
#include "gt_task_support.h"
#include "try-catch.h"

static void getHashFromCommandLine_throws(const char *imprint,KSI_CTX *ksi, KSI_DataHash **hash);
static int getHashAlgorithm_throws(const char *hashAlg);
static void printSignaturesRootHash_and_Time(const KSI_Signature *sig);

int GT_signTask(Task *task) {
	KSI_CTX *ksi = NULL;
	KSI_DataHasher *hsr = NULL;
	KSI_DataHash *hash = NULL;
	KSI_Signature *sign = NULL;
	int retval = EXIT_SUCCESS;

	bool H, t, n, d;
	char *hashAlgName_H = NULL;
	char *inDataFileName = NULL;
	char *outSigFileName = NULL;
	char *imprint = NULL;
	
	H = paramSet_getStrValueByNameAt(task->set, "H", 0,&hashAlgName_H);	
	paramSet_getStrValueByNameAt(task->set, "f", 0,&inDataFileName);
	paramSet_getStrValueByNameAt(task->set, "o", 0,&outSigFileName);
	paramSet_getStrValueByNameAt(task->set, "F", 0,&imprint);
	n = paramSet_isSetByName(task->set, "n");
	t = paramSet_isSetByName(task->set, "t");
	d = paramSet_isSetByName(task->set, "d");
	
	resetExceptionHandler();
	try
		CODE{
			/*Initalization of KSI */
			initTask_throws(task, &ksi);
			/*Getting the hash for signing process*/
			if(task->id == signDataFile){
				char *hashAlg;
				int hasAlgID=-1;
				/*Choosing of hash algorithm and creation of data hasher*/
				printf("Getting hash from file for signing process...");
				hashAlg = H ? (hashAlgName_H) : ("default");
				hasAlgID = getHashAlgorithm_throws(hashAlg);
				KSI_DataHasher_open_throws(ksi,hasAlgID , &hsr);
				getFilesHash_throws(hsr, inDataFileName, &hash );
				printf("ok.\n");
			}
			else if(task->id == signHash){
				printf("Getting hash from input string for signing process...");
				getHashFromCommandLine_throws(imprint,ksi, &hash);
				printf("ok.\n");
			}

			/* Sign the data hash. */
			printf("Creating signature from hash...");
			MEASURE_TIME(KSI_createSignature_throws(ksi, hash, &sign));
			printf("ok. %s\n",t ? str_measuredTime() : "");

			/* Save signature file */
			saveSignatureFile_throws(sign, outSigFileName);
			printf("Signature saved.\n");
		}
		CATCH_ALL{
			printf("failed.\n");
			printErrorMessage();
			retval = _EXP.exep.ret;
			exceptionSolved();
		}
	end_try

	if(n || d) printf("\n");
	if (n) printSignerIdentity(sign);
	
				
	KSI_Signature_free(sign);
	KSI_DataHash_free(hash);
	KSI_DataHasher_free(hsr);
	KSI_CTX_free(ksi);
	
	return retval;
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

static int xx(char c1, char c2){
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
 * @throws INVALID_ARGUMENT_EXCEPTION, OUT_OF_MEMORY_EXCEPTION.
 */
static void getBinaryFromHexString(KSI_CTX *ksi, const char *hexin, unsigned char **binout, size_t *lenout){
	size_t len = strlen(hexin);
	unsigned char *tmp=NULL;
	size_t arraySize = len/2;
	int i,j;

	if(hexin == NULL || binout == NULL || lenout == NULL)
		THROW(INVALID_ARGUMENT_EXCEPTION,EXIT_FAILURE);
	
	if(len%2 != 0){
		THROW_MSG(INVALID_ARGUMENT_EXCEPTION,EXIT_INVALID_FORMAT, "Error: The hash length is not even number!\n");
	}

	tmp = KSI_calloc(arraySize, sizeof(unsigned char));
	if(tmp == NULL){
		THROW_MSG(OUT_OF_MEMORY_EXCEPTION,EXIT_OUT_OF_MEMORY, "Error: Unable to get memory for parsing hex to binary.\n");
	}

	for(i=0,j=0; i<arraySize; i++, j+=2){
		int res = xx(hexin[j], hexin[j+1]);
		if(res == -1){
			KSI_free(tmp);
			tmp = NULL;
			THROW_MSG(INVALID_ARGUMENT_EXCEPTION,EXIT_INVALID_FORMAT, "Error: The hex number is invalid: %c%c!\n", hexin[j], hexin[j+1]);
		}
		tmp[i] = res;
		//printf("%c%c -> %i\n", hexin[j], hexin[j+1], tempBin[i]);
	}

	*lenout = arraySize;
	*binout = tmp;

	return;
}

static bool getHashAndAlgStrings(const char *instrn, char **strnAlgName, char **strnHash){
	char *colon = NULL;
	size_t algLen = 0;
	size_t hahsLen = 0;
	char *temp_strnAlg = NULL;
	char *temp_strnHash = NULL;
	
	
	if(strnAlgName == NULL || strnHash == NULL) return false;
	*strnAlgName = NULL;
	*strnHash = NULL;
	if (instrn == NULL) return false;

	colon = strchr(instrn, ':');
	if (colon != NULL) {
		algLen = (colon - instrn) / sizeof (char);
		hahsLen = strlen(instrn) - algLen - 1;
		temp_strnAlg = calloc(algLen + 1, sizeof (char));
		temp_strnHash = calloc(hahsLen + 1, sizeof (char));
		memcpy(temp_strnAlg, instrn, algLen);
		temp_strnAlg[algLen] = 0;
		memcpy(temp_strnHash, colon + 1, hahsLen);
		temp_strnHash[hahsLen] = 0;
	}

	*strnAlgName = temp_strnAlg;
	*strnHash = temp_strnHash;
	//printf("Alg %s\nHash %s\n", *strnAlgName, *strnHash);
	return true;
}

/**
 * Reads hash from command line and creates the KSI_DataHash object.
 * 
 * @param[in] cmdparam Pointer to command line data object.
 * @param[in] ksi Pointer to ksi context object.
 * @param[out] hash Pointer to receiving pointer to KSI_DataHash object.
 * 
 * @throws KSI_EXCEPTION, INVALID_ARGUMENT_EXCEPTION.
 */
static void getHashFromCommandLine_throws(const char *imprint,KSI_CTX *ksi, KSI_DataHash **hash){
	unsigned char *data = NULL;
	size_t len;
	int res = KSI_UNKNOWN_ERROR;
	int hasAlg = -1;

	char *strAlg = NULL;
	char *strHash = NULL;
	try
		CODE{
			if(imprint == NULL) THROW_MSG(INVALID_ARGUMENT_EXCEPTION,EXIT_INVALID_CL_PARAMETERS, "");
			getHashAndAlgStrings(imprint, &strAlg, &strHash);
			if(strAlg == NULL || strHash== NULL ) THROW_MSG(INVALID_ARGUMENT_EXCEPTION,EXIT_INVALID_CL_PARAMETERS, "");
			
			getBinaryFromHexString(ksi, strHash, &data, &len);
			hasAlg = getHashAlgorithm_throws(strAlg);
			KSI_DataHash_fromDigest_throws(ksi, hasAlg, data, (unsigned int)len, hash);
		}
		CATCH_ALL{
			free(strAlg);
			free(strHash);
			free(data);
			THROW_FORWARD_APPEND_MESSAGE("Error: Unable to get hash from command-line.\n");
		}
	end_try

	free(strAlg);
	free(strHash);
	free(data);
	return;
}

/**
 * Gives hash algorithm identifier by name.
 * 
 * @param[in] hashAlg Hash algorithm name.
 * @return Hash algorithm identifier.
 * 
 * @throws KSI_EXCEPTION.
 */
static int getHashAlgorithm_throws(const char *hashAlg){
	int hasAlgID = KSI_getHashAlgorithmByName(hashAlg);
	if(hasAlgID == -1) THROW_MSG(KSI_EXCEPTION, EXIT_CRYPTO_ERROR, "Error: The hash algorithm \"%s\" is unknown\n", hashAlg);
	return hasAlgID;
}
