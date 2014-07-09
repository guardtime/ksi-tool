#ifndef GT_TASK_H
#define	GT_TASK_H

#include <stdio.h>
#include <string.h>
#include <ksi\ksi.h>
#include "gt_cmdparameters.h"
#include <time.h>



#ifdef	__cplusplus
extern "C" {
#endif

    
/**
 * Configures NetworkProvider using info from commandline.
 * Sets urls and timeouts.
 * @param[in] cmdparam pointer to command-line parameters.
 * @param[in] ksi pointer to KSI context.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code). 
 */
int configureNetworkProvider(GT_CmdParameters *cmdparam, KSI_CTX *ksi);

/**
 * Calculates the hash of an input file.
 * @param[in] hsr Pointer to hasher object. 
 * @param[in] fname Pointer to file path.
 * @param[out] hash Pointer to the receiving pointer to the KSI hash object.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int calculateHashOfAFile(KSI_DataHasher *hsr, KSI_DataHash **hash ,const char *fname); 

/**
 * Saves signature object to file.
 * @param [in] sign Pointer to signature object for saving.
 * @param [in] fname Pointer to file path.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int saveSignatureFile(KSI_Signature *sign, const char *fname);

/**
 * Task that deals with signing operations.
 * @param [in] cmdparam Pointer to command-line parameters.
 * @return Ptatus code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int GT_signTask(GT_CmdParameters *cmdparam, GT_Tasks task);

/**
 * Task that deals with verifying operations.
 * @param [in] cmdparam Pointer to command-line parameters.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int GT_verifyTask(GT_CmdParameters *cmdparam, GT_Tasks task);

/**
 * Task that deals with extending operations.
 * @param [in] cmdparam Pointer to command-line parameters
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int GT_extendTask(GT_CmdParameters *cmdparam, GT_Tasks task);

/**
 * Task that deals with extending operations.
 * @param [in] cmdparam Pointer to command-line parameters.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int GT_getPublicationsFileTask(GT_CmdParameters *cmdparam, GT_Tasks task);

/**
 * Prints the signer identity.
 * @param[in] sign Pointer to KSI signture object.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int printSignerIdentity(KSI_Signature *sign);

/**
 * Prints publications file publications references.
 *  [date]
 *   list of references
 * 
 * @param[in] pubFile Pointer to KSI publications file object.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int printPublicationReferences(const KSI_PublicationsFile *pubFile);

/**
 * Prints signatures publication references.
 * @param[in] sig Pointer to KSI signture object.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int printSignaturePublicationReference(const KSI_Signature *sig);


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

#define MEASURE_TIME(code_here, process_name, enable) \
    {   \
    clock_t start, finish; \
    unsigned int elapsed_time; \
    start = clock(); \
    code_here; \
    finish = clock(); \
    elapsed_time = 1000*(finish - start) / CLOCKS_PER_SEC; \
    if(enable == true) \
    printf("%s took %i ms.\n",process_name, elapsed_time); \
    }

#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

