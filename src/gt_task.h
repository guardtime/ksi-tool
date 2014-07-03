#ifndef GT_TASK_H
#define	GT_TASK_H

#include <stdio.h>
#include <string.h>
#include <ksi\ksi.h>
#include "gt_cmdparameters.h"



#ifdef	__cplusplus
extern "C" {
#endif

    
/**
 * Configures NetworkProvider using info from commandline.
 * Sets urls and timeouts.
 * @param [in] cmdparam - pointer to command-line parameters
 * @param [in] ksi - pointer to KSI context.
 * @return status code (KSI_OK, when operation succeeded, otherwise an error code). 
 */
int configureNetworkProvider(GT_CmdParameters *cmdparam, KSI_CTX *ksi);

/**
 * Calculates the hash of an input file.
 * @param [in] hsr pointer to hasher object 
 * @param [in] fname pointer to file path.
 * @param hash [obj] hash object generated from file.
 * @return status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int calculateHashOfAFile(KSI_DataHasher *hsr, KSI_DataHash **hash ,const char *fname); 

/**
 * Saves signature object to file.
 * @param [in] sign pointer to signature object for saving.
 * @param [in] fname pointer to file path.
 * @return status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int saveSignatureFile(KSI_Signature *sign, const char *fname);

/**
 * Task that deals with signing operations.
 * @param [in] cmdparam - pointer to command-line parameters
 * @return status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int GT_signTask(GT_CmdParameters *cmdparam, GT_Tasks task);

/**
 * Task that deals with verifying operations.
 * @param [in] cmdparam - pointer to command-line parameters
 * @return status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int GT_verifyTask(GT_CmdParameters *cmdparam, GT_Tasks task);

/**
 * Task that deals with extending operations.
 * @param [in] cmdparam - pointer to command-line parameters
 * @return status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int GT_extendTask(GT_CmdParameters *cmdparam, GT_Tasks task);

/**
 * Task that deals with extending operations.
 * @param [in] cmdparam - pointer to command-line parameters
 * @return status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int GT_getPublicationsFileTask(GT_CmdParameters *cmdparam, GT_Tasks task);


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

