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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include "param_set_obj_impl.h"
#include "param_value.h"
#include "parameter.h"
#include "param_set.h"
#include "strn.h"

#define TYPO_SENSITIVITY 10
#define TYPO_MAX_COUNT 5

//typedef struct INT_st {int value;} INT;

static int isValidNameChar(int c) {
	if ((ispunct(c) || isspace(c)) && c != '_' && c != '-') return 0;
	else return 1;
}

const char* extract_next_name(const char* name_string, int (*isValidNameChar)(int), char *buf, short len, int *flags) {
	int cat_i = 0;
	int buf_i = 0;
	int tmp_flags = 0;
	int isNameOpen = 0;
	int isFlagsOpen = 0;


	if (name_string == NULL || name_string[0] == 0 || buf == NULL) {
		return NULL;
	}

	while (name_string[cat_i] != 0) {
		/* If buf is going to be full, return NULL. */
		if (buf_i >= len - 1) {
			buf[len - 1] = 0;
			return NULL;
		}

		/**
		 * Extract the name and extra flags.
		 */
		if (buf_i == 0 && !isNameOpen && !isFlagsOpen && isValidNameChar(name_string[cat_i])) {
			isNameOpen = 1;
		} else if (isNameOpen && buf_i > 0 && !isValidNameChar(name_string[cat_i]) && name_string[cat_i] != '-') {
			isNameOpen = 0;
		}


		if (buf_i > 0 && !isNameOpen && !isFlagsOpen && name_string[cat_i] == '[') {
			isFlagsOpen = 1;
		} else if (isFlagsOpen && name_string[cat_i] == ']') {
			isFlagsOpen = 0;
		}

		/**
		 * Extract the data fields.
		 */
		if (isNameOpen) {
			buf[buf_i++] = name_string[cat_i];
		} else if (isFlagsOpen) {
			/*TODO: extract flags*/
		} else if (buf_i > 0){
			break;
		}

		cat_i++;
	}

	if (buf[0] == '0') {
		return NULL;
	}

	buf[buf_i] = 0;
	if (flags != NULL) {
		*flags = tmp_flags;
	}

	return buf_i == 0 ? NULL : &name_string[cat_i];
}

/*TODO: Refactore (use and extend extract_next_name).*/
static char *getParametersName(const char* list_of_names, char *name, char *alias, short len, int *flags){
	char *pName = NULL;
	int i = 0;
	int isAlias = 0;
	short i_name = 0;
	short i_alias = 0;
	int tmp_flags = 0;

	if(list_of_names == NULL || name == NULL) return NULL;
	if(list_of_names[0] == 0) return NULL;

	pName=strchr(list_of_names, '{');
	if(pName == NULL) return NULL;
	pName++;
	while(pName[i] != '}' && pName[i] != 0){
		if(len-1 <= i_name || len-1 <= i_alias){
			return NULL;
		}

		if(pName[i] == '|'){
			isAlias = 1;
			i++;
		}

		if(isAlias && alias){
			alias[i_alias++] = pName[i];
		}else{
			name[i_name++] = pName[i];
		}
		i++;
	}
	if (pName[i] == '}') {
		if(pName[i+1] == '*')
			tmp_flags |= 0;
		else
			tmp_flags |= PARAM_SINGLE_VALUE;
	}
	if(pName[i] == '}'){
		if(pName[i+1] == '>') {
			tmp_flags |= PARAM_SINGLE_VALUE_FOR_PRIORITY_LEVEL;
			tmp_flags &= ~(PARAM_SINGLE_VALUE);
		}
	}


	name[i_name] = 0;
	if(alias)
		alias[i_alias] = 0;

	if (flags != NULL) *flags = tmp_flags;
	return &pName[i];
}

const char not_valid_key_beginning_characters[] = "-_";

int parse_key_value_pair(const char *line, char *key, char *value, size_t buf_len) {
	int res;
	size_t i = 0;
	size_t key_i = 0;
	size_t value_i = 0;
	int key_opend = 0;
	int value_opend = 0;
	int is_ecape_opend = 0;
	int is_quote_mark_opend = 0;
	int C;


	if (line == NULL || key == NULL || value == NULL || buf_len == 0) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Search for the first character that is valid for a KEY string. Everything else
	 * than space
	 */
	while ((C = 0xff & line[i]) != '\0') {
		if (!isspace(C) && C != '-' && !isalpha(C)) {
			res = PST_INVALID_FORMAT;
			goto cleanup;
		} else if (isalnum(C) || C == '-') {
			break;
		}

		i++;
	}

	if (C == '\0') {
		res = PST_OK;
		goto cleanup;
	}

	/**
	 * The first key character must be available.
	 */
	key_opend = 1;
	while ((C = 0xff & line[i]) != '\0') {
		if (!is_ecape_opend && C == '\\') {
			is_ecape_opend = 1;
			i++;
			continue;
		}

		if (!is_ecape_opend && !is_quote_mark_opend && C == '"') {
			is_quote_mark_opend = 1;
			i++;
			continue;
		} else if (!is_ecape_opend && is_quote_mark_opend && C == '"') {
			break;
		}

		if (is_ecape_opend && C == '\\') {
			is_ecape_opend = 0;
		} else if (is_ecape_opend && C == '"') {
			is_ecape_opend = 0;
		}


		if (key_opend && key_i < buf_len - 1) {
			if (isValidNameChar(C)) {
				key[key_i] = (char)(0xff & C);
				key_i++;
			} else {
				key_opend = 0;
			}
		} else if (key_opend == 0 && value_opend == 0) {
			if ((!isspace(C) && C != '=') || (isspace(C) && is_quote_mark_opend)) {
				value_opend = 1;
				value[value_i] = (char)(0xff & C);
				value_i++;
			}
		} else if (value_opend && value_i < buf_len - 1) {
			if (isspace(C) && !is_quote_mark_opend) break;

			value[value_i] = (char)(0xff & C);
			value_i++;
		}

		i++;
	}


	res = PST_OK;


cleanup:
	key[key_i] = '\0';
	value[value_i] = '\0';

	return res;
}

int read_line(FILE *file, char *buf, size_t len, size_t *row_pointer, size_t *read_count) {
	int c;
	size_t count = 0;
	size_t line_coun = 0;
	int is_line_open = 0;

	if (file == NULL || buf == NULL || len == 0) return 0;
	buf[0] = '\0';

	while ((c = fgetc(file)) != 0 && count < len - 1) {
		if (c == EOF || (c == '\r' || c == '\n')) {
			line_coun++;
			if (c == EOF) break;
		}

		if (c != '\r' && c != '\n') {
			is_line_open = 1;
			buf[count++] = 0xff & c;
		} else if (is_line_open) {
			break;
		}
	}
	buf[count] = '\0';

	if (row_pointer != NULL) {
		*row_pointer += line_coun;
	}

	if (read_count != NULL) {
		*read_count = count;
	}


	return (c == EOF) ? EOF : 0;
}

static unsigned min_of_3(unsigned A, unsigned B,unsigned C){
	unsigned tmp;
	tmp = A < B ? A : B;
	return tmp < C ? tmp : C;
}

#define UP(m,i,j) m[i-1][j]	//del
#define LEFT(m,i,j) m[i][j-1] //ins
#define DIAG(m,i,j) m[i-1][j-1] //rep

/*TODO refactor*/
static int editDistance_levenshtein(const char *A, const char *B){
	unsigned lenA, lenB;
	unsigned M_H, M_W;
	unsigned i=0, j=0;
	char **m;
	unsigned rows_created = 0;
	int edit_distance = -1;

	/*Get the size of each string*/
	lenA = (unsigned)strlen(A);//vertical
	lenB = (unsigned)strlen(B);//horizontal
	/*Data matrix is larger by 1 row and column*/
	M_H = lenA+1;
	M_W = lenB+1;

	/*Creating of initial matrix*/
	m=(char**)malloc(M_H*sizeof(char*));
	if(m == NULL) goto cleanup;

	for(i=0; i<M_H; i++){
		m[i]=(char*)malloc(M_W*sizeof(char));
		if(m[i] == NULL) goto cleanup;
		m[i][0] = 0xff & i;
		rows_created++;
	}

	for(j=0; j<M_W; j++) m[0][j] = 0xff & j;

	for(j=1; j<M_W; j++){
		for(i=1; i<M_H; i++){
			if(A[i-1] == B[j-1]) m[i][j] = DIAG(m,i,j);
			else m[i][j] = (0xff & min_of_3(UP(m,i,j), LEFT(m,i,j), DIAG(m,i,j))) + 1;
		}
	}
	edit_distance = m[i-1][j-1];


cleanup:
	if(m)
		for(i=0; i<rows_created; i++) free(m[i]);
	free(m);

	return edit_distance;
}

static int param_set_getParameterByName(const PARAM_SET *set, const char *name, PARAM **param){
	int res = 0;
	PARAM *parameter = NULL;
	PARAM *tmp = NULL;
	int i = 0;

	if (set == NULL || param == NULL || name == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}


	for(i = 0; i < set->count; i++) {
		parameter = set->parameter[i];
		if(parameter != NULL){
			if(strcmp(parameter->flagName, name) == 0 || (parameter->flagAlias && strcmp(parameter->flagAlias, name) == 0)) {
				tmp = parameter;
				break;
			}
		}
	}

	if (tmp == NULL) {
		res = PST_PARAMETER_NOT_FOUND;
		goto cleanup;
	}

	*param = parameter;
	res = PST_OK;

cleanup:

	return res;
}

typedef struct TYPO_st {
	char *name;
	int difference;
	int isTypo;
} TYPO;

static int string_is_substring(const char *str, const char *substr) {
	char *ret_str = NULL;
	char *ret_substr = NULL;

	if (str == NULL || substr == NULL) return 0;

	ret_str = strstr(str, substr);
	ret_substr = strstr(substr, str);

	return (ret_str == NULL && ret_substr == NULL) ? 0 : 1;
}

static int string_is_substring_at_the_beginning(const char *str, const char *substr) {
	char *ret_str = NULL;
	char *ret_substr = NULL;

	if (str == NULL || substr == NULL) return 0;

	ret_str = strstr(str, substr);
	ret_substr = strstr(substr, str);

	return (ret_str == str || ret_substr == substr) ? 1 : 0;
}

//#define dbg_print_(format, ...) printf(format, ##__VA_ARGS__)
#define dbg_print_(format, ...) ;


static int param_set_analyze_similarity(PARAM_SET *set, const char *str, int sens, int max_count, TYPO *typo_index){
	int numOfElements = 0;
	PARAM **array = NULL;
	int smallest_difference = 100;
	int typo_count = 0;
	int i = 0;

	if(set == NULL || str == NULL || max_count == 0 || typo_index == NULL) return 0;

	numOfElements = set->count;
	array = set->parameter;


	dbg_print_("%10s", str);
	for (i = 0; i < numOfElements; i++) {
		typo_index[i].name = NULL;
		typo_index[i].isTypo = 0;
		typo_index[i].difference = 100;
		dbg_print_("%10s", array[i]->flagName);
	}
	dbg_print_("\n");
	dbg_print_("%10s", "f(x) = ");
	/**
	 * Analyze the set for possible typos.
	 */
	for (i = 0; i < numOfElements; i++) {
		unsigned name_len = 0;
		unsigned alias_len = 0;
		int name_edit_distance = 0;
		int alias_edit_distance = 0;
		int name_difference = 100;
		int alias_difference = 100;
		int isSubstring = 0;
		int isSubstringAtTheBeginning = 0;

		if (PARAM_isParsOptionSet(array[i], PST_PRSCMD_NO_TYPOS)) continue;

		/**
		 * Examine both the name and its alias and calculate how big is the difference
		 * compared with input string. If alias exists select the one that is more
		 * similar.
		 */
		name_edit_distance = editDistance_levenshtein(array[i]->flagName, str);
		name_len = (unsigned)strlen(array[i]->flagName);
		name_difference = (name_edit_distance * 100) / name_len;

		if (array[i]->flagAlias) {
			alias_edit_distance = editDistance_levenshtein(array[i]->flagAlias, str);
			alias_len = (unsigned)strlen(array[i]->flagAlias);
			alias_difference = (alias_edit_distance * 100) / alias_len;
		}

		if (name_difference <= alias_difference) {
			isSubstring = string_is_substring(str, array[i]->flagName);
			isSubstringAtTheBeginning = string_is_substring_at_the_beginning(str, array[i]->flagName);
			typo_index[i].difference = name_difference;
			typo_index[i].name = array[i]->flagName;
		} else {
			isSubstring = string_is_substring(str, array[i]->flagAlias);
			isSubstringAtTheBeginning = string_is_substring_at_the_beginning(str, array[i]->flagAlias);
			typo_index[i].difference = alias_difference;
			typo_index[i].name = array[i]->flagAlias;
		}

		if (isSubstring) {
			typo_index[i].difference -= 15;
		}

		if (isSubstringAtTheBeginning) {
			typo_index[i].difference -= 15;
		}

		dbg_print_("%5i%2c%3c", typo_index[i].difference, isSubstring ? 'S' : '-', isSubstringAtTheBeginning ? 'B' : '-');

		/**
		 * Register the smallest difference value.
		 */
		if (typo_index[i].difference < smallest_difference) {
			smallest_difference = typo_index[i].difference;
		}

	}

	dbg_print_("\n..............\n");
	dbg_print_("%12s %2i/%2i, %s\n", "typo", 0,0, "suggestion");
	for (i = 0; i < numOfElements; i++) {
		if (typo_index[i].difference < 90 && typo_index[i].difference <= smallest_difference + sens) {
			typo_count++;
			dbg_print_("%12s %2i/%2i, %s\n", str, typo_index[i].difference, smallest_difference, typo_index[i].name); typo_index[i].isTypo = 1;
		}
	}
	dbg_print_("\n..............\n");

	dbg_print_("RET %i\n", typo_count <= max_count ? 1 : 0);
	return (typo_count > 0 && typo_count <= max_count) ? 1 : 0;
}

int param_set_add_typo_from_list(PARAM_SET *set, const char *typo, const char *source, TYPO *typo_list, size_t typo_list_len) {
	int res;
	int i = 0;

	if (set == NULL || typo == NULL || typo_list == NULL || typo_list_len == 0) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = PARAM_addValue(set->typos, typo, source, 1);
	if (res != PST_OK) goto cleanup;


	for (i = 0; i < typo_list_len; i++) {
		if (typo_list[i].isTypo) {
			res = PARAM_addValue(set->typos, typo_list[i].name, typo, 0);
			if (res != PST_OK) goto cleanup;
		}
	}

	res = PST_OK;

cleanup:

	return res;
}

static int bunch_of_flags_get_unknown_count(PARAM_SET *set, const char *bunch_of_flags) {
	int res;
	char str_flg[2] = {255,0};
	int itr = 0;
	PARAM *tmp = NULL;
	int unknowns = 0;

	if (set == 0 || bunch_of_flags == NULL) {
		return 100;
	}

	while ((str_flg[0] = bunch_of_flags[itr++]) != '\0') {
		res = param_set_getParameterByName(set, str_flg, &tmp);
		if (res != PST_OK && res != PST_PARAMETER_NOT_FOUND) {
			return 100;
		}

		if (res == PST_PARAMETER_NOT_FOUND) {
			unknowns++;
		}
	}

	return unknowns;
}

static int param_set_add_typo_or_unknown(PARAM_SET *set, TYPO *typo_list, const char *source, const char *param, const char *arg) {
	int res;

	if (set == NULL || typo_list == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (param != NULL) {
		if (param_set_analyze_similarity(set, param, TYPO_SENSITIVITY, TYPO_MAX_COUNT, typo_list)) {
			res = param_set_add_typo_from_list(set, param, source, typo_list, set->count);
			if (res != PST_OK) goto cleanup;
		} else {
			res = PARAM_addValue(set->unknown, param, source, PST_PRIORITY_VALID_BASE);
			if (res != PST_OK && res != PST_PARAMETER_IS_UNKNOWN && res != PST_PARAMETER_IS_TYPO) goto cleanup;
		}
	}

	if (arg != NULL) {
		if (param_set_analyze_similarity(set, arg, TYPO_SENSITIVITY, TYPO_MAX_COUNT, typo_list)) {
			res = param_set_add_typo_from_list(set, arg, source, typo_list, set->count);
			if (res != PST_OK) goto cleanup;
		} else {
			res = PARAM_addValue(set->unknown, arg, source, PST_PRIORITY_VALID_BASE);
			if (res != PST_OK && res != PST_PARAMETER_IS_UNKNOWN && res != PST_PARAMETER_IS_TYPO) goto cleanup;
		}
	}

	res = PST_OK;

cleanup:

	return res;
}

/**
 * This functions adds raw parameter to the set. It parses parameters formatted as:
 * --long		- long parameter without argument.
 * --long <arg>	- long parameter with argument.
 * -i <arg>		- short parameter with argument.
 * -vxn			- bunch of flags.
 * @param param - parameter.
 * @param arg
 * @param set
 */
static int param_set_addRawParameter(const char *param, const char *arg, const char *source, PARAM_SET *set, int priority){
	int res;
	const char *flag = NULL;
	unsigned len;
	TYPO *typo_list = NULL;
	int unknown_count = 0;
	len = (unsigned)strlen(param);

	typo_list = (TYPO*) malloc(set->count * sizeof(TYPO));
	if (typo_list == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}


	if(param[0] == '-' && param[1] != 0) {
		flag = param + (param[1] == '-' ? 2 : 1);

		/**
		 * When it is long or short parameters, append it to the list and include
		 * the argument. Otherwise it must be bunch of flags.
		 */
		if ((strncmp("--", param, 2) == 0 && len >= 3) || (param[0] == '-' && len == 2)) {
			res = PARAM_SET_add(set, flag, arg, source, priority);
			if (res != PST_OK && res != PST_PARAMETER_IS_UNKNOWN && res != PST_PARAMETER_IS_TYPO) {
				goto cleanup;
			}

			res = PST_OK;
		} else {
			char str_flg[2] = {255,0};
			int itr = 0;

			/**
			 * If bunch of flags have an argument it must be a typo or unknown parameter.
			 */
			res = param_set_add_typo_or_unknown(set, typo_list, source, NULL, arg);
			if (res != PST_OK) goto cleanup;

			/**
			 * Check how many flags from the group are unknown. If less than 25%
			 * are unknown treat them as regular flags. If more than 25% are
			 * unknown, set the whole string to typo or unknown list.
			 */
			unknown_count = bunch_of_flags_get_unknown_count(set, flag);

			if (unknown_count < 3) {
				while ((str_flg[0] = flag[itr++]) != '\0') {
					res = PARAM_SET_add(set, str_flg, NULL, source, priority);
					if (res != PST_OK && res != PST_PARAMETER_IS_UNKNOWN && res != PST_PARAMETER_IS_TYPO) {
						goto cleanup;
					}

					res = PST_OK;
				}
			} else {
				res = param_set_add_typo_or_unknown(set, typo_list, source, flag, NULL);
				if (res != PST_OK) goto cleanup;
			}

		}
	}
	else{
		res = param_set_add_typo_or_unknown(set, typo_list, source, param, arg);
		if (res != PST_OK) goto cleanup;
		goto cleanup;
	}

	res = PST_OK;

cleanup:

	if (typo_list != NULL) free(typo_list);

	return res;
}

static int isComment(const char *line) {
	int i = 0;
	int C;
	if (line == NULL) return 0;
	if (line[0] == '\0') return 0;

	while ((C = 0xff & line[i]) != '\0') {
		if(C == '#') return 1;
		else if (!isspace(C)) return 0;
		i++;
	}
	return 0;
}

int param_set_getParameterByConstraints(PARAM_SET *set, const char *names, const char *source, int priority, int at, PARAM **param, int *index, int *value_c_before) {
	int res = PST_UNKNOWN_ERROR;
	const char *pName = NULL;
	char buf[1024];
	int count_sum = 0;
	PARAM *has_value = NULL;
	PARAM *tmp = NULL;

	if (set == NULL || names == NULL || index == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}



	/**
	 * Get all the.
	 */
	pName = names;
	while ((pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL)) != NULL) {
		PARAM *param = NULL;
		int count = 0;

		res = param_set_getParameterByName(set, buf, &param);
		if (res != PST_OK) goto cleanup;;

		res = PARAM_getValueCount(param, source, priority, &count);
		if (res != PST_OK) goto cleanup;

		if (count != 0) {
			has_value = param;
		}

		if (at == PST_INDEX_FIRST && has_value != NULL) {
			tmp = has_value;
			*index = PST_INDEX_FIRST;
			break;
		} else if (at != PST_INDEX_FIRST && at != PST_INDEX_LAST && count_sum + count > at) {
			tmp = param;
			*index = at - count_sum;
			break;
		}

		count_sum += count;
	}

	if (at == PST_INDEX_LAST && has_value != NULL) {
		tmp = has_value;
		*index = PST_INDEX_LAST;
	}

	*param = tmp;

	/**
	 * Indicators if some values existed before the first value is found;
	 */
	if (value_c_before != NULL) *value_c_before = count_sum;

	res = PST_OK;

cleanup:

	return res;
}

int PARAM_SET_new(const char *names, PARAM_SET **set){
	int res;
	PARAM_SET *tmp = NULL;
	PARAM **tmp_param = NULL;
	PARAM *tmp_typo = NULL;
	PARAM *tmp_unknwon = NULL;
	PARAM *tmp_syntax = NULL;
	const char *pName = NULL;
	int paramCount = 0;
	int i = 0;
	char mem = 0;
	char buf[1024];
	char alias[1024];
	int flags = 0;

	if (set == NULL || names == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Calculate the parameters count.
	 */
	while(names[i]){
		if(names[i] == '{') mem = names[i];
		else if(mem == '{' && names[i] == '}'){
			paramCount++;
			mem = 0;
		}
		i++;
	}

	/**
	 * Create empty objects.
	 */
	tmp = (PARAM_SET*)malloc(sizeof(PARAM_SET));
	if(tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->parameter = NULL;
	tmp->typos = NULL;
	tmp->unknown = NULL;
	tmp->syntax = NULL;

	tmp_param = (PARAM**)calloc(paramCount, sizeof(PARAM*));
	if(tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	/**
	 * Initialize two special parameters to hold and extract unknown parameters.
	 */
	res = PARAM_new("unknown", NULL, 0, PST_PRSCMD_NONE, &tmp_unknwon);
	if(res != PST_OK) goto cleanup;

	res = PARAM_new("typo", NULL, 0, PST_PRSCMD_NONE, &tmp_typo);
	if(res != PST_OK) goto cleanup;

	res = PARAM_new("syntax", NULL, 0, PST_PRSCMD_NONE, &tmp_syntax);
	if(res != PST_OK) goto cleanup;

	res = PARAM_setObjectExtractor(tmp_typo, NULL);
	if(res != PST_OK) goto cleanup;

	res = PARAM_setObjectExtractor(tmp_unknwon, NULL);
	if(res != PST_OK) goto cleanup;

	tmp->count = paramCount;
	tmp->parameter = tmp_param;
	tmp->typos = tmp_typo;
	tmp->syntax = tmp_syntax;
	tmp->unknown = tmp_unknwon;
	tmp_typo = NULL;
	tmp_unknwon = NULL;
	tmp_param = NULL;
	tmp_syntax = NULL;

	/**
	 * Add parameters to the list.
	 */
	i = 0;
	pName = names;
	while((pName = getParametersName(pName, buf, alias, sizeof(buf), &flags)) != NULL){
		res = PARAM_new(buf, alias[0] ? alias : NULL, flags, PST_PRSCMD_DEFAULT, &tmp->parameter[i]);
		if(res != PST_OK) goto cleanup;
		i++;
	}

	*set = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	PARAM_free(tmp_unknwon);
	PARAM_free(tmp_typo);
	PARAM_free(tmp_syntax);
	PARAM_SET_free(tmp);
	free(tmp_param);

	return res;
}

void PARAM_SET_free(PARAM_SET *set){
	int numOfElements = 0;
	PARAM **array = NULL;
	int i =0;

	if(set == NULL) return;
	numOfElements = set->count;
	array = set->parameter;

	for(i=0; i<numOfElements;i++)
		PARAM_free(array[i]);
	free(set->parameter);

	PARAM_free(set->typos);
	PARAM_free(set->unknown);
	PARAM_free(set->syntax);

	free(set);
	return;
}

int PARAM_SET_addControl(PARAM_SET *set, const char *names,
		int (*controlFormat)(const char *),
		int (*controlContent)(const char *),
		int (*convert)(const char*, char*, unsigned),
		int (*extractObject)(void *, const char *, void**)){
	int res;
	PARAM *tmp = NULL;
	const char *pName = NULL;
	char buf[1024];

	if (set == NULL || names == NULL) return PST_INVALID_ARGUMENT;

	pName = names;
	while ((pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL)) != NULL) {
		res = param_set_getParameterByName(set, buf, &tmp);
		if (res != PST_OK) return res;

		res = PARAM_addControl(tmp, controlFormat, controlContent, convert);
		if (res != PST_OK) return res;

		res = PARAM_setObjectExtractor(tmp, extractObject);
		if (res != PST_OK) return res;
	}

	return PST_OK;
}

int PARAM_SET_wildcardExpander(PARAM_SET *set, const char *names,
		void *ctx,
		int (*expand_wildcard)(PARAM_VAL *param_value, void *ctx, int *value_shift)){
	int res;
	PARAM *tmp = NULL;
	const char *pName = NULL;
	char buf[1024];

	if (set == NULL || names == NULL) return PST_INVALID_ARGUMENT;

	pName = names;
	while ((pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL)) != NULL) {
		res = param_set_getParameterByName(set, buf, &tmp);
		if (res != PST_OK) return res;

		res = PARAM_setWildcardExpander(tmp, ctx, expand_wildcard);
		if (res != PST_OK) return res;
	}

	return PST_OK;
}

int PARAM_SET_setParseOptions(PARAM_SET *set, const char *names, int options){
	int res;
	PARAM *tmp = NULL;
	const char *pName = NULL;
	char buf[1024];

	if (set == NULL || names == NULL) return PST_INVALID_ARGUMENT;

	pName = names;
	while ((pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL)) != NULL) {
		res = param_set_getParameterByName(set, buf, &tmp);
		if (res != PST_OK) return res;

		res = PARAM_setParseOption(tmp, options);
		if (res != PST_OK) return res;
	}

	return PST_OK;
}

int PARAM_SET_add(PARAM_SET *set, const char *name, const char *value, const char *source, int priority) {
	int res;
	PARAM *param = NULL;
	TYPO *typo_list = NULL;
	if (set == NULL || name == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	typo_list = (TYPO*) malloc(set->count * sizeof(TYPO));
	if (typo_list == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	res = param_set_getParameterByName(set, name, &param);
	if (res == PST_PARAMETER_NOT_FOUND) {
		/**
		 * If the parameters name is not found, check if it is a typo? If typo
		 * is found push it to the typo list. If not a typo, push it to the unknown
		 * list and if unknown flag has argument, push it too.
		 */

		/* Analyze similarity and */
		if (param_set_analyze_similarity(set, name, TYPO_SENSITIVITY, TYPO_MAX_COUNT, typo_list)) {
			res = param_set_add_typo_from_list(set, name, source, typo_list, set->count);
			if (res != PST_OK) goto cleanup;

			res = PST_PARAMETER_IS_TYPO;
			goto cleanup;
		} else {
			res = PARAM_addValue(set->unknown, name, source, PST_PRIORITY_VALID_BASE);
			if(res != PST_OK) goto cleanup;

			if (value != NULL) {
				res = PARAM_addValue(set->unknown, value, source, PST_PRIORITY_VALID_BASE);
				if(res != PST_OK) goto cleanup;
			}

			res = PST_PARAMETER_IS_UNKNOWN;
			goto cleanup;
		}
	} else if (res != PST_OK) {
		goto cleanup;
	}

	res = PARAM_addValue(param, value, source, priority);
	if(res != PST_OK) goto cleanup;

	res = PST_OK;

cleanup:

	if (typo_list != NULL) free(typo_list);

	return res;
}

int PARAM_SET_getObjExtended(PARAM_SET *set, const char *name, const char *source, int priority, int at, void *ctxt, void **obj) {
	int res;
	PARAM *param = NULL;
	void *extras[2] = {NULL, NULL};
	int virtual_at = 0;
	int values_before_target;

	if (set == NULL || name == NULL || obj == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;;
	}

	res = param_set_getParameterByConstraints(set, name, source, priority, at, &param, &virtual_at, &values_before_target);
	if (res != PST_OK) goto cleanup;

	if (param == NULL && values_before_target > 0) {
		res = PST_PARAMETER_VALUE_NOT_FOUND;
		goto cleanup;
	} else if (param == NULL && values_before_target == 0) {
		res = PST_PARAMETER_EMPTY;
		goto cleanup;
	}

	extras[0] = set;
	extras[1] = ctxt;

	/**
	 * Obj must be feed directly to the getter function, asi it enables to manipulate
	 * the data pointed by obj.
	 */
	res = PARAM_getObject(param, source, priority, at, extras, obj);
	if (res != PST_OK) goto cleanup;

	res = PST_OK;

cleanup:

	return res;
}

int PARAM_SET_getObj(PARAM_SET *set, const char *name, const char *source, int priority, int at, void **obj) {
	return PARAM_SET_getObjExtended(set, name, source, priority, at, set, obj);
}

int PARAM_SET_getStr(PARAM_SET *set, const char *name, const char *source, int priority, int at, char **value) {
	int res;
	PARAM *param = NULL;
	PARAM_VAL *val = NULL;
	int virtual_at = 0;
	int values_before_target;


	if (set == NULL || name == NULL || value == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;;
	}

	res = param_set_getParameterByConstraints(set, name, source, priority, at, &param, &virtual_at, &values_before_target);
	if (res != PST_OK) goto cleanup;

	if (param == NULL && values_before_target > 0) {
		res = PST_PARAMETER_VALUE_NOT_FOUND;
		goto cleanup;
	} else if (param == NULL && values_before_target == 0) {
		res = PST_PARAMETER_EMPTY;
		goto cleanup;
	}

	res = PARAM_getValue(param, source, priority, virtual_at, &val);
	if (res != PST_OK) goto cleanup;


	*value = val->cstr_value;

	if (val->formatStatus != 0 || val->contentStatus != 0) {
		res = PST_PARAMETER_INVALID_FORMAT;
	} else {
		res = PST_OK;
	}

cleanup:

	return res;
}

int PARAM_SET_getAtr(PARAM_SET *set, const char *name, const char *source, int priority, int at, PARAM_ATR *atr) {
	int res;
	PARAM *param = NULL;
	PARAM_VAL *val = NULL;

	if (set == NULL || name == NULL || atr == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = param_set_getParameterByName(set, name, &param);
	if (res != PST_OK) goto cleanup;

	res = PARAM_getValue(param, source, priority, at, &val);
	if (res != PST_OK) goto cleanup;


	atr->cstr_value = val->cstr_value;
	atr->formatStatus = val->formatStatus;
	atr->contentStatus = val->contentStatus;
	atr->priority = val->priority;
	atr->source = val->source;
	atr->name = param->flagName;
	atr->alias = param->flagAlias;

	res = PST_OK;

cleanup:

	return res;
}

int PARAM_SET_clearParameter(PARAM_SET *set, const char *names){
	int res;
	PARAM *tmp = NULL;
	const char *pName = NULL;
	char buf[1024];

	if (set == NULL || names == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * If there is no '{', assume that there is a single value.
	 */
	if (strchr(names, '{') == NULL) {
		res = param_set_getParameterByName(set, names, &tmp);
		if (res != PST_OK) return res;

		res = PARAM_clearAll(tmp);
		if (res != PST_OK) return res;
	} else {
		pName = names;
		while((pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL)) != NULL) {
			res = param_set_getParameterByName(set, buf, &tmp);
			if (res != PST_OK) return res;

			res = PARAM_clearAll(tmp);
			if (res != PST_OK) return res;
		}
	}


	res = PST_OK;

cleanup:

	return res;
}

int PARAM_SET_clearValue(PARAM_SET *set, const char *names, const char *source, int priority, int at) {
	int res;
	PARAM *tmp = NULL;
	const char *pName = NULL;
	char buf[1024];

	if (set == NULL || names == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	pName = names;
	while((pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL)) != NULL) {
		res = param_set_getParameterByName(set, buf, &tmp);
		if (res != PST_OK) return res;

		res = PARAM_clearValue(tmp, source, priority, at);
		if (res != PST_OK) goto cleanup;
	}

	res = PST_OK;

cleanup:

	return res;
}

int PARAM_SET_getValueCount(PARAM_SET *set, const char *names, const char *source, int priority, int *count) {
	int res;
	const char *pName = NULL;
	char buf[1024];
	PARAM *param = NULL;
	int sub_count = 0;
	int C = 0;
	int i = 0;

	if (set == NULL || count == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;;
	}

	/**
	 * Count the values according to the input parameters.
	 */
	if (names != NULL) {
		pName = names;

		/**
		 * If parameters name list is specified, use that to count the values needed.
		 */
		while ((pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL)) != NULL) {
			res = param_set_getParameterByName(set, buf, &param);
			if (res != PST_OK) return res;

			res = PARAM_getValueCount(param, source, priority, &sub_count);
			if (res != PST_OK) goto cleanup;

			C+= sub_count;
		}
	} else {
		/**
		 * If parameters name list is NOT specified, count all parameters.
		 */
		for(i = 0; i < set->count; i++) {
			res = PARAM_getValueCount(set->parameter[i], source, priority, &sub_count);
			if (res != PST_OK) goto cleanup;

			C += sub_count;
		}
	}


	*count = C;
	res = PST_OK;

cleanup:
	return res;
}

static int param_set_count_parameters_set_by_name(const PARAM_SET *set, const char *names, int *set_count, int *unset_count){
	int res;
	PARAM *tmp = NULL;
	const char *pName = NULL;
	char buf[1024];
	int set_c = 0;
	int uset_c = 0;


	if (set == NULL || names == NULL || (set_count == NULL && unset_count == NULL)) return PST_INVALID_ARGUMENT;

	pName = names;
	while ((pName = extract_next_name(pName, isValidNameChar, buf, sizeof(buf), NULL)) != NULL) {
		res = param_set_getParameterByName(set, buf, &tmp);
		if (res != PST_OK && res != PST_PARAMETER_EMPTY) return res;

		if (tmp->argCount > 0) set_c++;
		else uset_c++;
	}

	if (set_count != NULL) {
		*set_count = set_c;
	}

	if (unset_count != NULL) {
		*unset_count = uset_c;
	}

	return PST_OK;
}

int PARAM_SET_isSetByName(const PARAM_SET *set, const char *names){
	int unset_count = 0;
	int set_count = 0;

	if (set == NULL || names == NULL) return 0;

	if (param_set_count_parameters_set_by_name(set, names, &set_count, &unset_count) != PST_OK) return 0;

	return (set_count > 0 && unset_count == 0) ? 1 : 0;
}

int PARAM_SET_isOneOfSetByName(const PARAM_SET *set, const char *names){
	int set_count = 0;

	if (set == NULL || names == NULL) return 0;

	if (param_set_count_parameters_set_by_name(set, names, &set_count, NULL) != PST_OK) return 0;

	return (set_count > 0) ? 1 : 0;
}

int PARAM_SET_isFormatOK(const PARAM_SET *set){
	int res;
	int i = 0;
	PARAM *parameter = NULL;
	PARAM_VAL *invalid = NULL;

	if (set == NULL) {
		return 0;
	}

	/**
	 * Extract all invalid values from parameter. If flag
	 * PST_PRSCMD_FORMAT_CONTROL_ONLY_FOR_LAST_HIGHST_PRIORITY_VALUE is set
	 * check only the last highest priority value.
	 */
	for (i = 0; i < set->count; i++) {
		parameter = set->parameter[i];
		invalid = NULL;
		if (PARAM_isParsOptionSet(parameter, PST_PRSCMD_FORMAT_CONTROL_ONLY_FOR_LAST_HIGHST_PRIORITY_VALUE)) {
			res = PARAM_getValue(parameter, NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &invalid);
			if (res == PST_PARAMETER_VALUE_NOT_FOUND || res == PST_PARAMETER_EMPTY) continue;

			if (invalid->contentStatus != 0 || invalid->formatStatus != 0) return 0;
		} else {
			res = PARAM_getInvalid(parameter, NULL, PST_PRIORITY_NONE, 0, &invalid);
			if (res == PST_PARAMETER_VALUE_NOT_FOUND || res == PST_PARAMETER_EMPTY) continue;
			else if (res == PST_OK && invalid != NULL) return 0;
			else return 0;
		}

	}

	return 1;
}

int PARAM_SET_isConstraintViolation(const PARAM_SET *set){
	int i = 0;
	PARAM *parameter = NULL;

	if (set == NULL) {
		return -1;
	}

	/**
	 * Extract all invalid values from parameter. If flag
	 * PST_PRSCMD_FORMAT_CONTROL_ONLY_FOR_LAST_HIGHST_PRIORITY_VALUE is set
	 * check only the last highest priority value.
	 */
	for (i = 0; i < set->count; i++) {
		parameter = set->parameter[i];

		if (PARAM_checkConstraints(parameter, PARAM_SINGLE_VALUE | PARAM_SINGLE_VALUE_FOR_PRIORITY_LEVEL) != 0) {
			return 1;
		}
	}

	return 0;
}

int PARAM_SET_isTypoFailure(const PARAM_SET *set){
	int count = 0;
	PARAM_getValueCount(set->typos, NULL, PST_PRIORITY_NONE, &count);
	return count > 0 ? 1 : 0;
}

int PARAM_SET_isUnknown(const PARAM_SET *set){
	int count = 0;
	PARAM_getValueCount(set->unknown, NULL, PST_PRIORITY_NONE, &count);
	return count > 0 ? 1 : 0;
}

int PARAM_SET_isSyntaxError(const PARAM_SET *set){
	int count = 0;
	PARAM_getValueCount(set->syntax, NULL, PST_PRIORITY_NONE, &count);
	return count > 0 ? 1 : 0;
}

int PARAM_SET_readFromFile(PARAM_SET *set, const char *fname, const char* source, int priority) {
	int res;
	FILE *file = NULL;
	char line[1024];
	char flag[1024];
	char arg[1024];
	char buf[1024];
	size_t line_nr = 0;
	size_t error_count = 0;
	size_t read_count = 0;

	if(fname == NULL || set == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	file = fopen(fname, "r");
	if(file == NULL) {
		res = PST_IO_ERROR;
		goto cleanup;
	}

	do {
		res = read_line(file, line, sizeof(line), &line_nr, &read_count);
		if (res == EOF && read_count == 0) break;

		if (isComment(line)) continue;

		flag[0] = '\0';
		arg[0] = '\0';
		res = parse_key_value_pair(line, flag, arg, sizeof(flag));
		if (res == PST_INVALID_FORMAT) {
			PST_snprintf(buf, sizeof(buf), "Syntax error at line %4i. Unknown character. '%.60s'.\n", line_nr, line);
			PARAM_addValue(set->syntax, buf, source, priority);
			error_count++;
			res = PST_OK;
		} else if (flag[0] != '-' && flag[0] != '\0') {
			PST_snprintf(buf, sizeof(buf) , "Syntax error at line %4i. Missing character '-'. '%.60s'.\n", line_nr, line);
			PARAM_addValue(set->syntax, buf, source, priority);
			error_count++;
			res = PST_OK;
		} else if (res != PST_OK) {
			goto cleanup;
		}

		if (flag[0] == '\0' && arg[0] == '\0') continue;

		if(flag[0] != '\0' && arg[0] != '\0') {
			res = param_set_addRawParameter(flag, arg, source, set, priority);
			if (res != PST_OK) goto cleanup;
		} else {
			res = param_set_addRawParameter(flag, NULL, source, set, priority);
			if (res != PST_OK) goto cleanup;
		}

	} while (read_count != 0);

	res = (error_count == 0) ? PST_OK : PST_INVALID_FORMAT;

cleanup:

	if(file) fclose(file);
	return res;
}

int PARAM_SET_readFromCMD(PARAM_SET *set, int argc, char **argv, const char *source, int priority) {
	int res;
	int i = 0;
	char *tmp = NULL;
	char *arg = NULL;

	if(set == NULL || argc == 0 || argv == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	for (i = 1; i < argc; i++){
		tmp = argv[i];
		arg = NULL;

		if (i + 1 < argc) {
			if(argv[i + 1][0] != '-' || (argv[i + 1][0] == '-' && argv[i + 1][1] == 0))
				arg = argv[++i];
		}

		res = param_set_addRawParameter(tmp, arg, source, set, priority);
		if (res != PST_OK) goto cleanup;
	}

	res = PST_OK;

cleanup:

	return res;
}

static const char *remove_dashes(const char *str) {
	int i = 0;
	if (str == NULL)  return NULL;
	while (str[i] == '-' && i < 2) i++;
	return &str[i];
}

enum {
	TOKEN_IS_VALUE = 0x01,
	TOKEN_HAS_DASH = 0x02,
	TOKEN_HAS_DOUBLE_DASH = 0x04,
	TOKEN_MATCHES_PARAMETER = 0x08,
	TOKEN_FLAG_LEN_0 = 0x10,
	TOKEN_FLAG_LEN_1 = 0x20,
	TOKEN_FLAG_LEN_N = 0x40,
	TOKEN_UNKNOW = 0x80000000
};

static int is_flag_set(int field, int flag) {
	if (((field & flag) == flag) ||
			(field == flag)) return 1;
	return 0;
}

#define TOKEN_IS_VALUE_PARAM(type) is_flag_set(type, TOKEN_IS_VALUE)
#define TOKEN_IS_SHORT_PARAM(type) is_flag_set(type, TOKEN_FLAG_LEN_1 | TOKEN_HAS_DASH)
#define TOKEN_IS_LONG_PARAM(type) is_flag_set(type, TOKEN_FLAG_LEN_N | TOKEN_HAS_DOUBLE_DASH)
#define TOKEN_IS_BUNCH_OF_FLAGS_PARAM(type) is_flag_set(type, TOKEN_FLAG_LEN_N | TOKEN_HAS_DASH)
#define TOKEN_IS_MATCH(type) is_flag_set(type, TOKEN_MATCHES_PARAMETER)

#define TOKEN_IS_SHORT_PARAM_LONG_DASH(type) is_flag_set(type, TOKEN_FLAG_LEN_1 | TOKEN_HAS_DOUBLE_DASH)
#define TOKEN_IS_NULL_HAS_DOUBLE_DASH(type) is_flag_set(type, TOKEN_FLAG_LEN_0 | TOKEN_HAS_DOUBLE_DASH)
#define TOKEN_IS_NULL_HAS_DASH(type) is_flag_set(type, TOKEN_FLAG_LEN_0 | TOKEN_HAS_DASH)
#define TOKEN_IS_PARAM(type) (TOKEN_IS_SHORT_PARAM(type) || TOKEN_IS_LONG_PARAM(type) || TOKEN_IS_BUNCH_OF_FLAGS_PARAM(type))

#define TOKEN_IS_VALID_FOR_OPEN(type) (TOKEN_IS_MATCH(type) && (TOKEN_IS_LONG_PARAM(type) || TOKEN_IS_SHORT_PARAM(type) || TOKEN_IS_SHORT_PARAM_LONG_DASH(type)))
#define TOKE_IS_VALID_POT_PARAM_BREAKER(type) (TOKEN_IS_SHORT_PARAM(type) || TOKEN_IS_LONG_PARAM(type) || TOKEN_IS_BUNCH_OF_FLAGS_PARAM(type) || TOKEN_IS_SHORT_PARAM_LONG_DASH(type))
static int get_parameter_from_token(PARAM_SET *set, const char *token, int *token_type, PARAM **param) {
	PARAM *tmp = NULL;
	int type = TOKEN_UNKNOW;
	int res = 0;
	size_t len = 0;

	if (set == NULL || token == NULL || param == NULL) return PST_INVALID_ARGUMENT;

	if (token[0] != '-') {
		type = TOKEN_IS_VALUE;
	} else {
		res = param_set_getParameterByName(set, remove_dashes(token), &tmp);
		if (res != PST_OK && res != PST_PARAMETER_NOT_FOUND) return res;

		type = 0;

		if (res == PST_OK && !PARAM_isParsOptionSet(tmp, PST_PRSCMD_HAS_NO_FLAG)) {
			type |= TOKEN_MATCHES_PARAMETER;
		}

		type |= (token[0] == '-' && token[1] == '-') ? TOKEN_HAS_DOUBLE_DASH : TOKEN_HAS_DASH;

		len = strlen(token) - (type & TOKEN_HAS_DASH ? 1 : 2);
		if (len == 0) type |= TOKEN_FLAG_LEN_0;
		else if (len == 1) type |= TOKEN_FLAG_LEN_1;
		else if (len > 1) type |= TOKEN_FLAG_LEN_N;
		else return PST_INVALID_FORMAT;
	}

	*param = tmp;
	*token_type = type;

	return PST_OK;
}

static char* token_type_to_string(int type, char *buf, size_t len) {
	PST_snprintf(buf, len, "[%s%s%s%s%s%s%s%s]",
			(type & TOKEN_IS_VALUE) ? "V:" : "",
			(type & TOKEN_HAS_DASH) ? "D:" : "",
			(type & TOKEN_HAS_DOUBLE_DASH) ? "DD:" : "",
			(type & TOKEN_MATCHES_PARAMETER) ? "M:" : "",
			(type & TOKEN_FLAG_LEN_0) ? "L0:" : "",
			(type & TOKEN_FLAG_LEN_1) ? "L1:" : "",
			(type & TOKEN_FLAG_LEN_N) ? "LN:" : "",
			(type & TOKEN_UNKNOW) ? "?:" : "");
	return buf;
}

static char* pars_flags_to_string(int type, char *buf, size_t len) {
	PST_snprintf(buf, len, "$[%s%s%s%s%s%s%s]",
			(type == PST_PRSCMD_NONE) ? "-" : "",
			((type & PST_PRSCMD_DEFAULT) == PST_PRSCMD_DEFAULT) ? "D:" : "",
			(type & PST_PRSCMD_HAS_NO_VALUE) ? "NV:" : "",
			(type & PST_PRSCMD_HAS_VALUE) ? "V:" : "",
			(type & PST_PRSCMD_HAS_MULTIPLE_INSTANCES) ? "M:" : "",
			(type & PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER) ? "PPBR:" : "",
			(type & PST_PRSCMD_BREAK_VALUE_WITH_EXISTING_PARAMETER_MATCH) ? "MBR:" : "");
	return buf;
}

static char* break_type_to_string(int type, char *buf, size_t len) {
	PST_snprintf(buf, len, "[%s%s%s%s]",
			(type & 1) ? "MATCH_BREAK  " : "",
			(type & 2) ? "POT_PARAM_BREAK   " : "",
			(type & 4) ? "NO_PARA_BREAK" : "",
			(type & 8) ? "VAL_SAT_BREAK" : "");
	return buf;
}



static void print_nothing(const char* format, ...) {(void)(format);}
//#define dpgprint printf
#define dpgprint print_nothing

typedef struct COL_REC_st {
	PARAM *parameter;
	int collect_values;
	int collect_flags;
	int collect_when_parsing_is_closed;
	int reduced_priority;
	int collect_limiter;
	int max_collect_count;
	int collected;
} COL_REC;

#define MAX_COL_COUNT 32

typedef struct COLLECTORS_st {
	int count;
	int close_parsing_permited;
	int parsing_is_closed;
	COL_REC rec[MAX_COL_COUNT];
} COLLECTORS;

static void COLLECTORS_free(COLLECTORS *obj) {
	if (obj == NULL) return;
	free(obj);
}

static int COLLECTORS_new(PARAM_SET *set, COLLECTORS **new) {
	int res = PST_UNKNOWN_ERROR;
	int i = 0;
	COLLECTORS *tmp = NULL;

	if (set == NULL || new == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	tmp = (COLLECTORS*)malloc(sizeof(COLLECTORS) * 1);
	if (tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	tmp->close_parsing_permited = 0;
	tmp->parsing_is_closed = 0;
	tmp->count = 0;

	for (i = 0; i < MAX_COL_COUNT; i++) {
		tmp->rec[i].collect_flags = 0;
		tmp->rec[i].collect_values = 0;
		tmp->rec[i].collect_when_parsing_is_closed = 0;
		tmp->rec[i].reduced_priority = 0;
		tmp->rec[i].collect_limiter = 0;
		tmp->rec[i].max_collect_count = 0;
		tmp->rec[i].collected = 0;
		tmp->rec[i].parameter = NULL;
	}

	for (i = 0; i < set->count && tmp->count < MAX_COL_COUNT; i++) {
		PARAM *p = set->parameter[i];

		if (p->parsing_options & (PST_PRSCMD_CLOSE_PARSING | PST_PRSCMD_COLLECT_LOOSE_FLAGS
				| PST_PRSCMD_COLLECT_LOOSE_VALUES | PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED | PST_PRSCMD_COLLECT_HAS_LOWER_PRIORITY)) {
			if (PARAM_isParsOptionSet(p, PST_PRSCMD_CLOSE_PARSING)) tmp->close_parsing_permited = 1;
			if (PARAM_isParsOptionSet(p, PST_PRSCMD_COLLECT_LOOSE_FLAGS)) tmp->rec[tmp->count].collect_flags = 1;
			if (PARAM_isParsOptionSet(p, PST_PRSCMD_COLLECT_LOOSE_VALUES)) tmp->rec[tmp->count].collect_values = 1;
			if (PARAM_isParsOptionSet(p, PST_PRSCMD_COLLECT_WHEN_PARSING_IS_CLOSED)) tmp->rec[tmp->count].collect_when_parsing_is_closed = 1;
			if (PARAM_isParsOptionSet(p, PST_PRSCMD_COLLECT_HAS_LOWER_PRIORITY)) tmp->rec[tmp->count].reduced_priority = 1;

			if (PARAM_isParsOptionSet(p, PST_PRSCMD_COLLECT_LIMITER_ON)) {
				tmp->rec[tmp->count].collect_limiter = 1;
				tmp->rec[tmp->count].max_collect_count = (p->parsing_options & PST_PRSCMD_COLLECT_LIMITER_MAX_MASK) / PST_PRSCMD_COLLECT_LIMITER_1X;
			}
			tmp->rec[tmp->count].parameter = p;
			tmp->count++;
		}
	}


	*new = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	COLLECTORS_free(tmp);

	return res;
}

int COLLECTORS_disableParsing(COLLECTORS *obj, int token_type) {
	if (obj == NULL) return 0;
	if (obj->close_parsing_permited && obj->parsing_is_closed == 0 && TOKEN_IS_NULL_HAS_DOUBLE_DASH(token_type)) {
		obj->parsing_is_closed = 1;
		return 1;
	}
	return 0;
}

int COLLECTORS_isParsingClosed(COLLECTORS *obj) {
	if (obj == NULL) return 0;
	if (obj->parsing_is_closed) return 1;
	else return 0;
}

int COLLECTORS_doCollectorsExist(COLLECTORS *obj) {
	if (obj == NULL) return 0;
	if (obj->count > 0) return 1;
	else return 0;
}

int COLLECTORS_add(COLLECTORS *obj, int token_type, const char *src, int priority, const char *token) {
	int res = PST_UNKNOWN_ERROR;
	int ret = 0;
	int i = 0;


	if (obj == NULL || token == NULL) {
		goto cleanup;
	}

	/**
	 * Go through the list of collectors and fill the values as configured.
	 */
	for (i = 0; i < obj->count; i++) {
		COL_REC *col = &obj->rec[i];
		PARAM *p = col->parameter;
		int values = (col->collect_values && !obj->parsing_is_closed && (TOKEN_IS_VALUE_PARAM(token_type) || TOKEN_IS_NULL_HAS_DASH(token_type) || TOKEN_IS_NULL_HAS_DOUBLE_DASH(token_type)));
		int flags = (col->collect_flags && (TOKEN_IS_PARAM(token_type) || TOKEN_IS_SHORT_PARAM_LONG_DASH(token_type)) && !obj->parsing_is_closed);
		int after_parsing_is_closed = col->collect_when_parsing_is_closed && obj->parsing_is_closed;
		int prio = (col->reduced_priority && priority > 0) ? priority - 1 : priority;

		if (values || flags || after_parsing_is_closed) {
			if (col->collect_limiter && col->collected >= col->max_collect_count) {
				continue;
			} else {
				res = PARAM_addValue(p, token, src, prio);
				if (res != PST_OK) goto cleanup;
				col->collected++;
				ret++;
			}
		}


	}


cleanup:

	return ret;
}

int PARAM_SET_parseCMD(PARAM_SET *set, int argc, char **argv, const char *source, int priority) {
	int res;
	TYPO *typo_helper = NULL;
	int i = 0;
	char *token = NULL;
	int token_type = 0;
	PARAM *tmp_parameter = NULL;
	char buf[1024];
	char buf2[1024];
	int is_parameter_opend = 0;
	int token_match_break = 0;
	int token_pot_param_break = 0;
	int token_no_param_break = 0;
	int value_saturation_break = 0;
	int last_token_brake = 0;
	int last_token_type = 0;
	PARAM *opend_parameter = NULL;
	size_t value_counter = 0;
	COLLECTORS *collector = NULL;


	if(set == NULL || argc == 0 || argv == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	typo_helper = (TYPO*) malloc(set->count * sizeof(TYPO));
	if (typo_helper == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	res = COLLECTORS_new(set, &collector);
	if (res != PST_OK) goto cleanup;

	for (i = 1; i < argc; i++) {
		last_token_brake = (i + 1 == argc) ? 1 : 0;
		token = argv[i];

		/**
		 * Analyze the tokens.
		*/
		if (!COLLECTORS_isParsingClosed(collector)) {
			last_token_type = token_type;
			res = get_parameter_from_token(set, token, &token_type, &tmp_parameter);
			if (res != PST_OK) goto cleanup;

			dpgprint("Token '%10s' %8s %10s\n", token, token_type_to_string(token_type, buf, sizeof(buf)), pars_flags_to_string(tmp_parameter != NULL ? tmp_parameter->parsing_options : 0, buf2, sizeof(buf2)));


			if (is_parameter_opend) {
				token_match_break = (TOKEN_IS_MATCH(token_type) && PARAM_isParsOptionSet(opend_parameter, PST_PRSCMD_BREAK_VALUE_WITH_EXISTING_PARAMETER_MATCH)) ? 1 : 0;
				token_pot_param_break = ((TOKE_IS_VALID_POT_PARAM_BREAKER(token_type)
						|| (TOKEN_IS_NULL_HAS_DOUBLE_DASH(token_type) && COLLECTORS_doCollectorsExist(collector)))
						&& PARAM_isParsOptionSet(opend_parameter, PST_PRSCMD_BREAK_WITH_POTENTIAL_PARAMETER)) ? 2 : 0;
				token_no_param_break = PARAM_isParsOptionSet(opend_parameter, PST_PRSCMD_HAS_NO_VALUE) ? 4 : 0;

				if (!token_no_param_break && !token_pot_param_break && !token_match_break) {
						res = PARAM_addValue(opend_parameter, token, source, priority);
						if (res != PST_OK) goto cleanup;
						dpgprint("P:VALUE++ (%s = %s)\n", opend_parameter->flagName, token);
						value_counter++;
				}

				value_saturation_break = (value_counter && (PARAM_isParsOptionSet(opend_parameter, PST_PRSCMD_DEFAULT) || PARAM_isParsOptionSet(opend_parameter, PST_PRSCMD_HAS_VALUE))) ? 8 : 0;

				if (token_match_break || token_pot_param_break || token_no_param_break || value_saturation_break || last_token_brake) {
					dpgprint("----------------------------------\n");
					if (value_counter == 0) {
						res = PARAM_SET_add(set, opend_parameter->flagName, NULL, source, priority);
						dpgprint("P:CLOSE (%s = NULL)%s\n", opend_parameter->flagName, break_type_to_string(token_match_break + token_pot_param_break + token_no_param_break + value_saturation_break, buf, sizeof(buf)));
					} else {
						dpgprint("P:CLOSE (%s ---)%s\n", opend_parameter->flagName, break_type_to_string(token_match_break + token_pot_param_break + token_no_param_break + value_saturation_break, buf, sizeof(buf)));
					}

					is_parameter_opend = 0;
					value_counter = 0;
					opend_parameter = NULL;
				}

				if (value_saturation_break) continue;
				/**
				 * 1) Don't exit the loop if the last token brake occurred simultaneously
				 *    with the token match brake. Maybe a last parameter must be opened!
				 * 2) Don't exit the loop if the last token brake occurred simultaneously
				 *    with the token_no_param_break. The last value must be parsed.
				 */
				if ((!token_pot_param_break) && (!token_match_break) && (!token_no_param_break) && last_token_brake) continue;
			}

			/** Value saturation occurs when a single vale parameter is willed with current token, continue to the next token. */

			/* If parameter is not opened and the match is found, set is_parameter_opend flag and initialize opend_parameter.*/
			if (!is_parameter_opend && TOKEN_IS_VALID_FOR_OPEN(token_type)) {
				dpgprint("----------------------------------\n");
				dpgprint("P:OPEN %s\n", token);
				is_parameter_opend = 1;
				value_counter = 0;
				opend_parameter = tmp_parameter;

				if (last_token_brake && value_counter == 0) {
					res = PARAM_SET_add(set, opend_parameter->flagName, NULL, source, priority);
				}

				continue;
			}
		}

		/**
		 * If parameter is not opened and the next value is not a command line option, it must be a typo or unknown.
		 */
		if (!is_parameter_opend) {
			if (COLLECTORS_disableParsing(collector, token_type)) {
				dpgprint("--------------PARSING CLOSED %X\n", token_type );
				token_type = 0;
				last_token_type = 0;
				continue;
			}

			/**
			 * If the token is long or short parameter add it as a typo or unknown.
			 * If the token is value, check if the loose parameter collector exists.
			 * If it does, break. If it does not add typo or unknown.
			 */
			if (TOKEN_IS_BUNCH_OF_FLAGS_PARAM(token_type)) {
				dpgprint("------------BUNCH OF FLAGS-----------------\n");
				res = param_set_addRawParameter(token, NULL, source, set, priority);
				if (res != PST_OK) goto cleanup;
				dpgprint("------------------  DONE   -----------------\n");
				continue;
			} else if (COLLECTORS_add(collector, token_type, source, priority, token) > 0) {
				dpgprint("Collected '%s'.\n", token);
				continue;
			} else {
				if (TOKEN_IS_NULL_HAS_DOUBLE_DASH(token_type) || TOKEN_IS_NULL_HAS_DASH(token_type)) {
					res = param_set_add_typo_or_unknown(set, typo_helper, source, token, NULL);
				} else {
					res = param_set_add_typo_or_unknown(set, typo_helper, source, remove_dashes(token), NULL);
				}
				if (res != PST_OK) goto cleanup;
				dpgprint("Typo added '%s'\n", token);
				continue;
			}

		}
	}

	/**
	 * Expand wildcards when enabled and configured.
	 */
	for (i = 0; i < set->count; i++) {
		if (PARAM_isParsOptionSet(set->parameter[i], PST_PRSCMD_EXPAND_WILDCARD)) {
			res = PARAM_expandWildcard(set->parameter[i], NULL);
			if (res != PST_OK) goto cleanup;
		}
	}

	res = PST_OK;

cleanup:

	if (typo_helper != NULL) free(typo_helper);
	COLLECTORS_free(collector);

	return res;
}
#undef dpgprint

int PARAM_SET_IncludeSet(PARAM_SET *target, PARAM_SET *src) {
	int res;
	int i;
	int n;
	PARAM_VAL *param_value = NULL;
	PARAM *target_param = NULL;

	if (target == NULL || src == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	/**
	 * Scan the source set for parameters. If there is a parameter with the same
	 * name and it has values, add those values to the target set.
	 */
	for (i = 0; i < src->count; i++) {
		/**
		 * If the count is zero, skip that round.
		*/
		if (src->parameter[i]->argCount == 0) continue;

		/**
		 * If parameter in src has values, check if it exists in target. If it does
		 * not skip the round.
		 */
		res = param_set_getParameterByName(target, src->parameter[i]->flagName, &target_param);
		if (res != PST_OK && res != PST_PARAMETER_NOT_FOUND) {
			goto cleanup;
		} else if (res == PST_PARAMETER_NOT_FOUND) {
			continue;
		}


		/**
		 * The target has the parameter with the same name. Include all values.
		 */
		n = 0;
		while (PARAM_getValue(src->parameter[i], NULL, PST_PRIORITY_NONE, n, &param_value) == PST_OK) {
			res = PARAM_SET_add(target, src->parameter[i]->flagName, param_value->cstr_value, param_value->source, param_value->priority);
			if (res != PST_OK) 	goto cleanup;

			n++;
		}
	}

	res = PST_OK;

cleanup:

	return res;
}

static size_t param_value_add_errorstring_to_buf(PARAM *parameter, PARAM_VAL *invalid, const char *prefix, const char* (*getErrString)(int), char *buf, size_t buf_len) {
	int res;
	size_t count = 0;
	const char *value = NULL;
	const char *source = NULL;
	int formatStatus = 0;
	int contentStatus = 0;
	const char *use_prefix = NULL;

	use_prefix = prefix == NULL ? "" : prefix;

	if (parameter == NULL || invalid == NULL || buf == NULL || buf_len == 0) return 0;
	/**
	 * Extract error codes, if not set exit the function.
	 */
	res = PARAM_VAL_getErrors(invalid, &formatStatus, &contentStatus);
	if (res != PST_OK) return 0;

	if (formatStatus == 0 && contentStatus == 0) return 0;

	res = PARAM_VAL_extract(invalid, &value, &source, NULL);
	if (res != PST_OK) return 0;


	count += PST_snprintf(buf + count, buf_len - count, "%s", use_prefix);
	/**
	 * Add Error string or error code.
	 */
	if (getErrString != NULL) {
		if (formatStatus != 0) {
			count += PST_snprintf(buf + count, buf_len - count, "%s.", getErrString(formatStatus));
		} else {
			count += PST_snprintf(buf + count, buf_len - count, "%s.", getErrString(contentStatus));
		}
	} else {
		if (formatStatus != 0) {
			count += PST_snprintf(buf + count, buf_len - count, "Error: 0x%0x.", formatStatus);
		} else {
			count += PST_snprintf(buf + count, buf_len - count, "Error: 0x%0x.", contentStatus);
		}
	}

	/**
	 * Add the source, if NULL not included.
	 */
	if(source != NULL) {
		count += PST_snprintf(buf + count, buf_len - count, " Parameter (from '%s') ", source);
	} else {
		count += PST_snprintf(buf + count, buf_len - count, " Parameter ");
	}

	/**
	 * Add the parameter and its value.
	 */
	count += PST_snprintf(buf + count, buf_len - count, "%s%s '%s'.",
							strlen(parameter->flagName) > 1 ? "--" : "-",
							parameter->flagName,
							value != NULL ? value : ""
							);



	count += PST_snprintf(buf + count, buf_len - count, "\n");

	return count;
}

char* PARAM_SET_invalidParametersToString(const PARAM_SET *set, const char *prefix, const char* (*getErrString)(int), char *buf, size_t buf_len) {
	int res;
	int i = 0;
	int n = 0;
	PARAM *parameter = NULL;
	PARAM_VAL *invalid = NULL;
	size_t count = 0;

	if (set == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}

	/**
	 * Scan all parameter values for errors.
	 */
	for (i = 0; i < set->count; i++) {
		parameter = set->parameter[i];
		n = 0;

		/**
		 * Extract all invalid values from parameter. If flag
		 * PST_PRSCMD_FORMAT_CONTROL_ONLY_FOR_LAST_HIGHST_PRIORITY_VALUE is set
		 * check only the last highest priority value.
		 */
		if (PARAM_isParsOptionSet(parameter, PST_PRSCMD_FORMAT_CONTROL_ONLY_FOR_LAST_HIGHST_PRIORITY_VALUE)) {
			res = PARAM_getValue(parameter, NULL, PST_PRIORITY_HIGHEST, PST_INDEX_LAST, &invalid);

			if (res != PST_OK && res != PST_PARAMETER_EMPTY && res != PST_PARAMETER_VALUE_NOT_FOUND) return NULL;
			count += param_value_add_errorstring_to_buf(parameter, invalid, prefix, getErrString, buf + count, buf_len - count);
		} else {
			while (PARAM_getInvalid(parameter, NULL, PST_PRIORITY_NONE, n++, &invalid) == PST_OK) {
				count += param_value_add_errorstring_to_buf(parameter, invalid, prefix, getErrString, buf + count, buf_len - count);

				if (count >= buf_len - 1) return buf;
			}
		}
	}

	buf[buf_len - 1] = '\0';
	return buf;
}

char* PARAM_SET_unknownsToString(const PARAM_SET *set, const char *prefix, char *buf, size_t buf_len) {
	int res;
	const char *use_prefix = NULL;
	int i = 0;
	PARAM_VAL *unknown = NULL;
	const char *name = NULL;
	const char *source = NULL;
	size_t count = 0;


	if (set == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}

	if (set->unknown->argCount == 0) {
		buf[0] = '\0';
		return NULL;
	}

	use_prefix = prefix == NULL ? "" : prefix;

	for (i = 0; i < set->unknown->argCount; i++) {
		res = PARAM_getValue(set->unknown, NULL, PST_PRIORITY_NONE, i, &unknown);
		if (res != PST_OK) return NULL;

		res = PARAM_VAL_extract(unknown, &name, &source, NULL);
		if (res != PST_OK) return NULL;

		count += PST_snprintf(buf + count, buf_len - count, "%sUnknown parameter '%s'", use_prefix, name);
		if(source != NULL) count += PST_snprintf(buf + count, buf_len - count, " from '%s'", source);
		count += PST_snprintf(buf + count, buf_len - count, ".\n");
		if (count >= buf_len - 1) return buf;
	}

	buf[buf_len - 1] = '\0';
	return buf;
}

char* PARAM_SET_syntaxErrorsToString(const PARAM_SET *set, const char *prefix, char *buf, size_t buf_len) {
	int res;
	const char *use_prefix = NULL;
	int i = 0;
	PARAM_VAL *syntax_error = NULL;
	const char *name = NULL;
	const char *source = NULL;
	size_t count = 0;


	if (set == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}

	if (set->syntax->argCount == 0) {
		buf[0] = '\0';
		return NULL;
	}

	use_prefix = prefix == NULL ? "" : prefix;

	for (i = 0; i < set->syntax->argCount; i++) {
		res = PARAM_getValue(set->syntax, NULL, PST_PRIORITY_NONE, i, &syntax_error);
		if (res != PST_OK) return NULL;

		res = PARAM_VAL_extract(syntax_error, &name, &source, NULL);
		if (res != PST_OK) return NULL;

		count += PST_snprintf(buf + count, buf_len - count, "%s%s", use_prefix, name);
		if (count >= buf_len - 1) return buf;
	}

	buf[buf_len - 1] = '\0';
	return buf;
}

char* PARAM_SET_typosToString(PARAM_SET *set, int flags, const char *prefix, char *buf, size_t buf_len) {
	int res;
	const char *use_prefix = NULL;
	PARAM_VAL *typo = NULL;
	int i = 0;
	const char *name = NULL;
	const char *source = NULL;
	int similar_count = 0;
	int n = 0;
	const char *similar = NULL;
	size_t count = 0;
	char *hyphen = "";
	char *d_hyphen = "";

	if (flags & PST_TOSTR_HYPHEN) {
		hyphen = "-";
		d_hyphen = "-";
	} else if (flags & PST_TOSTR_DOUBLE_HYPHEN) {
		hyphen = "-";
		d_hyphen = "--";
	}

	if (set == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}

	if (set->typos->argCount == 0) {
		buf[0] = '\0';
		return NULL;
	}

	use_prefix = prefix == NULL ? "" : prefix;

	while (PARAM_getValue(set->typos, NULL, 1, i, &typo) == PST_OK) {
		res = PARAM_VAL_extract(typo, &name, &source, NULL);
		if (res != PST_OK) return NULL;

		res = PARAM_getValueCount(set->typos, name, 0, &similar_count);
		if (res != PST_OK) return NULL;

		for (n = 0; n < similar_count; n++) {
			res = PARAM_getObject(set->typos, name, 0, n, NULL, (void**)&similar);
			if (res != PST_OK || similar == NULL) return NULL;

			count += PST_snprintf(buf + count, buf_len - count, "%sDid You mean '%s%s' instead of '%s'.\n",
						use_prefix,
						strlen(similar) > 1 ? d_hyphen : hyphen,
						similar,
						name);
			if (count >= buf_len - 1) return buf;
		}

		i++;
	}

	return buf;
}

char* PARAM_SET_toString(PARAM_SET *set, char *buf, size_t buf_len) {
	int res;
	PARAM_VAL *param_value = NULL;
	int i = 0;
	const char *value = NULL;
	const char *source = NULL;
	int priority = 0;
	int n = 0;
	size_t count = 0;

	if (set == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}


	count += PST_snprintf(buf + count, buf_len - count, "  %3s %10s %50s %10s\n",
			"nr", "value", "source", "priority");

	for (i = 0; i < set->count; i++) {
		count += PST_snprintf(buf + count, buf_len - count, "Parameter: '%s' (%i):\n",
				set->parameter[i]->flagName, set->parameter[i]->argCount);


		n = 0;
		while (PARAM_getValue(set->parameter[i], NULL, PST_PRIORITY_NONE, n, &param_value) == PST_OK) {
			res = PARAM_VAL_extract(param_value, &value, &source, &priority);
			if (res != PST_OK) return NULL;

			count += PST_snprintf(buf + count, buf_len - count, "  %2i) '%s' %50s %10i\n",
					n,
					value == NULL ? "-" : value,
					source == NULL ? "-" : source,
					priority);

			n++;
		}
	}
	count += PST_snprintf(buf + count, buf_len - count, "\n");

	buf[buf_len - 1] = '\0';
	return buf;
}

char* PARAM_SET_constraintErrorToString(const PARAM_SET *set, const char *prefix, char *buf, size_t buf_len) {
	int i;
	size_t count = 0;
	PARAM *parameter = NULL;
	char tmp[1024];
	char *p = NULL;

	if (set == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}

	for (i = 0; i < set->count; i++) {
		tmp[0] = '\0';
		parameter = set->parameter[i];
		p = PARAM_constraintErrorToString(parameter, prefix, tmp, sizeof(tmp));

		if (p != NULL && p[0] != '\0') {
			count += PST_snprintf(buf + count, buf_len - count, "%s", p);
			if (count >= buf_len - 1) return buf;
		}
	}

	buf[buf_len - 1] = '\0';
	return buf;
}

const char* PARAM_SET_errorToString(int err) {
	switch(err) {
	case PST_OK:
		return "OK.";
	case PST_INVALID_ARGUMENT:
		return "Invalid argument.";
	case PST_OUT_OF_MEMORY:
		return "PARAM_SET out of memory.";
	case PST_IO_ERROR:
		return "PARAM_SET IO error.";
	case PST_INDEX_OVF:
		return "Index is too large.";
	case PST_PARAMETER_INVALID_FORMAT:
	case PST_INVALID_FORMAT:
		return "Invalid input format.";
	case PST_PARAMETER_NOT_FOUND:
		return "Parameter not found.";
	case PST_PARAMETER_VALUE_NOT_FOUND:
		return "Parameter value not found.";
	case PST_PARAMETER_EMPTY:
		return "Parameter is empty.";
	case PST_PARAMETER_IS_TYPO:
		return "Parameters name looks like typo.";
	case PST_PARAMETER_IS_UNKNOWN:
		return "Parameters is unknown.";
	case PST_PARAMETER_UNIMPLEMENTED_OBJ:
		return "Parameters object extractor unimplemented.";
	case PST_PRIORITY_NEGATIVE:
		return "Negative priority.";
	case PST_PRIORITY_TOO_LARGE:
		return "Too large priority.";
	case PST_TASK_ZERO_CONSISTENT_TASKS:
		return "None consistent task.";
	case PST_TASK_MULTIPLE_CONSISTENT_TASKS:
		return "More than one consistent tasks.";
	case PST_TASK_SET_HAS_NO_DEFINITIONS:
		return "Task definition empty.";
	case PST_TASK_SET_NOT_ANALYZED:
		return "Task definition not analyzed.";
	case PST_TASK_UNABLE_TO_ANALYZE_PARAM_SET_CHANGED:
		return "Unable to analyze with different PARAM_SET.";
	case PST_WILDCARD_ERROR:
		return "Unable to expand wildcard.";
	case PST_PARAMETER_UNIMPLEMENTED_WILDCARD:
		return "Wildcard expander function not specified.";
	case PST_UNDEFINED_BEHAVIOUR:
		return "PARAM_SET undefined behaviour.";
	case PST_UNKNOWN_ERROR:
		return "PARAM_SET unknown error.";
	}
	return "PARAM_SET unknown error.";
}
