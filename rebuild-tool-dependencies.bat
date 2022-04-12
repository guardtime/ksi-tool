GOTO copyrightend

    Copyright 2013-2018 Guardtime, Inc.

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

@echo off
setlocal

REM Git repositories and corresponding versions.
set libksi_git="https://github.com/guardtime/libksi.git"
set libksi_version=v3.21.3075
set libparamset_git="https://github.com/guardtime/libparamset.git"
set libparamset_version=v1.1.240

REM Temporary folders for building libksi and libparamset.
set tmp_dir=tmp_dep_build
set libksi_dir_name=libksi
set libparamset_dir_name=libparamset

REM Output directories.
set out_dir=dependencies
set libksi_inc_dir=%out_dir%\libksi\include
set libksi_lib_dir=%out_dir%\libksi\lib
set libksi_dll_dir=%out_dir%\libksi\dll
set libparamset_inc_dir=%out_dir%\libparamset\include
set libparamset_lib_dir=%out_dir%\libparamset\lib
set libparamset_dll_dir=%out_dir%\libparamset\dll

REM Parsing input parameters.
set libksimakeopt=%~1
set libparamsetmakeopt=%~2
set ignore_exit_code=%~3

set argc=0
for %%x in (%*) do Set /A argc+=1

if /I "%argc%" EQU "2" (
	echo Rebuilding dependencies for KSI tool.
) else if /I "%argc%" EQU "3" (
	echo Rebuilding dependencies for KSI tool without testing.
	if /I "%ignore_exit_code%" EQU "--ign-dep-online-err" (
		echo "NB! Tests for dependencies ignored."
	) else (
		echo Unknown parameter %ignore_exit_code%
		exit /B 1
	)
) else (
	echo Usage:
	echo.
	echo   %0 'libksi make options' 'libparamset make options'
	echo.
	echo Description:
	echo.
	echo   This script needs exactly 2 parameters - make options for libksi and
	echo   libparamset. Extra parameter 3 can be used. If given value
	echo   --ign-dep-online-err tests for dependencies are ignored.
	echo.
	echo   A temporary folder '%tmp_dir%' will be created.
	echo   On success, include and library files will be generated into following folders:
	echo   folders:
	echo.
	echo.    %libksi_inc_dir%
	echo.    %libksi_lib_dir%
	echo.    %libksi_dll_dir%
	echo.    %libparamset_inc_dir%
	echo.    %libparamset_lib_dir%
	echo.    %libparamset_dll_dir%
	exit /B 1
)


REM Get and build dependecies.
if exist %tmp_dir% rd /S /Q %tmp_dir%
md %tmp_dir%
if exist %out_dir% rd /S /Q %out_dir%

cd %tmp_dir%
  git clone %libksi_git% %libksi_dir_name%
  if %errorlevel% neq 0 exit /b %errorlevel%

  git clone %libparamset_git% %libparamset_dir_name%
  if %errorlevel% neq 0 exit /b %errorlevel%

  cd %libksi_dir_name%
	git checkout %libksi_version%
	if %errorlevel% neq 0 exit /b %errorlevel%

	nmake %libksimakeopt% clean test
	if %errorlevel% neq 0 if /I not "%ignore_exit_code%" EQU "--ign-dep-online-err" exit /b %errorlevel%
  cd ..

  cd %libparamset_dir_name%
	git checkout %libparamset_version%
	if %errorlevel% neq 0 if /I not "%ignore_exit_code%" EQU "--ign-dep-online-err" exit /b %errorlevel%


	nmake %libparamsetmakeopt% clean test
	if %errorlevel% neq 0 if /I not "%ignore_exit_code%" EQU "--ign-dep-online-err" exit /b %errorlevel%

	REM Remove when libparamset windows build is fixed.
	out\bin\test.exe .\test
	if %errorlevel% neq 0 exit /b %errorlevel%
  cd ..
cd ..

REM Create and fill output directory.
if not exist %libksi_inc_dir%\ksi md %libksi_inc_dir%
if not exist %out_dir%\libksi\lib md %libksi_lib_dir%
if not exist %out_dir%\libksi\dll md %libksi_dll_dir%
if not exist %out_dir%\libparamset\include\param_set md %libparamset_inc_dir%
if not exist %out_dir%\libparamset\lib md %libparamset_lib_dir%
if not exist %out_dir%\libparamset\dll md %libparamset_dll_dir%

xcopy /s /y %tmp_dir%\%libksi_dir_name%\out\include %libksi_inc_dir%
xcopy /s /y %tmp_dir%\%libksi_dir_name%\out\dll\* %libksi_dll_dir%
xcopy /s /y %tmp_dir%\%libksi_dir_name%\out\lib\* %libksi_lib_dir%
xcopy /y %tmp_dir%\%libksi_dir_name%\src\ksi\*.pdb %libksi_lib_dir%
xcopy /s /y %tmp_dir%\%libparamset_dir_name%\out\include %libparamset_inc_dir%
xcopy /s /y %tmp_dir%\%libparamset_dir_name%\out\lib\* %libparamset_lib_dir%
xcopy /s /y %tmp_dir%\%libparamset_dir_name%\out\dll\* %libparamset_dll_dir%
xcopy /y %tmp_dir%\%libparamset_dir_name%\src\param_set\*.pdb %libparamset_lib_dir%

REM Remove tmp directory.
if exist %tmp_dir% rd /S /Q %tmp_dir%

endlocal
exit /B %errorlevel%