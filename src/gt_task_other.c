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

#include "gt_task_support.h"
#include "ksitool_err.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <errno.h>
#endif

static void printSignaturesRootHash_and_Time(const KSI_Signature *sig);
static int setSystemTime(KSI_CTX *ksi, const KSI_Signature *sig, ERR_TRCKR *err);

int GT_other(Task *task){
	int res;
	paramSet *set = NULL;
	KSI_CTX *ksi = NULL;
	KSI_DataHasher *hsr = NULL;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	int retval = EXIT_SUCCESS;
	ERR_TRCKR *err = NULL;

	bool n, d;

	set = Task_getSet(task);
	n = paramSet_isSetByName(set, "n");
	d = paramSet_isSetByName(set, "d");

	/*Initalization of KSI */
	res = initTask(task, &ksi, &err);
	if (res != KT_OK) goto cleanup;

	if (Task_getID(task) == getRootH_T || Task_getID(task) == setSysTime) {
		res = KSI_DataHasher_open(ksi, KSI_getHashAlgorithmByName("default"), &hsr);
		ERR_CATCH_MSG(err, res, "Error: Unable to open hasher.");
		res = KSI_DataHasher_add(hsr, (void*)task,sizeof(Task*));
		ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
		res = KSI_DataHasher_close(hsr, &hsh);
		ERR_CATCH_MSG(err, res, "Error: %s", errToString(res));
		res = KSI_Signature_create(ksi, hsh, &sig);
		ERR_CATCH_MSG(err, res, "Error: Unable to create signature.");

		if (Task_getID(task) == getRootH_T) {
			printSignaturesRootHash_and_Time(sig);
		}
		else if (Task_getID(task) == setSysTime) {
			res = setSystemTime(ksi, sig, err);
			if (res != KT_OK) goto cleanup;
		}

	}


	if(n) print_info("\n");
	if (n) printSignerIdentity(sig);

cleanup:

	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
	ERR_TRCKR_free(err);
	closeTask(ksi);

	return retval;
}


static void printSignaturesRootHash_and_Time(const KSI_Signature *sig){
	int res;
	KSI_CalendarAuthRec *calAuthrec = NULL;
	KSI_PublicationData *pubData = NULL;
	KSI_DataHash *hsh = NULL;
	KSI_Integer *time = NULL;
	char buf[256];
	char strTime[1024];
	time_t pubTm;
	struct tm tm;

	res = KSI_Signature_getCalendarAuthRec(sig, &calAuthrec);
	if(res != KSI_OK || calAuthrec == NULL ) return;

	res = KSI_CalendarAuthRec_getPublishedData(calAuthrec, &pubData);
	if(res != KSI_OK || pubData == NULL ) return;

	res = KSI_PublicationData_getImprint(pubData, &hsh);
	if(res != KSI_OK || hsh == NULL ) return;

	res = KSI_PublicationData_getTime(pubData, &time);
	if(res != KSI_OK || time == NULL ) return;

	if(KSI_DataHash_toString(hsh, buf, sizeof(buf)) != buf)
		return;

	pubTm = (time_t)KSI_Integer_getUInt64(time);
	gmtime_r(&pubTm, &tm);
	strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", &tm);

	print_result("[%s]\n%s\n",strTime, buf+1);

	return;
	}

static int setSystemTime(KSI_CTX *ksi, const KSI_Signature *sig, ERR_TRCKR *err){
	int res;
	KSI_CalendarAuthRec *calAuthrec = NULL;
	KSI_PublicationData *pubData = NULL;
	KSI_Integer *time = NULL;
	time_t pubTm;
	struct tm tm;

#ifdef _WIN32
	SYSTEMTIME newTime;
#else
	struct timeval tv ={0,0};
#endif

	if (ksi == NULL || sig == NULL || err == NULL) {
		return KT_INVALID_ARGUMENT;
	}


	res = KSI_Signature_getCalendarAuthRec(sig, &calAuthrec);
	ERR_CATCH_MSG(err, res, "Error: Unable to get calendar authentication record.");

	res = KSI_CalendarAuthRec_getPublishedData(calAuthrec, &pubData);
	ERR_CATCH_MSG(err, res, "Error: Unable to get published data.");

	res = KSI_PublicationData_getTime(pubData, &time);
	ERR_CATCH_MSG(err, res, "Error: Unable to get publication time.");

	pubTm = (time_t)KSI_Integer_getUInt64(time);
	gmtime_r(&pubTm, &tm);

#ifdef _WIN32
	newTime.wYear = (WORD)(1900 + tm.tm_year);
	newTime.wMonth = (WORD)(1 +tm.tm_mon);
	newTime.wDay = (WORD)tm.tm_mday;
	newTime.wHour = (WORD)tm.tm_hour;
	newTime.wMinute = (WORD)tm.tm_min;
	newTime.wSecond = (WORD)tm.tm_sec;
	newTime.wMilliseconds = 0;

	if(!SetSystemTime(&newTime)){
		DWORD error = GetLastError();
		if(error == ERROR_PRIVILEGE_NOT_HELD){
			ERR_TRCKR_ADD(err, res = KT_NO_PRIVILEGES, "Error: User has no privileges to change system time.");
			goto cleanup;
		}
		else{
			ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Unable to set system time.");
			goto cleanup;
		}
	}
#else
        tv.tv_sec = mktime(&tm);
        if (settimeofday(&tv, 0) == -1) {
            if (errno == EPERM) {
				ERR_TRCKR_ADD(err, res = KT_NO_PRIVILEGES, "Error: User has no privileges to change system time.");
				goto cleanup;
            } else {
				ERR_TRCKR_ADD(err, res = KT_UNKNOWN_ERROR, "Error: Unable to set system time.");
				goto cleanup;
            }
        }

#endif

	res = KT_OK;

cleanup:

	return res;
}