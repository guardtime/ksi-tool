#ifndef KSITOOL_ERR_H
#define	KSITOOL_ERR_H



#ifdef	__cplusplus
extern "C" {
#endif

#define KSITOOL_ERR_BASE 0x100001
#define MAX_MESSAGE_LEN 1024
#define MAX_FILE_NAME_LEN 256
#define MAX_ERROR_COUNT 128
	
typedef struct ERR_TRCKR_st ERR_TRCKR;
	
enum Ksitool_exit {
	EXIT_INVALID_CL_PARAMETERS = 3,
	EXIT_INVALID_FORMAT = 4,
	EXIT_NETWORK_ERROR = 5,
	EXIT_VERIFY_ERROR = 6, 
	EXIT_EXTEND_ERROR = 7, 
	EXIT_AGGRE_ERROR = 8,
	EXIT_IO_ERROR = 9,
	EXIT_CRYPTO_ERROR = 10,
	EXIT_HMAC_ERROR = 11,
	EXIT_NO_PRIVILEGES = 12,
	EXIT_OUT_OF_MEMORY = 13,
	EXIT_AUTH_FAILURE = 14
};

enum Ksitool_errors {
	KT_OK = 0,
	KT_OUT_OF_MEMORY = KSITOOL_ERR_BASE,
	KT_INVALID_ARGUMENT,
	KT_UNABLE_TO_SET_STREAM_MODE,
	KT_IO_ERROR,
	KT_INDEX_OVF,
	KT_INVALID_INPUT_FORMAT,
	KT_HASH_LENGTH_IS_NOT_EVEN,
	KT_INVALID_HEX_CHAR,
	KT_UNKNOWN_HASH_ALG
};	
	
int errToExitCode(int error);
const char* errToString(int error);

ERR_TRCKR *ERR_TRCKR_new(void (*printErrors)(const char*, ...));
void ERR_TRCKR_free(ERR_TRCKR *obj);
void ERR_TRCKR_add(ERR_TRCKR *err, int code, const char *fname, int lineN, const char *msg, ...);
#define ERR_TRCKR_ADD(err, code, msg, ...) ERR_TRCKR_add(err, code, __FILE__, __LINE__, msg, __VA_ARGS__)
void ERR_TRCKR_reset(ERR_TRCKR *err);
void ERR_TRCKR_printErrors(ERR_TRCKR *err);
void ERR_TRCKR_printExtendedErrors(ERR_TRCKR *err);

#ifdef	__cplusplus
}
#endif

#endif	/* KSITOOL_ERR_H */

