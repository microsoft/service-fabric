:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

@echo off
setlocal enabledelayedexpansion

set AzureTestDir=%_NTTREE%\FabricUnitTests
set TaefBinariesDir=%SDXROOT%\services\winfab\imports\taef\amd64\x64\TestExecution
set SubscriptionId= 
set ManagementCertThumbprint=
set HostedService=
set StorageAccount=
set StorageKey=
set TestSelectionQuery='*'
set LogLevel=Low
set OverwriteRunningDeployment=False

:BeginArgs
set Arg=%~1
if "%Arg%" == "" goto :EndArgs
if /i "%Arg%" == "/?" goto :Usage
if /i "%Arg:~0,15%" == "/subscriptionId" set SubscriptionId=%Arg:~16%
if /i "%Arg:~0,25%" == "/managementCertThumbprint" set ManagementCertThumbprint=%Arg:~26%
if /i "%Arg:~0,14%" == "/hostedService" set HostedService=%Arg:~15%
if /i "%Arg:~0,15%" == "/storageAccount" set StorageAccount=%Arg:~16%
if /i "%Arg:~0,11%" == "/storageKey" set StorageKey=%Arg:~12%
if /i "%Arg:~0,22%" == "/clusterDeploymentOnly" set TestSelectionQuery='*ClusterDeployment*'
if /i "%Arg:~0,22%" == "/enableDetailedLogging" set LogLevel=High
if /i "%Arg:~0,27%" == "/overwriteRunningDeployment" set OverwriteRunningDeployment=True

shift /1
goto :BeginArgs
:EndArgs

:ArgValidation

if "%SubscriptionId%" == "" (
    echo SubscriptionId is not specified. 
    goto :Usage
)

if "%ManagementCertThumbprint%" == "" (
    echo ManagementCertThumbprint is not specified. 
    goto :Usage
)

if "%HostedService%" == "" (
    echo HostedService is not specified. 
    goto :Usage
) 

if "%StorageAccount%" == "" (
    echo StorageAccount is not specified. 
    goto :Usage
) 

if "%StorageKey%" == "" (
    echo StorageKey is not specified. 
    goto :Usage
) 

goto :RunTests

:: ===========================================================================
:: Run Tests

:RunTests

pushd %AzureTestDir%
%TaefBinariesDir%\te.exe AzureEndToEnd.Test.dll /coloredConsoleOutput:true /sessionTimeout:10.1 /testTimeout:10.1 /enableWttLogging /logFile:AzureEndtoEnd.log /logOutput:%LogLevel% /p:SubscriptionId=%SubscriptionId% /p:ManagementCertThumbprint=%ManagementCertThumbprint% /p:HostedService=%HostedService% /p:StorageAccount=%StorageAccount% /p:StorageAccountKey=%StorageKey% /p:OverwriteRunningDeployment=%OverwriteRunningDeployment% /select:@name=%TestSelectionQuery%
popd

endlocal
goto :EOF

:: ===========================================================================
:: Show the usage 
::

:Usage
echo.
echo This script runs Windows Fabric on Windows Azure CITs through TAEF 
echo.
echo Usage: RunAzureTests.cmd [options]
echo Options:
echo /SubscriptionId:^<SubscriptionId^>
echo   Mandatory. Specifies ID of the Windows Azure subscription. 
echo /ManagementCertThumbprint:^<ManagementCertThumbprint^>
echo   Mandatory. Specifies thumbprint of management cert which has been installed locally under CurrentUser\My.
echo /HostedService:^<HostedServiceName^>
echo   Mandatory. Specifies name of hosted service the test is to be deployed to.
echo /StorageAccount:^<StorageAccount^>
echo   Mandatory. Specifies name of Windows Azure storage account for image store.
echo /StorageKey:^<StorageKey^>
echo   Mandatory. Specifies key of Windows Azure storage account for image store.
echo /OverwriteRunningDeployment
echo   Optional. If a deployment exists, overwrite it. If the option is not specified, the test would not proceed upon an existing deployment. 
echo /ClusterDeploymentOnly
echo   Optional. Run ClusterDeploymentTest only.
echo /EnableDetailedLogging
echo   Optional. Enable detailed logging.

endlocal
goto :EOF
