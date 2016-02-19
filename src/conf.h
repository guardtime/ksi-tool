#ifndef CONF_H
#define	CONF_H

#include "param_set/param_set.h"

#define CONF_PARAM_DESC "{S}{X}{P}{aggr-user}{aggr-pass}{ext-user}{ext-pass}{cnstr}{V}{W}{C}{c}"

#ifdef	__cplusplus
extern "C" {
#endif

char* CONF_generate_desc(char *description, char *buf, size_t buf_len);
	
int CONF_createSet(PARAM_SET **conf);

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

