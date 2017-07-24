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

#ifndef STRN_H
#define	STRN_H
#include <stddef.h>
#include <stdarg.h>

#ifdef	__cplusplus
extern "C" {
#endif


/**
 * Platform independent version of snprintf.
 * \param[in]		buf		Pointer to buffer.
 * \param[in]		n		Maximum number of bytes to be written into buffer. Includes terminating \c null character.
 * \param[in]		format	Format string.
 * \param[in]		...		Extra parameters for formatting.
 * \return The number of characters written, not including terminating \c null character. On error \c  0 is returned.
 */
size_t PST_snprintf(char *buf, size_t n, const char *format, ... );

/**
 * Platform independent version of strncpy that guarantees \c null terminated
 * destination. To copy \c N characters from source to destination \c n and size of
 * destination must be <tt>N + 1</tt>.
 * \param[in]		destination Pointer to destination.
 * \param[in]		source		Pointer to source.
 * \param[in]		n			Maximum number of characters to be copied including terminating \c null.
 * \return The pointer to destination is returned. On error \c NULL is returned.
 */
char *PST_strncpy (char *destination, const char *source, size_t n);

/**
 * This function works similar to the #PST_snprintf but is used to print text for
 * command-line help. Formatter can:
 *
 *  + Print formatted text (\c desc) thats maximum length (including indention) is \c rowLen.
 *  + Print indented text (\c desc) where indention is size of \c indent. See Example 1.
 *  + Print indented text (\c desc) where next line has extra indention in size of \c nxtLnIndnt. See Example 2.
 *  + Print a header in size of \c headerLen. It is composed of \c paramName and \c delimiter (including \c indent). If real header is larger than \c headerLen, it is printed without delimiter on the first line. In that case \c delimiter is printed on the next row followed by description (\c desc). See Example 3 and Example 4.
 *
 * Usage and examples:
 * \code{.txt}
 *
 * [          maximum row size           ]
 *
 * Regular text with indention:
 * [     indent      ][    text  line 1  ]
 * [     indent      ][    text  line N  ]
 * Example 1. (indent = 2, headerLen = 0, delimiter = '\0', nxtLnIndnt = 0, paramName = NULL, rowLen = 10):
 *     "1234567890"
 *     "  this is "
 *     "  sample"
 *
 * Regular text with extra indention from next row:
 * [indent][         text line 1         ]
 * [indent][eint][   text line 2         ]
 * [indent][eint][   text line N         ]
 * Example 2. (indent = 2, headerLen = 0, delimiter = '\0', nxtLnIndnt = 2, paramName = NULL, rowLen = 10):
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
 * \param buf			Pointer to buffer.
 * \param buf_len		The size of the buffer.
 * \param indent		The size of regular indention. Can be \c 0.
 * \param nxtLnIndnt	The amount of extra indention beginning from the next line. Can be \c 0.
 * \param headerLen		The size of the header. Can be \c 0.
 * \param rowLen		The overall size of the line.
 * \param paramName		Parameter name used to compose header.
 * \param delimiter		Delimiter character used for delimiter that separates parameters name from description.
 * \param desc		Format string, parameter description.
 * \param ...			Extra parameters for formatting.
 * \return The number of characters written, not including terminating \c null character. On error \c  0 is returned.
 */
size_t PST_snhiprintf(char *buf, size_t buf_len, int indent, int nxtLnIndnt, int headerLen, int rowLen, const char *paramName, const char delimiter, const char *desc, ...);
/*
 * @}
 */

#ifdef	__cplusplus
}
#endif

#endif	/* STRN_H */

