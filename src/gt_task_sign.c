#include "gt_task.h"
#include "gt_cmdparameters.h"

#include <stdio.h>
#include <string.h>

#include <ksi.h>
#include <net_curl.h>




int GT_signTask(GT_CmdParameters *cmdparam, GT_Tasks task) {
	KSI_CTX *ksi = NULL;
	int res = KSI_UNKNOWN_ERROR;

	FILE *in = NULL;
	FILE *out = NULL;

	KSI_DataHasher *hsr = NULL;
	KSI_DataHash *hash = NULL;
	KSI_Signature *sign = NULL;

	unsigned char *raw = NULL;
	int raw_len;

	unsigned char buf[1024];
	int buf_len;

        /*initializing of KSI*/
        
	/** Global init of KSI */
	res = KSI_global_init();
	if (res != KSI_OK) goto cleanup;
        
	/* Create new KSI context for this thread. */
	res = KSI_CTX_new(&ksi);
	if (res != KSI_OK) {
		fprintf(stderr, "Unable to create context.\n");
		goto cleanup;
	}

	/* Check if uri's are specified. */
	if (cmdparam->S || cmdparam->P) {
		KSI_NetProvider *net = NULL;
		res = KSI_CurlNetProvider_new(ksi, &net);
		if (res != KSI_OK) {
			fprintf(stderr, "Unable to create new network provider.\n");
			goto cleanup;
		}

		/* Check aggregator url */
		if (cmdparam->S) {
			res = KSI_CurlNetProvider_setSignerUrl(net, cmdparam->signingService_url);
			if (res != KSI_OK) {
				fprintf(stderr, "Unable to set aggregator url %s.\n", cmdparam->signingService_url);
				goto cleanup;
			}
		}

		/* Check publications file url. */
		if (cmdparam->P) {
			res = KSI_CurlNetProvider_setPublicationUrl(net, cmdparam->publicationsFile_url);
			if (res != KSI_OK) {
				fprintf(stderr, "Unable to set publications file url %s.\n", cmdparam->publicationsFile_url);
				goto cleanup;
			}
		}

		/* Set the new network provider. */
		res = KSI_setNetworkProvider(ksi, net);
		if (res != KSI_OK) {
			fprintf(stderr, "Unable to set network provider.\n");
			res = KSI_UNKNOWN_ERROR;

			goto cleanup;
		}
	}

        /*Getting the hash for signing process*/
        
        if(GT_getTask() == signDataFile){
            /*Choosing of hash algorithm*/
            char *hashAlg = cmdparam->H ? (cmdparam->rawHashString) : ("default");
            res = KSI_DataHasher_open(ksi, KSI_getHashAlgorithmByName(hashAlg), &hsr);
            if (res != KSI_OK) {
		fprintf(stderr, "Unable to create hasher.\n");
		goto cleanup;
                }
            
            /* Open Input file */
            in = fopen(cmdparam->inDataFileName, "rb");
            if (in == NULL){
                fprintf(stderr, "Unable to open input file '%s'\n", cmdparam->inDataFileName);
                res = KSI_IO_ERROR;
                goto cleanup;
                }
            
            /* Read the input file and calculate the hash of its contents. */
            while (!feof(in)){
		buf_len = fread(buf, 1, sizeof(buf), in);

		/* Add  next block to the calculation. */
		res = KSI_DataHasher_add(hsr, buf, buf_len);
		if (res != KSI_OK) {
                    fprintf(stderr, "Unable to add data to hasher.\n");
                    goto cleanup;
                    }
            }

            /* Close the data hasher and retreive the data hash. */
            res = KSI_DataHasher_close(hsr, &hash);
            if (res != KSI_OK) {
		fprintf(stderr, "Unable to create hash.\n");
		goto cleanup;
            }
            }
        else if(GT_getTask() == signHash){
            printf("todo hash signing.\n");
            goto cleanup;
            }
        else
            {
            goto cleanup;
            }
        
	/* Sign the data hash. */
	res = KSI_createSignature(ksi, hash, &sign);
	if (res != KSI_OK) {
		fprintf(stderr, "Unable to sign %d.\n", res);
		KSI_ERR_statusDump(ksi, stderr);
		goto cleanup;
	}

	/* Serialize the signature. */
	res = KSI_Signature_serialize(sign, &raw, &raw_len);
	if (res != KSI_OK) {
		fprintf(stderr, "Unable to serialize signature.");
		goto cleanup;
	}
        
        /**************************SALVESTAMINE*****************************/
        
	/* Output file */
	out = fopen(cmdparam->outSigFileName, "wb");
	if (out == NULL) {
		fprintf(stderr, "Unable to open input file '%s'\n", cmdparam->outSigFileName);
		res = KSI_IO_ERROR;
		goto cleanup;
	}

	/* Write the signature file. */
	if (!fwrite(raw, 1, raw_len, out)) {
		fprintf(stderr, "Unable to write output file %s.\n", cmdparam->outSigFileName);
		res = KSI_IO_ERROR;
		goto cleanup;
	}

	/* Only print message when signature output is not stdout. */
	if (out != NULL) {
		printf("Signature saved.\n");
	}

	res = KSI_OK;

cleanup:

	if (in != NULL) fclose(in);
	if (out != NULL) fclose(out);

	KSI_Signature_free(sign);
	KSI_DataHash_free(hash);
	KSI_DataHasher_free(hsr);

	KSI_free(raw);
	KSI_CTX_free(ksi);

	/* Global cleanup */
	KSI_global_cleanup();

	return res;
}