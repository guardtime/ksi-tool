@ECHO OFF
cd ../bin

REM test configuration
SET WAIT=5

REM Services to use
REM SET SIG_SERV_IP=http://ksigw.test.guardtime.com:3333/gt-signingservice 
SET SIG_SERV_IP=htTp://192.168.100.29:1234/
REM SET PUB_SERV_IP=http://172.20.20.7/publications.tlv
set PUB_SERV_IP=Http://verify.guardtime.com/ksi-publications.bin
REM SET SIG_SERV_IP=http://172.20.20.4:3333/
REM SET VER_SERV_IP=http://192.168.100.36:8081/gt-extendingservice
REM SET VER_SERV_IP=http://192.168.100.36:8081/
SET VER_SERV_IP=httP://ksigw.test.guardtime.com:8010/gt-extendingservice


SET SERVICES=-S %SIG_SERV_IP% -X %VER_SERV_IP% -P %PUB_SERV_IP% -C 5 -c 5 --user anon --pass anon --log KSI_LOGI.txt

REM input files
SET TEST_FILE=../test/testFile
SET TEST_OLD_SIG=../test/ok-sig-2014-04-30.1.ksig
SET SH256_DATA_FILE=../test/data_sh256.txt
SET INVALID_PUBFILE=../test/nok_pubfile

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
SET SIGN_FLAGS= -n -r -d -t
SET VERIFY_FLAGS= -n -r -d -t 
SET EXTEND_FLAGS= -t -t

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
sleep %WAIT%

echo ****************** Verify online ******************
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig 
echo %errorlevel%

echo ****************** Verify using publications file ******************
ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -b %PUBFILE% 
echo %errorlevel%


echo ****************** Sign data with algorithm [-H SH-1] ****************** 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -f %TEST_FILE% -o %TEST_FILE_OUT%SH-1.ksig  -H SHA-1
echo %errorlevel%
sleep %WAIT%
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%SH-1.ksig
echo %errorlevel%


echo ****************** Extend old signature ****************** 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_OLD_SIG% -o %TEST_EXTENDED_SIG%
echo %errorlevel%
sleep %WAIT%
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_EXTENDED_SIG%
echo %errorlevel%


echo ****************** Extend old signature to 1418601600 ****************** 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_OLD_SIG% -o %TEST_EXTENDED_SIG%2 -T 1418601600
echo %errorlevel%
sleep %WAIT%
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_EXTENDED_SIG%2
echo %errorlevel%


echo "****************** Sign raw hash with algorithm specified [-F SH1:<hash>] ******************" 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %SH1_file%  -F SHA-1:%SH1_HASH%
echo %errorlevel%
sleep %WAIT%
ksitool.exe -v %GLOBAL% %VERIFY_FLAGS% -x -i %SH1_file%
echo %errorlevel%

echo "****************** Sign raw hash with algorithm specified [-F SH256:<hash>] ******************" 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %SH256_file% -F SHA-256:%SH256_HASH%
echo %errorlevel%
sleep %WAIT%
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %SH256_file% -f %SH256_DATA_FILE%
echo %errorlevel%

echo "****************** Sign raw hash with algorithm specified [-F RIPMED160:<hash>] ******************" 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %RIPMED160_file% -F RIPEMD-160:%RIPMED160_HASH%
echo %errorlevel%
sleep %WAIT%
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %RIPMED160_file%
echo %errorlevel%

echo "****************** Test include. Must show ignored parameters and fail ******************" 
ksitool.exe --inc ../test/conf1 --inc ../test/conf3
echo %errorlevel%




echo ****************** Error extend no suitable publication ****************** 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_FILE_OUT%.ksig -o %TEST_EXTENDED_SIG%
echo %errorlevel%

echo ****************** Error extend not suitable format ****************** 
ksitool.exe -x %GLOBAL% %EXTEND_FLAGS% -i %TEST_FILE% -o %TEST_EXTENDED_SIG%
echo %errorlevel%

echo ****************** Error verify not suitable format ****************** 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE% 
echo %errorlevel%

echo ****************** Error verifying signature and wrong file ****************** 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -f %TEST_FILE_OUT%.ksig
echo %errorlevel%

echo ****************** Error verifying signature (no references) and wrong file ****************** 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -f %TEST_FILE_OUT%.ksig
echo %errorlevel%

echo ****************** Error signing with SH1 and wrong hash ****************** 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %TEST_FILE_OUT%err  -F SHA-1:%SH1_HASH%ff
echo %errorlevel%

echo ****************** Error signing with SH1 and invalid hash ****************** 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %TEST_FILE_OUT%err  -F SHA-1:%SH1_HASH%f
echo %errorlevel%

echo ****************** Error signing with unknown algorithm and wrong hash ****************** 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -o %TEST_FILE_OUT%err  -F _UNKNOWN:%SH1_HASH%
echo %errorlevel%

echo ****************** Error with CRYPTOAPI signing with unimplemented algorithm ****************** 
ksitool.exe -s %GLOBAL% %SIGN_FLAGS% -f %TEST_FILE% -o %TEST_FILE_OUT%err  -H RIPEMD-160
echo %errorlevel%

echo ****************** Error bad network provider ****************** 
ksitool.exe -s %SIGN_FLAGS% -o %SH1_file%  -F SHA-1:%SH1_HASH% -S plaplaplaplpalpalap
echo %errorlevel%

echo ****************** Error Verify signature and missing file ****************** 
ksitool.exe -v -x %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -f missing_file
echo %errorlevel%

echo ****************** Error missing cert files ****************** 
ksitool.exe -p -o %PUBFILE% %CERTS% -V missing1 -V missing2 -V missing3 
echo %errorlevel%

echo ****************** Error Invalid publications file ******************
ksitool.exe -v -t %GLOBAL% %VERIFY_FLAGS% -i %TEST_FILE_OUT%.ksig -b %INVALID_PUBFILE%
echo %errorlevel%

echo "****************** Error Unable to Get Publication string ******************" 
ksitool.exe -p %GLOBAL% -T 969085709
echo %errorlevel%

echo ****************** Error wrong E-mail******************
ksitool.exe -p -t -o %PUBFILE%err %GLOBAL% -E magic@email.null 
echo %errorlevel%


pause