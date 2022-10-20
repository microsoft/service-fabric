:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

@SETLOCAL
@echo off
cd /d %windir%\ktl
del *.etl
del pftemp.tmp
echo Microsoft-ktlProv 1>> pftemp.tmp
logman delete ktl
logman create trace ktl -pf pftemp.tmp -o %windir%\ktl\trace -ct Perf %*
logman start ktl
