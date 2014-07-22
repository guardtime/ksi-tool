@ECHO OFF
cd ../bin

REM test configuration
SET WAIT=5

REM 
SET SIG_SERV_IP=192.168.1.36:3333/
SET VER_SERV_IP=192.168.1.29:1111/gt-extendingservice
SET PUB_SERV_IP=172.20.20.7/publications.tlv
REM SET SIG_SERV_IP=172.20.20.4:3333/
REM SET VER_SERV_IP=192.168.1.36:8081/gt-extendingservice
SET SERVICES=-S %SIG_SERV_IP% -X %VER_SERV_IP% -P %PUB_SERV_IP%

REM input files
SET TEST_FILE=../test/testFile
SET TEST_OLD_SIG=../test/ok-sig-2014-04-30.1.ksig
SET SH256_DATA_FILE=../test/data_sh256.txt

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



rm %TEST_EXTENDED_SIG% %RIPMED160_file% %SH1_file% %SH256_file% %TEST_FILE_OUT%.ksig %TEST_FILE_OUT%SH-1.ksig %PUBFILE%


echo ****************** download publications file ******************
gtime.exe -p -t -o %PUBFILE%
echo %errorlevel%
gtime.exe -v -t -b %PUBFILE%


echo ****************** signing data -n ******************
gtime.exe -s %SERVICES% -f %TEST_FILE% -o %TEST_FILE_OUT%.ksig  -n
echo %errorlevel%
sleep %WAIT%
gtime.exe -v -t %SERVICES% -x -i %TEST_FILE_OUT%.ksig -n %SERVICES% 
echo %errorlevel%

echo ****************** signing data with algorithm -H SH-1 ****************** 
gtime.exe -s %SERVICES% -f %TEST_FILE% -o %TEST_FILE_OUT%SH-1.ksig  -H SHA-1
echo %errorlevel%
sleep %WAIT%
gtime.exe -v %SERVICES% -x -i %TEST_FILE_OUT%SH-1.ksig
echo %errorlevel%

echo ****************** verifying signature and file ****************** 
gtime.exe -v -t %SERVICES% -x -i %TEST_FILE_OUT%.ksig -f %TEST_FILE%
echo %errorlevel%

echo ****************** verifying signature and missing file ****************** 
gtime.exe -v -t %SERVICES% -x -i %TEST_FILE_OUT%.ksig -f missing_file
echo %errorlevel%

echo ****************** extend old signature ****************** 
gtime.exe -x -t %SERVICES% -i %TEST_OLD_SIG% -o %TEST_EXTENDED_SIG%
echo %errorlevel%
sleep %WAIT%
gtime.exe -v %SERVICES% -x -i %TEST_EXTENDED_SIG%
echo %errorlevel%

echo ****************** signing with SH1 ****************** 
gtime.exe -s %SERVICES% -o %SH1_file%  -F SHA-1:%SH1_HASH%
echo %errorlevel%
sleep %WAIT%
gtime.exe -v %SERVICES% -x -i %SH1_file%
echo %errorlevel%

echo ****************** signing with SH256 ****************** 
gtime.exe -s %SERVICES% -o %SH256_file% -S %SIG_SERV_IP% -F SHA-256:%SH256_HASH%
echo %errorlevel%
sleep %WAIT%
gtime.exe -v %SERVICES% -x -i %SH256_file% -f %SH256_DATA_FILE%
echo %errorlevel%

echo ****************** signing with RIPMED160 ****************** 
gtime.exe -s %SERVICES% -o %RIPMED160_file% -S %SIG_SERV_IP% -F RIPEMD-160:%RIPMED160_HASH%
echo %errorlevel%
sleep %WAIT%
gtime.exe -v %SERVICES% -x -i %RIPMED160_file%
echo %errorlevel%

echo ****************** error extend no suitable publication ****************** 
gtime.exe -x -t %SERVICES% -i %TEST_FILE_OUT%.ksig -o %TEST_EXTENDED_SIG%
echo %errorlevel%

echo ****************** error extend not suitable format ****************** 
gtime.exe -x -t %SERVICES% -i %TEST_FILE% -o %TEST_EXTENDED_SIG%
echo %errorlevel%

echo ****************** error extend not suitable format ****************** 
gtime.exe -v -x -t %SERVICES% -i %TEST_FILE% 
echo %errorlevel%

echo ****************** error verifying signature and wrong file ****************** 
gtime.exe -v -t %SERVICES% -x -i %TEST_FILE_OUT%.ksig -f %TEST_FILE_OUT%.ksig
echo %errorlevel%

echo ****************** error signing with SH1 and wrong hash ****************** 
gtime.exe -s %SERVICES% -o %SH1_file%  -F SHA-1:%SH1_HASH%ff
echo %errorlevel%

echo ****************** error signing with unknown algorithm and wrong hash ****************** 
gtime.exe -s %SERVICES% -o %TEST_FILE%  -F _UNKNOWN:%SH1_HASH%
echo %errorlevel%

echo ****************** error bad network provider****************** 
gtime.exe -s -o %SH1_file%  -F SHA-1:%SH1_HASH% -S plaplaplaplpalpalap
echo %errorlevel%





pause