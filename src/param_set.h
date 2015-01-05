/* 
 * File:   gt_cmd_raw.h
 * Author: Taavi
 *
 * Created on September 18, 2014, 6:07 PM
 */

#ifndef PARAM_SET_H
#define	PARAM_SET_H

#include "gt_cmd_control.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * This files deals with command-line parameters. An object paramSet used in 
 * functions is parameter set containing all parameters and its values.
 * The flow of using parameter set is as follows:
 * 1) new paramSet
 * 2) add control functions
 * 3) Read from file or command-line
 * 4) get values, print errors and warnings, work
 * 5) free paramSet
 */	
	

typedef struct paramSet_st paramSet;	

/**
 * Creates new parameter set object using parameter name definition.
 * Parameter names are defined using {name}* where '*' means that the
 * parameter can have multiple values.
 * Exampl {h}{file}*{o}*{n}
 * 
 * @param[in] names pointer to parameter names.
 * @param[out] set pointer to recieving pointer to paramSet obj.
 * @return true if successful false otherwise.
 */
bool paramSet_new(const char *names, paramSet **set);

void paramSet_free(paramSet *set);

/**
 * Adds format and content control to a set of parameters. The set is defined 
 * using {} containing parameters name. For example {a}{h}{file} 
 * @param[in] set pointer to parameter set
 * @param[in] names parameters names
 * @param[in] controlFormat function for format control
 * @param[in] controlContent function for content control 
 */
void paramSet_addControl(paramSet *set, const char *names, FormatStatus (*controlFormat)(const char *), ContentStatus (*controlContent)(const char *));

/**
 * Reads parameter values from command-line using predefined 
 * parameter set.
 * 
 * @param[in] argc count of command line objects
 * @param[in] argv array of command line objects
 * @param[in/out] set parameter set
 */
void paramSet_readFromCMD(int argc, char **argv, paramSet *set);

/**
 * Reads parameter values from file predefined 
 * parameter set.
 * 
 * @param[in] fname files path
 * @param[in/out] set pointer to parameter set
 */
void paramSet_readFromFile(const char *fname, paramSet *set);

/**
 * Controls if the format and content of the parameters is OK.
 * 
 * @param[in] set pointer to parameter set
 * @return true if successful false otherwise.
 */
bool paramSet_isFormatOK(paramSet *set);

bool paramSet_isSetByName(paramSet *set, char *name);

/**
 * Search for a integer value from parameter set by name at index. If parameter has 
 * format or content error function fails even if some kind of information is present.
 * 
 * @param[in] set pointer to parameter set
 * @param[in] name pointer to parameters name
 * @param[in] at parameters index
 * @param[out] value pointer to receiving variable 
 * @return true if successful false otherwise.
 * 
 **/
bool paramSet_getIntValueByNameAt(paramSet *set, char *name,int at, int *value);

/**
 * Searches for a c string value from parameter set by name at index. If parameter has 
 * format or content error function fails even if some kind of information is present.
 * 
 * @param[in] set pointer to parameter set
 * @param[in] name pointer to parameters name
 * @param[in] at parameters index
 * @param[out] value pointer to receiving pointer to variable 
 * @return true if successful false otherwise.
 * 
 * @note value must not be freed
 **/

bool paramSet_getStrValueByNameAt(paramSet *set, char *name,int at, char **value);

/**
 * Searches for a parameter by name and gives its count of values. Even the 
 * invalid values are counted.
 * 
 * @param[in] set pointer to parameter set
 * @param[in] name pointer to parameters name
 * @param[out] count pointer to receiving pointer to variable 
 * @return true if successful false otherwise.
 */
bool paramSet_getValueCountByName(paramSet *set, char *name, int *count);

void paramSet_removeParameterByName(paramSet *set, char *name);
bool paramSet_appendParameterByName(const char *argument, char *name, paramSet *set);

void paramSet_Print(paramSet *set);
void paramSet_PrintErrorMessages(paramSet *set);
void paramSet_printUnknownParameterWarnings(paramSet *set);


#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMD_RAW_H */

