#include <stdio.h>		//input output
#include <string.h>		
#include <stdlib.h>		//malloc, random, int ja strn 
#include <ksi/ksi.h>
#include "getopt.h"
#include "gt_cmd_parameters.h"
#include "gt_cmd_control.h"

//Static variable that contains all valid command-line prameters
static GT_CmdParameters cmdParameters;

/**
 * A struct containing a flag and its argument.
 * -flag <argument>
 * If flag is not set or has no argument, arg must be NULL
 */
struct st_param {
    bool flag;
    char *arg;
};

/**
 * A RAW struct containing all the command line parameters.
 * Used for reading all raw command line parameters.
 */
struct raw_parameters {
    struct st_param o; //output file  
    struct st_param i; //input signature  file  
    struct st_param b; //publications file in  
    struct st_param f; //input data file  
    struct st_param V; //OpenSSL-style truststore file  
    struct st_param W; //OpenSSL-style truststore destroy  
    struct st_param F; //Input data hash  
    struct st_param H; //Different hashAlg  
    struct st_param S; //signing service url  
    struct st_param X; //extending service url  
    struct st_param P; //publications file url  
    struct st_param c; //networkTransferTimeout  
    struct st_param C; //networkConnectionTimeout  

    struct st_param x; //task extending
    struct st_param s; //task signing
    struct st_param v; //task verification
    struct st_param p; //task download Publications File

    struct st_param t; //print timing info
    struct st_param r; //print publication reference
    struct st_param n; //print signer name
    struct st_param l; //print 'extended Location ID' value
    struct st_param d; //Dump detailed information
    struct st_param h; //print help
};

/****************Functions for parsing command line parameters****************/

/**
 * Reads RAW command line parameters. See struct raw_parameters.
 * 
 * @param[in] argc Count of command line arguments.
 * @param[in] argv Pointer to the list of all command line parameters strings.
 * @param[out] rawParam Pointer to the receiving pointer to struct raw_parameters.
 */
static void readRawCmdParam(int argc, char **argv, struct raw_parameters **rawParam)
{
    int c = 0;
    struct raw_parameters *rawParam_tmp = (struct raw_parameters*) calloc(1, sizeof (struct raw_parameters));

    while (1) {
        c = getopt(argc, argv, "sxpvtrdo:i:f:b:a:hc:C:V:W:S:X:P:F:lH:n");
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'h':
            rawParam_tmp->h.flag = true;
            rawParam_tmp->h.arg = NULL;
            break;
        case 's':
            rawParam_tmp->s.flag = true;
            rawParam_tmp->s.arg = NULL;
            break;
        case 'x':
            rawParam_tmp->x.flag = true;
            rawParam_tmp->x.arg = NULL;
            break;
        case 'p':
            rawParam_tmp->p.flag = true;
            rawParam_tmp->p.arg = NULL;
            break;
        case 'v':
            rawParam_tmp->v.flag = true;
            rawParam_tmp->v.arg = NULL;
            break;
        case 't':
            rawParam_tmp->t.flag = true;
            rawParam_tmp->t.arg = NULL;
            break;
        case 'd':
            rawParam_tmp->d.flag = true;
            rawParam_tmp->d.arg = NULL;
            break;
        case 'r':
            rawParam_tmp->r.flag = true;
            rawParam_tmp->r.arg = NULL;
            break;
        case 'l':
            rawParam_tmp->l.flag = true;
            rawParam_tmp->l.arg = NULL;
            break;
        case 'n':
            rawParam_tmp->n.flag = true;
            rawParam_tmp->n.arg = NULL;
            break;
            //   case 'a': rawParameters.a = atoi(optarg); break;
        case 'i':
            rawParam_tmp->i.flag = true;
            rawParam_tmp->i.arg = optarg;
            break;
        case 'f':
            rawParam_tmp->f.flag = true;
            rawParam_tmp->f.arg = optarg;
            break;
        case 'b':
            rawParam_tmp->b.flag = true;
            rawParam_tmp->b.arg = optarg;
            break;
        case 'o':
            rawParam_tmp->o.flag = true;
            rawParam_tmp->o.arg = optarg;
            break;
        case 'c':
            rawParam_tmp->c.flag = true;
            rawParam_tmp->c.arg = optarg;
            break;
        case 'C':
            rawParam_tmp->C.flag = true;
            rawParam_tmp->C.arg = optarg;
            break;
        case 'S':
            rawParam_tmp->S.flag = true;
            rawParam_tmp->S.arg = optarg;
            break;
        case 'X':
            rawParam_tmp->X.flag = true;
            rawParam_tmp->X.arg = optarg;
            break;
        case 'P':
            rawParam_tmp->P.flag = true;
            rawParam_tmp->P.arg = optarg;
            break;
        case 'V':
            rawParam_tmp->V.flag = true;
            rawParam_tmp->V.arg = optarg;
            break;
        case 'W':
            rawParam_tmp->W.flag = true;
            rawParam_tmp->W.arg = optarg;
            break;

        case 'F':
            rawParam_tmp->F.flag = true;
            rawParam_tmp->F.arg = optarg;
            break;
        case 'H':
            rawParam_tmp->H.flag = true;
            rawParam_tmp->H.arg = optarg;
            break;
        }
    }
    *rawParam = rawParam_tmp;
}

/**
 * Extracts hash algorithm name and hash value from string 
 * formatted as <algorithm>:<hash value in hex> .
 * 
 * @param[in] instrn Input string.
 * @param[out] strnAlgName Pointer to the receiving pointer to string containing hash algorithm name.  
 * @param[out] strnHash Pointer to the receiving pointer to string containing hash.
 * 
 * @note strnAlgName and strnHash belongs to the caller and must be freed manually.
 */
static void getHashAndAlgStrings(const char *instrn, char **strnAlgName, char **strnHash)
{
    char *colon;
    size_t algLen = 0;
    size_t hahsLen = 0;
    char *temp_strnAlg = NULL;
    char *temp_strnHash = NULL;
    *strnAlgName = NULL;
    *strnHash = NULL;

    if (instrn == NULL)
        return;

    colon = strchr(instrn, ':');
    if (colon != NULL) {
        algLen = (colon - instrn) / sizeof (char);
        hahsLen = strlen(instrn) - algLen - 1;
        temp_strnAlg = calloc(algLen + 1, sizeof (char));
        temp_strnHash = calloc(hahsLen + 1, sizeof (char));
        memcpy(temp_strnAlg, instrn, algLen);
        temp_strnAlg[algLen] = 0;
        memcpy(temp_strnHash, colon + 1, hahsLen);
        temp_strnHash[hahsLen] = 0;
    }
    *strnAlgName = temp_strnAlg;
    *strnHash = temp_strnHash;
    //printf("Alg %s\nHash %s\n", *strnAlgName, *strnHash);
}

//Copies a flag_ from raw (struct raw_parameters) to param (GT_CmdParameters))
#define copyFlag(param,raw,flag_) param.flag_ = raw.flag_.flag

/**
 * Extracts raw string data from command line buffer (raw) and inserts into param. 
 * Takes a control function name (control) that is used to control the form of the parameter.
 * If control fails an error message is printed, flag_ is disabled and return false is called.
 * 
 * @param[in] param Destination variable type GT_CmdParameters.
 * @param[in] raw Source variable type struct raw_parameters.
 * @param[in] flag_ Name of parameter (member of data type).
 * @param[in] cmd_arg Argument name that correspond to flag_.
 * @param[in] control Name of a form controlling function.
 */
#define SET_STRN_PARAM(param,raw,flag_, cmd_arg, control) \
    { \
    PARAM_RES _p_res = PARAM_UNKNOWN_ERROR; \
    if (raw.flag_.flag){ \
        _p_res = control(raw.flag_.arg); \
        if (_p_res == PARAM_OK){ \
            param.cmd_arg = raw.flag_.arg; \
            param.flag_ = true; \
            } \
        else{ \
            param.cmd_arg = NULL; \
            fprintf(stderr, "Error: Invalid parameter -%s '%s'format. %s\n", #flag_,raw.flag_.arg, getFormatErrorString(_p_res)); \
            param.flag_ = false; \
            return false; \
            } \
        } \
    }
/**
 * Extracts raw integer data from command line buffer (raw) and inserts into param. 
 * Takes a control function name (control) that is used to control the form of the parameter.
 * If control fails an error message is printed, flag_ is disabled and return false is called.
 * 
 * @param[in] param Destination variable type GT_CmdParameters.
 * @param[in] raw Source variable type struct raw_parameters.
 * @param[in] flag_ Name of parameter (member of data type).
 * @param[in] cmd_arg Argument name that correspond to flag_.
 * @param[in] control Name of a form controlling function.
 */
#define SET_INT_PARAM(param,raw,flag_,cmd_arg, control) \
   { \
    PARAM_RES _p_res = PARAM_UNKNOWN_ERROR; \
    if (raw.flag_.flag){ \
        _p_res = control(raw.flag_.arg); \
        if (_p_res == PARAM_OK){ \
            param.cmd_arg = atoi(raw.flag_.arg); \
            param.flag_ = true; \
            } \
        else{ \
            param.cmd_arg = 0; \
            fprintf(stderr, "Error: Invalid parameter -%s '%s' format. %s\n", #flag_,raw.flag_.arg, getFormatErrorString(_p_res)); \
            param.flag_ = false; \
            return false; \
            } \
        } \
    }

/**
 * Reads command line parameters and controls the format. At first raw parameters  
 * (see readRawCmdParam) are extracted and controlled. If error occurs an error message is printed.
 * The result is inserted into static variable cmdParameters.
 * 
 * @param[in] argc Count of command line arguments.
 * @param[in] argv Pointer to the list of all command line parameters strings.
 * @return Returns true if the form of the parameters is OK. False otherwise.
 */
static bool readCmdParam(int argc, char **argv)
{
    struct raw_parameters *rawParam = NULL;
    readRawCmdParam(argc, argv, &rawParam);
    copyFlag(cmdParameters, (*rawParam), x);
    copyFlag(cmdParameters, (*rawParam), s);
    copyFlag(cmdParameters, (*rawParam), v);
    copyFlag(cmdParameters, (*rawParam), p);
    copyFlag(cmdParameters, (*rawParam), t);
    copyFlag(cmdParameters, (*rawParam), r);
    copyFlag(cmdParameters, (*rawParam), n);
    copyFlag(cmdParameters, (*rawParam), l);
    copyFlag(cmdParameters, (*rawParam), d);
    copyFlag(cmdParameters, (*rawParam), h);
    SET_STRN_PARAM(cmdParameters, (*rawParam), f, inDataFileName, isPathFormOk);
    SET_STRN_PARAM(cmdParameters, (*rawParam), b, inPubFileName, isPathFormOk);
    SET_STRN_PARAM(cmdParameters, (*rawParam), i, inSigFileName, isPathFormOk);
    SET_STRN_PARAM(cmdParameters, (*rawParam), o, outPubFileName, isPathFormOk);
    SET_STRN_PARAM(cmdParameters, (*rawParam), o, outSigFileName, isPathFormOk);
    SET_STRN_PARAM(cmdParameters, (*rawParam), W, openSSLTrustStoreDirName, isPathFormOk);
    SET_STRN_PARAM(cmdParameters, (*rawParam), V, openSSLTruststoreFileName, isPathFormOk);
    SET_STRN_PARAM(cmdParameters, (*rawParam), P, publicationsFile_url, isURLFormatOK);
    SET_STRN_PARAM(cmdParameters, (*rawParam), S, signingService_url, isURLFormatOK);
    SET_STRN_PARAM(cmdParameters, (*rawParam), X, verificationService_url, isURLFormatOK);
    SET_INT_PARAM(cmdParameters, (*rawParam), C, networkConnectionTimeout, isIntegerFormatOK);
    SET_INT_PARAM(cmdParameters, (*rawParam), c, networkTransferTimeout, isIntegerFormatOK);
    SET_STRN_PARAM(cmdParameters, (*rawParam), H, hashAlgName_H, isHashAlgFormatOK);

    if (rawParam->F.flag) {
        char *hashAlg;
        char *hash;
        PARAM_RES _p_res_hash = PARAM_UNKNOWN_ERROR;
        PARAM_RES _p_res_alg = PARAM_UNKNOWN_ERROR;
        getHashAndAlgStrings(rawParam->F.arg, &hashAlg, &hash);

        _p_res_hash = isHexFormatOK(hash);
        _p_res_alg = isHashAlgFormatOK(hashAlg);

        if ((_p_res_hash == PARAM_OK) && (_p_res_alg == PARAM_OK)) {
            cmdParameters.inputHashStrn = hash;
            cmdParameters.hashAlgName_F = hashAlg;
            cmdParameters.F = true;
        } else {
            cmdParameters.inputHashStrn = NULL;
            cmdParameters.hashAlgName_F = NULL;
            fprintf(stderr, "Error: Invalid parameter -F '%s' format.\n<alg> %s\n<hash> %s\n",rawParam->F.arg,getFormatErrorString(_p_res_alg), getFormatErrorString(_p_res_hash));
                    cmdParameters.F = false;

            return false;
        }
    }
    return true;
}

/**
 * Extracts the task by checking the combinations of command line parameters.
 * The task is stored in cmdParameters.task and it can be valid, invalid or undefined.
 */
static void extractTask(void)
{
    if (cmdParameters.h) {
        cmdParameters.task = showHelp;
    }//No multiple tasks allowed ((p or s) and (v or x)) and (p and v)
    else if (((cmdParameters.p || cmdParameters.s) && (cmdParameters.v || cmdParameters.x)) || (cmdParameters.p && cmdParameters.s)) {
        cmdParameters.task = invalid_multipleTasks;
    }//Download publications file
    else if (cmdParameters.p) {
        if (cmdParameters.o == true)
                cmdParameters.task = downloadPublicationsFile;
        else
            cmdParameters.task = invalid_p;
        }//Sign data or hash    
    else if (cmdParameters.s) {
        if (cmdParameters.o && cmdParameters.F)
                cmdParameters.task = signHash;
        else if (cmdParameters.o && cmdParameters.f)
                cmdParameters.task = signDataFile;
        else
            cmdParameters.task = invalid_s;
        }//Verify locally or online
    else if (cmdParameters.v) {
        if (cmdParameters.x && cmdParameters.i) {
            cmdParameters.task = verifyTimestamp_online;
        } else if (cmdParameters.i && cmdParameters.b) {
            cmdParameters.task = verifyTimestamp_locally;
        } else if (cmdParameters.b)
                cmdParameters.task = verifyPublicationsFile;
        else
            cmdParameters.task = invalid_v;
        }//Extend timestamps
    else if (cmdParameters.x && !cmdParameters.v) {
        if (cmdParameters.i && cmdParameters.o)
                cmdParameters.task = extendTimestamp;
        else
            cmdParameters.task = invalid_x;
        }//There is no task -p, -s, -v, x,    

    else
        cmdParameters.task = noTask;
    }

//Comparison macro, where _task is objects GT_CmdParameters member task one possible value. 
#define IS_TASK(_task) (cmdParameters.task == _task)

/**
 * Helper Macro for analyzing a parameter. If analyze fails a error message is printed and 
 * return false is called.
 * @param[in] _analyze A function name that analyzes parameter and returns PARAM_RES .
 * @param[in] flag_ Command line flag name for error message construction.
 * @param[in] param Parameter for analyzing and error message construction.
 */
#define ANALYZ_RESULT(_analyze, flag_, param) \
{ \
PARAM_RES _p_res = PARAM_UNKNOWN_ERROR; \
    _p_res = _analyze(param); \
    if(_p_res != PARAM_OK){ \
        fprintf(stderr, "Error: Invalid parameter %s '%s'. %s\n", flag_,param, getFormatErrorString(_p_res)); \
        return false; \
}};

/**
 * Analyzes the content of the parameters according to task defined. If error 
 * occurs a error message is printed.  
 * 
 * @return True if successful, false otherwise.
 * 
 * @note Currently only input and output files are under the test.
 */
static bool controlParameters(void)
{
    PARAM_RES res = PARAM_UNKNOWN_ERROR;
    
    if (IS_TASK(verifyPublicationsFile)) {
        ANALYZ_RESULT(analyseInputFile,"-b", cmdParameters.inPubFileName);
        return true;
    } else if (IS_TASK(downloadPublicationsFile)) {
        ANALYZ_RESULT(analyseOutputFile, "-o", cmdParameters.outPubFileName);
        return true;
    } else if (IS_TASK(signDataFile)) {
        ANALYZ_RESULT(analyseInputFile, "-f", cmdParameters.inDataFileName);
        return true;
    } else if (IS_TASK(signHash)) {
        ANALYZ_RESULT(analyseOutputFile, "-o", cmdParameters.outSigFileName);
        return true;
    } else if (IS_TASK(extendTimestamp)) {
        ANALYZ_RESULT(analyseInputFile, "-i", cmdParameters.inSigFileName);
        ANALYZ_RESULT(analyseOutputFile, "-o", cmdParameters.outSigFileName);
        return true;
    } else if (IS_TASK(verifyTimestamp_locally)) {
        ANALYZ_RESULT(analyseInputFile, "-i", cmdParameters.inSigFileName);
        ANALYZ_RESULT(analyseInputFile, "-b", cmdParameters.inPubFileName);
        goto extra_check;
    } else if (IS_TASK(verifyTimestamp_online)) {
        ANALYZ_RESULT(analyseInputFile, "-i", cmdParameters.inSigFileName);
        goto extra_check;
    } else if (IS_TASK(showHelp)) {
        return true;
    } else {
        return false;
    }

extra_check :
    res = analyseInputFile(cmdParameters.inDataFileName);
    if (cmdParameters.f && (res != PARAM_OK)) {
        fprintf(stderr, "Warning: Ignoring parameter -f '%s'. %s\n",cmdParameters.inDataFileName, getFormatErrorString(res));
        cmdParameters.f = false;
    }
    
    return true;
}

/**
 * If task is invalid prints error message explaining the fault.
 */
static void printTaskErrorMessage(void)
{
    switch (cmdParameters.task) {
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
                !(cmdParameters.o) ? "-o <output file> " : "",
                !(cmdParameters.f && cmdParameters.F) ? "-f <fn> or -F<hash>" : ""
                );
        break;
    case invalid_v:
        fprintf(stderr, "Error: Using -v you have to define missing parameter(s) %s%s\n",
                !(cmdParameters.i) ? "-i <fn> " : "",
                !(cmdParameters.b && cmdParameters.x) ? "-b <publications file> or -x (verify online)" : ""
                );
        break;
    case invalid_x:
        fprintf(stderr, "Error: Using -x you have to define missing parameter(s) %s%s\n",
                !(cmdParameters.o) ? "-o <output file> " : "",
                !(cmdParameters.i) ? "-i <publications file in> " : ""
                );
        break;
    case noTask:
        fprintf(stderr, "Error: The task is not defined. Use parameters -s or -x or -v or -p to define one. \n Use -h parameter for help.\n");

        break;

    }
}

//Check if a flag in (GT_CmdParameters) is set and print warning message about unused parameter.
#define UNUSED_FLAG_WARNING(param, flag_, ...) if(param.flag_ == true) fprintf(stderr,__VA_ARGS__);

/**
 * Prints warning message about unused parameters. 
 */
static void printTaskWarningMessage(void)
{
    switch (cmdParameters.task) {
    case signHash:
    case signDataFile:
        UNUSED_FLAG_WARNING(cmdParameters, b, "Warning: Can't use -b with -s.\n");
        UNUSED_FLAG_WARNING(cmdParameters, i, "Warning: Can't use -i with -s.\n");
        break;
    case verifyTimestamp_online:
        UNUSED_FLAG_WARNING(cmdParameters, b, "Warning: Can't use -b with -v -x.\n");
    case verifyTimestamp_locally:
    case verifyPublicationsFile:
        UNUSED_FLAG_WARNING(cmdParameters, H, "Warning: Can't use -H with -v.\n");
        UNUSED_FLAG_WARNING(cmdParameters, F, "Warning: Can't use -F with -v.\n");
        UNUSED_FLAG_WARNING(cmdParameters, o, "Warning: Can't use -o with -v.\n");
        break;
    case extendTimestamp:
        UNUSED_FLAG_WARNING(cmdParameters, H, "Warning: Can't use -H with -x.\n");
        UNUSED_FLAG_WARNING(cmdParameters, F, "Warning: Can't use -F with -x.\n");
        UNUSED_FLAG_WARNING(cmdParameters, f, "Warning: Can't use -f with -x.\n");

        break;
    }
}

/**
 * Prinst all the parameters and their values extracted from command line.
 */
static void GT_printParameters(void)
{
    printf("Parameters and arguments transfered from command line:\n");
    if (cmdParameters.o == true) printf("-o '%s'\n", cmdParameters.outPubFileName);
    if (cmdParameters.i == true) printf("-i '%s'\n", cmdParameters.inSigFileName);
    if (cmdParameters.b == true) printf("-b '%s'\n", cmdParameters.inPubFileName);
    if (cmdParameters.f == true) printf("-f '%s'\n", cmdParameters.inDataFileName);
    if (cmdParameters.V == true) printf("-V '%s'\n", cmdParameters.openSSLTruststoreFileName);
    if (cmdParameters.W == true) printf("-W '%s'\n", cmdParameters.openSSLTrustStoreDirName);
    if (cmdParameters.F == true) printf("-F Alg: '%s' and Hash: '%s'\n", cmdParameters.hashAlgName_F, cmdParameters.inputHashStrn);
    if (cmdParameters.H == true) printf("-H '%s'\n", cmdParameters.hashAlgName_H);
    if (cmdParameters.S == true) printf("-S '%s'\n", cmdParameters.signingService_url);
    if (cmdParameters.X == true) printf("-X '%s'\n", cmdParameters.verificationService_url);
    if (cmdParameters.P == true) printf("-P '%s'\n", cmdParameters.publicationsFile_url);
    if (cmdParameters.c == true) printf("-c %i\n", cmdParameters.networkTransferTimeout);
    if (cmdParameters.C == true) printf("-C %i\n", cmdParameters.networkConnectionTimeout);

    if (cmdParameters.x == true) printf("-x\n");
    if (cmdParameters.s == true) printf("-s\n");
    if (cmdParameters.v == true) printf("-v\n");
    if (cmdParameters.p == true) printf("-p\n");

    if (cmdParameters.t == true) printf("-t\n");
    if (cmdParameters.r == true) printf("-r\n");
    if (cmdParameters.n == true) printf("-n\n");
    if (cmdParameters.l == true) printf("-l\n");
    if (cmdParameters.d == true) printf("-d\n");

    if (cmdParameters.h == true) printf("-h\n");
}

/*********************************User Functions******************************/
void GT_pritHelp(void)
{

    fprintf(stderr,
            "\nGuardTime command-line signing tool, using API\n"
            "Usage: <-s|-x|-p|-v> [more options]\n"
            "Where recognized options are:\n"
            " -s		sign data \n"
            " -S <url>	specify Signing service URL\n"
            " -x		use online verification (eXtending) service\n"
            " -X <url>	specify verification (eXtending) service URL\n"
            " -p		download Publications file (The result is automatically verified)\n"
            " -P <url>	specify Publications file URL\n"
            " -v		verify signature token (-i <ts>); online verify with -x;\n"
            " -t		include service Timing\n"
            " -n		print signer Name (identity)\n"
            " -r		print publication References (use with -vx)\n"
            " -l		print 'extended Location ID' value\n"
            " -d		dump detailed information\n"
            " -f <fn>	file to be signed / verified\n"
            " -H <ALG>	hash algorithm used to hash the file to be signed\n"
            " -F <hash>	data hash to be signed / verified. hash Format: <ALG>:<hash in hex>\n"
            " -i <fn>	input signature token file to be extended / verified\n"
            " -o <fn>	output filename to store signature token or publications file\n"
            " -b <fn>	use specified publications file\n"
            " -V <fn>	use specified OpenSSL-style trust store file for publications file Verification\n"
            " -W <dir>	use specified OpenSSL-style trust store directory for publications file verification\n"
            " -c <num>	network transfer timeout, after successful Connect\n"
            " -C <num>	network Connect timeout.\n"
            " -h		Help (You are reading it now)\n"

            );

            fprintf(stderr, "\nDefault service access URL-s:\n"
            "\tSigning:			  %s\n"
            "\tVerifying:         %s\n"
            "\tPublications file: %s\n", KSI_DEFAULT_URI_AGGREGATOR, KSI_DEFAULT_URI_EXTENDER, KSI_DEFAULT_URI_PUBLICATIONS_FILE);
            fprintf(stderr, "\nSupported hash algorithms (-H, -F):\n"
            "\tSHA-1, SHA-256 (default), RIPEMD-160, SHA-224, SHA-384, SHA-512, RIPEMD-256, SHA3-244, SHA3-256, SHA3-384, SHA3-512, SM3\n");
}

bool GT_parseCommandline(int argc, char **argv)
{
    if (readCmdParam(argc, argv)) {
        extractTask();
                //  GT_printParameters();
                printTaskErrorMessage();
                printTaskWarningMessage();
				if(cmdParameters.task == noTask)
					GT_pritHelp();
        if (controlParameters()) {
            return true;
        } else {
            

            return false;
        }

    }
    return false;
}

GT_CmdParameters GT_getCMDParam(void)
{
    return cmdParameters;
}

