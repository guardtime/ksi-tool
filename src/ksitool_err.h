#ifndef KSITOOL_ERR_H
#define	KSITOOL_ERR_H

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
#define EXIT_AUTH_FAILURE 14


#ifdef	__cplusplus
extern "C" {
#endif

int ksiErrToExitcode(int error_code);



#ifdef	__cplusplus
}
#endif

#endif	/* KSITOOL_ERR_H */

