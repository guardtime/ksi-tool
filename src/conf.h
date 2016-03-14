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
 * {V}{W}    - isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, NULL.
 * {X}{P}{S} - isFormatOk_url, NULL, convertRepair_url, NULL
 * {aggr-user}{aggr-key}{ext-key}{ext-user}
 *			 - isFormatOk_userPass, NULL, NULL, NULL
 * {cnstr}   - isFormatOk_constraint, NULL, convertRepair_constraint, NULL
 * {c}{C}    - isFormatOk_int, isContentOk_int, NULL, extract_int
 * \param conf	parameter set object.
 * \return KT_OK if successful, error code otherwise.
 */
int CONF_initialize_set_functions(PARAM_SET *conf);

int CONF_fromFile(PARAM_SET *set, const char *fname, const char *source, int priority);

int CONF_fromEnvironment(PARAM_SET *set, const char *env_name, char **envp, int priority);

int CONF_isInvalid(PARAM_SET *set);

char *CONF_errorsToString(PARAM_SET *set, const char *prefix, char *buf, size_t buf_len);

int conf_run(int argc, char** argv, char **envp);

char *conf_help_toString(char *buf, size_t len);

const char *conf_get_desc(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CONF_H */

