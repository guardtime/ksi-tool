/* 
 * File:   gt_cmd_control.h
 * Author: Taavi
 *
 * Created on July 10, 2014, 6:19 PM
 */

#ifndef GT_CMD_CONTROL_H
#define	GT_CMD_CONTROL_H

#include "gt_cmd_common.h"

#ifdef	__cplusplus
extern "C" {
#endif

    typedef enum paramContRes {
        PARAM_OK,
        PARAM_NULLPTR,
        PARAM_NOCONTENT,
        PARAM_INVALID,
        FILE_ACCESS_DENIED,
        FILE_DOSE_NOT_EXIST,
        FILE_INVALID_PATH,
        PARAM_UNKNOWN_ERROR
    } PARAM_RES;

    PARAM_RES isPathFormOk(const char *path);
    PARAM_RES isHexFormatOK(const char *hex);
    PARAM_RES isURLFormatOK(const char *url);
    PARAM_RES isIntegerFormatOK(const char *integer);
    PARAM_RES isHashAlgFormatOK(const char *hashAlg);

    PARAM_RES analyseInputFile(const char* path);
    PARAM_RES analyseOutputFile(const char* path);
    
    const char * getFormatErrorString(PARAM_RES res);



#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMD_CONTROL_H */

