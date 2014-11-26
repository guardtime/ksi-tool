#ifndef GT_CMDCOMMON_H
#define	GT_CMDCOMMON_H


#ifdef _WIN32
#	ifndef snprintf
#		define snprintf _snprintf
#	endif
#	ifndef gmtime_r
#		define gmtime_r(time, resultp) gmtime_s(resultp, time)
#	endif
#	ifdef _DEBUG
#		define _CRTDBG_MAP_ALLOC
#		include <stdlib.h>
#		include <crtdbg.h>
#	endif
#endif

#define EXIT_INVALID_CL_PARAMETERS 3
#define EXIT_INVALID_FORMAT 4
#define EXIT_NETWORK_ERROR 5
#define EXIT_VERIFY_ERROR 6
/*Extending failure*/
#define EXIT_EXTEND_ERROR 7
/*Aggregation failure*/
#define EXIT_AGGRE_ERROR 8	
#define EXIT_IO_ERROR 9
#define EXIT_CRYPTO_ERROR 10
#define EXIT_HMAC_ERROR 11
#define EXIT_NO_PRIVILEGES 12
#define EXIT_OUT_OF_MEMORY 13

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