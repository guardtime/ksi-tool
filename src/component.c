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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "component.h"
#include "ksitool_err.h"
#include "gt_cmd_common.h"

#ifdef _WIN32
#include <Windows.h>
#endif

typedef struct TOOL_COMPONENT_st TOOL_COMPONENT;

struct TOOL_COMPONENT_st {
	/**
	 *
	 */
	const char *name;
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


static int loadLibrary(const char *fname, void **lib) {
#ifdef _WIN32
	HMODULE tmp = NULL;
#else
	void *tmp = NULL;
#endif

	if (fname == NULL  || lib == NULL) {
		return KT_INVALID_ARGUMENT;
	}

#ifdef _WIN32
	tmp = LoadLibrary(fname);
	if (tmp == NULL) return KT_UNABLE_TO_LOAD_LIB;
#else
	tmp = NULL;
#endif

	*lib = (void*)tmp;
	return KT_OK;
}

static void freeLibrary(void *lib) {
#ifdef _WIN32
	HMODULE plugin = (HMODULE)lib;
	if (lib == NULL) return;
	FreeLibrary(plugin);
#else
	;
#endif
}

static int getFunctionPointer(void *lib, const char *funcName, void **func) {
	void *funcPointer = NULL;
	if (lib == NULL || funcName == NULL || func == NULL) {
		return KT_INVALID_ARGUMENT;
	}
#ifdef _WIN32
	funcPointer = (void*) GetProcAddress(lib, funcName);
#else
	funcPointer = NULL;
#endif
	if (funcPointer == NULL) {
		return KT_FUNCTION_NOT_FOUND;
	}

	*func = funcPointer;
	return KT_OK;
}


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

	free(obj);
	}
}

static int tool_component_load(char *name, const char *fname, int id, TOOL_COMPONENT **new) {
	int res;
	TOOL_COMPONENT *tmp;
	void *lib = NULL;
	void *run = NULL;
	void *help_toString = NULL;
	void *get_desc = NULL;

	if (name == NULL || fname == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Load the library and functions needed.
     */
	res = loadLibrary(fname, &lib);
	if (res != KT_OK) goto cleanup;

	res = getFunctionPointer(lib, RUN_FUNC_NAME, &run);
	if (res != KT_OK) goto cleanup;

	res = getFunctionPointer(lib, HELP_FUNC_NAME, &help_toString);
	if (res != KT_OK) goto cleanup;

	res = getFunctionPointer(lib, DESC_FUNC_NAME, &get_desc);
	if (res != KT_OK) goto cleanup;

	res = tool_component_new(name,
		(int (*)(int , char **, char **))run,
		(char* (*)(char *, size_t))help_toString,
		(const char* (*)(void))get_desc,
		id,
		&tmp);
	if (res != KT_OK) goto cleanup;

	tmp->libloader = lib;
	tmp->libloader_free = freeLibrary;
	lib = NULL;

	*new = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	tool_component_free(tmp);
	freeLibrary(lib);

	return res;
}

static int tool_component_run(TOOL_COMPONENT *new, int argc, char **argv, char **envp) {
	if (new == NULL || argc == 0 || argv == NULL || envp == NULL) return KT_INVALID_ARGUMENT;
	if (new->run == NULL) return KT_COMPONENT_HAS_NO_IMPLEMENTATION;
	return new->run(argc, argv, envp);
}

static char* tool_component_helpToString(TOOL_COMPONENT *new, char *buf, size_t buf_len) {
	if (new == NULL || buf == NULL || buf_len == 0) return NULL;
	if (new->help_toString == NULL) return NULL;
	return new->help_toString(buf, buf_len);
}


int TOOL_COMPONENT_LIST_new(size_t max_size, TOOL_COMPONENT_LIST **new) {
	int res;
	TOOL_COMPONENT_LIST *tmp;
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
	size_t i = 0;

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
	TOOL_COMPONENT *tmp = NULL;
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
	TOOL_COMPONENT *tmp = NULL;
	size_t i = 0;
	size_t count = 0;


	if (list == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}


	/*TODO: ksitool is hard coded. Make version extracting global.*/
	for (i = 0; i < list->count; i++) {
		if (list->component[i]->id == id) {
			if (list->component[i]->help_toString == NULL) {
				count += snprintf(buf + count, buf_len - count, "ksitool %s help.\n", list->component[i]->name);
				count += snprintf(buf + count, buf_len - count, "Unavailable.\n");
			}
			return list->component[i]->help_toString(buf, buf_len);
		}
	}

	return NULL;
}

char* TOOL_COMPONENT_LIST_toString(TOOL_COMPONENT_LIST *list, char *buf, size_t buf_len) {
	int i;
	size_t count = 0;
	size_t sub_count = 0;
	size_t line_counter = 0;
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

	snprintf(format, sizeof(format), "%%-%is ", max_size);

	/**
	 * Generate help string for all known tasks.
     */
	for (i = 0; i < list->count; i++) {
		/*TODO: If lines are too long make it possible to fix the format.*/
		tmp = strlen(list->component[i]->name);
		/**
		 * Print the tasks name.
         */
		count += snprintf(buf + count, buf_len - count, format, list->component[i]->name);

		/**
		 * Print the description.
		 */
		count += snprintf(buf + count, buf_len - count, "- %s\n",
				list->component[i]->getDesc == NULL ? "description not available." : list->component[i]->getDesc());
	}


	buf[buf_len - 1] = '\0';
	return buf;
}