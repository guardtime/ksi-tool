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
!IF "$(INSTALL_MACHINE)" != "32" && "$(INSTALL_MACHINE)" != "64"
!IF "$(INSTALL_MACHINE)" == "x64"
INSTALL_MACHINE = 64
!ELSE
INSTALL_MACHINE = 32
!ENDIF
!ENDIF

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

PARAMSET_SRC_DIR = $(SRC_DIR)\param_set
PARAMSET_OBJ_DIR = $(OBJ_DIR)\param_set
TOOL_BOX_SRC_DIR = $(SRC_DIR)\tool_box
TOOL_BOX_OBJ_DIR = $(OBJ_DIR)\tool_box

VERSION_FILE = VERSION
COMM_ID_FILE = COMMIT_ID


TOOL_NAME = ksitool

#Objects for making command-line tool

PARAMSET_OBJ = \
	$(PARAMSET_OBJ_DIR)\param_value.obj \
	$(PARAMSET_OBJ_DIR)\parameter.obj \
	$(PARAMSET_OBJ_DIR)\param_set.obj \
	$(PARAMSET_OBJ_DIR)\task_def.obj

CMDTOOL_OBJ = \
	$(OBJ_DIR)\ksitool.obj \
	$(OBJ_DIR)\component.obj \
	$(OBJ_DIR)\printer.obj \
	$(OBJ_DIR)\ksitool_err.obj \
	$(OBJ_DIR)\api_wrapper.obj \
	$(OBJ_DIR)\obj_printer.obj \
	$(OBJ_DIR)\debug_print.obj
	
TOOL_BOX_OBJ = \
    $(TOOL_BOX_OBJ_DIR)\tool_box.obj \
    $(TOOL_BOX_OBJ_DIR)\ksi_init.obj \
    $(TOOL_BOX_OBJ_DIR)\pubfile.obj \
    $(TOOL_BOX_OBJ_DIR)\extend.obj \
    $(TOOL_BOX_OBJ_DIR)\sign.obj \
    $(TOOL_BOX_OBJ_DIR)\verify.obj \
    $(TOOL_BOX_OBJ_DIR)\smart_file.obj \
    $(TOOL_BOX_OBJ_DIR)\param_control.obj


#Compiler and linker configuration


EXT_LIB = libksiapi$(RTL).lib \
	user32.lib gdi32.lib advapi32.lib Ws2_32.lib
	  
	
CCFLAGS = /nologo /W3 /D_CRT_SECURE_NO_DEPRECATE  /I$(SRC_DIR) /I$(PARAMSET_SRC_DIR) /I$(TOOL_BOX_SRC_DIR) /I$(KSI_DIR)\include /DTOOL_NAME=\"$(TOOL_NAME)\"
LDFLAGS = /NOLOGO /LIBPATH:"$(KSI_DIR)\$(KSI_LIB)"

!IF "$(KSI_LIB)" == "dll"
CCFLAGS = $(CCFLAGS) /DLINKEAGAINSTDLLKSI
!MESSAGE LNINKING AGAINST DLL
!ENDIF

!IF "$(RTL)" == "MT" || "$(RTL)" == "MD"
CCFLAGS = $(CCFLAGS) /DNDEBUG /O2
LDFLAGS = $(LDFLAGS) /RELEASE
!ELSE
CCFLAGS = $(CCFLAGS) /D_DEBUG /Od /RTC1 /Zi
LDFLAGS = $(LDFLAGS) /DEBUG
!ENDIF

!IF "$(LNK_CURL)" == "yes" || "$(LNK_CURL)" == "YES"
LDFLAGS = $(LDFLAGS) /LIBPATH:"$(CURL_DIR)\$(KSI_LIB)"
CCFLAGS = $(CCFLAGS) /I"$(CURL_DIR)\include"
CCFLAGS = $(CCFLAGS) /DCURL_STATICLIB
EXT_LIB = $(EXT_LIB) libcurl$(RTL).lib
!ENDIF

!IF "$(LNK_OPENSSL)" == "yes" || "$(LNK_OPENSSL)" == "YES"
LDFLAGS = $(LDFLAGS) /LIBPATH:"$(OPENSSL_DIR)\$(KSI_LIB)"
CCFLAGS = $(CCFLAGS) /I"$(OPENSSL_DIR)\include"
EXT_LIB = $(EXT_LIB) libeay32$(RTL).lib
!ENDIF

!IF "$(LNK_WININET)" == "yes" || "$(LNK_WININET)" == "YES"
EXT_LIB = $(EXT_LIB) wininet.lib
!ENDIF

!IF "$(LNK_WINHTTP)" == "yes" || "$(LNK_WINHTTP)" == "YES"
EXT_LIB = $(EXT_LIB) winhttp.lib
!ENDIF

!IF "$(LNK_CRYPTOAPI)" == "yes" || "$(LNK_CRYPTOAPI)" == "YES"
EXT_LIB = $(EXT_LIB) crypt32.lib
!ENDIF


CCFLAGS = $(CCFLAGS) $(CCEXTRA)
LDFLAGS = $(LDFLAGS) $(LDEXTRA)

VER = \
!INCLUDE <$(VERSION_FILE)>


!IF [git log -n1 --format="%H">$(COMM_ID_FILE)] == 0
COM_ID = \
!INCLUDE <$(COMM_ID_FILE)>
!MESSAGE Git OK. Include commit ID $(COM_ID).
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
$(BIN_DIR)\$(TOOL_NAME).exe: $(BIN_DIR) $(OBJ_DIR) $(PARAMSET_OBJ_DIR) $(TOOL_BOX_OBJ_DIR) $(CMDTOOL_OBJ) $(PARAMSET_OBJ) $(TOOL_BOX_OBJ)
	link $(LDFLAGS) /OUT:$@ $(CMDTOOL_OBJ) $(PARAMSET_OBJ) $(TOOL_BOX_OBJ) $(EXT_LIB)
!IF "$(KSI_LIB)" == "dll"
	xcopy "$(KSI_DIR)\$(KSI_LIB)\libksiapi$(RTL).dll" "$(BIN_DIR)\" /Y
!ENDIF
	
#C file compilation  	
{$(SRC_DIR)\}.c{$(OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<

{$(PARAMSET_SRC_DIR)\}.c{$(PARAMSET_OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<

{$(TOOL_BOX_SRC_DIR)\}.c{$(TOOL_BOX_OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<
	

#Folder factory	
	
$(OBJ_DIR) $(PARAMSET_OBJ_DIR) $(TOOL_BOX_OBJ_DIR) $(BIN_DIR):
	@if not exist $@ mkdir $@

!IF "$(KSI_LIB)" == "lib"

installer:$(BIN_DIR) $(OBJ_DIR) $(BIN_DIR)\$(TOOL_NAME).exe
!IF [candle.exe > nul] != 0
!MESSAGE Please install WiX to build installer.
!ELSE
	cd packaging\win
	candle.exe ksitool.wxs -o ..\..\obj\ksitool.wixobj -dVersion=$(VER) -dName=$(TOOL_NAME) -dKsitool=$(BIN_DIR)\$(TOOL_NAME).exe -darch=$(INSTALL_MACHINE)
	light.exe ..\..\obj\ksitool.wixobj -o ..\..\bin\ksitool -pdbout ..\..\bin\ksitool.wixpdb -cultures:en-us -ext WixUIExtension
	cd ..\..
!ENDIF

!ENDIF
	
clean:
	@for %i in ($(OBJ_DIR) $(BIN_DIR)) do @if exist .\%i rmdir /s /q .\%i

