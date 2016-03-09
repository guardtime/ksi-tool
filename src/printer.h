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

