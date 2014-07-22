/* 
 * File:   try-catch.h
 * Author: Taavi
 *
 * Created on July 21, 2014, 11:34 AM
 */

#ifndef TRY_CATCH_H
#define	TRY_CATCH_H

#include <setjmp.h>

/**
 * Macro for implementing try-catch block. When called, jump pointer index is
 * increased and setjmp is called to previous jump pointer index. After that
 * end_try must be called.
 * 
 * Must be used like that:
 * try
 *      CODE{<user code>}
 *      CATCH(id){<user code>}
 *      CATCH_ALL{<user code>}
 * end_try
 * 
 */
#define try \
    _EXP.jump_pos++; \
    switch(setjmp(_EXP.array_jmp[_EXP.jump_pos-1])){

/**
 * Macro for terminating try-catch block. When called, jump pointer index is decreased.
 */
#define end_try } \
    _EXP.jump_pos--; 

/**
 * Macro for catching exeption (see THROW()).  Must be used between try and end-try.
 * Must be used like that CATCH(<id>){<user code>}
 * 
 * @param[in] _exeption Exeption id. 
 */
#define CATCH(_exeption) break; case _exeption: \
    printf("Exeption %i, catched at level %i.\n", _EXP.exeption, _EXP.jump_pos);

/**
 * Macro for catching all exeptions.
 */
#define CATCH_ALL break; default: \
     printf("Exeption %i, catched at level %i.\n", _EXP.exeption, _EXP.jump_pos);

#define THROW_MSG(_exeption, ...){ \
    snprintf(_EXP.expMsg, 256, __VA_ARGS__);\
    THROW(_exeption);}

/**
 * Macro for normal code.
 */
#define CODE case 0:



#define KSI_EXEPTION 100
#define IO_EXEPTION 150
#define A_EXEPTION 80


#ifdef	__cplusplus
extern "C" {
#endif

    #define JUMP_DEPTH 64

    struct _exeption{
        jmp_buf array_jmp[JUMP_DEPTH];
        int jump_pos;
        int exeption;
        char expMsg[256];
    };
    
    typedef struct _exeption exp_array;
    extern exp_array _EXP;
    
    /**
     * Reset exeption handler.
     */
    void ResetExeptionHandler(void);
    
    /**
     * Throw an exeption.
     * @param[in] exeption Exeption id.
     */
    void THROW(int exeption);
    
    /**
     * If there is already thrown exeptions, throw it to another catcher.
     */
    void THROW_FORWARD(void);


    
    
#ifdef	__cplusplus
}
#endif

#endif	/* TRY_CATCH_H */

