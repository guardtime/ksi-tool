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

#include "conf_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ksi/compatibility.h>
#include "tool_box/param_control.h"
#include "tool_box.h"
#include "ksitool_err.h"
#include "smart_file.h"
#include "printer.h"

char* CONF_generate_param_set_desc(char *description, const char *flags, char *buf, size_t buf_len) {
	char *extra_desc = NULL;
	int is_S = 0;
	int is_X = 0;
	int is_P = 0;
	size_t count = 0;


	if (buf == NULL || buf_len == 0) return NULL;

	is_S = strchr(flags, 'S') != NULL ? 1 : 0;
	is_X = strchr(flags, 'X') != NULL ? 1 : 0;
	is_P = strchr(flags, 'P') != NULL ? 1 : 0;

	extra_desc = (description == NULL) ? "" : description;
	count += KSI_snprintf(buf + count, buf_len - count, "{C}{c}%s", extra_desc);

	if (is_S) {
		count += KSI_snprintf(buf + count, buf_len - count, "{S}{aggr-user}{aggr-key}");
	}

	if (is_X) {
		count += KSI_snprintf(buf + count, buf_len - count, "{X}{ext-user}{ext-key}");
	}

	if (is_P) {
		count += KSI_snprintf(buf + count, buf_len - count, "{P}{cnstr}{V}{W}{publications-file-no-verify}");
	}

	return buf;
}

int CONF_createSet(PARAM_SET **conf) {
	int res;
	PARAM_SET *tmp = NULL;
	char buf[1024];

	if (conf == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = PARAM_SET_new(CONF_generate_param_set_desc(NULL, "SXP", buf, sizeof(buf)), &tmp);
	if (res != PST_OK) goto cleanup;

	res = CONF_initialize_set_functions(tmp, "SXP");
	if (res != PST_OK) goto cleanup;

	*conf = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	PARAM_SET_free(tmp);

	return res;
}

int CONF_initialize_set_functions(PARAM_SET *conf, const char *flags) {
	int res = KT_UNKNOWN_ERROR;
	int is_S = 0;
	int is_X = 0;
	int is_P = 0;


	if (conf == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	is_S = strchr(flags, 'S') != NULL ? 1 : 0;
	is_X = strchr(flags, 'X') != NULL ? 1 : 0;
	is_P = strchr(flags, 'P') != NULL ? 1 : 0;

	if (is_P) {
		res = PARAM_SET_addControl(conf, "{V}{W}", isFormatOk_inputFile, isContentOk_inputFileRestrictPipe, convertRepair_path, NULL);
		if (res != PST_OK) goto cleanup;

		res = PARAM_SET_addControl(conf, "{P}", isFormatOk_url, NULL, convertRepair_url, NULL);
		if (res != PST_OK) goto cleanup;

		res = PARAM_SET_addControl(conf, "{publications-file-no-verify}", isFormatOk_flag, NULL, NULL, NULL);
		if (res != PST_OK) goto cleanup;

		res = PARAM_SET_addControl(conf, "{cnstr}", isFormatOk_constraint, NULL, convertRepair_constraint, NULL);
		if (res != PST_OK) goto cleanup;
	}

	if (is_S) {
		res = PARAM_SET_addControl(conf, "{S}", isFormatOk_url, NULL, convertRepair_url, NULL);
		if (res != PST_OK) goto cleanup;

		res = PARAM_SET_addControl(conf, "{aggr-user}{aggr-key}", isFormatOk_userPass, NULL, NULL, NULL);
		if (res != PST_OK) goto cleanup;
	}

	if (is_X) {
		res = PARAM_SET_addControl(conf, "{X}", isFormatOk_url, NULL, convertRepair_url, NULL);
		if (res != PST_OK) goto cleanup;

		res = PARAM_SET_addControl(conf, "{ext-key}{ext-user}", isFormatOk_userPass, NULL, NULL, NULL);
		if (res != PST_OK) goto cleanup;
	}

	res = PARAM_SET_addControl(conf, "{c}{C}", isFormatOk_int, isContentOk_int, NULL, extract_int);
	if (res != PST_OK) goto cleanup;


	res = KT_OK;

cleanup:

	return res;
}

static int conf_fromFile(PARAM_SET *set, const char *fname, const char *source, int priority) {
	int res;

	if (fname == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (!SMART_FILE_doFileExist(fname)) {
		res = KT_IO_ERROR;
		goto cleanup;
	}

	if (!SMART_FILE_isReadAccess(fname)) {
		res = KT_NO_PRIVILEGES;
		goto cleanup;
	}

	res = PARAM_SET_readFromFile(set, fname, source, priority);
	if (res != KT_OK) goto cleanup;


cleanup:

	return res;
}

int CONF_fromEnvironment(PARAM_SET *set, const char *env_name, char **envp, int priority, int convertPaths) {
	int res;
	const char *conf_file_name = NULL;


	if (env_name == NULL || envp == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = CONF_LoadEnvNameContent(set, env_name, envp);
	if (res != KT_OK) goto cleanup;

	conf_file_name = CONF_getEnvNameContent();

	if (conf_file_name != NULL) {
		res = conf_fromFile(set, conf_file_name, env_name, priority);
		if (res != PST_OK) goto cleanup;

		if (convertPaths) {
			res = CONF_convertFilePaths(set, conf_file_name, "{W}{V}{P}{X}{S}", env_name, priority);
			if (res != PST_OK) goto cleanup;
		}
	}

	res = KT_OK;

cleanup:

	return res;
}

int CONF_isInvalid(PARAM_SET *set) {
	if (set == NULL) return 1;

	if (!PARAM_SET_isFormatOK(set) || PARAM_SET_isUnknown(set) || PARAM_SET_isTypoFailure(set)
			|| PARAM_SET_isSetByName(set, "publications-file-no-verify")
			|| PARAM_SET_isSyntaxError(set)) {
		return 1;
	} else {
		return 0;
	}
}

int conf_report_errors(PARAM_SET *set, const char *fname, int res) {
	char buf[0xffff];
	if (res == KT_IO_ERROR) {
		print_errors("Error: Configurations file '%s' pointed by KSI_CONF does not exist.\n", fname);
		res = KT_INVALID_CONF;
		goto cleanup;
	} else if (res == KT_NO_PRIVILEGES) {
		print_errors("Error: User has no privileges to access configurations file '%s' pointed by KSI_CONF.\n", fname);
		res = KT_INVALID_CONF;
		goto cleanup;
	} else if (CONF_isInvalid(set)) {
		print_errors("Error: Configurations file '%s' pointed by KSI_CONF is invalid:\n", fname);
		print_errors("%s\n", CONF_errorsToString(set, "  ", buf, sizeof(buf)));
		res = KT_INVALID_CONF;
		goto cleanup;
	}
cleanup:
	return res;
}

char *CONF_errorsToString(PARAM_SET *set, const char *prefix, char *buf, size_t buf_len) {
	char tmp[4096];
	size_t count = 0;

	if (set == NULL || buf == NULL || buf_len == 0) return NULL;

	if (PARAM_SET_isSyntaxError(set)) {
		PARAM_SET_syntaxErrorsToString(set, prefix, tmp, sizeof(tmp));
		count += KSI_snprintf(buf + count, buf_len - count, "%s", tmp);
		goto cleanup;
	}

	if (PARAM_SET_isTypoFailure(set)) {
			PARAM_SET_typosToString(set, PST_TOSTR_DOUBLE_HYPHEN, prefix, tmp, sizeof(tmp));
			count += KSI_snprintf(buf + count, buf_len - count, "%s", tmp);
	}

	if (PARAM_SET_isUnknown(set)) {
			PARAM_SET_unknownsToString(set, prefix, tmp, sizeof(tmp));
			count += KSI_snprintf(buf + count, buf_len - count, "%s", tmp);
	}

	if (!PARAM_SET_isFormatOK(set)) {
		PARAM_SET_invalidParametersToString(set, prefix, getParameterErrorString, tmp, sizeof(tmp));
		count += KSI_snprintf(buf + count, buf_len - count, "%s", tmp);
	}

	if (PARAM_SET_isSetByName(set, "publications-file-no-verify")) {
		count += KSI_snprintf(buf + count, buf_len - count, "%sConfigurations flag 'publications-file-no-verify' can only be defined on command-line.\n",
				prefix == NULL ? "" : prefix);

	}

cleanup:
	return buf;
}

int env_is_loaded = 0;
static char env_var_name[1024];
static char env_name_content[2048];

int CONF_LoadEnvNameContent(PARAM_SET *set, const char *env_name, char **envp) {
	int res;
	char name[1024];
	char value[2024];


	if (env_name == NULL || envp == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	env_is_loaded = 0;

	while (*envp!=NULL) {
		if (STRING_extractAbstract(*envp, NULL, "=", name, sizeof(name), NULL, NULL, NULL) == NULL) {
			envp++;
			continue;
		}

		if (strcmp(name, env_name) == 0) {
			if (STRING_extract(*envp, "=", NULL, value, sizeof(value)) == NULL) {
				res = KT_INVALID_INPUT_FORMAT;
				goto cleanup;
			}

			KSI_strncpy(env_var_name, name, sizeof(env_var_name));
			KSI_strncpy(env_name_content, value, sizeof(env_name_content));
			env_is_loaded = 1;
			break;
		}

        envp++;
    }



	res = KT_OK;

cleanup:

	return res;
}

int CONF_isEnvSet(void) {
	return env_is_loaded;
}

const char *CONF_getEnvName(void) {
	if (env_is_loaded == 0) return NULL;
	else return env_var_name;
}

const char *CONF_getEnvNameContent(void) {
	if (env_is_loaded == 0) return NULL;
	else return env_name_content;
}

static int conf_convert_path(PARAM_SET *set, const char *conf_file, const char *param_name, const char *source, int prio) {
	int res = KT_INVALID_ARGUMENT;
	int count = 0;
	int i = 0;
	char *value = NULL;
	char buf[1024];
	const char *new_file_name = NULL;

	if (set == NULL || conf_file == NULL || param_name == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = PARAM_SET_getValueCount(set, param_name, source, prio, &count);
	if (res != PST_OK) goto cleanup;


	for (i = 0; i < count; i++) {
		value = NULL;
		new_file_name = NULL;
		buf[0] = '\0';

		/* Get a parameter and convert the path. Add the value to the top of the list.*/
		res = PARAM_SET_getStr(set, param_name, source, prio, i, &value);
		if (res != PST_OK && res != PST_PARAMETER_INVALID_FORMAT) goto cleanup;

		if (value == NULL) {
			new_file_name = NULL;
		} else {
			if (strcmp(param_name, "P") == 0 || strcmp(param_name, "X") == 0 || strcmp(param_name, "S") == 0) {
				new_file_name = PATH_URI_getPathRelativeToFile(conf_file, value, buf, sizeof(buf));
			} else {
				new_file_name = PATH_getPathRelativeToFile(conf_file, value, buf, sizeof(buf));
			}
		}

		res = PARAM_SET_add(set, param_name, new_file_name, source, prio);
		if (res != PST_OK) goto cleanup;

		/* Remove the old value. */
		res = PARAM_SET_clearValue(set, param_name, source, prio, i);
		if (res != PST_OK) goto cleanup;

	}

	res = KT_OK;

cleanup:

	return res;
}


static int isValidNameChar(int c) {
	if ((ispunct(c) || isspace(c)) && c != '_' && c != '-') return 0;
	else return 1;
}

int CONF_convertFilePaths(PARAM_SET *set, const char *conf_file, const char *names, const char *source, int prio) {
	int res;
	char buf[1024];
	const char *pName = names;

	if (set == NULL || names == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	while (pName != NULL && pName[0] != '\0') {
		pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL);
		res = conf_convert_path(set, conf_file, buf, source, prio);
		if (res != KT_OK) goto cleanup;
	}

	res = KT_OK;

cleanup:

	return res;
}