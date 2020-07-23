:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

@echo on

REM Copyright (c) .NET Foundation and contributors. All rights reserved.
REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

"%MSBuildToolsPath_150%\Roslyn\csc.exe" %*
SET CSCExitCode=%ERRORLEVEL%
IF %CSCExitCode% EQU 0 (
    exit /b %CSCExitCode%
)
ECHO "CSC exited with non zero error: %CSCExitCode%, retrying"
"%MSBuildToolsPath_150%\Roslyn\csc.exe" %*
exit /b %errorlevel%