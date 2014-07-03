@ECHO OFF
CALL "%ProgramW6432%\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

SET OPENSSL_DIR=C:\Users\Taavi\Documents\GuardTime\LIBS\openssl-0.9.8g-win64
SET CURL_DIR=C:\Users\Taavi\Documents\GuardTime\LIBS\curl-7.37.0
SET OPENSSL_CA_FILE=C:\Users\Taavi\Documents\GuardTime\ksi-c-api\test\resource\tlv\mock.crt
SET KSI_DIR=C:\Users\Taavi\Documents\GuardTime\ksi-c-api\out


ECHO ************ Rebuilding project ************
nmake clean
nmake RTL=MTd 

pause