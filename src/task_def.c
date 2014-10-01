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

static const char *getParametersNameFromCateghory(const char* categhory, char *buf, short len){
	int i = 0;
	
	if(categhory == NULL || buf == NULL) return NULL;
	if(categhory[0] == 0) return NULL;
	
	while(categhory[i] != '|' && categhory[i] != 0){
		if(len-1 <= i){
			return NULL;
		}
		buf[i] = categhory[i];
		i++;
	}
	
	buf[i] = 0;
	if(categhory[i] == '|') i++;
	return &categhory[i];
}

static int getFlagCount(const char* categhory){
	const char *c = NULL;
	int count = 0;
	if(categhory == NULL) return 0;
	if(categhory[0] == 0) return 0;
	
	c = categhory;
	while((c = strchr(c, '|'))){
		count++;
		c++;
	}
	return count+1;
}

static int TaskDefinition_getMissingFlagCount(const char* category, paramSet *set){
	int missedFlags = 0;
	const char *pName = NULL;
	char buf[256];

	if(category == NULL || set == NULL) return -1;
	
	pName = category;		
	while((pName = getParametersNameFromCateghory(pName,buf, sizeof(buf))) != NULL){
//		printf("'%s' is %i\n", buf, paramSet_isSetByName(set, buf));
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
	
//	printf("Is Task %s defined::\n", def->name);
	if(TaskDefinition_getMissingFlagCount(def->taskDefinitionFlags, set) > 0)
		state = false;
	else
		def->isDefined = true;
	
//	printf("Is Task consistent fb->%i::\n", getFlagCount(def->forbittenFlags));
	if(TaskDefinition_getMissingFlagCount(def->mandatoryFlags, set) > 0) state = false;
	if(TaskDefinition_getMissingFlagCount(def->forbittenFlags, set) != getFlagCount(def->forbittenFlags)) state = false;
	def->isConsistent = state;
	

	
	
	
	return state;
}

static void TaskDefinition_PrintErrors(TaskDefinition *def, paramSet *set){
	int i=0;
	char buf[256];
	const char *pName = NULL;
	
	if(def == NULL){
		printf("Error: Task is null pointer.\n");
		return;
	}

	if(set == NULL){
		printf("Error: Parameter set is null pointer.\n");
		return;
	}
	
		pName = def->mandatoryFlags;		
		while((pName = getParametersNameFromCateghory(pName,buf, sizeof(buf))) != NULL){
			if(paramSet_isSetByName(set, buf) == false){
				printf("Error: You have to define flag '-%s'\n",buf);
			}
		}
		
		pName = def->forbittenFlags;		
		while((pName = getParametersNameFromCateghory(pName,buf, sizeof(buf))) != NULL){
			if(paramSet_isSetByName(set, buf) == true){
				printf("Error: You must not use flag '-%s'\n",buf);
			}
		}
//	}
	
	return;
}

static void TaskDefinition_PrintWarnings(TaskDefinition *def, paramSet *set){
	int i=0;
	const char *pName = NULL;
	char buf[256];
	
	if(def == NULL){
		printf("Error: Task is null pointer.\n");
		return;
	}

	pName = def->ignoredFlags;		
		while((pName = getParametersNameFromCateghory(pName,buf, sizeof(buf))) != NULL){
			if(paramSet_isSetByName(set, buf) == true){
				printf("Warning: flag -%s is ignored\n",buf);
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
	
	if(definedCount == 0)
		printf("Task is not defined. Use (-x, -s, -v, -p) and read help -h\n");
	
	
	if(definedCount >= 1 && consistent == NULL){
		consistent = NULL;
		if(definedCount > 1)
			printf("Error: You can't define multiple tasks together:\n");
		
		for(i=0; i<count; i++){
			tmp = def[i];
			if(tmp->isDefined){
				printf("Task '%s' (%s) is invalid:\n", tmp->name, tmp->taskDefinitionFlags);
				TaskDefinition_PrintErrors(tmp, set);
				TaskDefinition_PrintWarnings(tmp, set);
			}
		}
	}
	
	if(consistent){
			if(TaskDefinition_getMissingFlagCount(consistent->ignoredFlags, set) != getFlagCount(consistent->ignoredFlags)){
//				printf("Task '%s' (%s) has warnings:\n", consistent->name, consistent->taskDefinitionFlags, );
				TaskDefinition_PrintWarnings(consistent, set);
			}
				
			pName = consistent->ignoredFlags;		
			while((pName = getParametersNameFromCateghory(pName,buf, sizeof(buf))) != NULL){
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
