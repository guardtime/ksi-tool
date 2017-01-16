/*
 * Copyright 2013-2016 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "task_def.h"
#include "param_set.h"
#include "parameter.h"
#include "param_set_obj_impl.h"
#include "strn.h"

#define debug_array_printf 	{int n; for (n = 0; n < task_set->count; n++) {printf("[%2.2f:%2i]", task_set->cons[n], task_set->index[n]);}printf("\n");}


static int new_string(const char *str, char **out) {
	char *tmp = NULL;
	if (out == NULL) return PST_INVALID_ARGUMENT;

	if (str == NULL) {
		*out = NULL;
		return PST_OK;
	}

	tmp = (char*)malloc(strlen(str)*sizeof(char) + 1);
	if (tmp == NULL) return PST_OUT_OF_MEMORY;

	strcpy(tmp, str);
	*out = tmp;
	return PST_OK;
}

static int isValidNameChar(int c) {
	if (c == ',' || isspace(c)) return 0;
	else return 1;
}

static const char* category_extract_name(const char* category, char *buf, short len, int *flags){
	return extract_next_name(category, isValidNameChar, buf, len, flags);
}

static int category_get_parameter_count(const char* categhory){
	const char *name = categhory;
	char buf[256];
	int count = 0;
	if (categhory == NULL) return 0;

	while ((name = category_extract_name(name ,buf, sizeof(buf), NULL)) != NULL)
		count++;

	return count;
}

static int category_get_missing_flag_count(const char* category, PARAM_SET *set){
	int missedFlags = 0;
	const char *pName = NULL;
	char buf[256];

	if (set == NULL) return -1;

	pName = category;
	while ((pName = category_extract_name(pName, buf, sizeof(buf), NULL)) != NULL){
		if (!PARAM_SET_isSetByName(set, buf)){
			missedFlags++;
		}
	}
	return missedFlags;
}

static int task_definition_getAtLeastOneSetMetrica(TASK_DEFINITION *def, PARAM_SET *set, int *count, int *missing){
	int atleastOneOfCount = 0;
	int atleastOneOfMissing = 0;

	if (def == NULL || set == NULL) {
		return PST_INVALID_ARGUMENT;
	}

	if (category_get_parameter_count(def->atleast_one) > 0) {
		if (category_get_parameter_count(def->atleast_one) - category_get_missing_flag_count(def->atleast_one, set) == 0) {
			atleastOneOfCount = 1;
			atleastOneOfMissing = 1;
		} else {
			atleastOneOfCount = category_get_parameter_count(def->atleast_one) - category_get_missing_flag_count(def->atleast_one, set);
			atleastOneOfMissing = 0;
		}
	}

	if (count != NULL) {
		*count = atleastOneOfCount;
	}

	if (missing != NULL) {
		*missing = atleastOneOfMissing;
	}
	return PST_OK;
}

static int TASK_new(TASK_DEFINITION *pDef, PARAM_SET *pSet, TASK **new){
	TASK *tmp = NULL;

	if (new == NULL) return PST_INVALID_ARGUMENT;

	tmp = (TASK*)malloc(sizeof(TASK));
	if (tmp == NULL) return PST_OUT_OF_MEMORY;

	tmp->def = pDef;
	tmp->id = pDef->id;
	tmp->set = pSet;

	*new = tmp;
	return PST_OK;
}

static void TASK_free(TASK *obj){
	if (obj == NULL) return;
	free(obj);
}

int TASK_getID(TASK *task){
	if (task == NULL) return -1;
	else return task->id;
}

const char* TASK_getName(TASK *task){
	if (task == NULL) return NULL;
	else return task->def->name;
}

PARAM_SET *TASK_getSet(TASK *task) {
	if (task == NULL) return NULL;
	else return task->set;
}

TASK_DEFINITION *TASK_getDef(TASK *task) {
	if (task == NULL) return NULL;
	else return task->def;
}


int TASK_DEFINITION_new(int id, const char *name, const char *man, const char *atleastone, const char *forb, const char *ignore, TASK_DEFINITION **new) {
	int res;
	TASK_DEFINITION *tmp = NULL;
	char buf[1024];

	if (name == NULL || man == NULL || new == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Construct new and empty task definition data structure.
	 */
	tmp = (TASK_DEFINITION*)malloc(sizeof(TASK_DEFINITION));
	if (tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->name = NULL;
	tmp->mandatory = NULL;
	tmp->atleast_one = NULL;
	tmp->forbitten = NULL;
	tmp->ignore = NULL;
	tmp->toString = NULL;
	tmp->isAnalyzed = 0;

	/**
	 * Initialize data structure.
	 */
	res = new_string(name, &tmp->name);
	if (res != PST_OK) goto cleanup;

	res = new_string(man, &tmp->mandatory);
	if (res != PST_OK) goto cleanup;

	res = new_string(atleastone, &tmp->atleast_one);
	if (res != PST_OK) goto cleanup;

	res = new_string(forb, &tmp->forbitten);
	if (res != PST_OK) goto cleanup;

	res = new_string(ignore,  &tmp->ignore);
	if (res != PST_OK) goto cleanup;

	tmp->id = id;
	tmp->isConsistent = 0;
	tmp->consistency = 0;

	/**
	 * TODO: check if make sens.
	 * Create a string representation of the task.
	 */
	if (TASK_DEFINITION_toString(tmp, buf, sizeof(buf)) != NULL){
		res = new_string(buf, &tmp->toString);
		if (res != PST_OK) goto cleanup;
	}

	*new = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	TASK_DEFINITION_free(tmp);

	return res;
}

void TASK_DEFINITION_free(TASK_DEFINITION *obj){
	if (obj == NULL) return;

	if (obj->ignore != NULL) free(obj->ignore);
	if (obj->forbitten != NULL) free(obj->forbitten);
	if (obj->atleast_one != NULL) free(obj->atleast_one);
	if (obj->mandatory != NULL) free(obj->mandatory);
	if (obj->name != NULL) free(obj->name);
	if (obj->toString != NULL) free(obj->toString);

	free(obj);
}

int TASK_DEFINITION_analyzeConsistency(TASK_DEFINITION *def, PARAM_SET *set, double *cons){
	int res;
	int manMissing;
	int manCount;
	double defMan;
	int forMissing;
	int forCount;
	int forbiddentSet;
	double defFor;
	int atleastOneOfMissing = 0;
	int atleastOneOfCount = 0;


	if (def == NULL || set == NULL || cons == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (def->isAnalyzed) {
		*cons = def->consistency;
		res = PST_OK;
		goto cleanup;
	}

	res = task_definition_getAtLeastOneSetMetrica(def, set, &atleastOneOfCount, &atleastOneOfMissing);
	if (res != PST_OK) goto cleanup;

	manMissing = category_get_missing_flag_count(def->mandatory, set) + atleastOneOfMissing;
	manCount = category_get_parameter_count(def->mandatory) + atleastOneOfCount;

	forMissing = category_get_missing_flag_count(def->forbitten, set);
	forCount = category_get_parameter_count(def->forbitten);

	forbiddentSet = forCount - forMissing;

//	printf("M %i/%i F %i/%i\n", manMissing, manCount, forMissing, forCount);

	/**
	 * First flag in category is most important and gives 0.5 of consistency and
	 * another 0.5 comes from all other mandatory flags. Every forbidden flag
	 * reduces the consistency.
	 */
	defMan = (manCount != 0 ? (1.0 - (double)manMissing / (double)manCount) : 0);
	defFor = manCount != 0 ? ((double)forbiddentSet / (double)manCount) : 0;

	def->consistency = defMan - defFor;

//	printf(">>> consistency (%i %s) %f\n",def->id, def->name, def->consistency);

	if (manMissing == 0 && forbiddentSet == 0){
		def->isConsistent = 1;
	} else{
		def->isConsistent = 0;
	}
	def->isAnalyzed = 1;
	*cons = def->consistency;

cleanup:

	return res = PST_OK;
}

int TASK_DEFINITION_getMoreConsistent(TASK_DEFINITION *A, TASK_DEFINITION *B, PARAM_SET *set, double sensitivity, TASK_DEFINITION **result) {
	int res;
	TASK_DEFINITION *tmp = NULL;
	double consisteny_A = 0;
	double consisteny_B = 0;
	const char *pNameA;
	const char *pNameB;
	char bufA[1024];
	char bufB[1024];
	int A_different_set_flag_count = 0;
	int B_different_set_flag_count = 0;
	int atlmissing;
	int atlcount;

	if (A == NULL || B == NULL || set == NULL || result == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	// TODO: refactor and make algorithm better.

	/**
	 * Analyse consistency of task definition A and B.
	 */
	res = TASK_DEFINITION_analyzeConsistency(A, set, &consisteny_A);
	if (res != PST_OK) goto cleanup;

	res = TASK_DEFINITION_analyzeConsistency(B, set, &consisteny_B);
	if (res != PST_OK) goto cleanup;


	/**
	 * If two tasks have very similar consistency, examine two tasks and select
	 * the task that is more consistent.
	 */
	if (fabs(consisteny_A - consisteny_B) <= sensitivity) {
		/**
		 * Get the count of parameters that are set and do NOT exist under both
		 * definitions. Include also at least one of category.
		 */
		res = task_definition_getAtLeastOneSetMetrica(A, set, &atlcount, &atlmissing);
		if (res != PST_OK) goto cleanup;
		A_different_set_flag_count = category_get_parameter_count(A->mandatory) + atlcount
				- category_get_missing_flag_count(A->mandatory, set) - atlmissing;

		res = task_definition_getAtLeastOneSetMetrica(B, set, &atlcount, &atlmissing);
		if (res != PST_OK) goto cleanup;
		B_different_set_flag_count = category_get_parameter_count(B->mandatory) + atlcount
				- category_get_missing_flag_count(B->mandatory, set) - atlmissing;

		pNameA = A->mandatory;
		while ((pNameA = category_extract_name(pNameA, bufA, sizeof(bufA), NULL)) != NULL){
			pNameB = B->mandatory;
			while ((pNameB = category_extract_name(pNameB, bufB, sizeof(bufB), NULL)) != NULL){
				/* If flags are bot set and exist in A and B, decrement the count*/
				if (strcmp(bufA, bufB) == 0 && PARAM_SET_isSetByName(set, bufA) && PARAM_SET_isSetByName(set, bufB)) {
						A_different_set_flag_count--;
						B_different_set_flag_count--;
//						printf("%s--\n", bufA);
				}
			}
		}

		pNameA = A->atleast_one;
		while ((pNameA = category_extract_name(pNameA, bufA, sizeof(bufA), NULL)) != NULL){
			pNameB = B->atleast_one;
			while ((pNameB = category_extract_name(pNameB, bufB, sizeof(bufB), NULL)) != NULL){
				/* If flags are bot set and exist in A and B, decrement the count*/
				if (strcmp(bufA, bufB) == 0 && PARAM_SET_isSetByName(set, bufA) && PARAM_SET_isSetByName(set, bufB)) {
						A_different_set_flag_count--;
						B_different_set_flag_count--;
//						printf("%s--\n", bufA);
				}
			}
		}

		if ((double)A_different_set_flag_count * A->consistency == (double)B_different_set_flag_count * B->consistency) {
			tmp = NULL;
		} else {
			tmp = ((double)A_different_set_flag_count * A->consistency > (double)B_different_set_flag_count * B->consistency) ? A : B;
//			printf("%s %i compared to %s %i, more consistent is %s %i\n", A->name, A->id, B->name, B->id, tmp->name, tmp->id);
//			printf("A: %f and B: %f\n", (double)A_different_set_flag_count * A->consistency , (double)B_different_set_flag_count * B->consistency);
		}
	} else if (consisteny_A == consisteny_B || fabs(consisteny_A - consisteny_B) >= 0.00000000001) {
		tmp = NULL;
	} else {
		tmp = consisteny_A > consisteny_B ? A : B;
	}

	*result = tmp;
	res = PST_OK;

cleanup:
	return res;
}

char* TASK_DEFINITION_toString(TASK_DEFINITION *def, char *buf, size_t buf_len) {
	const char *c = NULL;
	char name[256];
	size_t count = 0;
	int round = 0;

	if (def == NULL || buf == NULL || buf_len == 0) return NULL;

	c = def->mandatory;
	while ((c = category_extract_name(c, name, sizeof(name), NULL)) != NULL) {
		count += PST_snprintf(buf + count, buf_len - count, "%s%s%s",
				round == 0 ? "" : " ",
				strlen(name)>1 ? "--" : "-",
				name);
		round++;
	}

	c = def->atleast_one;
	if (c != NULL && c[0] != '\0') {
		count += PST_snprintf(buf+count, buf_len - count, "%sone or more of (",
				(round == 0) ? "" : " ");
		round = 0;

		while ((c = category_extract_name(c, name, sizeof(name), NULL)) != NULL) {
			count += PST_snprintf(buf + count, buf_len - count, "%s%s%s",
					round == 0 ? "" : " ",
					strlen(name)>1 ? "--" : "-",
					name);
			round++;
		}

		count += PST_snprintf(buf + count, buf_len - count, ")");
	}

//	count += PST_snprintf(buf + count, buf_len - count, "\n");
	return buf;
}

char *TASK_DEFINITION_howToRepiar_toString(TASK_DEFINITION *def, PARAM_SET *set, const char *prefix, char *buf, size_t buf_len) {
	const char *pName = NULL;
	int err_printed = 0;
	size_t count = 0;
	char name_buffer[1024];
	const char *pref = NULL;


	if (def == NULL || set == NULL || buf == NULL || buf_len == 0){
		return NULL;
	}

	pref = (prefix == NULL) ? "" : prefix;

	/**
	 * Error about MANDATORY flags.
	 */
	pName = def->mandatory;
	while ((pName = category_extract_name(pName, name_buffer, sizeof(name_buffer), NULL)) != NULL) {
		if (strlen(name_buffer) == 0) continue;
		if (!PARAM_SET_isSetByName(set, name_buffer)) {
			if (!err_printed){
				count += PST_snprintf(buf + count, buf_len - count, "%sYou have to define flag(s) '%s%s'",
						pref,
						strlen(name_buffer)>1 ? "--" : "-",
						name_buffer);
				err_printed = 1;
			}
			else{
				count += PST_snprintf(buf + count, buf_len - count, ", '%s%s'", strlen(name_buffer)>1 ? "--" : "-", name_buffer);
			}
		}
	}
	if (err_printed) count += PST_snprintf(buf + count, buf_len - count, ".\n");
	/**
	 * Error about AT LEAST ONE OF flags.
	 */
	if ((category_get_parameter_count(def->atleast_one) - category_get_missing_flag_count(def->atleast_one, set)) == 0) {
		err_printed = 0;
		pName = def->atleast_one;
		while ((pName = category_extract_name(pName, name_buffer, sizeof(name_buffer), NULL)) != NULL){
			if (!err_printed){
				count += PST_snprintf(buf + count, buf_len - count, "%sYou have to define at least one of the flag(s) '%s%s'",
						pref,
						strlen(name_buffer)>1 ? "--" : "-",
						name_buffer);
				err_printed = 1;
			}
			else{
				count += PST_snprintf(buf + count, buf_len - count, ", '%s%s'", strlen(name_buffer)>1 ? "--" : "-", name_buffer);
			}
		}
		if (err_printed) count += PST_snprintf(buf + count, buf_len - count, ".\n");
	}

	/**
	 * Error about FORBITTEN flags.
	 */
	pName = def->forbitten;
	err_printed = 0;
	while ((pName = category_extract_name(pName, name_buffer, sizeof(name_buffer), NULL)) != NULL){
		if (PARAM_SET_isSetByName(set, name_buffer)) {
			if (!err_printed){
				count += PST_snprintf(buf + count, buf_len - count, "%sYou must not use flag(s) '%s%s'",
						pref,
						strlen(name_buffer)>1 ? "--" : "-",
						name_buffer);
				err_printed = 1;
			}
			else{
				count += PST_snprintf(buf + count, buf_len - count, ", '%s%s'", strlen(name_buffer)>1 ? "--" : "-", name_buffer);
			}
		}
	}
	if (err_printed) count += PST_snprintf(buf + count, buf_len - count, ".\n");

	return buf;
}

char* TASK_DEFINITION_ignoredParametersToString(TASK_DEFINITION *def, PARAM_SET *set, const char *prefix, char *buf, size_t buf_len) {
	const char *pName = NULL;
	char name_buffer[1024];
	size_t count = 0;
	const char *pref = NULL;
	int err_printed = 0;


	if (def == NULL || set == NULL || buf == NULL || buf_len == 0) return NULL;

	pref = (prefix == NULL) ? "" : prefix;

	pName = def->ignore;
	err_printed = 0;
	while ((pName = category_extract_name(pName, name_buffer, sizeof(name_buffer), NULL)) != NULL){
		if (PARAM_SET_isSetByName(set, name_buffer)) {
			if (!err_printed){
				count += PST_snprintf(buf + count, buf_len - count, "%sIgnoring following flag(s) '%s%s'",
						pref,
						strlen(name_buffer)>1 ? "--" : "-",
						name_buffer);
				err_printed = 1;
			}
			else{
				count += PST_snprintf(buf + count, buf_len - count, ", '%s%s'", strlen(name_buffer)>1 ? "--" : "-", name_buffer);
			}
		}
	}

	if (err_printed) count += PST_snprintf(buf + count, buf_len - count, ".\n");

	return buf;
}


int TASK_SET_new(TASK_SET **new) {
	int res;
	TASK_SET *tmp = NULL;
	int i;

	if (new == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Construct new and empty task definition data structure.
	 */
	tmp = (TASK_SET*)malloc(sizeof(TASK_SET));
	if (tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->consistentTask = NULL;
	tmp->set_used = NULL;
	tmp->isAnalyzed = 0;
	tmp->consistent_count = 0;
	tmp->count = 0;

	for (i = 0; i < TASK_DEFINITION_MAX_COUNT; i++) {
		tmp->array[i] = NULL;
	}


	*new = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	TASK_SET_free(tmp);

	return res;
}

void TASK_SET_free(TASK_SET *obj){
	int i;
	if (obj != NULL) {
		TASK_free(obj->consistentTask);

		for (i = 0; i < TASK_DEFINITION_MAX_COUNT; i++) {
			TASK_DEFINITION_free(obj->array[i]);
		}

		free(obj);
	}
}

int TASK_SET_add(TASK_SET *obj, int id, const char *name, const char *man, const char *atleastone, const char *forb, const char *ignore) {
	int res;
	TASK_DEFINITION *tmp = NULL;

	if (obj == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (obj->count == TASK_DEFINITION_MAX_COUNT) {
		res = PST_INDEX_OVF;
		goto cleanup;
	}

	res = TASK_DEFINITION_new(id, name, man, atleastone, forb, ignore, &tmp);
	if (res != PST_OK) goto cleanup;

	obj->isAnalyzed = 0;
	obj->set_used = NULL;

	obj->array[obj->count] = tmp;
	obj->count++;
	tmp = NULL;

	res = PST_OK;

cleanup:

	TASK_DEFINITION_free(tmp);

	return res;
}

int TASK_SET_analyzeConsistency(TASK_SET *task_set, PARAM_SET *set, double sensitivity){
	int res;
	size_t i = 0;
	size_t j = 0;
	double cons = 0;
	double tmp_cons_small = 0;
	int tmp_index_for_small = 0;
	int smaller_index = -1;
	int bigger_index = -1;


	if (task_set == NULL || set == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	task_set->isAnalyzed = 0;
	task_set->consistent_count = 0;

	if (task_set->count == 0) {
		res = PST_TASK_SET_HAS_NO_DEFINITIONS;
		goto cleanup;
	}

	if (task_set->set_used != NULL && task_set->set_used != set) {
		res = PST_TASK_UNABLE_TO_ANALYZE_PARAM_SET_CHANGED;
		goto cleanup;
	}


	/**
	 * Analyze consistency.
	 */
	for (i = 0; i < task_set->count; i++) {
		res = TASK_DEFINITION_analyzeConsistency(task_set->array[i], set, &cons);
		if (res != PST_OK) goto cleanup;

		if (cons >= 1.0 || task_set->array[i]->isConsistent) {
			task_set->consistent_count++;
		}

		task_set->cons[i] = cons;
		task_set->index[i] = i;
	}

	/**
	 * Sort tasks consistency. Starting from more consistent (index == 0) and end
	 * with less consistent. If consistency is very similar analyze the order
	 * in more precise way.
	 */
//	debug_array_printf
	for (i = 0; i < task_set->count; i++) {
		for (j = i + 1; j < task_set->count; j++) {
			if (fabs(task_set->cons[i] - task_set->cons[j]) <= sensitivity) {
				TASK_DEFINITION *A_i = task_set->array[task_set->index[i]];
				TASK_DEFINITION *B_j = task_set->array[task_set->index[j]];
				TASK_DEFINITION *Bigger = task_set->array[task_set->index[j]];

				res = TASK_DEFINITION_getMoreConsistent(A_i, B_j, set, sensitivity, &Bigger);
				if (res != PST_OK) goto cleanup;

				if (Bigger == NULL || Bigger == A_i) {
					smaller_index = -1;
					bigger_index = -1;
				} else if (Bigger == B_j) {
					smaller_index = (int) i;
					bigger_index = (int) j;
				} else {
					res = PST_UNDEFINED_BEHAVIOUR;
					goto cleanup;
				}

			} else if (task_set->cons[i] < task_set->cons[j]) {
				smaller_index = (int) i;
				bigger_index = (int) j;
			} else {
				smaller_index = -1;
				bigger_index = -1;
			}

			if (smaller_index >= 0 && bigger_index >= 0) {
				tmp_cons_small = task_set->cons[smaller_index];
				task_set->cons[i] = task_set->cons[bigger_index];
				task_set->cons[j] = tmp_cons_small;

				tmp_index_for_small = (int) task_set->index[smaller_index];
				task_set->index[i] = task_set->index[bigger_index];
				task_set->index[j] = tmp_index_for_small;

			}
		}
//		debug_array_printf
	}

	task_set->isAnalyzed = 1;
	task_set->set_used = set;
	res = PST_OK;

cleanup:

	return res;
}

int TASK_SET_getConsistentTask(TASK_SET *task_set, TASK **task) {
	int res;
	TASK_DEFINITION *consistent = NULL;
	TASK_DEFINITION *def_tmp = NULL;
	TASK *tmp = NULL;
	size_t i;

	if (task_set == NULL || task == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (task_set->count == 0) {
		res = PST_TASK_SET_HAS_NO_DEFINITIONS;
		goto cleanup;
	}

	if (task_set->isAnalyzed == 0) {
		res = PST_TASK_SET_NOT_ANALYZED;
		goto cleanup;
	}

	if (task_set->set_used == NULL) {
		res = PST_UNDEFINED_BEHAVIOUR;
		goto cleanup;
	}

	/**
	 * If there is not exactly one consistent task there must be a error.
	 */
	if (task_set->consistent_count != 1) {
		res = task_set->consistent_count == 0 ? PST_TASK_ZERO_CONSISTENT_TASKS : PST_TASK_MULTIPLE_CONSISTENT_TASKS;
		goto cleanup;
	}

	/**
	 * Check for the single consistent value (as in some cases its not the first
	 * in the list, but is only consistent one) Create new task;
	 */

	for (i = 0; i < task_set->count; i++) {
		consistent = task_set->array[task_set->index[i]];
		if (consistent->isConsistent == 1) {
			def_tmp = consistent;
		}

	}

	if (def_tmp == NULL) {
		res = PST_UNDEFINED_BEHAVIOUR;
		goto cleanup;
	}

	if (task_set->consistentTask != NULL) {
		if (task_set->consistentTask->def != def_tmp) {
			res = PST_UNDEFINED_BEHAVIOUR;
			goto cleanup;
		}
	} else {
		res = TASK_new(def_tmp, task_set->set_used, &tmp);
		if (res != PST_OK) goto cleanup;

		task_set->consistentTask = tmp;
		tmp = NULL;
	}

	*task = task_set->consistentTask;
	res = PST_OK;


cleanup:

	TASK_free(tmp);

	return res;
}

int TASK_SET_cleanIgnored(TASK_SET *task_set, TASK *task, int *removed) {
	int res;
	const char *pName = NULL;
	char buf[1024];
	int removed_count = 0;

	if (task_set == NULL || task == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	PARAM_SET_clearParameter(task->set, task->def->ignore);

	pName = task->def->ignore;
	while ((pName = category_extract_name(pName, buf, sizeof(buf), NULL)) != NULL) {
		if (PARAM_SET_isSetByName(task->set, buf)){
			res = PARAM_SET_clearParameter(task->set, buf);
			if (res != PST_OK) goto cleanup;

			removed_count++;
		}
	}

	if (removed != NULL) {
		*removed = removed_count;
	}

	res = PST_OK;

cleanup:

	return res;
}

int TASK_SET_isOneFromSetTheTarget(TASK_SET *task_set, double diff, int *ID) {
	size_t i;
	TASK_DEFINITION *tmp = NULL;
	double cons = 0;

	if (task_set == NULL || diff <= 0) return 0;
	if (task_set->count == 0) return 0;

	cons = task_set->array[task_set->index[0]]->consistency;

	for (i = 1; i < task_set->count; i++) {
		tmp = task_set->array[task_set->index[i]];
		if (fabs(tmp->consistency - cons) < diff) {
			return 0;
		}
	}

	if (ID != NULL) {
		*ID = (task_set->count == 1) ? (task_set->array[0]->id) : (task_set->array[task_set->index[0]]->id);
	}

	return 1;
}

char* TASK_SET_howToRepair_toString(TASK_SET *task_set, PARAM_SET *set, int ID, const char *prefix, char *buf, size_t buf_len) {
	size_t i;
	size_t count = 0;
	char *tmp;

	if (task_set == NULL || buf == NULL || buf_len == 0 || set == NULL) return NULL;

	for (i = 0; i < task_set->count; i++) {
		if (task_set->array[i]->id == ID) {
			if (task_set->array[i]->isConsistent) {
				count += PST_snprintf(buf + count, buf_len - count, "Task '%s' %s is OK.", task_set->array[i]->name, task_set->array[i]->toString);
			} else {
				count += PST_snprintf(buf + count, buf_len - count, "Task '%s' %s is invalid:\n", task_set->array[i]->name, task_set->array[i]->toString);
				tmp = TASK_DEFINITION_howToRepiar_toString(task_set->array[i], set, prefix, buf + count, buf_len - count);

				if (tmp == NULL) return NULL;
				else return buf;
			}

		}
	}

	return buf;
}

char* TASK_SET_suggestions_toString(TASK_SET *task_set, int depth, char *buf, size_t buf_len) {
	size_t i;
	int n;
	size_t count = 0;
	TASK_DEFINITION *tmp = NULL;

	if (task_set == NULL || depth <= 0) return NULL;

	for (i = 0, n = 0; i < task_set->count && n < depth; i++) {
		tmp = task_set->array[task_set->index[i]];
		if (!tmp->isConsistent) {
			count += PST_snprintf(buf + count, buf_len - count, "Maybe you want to: %s %s\n", tmp->name, tmp->toString);
			n++;
		}
	}

	return buf;
}
