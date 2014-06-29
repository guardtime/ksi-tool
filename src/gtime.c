/* 
 * File:   gtime.c
 * Author: Taavi
 *
 * Created on June 18, 2014, 4:25 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "gt_cmdparameters.h"
#include "gt_task.h"
/*
 * 
 */
int main(int argc, char** argv) {
    bool state = GT_parseCommandline(argc, argv);

    
    
    if(state){
        GT_CmdParameters param = GT_getCMDParam();
        GT_Tasks task = GT_getTask();
        
        if(task == downloadPublicationsFile){
            printf("todo download publications file");
            }
        else if (task == verifyPublicationsFile){
            printf("todo verify publications file");
            }
        else if (task == signDataFile || task == signHash){
            GT_signTask(&param, task);
            }
        else if(task == extendTimestamp){
            printf("todo extending signature");
            }
        else if((task == verifyTimestamp_and_file_online) || (task == verifyTimestamp_online) || (task == verifyTimestamp_and_file_use_pubfile) || (task == verifyTimestamp_use_pubfile)){
            printf("todo verify ");
            }
        
        }
    else
        return (EXIT_FAILURE);
 
    return (EXIT_SUCCESS);
}

