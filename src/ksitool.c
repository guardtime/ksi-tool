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
#include "task_def.h"
#include "gt_task.h"
#include <ksi/ksi.h>

#ifndef _WIN32
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif

#ifdef COMMIT_ID
#  define KSITOOL_VERSION_STRING "ksitool " VERSION "-" COMMIT_ID
#else
#  define KSITOOL_VERSION_STRING "ksitool " VERSION
#endif

const char *getVersion(void) {
	static const char versionString[] = KSITOOL_VERSION_STRING;
	return versionString;
}

static bool includeParametersFromFile(paramSet *set, int priority){
	/*Read command-line parameters from file*/
	if(paramSet_isSetByName(set, "inc")){
		char *fname = NULL;
		char *fname2 = NULL;
		int i=0;
		int n=0;
		int count=0;
		
		while(paramSet_getStrValueByNameAt(set, "inc",i,&fname)){
			paramSet_getValueCountByName(set, "inc", &count);
			
			for(n=0; n<i; n++){
				paramSet_getStrValueByNameAt(set, "inc",n,&fname2);
				
				if(strcmp(fname, fname2)==0){
					fname = NULL;
					break;
				}
			}
			
			paramSet_readFromFile(fname, set, priority);
			if(++i>255){
				fprintf(stderr, "Error: Include file list is too long.");
				return false;
			}
		}
	}
	return true;
}

static bool getEnvValue(const char *str, const char *value, char *buf, unsigned bufLen){
	char *found = NULL;
	char format[1024];
	
	snprintf(format, sizeof(format),"%s=%%%is", value, bufLen);
	
	if((found=strstr(str, value)) == NULL) return false;
	if(sscanf(found, format, buf) != 1) return false;

	return true;
}

static char default_extenderUrl[1024] = "";
static char default_aggreUrl[1024] = "";

/*
 * This function contains a little hack. As parameter set has 2 parameters for user
 * and pass, there is no room where to put both aggregator and extender data. To avoid
 * defining new parameters this function decides from which variable user and pass is
 * extracted. This function MUST be called after reading files and command line to 
 * make correct decisions.     
 */
static bool includeParametersFromEnvironment(paramSet *set, char **envp, int priority){
	/*Read command line parameters from system variables*/
	bool ret = true;
	bool s, v, x, p, T, aggre;
	
	aggre = paramSet_isSetByName(set, "aggre");
	s = paramSet_isSetByName(set, "s");
	v = paramSet_isSetByName(set, "v");
	x = paramSet_isSetByName(set, "x");
	p = paramSet_isSetByName(set, "p");
	T = paramSet_isSetByName(set, "T");
	
	while(*envp!=NULL){
		char tmp[1024];
		char *found = NULL;

        if(strncmp(*envp, "KSI_AGGREGATOR", sizeof("KSI_AGGREGATOR")-1)==0){
			if(!getEnvValue(*envp, "url", tmp, sizeof(tmp))) {
				fprintf(stderr, "Error: Environment variable KSI_AGGREGATOR is invalid.\n");
				fprintf(stderr, "Error: Invalid '%s'.\n", *envp);
				ret = false;
			}

			paramSet_priorityAppendParameterByName("S", tmp, "KSI_AGGREGATOR", priority, set);
			strcpy(default_aggreUrl, tmp);
			
			if(s || aggre){
				if(getEnvValue(*envp, "user", tmp, sizeof(tmp)))
					paramSet_priorityAppendParameterByName("user", tmp, "KSI_AGGREGATOR", priority, set);
				if(getEnvValue(*envp, "pass", tmp, sizeof(tmp)))
					paramSet_priorityAppendParameterByName("pass", tmp, "KSI_AGGREGATOR", priority, set);
			}

		}
        else if(strncmp(*envp, "KSI_EXTENDER", sizeof("KSI_EXTENDER")-1)==0){
			if(!getEnvValue(*envp, "url", tmp, sizeof(tmp))){
				fprintf(stderr, "Error: Environment variable KSI_EXTENDER is invalid.\n");
				fprintf(stderr, "Error: Invalid '%s'.\n", *envp);
				ret  = false;
			}
			
			paramSet_priorityAppendParameterByName("X", tmp, "KSI_EXTENDER", priority, set);
			strcpy(default_extenderUrl, tmp);

			if(x || v || (p && T)){
				if(getEnvValue(*envp, "user", tmp, sizeof(tmp)))
					paramSet_priorityAppendParameterByName("user", tmp, "KSI_EXTENDER", priority, set);
				if(getEnvValue(*envp, "pass", tmp, sizeof(tmp)))
					paramSet_priorityAppendParameterByName("pass", tmp, "KSI_EXTENDER", priority, set);
			}

		}
		
        envp++;
    }
	return ret;
}

static void printSupportedHashAlgorithms(void){
	int i = 0;
	
	for(i=0; i< KSI_NUMBER_OF_KNOWN_HASHALGS; i++){
		if(KSI_isHashAlgorithmSupported(i)){
			fprintf(stderr,"%s ", KSI_getHashAlgorithmName(i));
		}
	}
}

static void GT_pritHelp(void){
	char *ext_url = NULL;
	char *aggre_url = NULL;
	const char *apiVersion = NULL;
	const char *toolVersion = NULL;
	
	ext_url = strlen(default_extenderUrl) > 0 ? default_extenderUrl : NULL;
	aggre_url = strlen(default_aggreUrl) > 0 ? default_aggreUrl : NULL;
	
	apiVersion = KSI_getVersion();
	toolVersion = getVersion();
	
	fprintf(stderr,
			"\nGuardTime command-line signing tool\n"
			"%s.\n"
			"%s.\n"
			"Usage: <-s|-x|-p|-v> [more options]\n"
			"Where recognized options are:\n"
			" -s --sign\tsign data (-n -d -t):\n"
			"\t\t-s -f -o sign data file.\n"
			"\t\t-s -f -H -o sign data file with specific hash algorithm.\n"
			"\t\t-s -F -o sign hash.\n"
			" -x --extend\tuse online verification (eXtending) service (-n -r -d -t):\n"
			"\t\t-x -i -o extend signature to the nearest publication.\n"
			"\t\t-x -i -T -o extend signature to specified time.\n"
			" -p\t\tdownload Publications file (-d -t):\n"
			"\t\t-p -o download publications file.\n"
			"\t\t-p -T create publication string.\n"
			" -v --verify\tverify signature or publications file. Use -f and -F\n"
			"\t\tfor document and its hash verification (-n -r -d -t):\n"
			"\t\t-v -x -i verify signature online (calendar-based verification).\n"
			"\t\t-v -i verify signature.\n"
			"\t\t-v -b -i verify signature using specific publications file.\n"
			"\t\t-v -b verify publication file.\n"
			" --aggre	use aggregator for (-n -t):\n"
			"\t\t--aggre --htime display current aggregation root hash value and time.\n"
			"\t\t--aggre --setsystime set system time from current aggregation.\n"
			
			
			"\nInput/output:\n"
			" -f <file>\tfile to be signed / verified.\n"
			" -F <hash>\tdata hash to be signed / verified. Hash format: <ALG>:<hash in hex>.\n"
			" -o <file>\toutput filename to store signature token or publications file.\n"
			" -i <file>\tinput signature token file to be extended / verified.\n"
			" -b <file>\tuse specified publications file.\n"
			" -H <ALG>\thash algorithm used to hash the file to be signed.\n"
			" -T <UTC>\tspecific publication time to extend to (use with -x) as number\n"
			"\t\tof seconds since 1970-01-01 00:00:00 UTC.\n"
			
			"\nDetails:\n"
			" -t\t\tprint service Timing in ms.\n"
			" -n\t\tprint signer Name (identity).\n"
			" -r\t\tprint publication References (use with -vx).\n"
			" -d\t\tdump detailed information.\n"
			" --log <file>\tdump KSI log into file.\n"
			
			"\nConfiguration:\n"
			" -S <url>\tspecify Signing service URL.\n"
			" -X <url>\tspecify verification (eXtending) service URL.\n"
			" -P <url>\tspecify Publications file URL.\n"
			" --user\t\tuser name.\n"
			" --pass\t\tpassword.\n"
			" -c <num>\tnetwork transfer timeout, after successful Connect.\n"
			" -C <num>\tnetwork Connect timeout (is not supported with tcp client).\n"
			" -V <file>\tuse specified OpenSSL-style trust store file for publications file\n"
			"\t\tverification. Can have multiple values (-V <file 1> -V <file 2>).\n"
			" -W <dir>\tuse specified OpenSSL-style trust store directory for publications\n"
			"\t\tfile verification.\n"
			" -E <mail>\tuse specified publication certificate email.\n"
			" --inc <file>\tuse configuration file containing command-line parameters.\n"
			"\t\tParameter must be written line by line."
			
			"\nHelp:\n"
			" -h --help\tHelp (You are reading it now).\n",
			toolVersion, apiVersion
			);

			fprintf(stderr, "\nDefault service access URL-s:\n"
			"\tTo define default URL-s system variables must be defined.\n"
			"\tFor aggregator define \"KSI_AGGREGATOR\"=\"url=<url> pass=<pass> user=<user>\".\n"
			"\tFor extender define \"KSI_EXTENDER\"=\"url=<url> pass=<pass> user=<user>\".\n"
			"\tOnly <url> part is mandatory. Default <pass> and <user> is \"anon\".\n"
			"\tUsing includes (--inc) or defining urls on command-line will override defaults.\n\n"		
			"\tSigning:		%s\n"
			"\tExtending/Verifying:	%s\n"
			"\tPublications file:	%s\n", (aggre_url ? aggre_url : "Not defined."), (ext_url ? ext_url : "Not defined."), KSI_DEFAULT_URI_PUBLICATIONS_FILE);
			
			fprintf(stderr, "\nSupported hash algorithms (-H, -F):\n\t");
			printSupportedHashAlgorithms();
			fprintf(stderr, "\n");
			//"\tSHA-1, SHA-256 (default), RIPEMD-160, SHA-224, SHA-384, SHA-512, RIPEMD-256, SHA3-244, SHA3-256, SHA3-384, SHA3-512, SM3\n");
}

int main(int argc, char** argv, char **envp) {
	TaskDefinition *taskDefArray[11]={NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	paramSet *set = NULL;
	int retval = EXIT_SUCCESS;
	Task *task = NULL;
	int i;
	
#ifdef _WIN32
#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
#endif	
#endif	

	/*Create parameter set*/
	paramSet_new("{s|sign}*{x|extend}*{p}*{v|verify}*{t}*{r}*{d}*{n}*{h|help}*{o|out}{i}{f}{b}{c}{C}{V}*"
				 "{W}{S}>{X}>{P}>{F}{H}{T}{E}{inc}*{aggre}{htime}{setsystime}"
				 "{user}>{pass}>{log}"
				 ,&set);
	if(set == NULL) goto cleanup;
	
	/*Configure parameter set*/
	paramSet_addControl(set, "{o}{log}", isPathFormOk, isOutputFileContOK, NULL);
	paramSet_addControl(set, "{i}{b}{f}{V}{W}{inc}", isPathFormOk, isInputFileContOK, convert_repairPath);
	paramSet_addControl(set, "{F}", isImprintFormatOK, isImprintContOK, NULL);
	paramSet_addControl(set, "{H}", isHashAlgFormatOK, isHashAlgContOK, NULL);
	paramSet_addControl(set, "{S}{X}{P}", isURLFormatOK, contentIsOK, convert_repairUrl);
	paramSet_addControl(set, "{c}{C}{T}", isIntegerFormatOK, contentIsOK, NULL);
	paramSet_addControl(set, "{E}", isEmailFormatOK, contentIsOK, NULL);
	paramSet_addControl(set, "{user}{pass}", isUserPassFormatOK, contentIsOK, NULL);
	paramSet_addControl(set, "{x}{s}{v}{p}{t}{r}{n}{d}{h}{aggre}{htime}{setsystime}", isFlagFormatOK, contentIsOK, NULL);
	
	/*Define possible tasks*/
	/*						ID							DESC					DEF					MAN		IGNORE				OPTIONAL		FORBIDDEN			NEW OBJ*/
	TaskDefinition_new(signDataFile,			"Sign data file",				"s,f",				"o",	"b,r,i,T",			"H,n,d,t",		"x,p,v,F",			&taskDefArray[0]);
	TaskDefinition_new(signHash,				"Sign hash",					"s,F",				"o",	"b,r,i,H,T",		"n,d,t",		"x,p,v,f",			&taskDefArray[1]);
	TaskDefinition_new(extendTimestamp,			"Extend signature",				"x",				"i,o",	"H,F,f,b,",			"T,n,r,t",		"s,p,v",			&taskDefArray[2]);
	TaskDefinition_new(downloadPublicationsFile,"Download publication file",	"p,o",				"",		"H,F,f,i,T,n,r",	"d,t",			",s,x,v,T",			&taskDefArray[3]);
	TaskDefinition_new(createPublicationString, "Create publication string",	"p,T",				"",		"H,F,f,i,n,r",		"d,t",			"s,x,v,o",			&taskDefArray[4]);
	TaskDefinition_new(verifyTimestamp,			"Verify signature",				"v,i",				"",		"H,T",				"f,F,n,d,r,t,x","s,p",			&taskDefArray[5]);
//	TaskDefinition_new(verifyTimestamp,			"Verify locally",				"v,b,i",			"",		"H,T",				"f,F,n,d,r,t",	"x,s,p",			&taskDefArray[6]);
	TaskDefinition_new(verifyPublicationsFile,	"Verify publications file",		"v,b",				"",		"T,H",				"n,d,r,t",		"x,s,p,i,f,F",		&taskDefArray[6]);
	TaskDefinition_new(getRootH_T,				"Get Aggregator root hash",		"aggre,htime",		"",		"",					"",				"x,s,p,v",			&taskDefArray[7]);
	TaskDefinition_new(setSysTime,				"Set system time",				"aggre,setsystime",	"",		"",					"",				"x,s,p,v",			&taskDefArray[8]);
	
	/*Read parameter set*/
	paramSet_readFromCMD(argc, argv, set, 3);
	
	if(includeParametersFromFile(set, 2) == false) goto cleanup;
	if(includeParametersFromEnvironment(set, envp, 1) == false) goto cleanup;
	if(paramSet_isSetByName(set, "h")) goto cleanup;
	
	if(paramSet_isTypos(set)){
		paramSet_printTypoWarnings(set);
		retval = EXIT_INVALID_CL_PARAMETERS;
		goto cleanup;
	}
	
	
	/*Extract task */
	task = Task_getConsistentTask(taskDefArray, 9, set);
	paramSet_printUnknownParameterWarnings(set);
	if(task == NULL){
		retval = EXIT_INVALID_CL_PARAMETERS;
		goto cleanup;
	}
	if(paramSet_isFormatOK(set) == false){
		paramSet_PrintErrorMessages(set);
		retval = EXIT_INVALID_CL_PARAMETERS;
		goto cleanup;
	}
	
	paramSet_printIgnoredLowerPriorityWarnings(set);
	
	/*DO*/
	if(Task_getID(task) == downloadPublicationsFile || Task_getID(task) == createPublicationString){
		retval=GT_publicationsFileTask(task);
	}
	else if (Task_getID(task) == verifyPublicationsFile){
		retval=GT_verifyTask(task);
	}
	else if (Task_getID(task) == signDataFile || Task_getID(task) == signHash){
		retval=GT_signTask(task);
	}
	else if(Task_getID(task) == extendTimestamp){
		retval=GT_extendTask(task);
	}
	else if(Task_getID(task) == getRootH_T || Task_getID(task) == setSysTime){
		retval=GT_other(task);
	}
	else if(Task_getID(task) == verifyTimestamp){
		retval=GT_verifyTask(task);
	}

cleanup:
	if(paramSet_isSetByName(set, "h")) GT_pritHelp();

	paramSet_free(set);
	for(i=0; i<11;i++)
		TaskDefinition_free(taskDefArray[i]);
	Task_free(task);
	/*TODO*/
//		_CrtDumpMemoryLeaks();
	return retval;
}
