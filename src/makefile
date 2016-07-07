#
# GUARDTIME CONFIDENTIAL
#
# Copyright (C) [2015] Guardtime, Inc
# All Rights Reserved
#
# NOTICE:  All information contained herein is, and remains, the
# property of Guardtime Inc and its suppliers, if any.
# The intellectual and technical concepts contained herein are
# proprietary to Guardtime Inc and its suppliers and may be
# covered by U.S. and Foreign Patents and patents in process,
# and are protected by trade secret or copyright law.
# Dissemination of this information or reproduction of this
# material is strictly forbidden unless prior written permission
# is obtained from Guardtime Inc.
# "Guardtime" and "KSI" are trademarks or registered trademarks of
# Guardtime Inc.
#

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

PARAM_SET_DIR = param_set
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

CCFLAGS = /DVERSION=\"$(VERSION)\" /DTOOL_NAME=\"$(TOOL_NAME)\" /I$(SRC_DIR) /I$(PARAM_SET_DIR) /I$(TOOL_BOX_DIR)
CCFLAGS = $(CCFLAGS) $(CCEXTRA)

default: $(OBJ_DIR) $(LIB_OBJ) build_param_set build_tool_box

build_param_set:
	cd $(PARAM_SET_DIR)
	nmake /S RTL=$(RTL) DLL=$(DLL)
	cd ..

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
	cd $(PARAM_SET_DIR)
	nmake RTL=$(RTL) clean
	cd ..
	
	cd $(TOOL_BOX_DIR)
	nmake RTL=$(RTL) clean
	cd ..
	
	del $(LIB_OBJ)
	