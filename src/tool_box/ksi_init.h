/*
 * Copyright 2013-2016 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#ifndef COMPONENT_H
#define	COMPONENT_H


#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>	
#include <ksi/ksi.h>
#include "ksitool_err.h"	
#include "smart_file.h"
#include "err_trckr.h"

/**
 * This function takes PARAM_SET as input and configures KSI_CTX and ERR_TRCKR.
 * Output parameter <ksi_log> is used to close the stream if needed by calling
 * \c SMART_FILE_close. If logging is directed to \c stderr or \c stdrout <ksi_log> will be
 * NULL.
 * 
 * \param set		PARAM_SET given.
 * \param ksi		Output parameter for KSI_CTX.
 * \param error		Output parameter for ERR_TRCKR.
 * \param ksi_log	Output parameter for KSI logging stream.
 * \return KT_OK if successful, error code otherwise.
 */
int TOOL_init_ksi(PARAM_SET *set, KSI_CTX **ksi, ERR_TRCKR **error, SMART_FILE **ksi_log);
	
#ifdef	__cplusplus
}
#endif

#endif	/* COMPONENT_H */

