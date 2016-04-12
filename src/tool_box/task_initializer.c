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
#include "tool_box/param_control.h"
#include "tool_box/tool_box.h"
#include "printer.h"
#include "conf.h"

enum service_info_priorities {
	PRIORITY_KSI_CONF,
	PRIORITY_KSI_CONF_USER,
	PRIORITY_KSI_CONF_FILE,
	PRIORITY_CMD,
};

static int isUserInfoInsideUrl(const char *url, char *buf_u, char *buf_k, size_t buf_len);
static int extract_user_info_from_url_if_needed(PARAM_SET *set, const char *flag_name, const char *usr_name, const char *key_name);

int TASK_INITIALIZER_check_analyze_report(PARAM_SET *set, TASK_SET *task_set, double task_set_sens, double task_dif, TASK **task) {
	int res;
	char buf[0xffff];
	TASK *pTask = NULL;


	if (set == NULL || task_set == NULL || task == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Check for invalid values.
     */
	if (!PARAM_SET_isFormatOK(set)) {
		PARAM_SET_invalidParametersToString(set, NULL, getParameterErrorString, buf, sizeof(buf));
		print_errors("%s", buf);
		res = KT_INVALID_CMD_PARAM;
		goto cleanup;
	}

	/**
	 * Check for typos and unknown parameters.
     */
	if (PARAM_SET_isTypoFailure(set)) {
			print_errors("%s\n", PARAM_SET_typosToString(set, PST_TOSTR_DOUBLE_HYPHEN, NULL, buf, sizeof(buf)));
			res = KT_INVALID_CMD_PARAM;
			goto cleanup;
	} else if (PARAM_SET_isUnknown(set)){
			print_errors("%s\n", PARAM_SET_unknownsToString(set, "Warning: ", buf, sizeof(buf)));
	}

	/**
	 * Analyze task set and Extract the task if consistent one exists, print help
	 * messaged otherwise.
     */
	res = TASK_SET_analyzeConsistency(task_set, set, task_set_sens);
	if (res != PST_OK) goto cleanup;

	res = TASK_SET_getConsistentTask(task_set, &pTask);
	if (res != PST_OK && res != PST_TASK_ZERO_CONSISTENT_TASKS && res !=PST_TASK_MULTIPLE_CONSISTENT_TASKS) goto cleanup;
	*task = pTask;


	/**
	 * If task is not present report errors.
     */
	if (pTask == NULL) {
		int ID;
		if (TASK_SET_isOneFromSetTheTarget(task_set, task_dif, &ID)) {
			print_errors("%s", TASK_SET_howToRepair_toString(task_set, set, ID, NULL, buf, sizeof(buf)));
		} else {
			print_errors("%s", TASK_SET_suggestions_toString(task_set, 3, buf, sizeof(buf)));
		}

		res = KT_INVALID_CMD_PARAM;
		goto cleanup;
	}


	res = KT_OK;

cleanup:

	/* pTask is just a reference. There is no need to free it. */

	return res;
}

int TASK_INITIALIZER_getServiceInfo(PARAM_SET *set, int argc, char **argv, char **envp) {
	int res;
	PARAM_SET *conf_env = NULL;
	PARAM_SET *conf_file = NULL;
	char buf[1024];
	char fname[1024];
	char *conf_file_name = NULL;

	res = CONF_createSet(&conf_env);
	if (res != KT_OK) goto cleanup;

	res = CONF_createSet(&conf_file);
	if (res != KT_OK) goto cleanup;

	/**
	 * Include conf from environment.
     */
	res = CONF_fromEnvironment(conf_env, "KSI_CONF", envp, PRIORITY_KSI_CONF, fname, sizeof(fname));
	res = conf_report_errors(set, fname, res);
	if (res != KT_OK) goto cleanup;

	/**
	 * Read conf from command line.
     */
	PARAM_SET_readFromCMD(set, argc, argv, "CMD", PRIORITY_CMD);

	/**
	 * Include configurations file.
     */
	if (PARAM_SET_isSetByName(set, "conf")) {
		res = PARAM_SET_getStr(set, "conf", NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &conf_file_name);
		if (res != PST_OK && res != PST_PARAMETER_INVALID_FORMAT) goto cleanup;

		if (conf_file_name != NULL && res == PST_OK) {
			res = PARAM_SET_readFromFile(conf_file, conf_file_name, conf_file_name, PRIORITY_KSI_CONF_FILE);
			if (res != PST_OK && res != PST_INVALID_FORMAT) goto cleanup;
		}

		if (CONF_isInvalid(conf_file)) {
			print_errors("Configurations file '%s' is invalid:\n", conf_file_name);
			print_errors("%s\n", CONF_errorsToString(conf_file, "  ", buf, sizeof(buf)));
			res = KT_INVALID_CONF;
			goto cleanup;
		}
	}


	/**
	 * Merge conf files to the set.
     */
	res = PARAM_SET_IncludeSet(set, conf_env);
	if (res != PST_OK) goto cleanup;

	res = PARAM_SET_IncludeSet(set, conf_file);
	if (res != PST_OK) goto cleanup;

	/**
	 * Check for embedded user info from the URLs. Parameters are stored in layers
	 * at specified priority. If there are URL (S and X), the most important URL
	 * values are extracted and it is checked if user info at the given priority level
	 * exists. If not, user info is extracted from URLs if present and appended to
	 * the same priority level as URLs.
     */
	res = extract_user_info_from_url_if_needed(set, "S", "aggr-user", "aggr-key");
	if (res != KT_OK) goto cleanup;

	res = extract_user_info_from_url_if_needed(set, "X", "ext-user", "ext-key");
	if (res != KT_OK) goto cleanup;

	res = KT_OK;

cleanup:

	PARAM_SET_free(conf_env);
	PARAM_SET_free(conf_file);

	return res;
}

static int isUserInfoInsideUrl(const char *url, char *buf_u, char *buf_k, size_t buf_len) {
	char *ret = NULL;
	char buf[1024];

	if (url == NULL || *url == '\0' || buf_u == NULL || buf_k == NULL || buf_len == 0) return 0;
	ret = STRING_extractAbstract(url, "://", "@", buf, sizeof(buf), find_charAfterStrn, find_charBeforeStrn, NULL);
	if (ret != buf) return 0;

	ret = STRING_extract(buf, NULL, ":", buf_u, buf_len);
	if (ret != buf_u) return 0;

	if (buf_u[0] == ':') buf_u[0] = '\0';

	ret = STRING_extract(buf, ":", NULL, buf_k, buf_len);
	if (ret != buf_k) return 0;

	return 1;
}

static int extract_user_info_from_url_if_needed(PARAM_SET *set, const char *flag_name, const char *usr_name, const char *key_name) {
	int res;
	char *url = NULL;
	char usr[1024];
	char key[1024];
	char src[1024];
	char *dummy = NULL;
	PARAM_ATR atr;

	if (set == NULL || flag_name == NULL || usr_name == NULL || key_name == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}
	/**
	 * Extract the url withe the greatest priority for further examination.
     */
	res = PARAM_SET_getStr(set, flag_name, NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &url);
	if (res != PST_OK && res != PST_PARAMETER_EMPTY && res != PST_PARAMETER_INVALID_FORMAT) {
		goto cleanup;
	} else if (res == PST_PARAMETER_EMPTY || res == PST_PARAMETER_INVALID_FORMAT) {
		res = KT_OK;
		goto cleanup;
	}

	/**
	 * If there is a user info embedded into url, check if there is a need to
	 * extract the values and append to the set.
     */
	if (isUserInfoInsideUrl(url, usr, key, sizeof(usr))) {
		res = PARAM_SET_getAtr(set, flag_name, NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &atr);
		if (res != PST_OK) goto cleanup;

		KSI_snprintf(src, sizeof(src), "%s.%s.url_user_info",
				atr.source == NULL ? "" : atr.source,
				flag_name);

		/**
		 * Check if the user at the given priority level exists.
		 */
		res = PARAM_SET_getStr(set, usr_name, NULL, atr.priority, PST_INDEX_LAST, &dummy);
		if (res == PST_PARAMETER_VALUE_NOT_FOUND || res == PST_PARAMETER_EMPTY) {
			res = PARAM_SET_add(set, usr_name, usr, src, atr.priority);
			if (res != PST_OK) goto cleanup;
		} else if (res != PST_OK && res != PST_PARAMETER_EMPTY && res != PST_PARAMETER_EMPTY) {
			goto cleanup;
		}

		/**
		 * Check if the key at the given priority level exists.
		 */
		res = PARAM_SET_getStr(set, key_name, NULL, atr.priority, PST_INDEX_LAST, &dummy);
		if (res == PST_PARAMETER_VALUE_NOT_FOUND || res == PST_PARAMETER_EMPTY) {
			res = PARAM_SET_add(set, key_name, key, src, atr.priority);
			if (res != PST_OK) goto cleanup;
		} else if (res != PST_OK && res != PST_PARAMETER_EMPTY && res != PST_PARAMETER_EMPTY) {
			goto cleanup;
		}
	}

	res = KT_OK;

cleanup:

	return res;
}