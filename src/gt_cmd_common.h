#ifndef GT_CMDCOMMON_H
#define	GT_CMDCOMMON_H


#ifdef _WIN32
#	ifndef snprintf
#		define snprintf _snprintf
#	endif
#	ifndef gmtime_r
#		define gmtime_r(time, resultp) gmtime_s(resultp, time)
#	endif
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