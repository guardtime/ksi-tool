/**************************************************************************
 *
 * GUARDTIME CONFIDENTIAL
 *
 * Copyright (C) [2015] Guardtime, Inc
 * All Rights Reserved
 *
 * NOTICE:  All information contained herein is, and remains, the
 * property of Guardtime Inc and its suppliers, if any.
 * The intellectual and technical concepts contained herein are
 * proprietary to Guardtime Inc and its suppliers and may be
 * covered by U.S. and Foreign Patents and patents in process,
 * and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this
 * material is strictly forbidden unless prior written permission
 * is obtained from Guardtime Inc.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime Inc.
 */

#ifndef PARAM_VALUE_TYPES_H
#define	PARAM_VALUE_TYPES_H

#ifdef	__cplusplus
extern "C" {
#endif

	//#define KSITOOL_ERR_BASE 0x100001
//#define KSITOOL_ERR_BASE 0x100001
//#define INI_ERR_BASE 0x020001

#ifndef PARAM_SET_ERROR_BASE 
#define		PARAM_SET_ERROR_BASE 0x30001 
#endif

enum param_set_err {
	PST_OK = 0,
	PST_INVALID_ARGUMENT = PARAM_SET_ERROR_BASE,
	PST_INVALID_FORMAT,
	PST_INDEX_OVF,
	PST_PARAMETER_NOT_FOUND,
	PST_PARAMETER_VALUE_NOT_FOUND,
	PST_PARAMETER_EMPTY,
	PST_OUT_OF_MEMORY,
	PST_NEGATIVE_PRIORITY,
	PST_PARAMETER_INVALID_FORMAT,
	PST_UNDEFINED_BEHAVIOUR,
	PST_UNKNOWN_ERROR,
};	
	
typedef struct PARAM_VAL_st PARAM_VAL;	
typedef struct PARAM_st PARAM;
typedef struct PARAM_SET_st PARAM_SET;

#ifdef	__cplusplus
}
#endif


#endif	/* PARAM_VALUE_TYPES_H */