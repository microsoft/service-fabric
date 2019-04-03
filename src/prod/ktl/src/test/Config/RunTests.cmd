:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

@echo off 
setlocal EnableDelayedExpansion

:: ===========================================================================
:: Set up directory paths, logs for failed and disabled tests, etc
::
set CurDir=%~dp0

set argument=%*

:: Ensure we are running from 'ktltest'
:: We cannot run it from Git source path since the corext environment
:: doesn't provide  build type (debug/retail) info in its environment.
:: So, we aren't sure if we need to go to debug-amd64\bin\ktltest or retail-amd64\bin\ktltest
:: So, executing from the destination directory itself.
echo %CurDir% >%temp%\runtests.curdir.txt
find /I "ktltest" %temp%\runtests.curdir.txt >nul
set ev=%errorlevel%
del %temp%\runtests.curdir.txt
if %ev% equ 1 goto RunFromKtlTestDirMsg

set CurDir=%~dp0
:: Remove the last backslash
set CurDir=%CurDir:~0,-1%

:: _NTTREE = parent dir path of ktltest
for %%d in (%CurDir%) do set ParentDir=%%~dpd
set _NTTREE=%ParentDir:~0,-1%%

set BINARIES_DIR=%_NTTREE%\ktltest
set DOTNET_CLI=%_NTTREE%\DotnetCoreCli
set path=%path%;%BINARIES_DIR%;%DOTNET_CLI%
set RunTestsReturn=true

set arg1=%1
if "!arg1!"=="/?" goto :PrintUsage
if "!arg1!"=="-?" goto :PrintUsage
if "!arg1!"=="/list" goto :ListTests

call :RunTests !argument!
if "!RunTestsReturn!"=="false" exit /b 1

endlocal
goto :EOF

:: ===========================================================================
:: Run Tests

:RunTests
set runtestsargs=%*

pushd %BINARIES_DIR%

echo Stopping trace sessions...
logman stop ktl -ets > nul
logman stop RunTests-PerfCounters
echo Trace sessions stopped.

echo.
echo Invoking RunTests.exe...

cd %CurDir%
del /Q log\PerfCounters\*.*

set Cmd=%CurDir%\RunTests.exe !runtestsargs!
echo %Cmd%
%Cmd%

if ERRORLEVEL 1 (
  echo FAILED. RunTests failed
  set RunTestsReturn=false
)

popd
goto :EOF

:: ===========================================================================
:: List the tests
::
:ListTests

%CurDir%\RunTests.exe %*

endlocal
goto :EOF

:: ===========================================================================
:: Print the usage
::

:PrintUsage

echo.
echo This script runs all checkin and Xml Tests through RunTests.exe
echo.
echo Usage: RunTests.cmd [Arguments for RunTests.exe]
echo.
echo where,
echo.
echo  ADDITIONAL Arguments for RunTests.exe are below:
echo.


%CurDir%\RunTests.exe /?
goto :EOF

:: ===========================================================================
:: Displays a message to run the unit tests from 'ktltest' folder
::
:RunFromKtlTestDirMsg
echo.
echo Please run this script from 'ktltest' folder under bin
echo E.g. from \rdnext\WindowsFabric\out\debug-amd64\bin\ktltest
echo.
echo For detailed usage, please type '%~nx0 /?'
echo.

endlocal
goto :EOF
