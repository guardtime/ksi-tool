/*
 * Copyright 2013-2016 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#ifndef COMPONENT_H
#define	COMPONENT_H

#include <stddef.h>	

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

char* TOOL_COMPONENT_LIST_toString(TOOL_COMPONENT_LIST *list, const char* preffix, char *buf, size_t buf_len);
char *TOOL_COMPONENT_LIST_helpToString(TOOL_COMPONENT_LIST *list, int id, char *buf, size_t buf_len);


#ifdef	__cplusplus
}
#endif

#endif	/* COMPONENT_H */

