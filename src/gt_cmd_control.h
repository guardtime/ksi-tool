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

bool isPathFormOk(char *path);
bool isHexFormatOK(char *hex);
bool isURLFormatOK(char *url);
bool isIntegerFormatOK(char *integer);
bool isHashAlgFormatOK(char *hashAlg);

bool analyseInputFile(const char* path);
bool analyseOutputFile(const char* path);

#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMD_CONTROL_H */

