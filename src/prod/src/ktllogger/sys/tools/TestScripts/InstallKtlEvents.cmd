:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

@SETLOCAL
@echo off

set BUILDBINS=%cd%
if NOT "%1" == "" (set BUILDBINS=%1)

mkdir %systemroot%\Ktl 2>:NUL
cd /d %systemroot%\Ktl
wevtutil um %BUILDBINS%\Microsoft-ServiceFabric-KtlEvents.man
xcopy /dy %BUILDBINS%\KtlEvents.dll
xcopy /dy %BUILDBINS%\Microsoft-ServiceFabric-KtlEvents.man
md en-us 2>:NUL
xcopy /dy %BUILDBINS%\ktlevents.dll.mui en-us
wevtutil im Microsoft-ServiceFabric-KtlEvents.man

