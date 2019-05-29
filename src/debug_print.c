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

#include "debug_print.h"
#include <string.h>
#include "printer.h"
#include "obj_printer.h"
#include "param_set/param_set.h"
#include "tool_box.h"
#include "ksi/compatibility.h"
#include "ksitool_err.h"

#ifdef _WIN32
#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#include <stdlib.h>
#else
#   include <limits.h>
#	include <sys/time.h>
#endif

void DEBUG_verifySignature(KSI_CTX *ksi, int res, KSI_Signature *sig, KSI_PolicyVerificationResult *result, KSI_DataHash *hsh) {
	KSI_PublicationsFile *pubFile = NULL;
	KSI_DataHash *input_hash = NULL;


	if (ksi == NULL || sig == NULL) return;

	if (hsh != NULL) {
		res = KSI_Signature_getDocumentHash(sig, &input_hash);
		if (res != KSI_OK) {
			print_errors("Error: Unable to extract signatures input hash.\n");
		} else if (!KSI_DataHash_equals(hsh, input_hash)) {
			OBJPRINT_Hash(hsh,        "Document hash:       ", print_debug);
			OBJPRINT_Hash(input_hash, "Expected Input hash: ", print_debug);
		}
	}

	if (result) {
		if (!strcmp(result->finalResult.ruleName, "KSI_VerificationRule_PublicationsFilePublicationTimeMatchesExtenderResponse") ||
			!strcmp(result->finalResult.ruleName, "KSI_VerificationRule_PublicationsFilePublicationHashMatchesExtenderResponse") ||
			!strcmp(result->finalResult.ruleName, "KSI_VerificationRule_PublicationsFileExtendedSignatureInputHash")) {
			res = KSI_CTX_getPublicationsFile(ksi, &pubFile);
			if (res == KSI_OK && pubFile != NULL) {
				OBJPRINT_publicationsFileCertificates(pubFile, print_debug);
			}
		}
	}
	print_debug("\n");
}

void DEBUG_verifyPubfile(KSI_CTX *ksi, PARAM_SET *set, int res, KSI_PublicationsFile *pub) {
	char *constraint = NULL;
	unsigned i = 0;

	if (ksi == NULL || pub == NULL) return;


	if (res == KSI_PKI_CERTIFICATE_NOT_TRUSTED || res == KSI_INVALID_PKI_SIGNATURE) {
		OBJPRINT_publicationsFileSigningCert(pub, print_debug);

		if (PARAM_SET_isSetByName(set, "cnstr")) {
			print_debug("Expected publications file PKI certificate constraints:\n");
		}

		while (PARAM_SET_getObj(set, "cnstr", NULL, PST_PRIORITY_HIGHEST, i++, (void**)&constraint) == PST_OK) {
			char OID[1204];
			char value[1204];
			char *ret = NULL;

			ret = STRING_extractRmWhite(constraint, NULL, "=", OID, sizeof(OID));
			if (ret != OID) continue;

			ret = STRING_extractRmWhite(constraint, "=", NULL, value, sizeof(value));
			if (ret != value) continue;

			print_debug("  * %s = '%s'\n", OID_getShortDescriptionString(OID), value);
		}

	}
}

static unsigned int elapsed_time_ms;
static int inProgress = 0;
static int timerOn = 0;


static unsigned int measureLastCall(void){
#ifdef _WIN32
	static LARGE_INTEGER thisCall;
	static LARGE_INTEGER lastCall;
	LARGE_INTEGER frequency;        // ticks per second

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&thisCall);

	elapsed_time_ms = (unsigned)((thisCall.QuadPart - lastCall.QuadPart) * 1000.0 / frequency.QuadPart);
#else
	static struct timeval thisCall = {0, 0};
	static struct timeval lastCall = {0, 0};

	gettimeofday(&thisCall, NULL);

	elapsed_time_ms = (unsigned)((thisCall.tv_sec - lastCall.tv_sec) * 1000.0 + (thisCall.tv_usec - lastCall.tv_usec) / 1000.0);
#endif

	lastCall = thisCall;
	return elapsed_time_ms;
}

void print_progressDesc(int showTiming, const char *msg, ...) {
	va_list va;
	char buf[1024];


	if (inProgress == 0) {
		inProgress = 1;
		/*If timing info is needed, then measure time*/
		if (showTiming == 1) {
			timerOn = 1;
			measureLastCall();
		}

		va_start(va, msg);
		KSI_vsnprintf(buf, sizeof(buf), msg, va);
		buf[sizeof(buf) - 1] = 0;
		va_end(va);

		print_debug("%s", buf);
	}
}

void print_progressResult(int res) {
	static char time_str[32];

	if (inProgress == 1) {
		inProgress = 0;

		if (timerOn == 1) {
			measureLastCall();

			KSI_snprintf(time_str, sizeof(time_str), " (%i ms)", elapsed_time_ms);
			time_str[sizeof(time_str) - 1] = 0;
		}

		if (res == KT_OK) {
			print_debug("ok.%s\n", timerOn ? time_str : "");
		} else if (res == KT_VERIFICATION_INCONCLUSIVE) {
			print_debug("na.%s\n", timerOn ? time_str : "");
		} else {
			print_debug("failed.%s\n", timerOn ? time_str : "");
		}

		timerOn = 0;
	}
}

int PROGRESS_BAR_display(int progress) {
	char buf[65] = "################################################################";
	int count = progress * 64 / 100;

	print_debug("\r");
	print_debug("[%-*.*s]", 64, count, buf);

	return 0;
}