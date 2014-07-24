/* 
/**
 * Try-catch is implemented using setjmp and longjmp. Every time a try block is
 * created system defines a location pointer (setjmp). That location pointer is used to 
 * direct code after THROW (longjmp) to CATCH block defined under earlier 
 * mentioned try block. 
 * 
 * System uses global variable for storing location pointers and error messages.
 * 
 * NB:
 * Try-catch can only be used without multithreading and interupts.
 * Care must be taken when calling try. It stores the current state of a program
 * to restore it after throw. If some local variables are stored in registers and
 * changed after try the changes will be corrupted. Using volatile keyword can help.
 * 
 * Usage
 * 
 * try
 *      CODE{
 *          }
 *      CATCH(exeption)
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
     * Availabel exeptions
     */
    typedef enum _exeptions_t{
        _CODE = 0,
        KSI_EXEPTION = 100,
        INVALID_ARGUMENT_EXEPTION, 
        IO_EXEPTION
    }exeptions_t;

    /**
     * Object for storing exeption data.
     */
    typedef struct _exep{
        //Array of messages.
        char message[JUMP_DEPTH][MESSAGE_SIZE];
        //Array of file names and lines where the exeption was thrown.
        char fileName[JUMP_DEPTH][FILE_NAME_SIZE];
        int lineNumber[JUMP_DEPTH];
        //Throwing count.
        int N;
        //Exeption ID.
        exeptions_t exeption;
    } exeption;
    
    /**
     * Object for exeption handler
     */
    typedef struct _exp_handler{
        jmp_buf array_jmp[JUMP_DEPTH];
        int jump_pos;
        exeption exep;
        char tmp[MESSAGE_SIZE];
    } exp_handler;
    
    /**
     * Global exeption handler object.
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
        _EXP.jump_pos--; 

    /**
     * Macro for code block.
     * 
     * * @note Must always be first block between try and end_try.
     */
    #define CODE case 0:

    /**
     * Macro for catching exeption (see THROW()).  Must be used between try and end-try.
     * 
     * @param[in] _exeption Exeption id. 
     */
    #define CATCH(_exeption) break; case _exeption: \
        //printf("Exeption %i, catched at level %i.\n", _EXP.exeption, _EXP.jump_pos);

    /**
     * Macro for catching all exeptions.
     * 
     * @note Must always be last block between try and end_try.
     */
    #define CATCH_ALL break; default: \
         //printf("Exeption %i, catched at level %i.\n", _EXP.exeption, _EXP.jump_pos);

    

    
    /**
     * Throws an exeption.
     * 
     * @param[in] exeption Exeption id.
     */
    void THROW(exeptions_t exeption);
    
    /**
     * If there is already thrown exeptions, throws it to another catcher.
     * 
     * @note USE macros THROW_FORWARD or THROW_FORWARD_APPEND_MESSAGE instead _THROW_FORWARD.
     */
    void _THROW_FORWARD(void);
    
    /**
     * Throws an exption with message + file and line number.
     *  
     * @param[in] _exeption Exeption id.
     * @param[in] ... Message format strings and parameters.
     */
    #define THROW_MSG(_exeption, ...){ \
        snprintf(_EXP.tmp, MESSAGE_SIZE, __VA_ARGS__);\
        _appendMessage(msg, __FILE__, __LINE__);\
        THROW(_exeption);}
    
    /**
     * Throws already thrown exeption forward and appends file name and line.
     */
    #define THROW_FORWARD() \
        _appendMessage("", __FILE__, __LINE__);  \
        _THROW_FORWARD(); 
    
    /**
     * Throws already thrown exeption forward and appends message + file name and line.
     * 
     * @param[in] ... Message format strings and parameters.
     */
    #define THROW_FORWARD_APPEND_MESSAGE(...) \
        snprintf(_EXP.tmp, MESSAGE_SIZE, __VA_ARGS__);\
        _appendMessage(msg, __FILE__, __LINE__);\
        _THROW_FORWARD();
    
    /**
     * Append message + file name and line to global exeption object.
     * @param msg
     */
    #define appendMessage(msg) \
        _appendMessage(msg, __FILE__, __LINE__);

    /**
     * Appends message + file name and line to global exeption object. 
     *      * 
     * @param[in] msg Message string.
     * @param[in] fname File name where the function is to be called.
     * @param[in] lineN Line number where the function is written.
     */
    void _appendMessage(const char *msg, const char *fname, int lineN);
    
    /**
     * Prints messages appended to global exeption object.
     */
    void printMessage(void);
    
    /**
     * Prints messages + file names and line numbers where exeptions were thrown.
     */
    void printErrorLocations(void);
    
    /**
     * Reset exeption handler.
     */
    void ResetExeptionHandler(void);
    
    /**
     * Clean all exeptions. 
     */
    void exeptionSolved(void);
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* TRY_CATCH_H */

