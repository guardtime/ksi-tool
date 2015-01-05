@ECHO OFF
CALL "%ProgramW6432%\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

SET OPENSSL_DIR=C:\Users\Taavi\Documents\GuardTime\LIBS\openssl-0.9.8g-win64
SET CURL_DIR=C:\Users\Taavi\Documents\GuardTime\LIBS\curl-7.37.0
SET KSI_DIR=C:\Users\Taavi\Documents\GuardTime\ksi-c-api\out
REM SET KSI_DIR=C:\Users\Taavi\Documents\GuardTime\MULTIBUILD\ksi-c-api_wininet\out


ECHO ************ Rebuilding project ************

nmake clean
nmake /S RTL=MTd KSI_LIB=lib  

pause