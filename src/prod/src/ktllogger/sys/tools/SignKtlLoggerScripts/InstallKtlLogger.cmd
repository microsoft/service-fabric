:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

rem
rem Required in directory are ktllogger.cdf, ktllloger.inf and ktllogger.sys
rem
makecat -v KtlLogger.cdf

certutil.exe -delstore root KtlLoggerCert
certutil.exe -delstore trustedpublisher KtlLoggerCert
certutil.exe -user -delstore KtlLoggerCertStore KtlLoggerCert

makecert.exe -r -pe -ss KtlLoggerCertStore -n "CN=KtlLoggerCert" KtlLoggerCert.cer

certutil.exe -addstore root KtlLoggerCert.cer
certutil.exe -addstore trustedpublisher KtlLoggerCert.cer

signtool.exe sign /s KtlLoggerCertStore /n KtlLoggerCert ktllogger.cat
signtool.exe sign /s KtlLoggerCertStore /n KtlLoggerCert ktllogger.sys