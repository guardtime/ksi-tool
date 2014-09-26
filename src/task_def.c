#include <stdio.h>		//input output
#include <string.h>		
#include <stdlib.h>		//malloc, random, int ja strn 
#include "task_def.h"

typedef struct taskdef_st{
	int id;
	const char *name;
	const char *taskDefinitionFlags;
	const char *mandatoryFlags;
	const char *ignoredFlags;
	const char *optionalFlags;
	const char *forbittenFlags;
	
	bool isDefined;		
	bool isConsistent;
};

void TaskDefinition_new(int id, const char *name, const char *def,const char *man, const char *ignore, const char *opt, const char *forb, TaskDefinition **new){
	TaskDefinition *tmp = NULL;
	
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

	tmp->isDefined = false;
	tmp->isConsistent = false;
	
	*new = tmp;
	return;
}

void TaskDefinition_free(TaskDefinition *obj){
	if(obj == NULL) return;
	free(obj);	
}

static int TaskDefinition_getMissingFlagCount(const char* category, paramSet *set){
	char c;
	int missedFlags = 0;
	if(category == NULL || set == NULL) return -1;
	
	c = *category;
	while((c = *category++)!= 0){
		if(paramSet_isSetByName(set, c) == false){
			missedFlags++;
		}
	}
	return missedFlags;
}

static bool TaskDefinition_analyse(TaskDefinition *def, paramSet *set){
	bool state = true;

	if(def == NULL || set == NULL) return false;
	
//	printf("Is Task defined::\n");
	if(TaskDefinition_getMissingFlagCount(def->taskDefinitionFlags, set) > 0)
		state = false;
	else
		def->isDefined = true;
	
	if(TaskDefinition_getMissingFlagCount(def->mandatoryFlags, set) > 0) state = false;
	if(TaskDefinition_getMissingFlagCount(def->forbittenFlags, set) != strlen(def->forbittenFlags)) state = false;
	
	def->isConsistent = state;
	return state;
}

static void TaskDefinition_PrintErrors(TaskDefinition *def, paramSet *set){
	int i=0;
	char c;
	
	if(def == NULL){
		printf("Task is null pointer.\n");
		return;
	}

	if(set == NULL){
		printf("Parameter set is null pointer.\n");
		return;
	}
	
	if(def->isDefined == false){
		printf("Task '%s' (%s) is not defined.\n", def->name, def->taskDefinitionFlags);
		return;
	}
	
	if(def->isConsistent == false){
		printf("Task '%s' (%s) is invalid:\n", def->name, def->taskDefinitionFlags);

		for(i=0; i<strlen(def->mandatoryFlags); i++){
			c = def->mandatoryFlags[i];
			if(paramSet_isSetByName(set, c) == false){
				printf("Error: You have to define flag -%c\n",c);
			}
		}

		for(i=0; i<strlen(def->forbittenFlags); i++){
			c = def->forbittenFlags[i];
			if(paramSet_isSetByName(set, c) == true){
				printf("Error: You must not use flag -%c\n",c);
			}
		}
	}
	
	return;
}

static void TaskDefinition_PrintWarnings(TaskDefinition *def, paramSet *set){
	int i=0;
	char c;
	
	if(def == NULL){
		printf("Task is null pointer.\n");
		return;
	}

	if(set == NULL){
		printf("Parameter set is null pointer.\n");
		return;
	}

	for(i=0; i<strlen(def->ignoredFlags); i++){
		c = def->ignoredFlags[i];
		if(paramSet_isSetByName(set, c) == true){
			printf("Warning: flag -%c is ignored\n",c);
		}
	}
	return;
}



static bool Task_new(Task **new){
	Task *tmp = NULL;
	
	if(new == NULL) return false;
	*new = NULL;
	
	tmp = (Task*)malloc(sizeof(Task));
	if(tmp == NULL) return false;
	
	tmp->def = NULL;
	tmp->id = noTask;
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
	
	if(definedCount == 0)
		printf("Task is not defined\n");
	
	
	if(definedCount >= 1 && consistent == NULL){
		consistent = NULL;
		if(definedCount > 1)
			printf("You cant define multiple tasks together::\n");
		else
			printf("Taski is invalid::\n");
		
		for(i=0; i<count; i++){
			tmp = def[i];
			if(tmp->isDefined){
				TaskDefinition_PrintErrors(tmp, set);
				TaskDefinition_PrintWarnings(tmp, set);
			}
		}
	}
	
	if(consistent){
		TaskDefinition_PrintWarnings(consistent, set);
		Task_new(&tmpTask);
		if(tmpTask == NULL) return NULL;
		
		tmpTask->def = consistent;
		tmpTask->id = consistent->id;
		tmpTask->set = set;
	}
	
	return tmpTask;
}
