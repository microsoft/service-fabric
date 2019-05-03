:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

@echo off

for %%p in (%MAKEDIR%\..\StoreCounters.h) do set IN_HEADER=%%~fp
for %%p in (%MAKEDIR%\..\Generated.PerformanceCounters.h) do set OUT_HEADER=%%~fp
set TEMP_HEADER=%MAKEDIR%\Generated.PerformanceCounters_temp.h

del %TEMP_HEADER%

cl.exe /C /EP %IN_HEADER% > %TEMP_HEADER%
IF ERRORLEVEL 1 EXIT 1

sd edit %OUT_HEADER%
IF ERRORLEVEL 1 EXIT 1

move /Y %TEMP_HEADER% %OUT_HEADER%
IF ERRORLEVEL 1 EXIT 1

sd revert -a %OUT_HEADER%
IF ERRORLEVEL 1 EXIT 1