rmdir /S /Q test\out\sign
mkdir test\out\sign
shelltest test\test_suites\sign.test --with=bin\ksi.exe -p -c -- -j1
shelltest test\test_suites\sign-verify.test --with=bin\ksi.exe -p -c -- -j1
shelltest test\test_suites\verify.test --with=bin\ksi.exe -p -c -- -j1
