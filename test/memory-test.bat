SETLOCAL

REM Run generated test scripts.

SET tool=bin\ksi.exe
SET mem_test_dir=test\out\memory
SET test_suite_dir=test\test_suites



REM Remove memory test directory.
rmdir /S /Q %mem_test_dir%

REM Create a temporary output directory for memory tests.
mkdir %mem_test_dir%
mkdir %mem_test_dir%\dr_memory

REM Create some test files to output directory.
copy test\resource\file\testFile	%mem_test_dir%\_
copy test\resource\file\testFile	%mem_test_dir%\10__
copy test\resource\file\testFile	%mem_test_dir%\test_file
copy test\resource\file\testFile	%mem_test_dir%\a_23_500
copy test\resource\file\testFile	%mem_test_dir%\a_23_1000
copy test\resource\file\testFile	%mem_test_dir%\a_23_1000.ksig
copy test\resource\file\testFile	%mem_test_dir%\a_23_1000_5.ksig
copy test\resource\signature\ok-sig-2014-08-01.1.ksig %mem_test_dir%\ok-sig.ksig
copy test\resource\signature\ok-sig-2014-08-01.1.ksig %mem_test_dir%\ok-sig

REM  Configure temporary KSI_CONF.
SET KSI_CONF=test\resource\conf\default-not-working-conf.cfg



 REM Convert test files to valgrind memory test files.
call test\convert-to-memory-test.bat %test_suite_dir%\sign.test %mem_test_dir%\sign.test
call test\convert-to-memory-test.bat %test_suite_dir%\static-sign.test %mem_test_dir%\static-sign.test
call test\convert-to-memory-test.bat %test_suite_dir%\sign-verify.test %mem_test_dir%\sign-verify.test
call test\convert-to-memory-test.bat %test_suite_dir%\extend.test %mem_test_dir%\extend.test
call test\convert-to-memory-test.bat %test_suite_dir%\extend-verify.test %mem_test_dir%\extend-verify.test
call test\convert-to-memory-test.bat %test_suite_dir%\static-verify.test %mem_test_dir%\static-verify.test
call test\convert-to-memory-test.bat %test_suite_dir%\static-sign-verify.test %mem_test_dir%\static-sign-verify.test
call test\convert-to-memory-test.bat %test_suite_dir%\static-extend.test %mem_test_dir%\static-extend.test
call test\convert-to-memory-test.bat %test_suite_dir%\sign-cmd.test %mem_test_dir%\sign-cmd.test
call test\convert-to-memory-test.bat %test_suite_dir%\extend-cmd.test %mem_test_dir%\extend-cmd.test
call test\convert-to-memory-test.bat %test_suite_dir%\static-verify-invalid-signatures.test %mem_test_dir%\static-verify-invalid-signatures.test
call test\convert-to-memory-test.bat %test_suite_dir%\pubfile.test %mem_test_dir%\pubfile.test
call test\convert-to-memory-test.bat %test_suite_dir%\static-pubfile.test %mem_test_dir%\static-pubfile.test
call test\convert-to-memory-test.bat %test_suite_dir%\verify-invalid-pubfile.test %mem_test_dir%\verify-invalid-pubfile.test
call test\convert-to-memory-test.bat %test_suite_dir%\verify-cmd.test %mem_test_dir%\verify-cmd.test
call test\convert-to-memory-test.bat %test_suite_dir%\default-conf.test %mem_test_dir%\default-conf.test
call test\convert-to-memory-test.bat %test_suite_dir%\invalid-conf.test %mem_test_dir%\invalid-conf.test
call test\convert-to-memory-test.bat %test_suite_dir%\file-name-gen.test %mem_test_dir%\file-name-gen.test
call test\convert-to-memory-test.bat %test_suite_dir%\sign-block-signer.test %mem_test_dir%\sign-block-signer.test
call test\convert-to-memory-test.bat %test_suite_dir%\sign-block-signer-cmd.test %mem_test_dir%\sign-block-signer-cmd.test

shelltest -a ^
%mem_test_dir%\sign.test ^
%mem_test_dir%\static-sign.test ^
%mem_test_dir%\sign-verify.test ^
%mem_test_dir%\extend.test ^
%mem_test_dir%\extend-verify.test ^
%mem_test_dir%\static-verify.test ^
%mem_test_dir%\static-sign-verify.test ^
%mem_test_dir%\static-extend.test ^
%mem_test_dir%\sign-cmd.test ^
%mem_test_dir%\extend-cmd.test ^
%mem_test_dir%\static-verify-invalid-signatures.test ^
%mem_test_dir%\pubfile.test ^
%mem_test_dir%\static-pubfile.test ^
%mem_test_dir%\verify-invalid-pubfile.test ^
%mem_test_dir%\verify-cmd.test ^
%mem_test_dir%\default-conf.test ^
%mem_test_dir%\invalid-conf.test ^
%mem_test_dir%\file-name-gen.test ^
%mem_test_dir%\sign-block-signer.test ^
%mem_test_dir%\sign-block-signer-cmd.test ^
--with="drmemory -leaks_only -logdir %mem_test_dir%\dr_memory -report_leak_max -1 -batch -- %tool%" -- -j1


rem -debug -dr_debug
SET exit_code=%errorlevel%

ENDLOCAL & exit /B %exit_code%