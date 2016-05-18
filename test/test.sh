rm -f test/out/sign 2> /dev/null
rm -f test/out/extend 2> /dev/null
rm -f test/out/pubfile 2> /dev/null
mkdir -p test/out/sign
mkdir -p test/out/extend
mkdir -p test/out/pubfile

export KSI_CONF=test/resource/conf/default-not-working-conf.cfg

shelltest \
test/test_suites/sign.test \
test/test_suites/static-sign.test \
test/test_suites/sign-verify.test \
test/test_suites/extend.test \
test/test_suites/extend-verify.test \
test/test_suites/static-verify.test \
test/test_suites/static-sign-verify.test \
test/test_suites/static-extend.test \
test/test_suites/linux-pipe.test \
test/test_suites/sign-cmd.test \
test/test_suites/extend-cmd.test \
test/test_suites/static-verify-invalid-signatures.test \
test/test_suites/pubfile.test \
test/test_suites/static-pubfile.test \
test/test_suites/verify-invalid-pubfile.test \
test/test_suites/verify-cmd.test \
test/test_suites/default-conf.test \
test/test_suites/invalid-conf.test \
--with=src/ksi -- -j1
