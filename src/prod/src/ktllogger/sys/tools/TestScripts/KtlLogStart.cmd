:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

@SETLOCAL
@echo on
cd /d %windir%\ktlevents
del *.etl
del pftemp.tmp
echo Microsoft-Windows-KTL 1>> pftemp.tmp
logman delete ktl
logman create trace ktl -pf pftemp.tmp -o %windir%\ktlevents\trace -ct Perf %*
logman start ktl
