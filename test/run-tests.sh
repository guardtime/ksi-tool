#!/bin/bash

dir=`dirname $0`
tmp=${TMPDIR:-'test/out'}
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
tlv_dir="${SCRIPT_DIR}/resource/tlv"
# Test Anything Protocol, from http://testanything.org/
. ${dir}/tap-functions
host=${1:-'localhost'}

echo \# Using $host as Guardtime Gateway Server. Specify custom server as 1st command-line argument.

url_s="http://172.20.20.7:3333"
url_x="http://172.20.20.7:8010/gt-extendingservice"
url_p="http://172.20.20.7/publications.tlv"
url_c="${tlv_dir}/mock.crt"

# input hash
SH1_HASH="da39a3ee5e6b4b0d3255bfef95601890afd80709"
SH256_HASH="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
RIPEMD160_HASH="9c1185a5c5e9fc54612808977ee8f548b2258d31"
SHA224_HASH="d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"
SHA384_HASH="38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"
SHA512_HASH="cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"

echo \# Running tests on `uname -n` at `date '+%F %T %Z'`

plan_tests 38 

diag "### Publications file download"
okx src/ksitool -p -t -o ${tmp}/pub.bin -X $url_x -V $url_c
diag "-------------------------------------------------------------------------------";

diag "### Verify Publications file using certificate"
okx src/ksitool -v -t -b ${tmp}/pub.bin -V $url_c
diag "-------------------------------------------------------------------------------";

diag "### Verify publications file using certificate email"
okx src/ksitool -v -t -b ${tmp}/pub.bin -E publications@guardtime.com
diag "-------------------------------------------------------------------------------";

diag "### Get Publications string"
okx src/ksitool -p -t -T 1410848909 -X $url_x # GMT: Tue, 21 Oct 2014 13:31:15 GMT 
diag "-------------------------------------------------------------------------------";

diag "### Sign"
okx src/ksitool -s -f ${SCRIPT_DIR}/testFile -o ${tmp}/tmp.ksig -S $url_s
diag "------------------------------------------------------------------------------";

diag "### Verify freshly created signature token"
okx src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/tmp.ksig -f ${SCRIPT_DIR}/testFile -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Extending signature"
okx src/ksitool -x -i ${tlv_dir}/ok-sig-2014-08-01.1.ksig -o ${tmp}/ext.ksig -X $url_x -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Verifying extended signature"
okx src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/ext.ksig -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Online verifying extended signature"
okx src/ksitool -v -x -i ${tmp}/ext.ksig -X $url_x -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Extend old signature [-T] "
okx src/ksitool -x -i ${tmp}/tmp.ksig -o ${tmp}/ext-t.ksig -X $url_x -T 1413120674 #  (GMT): Sun, 12 Oct 2014 13:31:14 GMT
diag "------------------------------------------------------------------------------";

diag "### Verify signature [-T] "
okx src/ksitool -v -x -b ${tmp}/pub.bin -i ${tmp}/ext-t.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -T 1413120674
diag "------------------------------------------------------------------------------";

diag "### Sign and verify raw hash using SHA 1"
okx src/ksitool -s -F SHA-1:${SH1_HASH} -o ${tmp}/sha1.ksig -S $url_s
okx src/ksitool -v -x -i ${tmp}/sha1.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Sign and verify raw hash using SHA 256"
okx src/ksitool -s -F SHA-256:${SH256_HASH} -o ${tmp}/sha256.ksig -S $url_s
okx src/ksitool -v -x -i ${tmp}/sha256.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Sign and verify raw hash using RIPEMD160"
okx src/ksitool -s -F RIPEMD-160:${RIPEMD160_HASH} -o ${tmp}/r160.ksig -S $url_s
okx src/ksitool -v -x -i ${tmp}/r160.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Sign and verify raw hash using SHA 224"
okx src/ksitool -s -F SHA-224:${SHA224_HASH} -o ${tmp}/sha224.ksig -S $url_s
okx src/ksitool -v -x -i ${tmp}/sha224.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Sign and verify raw hash using SHA 384"
okx src/ksitool -s -F SHA-384:${SHA384_HASH} -o ${tmp}/sha384.ksig -S $url_s
okx src/ksitool -v -x -i ${tmp}/sha384.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -V $url_c
diag "------------------------------------------------------------------------------";

diag "### Sign and verify raw hash using SHA 512"
okx src/ksitool -s -F SHA-512:${SHA512_HASH} -o ${tmp}/sha512.ksig -S $url_s
okx src/ksitool -v -x -i ${tmp}/sha512.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -V $url_c
diag "------------------------------------------------------------------------------";

diag "====== Testing errors ======";


like "`src/ksitool -x -i ${tmp}/tmp.ksig -o ${tmp}/ext.ksig -X $url_x -V $url_c 2>&1`" "There is no suitable publication yet." "Extending freshly created signature token"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${tmp}/tmp.ksig -o ${tmp}/ext.ksig -X $url_x -V $url_c 2>&1`" "There is no suitable publication yet" "Error extend no suitable publication"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${SCRIPT_DIR}/testFile -o ${tmp}/ext.ksig -X $url_x -V $url_c 2>&1`" "Unable to read signature from file." "Error extend not suitable format"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -x -b ${tmp}/pub.bin -i ${tmp}/ext-t.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -T 1413120674 2>&1`" "nope" "Error extend before calendar [-T]"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -x -b ${tmp}/pub.bin -i ${tmp}/ext-t.ksig -f ${SCRIPT_DIR}/testFile -X $url_x -T $(date +%s) 2>&1`" "nope" "Error extend to the future [-T]"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -x -i ${SCRIPT_DIR}/testFile -o ${tmp}/ext.ksig -X $url_x -V $url_c 2>&1`" "Unable to read signature from file." "Error verify not suitable format"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -x -i ${tmp}/sha1.ksig -f ${SCRIPT_DIR}/resource/TestData.txt -X $url_x -V $url_c 2>&1`" "Wrong document or signature" "Error verifying signature and wrong file"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -F SHA-1:${SH1_HASH}FF -o ${tmp}/sha1.ksig -S $url_s 2>&1`" "Hash length is incorrect" "Error signing with SH1 and wrong hash"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -F DUNNO:${SH1_HASH} -o ${tmp}/sha1.ksig -S $url_s 2>&1`" "Algorithm name is incorrect" "Error signing with unknown algorithm and wrong hash"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -s -f ${SCRIPT_DIR}/testFile -o ${tmp}/tmp.ksig -S no_network_here 2>&1`" "no_network_here" "Error bad network provider"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -b ${tmp}/pub.bin -i ${tmp}/ext-t.ksig -f missing_file -X $url_x  2>&1`" "File does not exist" "Error Verify signature and missing file"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -t -b ${tmp}/pub.bin -V no_certificate 2>&1`" "File does not exist" "Error missing cert files"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -v -t -b ${SCRIPT_DIR}/testFile -V $url_c 2>&1`" "Unable to read publications file" "Error Invalid publications file"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -p -S $url_s -X $url_x -P $url_p -T 969085709 2>&1`" "The requested extention time is older than available DB" "Error Unable to Get Publication string"
diag "------------------------------------------------------------------------------";

like "`src/ksitool -p -t -S $url_s -X $url_x -P $url_p -o $tmp/tpub.bin -E wrong@email.mail 2>&1`" "Wrong subject name. The PKI certificate is not trusted." "Error wrong E-mail"
diag "------------------------------------------------------------------------------";


# cleanup
# rm -f ${tmp}/pub.bin ${tmp}/tmp.ksig ${tmp}/ext.ksig ${tmp}/r160.ksig ${tmp}/sha1.ksig ${tmp}/sha256.ksig ${tmp}/sha224.ksig ${tmp}/sha384.ksig ${tmp}/sha512.ksig 2> /dev/null

