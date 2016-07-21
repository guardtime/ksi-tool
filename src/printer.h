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

#ifndef PRINTER_H
#define	PRINTER_H

#include <stdio.h>


#define	PRINT_RESULT  0x01
#define	PRINT_INFO  0x02
#define	PRINT_WARNINGS  0x04
#define	PRINT_ERRORS  0x08
#define	PRINT_DEBUG  0x10



#ifdef	__cplusplus
extern "C";
#endif

void print_init(void);
void print_setStream(unsigned print, FILE *stream);
void print_enable(unsigned print);
void print_disable(unsigned print);
int print_result(const char *format, ... );
int print_info(const char *format, ... );
int print_warnings(const char *format, ... );
int print_errors(const char *format, ... );
int print_debug(const char *format, ... );
#ifdef	__cplusplus
}
#endif

#endif	/* PRINTER_H */

