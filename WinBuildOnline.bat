GOTO copyrightend

    Copyright 2013-2019 Guardtime, Inc.

    This file is part of the Guardtime client SDK.

    Licensed under the Apache License, Version 2.0 (the "License").
    You may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
    express or implied. See the License for the specific language governing
    permissions and limitations under the License.
    "Guardtime" and "KSI" are trademarks or registered trademarks of
    Guardtime, Inc., and no license to trademarks is granted; Guardtime
    reserves and retains all trademark rights.

:copyrightend

@ECHO OFF
setlocal

nmake /? > NUL
IF ERRORLEVEL 1 (
	echo Error: This script must be run from Visual Studio Developer Command Prompt and 1>&2
	echo.       Visual Studio build tools must be installed. 1>&2
	exit /b %errorlevel%
)

SET KSI_DIR=%cd%\dependencies\libksi
SET PST_DIR=%cd%\dependencies\libparamset
SET rtl=MT
SET dll=lib

nmake clean
call rebuild-tool-dependencies.bat "RTL=%rtl% DLL=%dll% NET_PROVIDER=WININET CRYPTO_PROVIDER=CRYPTOAPI" "RTL=%rtl% DLL=%dll%" %1
nmake  RTL=%rtl% KSI_LIB=%dll% LNK_WININET=yes LNK_CRYPTOAPI=yes LNK_WINHTTP=no LNK_CURL=no LNK_OPENSSL=no

endlocal
pause
