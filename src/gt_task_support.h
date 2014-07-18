#ifndef GT_TASK_H
#define	GT_TASK_H

#include <stdio.h>
#include <string.h>
#include <ksi/ksi.h>
#include "gt_cmd_parameters.h"




#ifdef	__cplusplus
extern "C" {
#endif

    
const char *getLastSupportFunctionErrorMessage(void);
    
/**
 * Configures NetworkProvider using info from commandline.
 * Sets urls and timeouts.
 * @param[in] cmdparam pointer to command-line parameters.
 * @param[in] ksi pointer to KSI context.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code). 
 */
int configureNetworkProvider(KSI_CTX *ksi, GT_CmdParameters *cmdparam);

/**
 * Calculates the hash of an input file.
 * @param[in] hsr Pointer to hasher object. 
 * @param[in] fname Pointer to file path.
 * @param[out] hash Pointer to the receiving pointer to the KSI hash object.
 * @return Status code (KSI_OK, when operation succeeded, otherwise an error code).
 */
int calculateHashOfAFile(KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash ); 

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
 * @return True if successful, false otherwise.
 */
bool GT_signTask(GT_CmdParameters *cmdparam);

/**
 * Task that deals with verifying operations.
 * @param [in] cmdparam Pointer to command-line parameters.
 * @return True if successful, false otherwise.
 */
bool GT_verifyTask(GT_CmdParameters *cmdparam);

/**
 * Task that deals with extending operations.
 * @param [in] cmdparam Pointer to command-line parameters
 * @return True if successful, false otherwise.
 */
bool GT_extendTask(GT_CmdParameters *cmdparam);

/**
 * Task that deals with extending operations.
 * @param [in] cmdparam Pointer to command-line parameters.
 * @return True if successful, false otherwise.
 */
bool GT_getPublicationsFileTask(GT_CmdParameters *cmdparam);

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

/**
 * Gives time difference between the current and last call in ms.
 * @return Time difference in ms.
 */
unsigned int measureLastCall(void);

/**
 * Gives time difference between the current and last call of function measureLastCall in ms
 * @return Time difference in ms.
 */
unsigned int measuredTime(void);

/**
 * Gives a string representing string value of measured time by measureLastCall.
 * String format is (%i ms).
 * @return The pointer to the string
 * 
 * @note Pointer is always pointing to the same memory fiels. 
 */
char* str_measuredTime(void);

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

#define MEASURE_TIME(code_here) \
    {   \
    measureLastCall(); \
    code_here; \
    measureLastCall(); \
    }



#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

