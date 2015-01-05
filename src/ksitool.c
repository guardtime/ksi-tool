#include <stdio.h>
#include <stdlib.h>
#include "task_def.h"
#include "gt_task.h"
#include "gt_task_support.h"
#include <ksi/ksi.h>

static bool includeParametersFromFile(paramSet *set){
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
			
			paramSet_readFromFile(fname, set);
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

static bool includeParametersFromEnvironment(paramSet *set, char **envp){
	/*Read command line parameters from system variables*/
	while(*envp!=NULL){
		char tmp[1024];
		char *found = NULL;
        if(strncmp(*envp, "KSI_AGGREGATOR", sizeof("KSI_AGGREGATOR")-1)==0){
			if(!getEnvValue(*envp, "url", tmp, sizeof(tmp))) return false;
			paramSet_appendParameterByName(tmp, "sysvar_aggre_url", set);
			if(!getEnvValue(*envp, "user", tmp, sizeof(tmp))) return false;
			paramSet_appendParameterByName(tmp, "sysvar_aggre_user", set);
			if(!getEnvValue(*envp, "pass", tmp, sizeof(tmp))) return false;
			paramSet_appendParameterByName(tmp, "sysvar_aggre_pass", set);
		}
        else if(strncmp(*envp, "KSI_EXTENDER", sizeof("KSI_EXTENDER")-1)==0){
			if(!getEnvValue(*envp, "url", tmp, sizeof(tmp))) return false;
			paramSet_appendParameterByName(tmp, "sysvar_ext_url", set);
			if(!getEnvValue(*envp, "user", tmp, sizeof(tmp))) return false;
			paramSet_appendParameterByName(tmp, "sysvar_ext_user", set);
			if(!getEnvValue(*envp, "pass", tmp, sizeof(tmp))) return false;
			paramSet_appendParameterByName(tmp, "sysvar_ext_pass", set);
		}
		
        envp++;
    }
	return true;
}

static void printSupportedHashAlgorithms(void){
	int i = 0;
	
	for(i=0; i< KSI_NUMBER_OF_KNOWN_HASHALGS; i++){
		if(KSI_isHashAlgorithmSupported(i)){
			fprintf(stderr,"%s ", KSI_getHashAlgorithmName(i));
		}
	}
}

static void GT_pritHelp(paramSet *set){
	char *ext_url = NULL;
	char *aggre_url = NULL;
	
	paramSet_getStrValueByNameAt(set, "sysvar_ext_url", 0, &ext_url);
	paramSet_getStrValueByNameAt(set, "sysvar_aggre_url", 0, &aggre_url);
	
	fprintf(stderr,
			"\nGuardTime command-line signing tool\n"
			"Usage: <-s|-x|-p|-v> [more options]\n"
			"Where recognized options are:\n"
			" -s		sign data (-n -d -t) \n"
			"   		-s -f -o sign data file\n"
			"   		-s -f -H -o sign data file with specific hash algorithm\n"
			"   		-s -F -o sign hash\n"
			" -x		use online verification (eXtending) service (-n -r -t)\n"
			"   		-x -i -o extend signature to the nearest publication\n"
			"   		-x -i -T -o extend signature to specified time\n"
			" -p		download Publications file (-d -t)\n"
			"   		-p -o download publications file\n"
			"   		-p -T create publication string\n"
			" -v		verify signature or publications file (-n -r -d -t):\n"
			"   		-v -x -i (-f) verify signature (and signed document) online\n"
			"   		-v -b -i (-f) verify signature (and signed document) using specific\n"
			"		publications file\n"
			"   		-v -b verify publication file\n"
			" -aggre		use aggregator for (-n -t):\n"
			"   		-aggre -htime display current aggregation root hash value and time\n"
			"   		-aggre -setsystime set system time from current aggregation\n"
			
			
			"\nInput/output:\n"
			" -f <file>	file to be signed / verified\n"
			" -F <hash>	data hash to be signed / verified. Hash format: <ALG>:<hash in hex>\n"
			" -o <file>	output filename to store signature token or publications file\n"
			" -i <file>	input signature token file to be extended / verified\n"
			" -b <file>	use specified publications file\n"
			" -H <ALG>	hash algorithm used to hash the file to be signed\n"
			" -T <UTC>	specific publication time to extend to (use with -x) as number\n"
			"   		of seconds since 1970-01-01 00:00:00 UTC\n"
			
			"\nDetails:\n"
			" -t		print service Timing in ms\n"
			" -n		print signer Name (identity)\n"
			" -r		print publication References (use with -vx)\n"
			" -d		dump detailed information\n"
			"-log<file>	dump KSI log int file\n"
			
			"\nConfiguration:\n"
			" -S <url>	specify Signing service URL\n"
			" -X <url>	specify verification (eXtending) service URL\n"
			" -P <url>	specify Publications file URL\n"
			" -user		user name\n"
			" -pass		password\n"
			" -c <num>	network transfer timeout, after successful Connect\n"
			" -C <num>	network Connect timeout.\n"
			" -V <file>	use specified OpenSSL-style trust store file for publications file\n"
			"		Verification\n"
			"   		Can have multiple values (-V <file 1> -V <file 2>)\n"
			" -W <dir>	use specified OpenSSL-style trust store directory for publications\n"
			"		file verification\n"
			" -E <mail>	use specified publication certificate email\n"
			" -inc <fn>	use configuration file containing command-line parameters\n"
			
			"\nHelp:\n"
			" -h		Help (You are reading it now)\n"

			);

			fprintf(stderr, "\nDefault service access URL-s:\n"
			"\tSigning:		%s\n"
			"\tVerifying:		%s\n"
			"\tPublications file:	%s\n", (aggre_url ? aggre_url : "Define system variable \"KSI_AGGREGATOR\"=\"url=<url> pass=<pass> user=<user>\"."), (ext_url ? ext_url : "Define system variable \"KSI_EXTENDER\"=\"url=<url> pass=<pass> user=<user>\"."), KSI_DEFAULT_URI_PUBLICATIONS_FILE);
			
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
//	TODO
//	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
//	Send all reports to STDOUT
//	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
//	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
//	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
//	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
//	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
//	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
#endif	
#endif	

	/*Create parameter set*/
	paramSet_new("{s}*{x}*{p}*{v}*{t}*{r}*{d}*{n}*{h}*{o}{i}{f}{b}{a}{c}{C}{V}*"
				 "{W}{S}{X}{P}{F}{H}{T}{E}{inc}*{aggre}{htime}{setsystime}"
				 "{user}{pass}{log}"
				 "{sysvar_aggre_url}{sysvar_aggre_pass}{sysvar_aggre_user}"
				 "{sysvar_ext_url}{sysvar_ext_pass}{sysvar_ext_user}", &set);
	if(set == NULL) goto cleanup;
	
	/*Configure parameter set*/
	paramSet_addControl(set, "{o}{log}", isPathFormOk, isOutputFileContOK);
	paramSet_addControl(set, "{i}{b}{f}{V}{W}{inc}", isPathFormOk, isInputFileContOK);
	paramSet_addControl(set, "{F}", isImprintFormatOK, isImprintContOK);
	paramSet_addControl(set, "{H}", isHashAlgFormatOK, isHashAlgContOK);
	paramSet_addControl(set, "{S}{X}{P}{sysvar_aggre_url}{sysvar_ext_url}", isURLFormatOK, contentIsOK);
	paramSet_addControl(set, "{c}{C}{T}", isIntegerFormatOK, contentIsOK);
	paramSet_addControl(set, "{E}", isEmailFormatOK, contentIsOK);
	paramSet_addControl(set, "{user}{pass}{sysvar_ext_pass}{sysvar_ext_user}{sysvar_aggre_pass}{sysvar_aggre_user}", isUserPassFormatOK, contentIsOK);
	paramSet_addControl(set, "{x}{s}{v}{p}{t}{r}{n}{d}{h}{aggre}{htime}{setsystime}", isFlagFormatOK, contentIsOK);
	
	/*Define possible tasks*/
	/*						ID							DESC					DEF						MAN			IGNORE				OPTIONAL		FORBIDDEN			NEW OBJ*/
	TaskDefinition_new(signDataFile,			"Sign data file",				"-s -f",				"-o",		"-b-r-i-T",			"-H-n-d-t",		"-x-p-v-F",			&taskDefArray[0]);
	TaskDefinition_new(signHash,				"Sign hash",					"-s -F",				"-o",		"-b-r-i-H-T",		"-n-d-t",		"-x-p-v-f",			&taskDefArray[1]);
	TaskDefinition_new(extendTimestamp,			"Extend signature",				"-x",					"-i -o",	"-H-F-f-b-",		"-T-n-r-t",		"-s-p-v",			&taskDefArray[2]);
	TaskDefinition_new(downloadPublicationsFile,"Download publication file",	"-p -o",				"",			"-H-F-f-i-T-n-r",	"-d-t",			"-s-x-v-T",			&taskDefArray[3]);
	TaskDefinition_new(createPublicationString, "Create publication string",	"-p -T",				"",			"-H-F-f-i-n-r",		"-d-t",			"-s-x-v-o",			&taskDefArray[4]);
	TaskDefinition_new(verifyTimestamp,			"Verify online",				"-v -x",				"-i",		"-F-H-T",			"-f-n-d-r-t",	"-s-p-b",			&taskDefArray[5]);
	TaskDefinition_new(verifyTimestamp,			"Verify locally",				"-v -b -i",				"",			"-F-H-T",			"-f-n-d-r-t",	"-x-s-p",			&taskDefArray[6]);
	TaskDefinition_new(verifyPublicationsFile,	"Verify publications file",		"-v -b",				"",			"-T-F-H",			"-n-d-r-t",		"-x-s-p-i-f",		&taskDefArray[7]);
	TaskDefinition_new(getRootH_T,				"Get Aggregator root hash",		"-aggre -htime",		"",			"",					"",				"-x-s-p-v",			&taskDefArray[8]);
	TaskDefinition_new(setSysTime,				"Set system time",				"-aggre -setsystime",	"",			"",					"",				"-x-s-p-v",			&taskDefArray[9]);
	
	/*Read parameter set*/
	paramSet_readFromCMD(argc, argv,set);
	if(set == NULL) goto cleanup;
	
	if(includeParametersFromFile(set) == false) goto cleanup;
	if(includeParametersFromEnvironment(set, envp) == false) goto cleanup;
	if(paramSet_isSetByName(set, "h")) goto cleanup;

	/*Extract task */
	task = Task_getConsistentTask(taskDefArray, 10, set);
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
	
	/*DO*/
	if(task->id == downloadPublicationsFile || task->id == createPublicationString){
		retval=GT_publicationsFileTask(task);
	}
	else if (task->id == verifyPublicationsFile){
		retval=GT_verifyTask(task);
	}
	else if (task->id == signDataFile || task->id == signHash){
		retval=GT_signTask(task);
	}
	else if(task->id == extendTimestamp){
		retval=GT_extendTask(task);
	}
	else if(task->id == getRootH_T || task->id == setSysTime){
		retval=GT_other(task);
	}
	else if(task->id == verifyTimestamp){
		retval=GT_verifyTask(task);
	}

cleanup:
	if(paramSet_isSetByName(set, "h")) GT_pritHelp(set);

paramSet_free(set);
	for(i=0; i<11;i++)
		TaskDefinition_free(taskDefArray[i]);
	Task_free(task);
//	_CrtDumpMemoryLeaks();
	return retval;
}
