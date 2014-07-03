

#ifndef GT_CMDPARAMETERS_H
#define	GT_CMDPARAMETERS_H

#include "gt_cmdcommon.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct _gtimeCmdParameters {
    char *outSigFileName;		//Output file name for signature and publication file       (-o)
    char *outPubFileName;		//Output file name for signature and publication file       (-o)
    char *inSigFileName;                //Input signature file name for extending or verification   (-i)
    char *inPubFileName;                //Use specified publications file                           (-b)
    char *inDataFileName;		//Input file name for signing and verification              (-f)
    
    char *openSSLTruststoreFileName;    //specified OpenSSL-style truststore file                   (-V)
    char *openSSLTrustStoreDirName;     //specified OpenSSL-style truststore directory              (-W)
    
    //char *rawHashString;              //A buffer for storing raw HASH string                      (-F)
    char *hashAlgName_H;                //A buffer for storing raw HASH algorithm identifier        (-H)
    char *hashAlgName_F;                //A buffer for storing raw HASH algorithm identifier        (-F)
    char *inputHashStrn;                //A buffer for storing raw HASH                             (-F)
//    int hashAlg;                      //Hasalg extracted from parameters -H or -F                 (-H, -F)                        
    
    char* signingService_url;           //specify Signing Service URL                               (-S)
    char* verificationService_url;      //specify verification (eXtending) service URL              (-X)			
    char* publicationsFile_url;         //specify Publications file URL                             (-P)
    
    int networkTransferTimeout;         //                                                          (-c)
    int networkConnectionTimeout;       //                                                          (-C)
    
  //  int a;                              //simulated extending request age in days (when no signature token is specified), default 36 (-a)
    
    //Operations
    bool o;                             //is output file  
    bool i;				//is input signature  file  
    bool b;				//is publications file in  
    bool f;				//is input data file  
    bool V;				//is OpenSSL-style truststore file  
    bool W;				//is OpenSSL-style truststore destroy  
    bool F;				//is Input data hash  
    bool H;				//is Different hashAlg  
    bool S;				//is signing service url  
    bool X;				//is extending service url  
    bool P;				//is publications file url  
    bool c;				//is networkTransferTimeout  
    bool C;				//is networkConnectionTimeout  
    
    bool x;				//is task extending
    bool s;				//is task signing
    bool v;				//is task verification
    bool p;     			//is task download Publications File
    
    bool t;
    bool r;
    bool n;
    bool l;
    bool d;
    bool h;
  
} GT_CmdParameters;    

typedef enum _tasksTODO{
    downloadPublicationsFile,
    verifyPublicationsFile,
    signDataFile,
    signHash,
    extendTimestamp,
    verifyTimestamp_use_pubfile,
    verifyTimestamp_and_file_use_pubfile,
    verifyTimestamp_online,
    verifyTimestamp_and_file_online,
    invalid_s,
    invalid_v,
    invalid_x,
    invalid_p,
    invalid_multipleTasks,
    noTask
} GT_Tasks;


/**
 * Function for extracting raw data from commandline
 * @param argc - number of command line parameters.
 * @param argv - command line parameters.
 * @return - true if successfule, false otherwise.
 */
bool GT_parseCommandline(int argc, char **argv);

/**
 * Getter for function parameters extracted from command line.
 * @return command line parameters
 */
GT_CmdParameters GT_getCMDParam(void);

/**
 * Getter for extracted task
 * @return task todo
 */

GT_Tasks GT_getTask(void);

/**
 * Function for printing command line help. 
 */
void GT_pritHelp(void);

#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMDPARAMETERS_H */

