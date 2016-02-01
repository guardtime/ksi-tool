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

#include "gt_task_support.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "param_set/task_def.h"
#include "gt_task.h"
#include <ksi/ksi.h>
#include <ksi/compatibility.h>
#include <ksi/net.h>
#include "ksitool_err.h"
#include "gt_cmd_control.h"
#include "param_set/param_value.h"
#include "component.h"

#ifndef _WIN32
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif

#define KSITOOL_VERSION_STRING "ksitool " VERSION

const char *getVersion(void) {
	static const char versionString[] = KSITOOL_VERSION_STRING;
	return versionString;
}

static bool includeParametersFromFile(PARAM_SET *set, int priority){
	/*Read command-line parameters from file*/
	if(PARAM_SET_isSetByName(set, "inc")){
		char *fname = NULL;
		char *fname2 = NULL;
		int i=0;
		int n=0;
		unsigned count=0;

		while (PARAM_SET_getStrValue(set, "inc", NULL, PST_PRIORITY_NONE, i, &fname) == PST_OK) {
			PARAM_SET_getValueCount(set, "{inc}", NULL, PST_PRIORITY_NONE, &count);

			for(n=0; n<i; n++){
				PARAM_SET_getStrValue(set, "inc", NULL, PST_PRIORITY_NONE, n, &fname2);

				if(strcmp(fname, fname2)==0){
					fname = NULL;
					break;
				}
			}

			PARAM_SET_readFromFile(fname, set, priority);
			if(++i>255){
				print_errors("Error: Include file list is too long.");
				return false;
			}
		}
	}
	return true;
}

static char default_pubUrl[1024] = "";
static char default_extenderUrl[1024] = "";
static char default_aggreUrl[1024] = "";

static int ksitool_mightItBeUri(const char *uri) {
	if (uri == NULL) return 0;
	return KSI_UriSplitBasic(uri, NULL, NULL, NULL, NULL) == KSI_OK ? 1 : 0;
}

static int ksitool_load_urls_from_env(PARAM_SET *set, const char *line, const char *env_name, const char *param_name, int priority) {
	const char *nxt = line;
	char url[1024] = "";
	unsigned url_count = 0;
	char pass[1024] = "";
	unsigned pass_count = 0;
	char user[1024] = "";
	unsigned user_count = 0;
	char chunk[1024];
	int res = KT_OK;
	bool s, v, x, p, T, E, aggre, P, cnstr, cnstr_warn;
	bool constraintsAlreadySet;
	bool isUrlTagOk;

	aggre = PARAM_SET_isSetByName(set, "aggre");
	s = PARAM_SET_isSetByName(set, "s");
	v = PARAM_SET_isSetByName(set, "v");
	x = PARAM_SET_isSetByName(set, "x");
	p = PARAM_SET_isSetByName(set, "p");
	T = PARAM_SET_isSetByName(set, "T");
	P = PARAM_SET_isSetByName(set, "P");
	E = PARAM_SET_isSetByName(set, "E");
	cnstr = PARAM_SET_isSetByName(set, "cnstr");

	/* It should indicate if constraints have been defined on command-line. */
	constraintsAlreadySet = (E || cnstr || P) ? true : false;
	cnstr_warn = true;
	isUrlTagOk = (strstr(line, "url=") != NULL) ? true : false;

	while (1) {
		int success = 0;
		nxt = STRING_getChunks(nxt, chunk, sizeof(chunk));
		if (STRING_extract(chunk, "url=", NULL, url, sizeof(url)) != NULL) {
			success = 1;
			url_count++;
		}
		if (STRING_extract(chunk, "pass=", NULL, pass, sizeof(pass)) != NULL) {
			success = 1;
			pass_count++;
		}
		if (STRING_extract(chunk, "user=", NULL, user, sizeof(user)) != NULL) {
			success = 1;
			user_count++;
		}

		/* Add publications file constraints */
		if (success == 0 && (strcmp(env_name, "KSI_PUBFILE") == 0)) {
				if (constraintsAlreadySet) {
					if (cnstr_warn && isUrlTagOk) {
						print_warnings("Warning: Ignoring constraints from environment variable '%s'.\n", env_name);
						cnstr_warn = false;
					}
				} else {
					PARAM_SET_add(set, "cnstr", chunk, env_name, PST_PRIORITY_VALID_BASE);
				}
		} else if (success == 0 && strcmp(env_name, "KSI_PUBFILE") != 0) {
			if (ksitool_mightItBeUri(chunk)) {
				print_warnings("Warning: It seems that 'url=' is missing before '%s' in %s.\n", chunk, env_name);
			} else {
				print_warnings("Warning: Undefined field '%s' in %s.\n", chunk, env_name);
			}
		}

		if (nxt == NULL) break;
	}

	if (url_count > 1 || user_count > 1 || pass_count > 1) {
		print_errors("Error: Environment variable %s is invalid. Multiple %s%s%s defined.\n", env_name,
				url_count > 1 ? "url-s" : "",
				user_count > 1 ? " users" : "",
				pass_count > 1 ? " passwords" : "");
		print_errors("Error: Invalid '%s'.\n", line);
		res = KT_INVALID_INPUT_FORMAT;
	}

	if (url[0] == '\0') {
		print_errors("Error: Environment variable %s is invalid.%s\n", env_name, isUrlTagOk ? "" : " Tag 'url=' is missing.");
		print_errors("Error: Invalid '%s'.\n", line);
		res = KT_INVALID_INPUT_FORMAT;
	}

	/* ADD URL */
	PARAM_SET_add(set, param_name, url, env_name, priority);
	if(strcmp(env_name, "KSI_AGGREGATOR") == 0) strcpy(default_aggreUrl, url);
	else if(strcmp(env_name, "KSI_EXTENDER") == 0) strcpy(default_extenderUrl, url);
	else if(strcmp(env_name, "KSI_PUBFILE") == 0) strcpy(default_pubUrl, url);


	/* ADD password and user name */
	if((s || aggre) && (strcmp(env_name, "KSI_AGGREGATOR") == 0)
		|| (x || v || (p && T)) && (strcmp(env_name, "KSI_EXTENDER") == 0)) {
		if (user[0]) PARAM_SET_add(set, "user", user, env_name, priority);
		if (pass[0])PARAM_SET_add(set, "pass", pass, env_name, priority);
	}

	return res;
}

/*
 * This function contains a little hack. As parameter set has 2 parameters for user
 * and pass, there is no room where to put both aggregator and extender data. To avoid
 * defining new parameters this function decides from which variable user and pass is
 * extracted. This function MUST be called after reading files and command line to
 * make correct decisions.
 */
static bool includeParametersFromEnvironment(PARAM_SET *set, char **envp, int priority){
	bool ret = true;
	const char *key = NULL;
	char name[1024];
	char value[2024];
	int res = KT_OK;

	while(*envp!=NULL){
		if (STRING_extractAbstract(*envp, NULL, "=", name, sizeof(name), NULL, NULL, NULL) == NULL) {
			envp++;
			continue;
		}

		if(strcmp(name, "KSI_AGGREGATOR") == 0){
			res = ksitool_load_urls_from_env(set, STRING_extract(*envp, "=", NULL, value, sizeof(value)), name, "S", priority);
		} else if(strcmp(name, "KSI_EXTENDER") == 0){
			res = ksitool_load_urls_from_env(set, STRING_extract(*envp, "=", NULL, value, sizeof(value)), name, "X", priority);
		} else if(strcmp(name, "KSI_PUBFILE") == 0){
			res = ksitool_load_urls_from_env(set, STRING_extract(*envp, "=", NULL, value, sizeof(value)), name, "P", priority);
		}

		if (res != KT_OK) ret = false;

        envp++;
    }

	return ret;
}

static void printSupportedHashAlgorithms(void){
	int i = 0;

	for(i=0; i< KSI_NUMBER_OF_KNOWN_HASHALGS; i++){
		if(KSI_isHashAlgorithmSupported(i)){
			print_info("%s ", KSI_getHashAlgorithmName(i));
		}
	}
}

static void GT_pritHelp(void){
	char *ext_url = NULL;
	char *aggre_url = NULL;
	char *pub_url = NULL;
	const char *apiVersion = NULL;
	const char *toolVersion = NULL;

	ext_url = strlen(default_extenderUrl) > 0 ? default_extenderUrl : NULL;
	aggre_url = strlen(default_aggreUrl) > 0 ? default_aggreUrl : NULL;
	pub_url = strlen(default_pubUrl) > 0 ? default_pubUrl : NULL;;


	apiVersion = KSI_getVersion();
	toolVersion = getVersion();

	print_info(
			"\nGuardTime command-line signing tool\n"
			"%s.\n"
			"%s.\n"
			"Usage: <-s|-x|-p|-v> [more options]\n"
			"Where recognized options are:\n"
			" -s --sign\tsign data (-n -d -t):\n"
			"\t\t-s -f -o sign data file.\n"
			"\t\t-s -f -o -D sign stream and save data to file.\n"
			"\t\t-s -f -H -o sign data file with specific hash algorithm.\n"
			"\t\t-s -F -o sign hash.\n"
			" -x --extend\tuse online verification (eXtending) service (-n -r -d -t):\n"
			"\t\t-x -i -o extend signature to the nearest publication.\n"
			"\t\t-x -i -T -o extend signature to specified time.\n"
			" -p\t\tdownload Publications file (-r -d -t):\n"
			"\t\t-p -o --cnstr download publications file.\n"
			"\t\t-p -d dump publications file.\n"
			"\t\t-p -T create publication string.\n"
			" -v --verify\tverify signature or publications file. Use -f to provide\n"
			"\t\ta file or -F to use a pre-calculated hash value (-n -r -d -t):\n"
			"\t\t-v -x -i verify a signature online (calendar-based verification).\n"
			"\t\t-v -i verify a signature.\n"
			"\t\t-v -b -i verify a signature using specific publications file.\n"
			"\t\t-v --ref -i verify a signature using publication string.\n"
			"\t\t-v -b --cnstr verify publication file.\n"
			/*TODO: uncomment if implemented*/
//			" --aggre	use aggregator for (-n -t):\n"

//			"\t\t--aggre --htime display current aggregation root hash value and time.\n"
//			"\t\t--aggre --setsystime set system time from current aggregation.\n"


			"\nInput/output:\n"
			" -f <file>\tfile to be signed or verified.\n"
			" -F <hash>\tdata hash to be signed or verified. Hash format: <ALG>:<hash in hex>.\n"
			" -o <file>\toutput file name to store signature token or publications file.\n"
			" -D <file>\toutput file name to store data from stream.\n"
			" -i <file>\tinput signature token file to be extended or verified.\n"
			" --ref <str>\tpublication string.\n"
			" -b <file>\tuse a specified publications file.\n"
			" -H <ALG>\tuse a specific hash algorithm to hash the file to be signed.\n"
			" -T <UTC>\tspecify a publication time to extend to (use with -x) or a time\n"
			"\t\tto create a new publication code for (use with -p) as the number of\n"
			"\t\tseconds since 1970-01-01 00:00:00 UTC or time string formatted as\n"
			"\t\t\"YYYY-MM-DD hh:mm:ss\"\n"
			"\nDetails:\n"
			" -t\t\tprint service response Timing in ms.\n"
			" -n\t\tprint signer Name (identity).\n"
			" -r\t\tprint publication References (use with -vx).\n"
			" -d\t\tprint detailed information.\n"
			" --log <file>\tdump KSI log into file.\n"
			" --silent\tsilence info and warning messages.\n"
			" --nowarn\tsilence warning messages.\n"

			"\nConfiguration:\n"
			" -S <url>\tspecify Signing service URL.\n"
			" -X <url>\tspecify verification (eXtending) service URL.\n"
			" -P <url>\tspecify Publications file URL.\n"
			" --user <str>\tuser name.\n"
			" --pass <str>\tpassword.\n"
			" -c <num>\tset network transfer timeout, after successful Connect, in s.\n"
			" -C <num>\tset network Connect timeout in s (is not supported with tcp client).\n"
			" -V <file>\tspecify an OpenSSL-style trust store file for publications file\n"
			"\t\tverification. Can have multiple values (-V <file 1> -V <file 2>).\n"
			" -W <dir>\tspecify an OpenSSL-style trust store directory for publications\n"
			"\t\tfile verification.\n"
			" --cnstr <oid=value>\n"
			"\t\tuse OID and its expected value to verify publications file PKI\n"
			"\t\tsignature. At least one constraint must be defined to be able to\n"
			"\t\tverify publications file but it is possible to define more. If\n"
			"\t\tvalue part contains spaces use \" \" to wrap its contents. For\n"
            "\t\tcommon OID's there are string representations: 'email' for\n"
			"\t\t1.2.840.113549.1.9.1, 'country' for 2.5.4.6, 'org' for 2.5.4.10\n"
			"\t\tand 'common_name' for 2.5.4.3.\n"
			"\t\tExample --cnstr 2.5.4.6=EE --cnstr org=\"Guardtime AS\".\n"
			" --inc <file>\tuse configuration file containing command-line parameters.\n"
			"\t\tParameter must be written line by line.\n"

			"Help:\n"
			" -h --help\tHelp (You are reading it now).\n",
			toolVersion, apiVersion
			);

			print_info("\nDefault service access URL-s:\n"
			"\tTo define default URL-s, system environment variables must be defined.\n"
			"\tFor aggregator, define \"KSI_AGGREGATOR\"=\"url=<url> pass=<pass> user=<user>\".\n"
			"\tFor extender, define \"KSI_EXTENDER\"=\"url=<url> pass=<pass> user=<user>\".\n"
			"\tOnly the <url> part is mandatory. Default <pass> and <user> is \"anon\" and\n"
			"\tcan be used if such user is supported.\n\n"
			"\tFor publications file, define \"KSI_PUBFILE\"=\"url=<url> <constraint>\n"
			"\t<constraint> ...\". Constraint is formatted as <OID>=\"<value>\" where \"\"\n"
			"\tcan be omitted if 'value does not contain any white-space characters.\n"
			"\tPublications file url is mandatory but constraints are not if at least\n"
			"\tone constraint is defined on command-line (see --cnstr).\n\n"
			"\tUsing includes (--inc) or defining urls on command-line will\n"
			"\toverride defaults.\n\n"
			"\tSigning:		%s\n"
			"\tExtending/Verifying:	%s\n"
			"\tPublications file:	%s\n", (aggre_url ? aggre_url : "Not defined."), (ext_url ? ext_url : "Not defined."), (pub_url ? pub_url : "Not defined."));

			print_info("\nSupported hash algorithms (-H, -F):\n\t");
			printSupportedHashAlgorithms();
			print_info("\n");
}

static int wrapper_returnStr(const char* str, void** obj){
	*obj = (void*)str;
	return PST_OK;
}

static int wrapper_returnInt(const char* str,  void** obj){
	int *pI = (int*)obj;
	*pI = atoi(str);
	return PST_OK;
}


int main(int argc, char** argv, char **envp) {
	int res;
	PARAM_SET *set = NULL;
	TASK_SET *tasks = NULL;
	TASK *task = NULL;
	int retval = EXIT_SUCCESS;
	char buf[2048];
#ifdef _WIN32
#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
#endif
#endif

	print_init();

	/*Create parameter set*/
	PARAM_SET_new("{s|sign}*{x|extend}*{p}*{v|verify}*{t}*{r}*{d}*{n}*{h|help}*{o|out}{i}{D|dataout}{f}{b}{c}{C}{V}*"
				"{W}{S}>{X}>{P}>{F}{H}{T}{E}{inc}*{cnstr}*"
				"{ref}{user}>{pass}>{log}{silent}{nowarn}",
				&set);
	if(set == NULL) goto cleanup;

	/*Configure parameter set*/
	PARAM_SET_addControl(set, "{o}{D}{log}", isPathFormOk, isOutputFileContOK, NULL, wrapper_returnStr);
	PARAM_SET_addControl(set, "{i}{b}{f}{V}{W}{inc}", isPathFormOk, isInputFileContOK, convert_repairPath, wrapper_returnStr);
	PARAM_SET_addControl(set, "{F}", isImprintFormatOK, isImprintContOK, NULL, wrapper_returnStr);
	PARAM_SET_addControl(set, "{H}", isHashAlgFormatOK, isHashAlgContOK, NULL, wrapper_returnStr);
	PARAM_SET_addControl(set, "{S}{X}{P}", isURLFormatOK, contentIsOK, convert_repairUrl, wrapper_returnStr);
	PARAM_SET_addControl(set, "{c}{C}", isIntegerFormatOK, isIntegerContOk, NULL, wrapper_returnInt);
	PARAM_SET_addControl(set, "{T}", isUTCTimeFormatOk, contentIsOK, convert_UTC_to_UNIX, wrapper_returnInt);
	PARAM_SET_addControl(set, "{E}", isEmailFormatOK, contentIsOK, NULL, wrapper_returnStr);
	PARAM_SET_addControl(set, "{user}{pass}", isUserPassFormatOK, contentIsOK, NULL, wrapper_returnStr);
	PARAM_SET_addControl(set, "{ref}", formatIsOK, contentIsOK, NULL, wrapper_returnStr);
	PARAM_SET_addControl(set, "{cnstr}", isConstraintFormatOK, contentIsOK, convert_replaceWithOid, wrapper_returnStr);
	PARAM_SET_addControl(set, "{x}{s}{v}{p}{t}{r}{n}{d}{h}{silent}{nowarn}", isFlagFormatOK, contentIsOK, NULL, NULL);


	TASK_SET_new(&tasks);

	/*Define possible tasks*/
	/*						ID							DESC						MAN				ATL		FORBIDDEN					IGN	*/
	TASK_SET_add(tasks, signDataFile,				"Sign data file",				"s,f,o,S",		NULL,	"x,p,v,aggre,F,i,T",		"b,r");
	TASK_SET_add(tasks, signHash,					"Sign hash",					"s,F,o,S",		NULL,	"x,p,v,aggre,f,i,T,H",		"b,r");
	TASK_SET_add(tasks, extendTimestamp,			"Extend signature",				"x,i,o,P,X",	NULL,	"s,p,v,aggre,f,F,H",		NULL);
	TASK_SET_add(tasks, downloadPublicationsFile,	"Download publications file",	"p,o,P,cnstr",	NULL,	"s,x,v,aggre,f,F,T,i,H",	"n,b");
	TASK_SET_add(tasks, createPublicationString,	"Create publication string",	"p,T,X",		NULL,	"s,x,v,aggre,f,F,o,i,H",	"n,r");
	TASK_SET_add(tasks, verifyTimestamp,			"Verify signature",				"v,i",			NULL,	"s,p,x,aggre,H",			NULL);
	TASK_SET_add(tasks, verifyTimestampOnline,		"Verify online",				"v,x,i,X",		NULL,	"s,p,aggre,T,H",			NULL);
	TASK_SET_add(tasks, verifyPublicationsFile,		"Verify publications file",		"v,b,cnstr",	NULL,	"x,s,p,aggre,i,f,F,T,H",	NULL);
	TASK_SET_add(tasks, dumpPublicationsFile,		"Dump publications file",		"p,d",			NULL,	"s,x,v,aggre,f,F,T,i,H,o",	NULL);


	/*Read parameter set*/
	PARAM_SET_readFromCMD(argc, argv, set, 3);

	if (PARAM_SET_isSetByName(set, "silent")) {
		print_disable(PRINT_WARNINGS | PRINT_INFO);
	} else if (PARAM_SET_isSetByName(set, "nowarn")) {
		print_disable(PRINT_WARNINGS);
	}

	/* Add default user and pass if S or X is defined and user or pass is not. */
	if (PARAM_SET_isSetByName(set, "S") || PARAM_SET_isSetByName(set, "X")) {
		if(!PARAM_SET_isSetByName(set, "user"))
			PARAM_SET_add(set, "user", "anon", "default", 3);
		if(!PARAM_SET_isSetByName(set, "pass"))
			PARAM_SET_add(set, "pass", "anon", "default", 3);
	}

	if(includeParametersFromFile(set, 2) == false) goto cleanup;
	if(includeParametersFromEnvironment(set, envp, 1) == false) goto cleanup;
	if(PARAM_SET_isSetByName(set, "h")) goto cleanup;

	if (PARAM_SET_isSetByName(set, "E")) {
		char tmp[1024];
		char *email = NULL;
		print_warnings("Warning: Parameter E is deprecated and will be removed in later versions. Use --cnstr instead.\n");
		if (PARAM_SET_getStrValue(set, "E", NULL, PST_PRIORITY_NONE, PST_INDEX_FIRST, &email) != PST_OK) {
			print_errors("Error: Unable to convert parameter E to cnstr. Check email format.\n");
			retval = EXIT_FAILURE;
			goto cleanup;
		}

		print_warnings("Warning: Converting E to '--cnstr %s=%s'.\n", KSI_CERT_EMAIL, email);

		KSI_snprintf(tmp, sizeof(tmp), "%s=%s", KSI_CERT_EMAIL, email);

		if (PARAM_SET_add(set, "cnstr", tmp, "E", PST_PRIORITY_VALID_BASE) != PST_OK) {
			print_errors("Error: Unable to convert parameter E to cnstr.\n");
			retval = EXIT_FAILURE;
			goto cleanup;
		}
	}

	if(isPiping(set)) {
		print_disable(PRINT_WARNINGS | PRINT_INFO);
	}

	if (PARAM_SET_isTypoFailure(set)) {
		buf[0] = 0;
		print_errors("%s", PARAM_SET_typosToString(set, "Typo: ", buf, sizeof(buf)));
		retval = EXIT_INVALID_CL_PARAMETERS;
		goto cleanup;
	}

	if (PARAM_SET_isUnknown(set)) {
		buf[0] = 0;
		print_warnings("%s", PARAM_SET_unknownsToString(set, "Warning: ", buf, sizeof(buf)));
	}

	if(!PARAM_SET_isFormatOK(set)){
		buf[0] = 0;
		print_errors("%s", PARAM_SET_invalidParametersToString(set, "Error: ", getParameterErrorString, buf, sizeof(buf)));
		retval = EXIT_INVALID_CL_PARAMETERS;
		goto cleanup;
	}

	res = TASK_SET_analyzeConsistency(tasks, set, 0.1);
	if (res != PST_OK) {
		print_errors("Error: Unable to analyze task set.\n");
		retval = EXIT_FAILURE;
		goto cleanup;
	}

	res = TASK_SET_getConsistentTask(tasks, &task);
	if (res != PST_OK) {
		print_errors("Task is not defined. Use (-x, -s, -v, -p) and read help -h.\n");
		retval = EXIT_FAILURE;
		goto cleanup;
	}

	/*DO*/
	if(TASK_getID(task) == downloadPublicationsFile || TASK_getID(task) == createPublicationString
			|| TASK_getID(task) == dumpPublicationsFile) {
		retval=GT_publicationsFileTask(task);
	}
	else if (TASK_getID(task) == verifyPublicationsFile){
		retval = GT_verifyPublicationFileTask(task);
	}
	else if (TASK_getID(task) == signDataFile || TASK_getID(task) == signHash){
		retval=GT_signTask(task);
	}
	else if(TASK_getID(task) == extendTimestamp){
		retval=GT_extendTask(task);
	}
	else if(TASK_getID(task) == getRootH_T || TASK_getID(task) == setSysTime){
		retval=GT_other(task);
	}
	else if(TASK_getID(task) == verifyTimestamp || TASK_getID(task) == verifyTimestampOnline){
		retval = GT_verifySignatureTask(task);
	}

cleanup:

	if(PARAM_SET_isSetByName(set, "h")) GT_pritHelp();

	PARAM_SET_free(set);
	TASK_SET_free(tasks);

	/*Can be used in debug mode*/
	//_CrtDumpMemoryLeaks();
	return retval;
}
