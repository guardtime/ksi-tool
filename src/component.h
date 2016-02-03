/**************************************************************************
 *
 * GUARDTIME CONFIDENTIAL
 *
 * Copyright (C) [2015] Guardtime, Inc
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

#ifndef COMPONENT_H
#define	COMPONENT_H


#ifdef	__cplusplus
extern "C" {
#endif

typedef struct TOOL_COMPONENT_LIST_st TOOL_COMPONENT_LIST;

#define RUN_FUNC_NAME "run"
#define HELP_FUNC_NAME "help_toString"
#define DESC_FUNC_NAME "get_desc"

int TOOL_COMPONENT_LIST_new(size_t max_size, TOOL_COMPONENT_LIST **new);
void TOOL_COMPONENT_LIST_free(TOOL_COMPONENT_LIST *obj);
int TOOL_COMPONENT_LIST_add(TOOL_COMPONENT_LIST *list,
		char *name,
		int (*run)(int argc, char **argv, char **envp),
		char* (*help_toString)(char *buf, size_t buf_len),
		const char* (*getDesc)(void),
		int id);
int TOOL_COMPONENT_LIST_run(TOOL_COMPONENT_LIST *list, int id, int argc, char **argv, char **envp);

char* TOOL_COMPONENT_LIST_toString(TOOL_COMPONENT_LIST *list, char *buf, size_t buf_len);
char *TOOL_COMPONENT_LIST_helpToString(TOOL_COMPONENT_LIST *list, int id, char *buf, size_t buf_len);


#ifdef	__cplusplus
}
#endif

#endif	/* COMPONENT_H */

