#ifndef GT_TASK_H
#define	GT_TASK_H

#include "gt_cmdparameters.h"
#include <ksi.h>
#include <net_curl.h>

#ifdef	__cplusplus
extern "C" {
#endif

int configureNetworkProvider(GT_CmdParameters *cmdparam, KSI_CTX *ksi);
int calculateHashOfAFile(KSI_DataHasher *hsr, KSI_DataHash **hash ,const char *fname);  
int saveSignatureFile(KSI_Signature *sign, const char *fname);

int GT_signTask(GT_CmdParameters *cmdparam, GT_Tasks task);



#define ERROR_HANDLING(...) \
    if (res != KSI_OK){  \
	fprintf(stderr, __VA_ARGS__); \
	goto cleanup; \
	}


#define ERROR_HANDLING_STATUS_DUMP(...) \
    if (res != KSI_OK){  \
	fprintf(stderr, __VA_ARGS__); \
        KSI_ERR_statusDump(ksi, stderr); \
	goto cleanup; \
	}


#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

