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
CALL "%ProgramW6432%\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

SET OPENSSL_DIR=C:\Users\Taavi\Documents\GuardTime\LIBS\openssl-0.9.8g-win64
SET CURL_DIR=C:\Users\Taavi\Documents\GuardTime\LIBS\curl-7.37.0
SET KSI_DIR=C:\Users\Taavi\Documents\GuardTime\ksi-c-api\out
REM SET KSI_DIR=C:\Users\Taavi\Documents\GuardTime\MULTIBUILD\ksi-c-api_wininet\out


ECHO ************ Rebuilding project ************

nmake clean
nmake /S RTL=MDd KSI_LIB=dll  

pause