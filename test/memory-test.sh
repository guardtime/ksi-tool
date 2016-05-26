# Remove memory test directory.
rm -rf test/out/memory 2> /dev/null

# Create a temporary output directory for memory tests.
mkdir -p test/out/memory

# Configure temporary KSI_CONF .
export KSI_CONF=test/resource/conf/default-not-working-conf.cfg

# Convert test files to valgrind memory test files.
test/convert-to-memory-test.sh test/test_suites/sign.test test/out/memory/sign.test
test/convert-to-memory-test.sh test/test_suites/static-sign.test test/out/memory/static-sign.test
test/convert-to-memory-test.sh test/test_suites/sign-verify.test test/out/memory/sign-verify.test
test/convert-to-memory-test.sh test/test_suites/extend.test test/out/memory/extend.test
test/convert-to-memory-test.sh test/test_suites/extend-verify.test test/out/memory/extend-verify.test
test/convert-to-memory-test.sh test/test_suites/static-verify.test test/out/memory/static-verify.test
test/convert-to-memory-test.sh test/test_suites/static-sign-verify.test test/out/memory/static-sign-verify.test
test/convert-to-memory-test.sh test/test_suites/static-extend.test test/out/memory/static-extend.test
test/convert-to-memory-test.sh test/test_suites/sign-cmd.test test/out/memory/sign-cmd.test
test/convert-to-memory-test.sh test/test_suites/extend-cmd.test  test/out/memory/extend-cmd.test 
test/convert-to-memory-test.sh test/test_suites/static-verify-invalid-signatures.test  test/out/memory/static-verify-invalid-signatures.test
test/convert-to-memory-test.sh test/test_suites/pubfile.test  test/out/memory/pubfile.test
test/convert-to-memory-test.sh test/test_suites/static-pubfile.test  test/out/memory/static-pubfile.test
test/convert-to-memory-test.sh test/test_suites/verify-invalid-pubfile.test  test/out/memory/verify-invalid-pubfile.test
test/convert-to-memory-test.sh test/test_suites/verify-cmd.test  test/out/memory/verify-cmd.test
test/convert-to-memory-test.sh test/test_suites/default-conf.test  test/out/memory/default-conf.test
test/convert-to-memory-test.sh test/test_suites/invalid-conf.test  test/out/memory/invalid-conf.test


# Run generated test scripts.

shelltest \
test/out/memory/sign.test \
test/out/memory/static-sign.test \
test/out/memory/sign-verify.test \
test/out/memory/extend.test \
test/out/memory/extend-verify.test \
test/out/memory/static-verify.test \
test/out/memory/static-sign-verify.test \
test/out/memory/static-extend.test \
test/out/memory/sign-cmd.test \
test/out/memory/extend-cmd.test \
test/out/memory/static-verify-invalid-signatures.test \
test/out/memory/pubfile.test \
test/out/memory/static-pubfile.test \
test/out/memory/verify-invalid-pubfile.test \
test/out/memory/verify-cmd.test \
test/out/memory/default-conf.test \
test/out/memory/invalid-conf.test \
--with="valgrind --leak-check=full src/ksi" -- -j1
exit_code=$?

exit $exit_code