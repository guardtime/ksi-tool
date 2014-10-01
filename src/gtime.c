/* 
 * File:   gtime.c
 * Author: Taavi
 *
 * Created on June 18, 2014, 4:25 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "task_def.h"
#include "gt_task.h"
#include "gt_task_support.h"
#include <ksi/ksi.h>


static void GT_pritHelp(void);

int main(int argc, char** argv) {
	TaskDefinition *taskDefArray[9];
	paramSet *set = NULL;
	Task *task = NULL;
	bool state = false;
	
	/*Create parameter set*/
	paramSet_new("{s}*{x}*{p}*{v}*{t}*{r}*{d}*{n}*{h}*{o}{i}{f}{b}{a}{c}{C}{V}*{W}{S}{X}{P}{F}{l}{H}{T}{E}{user}{pass}", &set);
	if(set == NULL) goto cleanup;
	
	/*Configure parameter set*/
	paramSet_addControl(set, "{o}", isPathFormOk, isOutputFileContOK);
	paramSet_addControl(set, "{i}{b}{f}{V}{W}", isPathFormOk, isInputFileContOK);
	paramSet_addControl(set, "{F}", isImprintFormatOK, contentIsOK);
	paramSet_addControl(set, "{H}", isHashAlgFormatOK, isHashAlgContOK);
	paramSet_addControl(set, "{S}{X}{P}", isURLFormatOK, contentIsOK);
	paramSet_addControl(set, "{c}{C}{T}", isIntegerFormatOK, contentIsOK);
	paramSet_addControl(set, "{E}", isEmailFormatOK, contentIsOK);
	paramSet_addControl(set, "{user}{pass}", isUserPassFormatOK, contentIsOK);

	paramSet_addControl(set, "{x}{s}{v}{p}{t}{r}{n}{l}{d}{h}", isFlagFormatOK, contentIsOK);
	
	
	/*Read parameter set*/
	paramSet_readFromCMD(argc, argv,set);
	if(set == NULL) goto cleanup;
//	paramSet_Print(set); 
	
	if(paramSet_isSetByName(set, "h")){
		GT_pritHelp();
		state = true;
		goto cleanup;
	}
	
	/*Define possible tasks*/
	//						ID							DESC					DEF		MAN		IGNORE			OPTIONAL		FORBIDDEN			NEW OBJ
	TaskDefinition_new(signDataFile,			"Sign data file",				"s|f",	"o",	"b|r|i|T",		"H|n|d|t",		"x|p|v|F",			&taskDefArray[0]);
	TaskDefinition_new(signHash,				"Sign hash",					"s|F",	"o",	"b|r|i",		"n|d|t",		"x|p|v|H|f",		&taskDefArray[1]);
	TaskDefinition_new(extendTimestamp,			"Extend signature",				"x",	"i|o",	"H|F|f",		"T|n|r|t",		"s|p|v",			&taskDefArray[2]);
	TaskDefinition_new(downloadPublicationsFile,"Download publication file",	"p|o",	"",		"",				"d|t",			"s|x|v|T",			&taskDefArray[3]);
	TaskDefinition_new(createPublicationString, "Create publication string",	"p|T",	"",		"",				"d|t",			"s|x|v|o",			&taskDefArray[4]);
	TaskDefinition_new(verifyTimestamp,			"Verify online",				"v|x",	"i",	"",				"f|n|d|r|t",	"s|p|b",			&taskDefArray[5]);
	TaskDefinition_new(verifyTimestamp,			"Verify locally",				"v|b|i","",		"",				"f|n|d|r|t",	"x|s|p",			&taskDefArray[6]);
	TaskDefinition_new(verifyPublicationsFile,	"Verify publications file",		"v|b",	"",		"T|F|H",		"n|d|r|t",		"x|s|p|i|f|Test",	&taskDefArray[7]);
	
	
	/*Extract task */
	paramSet_printUnknownParameterWarnings(set);
	task = Task_getConsistentTask(taskDefArray, 8, set);
	if(task == NULL) goto cleanup;
	if(paramSet_isFormatOK(set) == false) goto cleanup;
	
	/*DO*/
	if(task->id == downloadPublicationsFile || task->id == createPublicationString){
		state=GT_publicationsFileTask(task);
	}
	else if (task->id == verifyPublicationsFile){
		state=GT_verifyTask(task);
	}
	else if (task->id == signDataFile || task->id == signHash){
		state=GT_signTask(task);
	}
	else if(task->id == extendTimestamp){
		state=GT_extendTask(task);
	}
	else if(task->id == verifyTimestamp){
		state=GT_verifyTask(task);
	}

cleanup:
	
	paramSet_free(set);
//	if(!state) GT_pritHelp();

	return state ? (EXIT_SUCCESS) : (EXIT_FAILURE);
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

	fprintf(stderr,
			"\nGuardTime command-line signing tool\n"
			"Usage: <-s|-x|-p|-v> [more options]\n"
			"Where recognized options are:\n"
			" -s		sign data (-n -d -t) \n"
			"   		-f -o sign data file\n"
			"   		-f -H -o sign data file with specific hash algorithm\n"
			"   		-F -o sign hash\n"
			" -x		use online verification (eXtending) service (-n -r -t)\n"
			"   		-i -o extend signature to the nearest publication\n"
			"   		-i -T -o extend signature to specified time\n"
			" -p		download Publications file (-d -t)\n"
			"   		-o download publications file\n"
			"   		-T create publication string\n"
			" -v		verify signature or publications file (-n -r -d -t):\n"
			"   		-x -i (-f) verify signature (and signed document) online\n"
			"   		-b -i (-f) verify signature (and signed document) using specific publications file\n"
			"   		-b verify publication file\n"
			
			"\nInput/output:\n"
			" -f <file>	file to be signed / verified\n"
			" -F <hash>	data hash to be signed / verified. hash Format: <ALG>:<hash in hex>\n"
			" -o <file>	output filename to store signature token or publications file\n"
			" -i <file>	input signature token file to be extended / verified\n"
			" -b <file>	use specified publications file\n"
			" -H <ALG>	hash algorithm used to hash the file to be signed\n"
			" -T <UTC>	specific publication time to extend to (use with -x) as number of seconds since\n"
			"   		1970-01-01 00:00:00 UTC\n"
			
			"\nDetails:\n"
			" -t		print service Timing in ms\n"
			" -n		print signer Name (identity)\n"
			" -r		print publication References (use with -vx)\n"
			" -l		print 'extended Location ID' value\n"
			" -d		dump detailed information\n"
			
			"\nConfiguration:\n"
			" -S <url>	specify Signing service URL\n"
			" -X <url>	specify verification (eXtending) service URL\n"
			" -P <url>	specify Publications file URL\n"
			" -c <num>	network transfer timeout, after successful Connect\n"
			" -C <num>	network Connect timeout.\n"
			" -V <file>	use specified OpenSSL-style trust store file for publications file Verification\n"
			"   		Can have multiple values (-V <file 1> -V <file 2>)\n"
			" -W <dir>	use specified OpenSSL-style trust store directory for publications file verification\n"
			" -E <mail>	use specified publication certificate email\n"
			
			"\nHelp:\n"
			" -h		Help (You are reading it now)\n"

			);

			fprintf(stderr, "\nDefault service access URL-s:\n"
			"\tSigning:		%s\n"
			"\tVerifying:		%s\n"
			"\tPublications file:	%s\n", KSI_DEFAULT_URI_AGGREGATOR, KSI_DEFAULT_URI_EXTENDER, KSI_DEFAULT_URI_PUBLICATIONS_FILE);
			
			fprintf(stderr, "\nSupported hash algorithms (-H, -F):\n\t");
			printSupportedHashAlgorithms();
			fprintf(stderr, "\n");
			//"\tSHA-1, SHA-256 (default), RIPEMD-160, SHA-224, SHA-384, SHA-512, RIPEMD-256, SHA3-244, SHA3-256, SHA3-384, SHA3-512, SM3\n");
}
