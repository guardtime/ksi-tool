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