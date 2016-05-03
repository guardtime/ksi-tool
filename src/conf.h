/**************************************************************************
 *
 * GUARDTIME CONFIDENTIAL
 *
 * Copyright (C) [2016] Guardtime, Inc
 * All Rights Reserved
 *
 * NOTICE:  All information contained herein is, and remains, the
 * property of Guardtime Inc and its suppliers, if any.
 * The intellectual and technical concepts contained herein are
 * proprietary to Guardtime Inc and its suppliers and may be
 * covered by U.S. and Foreign Patents and patents in process,
 * and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this
 * material is strictly forbidden unless prior written permission
 * is obtained from Guardtime Inc.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime Inc.
 */

#ifndef CONF_H
#define	CONF_H

#include "param_set/param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

int CONF_LoadEnvNameContent(PARAM_SET *set, const char *env_name, char **envp);
const char *CONF_getEnvName(void);
const char *CONF_getEnvNameContent(void);	
int CONF_isEnvSet(void);

/**
 * Generate \c PARAM_SET description and add configuration specific parameters.
 * \param description	-	Add extra descriptions.
 * \param flags			-	Add X, S and P do add extender, aggregator and publications file specific parameters.
 * \param buf			-	Buffer to store the description string.
 * \param buf_len		-	The size of the buffer to store the description strng.
 * \return \c buf if successful, NULL otherwise.
 */
char* CONF_generate_param_set_desc(char *description, const char *flags, char *buf, size_t buf_len);

/**
 * Creates a PARAM_SET object that is fully configured to support all paraeters
 * defined for ksi configurations file.
 * \param conf	-	Output parameter for PARAM_SET object.
 * \return KT_OK if successful, error code otherwise.
 */
int CONF_createSet(PARAM_SET **conf);

/**
 * Initialize default parameters that are service specific.
 * {V}{W}    - isFormatOk_inputFile, isContentOk_inputFileRestrictPipe, convertRepair_path, NULL.
 * {X}{P}{S} - isFormatOk_url, NULL, convertRepair_url, NULL
 * {aggr-user}{aggr-key}{ext-key}{ext-user}
 *			 - isFormatOk_userPass, NULL, NULL, NULL
 * {cnstr}   - isFormatOk_constraint, NULL, convertRepair_constraint, NULL
 * {c}{C}    - isFormatOk_int, isContentOk_int, NULL, extract_int
 * \param conf	parameter set object.
 * \param flags	-	Add X, S and P do add extender, aggregator and publications file specific configuration.
 * \return KT_OK if successful, error code otherwise.
 */
int CONF_initialize_set_functions(PARAM_SET *conf, const char *flags);

/**
 * Loads configurations file from environment variable \c env_name that points to the path
 * to ksi configurations file.
 * \param set		- PARAM_SET object to fill with parameters from the configurations file.
 * \param env_name	- The name of the environment variable.
 * \param env		- Pointer to the pointer to c-string values representing environment variables.
 * \param priority	- The priority of the parameters.
 * \convertPaths    - If this flag is set all paths in configurations file that are not full
 *						paths are interpreted as paths relative to the configurations file. 
 * \return KT_OK if successful, error code otherwise. KT_IO_ERROR if file do not exist or
 * KT_NO_PRIVILEGES if access is not permitted.
 */
int CONF_fromEnvironment(PARAM_SET *set, const char *env_name, char **envp, int priority, int convertPaths);

int conf_report_errors(PARAM_SET *set, const char *fname, int res);

int CONF_isInvalid(PARAM_SET *set);

char *CONF_errorsToString(PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);

int conf_run(int argc, char** argv, char **envp);

char *conf_help_toString(char *buf, size_t len);

const char *conf_get_desc(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CONF_H */

