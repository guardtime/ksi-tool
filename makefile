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

!IF "$(KSI_LIB)" != "lib" && "$(KSI_LIB)" != "dll"
KSI_LIB = lib
!ENDIF
!IF "$(RTL)" != "MT" && "$(RTL)" != "MTd" && "$(RTL)" != "MD" && "$(RTL)" != "MDd"
!IF "$(KSI_LIB)" == "lib"
RTL = MT
!ELSE
RTL = MD
!ENDIF
!ENDIF

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
VERSION_FILE = VERSION
COMM_ID_FILE = COMMIT_ID

TOOL_NAME = ksitool

#Objects for making command-line tool

CMDTOOL_OBJ = \
	$(OBJ_DIR)\gt_task_support.obj \
	$(OBJ_DIR)\gt_task_extend.obj \
	$(OBJ_DIR)\gt_task_pubfile.obj \
	$(OBJ_DIR)\gt_task_sign.obj \
	$(OBJ_DIR)\gt_task_verify.obj \
	$(OBJ_DIR)\gt_task_other.obj \
	$(OBJ_DIR)\ksitool.obj \
	$(OBJ_DIR)\gt_cmd_control.obj \
	$(OBJ_DIR)\try-catch.obj \
	$(OBJ_DIR)\task_def.obj \
	$(OBJ_DIR)\param_set.obj

	
#Compiler and linker configuration


EXT_LIB = libksiapi$(RTL).lib \
	user32.lib gdi32.lib advapi32.lib crypt32.lib
	  
	
CCFLAGS = /nologo /W3 /D_CRT_SECURE_NO_DEPRECATE  /I$(SRC_DIR) /I$(KSI_DIR)\include
CCFLAGS = $(CCFLAGS) /I"$(OPENSSL_DIR)\include" /I"$(CURL_DIR)\include"
LDFLAGS = /NOLOGO /LIBPATH:"$(KSI_DIR)\$(KSI_LIB)" /LIBPATH:"$(OPENSSL_DIR)\$(KSI_LIB)" /LIBPATH:"$(CURL_DIR)\$(KSI_LIB)"

!IF "$(RTL)" == "MT" || "$(RTL)" == "MD"
CCFLAGS = $(CCFLAGS) /DNDEBUG /O2
LDFLAGS = $(LDFLAGS) /RELEASE
!ELSE
CCFLAGS = $(CCFLAGS) /D_DEBUG /Od /RTC1 /Zi
LDFLAGS = $(LDFLAGS) /DEBUG
!ENDIF

!IF "$(KSI_LIB)" == "lib"
CCFLAGS = $(CCFLAGS) /DCURL_STATICLIB
EXT_LIB = $(EXT_LIB) libcurl$(RTL).lib libeay32$(RTL).lib wininet.lib winhttp.lib

!ELSE
LDFLAGS = $(LDFLAGS) /LIBPATH:"$(OPENSSL_DIR)\dll" 
LDFLAGS = $(LDFLAGS) /LIBPATH:"$(CURL_DIR)\dll"
!ENDIF
CCFLAGS = $(CCFLAGS) $(CCEXTRA)
LDFLAGS = $(LDFLAGS) $(LDEXTRA)

VER = \
!INCLUDE <$(VERSION_FILE)>


!IF [git log -n1 --format="%H">$(COMM_ID_FILE)] == 0
COM_ID = \
!INCLUDE <$(COMM_ID_FILE)>
!MESSAGE Git OK. Include commit ID.
!IF [rm $(COMM_ID_FILE)] == 0
!MESSAGE File $(COMM_ID_FILE) deleted.
!ENDIF
!ELSE
!MESSAGE Git is not installed. 
!ENDIF 

!IF "$(COM_ID)" != ""
CCFLAGS = $(CCFLAGS) /DCOMMIT_ID=\"$(COM_ID)\"
!ENDIF
!IF "$(VER)" != ""
CCFLAGS = $(CCFLAGS) /DVERSION=\"$(VER)\"
!ENDIF


#Making

default: $(BIN_DIR)\$(TOOL_NAME).exe 

#Linking 
$(BIN_DIR)\$(TOOL_NAME).exe: $(BIN_DIR) $(OBJ_DIR) $(CMDTOOL_OBJ)
	link $(LDFLAGS) /OUT:$@ $(CMDTOOL_OBJ) $(EXT_LIB) 
!IF "$(KSI_LIB)" == "dll"
	xcopy "$(KSI_DIR)\$(KSI_LIB)\libksiapi$(RTL).dll" "$(BIN_DIR)\" /Y
!ENDIF
	
#C file compilation  	
{$(SRC_DIR)\}.c{$(OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<

	
#Folder factory	
	
$(OBJ_DIR) $(BIN_DIR):
	@if not exist $@ mkdir $@
	
clean:
	@for %i in ($(OBJ_DIR) $(BIN_DIR)) do @if exist .\%i rmdir /s /q .\%i
