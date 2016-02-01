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

#ifndef GT_TASK_SUPPORT_H
#define	GT_TASK_SUPPORT_H

#ifdef _WIN32
#ifdef _DEBUG
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif
#endif


#include <stdio.h>
#include <string.h>
#include <ksi/ksi.h>
#include "param_set/task_def.h"
#include <stdlib.h>
#include "gt_cmd_common.h"
#include "ksitool_err.h"
#include "api_wrapper.h"
#include "printer.h"


#ifdef	__cplusplus
extern "C" {
#endif

typedef enum tasks_en{
	noTask = 0,
	downloadPublicationsFile,
	dumpPublicationsFile,
	createPublicationString,
	signDataFile,
	signHash,
	extendTimestamp,
	verifyPublicationsFile,
	verifyTimestamp,
	verifyTimestampOnline,
	getRootH_T,
	setSysTime,
	showHelp,
	invalid
} TaskID;

/**
 * Configures KSI using parameters extracted from command line.
 *
 * @param[in] task Pointer to task object.
 * @param[out] ksi Pointer to receiving pointer to KSI context object.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int initTask(TASK *task ,KSI_CTX **ksi, ERR_TRCKR **err);


/**
 * Closes KSI and logging file.
 * @param[in] task	Pointer to task object.
 * @param[in] ksi	Pointer to receiving pointer to KSI context object.
 */
void closeTask(KSI_CTX *ksi);

/**
 * Calculates the hash of an input file or stream.
 * If fnameout is not NULL, saves input data to a file. This is useful
 * for hashing a stream and saving the stream at the same time.
 *
 * @param err		Error tracker.
 * @param hsr		Hasher for file hashing.
 * @param fnamein	Files name to be hashed. If files name is -, file is read from stdin.
 * @param fnameout	Files name where input data is saved.
 * @param hash		Hash value of the file.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int getFilesHash(ERR_TRCKR *err, KSI_DataHasher *hsr, const char *fnamein, const char *fnameout, KSI_DataHash **hash);

/**
 * Saves signature object to a file. If files name is -, file is written to stdout.
 *
 * @param err	Error tracker.
 * @param ksi	KSI Context.
 * @param sign	Signature object.
 * @param fname	Files name where the signature is saved.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int saveSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sign, const char *fname);

/**
 * Saves publication file object to a file. If files name is -, file is written to stdout.
 *
 * @param err		Error tracker.
 * @param ksi		KSI Context.
 * @param pubfile	Publication file object.
 * @param fname		Files name where the signature is saved.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int savePublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_PublicationsFile *pubfile, const char *fname);

/**
 * Reads publication file from file. If files name is -, file is read from stdin.
 *
 * @param err		Error tracker.
 * @param ksi		KSI Context.
 * @param fname		Files name where the signature is saved.
 * @param pubfile	Publication file object.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int loadPublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_PublicationsFile **pubfile);

/**
 * Reads signature file from file. If files name is -, file is read from stdin.
 *
 * @param err		Error tracker.
 * @param ksi		KSI Context.
 * @param fname		Files name where the signature is saved.
 * @param sig		Signature file object.
 *
 * @return KT_OK if successful, error code otherwise.
 */
int loadSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_Signature **sig);

/**
 * Returns true if signature is extended. False if not.
 */
bool isSignatureExtended(const KSI_Signature *sig);

/*Controls parameter set and returns true if user wants to write data files to stdout*/
bool isPiping(PARAM_SET *set);

/**
 * Reads hash from command line and creates the KSI_DataHash object.
 *
 * @param[in] cmdparam Pointer to command line data object.
 * @param[in] ksi Pointer to ksi context object.
 * @param[out] hash Pointer to receiving pointer to KSI_DataHash object.
 *
 */
int getHashFromCommandLine(const char *imprint, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hash);

void print_progressDesc(bool showTiming, const char *msg, ...);

void print_progressResult(int res);

/**
 * Returns short string representing OID. If string representation is not found
 * the same OID is returned.
 * 
 * @param	OID[in]		OID value.
 * @return OID string representation if successful, NULL otherwise. If short string is not found
 * the input value is returned.
 */
const char *OID_getShortDescriptionString(const char *OID);

char *STRING_getBetweenWhitespace(const char *strn, char *buf, size_t buf_len);

const char * STRING_locateLastOccurance(const char *str, const char *findIt);

/**
 * Extract a substring from given string. String is extracted using two substrings
 * from and to. Extracted string is located as <from><extracted string><to>.
 * Strings from and to are located using abstract functions \find_from and \find_to.
 * If one of the substrings is missing the result is not extracted. When from is
 * set as NULL the string is extracted from the beginning of the input string.
 * When to is set as NULL the string is extracted from <from> until the end of
 * the input strings.
 * 
 * Abstract functions must have following format:
 * const char* (*find_to)(const char *str, const char *findIt)
 * This function searches a location from str according to findIt and must return
 * pointer to the first or last character that is extracted from the str or NULL if
 * locations is not found or an error occurred. 
 * 
 * Abstract functions can be NULL as in this case default search is performed.
 * 
 * @param strn[in]			Input string.
 * @param from[in]			The first border string.
 * @param to[in]			The last border string.
 * @param buf[in/out]		The buffer where substring is extracted.
 * @param buf_len[in]		The size of the buffer.
 * @param find_from[in]		Abstract function to find <from>. 	
 * @param find_to[in]		Abstract functions to find <to>.
 * @param firstChar[in/out]	The pointer to the pointer of first character not extracted from string. Can be NULL.
 * @return
 * @note buf is only changed on successful extraction.
 */
char *STRING_extractAbstract(const char *strn, const char *from, const char *to, char *buf, size_t buf_len,
		const char* (*find_from)(const char *str, const char *findIt),
		const char* (*find_to)(const char *str, const char *findIt),
		const char** firstChar);

/**
 * Extract a substring from given string. String is extracted using two substrings
 * from and to. Extracted string is located as <from><extracted string><to>.
 * For example when using from as "[" and to as "]" on input string "[test]" the
 * extracted value is "test". If one of the substrings is missing the result is 
 * not extracted. When from is set as NULL the string is extracted from the
 * beginning of the input string. When to is set as NULL the string is extracted
 * until the end of the input strings. For example from = "->", to = NULL, input
 * string is "->test test", the resulting output is "test test".
 * 
 * When searching "from" substring always the first occurrence is used, when searching
 * "to" string, always the last one is used.
 * 
 * @param strn[in]		Input string.
 * @param from[in]		The first border string.
 * @param to[in]		The last border string.
 * @param buf[in]		The buffer where substring is extracted.
 * @param buf_len[in]	The size of the buffer.
 * 
 * @return Returns buf is successful, NULL otherwise.
 */
char *STRING_extract(const char *strn, const char *from, const char *to, char *buf, size_t buf_len);

/**
 * Same as STRING_extract, but the white space characters from the beginning and
 * end are removed.
 * @param strn[in]		Input string.
 * @param from[in]		The first border string.
 * @param to[in]		The last border string.
 * @param buf[in]		The buffer where substring is extracted.
 * @param buf_len[in]	The size of the buffer.
 * 
 * @return Returns buf is successful, NULL otherwise.
 */
char *STRING_extractRmWhite(const char *strn, const char *from, const char *to, char *buf, size_t buf_len);

/**
 * This function extracts "chunks" from the input string. Input string is cut into
 * peaces by whitespace characters. If there are opening '"' inside the input string
 * next whitespace characters are ignored until closing '"'. Function returns a 
 * pointer (inside input string) that can be feed to the same function to extract
 * the next chunk. If the end of input string is reached NULL is returned.
 * For example:
 * 
 * @param	strn[in]	Pointer to NUL terminated C string.
 * @param	buf[out]	Pointer to receiving buffer.
 * @param	buf_len[in]	The size of the buffer.
 * 
 * @return Pointer for next iteration or NULL on error or end of string.
 */
const char *STRING_getChunks(const char *strn, char *buf, size_t buf_len);

/**
 * Functions for using with STRING_extractAbstract
 */

/**
 * Searching		XX
 * From				aXXbcXXd
 * Return value points to b.
 */
const char* find_charAfterStrn(const char *str, const char *findIt);

/**
 * Searching		XX
 * From				aXXbcXXd
 * Return value points to a.
 */
const char* find_charBeforeStrn(const char* str, const char* findIt);

/**
 * Searching		XX
 * From				aXXbcXXd
 * Return value points to c.
 */
const char* find_charBeforeLastStrn(const char* str, const char* findIt);


int PARAM_SET_getStrValue(PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, char **value);
int PARAM_SET_getIntValue(PARAM_SET *set, const char *name, const char *source, int prio, unsigned at, int *value);
#ifdef	__cplusplus
}
#endif

#endif	/* GT_TASK_H */

