#!/bin/bash

#
# GUARDTIME CONFIDENTIAL
#
# Copyright (C) [2015] Guardtime, Inc
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

dir=`dirname $0`
tmp=${TMPDIR:-'test/out'}
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
resource_dir="${SCRIPT_DIR}/resource"

# Test Anything Protocol, from http://testanything.org/
. ${dir}/tap-functions
host=${1:-'localhost'}

echo \# =============================================================================
echo \#                 KSI Tool Test
echo \# =============================================================================


echo \# Using $host as Guardtime Gateway Server. Specify custom server as 1st command-line argument.

. ${dir}/endpoints.sh
url_c="${resource_dir}/mock.crt"

echo \# Certificate: $url_c

# input hash
SH1_HASH="da39a3ee5e6b4b0d3255bfef95601890afd80709"
SH256_HASH="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
RIPEMD160_HASH="9c1185a5c5e9fc54612808977ee8f548b2258d31"
SHA224_HASH="d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"
SHA384_HASH="38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"
SHA512_HASH="cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"

echo \# Running tests on `uname -n` at `date '+%F %T %Z'`

plan_tests 18

diag "### Publications file download";
okx src/ksitool -p -t -o ${tmp}/pub.bin -V $url_c
diag "-------------------------------------------------------------------------------";

diag "### Verify Publications file using certificate";
okx src/ksitool -v -t -b ${tmp}/pub.bin -V $url_c
diag "-------------------------------------------------------------------------------";

diag "### Sign";
okx src/ksitool -s -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig
diag "------------------------------------------------------------------------------";

diag "### Verify freshly created signature token";
okx src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -V $url_c
diag "------------------------------------------------------------------------------";

diag "###    Sign and verify raw hash using SHA-1"
okx src/ksitool -s -F SHA-1:${SH1_HASH} -o ${tmp}/sha1.ksig
diag "------------------------------------------------------------------------------";

diag "### Extending signature";
okx src/ksitool -x -i ${resource_dir}/ok-sig-2014-08-01.1.ksig -o ${tmp}/ext.ksig -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Verifying extended signature";
okx src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/ext.ksig -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Online verifying extended signature";
okx src/ksitool -v -x -i ${tmp}/ext.ksig -V $url_c
diag "------------------------------------------------------------------------------";


diag "====== Testing errors ======";


like "`src/ksitool -x -i ${tmp}/tmp.ksig -o ${tmp}/ext.ksig -V $url_c 2>&1`" "Error: There is no suitable publication yet." "Extending freshly created signature token"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${tmp}/tmp.ksig -o ${tmp}/ext.ksig -V $url_c 2>&1`" "Error: There is no suitable publication yet." "Error extend no suitable publication"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${resource_dir}/testFile -o ${tmp}/ext.ksig -V $url_c 2>&1`" "Error: Unable to load signature file from" "Error extend not suitable format"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${resource_dir}/testFile -o ${tmp}/ext.ksig -V $url_c 2>&1`" "Error: Unable to load signature file from" "Error verify not suitable format"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -x -i ${resource_dir}/ok-sig-2014-04-30.1.ksig -f ${SCRIPT_DIR}/resource/TestData.txt -V $url_c 2>&1`" " Error: Unable to verify file hash." "Error verifying signature and wrong file"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -F SHA-1:${SH1_HASH}FF -o ${tmp}/sha1.ksig 2>&1`" "Hash length is incorrect" "Error signing with SH1 and wrong hash"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -F DUNNO:${SH1_HASH} -o ${tmp}/sha1.ksig 2>&1`" "Algorithm name is incorrect" "Error signing with unknown algorithm and wrong hash"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig -S no_network_here 2>&1`" "Error: Unable to create signature" "Error bad network provider"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/ext-t.ksig -f missing_file  2>&1`" "File does not exist" "Error Verify signature and missing file"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -t -b ${resource_dir}/testFile -V $url_c 2>&1`" "Error: Unable to load publication file from" "Error Invalid publications file"
diag "------------------------------------------------------------------------------";


# cleanup
# rm -f ${tmp}/pub.bin ${tmp}/tmp.ksig ${tmp}/ext.ksig ${tmp}/r160.ksig ${tmp}/sha1.ksig ${tmp}/sha256.ksig ${tmp}/sha224.ksig ${tmp}/sha384.ksig ${tmp}/sha512.ksig 2> /dev/null

