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
	