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

#ifndef COMPONENT_H
#define	COMPONENT_H


#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>	
#include <ksi/ksi.h>
#include "ksitool_err.h"	
	
/**
 * This function takes PARAM_SET as input and configures KSI_CTX and ERR_TRCKR.
 * Output parameter <ksi_log> is used to close the stream if needed by calling
 * \c fclose. If logging is directed to \c stderr or \c stdrout <ksi_log> will be
 * NULL.
 * 
 * \param set		PARAM_SET given.
 * \param ksi		Output parameter for KSI_CTX.
 * \param error		Output parameter for ERR_TRCKR.
 * \param ksi_log	Output parameter for KSI logging stream.
 * \return KT_OK if successful, error code otherwise.
 */
int TOOL_init_ksi(PARAM_SET *set, KSI_CTX **ksi, ERR_TRCKR **error, FILE **ksi_log);
	
#ifdef	__cplusplus
}
#endif

#endif	/* COMPONENT_H */

