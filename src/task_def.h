#ifndef TASK_DEF_H
#define	TASK_DEF_H

#include "gt_cmd_common.h"
#include "param_set.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct taskdef_st TaskDefinition;
typedef struct task_st Task;


typedef enum tasks_en{
	downloadPublicationsFile,
	createPublicationString,
	signDataFile,
	signHash,
	extendTimestamp,
	verifyPublicationsFile,
	verifyTimestamp,
	getRootH_T,
	setSysTime,
	showHelp,
	noTask,
	invalid		
} TaskID;

struct task_st{
	TaskDefinition *def;
	TaskID id;
	paramSet *set;
};

void TaskDefinition_new(int id, const char *name, const char *def,const char *man, const char *ignore, const char *opt, const char *forb, TaskDefinition **new);
void TaskDefinition_free(TaskDefinition *obj);
Task* Task_getConsistentTask(TaskDefinition **def, int count, paramSet *set);
void Task_free(Task *obj);
void Task_printSuggestions(TaskDefinition **def, int count, paramSet *set);
#ifdef	__cplusplus
}
#endif

#endif	/* GT_CMDPARAMETERS_H */

