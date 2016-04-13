#ifndef CONF_H
#define	CONF_H

#include "param_set/param_set.h"

#define CONF_PARAM_DESC "{S}{X}{P}{aggr-user}{aggr-key}{ext-user}{ext-key}{cnstr}{V}{W}{C}{c}{publications-file-no-verify}"

#ifdef	__cplusplus
extern "C" {
#endif

char* CONF_generate_desc(char *description, char *buf, size_t buf_len);
	
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
 * \return KT_OK if successful, error code otherwise.
 */
int CONF_initialize_set_functions(PARAM_SET *conf);

/**
 * Loads configurations file from environment variable \c env_name that points to the path
 * to ksi configurations file.
 * \param set		- PARAM_SET object to fill with parameters from the configurations file.
 * \param env_name	- The name of the environment variable.
 * \param env		- Pointer to the pointer to c-string values representing environment variables.
 * \param priority	- The priority of the parameters.
 * \param buf		- Pointer to the buffer to store the value of the specified environment variable. Can be NULL.
 * \param len		- The size of the buffer.
 * \return KT_OK if successful, error code otherwise. KT_IO_ERROR if file do not exist or
 * KT_NO_PRIVILEGES if access is not permitted.
 */
int CONF_fromEnvironment(PARAM_SET *set, const char *env_name, char **envp, int priority, char *buf, size_t len);

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

