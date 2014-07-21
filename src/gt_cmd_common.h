#ifndef GT_CMDCOMMON_H
#define	GT_CMDCOMMON_H

#define DEFAULT_S_URL "192.168.1.36:3333/"
#define DEFAULT_X_URL "192.168.1.29:1111/gt-extendingservice"
#define DEFAULT_P_URL "172.20.20.7/publications.tlv"

#ifdef _WIN32 
#   define snprintf _snprintf
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef __cplusplus
typedef enum { false=0, true=1 } bool;
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* GTCMDCOMMON_H */



#define _TRY { \
    int _res = KSI_UNKNOWN_ERROR; \
    char __msg[256]; \
    do \


#define _CATCH while(0); if(_res != KSI_OK)

#define _TEST(test) if(test) break;

#define _TEST_COMPLAIN(test, ...) \
    if(test) {\
    snprintf(__msg, 256, __VA_ARGS__);\
        break; \
    }

#define _DO_TEST(_do) _res = _do; \
    if(_res != KSI_OK) break;

#define _DO_TEST_COMPLAIN(_do, ...) _res = _do; \
    if(_res != KSI_OK) {\
    snprintf(__msg, 256, __VA_ARGS__);\
        break; \
    }

