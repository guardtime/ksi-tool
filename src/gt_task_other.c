#include "gt_task_support.h"
#include "try-catch.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

static void printSignaturesRootHash_and_Time(const KSI_Signature *sig);
static void setSystemTime_throws(const KSI_Signature *sig);

int GT_other(Task *task){
	KSI_CTX *ksi = NULL;
	KSI_DataHasher *hsr = NULL;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	int retval = EXIT_SUCCESS;
	
	bool n,r, d,t;
	
	n = paramSet_isSetByName(task->set, "n");
	r = paramSet_isSetByName(task->set, "r");
	d = paramSet_isSetByName(task->set, "d");
	t = paramSet_isSetByName(task->set, "t");
	
	resetExceptionHandler();
	try
		CODE{
			/*Initalization of KSI */
			initTask_throws(task, &ksi);

			if (task->id == getRootH_T || task->id == setSysTime) {
				KSI_DataHasher_open_throws(ksi, KSI_getHashAlgorithmByName("default"), &hsr);
				KSI_DataHasher_add_throws(ksi, hsr, "tere",4);
				KSI_DataHasher_close_throws(ksi, hsr, &hsh);
				KSI_Signature_create_throws(ksi, hsh, &sig);
				
				if (task->id == getRootH_T) {
					printSignaturesRootHash_and_Time(sig);
				}
				else if (task->id == setSysTime) {
					setSystemTime_throws(sig);
				}
				
			}
			
		}
		CATCH_ALL{
			printErrorMessage();
			retval = _EXP.exep.ret;
			exceptionSolved();
		}
	end_try
	
	if(n) printf("\n");
	if (n) printSignerIdentity(sig);
	
	KSI_DataHasher_free(hsr);
	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
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
	
	printf("[%s]\n%s\n",strTime, buf+1);
	
	return;
	}

static void setSystemTime_throws(const KSI_Signature *sig){
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
	
	res = KSI_Signature_getCalendarAuthRec(sig, &calAuthrec);
	if(res != KSI_OK || calAuthrec == NULL ) THROW_MSG(KSI_EXCEPTION,getReturnValue(res), "Unable to get calendar authentication record");

	res = KSI_CalendarAuthRec_getPublishedData(calAuthrec, &pubData);
	if(res != KSI_OK || pubData == NULL ) THROW_MSG(KSI_EXCEPTION,getReturnValue(res), "Unable to get published data");
	
	res = KSI_PublicationData_getTime(pubData, &time);
	if(res != KSI_OK || time == NULL ) THROW_MSG(KSI_EXCEPTION,getReturnValue(res), "Unable to get time");
	
	pubTm = (time_t)KSI_Integer_getUInt64(time);
	gmtime_r(&pubTm, &tm);
	
#ifdef _WIN32
	newTime.wYear = 1900 + tm.tm_year;
	newTime.wMonth = 1 +tm.tm_mon;
	newTime.wDay = tm.tm_mday;
	newTime.wHour = tm.tm_hour;
	newTime.wMinute = tm.tm_min;
	newTime.wSecond = tm.tm_sec;
	newTime.wMilliseconds = 0;
	
	if(!SetSystemTime(&newTime)){
		DWORD err = GetLastError();
		if(err == ERROR_PRIVILEGE_NOT_HELD){
			THROW_MSG(NO_PRIVILEGES_EXCEPTION,EXIT_NO_PRIVILEGES, "User has no privileges to change date");
		}
		else{
			THROW_MSG(NO_PRIVILEGES_EXCEPTION,EXIT_NO_PRIVILEGES, "Unable to set time");
		}
	}
#else
        tv.tv_sec = mktime(&tm);
        settimeofday(&tv, 0);
#endif
	return;
}