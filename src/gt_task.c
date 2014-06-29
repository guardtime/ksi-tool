#include "gt_task.h"
#include "gt_cmdparameters.h"

#include <stdio.h>
#include <string.h>

#include <ksi.h>
#include <net_curl.h>


int SetUris(GT_CmdParameters *cmdparam, GT_Tasks task, KSI_CTX *ksi){
    int res;
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
    cleanup:
    return res;

}