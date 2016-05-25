REM Remove test output directories.
rmdir /S /Q test\out\extend
rmdir /S /Q test\out\sign
rmdir /S /Q test\out\pubfile
rmdir /S /Q test\out\fname

REM Create test output directories.
mkdir test\out\sign
mkdir test\out\extend
mkdir test\out\pubfile
mkdir test\out\fname

REM Create some test files to output directory.
copy /Y test\resource\file\testFile	test\out\fname\_
copy /Y test\resource\file\testFile	test\out\fname\10__
copy /Y test\resource\file\testFile	test\out\fname\test_file
copy /Y test\resource\file\testFile	test\out\fname\a_23_500
copy /Y test\resource\file\testFile	test\out\fname\a_23_1000
copy /Y test\resource\file\testFile	test\out\fname\a_23_1000.ksig
copy /Y test\resource\file\testFile	test\out\fname\a_23_1000_5.ksig
copy /Y test\resource\signature\ok-sig-2014-08-01.1.ksig test\out\fname\ok-sig.ksig

REM Define KSI_CONF for temporary testing.
setlocal

set KSI_CONF=test/resource/conf/default-not-working-conf.cfg

shelltest ^
test\test_suites\sign.test ^
test\test_suites\static-sign.test ^
test\test_suites\sign-verify.test ^
test\test_suites\extend.test ^
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
--with=bin\ksi.exe -- -j1
set exit_code=%errorlevel%

endlocal

exit /B %exit_code%