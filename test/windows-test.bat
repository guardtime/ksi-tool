GOTO copyrightend

    Copyright 2013-2016 Guardtime, Inc.

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

REM Remove test output directories.
rmdir /S /Q test\out\extend
rmdir /S /Q test\out\extend-replace-existing
rmdir /S /Q test\out\sign
rmdir /S /Q test\out\pubfile
rmdir /S /Q test\out\fname
rmdir /S /Q test\out\mass_extend

REM Create test output directories.
mkdir test\out\sign
mkdir test\out\extend
mkdir test\out\extend-replace-existing
mkdir test\out\pubfile
mkdir test\out\fname
mkdir test\out\mass_extend

REM Create some test files to output directory.
copy /Y test\resource\file\testFile	test\out\fname\_
copy /Y test\resource\file\testFile	test\out\fname\10__
copy /Y test\resource\file\testFile	test\out\fname\test_file
copy /Y test\resource\file\testFile	test\out\fname\a_23_500
copy /Y test\resource\file\testFile	test\out\fname\a_23_1000
copy /Y test\resource\file\testFile	test\out\fname\a_23_1000.ksig
copy /Y test\resource\file\testFile	test\out\fname\a_23_1000_5.ksig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\fname\ok-sig.ksig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\fname\ok-sig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\fname\mass-extend-1.ksig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\fname\mass-extend-2.ksig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\fname\mass-extend-2.ksig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\extend-replace-existing\not-extended-1A.ksig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\extend-replace-existing\not-extended-1B.ksig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\extend-replace-existing\not-extended-2B.ksig

REM Define KSI_CONF for temporary testing.
setlocal

set KSI_CONF=test/resource/conf/default-not-working-conf.cfg

REM If ksi tool in project directory is available use that one, if not
REM use the one installed in the machine.
if exist bin\ksi.exe (
    set tool=bin\ksi.exe
) else (
    set tool=ksi
)

REM If gttlvdump and gttlvgrep exists include the tests using gttlvutil

gttlvdump -h > NUL && gttlvgrep -h > NUL
if not ERRORLEVEL 1 (
	set TEST_DEPENDING_ON_TLVUTIL=test\test_suites\sign-metadata.test test\test_suites\sign-masking.test
) else (
	set TEST_DEPENDING_ON_TLVUTIL=
)

gttlvdump -h > NUL && gttlvgrep -h > NUL && grep --help > NUL
if not ERRORLEVEL 1 (
	REM Add test/test_suites/tlvutil-pdu-header.test to the list when gttlvutil new version is released.
	REM set TEST_DEPENDING_ON_TLVUTIL_GREP=test\test_suites\tlvutil-pdu-header.test
	set TEST_DEPENDING_ON_TLVUTIL_GREP=
) else (
	set TEST_DEPENDING_ON_TLVUTIL_GREP=
)

shelltest ^
test\test_suites\sign.test ^
test\test_suites\static-sign.test ^
test\test_suites\sign-verify.test ^
test\test_suites\extend.test ^
test\test_suites\mass-extend.test ^
test\test_suites\extend-verify.test ^
test\test_suites\static-verify.test ^
test\test_suites\static-sign-verify.test ^
test\test_suites\static-extend.test ^
test\test_suites\win-pipe.test ^
test\test_suites\sign-cmd.test ^
test\test_suites\extend-cmd.test ^
test\test_suites\static-verify-invalid-signatures.test ^
test\test_suites\pubfile.test ^
test\test_suites\static-pubfile.test ^
test\test_suites\verify-invalid-pubfile.test ^
test\test_suites\verify-cmd.test ^
test\test_suites\default-conf.test ^
test\test_suites\invalid-conf.test ^
test\test_suites\file-name-gen.test ^
test\test_suites\sign-block-signer.test ^
test\test_suites\sign-block-signer-cmd.test ^
%TEST_DEPENDING_ON_TLVUTIL% ^
%TEST_DEPENDING_ON_TLVUTIL_GREP% ^
--with=%tool% -- -j1
set exit_code=%errorlevel%

endlocal

exit /B %exit_code%