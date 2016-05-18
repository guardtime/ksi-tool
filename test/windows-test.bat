rmdir /S /Q test\out\sign
rmdir /S /Q test\out\extend
rmdir /S /Q test\out\pubfile
mkdir test\out\sign
mkdir test\out\extend
mkdir test\out\pubfile

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
--with=bin\ksi.exe -- -j1

endlocal