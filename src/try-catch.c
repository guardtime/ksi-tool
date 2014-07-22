#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "try-catch.h"

exp_array _EXP;



void ResetExeptionHandler(void){
    _EXP.exeption = 0;
    _EXP.jump_pos =0;
    }

void THROW(int exeption){
    if(_EXP.jump_pos > 0){ 
        _EXP.jump_pos--; 
        _EXP.exeption = exeption; 
        printf("Exeption: %i Thrown at level %i.\n", _EXP.exeption, _EXP.jump_pos); 
        longjmp(_EXP.array_jmp[_EXP.jump_pos],exeption); 
        }
    else{
        printf("There is no catcher to catch.\n");
        }
    }

void THROW_FORWARD(void){
    THROW(_EXP.exeption);
    }    




