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


/**
 * Try-catch is implemented using setjmp and longjmp. Every time a try block is
 * created system defines a location pointer (setjmp). That location pointer is used to 
 * direct code after THROW (longjmp) to CATCH block defined under earlier 
 * mentioned try block. 
 * 
 * System uses global variable for storing location pointers and error messages.
 * 
 * NB:
 * Try-catch can only be used without multithreading and interrupts.
 * Care must be taken when calling try. It stores the current state of a program
 * to restore it after throw. If some local variables are stored in registers and
 * changed after try the changes will be corrupted. Using volatile keyword can help.
 * 
 * Usage
 * 
 * try
 *      CODE{
 *          }
 *      CATCH(exception)
 *              {
 *              }
 *      CATCHALL{
 *              }
 * end_try
 * 
 *  
 */

#ifndef TRY_CATCH_H
#define	TRY_CATCH_H

#include <setjmp.h>

#define JUMP_DEPTH 64
#define MESSAGE_SIZE 1024
#define FILE_NAME_SIZE 64

#ifdef	__cplusplus
extern "C" {
#endif
	/**
	 * Availabel exceptions
	 */
	typedef enum _exceptions_t{
		_CODE = 0,
		KSI_EXCEPTION = 100,
		NULLPTR_EXCEPTION,
		INVALID_ARGUMENT_EXCEPTION,
		OUT_OF_MEMORY_EXCEPTION,
		NO_PRIVILEGES_EXCEPTION,		
		IO_EXCEPTION
	}exceptions_t;

	/**
	 * Object for storing exception data.
	 */
	typedef struct _exep{
		//Array of messages.
		char message[JUMP_DEPTH][MESSAGE_SIZE];
		//Array of file names and lines where the exception was thrown.
		char fileName[JUMP_DEPTH][FILE_NAME_SIZE];
		int lineNumber[JUMP_DEPTH];
		//Throwing count.
		int N;
		int ret;
		//Exception ID.
		exceptions_t exception;
	} exception;

	/**
	 * Object for exception handler
	 */
	typedef struct _exp_handler{
		jmp_buf array_jmp[JUMP_DEPTH];
		int jump_pos;
		exception exep;
		int isCatched;
		char tmp[MESSAGE_SIZE];
	} exp_handler;

	/**
	 * Global exception handler object.
	 */
	extern exp_handler _EXP;



	/**
	 * Macro for implementing try-catch block.
	 */
	#define try \
		_EXP.jump_pos++; \
		switch(setjmp(_EXP.array_jmp[_EXP.jump_pos-1])){

	/**
	 * Macro for terminating try-catch block.
	 */
	#define end_try } \
		if(!_EXP.isCatched){ \
			_EXP.jump_pos--; \
		} \
		else \
			{_EXP.isCatched=0;} 

	/**
	 * Macro for code block.
	 * 
	 * * @note Must always be first block between try and end_try.
	 */
	#define CODE case 0:

	/**
	 * Macro for catching exception (see THROW()).  Must be used between try and end-try.
	 * 
	 * @param[in] _exception Exception id. 
	 */
	#define CATCH(_exception) break; case _exception: \
		_EXP.isCatched = 1; \
		//printf("Exception %i, catched at level %i.\n", _EXP.exception, _EXP.jump_pos);

	/**
	 * Macro for catching all exceptions.
	 * 
	 * @note Must always be last block between try and end_try.
	 */
	#define CATCH_ALL break; default: \
		_EXP.isCatched = 1; \
		 //printf("Exception %i, catched at level %i.\n", _EXP.exception, _EXP.jump_pos);




	/**
	 * Throws an exception.
	 * 
	 * @param[in] exception Exception id.
	 */
	void THROW(exceptions_t exception, int ret);

	/**
	 * If there is already thrown exceptions, throws it to another catcher.
	 * 
	 * @note USE macros THROW_FORWARD or THROW_FORWARD_APPEND_MESSAGE instead _THROW_FORWARD.
	 */
	void _THROW_FORWARD(void);

	/**
	 * Throws an exption with message + file and line number.
	 *  
	 * @param[in] _exception Exception id.
	 * @param[in] ... Message format strings and parameters.
	 */
	#define THROW_MSG(_exception,_ret, ...){ \
		snprintf(_EXP.tmp, MESSAGE_SIZE, __VA_ARGS__);\
		_appendMessage(_EXP.tmp, __FILE__, __LINE__);\
		THROW(_exception,_ret);}

	/**
	 * Throws already thrown exception forward and appends file name and line.
	 */
	#define THROW_FORWARD() \
		_appendMessage("", __FILE__, __LINE__);  \
		_THROW_FORWARD(); 

	/**
	 * Throws already thrown exception forward and appends message + file name and line.
	 * 
	 * @param[in] ... Message format strings and parameters.
	 */
	#define THROW_FORWARD_APPEND_MESSAGE(...) \
		snprintf(_EXP.tmp, MESSAGE_SIZE, __VA_ARGS__);\
		_appendMessage(_EXP.tmp, __FILE__, __LINE__);\
		_THROW_FORWARD();

	/**
	 * Append message + file name and line to global exception object.
	 * @param msg
	 */
	#define appendMessage(msg) \
		_appendMessage(msg, __FILE__, __LINE__);

	/**
	 * Appends message + file name and line to global exception object. 
	 *      * 
	 * @param[in] msg Message string.
	 * @param[in] fname File name where the function is to be called.
	 * @param[in] lineN Line number where the function is written.
	 */
	void _appendMessage(const char *msg, const char *fname, int lineN);

	/**
	 * Prints messages appended to global exception object.
	 */
	void printErrorMessage(void);

	/**
	 * Prints messages + file names and line numbers where exceptions were thrown.
	 */
	void printErrorLocations(void);

	/**
	 * Reset exception handler.
	 */
	void resetExceptionHandler(void);

	/**
	 * Clean all exceptions. 
	 */
	void exceptionSolved(void);


#ifdef	__cplusplus
}
#endif

#endif	/* TRY_CATCH_H */

