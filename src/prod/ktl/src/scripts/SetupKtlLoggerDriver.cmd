:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

if %1a == -?a goto Usage

set FileLocation=%1
if %1a == a set FileLocation=%OUTPUTROOT%\debug-AMD64\ktllogger

set Utilities=%2
if %2a == a set Utilities=\\winfabfs.redmond.corp.microsoft.com\public\Utilities

certutil.exe -delstore root KtlLoggerCert
certutil.exe -delstore trustedpublisher KtlLoggerCert
certutil.exe -user -delstore KtlLoggerCertStore KtlLoggerCert
%Utilities%\makecert.exe -r -pe -ss KtlLoggerCertStore -n "CN=KtlLoggerCert" KtlLoggerCert.cer
certutil.exe -addstore root KtlLoggerCert.cer
certutil.exe -addstore trustedpublisher KtlLoggerCert.cer
%Utilities%\signtool.exe sign /s KtlLoggerCertStore /n KtlLoggerCert %FileLocation%\KtlLogger.cat
%Utilities%\signtool.exe sign /s KtlLoggerCertStore /n KtlLoggerCert %FileLocation%\KtlLogger.sys
copy %FileLocation%\KtlLogger.sys %windir%\system32\drivers
dir C:\windows\system32\drivers\ktllogger.sys
goto Done

:Usage
echo "SetupKtlLoggerDriver [Debug-AMD64 | Retail-AMD64] [Utilities Path]"

:Done