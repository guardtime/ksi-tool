rm -f test/out/sign 2> /dev/null
rm -f test/out/extend 2> /dev/null
mkdir -p test/out/sign
mkdir -p test/out/extend

shelltest \
test/test_suites/sign.test \
test/test_suites/static-sign.test \
test/test_suites/sign-verify.test \
test/test_suites/extend.test \
test/test_suites/extend-verify.test \
test/test_suites/static-verify.test \
test/test_suites/static-sign-verify.test \
test/test_suites/static-extend.test \
--with=src/ksi -- -j1
