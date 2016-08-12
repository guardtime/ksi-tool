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


OBJ_DIR = ..\..\obj
BIN_DIR = ..\..\bin
KSI_WIX_SCRIPT = ksi.wxs
KSI_WIX_OBJ = $(OBJ_DIR)\$(KSI_WIX_SCRIPT)obj
PACKAGE_NAME = $(TOOL_NAME)



installer: error_handling $(BIN_DIR)\$(TOOL_NAME).exe
	candle.exe $(KSI_WIX_SCRIPT) -o $(KSI_WIX_OBJ) -dVersion=$(VERSION) -dName=$(TOOL_NAME) -dtool_file=$(BIN_DIR)\$(TOOL_NAME).exe -darch=$(INSTALL_MACHINE)
	light.exe $(KSI_WIX_OBJ) -o $(BIN_DIR)\$(PACKAGE_NAME) -pdbout $(BIN_DIR)\$(PACKAGE_NAME).wixpdb -cultures:en-us -ext WixUIExtension


error_handling:
	@echo ""
!IFNDEF VERSION
	@echo "packaging/win/makefile: VERSION must be specified!"
	exit 1
!ENDIF
!IFNDEF TOOL_NAME
	@echo "packaging/win/makefile: TOOL_NAME must be specified!"
	exit 1
!ENDIF
!IF "$(INSTALL_MACHINE)" != "32" && "$(INSTALL_MACHINE)" != "64"
	@echo "packaging/win/makefile: INSTALL_MACHINE must be specified as 32 or 64!"
	exit 1
!ENDIF
!IF "$(KSI_LIB)" != "lib" && "$(KSI_LIB)" != "dll"
	@echo "packaging/win/makefile: KSI_LIB must be specified as lib or dll!"
	exit 1
!ENDIF
!IF [candle.exe > nul] != 0
	@echo "packaging/win/makefile: Program candle.exe is missing. Please install WiX to build installer."
	exit 1
!ENDIF
!IF [light.exe > nul] != 0
	@echo "packaging/win/makefile: Program light.exe is missing. Please install WiX to build installer."
	exit 1
!ENDIF
!IF "$(KSI_LIB)" != "lib"
	@echo "packaging/win/makefile: KSI_LIB must be lib as only static ksi installer is supported."
	exit 1
!ENDIF

clean:
	DEL $(BIN_DIR)\$(PACKAGE_NAME).msi $(KSI_WIX_OBJ) $(BIN_DIR)\$(PACKAGE_NAME).wixpdb
	