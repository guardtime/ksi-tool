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
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
tmp=${TMPDIR:=$SCRIPT_DIR/out}
resource_dir="${SCRIPT_DIR}/resource"
DATE=`date +%s`

mkdir -p $tmp

exec="src/ksitool"

if [ ! -f src/ksitool ]; then
	exec="ksitool"
fi

# Test Anything Protocol, from http://testanything.org/
. ${dir}/tap-functions
host=${1:-'localhost'}

echo \# Using $host as Guardtime Gateway Server. Specify custom server as 1st command-line argument.


. ${dir}/endpoints.sh
url_c="${resource_dir}/mock.crt"

echo \# Certificate: $url_c

# input hash
SH1_HASH="a7d2c6238a92878b2a578c2477e8a33f9d8591ab"
SH256_HASH="11a700b0c8066c47ecba05ed37bc14dcadb238552d86c659342d1d7e87b8772d"
RIPEMD160_HASH="e889d4393145736ebdfb3702dd3180f5c6cffade"
SHA224_HASH="9b7ea5330761e8b50b36af0d61c10bc227c908ee57a545d40131cfa3"
SHA384_HASH="a5ac3bb2fa156480d1cf437c54481d9c77a145b682879e92e30a8b79f0a45a001be7969ffa02d81af0610b784ae72f4f"
SHA512_HASH="09e3fc9d3669eaf53d3afeb60e6a73af2c7c7b01a0fe49127253e0d466ba3d1c85ed541593775a12a880378335eeda5fc0ad5700920e11ed315f4b49f37c6d26"

TESTSIG="ok-sig-2014-08-01.1.ksig"

echo \# Running tests on `uname -n` at `date '+%F %T %Z'`, start time: ${DATE}

plan_tests 101


diag "######    Publications file download"
okx $exec -p -t -o ${tmp}/pub.bin -V $url_c --log ${tmp}/out.log
like "`[ -f ${tmp}/out.log ] && echo 'Log file exists' || echo 'Log file does not exist' 2>&1`" "Log file exists"

diag "######    Verify Publications file"
okx $exec -v -t -b ${tmp}/pub.bin

diag "######    Verify Publications file using certificate"
okx $exec -v -t -b ${resource_dir}/publications.tlv -V $url_c

like "`$exec -v -t -b ${resource_dir}/publications.tlv 2>&1`" "The PKI certificate is not trusted." "Publications file certificate not in truststore"
diag "------------------------------------------------------------------------------";


diag "######    Verify publications file using certificate email"
okx $exec -v -t -b ${tmp}/pub.bin -E publications@guardtime.com

diag "######    Get Publications string"
okx $exec -p -t -T 1410848909  # GMT: Tue, 21 Oct 2014 13:31:15 GMT
diag "-------------------------------------------------------------------------------";

diag "######    Get Publications string from date string"
like "`$exec -p  -T \"2015-10-15 00:00:00\"  2>&1`" "AAAAAA-CWD3WI-AAN5NF-5TTMUX-ZZ2OTP-REOQ2Q-WRNPQ7-IT7AYI-FML6EG-IAR3LT-LXM37S-23I5MA" "Get Publication string from date string."
like "`$exec -p  -T \"2012-2-29 00:00:00\"  2>&1`" "AAAAAA-CPJVVI-AAICRV-UGJPHG-2SGNWH-WKQQS5-ZZTZWZ-K7OO2P-TW5VOY-FVRA5N-UULCBC-7JQYZE" "Its a leap year!"

diag "######    Sign data over TCP"
okx $exec -s -f ${resource_dir}/testFile -o ${tmp}/data_over_tcp.ksig -S ${KSI_TCP_AGGREGATOR} ${KSI_TCP_LOGIN}

diag "######    Sign raw hash using SHA-256 over TCP"
okx $exec -s -F SHA-256:${SH256_HASH} -o ${tmp}/h_sha256.ksig -S ${KSI_TCP_AGGREGATOR} ${KSI_TCP_LOGIN}

diag "######    Sign raw hash using RIPEMD-160 over TCP"
okx $exec -s -F RIPEMD-160:${RIPEMD160_HASH} -o ${tmp}/h_r160.ksig -S ${KSI_TCP_AGGREGATOR} ${KSI_TCP_LOGIN}

diag "######    Sign raw hash using SHA-224 over TCP"
okx $exec -s -F SHA-224:${SHA224_HASH} -o ${tmp}/h_sha224.ksig -S ${KSI_TCP_AGGREGATOR} ${KSI_TCP_LOGIN}

diag "######    Sign raw hash using SHA2-384 over TCP"
okx $exec -s -F SHA-384:${SHA384_HASH} -o ${tmp}/h_sha384.ksig -S ${KSI_TCP_AGGREGATOR} ${KSI_TCP_LOGIN}

diag "######    Sign raw hash using SHA-512 over TCP"
okx $exec -s -F SHA-512:${SHA512_HASH} -o ${tmp}/h_sha512.ksig -S ${KSI_TCP_AGGREGATOR} ${KSI_TCP_LOGIN}
diag "------------------------------------------------------------------------------";


diag "######    Sign and save log file"
okx $exec -s -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig --log ${tmp}/out.log
like "`[ -f ${tmp}/out.log ] && echo 'Log file exists' || echo 'Log file does not exist' 2>&1`" "Log file exists"
diag "------------------------------------------------------------------------------";


diag "######    Sign and verify using algorithm: SHA1"
okx $exec -s -H SHA1 -f ${resource_dir}/testFile -o ${tmp}/f_sha1.ksig
okx $exec -v -i ${tmp}/f_sha1.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify using algorithm: SHA2-256"
okx $exec -s -H SHA2-256 -f ${resource_dir}/testFile -o ${tmp}/f_sha256.ksig
okx $exec -v -i ${tmp}/f_sha256.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify using algorithm: RIPEMD-160"
okx $exec -s -H RIPEMD-160 -f ${resource_dir}/testFile -o ${tmp}/f_ripemd160.ksig
okx $exec -v -i ${tmp}/f_ripemd160.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify using algorithm: SHA2-224"
okx $exec -s -H SHA2-224 -f ${resource_dir}/testFile -o ${tmp}/f_sha224.ksig
okx $exec -v -i ${tmp}/f_sha224.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify using algorithm: SHA2-384"
okx $exec -s -H SHA2-384 -f ${resource_dir}/testFile -o ${tmp}/f_sha384.ksig
okx $exec -v -i ${tmp}/f_sha384.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify using algorithm: SHA2-512"
okx $exec -s -H SHA2-512 -f ${resource_dir}/testFile -o ${tmp}/f_sha512.ksig
okx $exec -v -i ${tmp}/f_sha512.ksig -f ${resource_dir}/testFile
diag "------------------------------------------------------------------------------";


diag "######    Sign and verify raw hash using SHA-1"
okx $exec -s -F SHA-1:${SH1_HASH} -o ${tmp}/h_sha1.ksig
okx $exec -v -i ${tmp}/h_sha1.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify raw hash using SHA-256"
okx $exec -s -F SHA-256:${SH256_HASH} -o ${tmp}/h_sha256.ksig
okx $exec -v -i ${tmp}/h_sha256.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify raw hash using RIPEMD-160"
okx $exec -s -F RIPEMD-160:${RIPEMD160_HASH} -o ${tmp}/h_r160.ksig
okx $exec -v -i ${tmp}/h_r160.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify raw hash using SHA-224"
okx $exec -s -F SHA-224:${SHA224_HASH} -o ${tmp}/h_sha224.ksig
okx $exec -v -i ${tmp}/h_sha224.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify raw hash using SHA2-384"
okx $exec -s -F SHA-384:${SHA384_HASH} -o ${tmp}/h_sha384.ksig
okx $exec -v -i ${tmp}/h_sha384.ksig -f ${resource_dir}/testFile

diag "######    Sign and verify raw hash using SHA-512"
okx $exec -s -F SHA-512:${SHA512_HASH} -o ${tmp}/h_sha512.ksig
okx $exec -v -i ${tmp}/h_sha512.ksig -f ${resource_dir}/testFile
diag "------------------------------------------------------------------------------";


diag "######    Extend signature"
okx $exec -x -i ${resource_dir}/$TESTSIG -o ${tmp}/ext.ksig --log ${tmp}/out.log
like "`[ -f ${tmp}/out.log ] && echo 'Log file exists' || echo 'Log file does not exist' 2>&1`" "Log file exists"

diag "######    Extend old signature before publication time [-T] "
okx $exec -x -i ${resource_dir}/$TESTSIG -o ${tmp}/ext-t.ksig -T 1413120674 #  (GMT): Sun, 12 Oct 2014 13:31:14 GMT

diag "######    Extend old signature to publication time [-T] "
okx $exec -x -i ${resource_dir}/$TESTSIG -o ${tmp}/ext-t.ksig -T 1421280000 #  (GMT): Thu, 15 Jan 2015 00:00:00 GMT

diag "######    Extend old signature to nearest publication "
okx $exec -x -i ${resource_dir}/$TESTSIG -o ${tmp}/ext-t.ksig
diag "------------------------------------------------------------------------------";


diag "######    Verify freshly created signature token using publication file"
okx $exec -v -b ${tmp}/pub.bin -i ${tmp}/tmp.ksig -f ${resource_dir}/testFile

diag "######    Verifying extended signature using publication file"
okx $exec -v -b ${tmp}/pub.bin -i ${tmp}/ext.ksig

diag "######    Verify extended signature online"
okx $exec -v -x -i ${tmp}/ext.ksig

diag "######    Verify extended signature with pubstring. Bubstr points to exact publication."
okx $exec -v -i ${tmp}/ext.ksig --ref AAAAAA-CT5VGY-AAPUCF-L3EKCC-NRSX56-AXIDFL-VZJQK4-WDCPOE-3KIWGB-XGPPM3-O5BIMW-REOVR4

diag "######    Verify extended signature with pubstring. Bubstr points to newer publication."
okx $exec -v -i ${tmp}/ext.ksig --ref AAAAAA-CUM2LY-AANFLH-HIQ7FW-TGTOAL-VDVGBO-AN3SQO-5FOO7L-CYGQG6-FNMOBG-QGSTAG-K3VY6C

diag "######    Verify extended signature with pubstring. Bubstr points between publications."
okx $exec -v -i ${tmp}/ext.ksig --ref AAAAAA-CUBJQL-AAKVFD-VNJIK5-7DTJ6T-YYCOGP-N7J3RT-CRE5DU-WBB6AE-LANHHH-3CFEM4-7FM65J

diag "######    Verify signature with pubstring. Bubstr points to exact publication."
okx $exec -v -i ${resource_dir}/$TESTSIG --ref AAAAAA-CT5VGY-AAPUCF-L3EKCC-NRSX56-AXIDFL-VZJQK4-WDCPOE-3KIWGB-XGPPM3-O5BIMW-REOVR4

diag "######    Verify raw hash using SHA-1"
okx $exec -v -i ${resource_dir}/sha1.ksig -f ${resource_dir}/testFile

diag "######    Verify raw hash using SHA-224"
okx $exec -v -i ${resource_dir}/sha224.ksig -f ${resource_dir}/testFile

diag "######    Verify raw hash using SHA-256"
okx $exec -v -i ${resource_dir}/sha256.ksig -f ${resource_dir}/testFile

diag "######    Verify raw hash using SHA-384"
okx $exec -v -i ${resource_dir}/sha384.ksig -f ${resource_dir}/testFile

diag "######    Verify raw hash using SHA-512"
okx $exec -v -i ${resource_dir}/sha512.ksig -f ${resource_dir}/testFile

diag "######    Verify raw hash using RIPEMD-160"
okx $exec -v -i ${resource_dir}/ripemd160.ksig -f ${resource_dir}/testFile

diag "######    Verify Publications file with constraints"
$exec -v -b ${resource_dir}/publications.tlv -V ${resource_dir}/mock.crt --cnstr "2.5.4.10=Guardtime AS"
okx [ "$?" == "0" ]

diag "------------------------------------------------------------------------------";

#TODO: uncomment if implemeneted
# diag "######    Use aggregator"
# okx $exec -nt --aggre --setsystime
# okx $exec -nt --aggre --htime
# diag "------------------------------------------------------------------------------";

diag "######    Testing stdout and stdin"
echo "TEST" > ${tmp}/pipeFileClone
like "`echo \"TEST\" | $exec -s -f - -o - | $exec -v -i - -f ${tmp}/pipeFileClone`" "Verification of signature - successful"
like "`$exec -p -o - | $exec -v -b -`" "Verification of publication file - successful."
diag "------------------------------------------------------------------------------";



diag "====== Testing errors ======";


like "`$exec -x -i ${tmp}/tmp.ksig -o ${tmp}/tmp_n.ksig 2>&1`" "Error: There is no suitable publication yet." "Extending freshly created signature token"
diag "------------------------------------------------------------------------------";

like "`$exec -x -i ${tmp}/tmp.ksig -o ${tmp}/tmp_n.ksig 2>&1`" "Error: There is no suitable publication yet." "Error extend no suitable publication"
diag "------------------------------------------------------------------------------";

like "`$exec -x -i ${resource_dir}/testFile -o ${tmp}/tmp_n.ksig 2>&1`" "Error: Unable to load signature file from" "Error extend not suitable format"
diag "------------------------------------------------------------------------------";

like "`$exec -x -i ${resource_dir}/$TESTSIG -o ${tmp}/tmp_n.ksig -T 1311120674 2>&1`" "Error: Unable to extend signature" "Error extend before calendar [-T]"
diag "------------------------------------------------------------------------------";

like "`$exec -x -i ${resource_dir}/$TESTSIG -o ${tmp}/tmp_n.ksig -T $(date --date="+1 year" +"%s") 2>&1`" "Error: The request asked for hash values newer than the current real time" "Error extend to the future [-T]"
diag "------------------------------------------------------------------------------";

like "`$exec -x -i ${resource_dir}/testFile -o ${tmp}/tmp_n.ksig 2>&1`" "Error: Unable to load signature file from" "Error verify not suitable format"
diag "------------------------------------------------------------------------------";

like "`$exec -v -i ${tmp}/tmp.ksig -f ${SCRIPT_DIR}/resource/TestData.txt 2>&1`" "Error: Unable to verify files hash." "Error verifying signature and wrong file"
diag "------------------------------------------------------------------------------";

like "`$exec -s -F SHA-1:${SH1_HASH}FF -o ${tmp}/tmp_n.ksig  2>&1`" "Hash length is incorrect" "Error signing with SH1 and wrong hash"
diag "------------------------------------------------------------------------------";

like "`$exec -s -F DUNNO:${SH1_HASH} -o ${tmp}/tmp_n.ksig  2>&1`" "Algorithm name is incorrect" "Error signing with unknown algorithm and wrong hash"
diag "------------------------------------------------------------------------------";

like "`$exec -s -f ${resource_dir}/testFile -o ${tmp}/tmp_n.ksig -S no_network_here 2>&1`" "Error: Unable to create signature." "Error bad network provider"
diag "------------------------------------------------------------------------------";

like "`$exec -v -b ${tmp}/pub.bin -i ${tmp}/ext-t.ksig -f missing_file  2>&1`" "File does not exist" "Error Verify signature and missing file"
diag "------------------------------------------------------------------------------";

like "`$exec -v -t -b ${tmp}/pub.bin -V no_certificate 2>&1`" "File does not exist" "Error missing cert files"
diag "------------------------------------------------------------------------------";

like "`$exec -v -t -b ${resource_dir}/testFile 2>&1`" "Error: Unable to load publication file" "Error Invalid publications file"
diag "------------------------------------------------------------------------------";

like "`$exec -p -T 969085709 2>&1`" "The request asked for hash values older than the oldest round" "Error Unable to Get Publication string"
diag "------------------------------------------------------------------------------";

like "`$exec -p -t -o $tmp/tpub.bin -E wrong@email.mail 2>&1`" "Error: Unexpected OID value for PKI Certificate constraint." "Error wrong E-mail"
diag "------------------------------------------------------------------------------";

like "`$exec -s -F SHA-1:${SH1_HASH} -o ${tmp}/tmp_n.ksig --user nouserpresent --pass asd 2>&1`" "The request could not be authenticated" "Error Wrong user specified"
diag "------------------------------------------------------------------------------";

like "`$exec -s -F SHA-1:${SH1_HASH} -o ${tmp}/tmp_n.ksig --user anon --pass asd 2>&1`" "The request could not be authenticated" "Error Wrong pass specified"
diag "------------------------------------------------------------------------------";

like "`$exec -s -F SHA-1:${SH1_HASH} -o ${tmp}/tmp_n.ksig --user  --pass anon 2>&1`" "Parameter must have value" "Error No user specified"
diag "------------------------------------------------------------------------------";

like "`$exec -s -F SHA-1:${SH1_HASH} -o ${tmp}/tmp_n.ksig --user anon --pass  2>&1`" "Parameter must have value" "Error No pass specified"
diag "------------------------------------------------------------------------------";

like "`$exec -v -b ${resource_dir}/publications.tlv -V ${resource_dir}/mock.crt --cnstr 2.5.4.10="Fake company"  2>&1`" "Error: Unexpected OID value for PKI Certificate constraint." "Error invalid constraint value"
diag "------------------------------------------------------------------------------";

like "`$exec -s -F SHA-256:11a700b0c8066c47ecba05ed37bc14dcadb238552d86c659342d1d7e87b877 -o ${tmp}/too_short_hash.ksig -S ${KSI_TCP_AGGREGATOR} ${KSI_TCP_LOGIN} 2>&1`" "Error: Hash length is incorrect. Parameter -F 'SHA-256:11a700b0c8066c47ecba05ed37bc14dcadb238552d86c659342d1d7e87b877'."

like "`$exec -s -F SHA-256:11a700b0c8066c47ecba05ed37bc14dcadb238552d86c659342d1d7e87b8772d56 -o ${tmp}/too_long_hash.ksig -S ${KSI_TCP_AGGREGATOR} ${KSI_TCP_LOGIN} 2>&1`" "Error: Hash length is incorrect. Parameter -F 'SHA-256:11a700b0c8066c47ecba05ed37bc14dcadb238552d86c659342d1d7e87b8772d56'."
diag "------------------------------------------------------------------------------";

diag "====== Testing exchanged services ======";

like "`$exec -p -T 1410848909 -X ${KSI_AGGREGATOR:4} 2>&1`" "Error: Service returned unknown PDU and HTTP error 400. Check the service URL!" "Error extend request 1 to aggregator."
diag "------------------------------------------------------------------------------";

like "`$exec -x -i ${resource_dir}/$TESTSIG -o ${tmp}/ext2.ksig -X ${KSI_AGGREGATOR:4} 2>&1`" "Error: Service returned unknown PDU and HTTP error 400. Check the service URL!" "Error extend request 2 to aggregator."
diag "------------------------------------------------------------------------------";

like "`$exec -vx -i ${tmp}/ext.ksig -X ${KSI_AGGREGATOR:4} 2>&1`" "Error: Service returned unknown PDU and HTTP error 400. Check the service URL!" "Error extend request 3 to aggregator."
diag "------------------------------------------------------------------------------";

like "`$exec -s -f ${resource_dir}/testFile -o ${tmp}/tmp.ksig -S ${KSI_EXTENDER:4} 2>&1`" "Error: Service returned unknown PDU and HTTP error 400. Check the service URL!" "Error sign request 1 to extender."
diag "------------------------------------------------------------------------------";

like "`$exec -pd -P ${KSI_AGGREGATOR:4} 2>&1`" "Error: Unable to parse publications file. Check URL or file!" "Error pubfile 1 from aggregator url."
diag "------------------------------------------------------------------------------";

like "`$exec -p -o ${tmp}/pub.bin -P ${KSI_AGGREGATOR:4} --cnstr email=test 2>&1`" "Error: Unable to parse publications file. Check URL or file!" "Error pubfile 2 from aggregator url."
diag "------------------------------------------------------------------------------";

like "`$exec -v -i ${tmp}/ext.ksig -P ${KSI_AGGREGATOR:4} 2>&1`" "Error: Unable to parse publications file. Check URL or file!" "Error pubfile 3 from aggregator url."
diag "------------------------------------------------------------------------------";

diag "######    Invalid date string format and content"
like "`$exec -p  -T \"1899-12-12 00:00:00\"  2>&1`" "Time out of range" "Invalid year."
like "`$exec -p  -T \"2015-13-15 00:00:00\"  2>&1`" "Time out of range" "Invalid month."
like "`$exec -p  -T \"2015-0-31 00:00:00\"  2>&1`" "Time out of range" "Invalid month."
like "`$exec -p  -T \"2013-2-29 00:00:00\"  2>&1`" "Time out of range" "Invalid Day - not a leap year."
like "`$exec -p  -T \"2015-09-31 00:00:00\"  2>&1`" "Time out of range" "Invalid Day."
like "`$exec -p  -T \"2015-09-0 00:00:00\"  2>&1`" "Time out of range" "Invalid Day."
like "`$exec -p  -T \"2015-10-30 24:00:00\"  2>&1`" "Time out of range" "Invalid Hour."
like "`$exec -p  -T \"2015-10-30 00:60:00\"  2>&1`" "Time out of range" "Invalid Min."
like "`$exec -p  -T \"2015-10-30 00:00:60\"  2>&1`" "Time out of range" "Invalid Sec."
like "`$exec -p  -T \"12-15-2015 00:00:00\"  2>&1`" "Time not formatted as YYYY-MM-DD hh:mm:ss" "Invalid Format."
diag "------------------------------------------------------------------------------";


# cleanup
rm -f ${tmp} 2> /dev/null

