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

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "try-catch.h"
#include "printer.h"

exp_handler _EXP;

char *Exception_toString(exceptions_t e){
	switch(e){
	case KSI_EXCEPTION:
		return "KSI exception";
	case EXCEPTION:
		return "Exception";
	case NULLPTR_EXCEPTION:
		return "Nullptr exception";
	case INVALID_ARGUMENT_EXCEPTION:
		return "Invalid argument exception";
	case OUT_OF_MEMORY_EXCEPTION:
		return "Out of memory exception";
	case NO_PRIVILEGES_EXCEPTION:
		return "No privileges exception";
	case IO_EXCEPTION:
		return "IO exception";
	default:
		return "Unknown exception";
	}
}

void _appendMessage(const char *msg, const char *fname, int lineN){
	if((_EXP.exep.N +1 ) == JUMP_DEPTH ) return;
	strncpy(_EXP.exep.message[_EXP.exep.N], msg, MESSAGE_SIZE);
	strncpy(_EXP.exep.fileName[_EXP.exep.N], fname, FILE_NAME_SIZE);
	_EXP.exep.lineNumber[_EXP.exep.N] = lineN;
	_EXP.exep.N++;
	return;
	}

void printErrorMessage(void){
	int i=0;
	for(i=_EXP.exep.N-1; i>=0; i--){
		print_errors("%i) %s%s",i+1,  _EXP.exep.message[i], (_EXP.exep.message[i][strlen(_EXP.exep.message[i])-1] == '\n') ? ("") : ("\n"));
	}
	return;
	}

void printErrorLocations(void){
	int i=0;
	for(i=_EXP.exep.N-1; i>=0; i--){
		print_errors("%i)[%s] %s (%i) %s%s",i+1, Exception_toString(_EXP.exep.exception), _EXP.exep.fileName[i], _EXP.exep.lineNumber[i],_EXP.exep.message[i],(_EXP.exep.message[i][strlen(_EXP.exep.message[i])-1] == '\n') ? ("") : ("\n") );
	}
	return;
}

void exceptionSolved(void){
	_EXP.exep.N = 0;
	_EXP.exep.exception = 0;
	return;
}

void resetExceptionHandler(void){
	_EXP.exep.exception = 0;
	_EXP.jump_pos =0;
	_EXP.exep.N =0;
}

void THROW(exceptions_t exception, int ret){
	if(_EXP.jump_pos > 0){
		_EXP.jump_pos--;
		_EXP.exep.exception = exception;
		_EXP.exep.ret = ret;
		//printf("Exception: %i Thrown at level %i.\n", _EXP.exep.exception, _EXP.jump_pos);
		longjmp(_EXP.array_jmp[_EXP.jump_pos],exception);
	}
	else{
		print_errors("There is no catcher to catch. Nothing was thrown!\n");
	}
}

void _THROW_FORWARD(void){
	THROW(_EXP.exep.exception, _EXP.exep.ret);
}