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

mem_test_dir=test/out/memory
test_suite_dir=test/test_suites

# Remove memory test directory.
rm -rf $mem_test_dir 2> /dev/null

# Create a temporary output directory for memory tests.
mkdir -p $mem_test_dir

# Create some test files to output directory.
cp test/resource/file/testFile	$mem_test_dir/_
cp test/resource/file/testFile	$mem_test_dir/10__
cp test/resource/file/testFile	$mem_test_dir/test_file
cp test/resource/file/testFile	$mem_test_dir/a_23_500
cp test/resource/file/testFile	$mem_test_dir/a_23_1000
cp test/resource/file/testFile	$mem_test_dir/a_23_1000.ksig
cp test/resource/file/testFile	$mem_test_dir/a_23_1000_5.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig $mem_test_dir/ok-sig.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig $mem_test_dir/ok-sig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig $mem_test_dir/mass-extend-1.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig $mem_test_dir/mass-extend-2.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig $mem_test_dir/mass-extend-2.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig $mem_test_dir/not-extended-1A.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig $mem_test_dir/not-extended-1B.ksig
cp test/resource/signature/ok-sig-2014-08-01.1.ksig $mem_test_dir/not-extended-2B.ksig

# Configure temporary KSI_CONF.
export KSI_CONF=test/resource/conf/default-conf.cfg

# A function to convert a test file to memory test.
function generate_test() {
test/convert-to-memory-test.sh $test_suite_dir/$1  $mem_test_dir/$1
}

# Convert test files to valgrind memory test files.
generate_test sign.test
generate_test static-sign.test
generate_test sign-verify.test
generate_test extend.test
generate_test mass-extend.test
generate_test extend-verify.test
generate_test static-verify.test
generate_test static-sign-verify.test
generate_test static-extend.test
generate_test sign-cmd.test
generate_test signature-dump.test
generate_test extend-cmd.test
generate_test static-verify-invalid-signatures.test
generate_test pubfile.test
generate_test static-pubfile.test
generate_test verify-invalid-pubfile.test
generate_test verify-cmd.test
generate_test default-conf.test
generate_test invalid-conf.test
generate_test file-name-gen.test
generate_test sign-block-signer.test
generate_test sign-block-signer-cmd.test
generate_test verify-pub-suggestions.test

# Run generated test scripts.

# If ksi tool in project directory is available use that one, if not
# use the one installed in the machine.
if [ ! -f src/ksi ]; then
	tool=ksi
else
	tool=src/ksi
fi

shelltest \
$mem_test_dir/sign.test \
$mem_test_dir/static-sign.test \
$mem_test_dir/sign-verify.test \
$mem_test_dir/extend.test \
$mem_test_dir/mass-extend.test \
$mem_test_dir/extend-verify.test \
$mem_test_dir/static-verify.test \
$mem_test_dir/static-sign-verify.test \
$mem_test_dir/static-extend.test \
$mem_test_dir/sign-cmd.test \
$mem_test_dir/signature-dump.test \
$mem_test_dir/extend-cmd.test \
$mem_test_dir/static-verify-invalid-signatures.test \
$mem_test_dir/pubfile.test \
$mem_test_dir/static-pubfile.test \
$mem_test_dir/verify-invalid-pubfile.test \
$mem_test_dir/verify-cmd.test \
$mem_test_dir/default-conf.test \
$mem_test_dir/invalid-conf.test \
$mem_test_dir/file-name-gen.test \
$mem_test_dir/sign-block-signer.test \
$mem_test_dir/sign-block-signer-cmd.test \
$mem_test_dir/verify-pub-suggestions.test \
--with="valgrind --leak-check=full --fair-sched=yes $tool" -- -j1
exit_code=$?

exit $exit_code