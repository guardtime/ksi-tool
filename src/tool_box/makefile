#
# Copyright 2013-2016 Guardtime, Inc.
#
# This file is part of the Guardtime client SDK.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
# "Guardtime" and "KSI" are trademarks or registered trademarks of
# Guardtime, Inc., and no license to trademarks is granted; Guardtime
# reserves and retains all trademark rights.

# Check if C Run-Time library and output format is specified.
!IF "$(RTL)" != "MT" && "$(RTL)" != "MTd" && "$(RTL)" != "MD" && "$(RTL)" != "MDd"
!ERROR "src/tool_box/makefile: You Must specify RTL as MT, MTd, MD or MDd"
!ENDIF


SRC_DIR = .
OBJ_DIR = ..\..\obj\tool_box

PARAM_SET_DIR = ..\param_set
MAIN_DIR = ..\

LIB_OBJ = \
	$(OBJ_DIR)\ksi_init.obj \
	$(OBJ_DIR)\pubfile.obj \
	$(OBJ_DIR)\extend.obj \
	$(OBJ_DIR)\sign.obj \
	$(OBJ_DIR)\verify.obj \
	$(OBJ_DIR)\task_initializer.obj \
	$(OBJ_DIR)\param_control.obj \
	$(OBJ_DIR)\conf.obj

CCFLAGS = /I$(SRC_DIR) /I$(PARAM_SET_DIR) /I$(MAIN_DIR)
CCFLAGS = $(CCFLAGS) $(CCEXTRA)

default: $(OBJ_DIR) $(LIB_OBJ)

#C file compilation  	
{$(SRC_DIR)\}.c{$(OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<

#Folder factory	
$(OBJ_DIR):
	@if not exist $@ mkdir $@
	
clean:
	@for %i in ($(OBJ_DIR)) do @if exist .\%i rmdir /s /q .\%i
	