!IF "$(RTL)" != "MT" && "$(RTL)" != "MTd" && "$(RTL)" != "MD" && "$(RTL)" != "MDd"
!ERROR "ENGINE MAKE: You Must specify RTL as MT, MTd, MD or MDd"
!ENDIF

#Selecting of C Run-Time library and output format. 
!IF "$(DLL)" != "lib" && "$(DLL)" != "dll"
!ERROR "ENGINE MAKE: You Must specify DLL as lib or dll"
!ENDIF

LIB_NAME = libparamset

SRC_DIR = .
OBJ_DIR = ..\..\obj\$(LIB_NAME)$(RTL)
LIB_DIR = ..\..\out\$(DLL)
LIB_DEF = $(SRC_DIR)\$(LIB_NAME).def


LIB_OBJ = \
	$(OBJ_DIR)\param_value.obj \
	$(OBJ_DIR)\param_set.obj \
	$(OBJ_DIR)\parameter.obj \
	$(OBJ_DIR)\task_def.obj \
	
	
#Compiler and linker configuration
EXT_LIB = user32.lib gdi32.lib advapi32.lib Ws2_32.lib
	  
	
CCFLAGS = /nologo /W4 /D_CRT_SECURE_NO_DEPRECATE  /I$(SRC_DIR)
LDFLAGS = /NOLOGO 


!IF "$(RTL)" == "MT" || "$(RTL)" == "MD"
CCFLAGS = $(CCFLAGS) /DNDEBUG /O2
LDFLAGS = $(LDFLAGS) /RELEASE
!ELSE
CCFLAGS = $(CCFLAGS) /D_DEBUG /Od /RTC1 /Zi
!IF "$(DLL)" == "dll"
LDFLAGS = $(LDFLAGS) /DEBUG
!ENDIF
!ENDIF

CCFLAGS = $(CCFLAGS) $(CCEXTRA)
LDFLAGS = $(LDFLAGS) $(LDEXTRA)
#Making

default: $(LIB_DIR)\$(LIB_NAME)$(RTL).$(DLL)

$(LIB_DIR)\$(LIB_NAME)$(RTL).lib: $(OBJ_DIR) $(LIB_DIR) $(LIB_OBJ)  
	link /LIB /OUT:$@ $(LDFLAGS) $(LIB_OBJ)

$(LIB_DIR)\$(LIB_NAME)$(RTL).dll: $(OBJ_DIR) $(LIB_DIR) $(LIB_OBJ) $(LIB_DEF)
	link /DLL /OUT:$@ /IMPLIB:$*.lib /DEF:$(LIB_DEF) $(LDFLAGS) $(LIB_OBJ) $(EXT_LIB)
#C file compilation  	
{$(SRC_DIR)\}.c{$(OBJ_DIR)\}.obj:
	cl /c /$(RTL) $(CCFLAGS) /Fo$@ $<

	

#Folder factory	
$(OBJ_DIR) $(LIB_DIR):
	@if not exist $@ mkdir $@
	
clean:
	@for %i in ($(OBJ_DIR) $(LIB_DIR)) do @if exist .\%i rmdir /s /q .\%i
	