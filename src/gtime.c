/* 
 * File:   gtime.c
 * Author: Taavi
 *
 * Created on June 18, 2014, 4:25 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "gt_cmd_parameters.h"
#include "gt_task.h"
/*
 * 
 */
int main(int argc, char** argv) {
	bool state = GT_parseCommandline(argc, argv);

	if(state){
		GT_CmdParameters param = GT_getCMDParam();

		if(param.task == downloadPublicationsFile){
			state=GT_getPublicationsFileTask(&param);
		}
		else if (param.task == verifyPublicationsFile){
			state=GT_verifyTask(&param);
		}
		else if (param.task == signDataFile || param.task == signHash){
			state=GT_signTask(&param);
		}
		else if(param.task == extendTimestamp){
			state=GT_extendTask(&param);
		}
		else if(param.task == verifyTimestamp){
			state=GT_verifyTask(&param);
		}
		else if(param.task == showHelp){
			GT_pritHelp();
			state = true;
		}

	}
	else
		return (EXIT_FAILURE);

	return state ? (EXIT_SUCCESS) : (EXIT_FAILURE);
}

