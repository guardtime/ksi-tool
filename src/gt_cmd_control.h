/* 
 * File:   gt_cmd_control.h
 * Author: Taavi
 *
 * Created on July 10, 2014, 6:19 PM
 */

#ifndef GT_CMD_CONTROL_H
#define	GT_CMD_CONTROL_H



#ifdef	__cplusplus
extern "C" {
#endif

bool isPathFormOk(const char *path);
bool isHexFormatOK(const char *hex);
bool isURLFormatOK(const char *url);
bool isIntegerFormatOK(const char *integer);
bool isHashAlgFormatOK(const char *hashAlg);

bool analyseInputFile(const char* path);
bool analyseOutputFile(const char* path);

#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMD_CONTROL_H */

