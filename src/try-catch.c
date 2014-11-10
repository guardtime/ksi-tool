#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "try-catch.h"

exp_handler _EXP;

char *Exeption_toString(exeptions_t e){
	switch(e){
	case KSI_EXEPTION:
		return "KSI exeption";
	case NULLPTR_EXEPTION:
		return "Nullptr exeption";
	case INVALID_ARGUMENT_EXEPTION:
		return "Invalid argument exeption";
	case OUT_OF_MEMORY_EXEPTION:
		return "Out of memory exeption";
	case NO_PRIVILEGES_EXEPTION:
		return "No privileges exeption";
	case IO_EXEPTION:
		return "IO exeption";
	default:
		return "Unknown exeption";
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
		fprintf(stderr, "%s%s", _EXP.exep.message[i], (_EXP.exep.message[i][strlen(_EXP.exep.message[i])-1] == '\n') ? ("") : ("\n"));
	}
	return;
	}

void printErrorLocations(void){
	int i=0;
	for(i=_EXP.exep.N-1; i>=0; i--){
		fprintf(stderr, "%i)[%s] %s (%i) %s%s",i, Exeption_toString(_EXP.exep.exeption), _EXP.exep.fileName[i], _EXP.exep.lineNumber[i],_EXP.exep.message[i],(_EXP.exep.message[i][strlen(_EXP.exep.message[i])-1] == '\n') ? ("") : ("\n") );
	}
	return;
}

void exeptionSolved(void){
	_EXP.exep.N = 0;
	_EXP.exep.exeption = 0;
	return;
}

void resetExeptionHandler(void){
	_EXP.exep.exeption = 0;
	_EXP.jump_pos =0;
	_EXP.exep.N =0;
}

void THROW(exeptions_t exeption){
	if(_EXP.jump_pos > 0){ 
		_EXP.jump_pos--; 
		_EXP.exep.exeption = exeption; 
		//printf("Exeption: %i Thrown at level %i.\n", _EXP.exep.exeption, _EXP.jump_pos); 
		longjmp(_EXP.array_jmp[_EXP.jump_pos],exeption); 
	}
	else{
		fprintf(stderr, "There is no catcher to catch. Nothing was thrown!\n");
	}
}

void _THROW_FORWARD(void){
	THROW(_EXP.exep.exeption);
}