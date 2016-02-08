/* 
 * File:   default_tasks.h
 * Author: Taavi
 *
 * Created on February 4, 2016, 11:44 AM
 */

#ifndef DEFAULT_TASKS_H
#define	DEFAULT_TASKS_H

#ifdef	__cplusplus
extern "C" {
#endif

int sign_run(int argc, char** argv, char **envp);
char *sign_help_toString(char*buf, size_t len);
const char *sign_get_desc(void);



#ifdef	__cplusplus
}
#endif

#endif	/* DEFAULT_TASKS_H */

