rmdir /S /Q test\out\sign
rmdir /S /Q test\out\extend
mkdir test\out\sign
mkdir test\out\extend

shelltest ^
test\test_suites\sign.test ^
test\test_suites\static-sign.test ^
test\test_suites\sign-verify.test ^
test\test_suites\extend.test ^
test\test_suites\extend-verify.test ^
test\test_suites\static-verify.test ^
test\test_suites\static-sign-verify.test ^
test\test_suites\static-extend.test ^
--with=bin\ksi.exe -- -j1
