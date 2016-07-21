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

#ifndef DEFAULT_TASKS_H
#define	DEFAULT_TASKS_H

#ifdef	__cplusplus
extern "C" {
#endif

int sign_run(int argc, char** argv, char **envp);
char *sign_help_toString(char*buf, size_t len);
const char *sign_get_desc(void);

const char *verify_get_desc(void);
char *verify_help_toString(char*buf, size_t len);
int verify_run(int argc, char** argv, char **envp);

const char *extend_get_desc(void);
char *extend_help_toString(char*buf, size_t len);
int extend_run(int argc, char** argv, char **envp);

int pubfile_run(int argc, char** argv, char **envp);
char *pubfile_help_toString(char*buf, size_t len);
const char *pubfile_get_desc(void);

int conf_run(int argc, char** argv, char **envp);
char *conf_help_toString(char *buf, size_t len);
const char *conf_get_desc(void);

#ifdef	__cplusplus
}
#endif

#endif	/* DEFAULT_TASKS_H */

