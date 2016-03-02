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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "param_set_obj_impl.h"
#include "param_value.h"
#include "parameter.h"
#include "param_set.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif

#define TYPO_SENSITIVITY 10
#define TYPO_MAX_COUNT 5

typedef struct INT_st {int value;} INT;

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
		if(pName[i+1] == '>')
			tmp_flags |= PARAM_SINGLE_VALUE_FOR_PRIORITY_LEVEL;
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
			else m[i][j] = 0xff & min_of_3(UP(m,i,j), LEFT(m,i,j), DIAG(m,i,j)) + 1;
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

int PARAM_SET_new(const char *names, PARAM_SET **set){
	int res;
	PARAM_SET *tmp = NULL;
	PARAM **tmp_param = NULL;
	PARAM *tmp_typo = NULL;
	PARAM *tmp_unknwon = NULL;
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

	tmp_param = (PARAM**)calloc(paramCount, sizeof(PARAM*));
	if(tmp == NULL) {
		res = PST_OUT_OF_MEMORY;
		goto cleanup;
	}

	/**
	 * Initialize two special parameters to hold and extract unknown parameters.
     */
	res = PARAM_new("unknown", NULL, 0, &tmp_unknwon);
	if(res != PST_OK) goto cleanup;

	res = PARAM_new("typo", NULL, 0, &tmp_typo);
	if(res != PST_OK) goto cleanup;

	res = PARAM_setObjectExtractor(tmp_typo, NULL);
	if(res != PST_OK) goto cleanup;

	res = PARAM_setObjectExtractor(tmp_unknwon, NULL);
	if(res != PST_OK) goto cleanup;

	tmp->count = paramCount;
	tmp->parameter = tmp_param;
	tmp->typos = tmp_typo;
	tmp->unknown = tmp_unknwon;
	tmp_typo = NULL;
	tmp_unknwon = NULL;
	tmp_param = NULL;

	/**
	 * Add parameters to the list.
     */
	i = 0;
	pName = names;
	while((pName = getParametersName(pName, buf, alias, sizeof(buf), &flags)) != NULL){
		res = PARAM_new(buf, alias[0] ? alias : NULL, flags, &tmp->parameter[i]);
		if(res != PST_OK) goto cleanup;
		i++;
	}

	*set = tmp;
	tmp = NULL;
	res = PST_OK;

cleanup:

	PARAM_free(tmp_unknwon);
	PARAM_free(tmp_typo);
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

int PARAM_SET_add(PARAM_SET *set, const char *name, const char *value, const char *source, int priority) {
	int res;
	PARAM *param = NULL;
	TYPO *typo_list = NULL;
	if (set == NULL || name == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;;
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

	if (set == NULL || name == NULL || obj == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;;
	}

	res = param_set_getParameterByName(set, name, &param);
	if (res != PST_OK) goto cleanup;

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

	if (set == NULL || name == NULL || value == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;;
	}

	res = param_set_getParameterByName(set, name, &param);
	if (res != PST_OK) goto cleanup;

	res = PARAM_getValue(param, source, priority, at, &val);
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

int PARAM_SET_isSetByName(const PARAM_SET *set, const char *name){
	int res;
	PARAM *tmp = NULL;
	int count = 0;

	if (set == NULL || name == NULL) return PST_INVALID_ARGUMENT;

	res = param_set_getParameterByName(set, name, &tmp);
	if (res != PST_OK) return 0;

	res = PARAM_getValueCount(tmp, NULL, PST_PRIORITY_NONE, &count);
	if (res != PST_OK) return 0;

	return count == 0 ? 0 : 1;
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
	 * Scan all parameter values from errors.
	 */
	for (i = 0; i < set->count; i++) {
		parameter = set->parameter[i];
		invalid = NULL;
		/**
		 * Extract all invalid values from parameter.
         */
		res = PARAM_getInvalid(parameter, NULL, PST_PRIORITY_NONE, 0, &invalid);
		if (res == PST_PARAMETER_VALUE_NOT_FOUND || res == PST_PARAMETER_EMPTY) continue;
		else if (res == PST_OK && invalid != NULL) return 0;
		else return 0;
	}

	return 1;
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

int PARAM_SET_readFromFile(PARAM_SET *set, const char *fname, const char* source, int priority){
	int res;
	FILE *file = NULL;
	char *ln = NULL;
	char line[1024];
	char flag[1024];
	char arg[1024];

	if(fname == NULL || set == NULL) {
		res = PST_INVALID_ARGUMENT;
		goto cleanup;
	}

	file = fopen(fname, "r");
	if(file == NULL) {
		res = PST_IO_ERROR;
		goto cleanup;
	}

	while(fgets(line, sizeof(line), file)) {
		ln = strchr(line, '\n');
		if(ln != NULL) *ln = 0;

		if (isComment(line)) continue;

		res = parse_key_value_pair(line, flag, arg, sizeof(flag));
		if (res != PST_OK) {
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

	}

	res = PST_OK;

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

char* PARAM_SET_invalidParametersToString(const PARAM_SET *set, const char *prefix, const char* (*getErrString)(int), char *buf, size_t buf_len) {
	int res;
	const char *use_prefix = NULL;
	int i = 0;
	int n = 0;
	PARAM *parameter = NULL;
	PARAM_VAL *invalid = NULL;
	const char *value = NULL;
	const char *source = NULL;
	int formatStatus = 0;
	int contentStatus = 0;
	size_t count = 0;

	if (set == NULL || buf == NULL || buf_len == 0) {
		return NULL;
	}


	use_prefix = prefix == NULL ? "" : prefix;

	/**
	 * Scan all parameter values from errors.
	 */
	for (i = 0; i < set->count; i++) {
		parameter = set->parameter[i];
		n = 0;

		/**
		 * Extract all invalid values from parameter.
         */
		while (PARAM_getInvalid(parameter, NULL, PST_PRIORITY_NONE, n++, &invalid) == PST_OK) {
			res = PARAM_VAL_extract(invalid, &value, &source, NULL);
			if (res != PST_OK) return NULL;

			res = PARAM_VAL_getErrors(invalid, &formatStatus, &contentStatus);
			if (res != PST_OK) return NULL;

			count += snprintf(buf + count, buf_len - count, "%s", use_prefix);
			/**
			 * Add Error string or error code.
			 */
			if (getErrString != NULL) {
				if (formatStatus != 0) {
					count += snprintf(buf + count, buf_len - count, "%s.", getErrString(formatStatus));
				} else {
					count += snprintf(buf + count, buf_len - count, "%s.", getErrString(contentStatus));
				}
			} else {
				if (formatStatus != 0) {
					count += snprintf(buf + count, buf_len - count, "Error: 0x%0x.", formatStatus);
				} else {
					count += snprintf(buf + count, buf_len - count, "Error: 0x%0x.", contentStatus);
				}
			}

			/**
			 * Add the source, if NULL not included.
			 */
			if(source != NULL) {
				count += snprintf(buf + count, buf_len - count, " Parameter (from '%s') ", source);
			} else {
				count += snprintf(buf + count, buf_len - count, " Parameter ");
			}

			/**
			 * Add the parameter and its value.
			 */
			count += snprintf(buf + count, buf_len - count, "%s%s '%s'.",
									strlen(parameter->flagName) > 1 ? "--" : "-",
									parameter->flagName,
									value != NULL ? value : ""
									);



			count += snprintf(buf + count, buf_len - count, "\n");
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

		count += snprintf(buf + count, buf_len - count, "%sUnknown parameter '%s'", use_prefix, name);
		if(source != NULL) count += snprintf(buf + count, buf_len - count, " from '%s'", source);
		count += snprintf(buf + count, buf_len - count, ".\n");
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

			count += snprintf(buf + count, buf_len - count, "%sDid You mean '%s%s' instead of '%s'.\n",
						use_prefix,
						strlen(similar) > 1 ? d_hyphen : hyphen,
						similar,
						name);
		}

		i++;
	}

	buf[buf_len - 1] = '\0';
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


	count += snprintf(buf + count, buf_len - count, "  %3s %10s %50s %10s\n",
			"nr", "value", "source", "priority");

	for (i = 0; i < set->count; i++) {
		count += snprintf(buf + count, buf_len - count, "Parameter: '%s' (%i):\n",
				set->parameter[i]->flagName, set->parameter[i]->argCount);


		n = 0;
		while (PARAM_getValue(set->parameter[i], NULL, PST_PRIORITY_NONE, n, &param_value) == PST_OK) {
			res = PARAM_VAL_extract(param_value, &value, &source, &priority);
			if (res != PST_OK) return NULL;

			count += snprintf(buf + count, buf_len - count, "  %2i) '%s' %50s %10i\n",
					n,
					value == NULL ? "-" : value,
					source == NULL ? "-" : source,
					priority);

			n++;
		}
	}
	count += snprintf(buf + count, buf_len - count, "\n");

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
	case PST_NEGATIVE_PRIORITY:
		return "Negative priority.";
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
	case PST_UNDEFINED_BEHAVIOUR:
		return "PARAM_SET undefined behaviour.";
	case PST_UNKNOWN_ERROR:
		return "PARAM_SET unknown error.";
	}
	return "PARAM_SET unknown error.";
}