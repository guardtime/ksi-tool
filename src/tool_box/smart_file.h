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

#ifndef SMART_FILE_H
#define	SMART_FILE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct SMART_FILE_st SMART_FILE;

int SMART_FILE_open(const char *fname, const char *mode, SMART_FILE **file);
void SMART_FILE_close(SMART_FILE *file);
int SMART_FILE_write(SMART_FILE *file, char *raw, size_t raw_len, size_t *count);
int SMART_FILE_read(SMART_FILE *file, char *raw, size_t raw_len, size_t *count);

/**
 * 
 * @param file
 * @return A non-zero value is returned in the case that the end-of-file indicator associated with the stream is set.
Otherwise, zero is returned.
 */
int SMART_FILE_isEof(SMART_FILE *file);

/**
 * 
 * @param file
 * @return A non-zero value is returned in the case that the error indicator associated with the stream is set.
Otherwise, zero is returned.
 */
int SMART_FILE_isError(SMART_FILE *file);

#ifdef	__cplusplus
}
#endif

#endif	/* SMART_FILE_H */

