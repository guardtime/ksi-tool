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

#include "component.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tool_box.h"
#include "param_set/strn.h"
#include "ksitool_err.h"
#include "tool.h"

#ifdef _WIN32
#include <Windows.h>
#endif

typedef struct TOOL_COMPONENT_st TOOL_COMPONENT;

struct TOOL_COMPONENT_st {
	/**
	 *
	 */
	char *name;
	int id;

	/**
	 * Functions needed to run tool component.
	 */
	int (*run)(int argc, char **argv, char **envp);
	char* (*help_toString)(char *buf, size_t buf_len);
	const char* (*getDesc)(void);
	/**
	 * Support to load libraries.
	 */
	void *libloader;
	void (*libloader_free)(void *libloader);
};

struct TOOL_COMPONENT_LIST_st {
	TOOL_COMPONENT **component;
	size_t count;
	size_t size;
};

static int tool_component_new(char *name, int (*run)(int argc, char **argv, char **envp), char* (*help_toString)(char *buf, size_t buf_len), const char* (*getDesc)(void), int id, TOOL_COMPONENT **new) {
	int res;
	TOOL_COMPONENT *tmp = NULL;
	size_t name_len = 0;
	char *tmp_name = NULL;


	if (name == NULL || run == NULL || help_toString == NULL || getDesc == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Create empty TOOL_COMPONENT object and initialize function pointers.
	 */
	tmp = (TOOL_COMPONENT*)malloc(sizeof(TOOL_COMPONENT));
	if (tmp == NULL) {
		res = KT_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->name = NULL;
	tmp->id = id;
	tmp->run = run;
	tmp->getDesc = getDesc;
	tmp->help_toString = help_toString;
	tmp->libloader = NULL;
	tmp->libloader_free = NULL;

	/**
	 * Initialize name.
	 */
	name_len = strlen(name);

	tmp_name = (char*)malloc(name_len + 1);
	if (tmp_name == NULL) {
		res = KT_OUT_OF_MEMORY;
		goto cleanup;
	}

	strcpy(tmp_name, name);
	tmp_name[name_len] = '\0';

	tmp->name = tmp_name;
	*new = tmp;
	tmp = NULL;
	tmp_name = NULL;
	res = KT_OK;

cleanup:

	free(tmp);
	free(tmp_name);

	return res;
}

static void tool_component_free(TOOL_COMPONENT *obj) {
	if (obj != NULL) {
		if (obj->libloader != NULL && obj->libloader_free != NULL) {
			obj->libloader_free(obj->libloader);
		}

		if (obj->name != NULL) free(obj->name);
		free(obj);
	}
}

int TOOL_COMPONENT_LIST_new(size_t max_size, TOOL_COMPONENT_LIST **new) {
	int res;
	TOOL_COMPONENT_LIST *tmp = NULL;
	size_t i = 0;

	if (max_size == 0 || new == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	tmp = (TOOL_COMPONENT_LIST*)malloc(sizeof(TOOL_COMPONENT_LIST));
	if (tmp == NULL) {
		res = KT_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->component = NULL;
	tmp->count = 0;
	tmp->size = max_size;

	tmp->component = (TOOL_COMPONENT**)malloc(sizeof(TOOL_COMPONENT*) * max_size);
	if (tmp->component == NULL) {
		res = KT_OUT_OF_MEMORY;
		goto cleanup;
	}

	for(i = 0; i< max_size; i++) {
		tmp->component[i] = NULL;
	}

	*new = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	TOOL_COMPONENT_LIST_free(tmp);

	return res;
}

void TOOL_COMPONENT_LIST_free(TOOL_COMPONENT_LIST *obj) {
	int i;

	if (obj != NULL) {
		for (i = 0; i < obj->count; i++) {
			tool_component_free(obj->component[i]);
		}
		free(obj->component);
		free(obj);
	}
}

int TOOL_COMPONENT_LIST_add(TOOL_COMPONENT_LIST *list,
		char *name,
		int (*run)(int argc, char **argv, char **envp),
		char* (*help_toString)(char *buf, size_t buf_len),
		const char* (*getDesc)(void),
		int id) {
	int res;
	TOOL_COMPONENT *tmp = NULL;

	if (list == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (list->count >= list->size) {
		res = KT_INDEX_OVF;
		goto cleanup;
	}

	res = tool_component_new(name, run, help_toString, getDesc, id, &tmp);
	if (res != KT_OK) {
		goto cleanup;
	}

	list->component[list->count] = tmp;
	tmp = NULL;
	list->count++;

	res = KT_OK;

cleanup:

	tool_component_free(tmp);

	return res;
}

int TOOL_COMPONENT_LIST_run(TOOL_COMPONENT_LIST *list, int id, int argc, char **argv, char **envp) {
	size_t i = 0;

	if (list == NULL) {
		return -1;
	}

	for (i = 0; i < list->count; i++) {
		if (list->component[i]->id == id) {
			return list->component[i]->run(argc, argv, envp);
		}
	}

	return -1;
}

char *TOOL_COMPONENT_LIST_helpToString(TOOL_COMPONENT_LIST *list, int id, char *buf, size_t buf_len) {
	size_t i = 0;
	size_t count = 0;


	if (list == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}


	for (i = 0; i < list->count; i++) {
		if (list->component[i]->id == id) {
			if (list->component[i]->help_toString == NULL) {
				count += PST_snprintf(buf + count, buf_len - count, "%s %s help.\n", TOOL_getName(), list->component[i]->name);
				count += PST_snprintf(buf + count, buf_len - count, "Unavailable.\n");
			}
			return list->component[i]->help_toString(buf, buf_len);
		}
	}

	return NULL;
}

char* TOOL_COMPONENT_LIST_toString(TOOL_COMPONENT_LIST *list, const char* preffix, char *buf, size_t buf_len) {
	int i;
	size_t count = 0;
	size_t max_size = 0;
	size_t tmp;
	char format[32];

	if (list == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}

	/**
	 * Extract the largest task name, and create a formats sting.
	 */
	for (i = 0; i < list->count; i++) {
		tmp = strlen(list->component[i]->name);
		if (tmp > max_size) {
			max_size = tmp;
		}
	}

	PST_snprintf(format, sizeof(format), "%%s%%-%is ", max_size);

	/**
	 * Generate help string for all known tasks.
	 */
	for (i = 0; i < list->count; i++) {
		/*TODO: If lines are too long make it possible to fix the format.*/
		tmp = strlen(list->component[i]->name);
		/**
		 * Print the tasks name.
		 */
		count += PST_snprintf(buf + count, buf_len - count, format,
				preffix == NULL ? "" : preffix,
				list->component[i]->name);

		/**
		 * Print the description.
		 */
		count += PST_snprintf(buf + count, buf_len - count, "- %s\n",
				list->component[i]->getDesc == NULL ? "description not available." : list->component[i]->getDesc());
	}


	buf[buf_len - 1] = '\0';
	return buf;
}
