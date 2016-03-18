#include "conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ksi/compatibility.h>
#include "param_set/param_set.h"
#include "ksitool_err.h"
#include "tool_box/tool_box.h"
#include "tool_box/param_control.h"
#include "printer.h"

char* CONF_generate_desc(char *description, char *buf, size_t buf_len) {
	char *extra_desc = NULL;

	if (buf == NULL || buf_len == 0) return NULL;

	extra_desc = (description == NULL) ? "" : description;
	KSI_snprintf(buf, buf_len, "%s%s", extra_desc, CONF_PARAM_DESC);

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

	res = PARAM_SET_new(CONF_generate_desc(NULL, buf, sizeof(buf)), &tmp);
	if (res != PST_OK) goto cleanup;

	res = CONF_initialize_set_functions(tmp);
	if (res != PST_OK) goto cleanup;

	*conf = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	PARAM_SET_free(tmp);

	return res;
}

int CONF_initialize_set_functions(PARAM_SET *conf) {
	int res;

	if (conf == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = PARAM_SET_addControl(conf, "{V}{W}", isFormatOk_inputFile, isContentOk_inputFile, convertRepair_path, NULL);
	if (res != PST_OK) goto cleanup;

	PARAM_SET_addControl(conf, "{X}{P}{S}", isFormatOk_url, NULL, convertRepair_url, NULL);
	if (res != PST_OK) goto cleanup;

	PARAM_SET_addControl(conf, "{aggr-user}{aggr-key}{ext-key}{ext-user}", isFormatOk_userPass, NULL, NULL, NULL);
	if (res != PST_OK) goto cleanup;

	PARAM_SET_addControl(conf, "{cnstr}", isFormatOk_constraint, NULL, convertRepair_constraint, NULL);
	if (res != PST_OK) goto cleanup;

	PARAM_SET_addControl(conf, "{c}{C}", isFormatOk_int, isContentOk_int, NULL, extract_int);
	if (res != PST_OK) goto cleanup;

	PARAM_SET_addControl(conf, "{publications-file-no-verify}", isFormatOk_flag, NULL, NULL, NULL);
	if (res != PST_OK) goto cleanup;

	res = KT_OK;

cleanup:

	return res;
}

int CONF_fromFile(PARAM_SET *set, const char *fname, const char *source, int priority) {
	int res;

	if (fname == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = PARAM_SET_readFromFile(set, fname, source, priority);
	if (res != KT_OK) goto cleanup;


cleanup:

	return res;
}

int CONF_fromEnvironment(PARAM_SET *set, const char *env_name, char **envp, int priority) {
	int res;
	char name[1024];
	char value[2024];


	if (env_name == NULL || envp == NULL || set == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

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

			res = CONF_fromFile(set, value, env_name, priority);
			if (res != PST_OK) goto cleanup;

			break;
		}

        envp++;
    }


	res = KT_OK;

cleanup:

	return res;
}

int CONF_isInvalid(PARAM_SET *set) {
	if (set == NULL) return 1;

	if (!PARAM_SET_isFormatOK(set) || PARAM_SET_isUnknown(set) || PARAM_SET_isTypoFailure(set) || PARAM_SET_isSetByName(set, "publications-file-no-verify")) {
		return 1;
	} else {
		return 0;
	}
}

char *CONF_errorsToString(PARAM_SET *set, const char *prefix, char *buf, size_t buf_len) {
	char tmp[4096];
	size_t count = 0;

	if (set == NULL || buf == NULL || buf_len == 0) return NULL;

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

	return buf;
}


int conf_run(int argc, char** argv, char **envp) {
	char buf[0xffff];
	argc;
	argv;
	envp;

	print_info("%s\n", conf_help_toString(buf, sizeof(buf)));

	return EXIT_SUCCESS;
}

char *conf_help_toString(char *buf, size_t len) {
	size_t count = 0;

	if (buf == NULL || len == 0) return NULL;

	count += KSI_snprintf(buf + count, len - count,
		"KSI Configurations file help:\n\n"
		"To define default KSI service access parameters a configurations file must be\n"
		"defined. Default configurations file is searched from the path pointed from\n"
		"environment variable KSI_CONF. Values from the default configurations file can\n"
		"be overloaded with a specific configurations file given from the command-line\n"
		"(--conf <file>) or just typing the same parameters to the command-line.\n"
		"\n"
		"Configurations file is composed with following parameters described below.\n"
		"Following key-value pairs must be placed one per line. To write a comment use #\n"
		"at the beginning of the line. If unknown or invalid key-value pairs are used,\n"
		"an error is generated until user applies all fixes needed.\n\n"
		"If some parameter values contain whitespace characters double quote mark\"\n"
		"must be used to wrap the entire value. If double quote mark have to be used\n"
		"inside the value part an escape character must be typed before the quote mark\n"
		"like that \\\"\n"
		"\n"
		"All known parameters that can be used:\n"
		);

	count += KSI_snprintf(buf + count, len - count,
		" -S <url>  - specify signing service URL.\n"
		" --aggr-user <str>\n"
		"           - user name for signing service.\n"
		" --aggr-key <str>\n"
		"           - HMAC key for signing service.\n"
		" -X <url>  - specify extending service URL.\n"
		" --ext-user <str>\n"
		"           - user name for extending service.\n"
		" --ext-key <str>\n"
		"           - HMAC key for extending service.\n"
		" -P <url>  - specify publications file URL (or file with uri scheme 'file://').\n"
		" --cnstr <oid=value>\n"
		"           - publications file certificate verification constraints.\n"
		" -V <file> - specify an OpenSSL-style trust store file for publications file verification.\n"
		" -W <dir>  - specify an OpenSSL-style trust store directory for publications file verification.\n"
		" -c <num>  - set network transfer timeout, after successful connect, in s.\n"
		" -C <num>  - set network Connect timeout in s (is not supported with tcp client).\n"
		" --publications-file-no-verify\n"
		"           - a flag to force the tool to trust the publications file without\n"
		"             verifying it. The flag can only be defined on command-line to avoid\n"
		"             the usage of insecure configurations files. It must be noted that the\n"
		"             option is insecure and must only be used for testing.\n"
		"\n"
		"\n"
		);

	count += KSI_snprintf(buf + count, len - count,
		"An example configurations file:\n\n"
		" # --- BEGINNING ---\n"
		" # KSI Signing service parameters:\n"
		" -S http://ksigw.test.guardtime.com:3333/gt-signingservice\n"
		" --aggr-user anon\n"
		" --aggr-key anon\n"
		"\n"
		" # KSI Extending service:\n"
		" # Note that ext-key real value is &h/J\"kv\\G##\n"
		" -X http://ksigw.test.guardtime.com:8010/gt-extendingservice\n"
		" --ext-user anon\n"
		" --ext-key \"&h/J\\\"kv\\\\G##\"\n"
		"\n"
		" # KSI Publications file:\n"
		" -P http://verify.guardtime.com/ksi-publications.bin\n"
		" --cnstr email=publications@guardtime.com\n"
		" --cnstr \"org=Guardtime AS\"\n"
		" # --- END ---\n"
		"\n"
		);


	return buf;
}

const char *conf_get_desc(void) {
	return "KSI Service configuration file utility.";
}