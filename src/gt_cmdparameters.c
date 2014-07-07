#include "gt_cmdparameters.h"
#include "getopt.h"
#include <stdio.h>		//input output
#include <string.h>		
#include <stdlib.h>		//malloc, random, int ja strn muundused
#include <ctype.h>

static GT_CmdParameters cmdParameters;
static GT_Tasks taskTODO = noTask;

/**Functions for analysing and checking if command line parameters are valid**/

static bool isValidHex(const char *hexStrn, char *invalidchar){
    int i=0;
    char C;
    while(C=hexStrn[i++]){
        if(!isxdigit(C)){
            printf("\n invalid %c\n", C);
            *invalidchar=C;
            return false;
            }
        }
    return true;
    }

static bool doFileExists(const char* path){
    FILE *file;
    if (file = fopen(path, "r"))
        {
        fclose(file);
        return true;
        }
    return false;
    }

static bool analyseInputFile(const char* path){
    if(path == NULL){
        fprintf(stderr, "Error: File name is not inserted. It is nullptr!\n");
        return false;
        }
    if(doFileExists(path) == false){
        fprintf(stderr, "Error: File \"%s\" dose not exist!\n", path);
        return false;
        }
    else 
        return true;
    }

static bool analyseOutputFile(const char* path){
    if(path == NULL){
        fprintf(stderr, "Error: File name is not inserted. It is nullptr!\n");
        return false;
        }
    else
        return true;
    }

static bool analyseInputHash(void){
    char invalidChar=0;
    
        if(cmdParameters.hashAlgName_F == NULL){
            fprintf(stderr, "Error: Hash Algorithm name is not inserted. It is nullptr!\n");
            return false;
            }
        else if(strlen(cmdParameters.hashAlgName_F) == 0){
            fprintf(stderr, "Error: Hash Algorithm name \"\" is not valid\n");
            return false;
            }
        else if(cmdParameters.inputHashStrn == NULL){
            fprintf(stderr, "Error: Hash is not inserted. It is nullptr!\n");
            return false;
            }
        else if(strlen(cmdParameters.inputHashStrn) == 0){
            fprintf(stderr, "Error: Hash \"\" is not valid\n");
            return false;
            }
        
        if(!isValidHex(cmdParameters.inputHashStrn, &invalidChar)){
            fprintf(stderr, "Error: Hash contains invalid character: %c\n", invalidChar);
            return false;
            }
    return true;
    }



/****************Functions for parsing command line parameters****************/

static void getHashAndAlgStrings(const char *instrn, char **strnAlgName, char **strnHash){
    char *colon;
    size_t algLen=0;
    size_t hahsLen=0;
    char *temp_strnAlg = NULL;
    char *temp_strnHash = NULL;
    *strnAlgName = NULL;
    *strnHash = NULL;
    
    if(instrn==NULL)
        return;
    
    colon = strchr(instrn, ':');
    if (colon != NULL) {
            algLen = (colon-instrn)/sizeof(char);
            hahsLen = strlen(instrn)-algLen-1;
            temp_strnAlg = calloc(algLen+1, sizeof(char));
            temp_strnHash = calloc(hahsLen+1, sizeof(char));
            memcpy(temp_strnAlg, instrn, algLen);
            temp_strnAlg[algLen]=0;
            memcpy(temp_strnHash, colon+1, hahsLen);
            temp_strnHash[hahsLen]=0;
            }
    *strnAlgName = temp_strnAlg;
    *strnHash =temp_strnHash;
    //printf("Alg %s\nHash %s\n", *strnAlgName, *strnHash);
    }


//Just reads raw parameters and arguments from command line and inserts the data
// into cmdParameters
static void GT_readCommandLineParameters(int argc, char **argv){
    int c =0;
    
    while(1){
        c = getopt(argc, argv, "sxpvtrdo:i:f:b:a:hc:C:V:W:S:X:P:F:lH:n");
        		if (c == -1) {
			break;
		}
        
       // printf(">> %c (%i)\n", c,c);
        switch (c) {
            case 'h': cmdParameters.h = true; break;
            case 's': cmdParameters.s = true; break;
            case 'x': cmdParameters.x = true; break;
            case 'p': cmdParameters.p = true; break;
            case 'v': cmdParameters.v = true; break;
			
            case 't': cmdParameters.t = true; break;
            case 'd': cmdParameters.d = true; break;
            case 'r': cmdParameters.r = true; break;
            
            case 'l': cmdParameters.l = true;  break;
            case 'n': cmdParameters.n = true; break;
         //   case 'a': cmdParameters.a = atoi(optarg); break;
	
            case 'i': 
                cmdParameters.inSigFileName = optarg; 
                cmdParameters.i = true;
                break;
            case 'f':
                cmdParameters.inDataFileName = optarg;
                cmdParameters.f = true;
                break;
            case 'b':
                cmdParameters.inPubFileName = optarg;
                cmdParameters.b = true;
                break;
            case 'o':
                cmdParameters.outPubFileName = optarg;
                cmdParameters.outSigFileName = optarg;
                cmdParameters.o = true;
		break;
            case 'c': 
                cmdParameters.networkTransferTimeout = atoi(optarg); 
                cmdParameters.c = true;
                break;
            case 'C':
                cmdParameters.networkConnectionTimeout = atoi(optarg);
                cmdParameters.C = true;
                break;
            case 'S':
                cmdParameters.signingService_url = optarg;
                cmdParameters.S = true;
                break;
            case 'X': 
                cmdParameters.verificationService_url = optarg;
                cmdParameters.X = true;
                break;
            case 'P':
                cmdParameters.publicationsFile_url = optarg;
                cmdParameters.P =true;
                break;
            case 'V':
                cmdParameters.openSSLTruststoreFileName = optarg;
                cmdParameters.V = true;
                break;
            case 'W':
                cmdParameters.openSSLTrustStoreDirName = optarg;
                cmdParameters.W = true;
                break;
            
           case 'F':
               getHashAndAlgStrings(optarg, &cmdParameters.hashAlgName_F, &cmdParameters.inputHashStrn);
               cmdParameters.F = true;
               break;
           case 'H':
               cmdParameters.hashAlgName_H = optarg;
               cmdParameters.H = true;
               break;
		}
   }
    
    //printf("eof\n");

}


//Extracts the task and its status into taskTODO. The task can be ok ( all 
//important parameters are present) or invalid. The content of parameters is not checked. 
static void GT_extractTaskTODO(void){
    if(cmdParameters.h){
        taskTODO = showHelp;
        }
    //No multiple tasks allowed ((p or s) and (v or x)) and (p and v)
    else if(((cmdParameters.p || cmdParameters.s) && (cmdParameters.v || cmdParameters.x)) || (cmdParameters.p && cmdParameters.s)){
        taskTODO = invalid_multipleTasks;
        }
    //Download publications file
    else if(cmdParameters.p){
        if(cmdParameters.o == true)
            taskTODO = downloadPublicationsFile;
        else
            taskTODO = invalid_p;
        }
    //Sign data or hash    
    else if(cmdParameters.s){
        if(cmdParameters.o && cmdParameters.F)
            taskTODO = signHash;
        else if(cmdParameters.o && cmdParameters.f)
            taskTODO = signDataFile;
        else
            taskTODO = invalid_s;
        }
    //Verify locally or online
    else if(cmdParameters.v ){
        /*
        if(cmdParameters.b && cmdParameters.i)
            taskTODO = verifyTimestamp_use_pubfile;
        else if(cmdParameters.b && cmdParameters.f && cmdParameters.i)
            taskTODO = verifyTimestamp_and_file_use_pubfile;
        else if(cmdParameters.x && cmdParameters.f && cmdParameters.i)
            taskTODO = verifyTimestamp_and_file_online;
        else if(cmdParameters.x && cmdParameters.i)
            taskTODO = verifyTimestamp_online;*/
        if(cmdParameters.x && cmdParameters.i){
            taskTODO = verifyTimestamp_online;
            }
        else if(cmdParameters.i){
            taskTODO = verifyTimestamp_locally;
            }
        else if(cmdParameters.b)
            taskTODO = verifyPublicationsFile;
        else
            taskTODO = invalid_v;
        }
    //Extend timestamps
    else if(cmdParameters.x && !cmdParameters.v){
        if(cmdParameters.i && cmdParameters.o)
            taskTODO = extendTimestamp;
        else
            taskTODO = invalid_x;
        }
    //There is no task -p, -s, -v, x,    
    else
        taskTODO = noTask;
    }

//Prints an error message if something is wrong with the FORM of the parameters.
//Used after GT_extractTaskTODO().
static void GT_printTaskErrorMessage(void){
    switch(taskTODO){
        case invalid_multipleTasks:
            fprintf(stderr, "Error: You can't use multiple tasks together: %s%s%s%s\n",
                    cmdParameters.p ? "-p " : "",
                    cmdParameters.s ? "-s " : "",
                    cmdParameters.v ? "-v " : "",
                    cmdParameters.x ? "-x " : ""
                    );
        break;
        case invalid_p:
            fprintf(stderr, "Error: Using -p you have to define missing parameter -o <fn>\n");
        break;
        case invalid_s:
            fprintf(stderr, "Error: Using -s you have to define missing parameter(s) %s%s\n",
                    !(cmdParameters.o) ? "-o <fn> " : "",
                    !(cmdParameters.f && cmdParameters.F) ? "-f <fn> or -F<hash>" : ""
                    );
        break;
        case invalid_v:
            fprintf(stderr, "Error: Using -v you have to define missing parameter(s) %s%s\n",
                    !(cmdParameters.i) ? "-i <fn> " : "",
                    !(cmdParameters.b && cmdParameters.x) ? "-b <fn> or -x" : ""
                    );
        break;
        case invalid_x:
            fprintf(stderr, "Error: Using -x you have to define missing parameter(s) %s%s\n",
                    !(cmdParameters.o) ? "-o <fn> " : "",
                    !(cmdParameters.i) ? "-i <fn> " : ""
                    );
        break;
        case noTask:
            fprintf(stderr, "Error: The task is not defined. Use parameters -s or -x or -v or -p to define one. \n Use -h parameter for help.\n");
        break;
            
        }
    }

//Controls the parameters and prints error messages. Checks if input files exists
//and input strings are not nullptrs. The content of the files is not checked.
//Return true if Parameters are correct, false otherwise.
static bool GT_controlParameters(void){
    if(taskTODO == verifyPublicationsFile)
        {
        if(analyseInputFile(cmdParameters.inPubFileName)) return true;
        else return false;
        }
    else if(taskTODO == downloadPublicationsFile){
        return true;
        }
    else if(taskTODO == signDataFile){
        if(analyseInputFile(cmdParameters.inDataFileName) && analyseOutputFile(cmdParameters.outSigFileName)) return true;
        else return false;
        }
    else if(taskTODO == signHash){
        if(analyseInputHash() && analyseOutputFile(cmdParameters.outSigFileName)) return true;
        else return false;
        }
    else if(taskTODO == extendTimestamp){
         if(analyseInputFile(cmdParameters.inSigFileName) && analyseOutputFile(cmdParameters.outSigFileName)) return true;
         else return false;
        }
    else if ((taskTODO == verifyTimestamp_locally) || (taskTODO == verifyTimestamp_online)){
        if((taskTODO == verifyTimestamp_locally) && analyseInputFile(cmdParameters.inSigFileName) && analyseInputFile(cmdParameters.inPubFileName)) goto extra_check;
        else if ((taskTODO == verifyTimestamp_online) && analyseInputFile(cmdParameters.inSigFileName)) goto extra_check;
        else return false;
        
        extra_check:
        if(cmdParameters.f && !analyseInputFile(cmdParameters.inDataFileName)){
            printf("Warning: Ignoring parameter -f\n");
            cmdParameters.f = false;
            }
        return true;
        }    
    else if(taskTODO == showHelp){
        return true;
        }
    else{
        return false;
        }
    printf("ok\n");
}

//Visualize all extracted parameters and their values.
static void GT_printParameters(void){
    printf("Parameters and arguments transfered from command line:\n");
    if(cmdParameters.o == true) printf("-o %s\n", cmdParameters.outPubFileName);
    if(cmdParameters.i == true) printf("-i %s\n", cmdParameters.inSigFileName);
    if(cmdParameters.b == true) printf("-b %s\n", cmdParameters.inPubFileName);
    if(cmdParameters.f == true) printf("-f %s\n", cmdParameters.inDataFileName);
    if(cmdParameters.V == true) printf("-V %s\n", cmdParameters.openSSLTruststoreFileName);
    if(cmdParameters.W == true) printf("-W %s\n", cmdParameters.openSSLTrustStoreDirName);
    if(cmdParameters.F == true) printf("-F Alg: %s and Hash: %s\n", cmdParameters.hashAlgName_F, cmdParameters.inputHashStrn);
    if(cmdParameters.H == true) printf("-H %s\n", cmdParameters.hashAlgName_H);
    if(cmdParameters.S == true) printf("-S %s\n", cmdParameters.signingService_url);
    if(cmdParameters.X == true) printf("-X %s\n", cmdParameters.verificationService_url);
    if(cmdParameters.P == true) printf("-P %s\n", cmdParameters.publicationsFile_url);
    if(cmdParameters.c == true) printf("-c %i\n", cmdParameters.networkTransferTimeout);
    if(cmdParameters.C == true) printf("-C %i\n", cmdParameters.networkConnectionTimeout);

    if(cmdParameters.x == true) printf("-x\n");
    if(cmdParameters.s == true) printf("-s\n");
    if(cmdParameters.v == true) printf("-v\n");
    if(cmdParameters.p == true) printf("-p\n");

    if(cmdParameters.t == true) printf("-t\n");
    if(cmdParameters.r == true) printf("-r\n");
    if(cmdParameters.n == true) printf("-n\n");
    if(cmdParameters.l == true) printf("-l\n");
    if(cmdParameters.d == true) printf("-d\n");
    if(cmdParameters.h == true) printf("-h\n");
    }

/*********************************User Functions******************************/
void GT_pritHelp(void){
	fprintf(stderr,
			"\nGuardTime command-line signing tool, using API\n"
			"Usage: <-s|-x|-p|-v> [more options]\n"
			"Where recognized options are:\n"
			" -s		Sign data (The result is automatically verified)\n"
			" -S <url>	specify Signing Service URL\n"
			" -x		use online verification (eXtending) service\n"
			" -X <url>	specify verification (eXtending) service URL\n"
			" -p		download Publications file (The result is automatically verified)\n"
			" -P <url>	specify Publications file URL\n"
			" -v		Verify signature token (-i <ts>); online verify with -x;\n"
			" -t		include service Timing\n"
			" -n		print signer Name (identity)\n"
			" -r		print publication References (use with -vx)\n"			
			" -l		print 'extended Location ID' value\n"
			" -d		Dump detailed information\n"
			" -f <fn>	File to be signed / verified\n"
			" -H <ALG>	Hash algorithm used to hash the file to be signed\n"
			" -F <hash>	data hash to be signed / verified. hash Format: <ALG>:<hash in hex>\n"
			" -i <fn>	Input signature token file to be extended / verified\n"
			" -o <fn>	Output filename to store signature token or publications file\n"
			" -b <fn>	use specified BBublications file\n"
			" -V <fn>	use specified OpenSSL-style truststore file for publications file Verification\n"
			" -W <dir>	use specified OpenSSL-style truststore directory for publications file WWerification\n"
			" -c <num>	network transfer timeout, after successful Connect\n"
			" -C <num>	network Connect timeout.\n"
			" -h		Help (You are reading it now)\n"
			"		- instead of filename is stdin/stdout stream\n"
             
);
	
	fprintf(stderr, "\nDefault service access URL-s:\n"
			"\tSigning:      %s\n"
			"\tVerifying:         %s\n"
			"\tPublications file: %s\n", DEFAULT_S_URL, DEFAULT_X_URL, DEFAULT_P_URL);
	fprintf(stderr, "\nSupported hash algorithms (-H, -F):\n"
			"\tSHA-1, SHA-256 (default), RIPEMD-160, SHA-224, SHA-384, SHA-512, RIPEMD-256, SHA3-244, SHA3-256, SHA3-384, SHA3-512, SM3\n");
}

bool GT_parseCommandline(int argc, char **argv){
    GT_readCommandLineParameters(argc, argv);
    GT_extractTaskTODO();
   //GT_printParameters();
    GT_printTaskErrorMessage();
    if(GT_controlParameters()){
        return true;
        }
    else{
        //GT_pritHelp();
        return false;
        }
    }

GT_CmdParameters GT_getCMDParam(void){
    return cmdParameters;
    }

GT_Tasks GT_getTask(void){
    return taskTODO;
    }

/*
 Taskid::
"### Publications file download"                    -p -o <pubfile_out> -P <purl> // KUI P on vigane, siis saab warningu ja võetakse default
"### Verifying publications file"                   -v -b <pubfile_in>

"### Signing"                                       -s -f <datafile_in> -o <sigfail_out> -S <surl>  //KUI S on vigan, saab warningu ja võetakse default
"### Using RIPEMD160"                               -s -F <alg>:<hahs> -o <sigfile_out> -S <surl>

"### Extending timestamp"                           -x -i <sigfile_in> -o <sigfile_out> -X <xurl>   //Kui X on vigane, saab warningu ja võetakse default

"### Verifying freshly created signature token"     -v -b <pubfile_in> -i <sigfail_in>
"### Verifying extended timestamp"                  -v -b <pubfile_in> -i <sigfile_in>
"### Verifying old timestamp"                       -v -b <pubfile_in> -i <sigfile_in> -f <datafile_in>
"### Online verifying old timestamp"                -vx -b <pubfile_in> -i <sigfile_in> -f <datafile_in> -X <xurl>
"### Online verifying extended timestamp"           -vx -b <pubfile_in> -i <sigfile_in> -X <xurl>
*/





