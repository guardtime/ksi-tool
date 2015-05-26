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
DATE=`date +%s`

# Test Anything Protocol, from http://testanything.org/
. ${dir}/tap-functions
host=${1:-'localhost'}

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

echo \# Running tests on `uname -n` at `date '+%F %T %Z'`, start time: ${DATE}

plan_tests 62


diag "######    Publications file download"
okx src/ksitool -p -t -o ${tmp}/pub.bin -V $url_c --log ${tmp}/out.log
like "`[ -f ${tmp}/out.log ] && echo 'Log file exists' || echo 'Log file does not exist' 2>&1`" "Log file exists"

diag "######    Verify Publications file using certificate"
okx src/ksitool -v -t -b ${tmp}/pub.bin -V $url_c

diag "######    Verify publications file using certificate email"
okx src/ksitool -v -t -b ${tmp}/pub.bin -E publications@guardtime.com

diag "######    Get Publications string"
okx src/ksitool -p -t -T 1410848909  # GMT: Tue, 21 Oct 2014 13:31:15 GMT 
diag "-------------------------------------------------------------------------------";

diag "######    Sign and save log file"
okx src/ksitool -s -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig  --log ${tmp}/out.log
like "`[ -f ${tmp}/out.log ] && echo 'Log file exists' || echo 'Log file does not exist' 2>&1`" "Log file exists"
diag "------------------------------------------------------------------------------";


diag "######    Sign and verify using algorithm: SHA1"
okx src/ksitool -s -H SHA1 -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig 
okx src/ksitool -v -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -V $url_c -T ${DATE}
okx src/ksitool -v -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -b ${tmp}/pub.bin -V $url_c

diag "######    Sign and verify using algorithm: SHA2-256"
okx src/ksitool -s -H SHA2-256 -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig 
okx src/ksitool -v -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Sign and verify using algorithm: RIPEMD-160"
okx src/ksitool -s -H RIPEMD-160 -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig 
okx src/ksitool -v -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Sign and verify using algorithm: SHA2-224"
okx src/ksitool -s -H SHA2-224 -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig 
okx src/ksitool -v -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Sign and verify using algorithm: SHA2-384"
okx src/ksitool -s -H SHA2-384 -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig 
okx src/ksitool -v -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Sign and verify using algorithm: SHA2-512"
okx src/ksitool -s -H SHA2-512 -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig 
okx src/ksitool -v -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -V $url_c
diag "------------------------------------------------------------------------------";


diag "######    Sign and verify raw hash using SHA-1"
okx src/ksitool -s -F SHA-1:${SH1_HASH} -o ${tmp}/sha1.ksig 
okx src/ksitool -v -i ${tmp}/sha1.ksig -f ${resource_dir}/testFile -b ${tmp}/pub.bin -V $url_c

diag "######    Sign and verify raw hash using SHA-256"
okx src/ksitool -s -F SHA-256:${SH256_HASH} -o ${tmp}/sha256.ksig 
okx src/ksitool -v -i ${tmp}/sha256.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Sign and verify raw hash using RIPEMD-160"
okx src/ksitool -s -F RIPEMD-160:${RIPEMD160_HASH} -o ${tmp}/r160.ksig 
okx src/ksitool -v -i ${tmp}/r160.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Sign and verify raw hash using SHA-224"
okx src/ksitool -s -F SHA-224:${SHA224_HASH} -o ${tmp}/sha224.ksig 
okx src/ksitool -v -i ${tmp}/sha224.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Sign and verify raw hash using SHA2-384"
okx src/ksitool -s -F SHA-384:${SHA384_HASH} -o ${tmp}/sha384.ksig 
okx src/ksitool -v -i ${tmp}/sha384.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Sign and verify raw hash using SHA-512"
okx src/ksitool -s -F SHA-512:${SHA512_HASH} -o ${tmp}/sha512.ksig 
okx src/ksitool -v -i ${tmp}/sha512.ksig -f ${resource_dir}/testFile -V $url_c
diag "------------------------------------------------------------------------------";


diag "######    Extend signature"
okx src/ksitool -x -i ${resource_dir}/ok-sig-2014-08-01.1.ksig -o ${tmp}/ext.ksig -V $url_c --log ${tmp}/out.log
like "`[ -f ${tmp}/out.log ] && echo 'Log file exists' || echo 'Log file does not exist' 2>&1`" "Log file exists"

diag "######    Extend old signature before publication time [-T] "
okx src/ksitool -x -i ${resource_dir}/ok-sig-2014-08-01.1.ksig -o ${tmp}/ext-t.ksig -T 1413120674 #  (GMT): Sun, 12 Oct 2014 13:31:14 GMT

diag "######    Extend old signature to publication time [-T] "
okx src/ksitool -x -i ${resource_dir}/ok-sig-2014-08-01.1.ksig -o ${tmp}/ext-t.ksig -T 1421280000 #  (GMT): Thu, 15 Jan 2015 00:00:00 GMT

diag "######    Extend old signature to nearest publication "
okx src/ksitool -x -i ${resource_dir}/ok-sig-2014-08-01.1.ksig -o ${tmp}/ext-t.ksig
diag "------------------------------------------------------------------------------";


diag "######    Verify freshly created signature token using publication file"
okx src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile -V $url_c

diag "######    Verifying extended signature using publication file"
okx src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/ext.ksig -V $url_c

diag "######    Verify extended signature online"
okx src/ksitool -v -x -i ${tmp}/ext.ksig -V $url_c --log ${tmp}/out.log
like "`[ -f ${tmp}/out.log ] && echo 'Log file exists' || echo 'Log file does not exist' 2>&1`" "Log file exists"

diag "######    Verify extended signature with pubstring. Bubstr points to exact publication."
okx src/ksitool -v -i ${tmp}/ext.ksig -V $url_c --ref AAAAAA-CT5VGY-AAPUCF-L3EKCC-NRSX56-AXIDFL-VZJQK4-WDCPOE-3KIWGB-XGPPM3-O5BIMW-REOVR4

diag "######    Verify extended signature with pubstring. Bubstr points to newer publication."
okx src/ksitool -v -i ${tmp}/ext.ksig -V $url_c --ref AAAAAA-CUM2LY-AANFLH-HIQ7FW-TGTOAL-VDVGBO-AN3SQO-5FOO7L-CYGQG6-FNMOBG-QGSTAG-K3VY6C

diag "######    Verify extended signature with pubstring. Bubstr points between publications."
okx src/ksitool -v -i ${tmp}/ext.ksig -V $url_c --ref AAAAAA-CUBJQL-AAKVFD-VNJIK5-7DTJ6T-YYCOGP-N7J3RT-CRE5DU-WBB6AE-LANHHH-3CFEM4-7FM65J

diag "######    Verify signature with pubstring. Bubstr points to exact publication."
okx src/ksitool -v -i ${resource_dir}/ok-sig-2014-08-01.1.ksig -V $url_c --ref AAAAAA-CT5VGY-AAPUCF-L3EKCC-NRSX56-AXIDFL-VZJQK4-WDCPOE-3KIWGB-XGPPM3-O5BIMW-REOVR4


diag "------------------------------------------------------------------------------";

#TODO: uncomment if implemeneted
# diag "######    Use aggregator"
# okx src/ksitool -nt --aggre --setsystime 
# okx src/ksitool -nt --aggre --htime 
# diag "------------------------------------------------------------------------------";



diag "====== Testing errors ======";


like "`src/ksitool -x -i ${tmp}/tmp.ksig -o ${tmp}/ext.ksig -V $url_c 2>&1`" "There is no suitable publication yet." "Extending freshly created signature token"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${tmp}/tmp.ksig -o ${tmp}/ext.ksig -V $url_c 2>&1`" "There is no suitable publication yet" "Error extend no suitable publication"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${resource_dir}/testFile -o ${tmp}/ext.ksig -V $url_c 2>&1`" "Unable to read signature from file." "Error extend not suitable format"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${resource_dir}/ok-sig-2014-08-01.1.ksig -o ${tmp}/ext-t.ksig -T 1311120674 2>&1`" "Aggregation time may not be greater than the publication time" "Error extend before calendar [-T]"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${resource_dir}/ok-sig-2014-08-01.1.ksig -o ${tmp}/ext-t.ksig -T 1458914880 2>&1`" "The request asked for hash values newer than the current real time" "Error extend to the future [-T]"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${resource_dir}/testFile -o ${tmp}/ext.ksig -V $url_c 2>&1`" "Unable to read signature from file." "Error verify not suitable format"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -x -i ${tmp}/sha1.ksig -f ${SCRIPT_DIR}/resource/TestData.txt -V $url_c 2>&1`" "Wrong document or signature" "Error verifying signature and wrong file"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -F SHA-1:${SH1_HASH}FF -o ${tmp}/sha1.ksig  2>&1`" "Hash length is incorrect" "Error signing with SH1 and wrong hash"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -F DUNNO:${SH1_HASH} -o ${tmp}/sha1.ksig  2>&1`" "Algorithm name is incorrect" "Error signing with unknown algorithm and wrong hash"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig -S no_network_here 2>&1`" "no_network_here" "Error bad network provider"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/ext-t.ksig -f missing_file  2>&1`" "File does not exist" "Error Verify signature and missing file"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -t -b ${tmp}/pub.bin -V no_certificate 2>&1`" "File does not exist" "Error missing cert files"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -t -b ${resource_dir}/testFile -V $url_c 2>&1`" "Unable to read publications file" "Error Invalid publications file"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -p -T 969085709 2>&1`" "The request asked for hash values older than the oldest round" "Error Unable to Get Publication string"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -p -t -o $tmp/tpub.bin -E wrong@email.mail 2>&1`" "Wrong subject name" "Error wrong E-mail"
diag "------------------------------------------------------------------------------";


# cleanup
rm -f ${tmp}/out.log ${tmp}/ext-t.ksig ${tmp}/pub.bin ${tmp}/tmp.ksig ${tmp}/ext.ksig ${tmp}/r160.ksig ${tmp}/sha1.ksig ${tmp}/sha256.ksig ${tmp}/sha224.ksig ${tmp}/sha384.ksig ${tmp}/sha512.ksig 2> /dev/null

