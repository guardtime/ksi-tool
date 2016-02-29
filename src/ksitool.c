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
#include <string.h>
#include <ksi/compatibility.h>
#include "param_set/param_set.h"
#include "param_set/task_def.h"
#include "tool_box/default_tasks.h"
#include "tool_box/param_control.h"
#include "tool_box/tool_box.h"
#include "tool_box/component.h"
#include "ksitool_err.h"
#include "printer.h"
#include "conf.h"


#ifndef _WIN32
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif

const char *getVersion(void) {
	static const char versionString[] = VERSION;
	return versionString;
}

const char *getName(void) {
	static const char versionString[] = TOOL_NAME;
	return versionString;
}

static char *hash_algorithms_to_string(char *buf, size_t buf_len) {
	int i;
	size_t count = 0;

	if (buf == NULL || buf_len == 0) {
		return NULL;
	}


	for (i = 0; i < KSI_NUMBER_OF_KNOWN_HASHALGS; i++) {
		if (KSI_isHashAlgorithmSupported(i)) {
			count += KSI_snprintf(buf + count, buf_len - count, "%s%s",
				count == 0 ? "" : " ",
				KSI_getHashAlgorithmName(i)
				);
		}
	}
	if (count > 0) {
		count += KSI_snprintf(buf + count, buf_len - count, ".");
	}

	return buf;
}

static void print_general_help(PARAM_SET *set){
	int res;
	const char *toolVersion = NULL;
	const char *apiVersion = NULL;
	char *aggre_url = NULL;
	char *ext_url = NULL;
	char *pub_url = NULL;
	char *cnstr = NULL;
	char buf[1024];
	int i = 0;

	if (set == NULL) {
		return;
	}

	toolVersion = getVersion();
	apiVersion = KSI_getVersion();

	res = PARAM_SET_getStr(set, "S", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &aggre_url);
	if (res != PST_OK && res != PST_PARAMETER_EMPTY) return;

	res = PARAM_SET_getStr(set, "X", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &ext_url);
	if (res != PST_OK && res != PST_PARAMETER_EMPTY) return;

	res = PARAM_SET_getStr(set, "P", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &pub_url);
	if (res != PST_OK && res != PST_PARAMETER_EMPTY) return;

	aggre_url = aggre_url != NULL ? aggre_url : "Not defined.";
	ext_url = ext_url != NULL ? ext_url : "Not defined.";
	pub_url = pub_url != NULL ? pub_url : "Not defined.";

	print_info("\nDefault service access URL-s:\n");

	/**
	 * Print information about how to define default service access urls, if at
	 * least one is not defined.
     */
	if (aggre_url == NULL || ext_url == NULL || pub_url == NULL) {
		print_info(
		"  To define default URL-s, system environment variable KSI_CONF must be defined,\n"
		"  that is going to point to ksi configurations file. Configurations file has\n"
		"  similar syntax to the command-line, but parameters are placed line by line.\n"
		"  For extender and aggregator -X <url> [--ext-user <str> --ext-pass <str>] and \n"
		"  -S <url> [--aggr-user <str> --aggr-pass <str>] must be defined. For publications\n"
		"  file -P <url> [--cnstr <oid>=<value>] must be defined.\n\n"
		);
	}

	/**
	 * Print info about default services.
     */
	print_info(
	"  Signing:		%s\n"
	"  Extending/Verifying:	%s\n"
	"  Publications file:	%s\n\n",
		aggre_url, ext_url, pub_url
	);

	print_info("Default publications file certificate constraints:\n");
	while (PARAM_SET_getStr(set, "cnstr", NULL, PST_PRIORITY_HIGHEST, i, &cnstr) == PST_OK) {
		print_info("  %s\n", cnstr);
		i++;
	}

	if (i == 0) {
		print_info("  none\n");
	} else {
		print_info("\n");
	}

	/**
	 * Print info about supported hash algorithms.
     */
	print_info(
	"Supported hash algorithms (-H, -F):\n"
	"  %s\n",
		hash_algorithms_to_string(buf, sizeof(buf)));
}

static int ksitool_compo_get(TASK_SET *tasks, PARAM_SET **set, TOOL_COMPONENT_LIST **compo);


int main(int argc, char** argv, char **envp) {
	int res;
	PARAM_SET *set_task_name = NULL;
	PARAM_SET *set = NULL;
	PARAM_SET *configuration = NULL;
	TOOL_COMPONENT_LIST *components = NULL;
	TASK_SET *tasks = NULL;
	TASK *task = NULL;
	int retval = EXIT_SUCCESS;
	char buf[0xffff];

	print_init();

	/**
	 * Define parameter and task set.
     */
	res = PARAM_SET_new("{h}{version}{conf}", &set);
	if (res != PST_OK) goto cleanup;

	res = TASK_SET_new(&tasks);
	if (res != PST_OK) goto cleanup;

	res = CONF_createSet(&configuration);
	if (res != PST_OK) goto cleanup;

	/**
	 * Load the configurations file from environment.
     */
	res = CONF_fromEnvironment(configuration, "KSI_CONF", envp, 0);
	if (res != PST_OK) goto cleanup;


	/**
	 * Get all possible components to run.
     */
	res = ksitool_compo_get(tasks, &set_task_name, &components);
	if (res != PST_OK) {
		printf("Unable to load: Unable to run components");
		goto cleanup;
	}

	/**
	 * Add the values to the set.
     */
	res = PARAM_SET_add(set_task_name, argv[1], NULL, NULL, 0);

	if (argc > 1) {
		res = PARAM_SET_readFromCMD(set, argc, argv, NULL, 0);
	}

	/**
	 * Extract the task.
     */
	res = TASK_SET_analyzeConsistency(tasks, set_task_name, 0.2);
	if (res != PST_OK) goto cleanup;

	res = TASK_SET_getConsistentTask(tasks, &task);
	if (res != PST_OK && res != PST_TASK_ZERO_CONSISTENT_TASKS && res !=PST_TASK_MULTIPLE_CONSISTENT_TASKS) goto cleanup;

	/**
	 * Simple tool help handler.
     */
	if (PARAM_SET_isSetByName(set, "h")) {
		print_info("%s %s (C) Guardtime\n", getName(), getVersion());
		print_info("%s (C) Guardtime\n\n", KSI_getVersion());

		if (task == NULL) {
			print_info("Usage %s <task> <arguments>\n", getName());
			print_info("All known tasks:\n");
			print_info("%s", TOOL_COMPONENT_LIST_toString(components, "  ", buf, sizeof(buf)));
		} else {
			print_info("%s\n", TOOL_COMPONENT_LIST_helpToString(components, TASK_getID(task),buf, sizeof(buf)));
		}

		print_general_help(configuration);

		goto cleanup;
	} else if (PARAM_SET_isSetByName(set, "version")) {
		print_info("%s %s (C) Guardtime\n", getName(), getVersion());
		goto cleanup;
	}

	if (CONF_isInvalid(configuration)) {
		print_errors("KSI Service configuration is invalid:\n");
		print_errors("%s\n", CONF_errorsToString(configuration, "  ", buf, sizeof(buf)));
		res = KT_INVALID_CONF;
		goto cleanup;
	}


	/**
	 * Invalid task. Give user some hints.
     */
	if (task == NULL) {
		print_errors("Error: Invalid task. Read help (-h) or man page.\n");
		if (PARAM_SET_isTypoFailure(set_task_name)) {
			print_info("%s\n", PARAM_SET_typosToString(set_task_name, PST_TOSTR_NONE, NULL, buf, sizeof(buf)));
		} else if (PARAM_SET_isUnknown(set_task_name)){
			print_info("%s\n", PARAM_SET_unknownsToString(set_task_name, NULL, buf, sizeof(buf)));
		}

		goto cleanup;
	}


	/**
	 * Run component by its ID.
	 */
	retval = TOOL_COMPONENT_LIST_run(components, TASK_getID(task), argc - 1, argv + 1, envp);
//	printf("PROC RET %X\n", res);
//	printf("Consistent task is: %i\n", TASK_getID(task));
	res = KT_OK;


cleanup:

	if (res != KT_OK) {
		retval = EXIT_FAILURE;
	}

	PARAM_SET_free(set);
	PARAM_SET_free(set_task_name);
	PARAM_SET_free(configuration);
	TASK_SET_free(tasks);
	TOOL_COMPONENT_LIST_free(components);

	return retval;
}

static int ksitool_compo_get(TASK_SET *tasks, PARAM_SET **set, TOOL_COMPONENT_LIST **compo) {
	int res;
	TOOL_COMPONENT_LIST *tmp_compo = NULL;
	PARAM_SET *tmp_set = NULL;

	/**
	 * Create parameter list that contains all known tasks.
     */
	res = PARAM_SET_new("{sign}{extend}{verify}{pubfile}{tlv}{conf}", &tmp_set);
	if (res != PST_OK) goto cleanup;

	res = TOOL_COMPONENT_LIST_new(32, &tmp_compo);
	if (res != PST_OK) goto cleanup;

	/**
	 * Define all components as possible tasks.
     */
	TASK_SET_add(tasks, 0, "Sign", "sign", NULL, NULL, NULL);
	TASK_SET_add(tasks, 1, "Verify", "verify", NULL, NULL, NULL);
	TASK_SET_add(tasks, 2, "extend", "extend", NULL, NULL, NULL);
	TASK_SET_add(tasks, 3, "pubfile", "pubfile", NULL, NULL, NULL);
	TASK_SET_add(tasks, 0xffff, "conf", "conf", NULL, NULL, NULL);

	/**
	 * Define tool component as runnable.
     */
	TOOL_COMPONENT_LIST_add(tmp_compo, "sign", sign_run, sign_help_toString,  sign_get_desc, 0);
	TOOL_COMPONENT_LIST_add(tmp_compo, "verify", verify_run, verify_help_toString, verify_get_desc, 1);
	TOOL_COMPONENT_LIST_add(tmp_compo, "extend", extend_run, extend_help_toString, extend_get_desc, 2);
	TOOL_COMPONENT_LIST_add(tmp_compo, "pubfile", pubfile_run, pubfile_help_toString, pubfile_get_desc, 3);
	TOOL_COMPONENT_LIST_add(tmp_compo, "conf", conf_run, conf_help_toString, conf_get_desc, 0xffff);

	*set = tmp_set;
	*compo = tmp_compo;
	tmp_set = NULL;
	tmp_compo = NULL;
	res = KT_OK;

cleanup:

	PARAM_SET_free(tmp_set);
	TOOL_COMPONENT_LIST_free(tmp_compo);

	return res;
}
