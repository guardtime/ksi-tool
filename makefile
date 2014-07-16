!IF "$(DLL)" != "lib" && "$(DLL)" != "dll"
DLL = lib
!ENDIF
!IF "$(RTL)" != "MT" && "$(RTL)" != "MTd" && "$(RTL)" != "MD" && "$(RTL)" != "MDd"
RTL = MT
!ENDIF

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TOOL_NAME = gtime

#Objects for making command-line tool

CMDTOOL_OBJ = \
	$(OBJ_DIR)\getopt.obj \
	$(OBJ_DIR)\gt_cmd_parameters.obj \
	$(OBJ_DIR)\gt_task.obj \
	$(OBJ_DIR)\gt_task_extend.obj \
	$(OBJ_DIR)\gt_task_getpubfile.obj \
	$(OBJ_DIR)\gt_task_sign.obj \
	$(OBJ_DIR)\gt_task_verify.obj \
	$(OBJ_DIR)\gtime.obj \
	$(OBJ_DIR)\gt_cmd_control.obj


	
#Compiler and linker configuration


EXT_LIB = libeay32$(RTL).lib \
	user32.lib gdi32.lib advapi32.lib crypt32.lib\
	libksiapi$(RTL).$(DLL) libcurl$(RTL).lib 
	
CCFLAGS = /nologo /W3 /D_CRT_SECURE_NO_DEPRECATE /DCURL_STATICLIB /I$(SRC_DIR) /I$(KSI_DIR)\include 
LDFLAGS = /NOLOGO /LIBPATH:$(KSI_DIR)\$(DLL) /NODEFAULTLIB:libcmt.lib

!IF "$(RTL)" == "MT" || "$(RTL)" == "MD"
CCFLAGS = $(CCFLAGS) /DNDEBUG /O2
LDFLAGS = $(LDFLAGS) /RELEASE
!ELSE
CCFLAGS = $(CCFLAGS) /D_DEBUG /Od /RTC1 /Zi
LDFLAGS = $(LDFLAGS) /DEBUG
!ENDIF
CCFLAGS = $(CCFLAGS) /I"$(OPENSSL_DIR)\include" /I"$(CURL_DIR)\include"
!IF "$(DLL)" == "lib"
LDFLAGS = $(LDFLAGS) /LIBPATH:"$(OPENSSL_DIR)\lib" 
LDFLAGS = $(LDFLAGS) /LIBPATH:"$(CURL_DIR)\lib"
!ELSE
LDFLAGS = $(LDFLAGS) /LIBPATH:"$(OPENSSL_DIR)\dll" 
LDFLAGS = $(LDFLAGS) /LIBPATH:"$(CURL_DIR)\dll"
!ENDIF
CCFLAGS = $(CCFLAGS) $(CCEXTRA)
LDFLAGS = $(LDFLAGS) $(LDEXTRA)


#Making

default: $(BIN_DIR)\$(TOOL_NAME).exe 

#Linking 
$(BIN_DIR)\$(TOOL_NAME).exe: $(BIN_DIR) $(OBJ_DIR) $(CMDTOOL_OBJ)
	link $(LDFLAGS) /OUT:$@ $(CMDTOOL_OBJ) $(EXT_LIB) 

#C file compilation  	
{$(SRC_DIR)\}.c{$(OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<




#Folder factory	
	
$(OBJ_DIR) $(BIN_DIR):
	@if not exist $@ mkdir $@
	
clean:
	@for %i in ($(OBJ_DIR) $(BIN_DIR)) do @if exist .\%i rmdir /s /q .\%i
