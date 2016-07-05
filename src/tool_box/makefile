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
	