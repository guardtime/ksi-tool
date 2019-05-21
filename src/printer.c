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

#include "printer.h"
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>





struct printer_conf_st {
	unsigned print;
	FILE *info;
	FILE *result;
	FILE *warning;
	FILE *error;
	FILE *debug;
	FILE *suggestion;
};

struct printer_conf_st printer_conf = {(PRINT_INFO | PRINT_WARNINGS | PRINT_ERRORS | PRINT_RESULT | PRINT_DEBUG | PRINT_SUGGESTION), NULL, NULL, NULL, NULL, NULL, NULL};

void print_setStream(unsigned print, FILE* stream) {
	if (print & PRINT_INFO) {
		printer_conf.info = stream;
	}
	if (print & PRINT_WARNINGS) {
		printer_conf.warning = stream;
	}
	if (print & PRINT_ERRORS) {
		printer_conf.error = stream;
	}
	if (print & PRINT_RESULT) {
		printer_conf.result = stream;
	}
	if (print & PRINT_DEBUG) {
		printer_conf.debug = stream;
	}
	if (print & PRINT_SUGGESTION) {
		printer_conf.suggestion = stream;
	}
}

void print_enable(unsigned print) {
	printer_conf.print |= print;
}

void print_disable(unsigned print) {
	printer_conf.print &= ~print;
}

void print_init(void) {
	print_setStream(PRINT_RESULT | PRINT_INFO, stdout);
	print_setStream(PRINT_DEBUG | PRINT_ERRORS | PRINT_WARNINGS | PRINT_SUGGESTION, stderr);
	print_enable(PRINT_RESULT | PRINT_INFO | PRINT_WARNINGS | PRINT_ERRORS | PRINT_SUGGESTION);
}


int print_result(const char *format, ... ) {
	int res = 0;
	if (printer_conf.print & PRINT_RESULT) {
		va_list va;
		va_start(va, format);
		res = vfprintf(printer_conf.result, format, va);
		va_end(va);
	}
	return res;
}

int print_info(const char *format, ... ) {
	int res = 0;
	if (printer_conf.print & PRINT_INFO) {
		va_list va;
		va_start(va, format);
		res = vfprintf(printer_conf.info, format, va);
		va_end(va);
	}
	return res;
}

int print_warnings(const char *format, ... ) {
	int res = 0;
	if (printer_conf.print & PRINT_WARNINGS) {
		va_list va;
		va_start(va, format);
		res = vfprintf(printer_conf.warning, format, va);
		va_end(va);
	}
	return res;
}

int print_errors(const char *format, ... ) {
	int res = 0;
	if (printer_conf.print & PRINT_ERRORS) {
		va_list va;
		va_start(va, format);
		res = vfprintf(printer_conf.error, format, va);
		va_end(va);
	}
	return res;
}

int print_debug(const char *format, ... ) {
	int res = 0;
	if (printer_conf.print & PRINT_DEBUG) {
		va_list va;
		va_start(va, format);
		res = vfprintf(printer_conf.debug, format, va);
		va_end(va);
	}
	return res;
}

int print_suggestion(const char *format, ... ) {
	int res = 0;
	if (printer_conf.print & PRINT_SUGGESTION) {
		va_list va;
		va_start(va, format);
		res = vfprintf(printer_conf.suggestion, format, va);
		va_end(va);
	}
	return res;
}
