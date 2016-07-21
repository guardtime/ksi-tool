#!/bin/bash

#
# Copyright 2013-2016 Guardtime, Inc.
#
# This file is part of the Guardtime client SDK.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
# "Guardtime" and "KSI" are trademarks or registered trademarks of
# Guardtime, Inc., and no license to trademarks is granted; Guardtime
# reserves and retains all trademark rights.

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

# If ksi tool in project directory is available use that one, if not
# use the one installed in the machine.
if [ ! -f src/ksi ]; then
	tool=ksi
else
	tool=src/ksi
fi

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
--with=$tool -- -j1
exit_code=$?

exit $exit_code