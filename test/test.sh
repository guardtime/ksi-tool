#!/bin/bash

#
# GUARDTIME CONFIDENTIAL
#
# Copyright (C) [2016] Guardtime, Inc
# All Rights Reserved
#
# NOTICE:  All information contained herein is, and remains, the
# property of Guardtime Inc and its suppliers, if any.
# The intellectual and technical concepts contained herein are
# proprietary to Guardtime Inc and its suppliers and may be
# covered by U.S. and Foreign Patents and patents in process,
# and are protected by trade secret or copyright law.
# Dissemination of this information or reproduction of this
# material is strictly forbidden unless prior written permission
# is obtained from Guardtime Inc.
# "Guardtime" and "KSI" are trademarks or registered trademarks of
# Guardtime Inc.
#

# Remove test output directories.  
rm -rf test/out/sign 2> /dev/null
rm -rf test/out/extend 2> /dev/null
rm -rf test/out/pubfile 2> /dev/null
rm -rf test/out/fname 2> /dev/null

# Create test output directories.
mkdir -p test/out/sign
mkdir -p test/out/extend
mkdir -p test/out/pubfile
mkdir -p test/out/fname

# Create some test files to output directory.
cp test/resource/file/testFile	test/out/fname/_
cp test/resource/file/testFile	test/out/fname/10__
cp test/resource/file/testFile	test/out/fname/test_file
cp test/resource/file/testFile	test/out/fname/a_23_500
cp test/resource/file/testFile	test/out/fname/a_23_1000
cp test/resource/file/testFile	test/out/fname/a_23_1000.ksig
cp test/resource/file/testFile	test/out/fname/a_23_1000_5.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig test/out/fname/ok-sig.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig test/out/fname/ok-sig

# Define KSI_CONF for temporary testing.
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
test/test_suites/file-name-gen.test \
--with=src/ksi -- -j1
exit_code=$?

exit $exit_code