#
# Copyright 2013-2018 Guardtime, Inc.
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
!ERROR "src/makefile: You Must specify RTL as MT, MTd, MD or MDd"
!ENDIF


!IFNDEF VERSION
!ERROR "src/makefile: VERSION must be specified!"
!ENDIF

!IFNDEF TOOL_NAME
!ERROR "src/makefile: TOOL_NAME must be specified!"
!ENDIF

SRC_DIR = .
OBJ_DIR = ..\obj

TOOL_BOX_DIR = tool_box

LIB_OBJ = \
	$(OBJ_DIR)\main.obj \
	$(OBJ_DIR)\conf_file.obj \
	$(OBJ_DIR)\ksitool_err.obj \
	$(OBJ_DIR)\api_wrapper.obj \
	$(OBJ_DIR)\printer.obj \
	$(OBJ_DIR)\obj_printer.obj \
	$(OBJ_DIR)\debug_print.obj \
	$(OBJ_DIR)\component.obj \
	$(OBJ_DIR)\tool_box.obj \
	$(OBJ_DIR)\smart_file.obj \
	$(OBJ_DIR)\err_trckr.obj


!IF "$(COM_ID)" != ""
CCFLAGS = $(CCFLAGS) /DCOMMIT_ID=\"$(COM_ID)\"
!ENDIF
!IF "$(VER)" != ""
CCFLAGS = $(CCFLAGS) /DVERSION=\"$(VER)\"
!ENDIF

CCFLAGS = /DVERSION=\"$(VERSION)\" /DTOOL_NAME=\"$(TOOL_NAME)\" /I$(SRC_DIR) /I$(TOOL_BOX_DIR)
CCFLAGS = $(CCFLAGS) $(CCEXTRA)

default: $(OBJ_DIR) $(LIB_OBJ)  build_tool_box

build_tool_box:
	cd $(TOOL_BOX_DIR)
	nmake /S RTL=$(RTL) DLL=$(DLL)
	cd ..

#C file compilation
{$(SRC_DIR)\}.c{$(OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<

#Folder factory
$(OBJ_DIR):
	@if not exist $@ mkdir $@

clean:
	cd $(TOOL_BOX_DIR)
	nmake RTL=$(RTL) clean
	cd ..

	del $(LIB_OBJ)
