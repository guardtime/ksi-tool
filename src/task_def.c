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
	const char *mandatoryFlags;
	const char *ignoredFlags;
	const char *optionalFlags;
	const char *forbittenFlags;
	char *toString;
	bool isDefined;
	bool isConsistent;
	double consistency;
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
 * "h, save, read, r". If nothing is found buf will be empty string "".
 *
 * @param[in]	category	category definition.
 * @param[out]	buf			output buffer for extracted name.
 * @param[in]	len			length of the buf.
 * @return On error or at end of the category function returns NULL. Otherwise pointer to the next name (inside category) will be returned.
 */
static const char *category_getParametersName(const char* category, char *buf, short len){
	int cat_i = 0;
	int buf_i = 0;

	if(buf != NULL)
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

	c = def->mandatoryFlags;
	while((c = category_getParametersName(c,name, sizeof(name))) != NULL){
		size += snprintf(buf+size, len-size, " %s%s", strlen(name)>1 ? "--" : "-", name);
	}

	return buf;
}

void TaskDefinition_new(int id, const char *name,const char *man, const char *ignore, const char *opt, const char *forb, TaskDefinition **new){
	TaskDefinition *tmp = NULL;
	char buf[1024];
	if(new == NULL) return;

	*new = NULL;

	tmp = (TaskDefinition*)malloc(sizeof(TaskDefinition));
	if(tmp == NULL) return;

	tmp->id = id;
	tmp->name = name;
	tmp->mandatoryFlags = (man == NULL) ? "" : man;
	tmp->ignoredFlags = (ignore == NULL) ? "" : ignore;
	tmp->optionalFlags = (opt == NULL) ? "" : opt;
	tmp->forbittenFlags = (forb == NULL) ? "" : forb;
	tmp->toString = NULL;

	tmp->isDefined = false;
	tmp->isConsistent = false;
	tmp->consistency = 0;

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

static bool taskDefinition_isFirstFlagSet(const char* category, paramSet *set){
	char buf[1024];
	category_getParametersName(category, buf, sizeof(buf));
	return paramSet_isSetByName(set, buf);
}

/**
 * This function analysis a task definition according to given parameter set and
 * determines if its consistent. During analysis some fields of the task definition
 * are updated.
 * @return true id task is consistent false otherwise.
 */
static bool taskDefinition_analyseConsistency(TaskDefinition *def, paramSet *set){
	bool state = false;
	int manMissing;
	int manCount;
	double defMan;
	int forMissing;
	int forCount;
	int forbiddentSet;
	double defFor;
	bool isFirstFlagSet;


	if (def == NULL || set == NULL)
		goto cleanup;

	isFirstFlagSet = taskDefinition_isFirstFlagSet(def->mandatoryFlags, set);
	manMissing = TaskDefinition_getMissingFlagCount(def->mandatoryFlags, set);
	manCount = category_getParameterCount(def->mandatoryFlags);
	forMissing = TaskDefinition_getMissingFlagCount(def->forbittenFlags, set);
	forCount = category_getParameterCount(def->forbittenFlags);
	forbiddentSet = forCount - forMissing;

	/**
	 * First flag in category is most important and gives 0.5 of consistency and
	 * another 0.5 comes from all other mandatory flags. Every forbidden flag
	 * reduces the consistency.
	 */
	defMan = 0.5 * (manCount != 0 ? (1.0 - (double)manMissing / (double)manCount) : 0);
	defMan += isFirstFlagSet ? 0.5 : 0;
	defFor = manCount != 0 ? ((double)forbiddentSet / (double)manCount) : 0;

	def->consistency = defMan - defFor;

	if (isFirstFlagSet)
		def->isDefined = true;

	if (manMissing == 0 && forbiddentSet == 0){
		def->isDefined = true;
		def->isConsistent = true;
	} else{
		def->isConsistent = false;
	}

cleanup:

	return def->isConsistent;
}

static void TaskDefinition_PrintErrors(TaskDefinition *def, paramSet *set){
	char buf[256];
	const char *pName = NULL;
	bool def_printed = false;
	bool err_printed = false;
	void (*printError)(const char*, ...) = NULL;



	if(def == NULL){
		fprintf(stderr, "Error: Task is null pointer.\n");
		return;
	}

	if(set == NULL){
		fprintf(stderr, "Error: Parameter set is null pointer.\n");
		return;
	}

	printError = paramSet_getErrorPrinter(set);

	pName = def->mandatoryFlags;
	while((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
		if(paramSet_isSetByName(set, buf) == false){
			if(!def_printed){
				printError("Error: You have to define flag(s) '%s%s'", strlen(buf)>1 ? "--" : "-", buf);
				def_printed = true;
			}
			else{
				printError(", '%s%s'", strlen(buf)>1 ? "--" : "-", buf);
			}
		}
	}
	if(def_printed) printError("\n");


	pName = def->forbittenFlags;
	while((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
		if(paramSet_isSetByName(set, buf) == true){
			if(!err_printed){
				printError("Error: You must not use flag(s) '%s%s'", strlen(buf)>1 ? "--" : "-", buf);
				err_printed = true;
			}
			else{
				printError(", '%s%s'", strlen(buf)>1 ? "--" : "-", buf);
			}
		}
	}
	if(err_printed) printError("\n");

	return;
}

static void TaskDefinition_PrintWarnings(TaskDefinition *def, paramSet *set){
	const char *pName = NULL;
	char buf[256];
	void (*printWarning)(const char*, ...) = NULL;

	if(def == NULL || set == NULL){
		fprintf(stderr, "Error: Task is null pointer.\n");
		return;
	}

	printWarning = paramSet_getWarningPrinter(set);

	pName = def->ignoredFlags;
		while((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
			if(paramSet_isSetByName(set, buf) == true){
				printWarning("Warning: flag %s%s is ignored\n", strlen(buf)>1 ? "--" : "-", buf);
			}
		}
	return;
}

static void TaskDefinition_printSuggestions(TaskDefinition **def, int count, paramSet *set){
	int i;
	TaskDefinition *tmp;
	void (*printError)(const char*, ...) = NULL;

	if(def == NULL || set == NULL || count == 0) return;

	printError = paramSet_getErrorPrinter(set);

	for (i = 0; i < count; i++){
		tmp = def[i];
		if (tmp->isDefined && tmp->consistency >= 0.25 || tmp->consistency >= 0.75)
			printError("Maybe you want to: %s %s\n", tmp->name, tmp->toString);
	}
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

static int taskDefinition_getDefManCount(TaskDefinition *def){
	if (def == NULL)
		return -1;

	return category_getParameterCount(def->mandatoryFlags);
}

bool Task_analyse(TaskDefinition **def, int count, paramSet *set){
	int i=0;
	TaskDefinition *tmp = NULL;
	bool stat = false;

	if(def == NULL || set == NULL) return false;

	for(i=0; i<count; i++){
		tmp = def[i];
		if(taskDefinition_analyseConsistency(tmp, set)){
			stat = true;
		}
	}

	return stat;
}

static TaskDefinition *taskDefinition_getConsistentTask(TaskDefinition **def, int count){
	int i=0;
	TaskDefinition *tmp = NULL;
	TaskDefinition *consistent = NULL;

	for (i = 0; i < count; i++){
		tmp = def[i];
		if (tmp->isConsistent){
			if (consistent == NULL ||
				taskDefinition_getDefManCount(consistent) < taskDefinition_getDefManCount(tmp)){
				consistent = tmp;
			}
		}
	}

	return consistent;
}

Task* Task_getConsistentTask(TaskDefinition **def, int count, paramSet *set){
	int i=0;
	int definedCount = 0;
	const char *pName = NULL;
	char buf[256];
	Task *tmpTask = NULL;
	TaskDefinition *tmp = NULL;
	TaskDefinition *consistent = NULL;
	TaskDefinition *invalidAttempt = NULL;

	if (def == NULL || set == NULL) return NULL;

	consistent = taskDefinition_getConsistentTask(def, count);

	if (consistent){
		if(TaskDefinition_getMissingFlagCount(consistent->ignoredFlags, set) != category_getParameterCount(consistent->ignoredFlags)){
			TaskDefinition_PrintWarnings(consistent, set);
		}

		pName = consistent->ignoredFlags;
		while ((pName = category_getParametersName(pName,buf, sizeof(buf))) != NULL){
			paramSet_removeParameterByName(set, buf);
		}

		Task_new(&tmpTask);
		if (tmpTask == NULL) return NULL;

		tmpTask->def = consistent;
		tmpTask->id = consistent->id;
		tmpTask->set = set;
	}

	return tmpTask;
}

void Task_printError(TaskDefinition **def, int count, paramSet *set){
	int i=0;
	int definedCount = 0;
	const char *pName = NULL;
	Task *tmpTask = NULL;
	TaskDefinition *tmp = NULL;
	TaskDefinition *consistent = NULL;
	TaskDefinition *invalidAttempt = NULL;
	double highestConsistency;
	int maxConsistencyCout;
	bool isSimilarHighConsistencyTasks = false;
	void (*printError)(const char*, ...) = NULL;

	if(def == NULL || set == NULL) return;

	printError = paramSet_getErrorPrinter(set);

	/**
	 * Examine if some task is consistent, find if there is highest consistency
	 * invalid task attempt and what is the highest consistency. Count if there
	 * are multiple highest consistency tasks.
     */
	for (i = 0; i < count; i++){
		tmp = def[i];

		if (tmp->isConsistent)
			consistent = tmp;
		else if (invalidAttempt == NULL || invalidAttempt->consistency < tmp->consistency){
			invalidAttempt = tmp;
			highestConsistency = tmp->consistency;
			maxConsistencyCout = 0;
		}

		if(tmp->consistency == highestConsistency)
			maxConsistencyCout++;

		if (tmp->isDefined)
			definedCount++;
	}

	/**
	 * Examine if there are multiple tasks that have consistency very close to
	 * the highest consistency.
     */
	for (i = 0; i < count; i++){
		tmp = def[i];
		if (tmp->consistency != highestConsistency &&
			tmp->consistency / highestConsistency >= 0.8){
			isSimilarHighConsistencyTasks = true;
		}
	}

	if (definedCount == 0){
		printError("Task is not defined. Use (-x, -s, -v, -p) and read help -h\n");
		TaskDefinition_printSuggestions(def, count, set);
	}else if (definedCount > 1 && consistent == NULL && invalidAttempt->consistency < 0.5 ||
			  maxConsistencyCout > 1 || isSimilarHighConsistencyTasks){
		printError("Task is not fully defined:\n");
		TaskDefinition_printSuggestions(def, count, set);
	}else{
		printError("Error: Task '%s' (%s) is invalid:\n", invalidAttempt->name, invalidAttempt->toString);
		TaskDefinition_PrintErrors(invalidAttempt, set);
		TaskDefinition_PrintWarnings(invalidAttempt, set);
	}

}


int Task_getID(Task *task){
	if(task == NULL) return -1;
	else return task->id;
}

paramSet *Task_getSet(Task *task){
	if(task == NULL) return NULL;
	else return task->set;
}