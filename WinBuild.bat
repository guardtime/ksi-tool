@ECHO OFF
CALL "%ProgramW6432%\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

REM SET CURL_DIR=C:\Users\Taavi\Documents\GuardTime\TEST_OLD_LIB\curl-7.37.0\builds\libcurl-vc10-x64-debug-static-ipv6-sspi-spnego-winssl
REM SET CURL_DIR="C:\Users\Taavi\Documents\GuardTime\TEST_OLD_LIB\curl-7.37.0\builds\libcurl-vc10-x64-release-static-ipv6-sspi-spnego-winssl"
REM SET OPENSSL_CA_FILE="C:\Users\Taavi\Documents\GuardTime\ksicapi\test\resource\tlv\mock.crt"
REM SET OPENSSL_DIR="C:\Users\Taavi\Documents\GuardTime\TEST_OLD_LIB\openssl-0.9.8g-bin-win64" 
REM SET KSI_INCLUDE_DIR = "C:\Users\Taavi\Documents\GuardTime\ksicapi\src\ksi"
REM CURL_DIR=$(CURL_DIR) OPENSSL_CA_FILE=$(OPENSSL_CA_FILE) OPENSSL_DIR=$(OPENSSL_DIR) KSI_INCLUDE_DIR=$(KSI_INCLUDE_DIR)


ECHO ************ Rebuilding project ************
nmake /f makefile.vc clean
nmake /f makefile.vc RTL=MTd 

pause