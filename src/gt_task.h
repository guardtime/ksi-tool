/* 
 * File:   gt_task.h
 * Author: Taavi
 *
 * Created on July 22, 2014, 1:31 PM
 */

#ifndef GT_TASK_H
#define	GT_TASK_H

#include "gt_cmd_parameters.h"

#ifdef	__cplusplus
extern "C" {
#endif

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
bool GT_publicationsFileTask(GT_CmdParameters *cmdparam);


#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

