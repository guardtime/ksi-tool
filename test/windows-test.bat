rmdir /S /Q test\out\sign
mkdir test\out\sign
shelltest test\test_suites\sign.test test\test_suites\verify.test --with=bin\ksi.exe -- -j15
shelltest test\test_suites\sign-verify.test --with=bin\ksi.exe -- -j8
