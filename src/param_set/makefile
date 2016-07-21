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
!ERROR "src/param_set/makefile: You Must specify RTL as MT, MTd, MD or MDd"
!ENDIF


SRC_DIR = .
OBJ_DIR = ..\..\obj\param_set

LIB_OBJ = \
	$(OBJ_DIR)\param_value.obj \
	$(OBJ_DIR)\param_set.obj \
	$(OBJ_DIR)\parameter.obj \
	$(OBJ_DIR)\task_def.obj \
	$(OBJ_DIR)\strn.obj

CCFLAGS = /I$(SRC_DIR)
CCFLAGS = $(CCFLAGS) $(CCEXTRA)
#Making

default: $(OBJ_DIR) $(LIB_OBJ)

#C file compilation  	
{$(SRC_DIR)\}.c{$(OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<

#Folder factory	
$(OBJ_DIR):
	@if not exist $@ mkdir $@
	
clean:
	@for %i in ($(OBJ_DIR)) do @if exist .\%i rmdir /s /q .\%i
	