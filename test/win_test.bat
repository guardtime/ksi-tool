GOTO copyrightend

    GUARDTIME CONFIDENTIAL

    Copyright (C) [2015] Guardtime, Inc
    All Rights Reserved

    NOTICE:  All information contained herein is, and remains, the
    property of Guardtime Inc and its suppliers, if any.
    The intellectual and technical concepts contained herein are
    proprietary to Guardtime Inc and its suppliers and may be
    covered by U.S. and Foreign Patents and patents in process,
    and are protected by trade secret or copyright law.
    Dissemination of this information or reproduction of this
    material is strictly forbidden unless prior written permission
    is obtained from Guardtime Inc.
    "Guardtime" and "KSI" are trademarks or registered trademarks of
    Guardtime Inc.

:copyrightend

@ECHO OFF
cd ../bin

REM test configuration
SET WAIT=5

REM Services to use
SET SIG_SERV_IP=http://ksigw.test.guardtime.com:3333/gt-signingservice 
SET TCP_SERV=ksi+tcp://ksigw.test.guardtime.com:3332
REM SET SIG_SERV_IP=http://192.168.100.36:3333/
REM SET PUB_SERV_IP=http://172.20.20.7/publications.tlv
set PUB_SERV_IP=Http://verify.guardtime.com/ksi-publications.bin
SET PUB_CNSTR=--cnstr 1.2.840.113549.1.9.1=publications@guardtime.com
REM SET SIG_SERV_IP=http://172.20.20.4:3333/
REM SET VER_SERV_IP=http://192.168.100.36:8081/gt-extendingservice
REM SET VER_SERV_IP=http://192.168.100.36:8081/
SET VER_SERV_IP=httP://ksigw.test.guardtime.com:8010/gt-extendingservice


SET SERVICES=-S %SIG_SERV_IP% -X %VER_SERV_IP% -P %PUB_SERV_IP% %PUB_CNSTR% -C 5 -c 5 --user anon --pass anon
REM SET SERVICES=-S %SIG_SERV_IP% -X %VER_SERV_IP% -C 5 -c 5 --user anon --pass anon
SET TCP_login=--user anon --pass anon

REM input files
SET TEST_FILE=../test/resource/testFile
SET TEST_OLD_SIG=../test/resource/ok-sig-2014-04-30.1.ksig
SET SH256_DATA_FILE=../test/resource/data_sh256.txt
SET INVALID_PUBFILE=../test/resource/nok_pubfile
SET OLD_TESTFILE=../test/resource/old_testData-2015-01
SET LEGACY_SIGNATURE=../test/resource/old_testData-2015-01.gtts
SET LEGACY_PRE_EXTENDED=../test/resource/old_testdata-extended.2015-01.gtts

REM input hash
SET SH1_HASH=bf9661defa3daecacfde5bde0214c4a439351d4d
SET SH256_HASH=c8ef6d57ac28d1b4e95a513959f5fcdd0688380a43d601a5ace1d2e96884690a
SET RIPMED160_HASH=0a89292560ae692d3d2f09a3676037e69630d022
SET SHA224_HASH=9b7ea5330761e8b50b36af0d61c10bc227c908ee57a545d40131cfa3
SET SHA384_HASH=a5ac3bb2fa156480d1cf437c54481d9c77a145b682879e92e30a8b79f0a45a001be7969ffa02d81af0610b784ae72f4f
SET SHA512_HASH=09e3fc9d3669eaf53d3afeb60e6a73af2c7c7b01a0fe49127253e0d466ba3d1c85ed541593775a12a880378335eeda5fc0ad5700920e11ed315f4b49f37c6d26

REM output files
SET TEST_EXTENDED_SIG=../test/out/extended.ksig
SET OLD_EXTENDED=../test/out/legacy_extended.gtts
SET SH1_file=../test/out/sh1.ksig
SET SH256_file=../test/out/SH256.ksig
SET PIPE_FILE=../test/out/pipe.file
SET PIPE_SIG=../test/out/pipe.sig
SET PIPE_LOG_FILE../test/out/ksi.pipe.log

SET RIPMED160_file=../test/out/RIPMED160.ksig
SET TEST_FILE_OUT=../test/out/testFile
SET PUBFILE=../test/out/pubfile



rem ksitool.exe -x -i ..\test\out\testFile.ksig -o ..\test\out\__extended -X http://192.168.100.36:8081/gt-extendingservice -T 1410858222
REM Cert files to use
REM SET CERTS= -V "C:\Users\Taavi\Documents\GuardTime\certs\Symantec Class 1 Individual Subscriber CA(64).crt" 
REM set CERTS= %CERTS% -V "C:\Users\Taavi\Documents\GuardTime\certs\VerSign Class 1 Public Primary Certification Authority - G3(64).crt"
REM set CERTS= %CERTS% -V "C:\Users\Taavi\Documents\GuardTime\certs\ca-bundle.trust.crt"

rem remove dir
rm -r ..\test\out
md ..\test\out\

SET GLOBAL= %CERTS% %SERVICES%
SET SIGN_FLAGS= -n -r -d -t
SET VERIFY_FLAGS= -n -r -d -t 
SET EXTEND_FLAGS= -t -t



ksitool.exe -vd -i test\testData-2015-01.gtts --log
logi.txt

echo ****************** Download publications file ******************
ksitool.exe -p -t -o %PUBFILE% %GLOBAL% -d 
echo %errorlevel%

ksitool.exe -v -t -b %PUBFILE% %GLOBAL% 
echo %errorlevel%

echo "****************** Get Publication string ******************" 
ksitool.exe -p %GLOBAL% -T 1410848909
echo %errorlevel%


echo ****************** Sign data ******************
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -f %TEST_FILE% -o %TEST_FILE_OUT%.ksig -b %PUBFILE% 
echo %errorlevel%


echo ******************Sign data over TCP ******************
ksitool.exe -s %SIGN_FLAGS% -f %TEST_FILE% -o %TEST_FILE_OUT%Tcp.ksig -S %TCP_SERV% %TCP_login%
echo %errorlevel%


echo ******************Sign different data hashes over TCP ******************
ksitool.exe -s %SIGN_FLAGS% -F SHA-256:%SH256_HASH% -o %TEST_FILE_OUT%Sha-256Tcp.ksig -S %TCP_SERV% %TCP_login%
echo %errorlevel%

ksitool.exe -s %SIGN_FLAGS% -F RIPEMD-160:%RIPMED160_HASH% -o %TEST_FILE_OUT%Ripmed160Tcp.ksig -S %TCP_SERV% %TCP_login%
echo %errorlevel%

ksitool.exe -s %SIGN_FLAGS% -F SHA-224:%SHA224_HASH% -o %TEST_FILE_OUT%Sha224Tcp.ksig -S %TCP_SERV% %TCP_login%
echo %errorlevel%

ksitool.exe -s %SIGN_FLAGS% -F SHA-384:%SHA384_HASH% -o %TEST_FILE_OUT%Sha384Tcp.ksig -S %TCP_SERV% %TCP_login%
echo %errorlevel%

ksitool.exe -s %SIGN_FLAGS% -F SHA-512:%SHA512_HASH% -o %TEST_FILE_OUT%Sha512Tcp.ksig -S %TCP_SERV% %TCP_login%
echo %errorlevel%


echo ****************** Verify using publications file ******************
ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -b %PUBFILE% 
echo %errorlevel%


echo ****************** Sign data with algorithm [-H SH-1] ****************** 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -f %TEST_FILE% -o %TEST_FILE_OUT%SH-1.ksig  -H SHA-1
echo %errorlevel%

ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%SH-1.ksig
echo %errorlevel%


echo ****************** Extend old signature ****************** 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_OLD_SIG% -o %TEST_EXTENDED_SIG%
echo %errorlevel%

ksitool.exe -vrd -i %TEST_EXTENDED_SIG% %GLOBAL%
echo %errorlevel%

echo ****************** Extend old signature to 1418601800 (between publications)****************** 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_OLD_SIG% -o %TEST_EXTENDED_SIG%2 -T 1418601800
echo %errorlevel%


ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %TEST_EXTENDED_SIG%2
echo %errorlevel%


echo ****************** Extend old signature to publication 1418601600 ****************** 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_OLD_SIG% -o %TEST_EXTENDED_SIG%3 -T 1418601600
echo %errorlevel%


ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %TEST_EXTENDED_SIG%3
echo %errorlevel%



echo "****************** Sign raw hash with algorithm specified [-F SH1:<hash>] ******************" 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %SH1_file%  -F SHA-1:%SH1_HASH%
echo %errorlevel%


echo "****************** Verify HASH [-F SH1:<hash>] ******************" 
ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %SH1_file% -F SHA-1:%SH1_HASH%
echo %errorlevel%


echo "****************** Sign raw hash with algorithm specified [-F SH256:<hash>] ******************" 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %SH256_file% -F SHA-256:%SH256_HASH%
echo %errorlevel%

ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %SH256_file% -f %SH256_DATA_FILE% -F SHA-256:%SH256_HASH%
echo %errorlevel%

echo "****************** Sign raw hash with algorithm specified [-F RIPMED160:<hash>] ******************" 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %RIPMED160_file% -F RIPEMD-160:%RIPMED160_HASH%
echo %errorlevel%

ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %RIPMED160_file%
echo %errorlevel%

echo "****************** Test include. Must show ignored parameters and fail ******************" 
ksitool.exe --inc ../test/resource/conf1 --inc ../test/resource/conf3
echo %errorlevel%

echo ****************** Verify legacy signature and file ******************
ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %LEGACY_SIGNATURE% -f %OLD_TESTFILE% 
echo %errorlevel%

echo ****************** Verify legacy signature and file online******************
ksitool.exe -vx %GLOBAL% %VERIFY_FLAGS% -i %LEGACY_SIGNATURE% -f %OLD_TESTFILE% 
echo %errorlevel%

echo ****************** Verify extended legacy signature and file ******************
ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %LEGACY_PRE_EXTENDED% -f %OLD_TESTFILE% 
echo %errorlevel%

echo ****************** Verify extended legacy signature and file online******************
ksitool.exe -vx %GLOBAL% %VERIFY_FLAGS% -i %LEGACY_PRE_EXTENDED% -f %OLD_TESTFILE% 
echo %errorlevel%

echo ****************** Extend legacy signature ****************** 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %LEGACY_SIGNATURE% -o %OLD_EXTENDED%
echo %errorlevel%

ksitool.exe -vrd %GLOBAL% -i %OLD_EXTENDED% -f %OLD_TESTFILE%
echo %errorlevel%

echo ****************** Verify with user publication. ****************** 
ksitool -vdr -i %TEST_EXTENDED_SIG% --ref AAAAAA-CT5VGY-AAPUCF-L3EKCC-NRSX56-AXIDFL-VZJQK4-WDCPOE-3KIWGB-XGPPM3-O5BIMW-REOVR4
echo %errorlevel%

echo ********** Verify with user publication - needs extending. Publication exists. ********** 
ksitool -vdr -i %TEST_EXTENDED_SIG% %PUB_CNSTR% -P %PUB_SERV_IP% --ref AAAAAA-CVFWVA-AAPV2S-SN3JLW-YEKPW3-AUSQP6-PF65K5-KVGZZA-7UYTOV-27VX54-VVJQFG-VCK6GR
echo %errorlevel%

echo ********** Verify with user publication - needs extending. No publication. ********** 
ksitool -vdr -i %TEST_EXTENDED_SIG% %PUB_CNSTR% -P %PUB_SERV_IP% --ref AAAAAA-CUBJQL-AAKVFD-VNJIK5-7DTJ6T-YYCOGP-N7J3RT-CRE5DU-WBB6AE-LANHHH-3CFEM4-7FM65J
echo %errorlevel%

echo ********** Verify with user publication - signature not extended. Needs extending.********** 
ksitool -vdr -i %TEST_OLD_SIG% %PUB_CNSTR% -P %PUB_SERV_IP% --ref AAAAAA-CUBJQL-AAKVFD-VNJIK5-7DTJ6T-YYCOGP-N7J3RT-CRE5DU-WBB6AE-LANHHH-3CFEM4-7FM65J
echo %errorlevel%

echo ********** Verify with user publication - signature not extended. Needs extending. No Publication.********** 
ksitool -vdr -i %TEST_OLD_SIG% %PUB_CNSTR% -P %PUB_SERV_IP% --ref AAAAAA-CUBJQL-AAKVFD-VNJIK5-7DTJ6T-YYCOGP-N7J3RT-CRE5DU-WBB6AE-LANHHH-3CFEM4-7FM65J
echo %errorlevel%

echo ****************** Test stdin stdout 1******************
echo "TEST" | ksitool.exe %GLOBAL% -s -f - -o - > %PIPE_SIG%
echo %errorlevel%
ksitool.exe %GLOBAL% -v -i - < %PIPE_SIG%
echo %errorlevel%

echo ****************** Test stdin stdout 2******************
echo "TEST" > %PIPE_FILE%
echo "TEST" | ksitool.exe %GLOBAL% -s -f - -o - | ksitool.exe %GLOBAL% -v -f %PIPE_FILE% -i - 
echo %errorlevel%
 
echo ****************** Test stdin stdout 3******************
ksitool.exe %GLOBAL% -p -T 1410848909 --log - > PIPE_LOG_FILE
echo %errorlevel%

echo ****************** Test stdin stdout 4******************
ksitool.exe %GLOBAL% -p --cnstr 1.2.840.113549.1.9.1=publications@guardtime.com -o - | ksitool.exe -v --cnstr 1.2.840.113549.1.9.1=publications@guardtime.com -b -
echo %errorlevel%

echo ****************** Verify online ******************
ksitool.exe -vx %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig 
echo %errorlevel%

echo ****************** Verify pubfile with constraintsss ******************
ksitool.exe -v %GLOBAL% -b ..\test\resource\publications.tlv --cnstr 2.5.4.10="Guardtime AS" -V ..\test\resource\mock.crt
echo %errorlevel%
pause