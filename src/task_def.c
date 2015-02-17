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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "task_def.h"

struct taskdef_st{
	int id;
	const char *name;
	const char *taskDefinitionFlags;
	const char *mandatoryFlags;
	const char *ignoredFlags;
	const char *optionalFlags;
	const char *forbittenFlags;
	char *toString;
	bool isDefined;		
	bool isConsistent;
};

struct task_st{
	TaskDefinition *def;
	int id;
	paramSet *set;
};


/**
 * Extracts one name frome category, formatted as "<name><ndn><name><ndn><name>",
 * where <name> is parameter that belongs to category and <ndn> is character
 * that is not a digit or letter (@ref #isalnum will return 0). For example
 * "h, save, read, r".
 *  
 * @param[in]	category	category definition.
 * @param[out]	buf			output buffer for extracted name.
 * @param[in]	len			length of the buf.
 * @return On error or at end of the category function returns NULL. Otherwise pointer to the next name (inside category) will be returned.
 */
static const char *category_getParametersName(const char* category, char *buf, short len){
	int cat_i = 0;
	int buf_i = 0;
	
	buf[0] = 0;
	
	if(category == NULL || buf == NULL || category[0] == 0) return NULL;
	
	/*Scan category for first name*/
	while(category[cat_i] != 0){
		if(len-1 <= buf_i) return NULL;
		
		if(isalnum(category[cat_i]))
			buf[buf_i++] = category[cat_i];
		else if(buf_i>0)
			break;
		
		cat_i++;
	}
	
	buf[buf_i] = 0;
	return &category[cat_i];
}

static int category_getParameterCount(const char* categhory){
	const char *c = categhory;
	char buf[256];
	int count = 0;
	if(categhory == NULL) return 0;
	
	while((c = category_getParametersName(c,buf, sizeof(buf))) != NULL)
		count++;

	return count;
}


static char *TaskDefinition_toString(TaskDefinition *def, char *buf, int len){
	const char *c = NULL;
	char name[256];
	int size = 0;
	if(def == NULL || buf == NULL || len < 0) return NULL;
	
	c = def->taskDefinitionFlags;
	while((c = category_getParametersName(c,name, sizeof(name))) != NULL){
		size += snprintf(buf+size, len-size, "%s%s%s", size > 0 ? " " : "", strlen(name)>1 ? "--" : "-", name);
	}
	c = def->mandatoryFlags;
	while((c = category_getParametersName(c,name, sizeof(name))) != NULL){
		size += snprintf(buf+size, len-size, " %s%s", strlen(name)>1 ? "--" : "-", name);
	}
	
	return buf;
}

void TaskDefinition_new(int id, const char *name, const char *def,const char *man, const char *ignore, const char *opt, const char *forb, TaskDefinition **new){
	TaskDefinition *tmp = NULL;
	char buf[1024];
	if(new == NULL) return;
	
	*new = NULL;
	
	tmp = (TaskDefinition*)malloc(sizeof(TaskDefinition));
	if(tmp == NULL) return;
	
	tmp->id = id;
	tmp->name = name;
	tmp->taskDefinitionFlags = (def == NULL) ? "" : def;
	tmp->mandatoryFlags = (man == NULL) ? "" : man;
	tmp->ignoredFlags = (ignore == NULL) ? "" : ignore;
	tmp->optionalFlags = (opt == NULL) ? "" : opt;
	tmp->forbittenFlags = (forb == NULL) ? "" : forb;
	tmp->toString = NULL;
	
	tmp->isDefined = false;
	tmp->isConsistent = false;
	
	
	if(TaskDefinition_toString(tmp, buf, sizeof(buf)) != NULL){
		tmp->toString = (char*)malloc(sizeof(char)*(strlen(buf) + 1));
		if(tmp->toString != NULL)
			strcpy(tmp->toString, buf);
	}	
	
	*new = tmp;
	return;
}

void TaskDefinition_free(TaskDefinition *obj){
	if(obj == NULL) return;
	free(obj->toString);
	free(obj);	
}

static int TaskDefinition_getMissingFlagCount(const char* category, paramSet *set){
	int missedFlags = 0;
	const char *pName = NULL;
	char buf[256];

	if(category == NULL || set == NULL) return -1;
	
	pName = category;		
	while((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
		if(paramSet_isSetByName(set, buf) == false){
			missedFlags++;
		}
	}
	
	return missedFlags;
}

static bool TaskDefinition_analyse(TaskDefinition *def, paramSet *set){
	bool state = true;
	const char *pName = NULL;
	if(def == NULL || set == NULL) return false;
	
	if(TaskDefinition_getMissingFlagCount(def->taskDefinitionFlags, set) > 0)
		state = false;
	else
		def->isDefined = true;
	
	if(TaskDefinition_getMissingFlagCount(def->mandatoryFlags, set) > 0) state = false;
	if(TaskDefinition_getMissingFlagCount(def->forbittenFlags, set) != category_getParameterCount(def->forbittenFlags)) state = false;
	def->isConsistent = state;
	
	return state;
}

static void TaskDefinition_PrintErrors(TaskDefinition *def, paramSet *set){
	int i=0;
	char buf[256];
	const char *pName = NULL;
	bool def_printed = false;
	bool err_printed = false;
	
	if(def == NULL){
		fprintf(stderr, "Error: Task is null pointer.\n");
		return;
	}

	if(set == NULL){
		fprintf(stderr, "Error: Parameter set is null pointer.\n");
		return;
	}
	
	pName = def->mandatoryFlags;		
	while((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
		if(paramSet_isSetByName(set, buf) == false){
			if(!def_printed){
				fprintf(stderr, "Error: You have to define flag(s) '%s%s'", strlen(buf)>1 ? "--" : "-", buf);
				def_printed = true;
			}
			else{
				fprintf(stderr, ", '%s%s'", strlen(buf)>1 ? "--" : "-", buf);
			}
		}
	}
	if(def_printed) fprintf(stderr, "\n");


	pName = def->forbittenFlags;		
	while((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
		if(paramSet_isSetByName(set, buf) == true){
			if(!err_printed){
				fprintf(stderr, "Error: You must not use flag(s) '%s%s'", strlen(buf)>1 ? "--" : "-", buf);
				err_printed = true;
			}
			else{
				fprintf(stderr, ", '%s%s'", strlen(buf)>1 ? "--" : "-", buf);
			}
		}
	}
	if(err_printed) fprintf(stderr, "\n");
	
	return;
}

static void TaskDefinition_PrintWarnings(TaskDefinition *def, paramSet *set){
	int i=0;
	const char *pName = NULL;
	char buf[256];
	
	if(def == NULL){
		fprintf(stderr, "Error: Task is null pointer.\n");
		return;
	}

	pName = def->ignoredFlags;		
		while((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
			if(paramSet_isSetByName(set, buf) == true){
				fprintf(stderr, "Warning: flag %s%s is ignored\n", strlen(buf)>1 ? "--" : "-", buf);
			}
		}
	return;
}

static void TaskDefinition_printSuggestions(TaskDefinition **def, int count, paramSet *set){
	int i;
	TaskDefinition *tmp;
	char first_flag[128];
	int missing;
	int all;
	unsigned char *matchChart = NULL;
	unsigned min = 101;
	
	matchChart = malloc(count*sizeof(unsigned char));
	if(matchChart == NULL) return;
	
	
	for(i=0; i<count; i++){
		tmp = def[i];
		
		all = category_getParameterCount(tmp->taskDefinitionFlags )+category_getParameterCount(tmp->mandatoryFlags);
		missing = TaskDefinition_getMissingFlagCount(tmp->taskDefinitionFlags, set)+TaskDefinition_getMissingFlagCount(tmp->mandatoryFlags, set);
		category_getParametersName(tmp->taskDefinitionFlags, first_flag, sizeof(first_flag));
		
		matchChart[i] = (100*missing)/all;
		min = matchChart[i] < min ? matchChart[i] : min; 
	}
	
	if(min == 100) return;
	
	for(i=0; i<count; i++){
		tmp = def[i];
		if(matchChart[i] == min)
			fprintf(stderr, "Maybe you want to: %s %s\n", tmp->name, tmp->toString);
	}
	
	free(matchChart);
}



static bool Task_new(Task **new){
	Task *tmp = NULL;
	
	if(new == NULL) return false;
	*new = NULL;
	
	tmp = (Task*)malloc(sizeof(Task));
	if(tmp == NULL) return false;
	
	tmp->def = NULL;
	tmp->id = 0;
	tmp->set = NULL;
	
	*new = tmp;
	return true;
}

void Task_free(Task *obj){
	if(obj == NULL) return;
	free(obj);
}

Task* Task_getConsistentTask(TaskDefinition **def, int count, paramSet *set){
	int i=0;
	int definedCount = 0;
	const char *pName = NULL;
	char buf[256];
	Task *tmpTask = NULL;
	TaskDefinition *tmp = NULL;
	TaskDefinition *consistent = NULL;
	
	if(def == NULL || set == NULL) return NULL;
	
	for(i=0; i<count; i++){
		tmp = def[i];
		if(TaskDefinition_analyse(tmp, set)){
			consistent = tmp;
		}
		if(tmp->isDefined) definedCount++;
	}
	
	if(definedCount == 0){
		fprintf(stderr, "Task is not defined. Use (-x, -s, -v, -p) and read help -h\n");
		TaskDefinition_printSuggestions(def, count, set);
	}
	
	
	if(definedCount >= 1 && consistent == NULL){
		consistent = NULL;
		if(definedCount > 1){
			fprintf(stderr, "Task is not fully defined:\n");
			TaskDefinition_printSuggestions(def, count, set);
		}
		else{
			for(i=0; i<count; i++){
				tmp = def[i];
				if(tmp->isDefined){
					fprintf(stderr, "Error: Task '%s' (%s) is invalid:\n", tmp->name, tmp->toString);
					TaskDefinition_PrintErrors(tmp, set);
					TaskDefinition_PrintWarnings(tmp, set);
					break;
				}
			}
		}
		
	}
	
	if(consistent){
		if(TaskDefinition_getMissingFlagCount(consistent->ignoredFlags, set) != category_getParameterCount(consistent->ignoredFlags)){
			TaskDefinition_PrintWarnings(consistent, set);
		}

		pName = consistent->ignoredFlags;		
		while((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
			paramSet_removeParameterByName(set, buf);
		}

		Task_new(&tmpTask);
		if(tmpTask == NULL) return NULL;

		tmpTask->def = consistent;
		tmpTask->id = consistent->id;
		tmpTask->set = set;
	}
	
	return tmpTask;
}

int Task_getID(Task *task){
	if(task == NULL) return -1;
	else return task->id;
}

paramSet *Task_getSet(Task *task){
	if(task == NULL) return NULL;
	else return task->set;
}