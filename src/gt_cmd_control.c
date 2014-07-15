#include <ctype.h>
#include <stdio.h>		//input output
#include <string.h>
#ifdef _WIN32 
#   include <io.h>
#else
#   define _access_s access
#endif


#include <errno.h>
#include "gt_cmdcommon.h"
#include "gt_cmd_control.h"

#define CheckNullPtr(strn, msg) \
    if(strn==NULL){ \
    fprintf(stderr, msg); \
    return false;}

#define CheckEmpty(strn, msg) \
    if(strlen(strn) == 0){ \
    fprintf(stderr, msg); \
    return false;}

bool isPathFormOk(const char *path)
{
    CheckNullPtr(path, "Path is nullptr\n");
    return true;
}

bool isHexFormatOK(const char *hex)
{
    int i = 0;
    char C;
    bool failure = false;

    CheckNullPtr(hex, "Hex is nullptr.\n");
    CheckEmpty(hex, "Hex has no content: ''.\n");
    
    while (C = hex[i++]) {
        if (!isxdigit(C)) {
            printf("Invalid hex char: '%c'\n", C);
            failure = true;
        }
    }
    return failure ? false : true;
}

bool isURLFormatOK(const char *url)
{
    CheckNullPtr(url, "Url is nullptr.\n");
    CheckEmpty(url, "Url has no content: ''.\n");
    return true;
}

bool isIntegerFormatOK(const char *integer)
{
    int i = 0;
    int C;
    CheckNullPtr(integer, "Integer is nullptr.\n");
    CheckEmpty(integer, "Integer has no content: ''.\n");

    while (C = integer[i++]) {
        if (isdigit(C)==0) {
            printf("'%s' is not a valid integer.\n", integer);
            return false;
        }
    }
    
    
    
    return true;
}

bool isHashAlgFormatOK(const char *hashAlg){
    CheckNullPtr(hashAlg, "HashAlg is nullptr.");
    CheckEmpty(hashAlg, "Hash algorithm has no content: ''.\n");
    return true;
    }

static int doFileExists(const char* path)
{
    _access_s(path, 0);
    return errno;
}

bool analyseInputFile(const char* path)
{
    int fileStatus = EINVAL;
    if(isPathFormOk(path))
        fileStatus = doFileExists(path);
    
    switch (fileStatus) {
    case 0:
        return true;
        break;
    case EACCES:
        fprintf(stderr, "Error: Access denied for file \"%s\" !\n", path);
        break;
    case ENOENT:
        fprintf(stderr, "Error: File \"%s\" dose not exist!\n", path);
        break;
    case EINVAL:
        fprintf(stderr, "Error: File \"%s\" is invalid parameter!\n", path);
        break;
    }

    return false;
}

//TODO add some functionality
bool analyseOutputFile(const char* path)
{
    if(isPathFormOk(path))
        return true;
    else
        return false;
}
