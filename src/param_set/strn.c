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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include "strn.h"


static size_t param_set_vsnprintf(char *buf, size_t n, const char *format, va_list va){
	size_t ret = 0;
	int tmp;
	if (buf == NULL || n > INT_MAX || n == 0 || format == NULL) goto cleanup;
#ifdef _WIN32
	/* NOTE: If there is empty space in buf, it will be filled with 0x00 or 0xfe. */
	tmp = vsnprintf_s(buf, n, _TRUNCATE, format, va);
	if (tmp < 0) {
		ret = n - 1;
		goto cleanup;
	}
	ret = (size_t) tmp;
#else
	(void)tmp;
	ret = vsnprintf(buf, n, format, va);
	if (ret >= n) {
		ret = n - 1;
		goto cleanup;
	}
#endif

cleanup:

	return ret;
}

size_t PST_snprintf(char *buf, size_t n, const char *format, ... ){
	size_t ret;
	va_list va;
	va_start(va, format);
	ret = param_set_vsnprintf(buf, n, format, va);
	va_end(va);
	return ret;
}

char *PST_strncpy (char *destination, const char *source, size_t n){
	char *ret = NULL;
	if (destination == NULL || source == NULL || n == 0) {
		goto cleanup;
	}
	ret = strncpy(destination, source, n - 1);
	destination[n - 1] = 0;

cleanup:

	return ret;
}

static size_t getWord(char *buf, size_t buf_len, const char *str, const char **next) {
	size_t i = 0;
	int C = '\0';

	if (buf == NULL || buf_len < 2 || str == NULL || next == NULL) {
		return 0;
	}

	/* Remove some white space. */
	while(*str != '\0' && *str != '\n' && isspace((int)(*str))) str++;
	if (*str == '\0') {
		*next = NULL;
		return 0;
	}

	if (*str == '\n') {
		buf[0] = '\n';
		buf[1] = '\0';
		*next = str + 1;
		return 1;
	}

	/* Load another chunk of data. */
	buf[0] = '\0';
	for (i = 0, C = str[0]; C != '\0' && !isspace(C) && i < (buf_len - 1); i++, C = str[i]) {
		buf[i] = (char) (0xff & C);
	}

	/* Set pointer to the next value and return the size of the chunk extracted. */
	buf[i] = '\0';
	*next = &str[i];
	return i;
}

/**
 * This function works similarly to the #PST_snprintf but is used to print text for
 * command-line help. Formatter can:
 *
 *  + Print formatted text (\c desc) with maximum length (including indention) of \c rowLen.
 *  + Print indented text (\c desc) where indention size is \c indent. See Example 1.
 *  + Print indented text (\c desc) where next line has extra indention of \c nxtLnIndnt. See Example 2.
 *  + Print a header with size of \c headerLen. It is composed of \c paramName and \c delimiter (including \c indent).
 *    If real header is larger than \c headerLen, it is printed without delimiter on the first line. In that case
 *    \c delimiter is printed on the next row followed by description (\c desc). See Example 3 and Example 4.
 *
 * Usage and examples:
 * \code{.txt}
 * header includes parameter name ()
 *
 * [          maximum row size           ]
 *
 * Regular text with indention:
 * [     indent      ][    text  line 1  ]
 * [     indent      ][    text  line N  ]
 * Example 1. (indent = 2, headerLen = 0, delimiter = NUL, nxtLnIndnt = 0, paramName = NULL, rowLen = 10):
 *     "1234567890"
 *     "  this is "
 *     "  sample"
 *
 * Regular text with extra indention from next row:
 * [indent][         text line 1         ]
 * [indent][eint][   text line 2         ]
 * [indent][eint][   text line N         ]
 * Example 2. (indent = 2, headerLen = 0, delimiter = NUL, nxtLnIndnt = 2, paramName = NULL, rowLen = 10):
 *     "1234567890"
 *     "  this is "
 *     "    sample"
 *     "    text  "
 *
 * Parameter description with indention:
 * [header][delimiter][    desc. line 1  ]
 * [     indent      ][    desc. line N  ]
 * Example 3. (indent = 2, headerLen = 8, delimiter = '-', nxtLnIndnt = 0, paramName = "-a", rowLen = 20):
 *     "123456789_1234567890"
 *     "  -a  - this is my  "
 *     "        description "
 *
 * Parameter description with indention where header is larger than expected:
 * [ too large header]
 * [indent][delimiter][   desc. line 1   ]
 * [indent           ][   desc. line N   ]
 * Example 4. (indent = 2, headerLen = 8, delimiter = '-', nxtLnIndnt = 0, paramName = "-a", rowLen = 20):
 *     "123456789_1234567890"
 *     "  --long            "
 *     "      - this is long"
 *     "        description "
 * \endcode
 *
 * \param buf
 * \param buf_len
 * \param indent
 * \param nxtLnIndnt
 * \param headerLen
 * \param rowLen
 * \param paramName
 * \param delimiter
 * \param desc
 * \param ...
 * \return
 */
size_t PST_snhiprintf(char *buf, size_t buf_len, int indent, int nxtLnIndnt, int headerLen, int rowLen, const char *paramName, const char delimiter, const char *desc, ...) {
	va_list va;
	char *description = NULL;
	int calculated = 0;
	size_t current_row_len = 0;
	size_t count = 0;
	size_t c = 0;
	int spaceNeeded = 0;
	const char *next = NULL;

	if (buf == NULL || buf_len == 0 || paramName == NULL || desc == NULL) {
		return 0;
	}


	/* Create buff value for preprocessing. */
	description =(char*)malloc(sizeof(char) * buf_len);
	if (description == NULL) return 0;

	va_start(va, desc);
	param_set_vsnprintf(description, buf_len, desc, va);
	buf[buf_len - 1] = 0;
	va_end(va);

	if (headerLen > 0) {
		/* Get calculated size of the header, if it is too large insert a line break. */
		calculated = (headerLen - indent - (int)strlen(paramName) - 3);
		calculated = calculated < 0 ? 0 : calculated;

		/* Print the header of the help row (indention, parameter, delimiter and description. */
		count += PST_snprintf(buf + count, buf_len - count, "%*s%s%*s", indent, "", paramName, calculated, "");
		current_row_len = count;
		if (current_row_len > headerLen - 3) {
			c = PST_snprintf(buf + count, buf_len - count, "\n%*s %c ", headerLen - 3, "", delimiter);
			count += c;
			current_row_len = c - 1;
			spaceNeeded = 0;
		} else {
			c = PST_snprintf(buf + count, buf_len - count, " %c ", delimiter);
			current_row_len += c;
			count += c;
			spaceNeeded = 0;
		}
	}


	next = description;
	while (next != NULL) {
		size_t word_len = 0;
		char wordBuffer[1024];

		c = 0;
		word_len = getWord(wordBuffer, sizeof(wordBuffer), next, &next);
		if (next == NULL) break;

		/**
		 * If word is a new line character, force the print function to change
		 * the line and handle indention.
		 */
		if (*wordBuffer == '\n') {

			spaceNeeded = 0;
			if (next != NULL && *next != '\0') {
				current_row_len = rowLen + 1;
				wordBuffer[0] = '\0';
			}
		}

		if (current_row_len + word_len + 1 >= rowLen) {

			if (word_len + indent >= rowLen) {
				/* TODO: Do something about that. */
				c = PST_snprintf(buf + count, buf_len - count, "\n%*s%s", headerLen + nxtLnIndnt, "", wordBuffer);
			} else {
				c = PST_snprintf(buf + count, buf_len - count, "\n%*s%s", headerLen + nxtLnIndnt, "", wordBuffer);
			}

			count += c;
			current_row_len = c - 1;
			continue;
		}

		c = PST_snprintf(buf + count, buf_len - count, "%s%s", spaceNeeded ? " " : "", wordBuffer);
		spaceNeeded = 1;
		current_row_len += c;
		count += c;

	}

	if (description != NULL) free(description);
	return count;
}

