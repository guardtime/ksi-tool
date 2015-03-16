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
REM SET SIG_SERV_IP=http://192.168.100.29:1234/ 
SET SIG_SERV_IP=Http://192.168.100.36:3333/
rem SET VER_SERV_IP=http://192.168.100.29:1111/gt-extendingservice
REM SET PUB_SERV_IP=http://172.20.20.7/publications.tlv
set PUB_SERV_IP=http://verify.guardtime.com/ksi-publications.bin
REM SET SIG_SERV_IP=http://172.20.20.4:3333/
REM SET VER_SERV_IP=http://192.168.100.36:8081/gt-extendingservice
SET VER_SERV_IP=http://192.168.100.36:8081/



SET SERVICES=-S %SIG_SERV_IP% -X %VER_SERV_IP% -P %PUB_SERV_IP% -C 5 -c 5

REM input files
SET TEST_FILE=../test/testFile
SET TEST_OLD_SIG=../test/ok-sig-2014-04-30.1.ksig
SET SH256_DATA_FILE=../test/data_sh256.txt
SET INVALID_PUBFILE=../test/nok_pubfile
SET INVALID_SIG_CERTID=../test/invalid_cert_id.ksig
SET INVALID_SIG_CORPSIG=../test/invalid_signature_value.ksig
SET INVALID_SIG_CORPSIGLEN=../test/invalid_signature_len.ksig

REM input hash
SET SH1_HASH=bf9661defa3daecacfde5bde0214c4a439351d4d
SET SH256_HASH=c8ef6d57ac28d1b4e95a513959f5fcdd0688380a43d601a5ace1d2e96884690a
SET RIPMED160_HASH=0a89292560ae692d3d2f09a3676037e69630d022

REM output files
SET TEST_EXTENDED_SIG=../test/out/extended.ksig
SET SH1_file=../test/out/sh1.ksig
SET SH256_file=../test/out/SH256.ksig
SET RIPMED160_file=../test/out/RIPMED160.ksig
SET TEST_FILE_OUT=../test/out/testFile
SET PUBFILE=../test/out/pubfile

rem ksitool.exe -x -i ..\test\out\testFile.ksig -o ..\test\out\__extended -X http://192.168.100.36:8081/gt-extendingservice -T 1410858222
REM Cert files to use
REM SET CERTS= -V "C:\Users\Taavi\Documents\GuardTime\certs\Symantec Class 1 Individual Subscriber CA(64).crt" 
REM set CERTS= %CERTS% -V "C:\Users\Taavi\Documents\GuardTime\certs\VerSign Class 1 Public Primary Certification Authority - G3(64).crt"
set CERTS= %CERTS% -V "C:\Users\Taavi\Documents\GuardTime\certs\ca-bundle.trust.crt"

rem remove dir
rm -r ..\test\out
md ..\test\out\

SET GLOBAL= %CERTS% %SERVICES%
SET SIGN_FLAGS= -t
SET VERIFY_FLAGS= -n -r -d -t 
SET EXTEND_FLAGS= -t -t

echo ************************************************************************** 
echo ************************** ENVIRONMENT VARIABLES ************************* 
echo ************************************************************************** 

SETLOCAL 
echo 1) Invalid variable format
set KSI_EXTENDER="http://ksigw.test.guardtime.com:8010/gt-extendingservice"
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_FILE_OUT%.ksig -o %TEST_EXTENDED_SIG%
echo __________________________________________________________________________

echo 2) Unknown url scheme from environment variable
set KSI_EXTENDER="url=bugi://ksigw.test.guardtime.com:8010/gt-extendingservice"
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_FILE_OUT%.ksig -o %TEST_EXTENDED_SIG%
echo __________________________________________________________________________

echo 3) Empry Url
set KSI_EXTENDER="" 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_FILE_OUT%.ksig -o %TEST_EXTENDED_SIG%
echo __________________________________________________________________________
ENDLOCAL


echo ************************************************************************** 
echo ********************************** SIGN ********************************** 
echo ************************************************************************** 
echo 1) Sign data missing -f
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %TEST_FILE_OUT%.ksig 
echo return %errorlevel%

echo __________________________________________________________________________
echo 2) Sign data missing -o
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -f %TEST_FILE%  
echo return %errorlevel%

echo __________________________________________________________________________
echo 3) Sign data wrong user or pass
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -f %TEST_FILE% -o %TEST_FILE_OUT%.ksig --user ano --pass anon
echo return %errorlevel%

echo __________________________________________________________________________
echo 4) Error signing with SH1 and wrong hash 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %TEST_FILE_OUT%err  -F SHA-1:%SH1_HASH%ff
echo return %errorlevel%

echo __________________________________________________________________________
echo 5) Error signing with SH1 and invalid hash 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %TEST_FILE_OUT%err  -F SHA-1:%SH1_HASH%f
echo return %errorlevel%

echo __________________________________________________________________________
echo 6) Error signing with unknown algorithm and wrong hash 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %TEST_FILE_OUT%err  -F _UNKNOWN:%SH1_HASH%
echo return %errorlevel%

echo __________________________________________________________________________
echo 7) Error with CRYPTOAPI signing with unimplemented algorithm  
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -f %TEST_FILE% -o %TEST_FILE_OUT%err  -H RIPEMD-160
echo return %errorlevel%

echo __________________________________________________________________________
echo 8) Error bad network provider 
ksitool.exe -s %SIGN_FLAGS% -o %SH1_file%  -F SHA-1:%SH1_HASH% -S plaplaplaplpalpalap
echo return %errorlevel%
echo __________________________________________________________________________


echo ************************************************************************** 
echo ********************************* VERIFY ********************************* 
echo ************************************************************************** 

echo __________________________________________________________________________
echo 1) Verify missing x or b 
ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig 
echo return %errorlevel%

echo __________________________________________________________________________
echo 2) Verify missing i 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% 
echo return %errorlevel%

echo __________________________________________________________________________
echo 3) Error verify not suitable format 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE% 
echo return %errorlevel%

echo __________________________________________________________________________
echo 4) Error verifying signature and wrong file 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -f %TEST_FILE_OUT%.ksig
echo return %errorlevel%

echo __________________________________________________________________________
echo 5) Error verifying signature (no references) and wrong file 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -f %TEST_FILE_OUT%.ksig
echo return %errorlevel%

echo __________________________________________________________________________
echo 6) Error Verify signature and missing file 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -f missing_file
echo return %errorlevel%

echo __________________________________________________________________________
echo 7) Error Invalid publications file
ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -b %INVALID_PUBFILE%
echo return %errorlevel%
echo __________________________________________________________________________

echo __________________________________________________________________________
echo 8) Error Invalid signature: cert ID is wrong
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %INVALID_SIG_CERTID%
echo return %errorlevel%
echo __________________________________________________________________________

echo 9) Error Invalid signature: signature data is corrupted
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %INVALID_SIG_CORPSIG%
echo return %errorlevel%
echo __________________________________________________________________________

echo 10) Error Invalid signature: signature data end is deleted
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %INVALID_SIG_CORPSIGLEN%
echo return %errorlevel%
echo __________________________________________________________________________

echo ************************************************************************** 
echo ********************************* EXTEND ********************************* 
echo **************************************************************************

echo __________________________________________________________________________
echo 1) Error extend no suitable publication
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_FILE_OUT%.ksig -o %TEST_EXTENDED_SIG%
echo return %errorlevel%

echo __________________________________________________________________________
echo 2) Error extend not suitable format
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_FILE% -o %TEST_EXTENDED_SIG%
echo return %errorlevel%
echo __________________________________________________________________________


echo ************************************************************************** 
echo ******************************* PUBLICATION ****************************** 
echo **************************************************************************

echo __________________________________________________________________________
echo 1) Error missing cert files 
ksitool.exe -p -o %PUBFILE% %CERTS% -V missing1 -V missing2 -V missing3 
echo return %errorlevel%

echo __________________________________________________________________________
echo 2) Error Unable to Get Publication string
ksitool.exe -p %GLOBAL% -T 969085709
echo return %errorlevel%

echo __________________________________________________________________________
echo 3) Error wrong E-mail
ksitool.exe -p -t -o %PUBFILE%err %GLOBAL% -E magic@email.null 
echo return %errorlevel%

echo __________________________________________________________________________
echo 4) Error invalid empty cert file (Dose not fail with cryptoAPI)
ksitool.exe -p -t -o %PUBFILE%err %GLOBAL% -V %TEST_FILE% 
echo return %errorlevel%
echo __________________________________________________________________________

echo 5) Error invalid empty cert file
ksitool.exe -p -t -o %PUBFILE%err %GLOBAL% -V %TEST_OLD_SIG% 
echo return %errorlevel%
echo __________________________________________________________________________
pause