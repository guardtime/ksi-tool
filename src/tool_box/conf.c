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

#include <stdio.h>
#include <stdlib.h>
#include <ksi/compatibility.h>
#include "param_set/param_set.h"
#include "param_set/strn.h"
#include "ksitool_err.h"
#include "printer.h"
#include "conf_file.h"
#include "tool.h"
#include "default_tasks.h"
#include "common.h"

static void print_conf_file(const char *fname, int (*print)(const char *format, ... ));

int conf_run(int argc, char** argv, char **envp) {
	int res;
	PARAM_SET *set = NULL;
	char buf[0xffff];
	VARIABLE_IS_NOT_USED(envp);

	res = PARAM_SET_new("{h|help}{dump}{d}", &set);
	if (res != PST_OK) goto cleanup;


	res = PARAM_SET_parseCMD(set, argc, argv, "CMD", 3);
	if (res != PST_OK) {
		print_errors("Error: Unable to parse command-line.\n");
		goto cleanup;
	}

	/**
	 * Check for typos and unknown parameters.
	 */
	if (PARAM_SET_isTypoFailure(set)) {
			print_errors("%s\n", PARAM_SET_typosToString(set, NULL, buf, sizeof(buf)));
			res = KT_INVALID_CMD_PARAM;
			goto cleanup;
	} else if (PARAM_SET_isUnknown(set)){
			print_errors("%s\n", PARAM_SET_unknownsToString(set, "Error: ", buf, sizeof(buf)));
			res = KT_INVALID_CMD_PARAM;
			goto cleanup;
	}

	res = CONF_LoadEnvNameContent(set, "KSI_CONF", envp);
	if (res != KT_OK) goto cleanup;

	if (PARAM_SET_isSetByName(set, "dump")) {
		if (CONF_isEnvSet()) {
			print_conf_file(CONF_getEnvNameContent(), print_result);
		}
	} else if (PARAM_SET_isSetByName(set, "d")) {
		if (CONF_isEnvSet()) {
			print_debug("%s", CONF_getEnvNameContent());
		}
	} else {
		print_result("%s\n", conf_help_toString(buf, sizeof(buf)));
	}

	res = KT_OK;

cleanup:

	PARAM_SET_free(set);

	return KSITOOL_errToExitCode(res);
}

char *conf_help_toString(char *buf, size_t len) {
	int res;
	PARAM_SET *set;
	size_t count = 0;
	char tmp[0xfff];
	char *ret = NULL;

	if (buf == NULL || len == 0) return NULL;

	res = CONF_createSet(&set);
	if (res != PST_OK) goto cleanup;

	PARAM_SET_setHelpText(set, "V", "<file>", "Certificate file in PEM format for publications file verification.");

	count += KSI_snprintf(buf + count, len - count,
		"Usage:\n"
		" %s conf -h | -d | --dump \n"
		" -d        - print KSI_CONF value to stderr if is configured.\n"
		" --dump    - dump configuration file pointed by KSI_CONF to stdout.\n"
		" -h        - print the current help message.\n\n"
			, TOOL_getName()
			);

	count += PST_snhiprintf(buf + count, len - count, 80, 0, 0, NULL, ' ', "KSI configuration file help:\n\\>2\n"
			"The KSI command-line tool has several configuration options, most of them are  related to the KSI service configuration (e.g. KSI signing service URL and access credentials). The configuration options are described below. There are following ways to specify these configuration options:\n"
			"\\>3\n*\\>5 directly on command line (highest priority);"
			"\\>3\n*\\>5 in a file specified by the --conf command-line argument;"
			"\\>3\n*\\>5 in a file specified by the KSI_CONF environment variable (lowest priority)."
			"\\>2\n\n"
			"If a configuration option is specified in more than one source (e.g. both directly on command-line argument and in a configuration file) the source with the highest priority will be used. A short parameter or multiple flags must have prefix - and long parameters have prefix --. If some parameter values contain whitespace characters double quote marks (\") must be used to wrap the entire value. If double quote mark or backslash have to be used inside the value part an escape character (\\\\) must be typed before the character(\\\\\" or \\\\\\\\). If configuration option with unknown or invalid key-value pairs is used, an error is generated.\n\n"
			"In configuration file each key-value pair must be placed on a single line. Start the line with # to write a comment. Not full paths (V, W and P with URI scheme file://) are interpreted as relative to the configuration file.\n\n");

	count += KSI_snprintf(buf + count, len - count, "All known parameters:\n");

	ret = PARAM_SET_helpToString(set, "S,aggr-user,aggr-key,aggr-hmac-alg,H,X,ext-user,ext-key,ext-hmac-alg,P,cnstr,V,W,max-lvl,max-aggr-rounds,mdata-cli-id,mdata-mac-id,mdata-sqn-nr,mdata-req-tm,c,C,publications-file-no-verify,apply-remote-conf", 1, 13, 80, tmp, sizeof(tmp));

	count += KSI_snprintf(buf + count, len - count,
		"%s\n\n"
		"An example configuration file:\n\n"
		" # --- BEGINNING ---\n"
		" # KSI Signing service parameters:\n"
		" -S http://example.gateway.com:3333/gt-signingservice\n"
		" --aggr-user anon\n"
		" --aggr-key anon\n"
		"\n"
		" # KSI Extending service:\n"
		" # Note that ext-key real value is &h/J\"kv\\G##\n"
		" -X http://example.gateway.com:8010/gt-extendingservice\n"
		" --ext-user anon\n"
		" --ext-key \"&h/J\\\"kv\\\\G##\"\n"
		"\n"
		" # KSI Publications file:\n"
		" -P http://verify.guardtime.com/ksi-publications.bin\n"
		" --cnstr email=publications@guardtime.com\n"
		" --cnstr \"org=Guardtime AS\"\n"
		" # --- END ---\n"
		"\n", tmp
		);


cleanup:
	if (res != PST_OK || ret == NULL) {
		PST_snprintf(buf + count, len - count, "\nError: There were failures while generating help by PARAM_SET.\n");
	}
	PARAM_SET_free(set);
	return buf;
}

const char *conf_get_desc(void) {
	return "KSI Service configuration file utility.";
}

static void print_conf_file(const char *fname, int (*print)(const char *format, ... )) {
	FILE *f = NULL;
	char buf[1024];
	size_t count = 0;

	if (fname == NULL || print == NULL) return;

	f = fopen(fname, "r");
	if (f == NULL) print("Error: Unable to read file for printing '%s'.\n", fname);

	while (!feof(f)) {
		buf[0] = '\0';
		count = fread(buf, 1, sizeof(buf) - 1, f);
		if (feof(f)) buf[count] = '\0';
		print("%s", buf);
	}
	return;
}
