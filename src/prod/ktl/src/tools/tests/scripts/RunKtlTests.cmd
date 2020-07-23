:: ------------------------------------------------------------
:: Copyright (c) Microsoft Corporation.  All rights reserved.
:: Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
:: ------------------------------------------------------------

@echo off

set SKIP_KM=false
set SKIP_UM=false
set SKIP_SETUP=false
set Tests=KAllocationBitmap,KAsyncQueue,KBitmap,KBlockFile,KBuffer,KChecksum,KDelegate,KHashTable,KHttp,KInvariant,KIoBuffer,KMemChannel,KNetChannel,KNetFaultInjector,KNetworkEndpoint,KNetwork,KNodeTable,KPerfCounter,KRegistry,KRTT,KSerial,KStringView,KTask,KThreadPool,KThread,KtlCommon,KUri,KVariant,KVolumeNamespace,KWeakRef,KWString,RvdLogger,Xml,KAsyncContext
set _NT_SYMBOL_PATH=%_NTTREE%\Symbols.pri\ktltest;%_NT_SYMBOL_PATH%

:Loop
if [%1]==[] (
    GOTO Continue
) else (
    if "%1"=="/skipkm" (
        echo Skipping Kernel Mode tests
        SET SKIP_KM=true
    ) else if "%1"=="/skipum" (
        echo Skipping User Mode tests
        set SKIP_UM=true
    ) else if "%1"=="/skipsetup" (
        echo Skipping setup tasks
        set SKIP_SETUP=true
    ) else if "%1"=="/tests" (
        set Tests=%~2
        SHIFT
    ) else if "%1"=="/tedir" (
        set TE_DIR=%~2
        SHIFT
    ) else if "%1"=="/bindir" (
        set BINARIES_DIR=%~2
        SHIFT
    ) else (
        echo Unrecognized argument: %1
        GOTO :EOF
    )

    SHIFT
    GOTO Loop
)
:Continue

REM   Validate the TAEF directory
if not "%TE_DIR%"=="" (
    if not exist "%TE_DIR%\te.exe" (
        echo te.exe not found in supplied /tedir: %TE_DIR%
        GOTO :EOF
    )
) else (
    if exist "%_NTTREE%\tools\te.exe" (
        set TE_DIR=%_NTTREE%\tools
    ) else if exist "%BASEDIR%\tools\te.exe" (
        set TE_DIR=%BASEDIR%\tools
    ) else if exist "%BASEDIR%\services\ktl\imports\taef\amd64\x64\TestExecution\te.exe" (
        set TE_DIR=%BASEDIR%\services\ktl\imports\taef\amd64\x64\TestExecution
    ) else (
        echo "taef (te.exe) not found in default locations.  Please specify the location of taef with the /tedir option."
        GOTO :EOF
    )
)
echo Loading TAEF from %TE_DIR%


REM   Validate the binaries directory
if not "%BINARIES_DIR%"=="" (
    if not exist "%BINARIES_DIR%" (
        echo "Supplied binaries dir does not exist: %BINARIES_DIR%"
        GOTO :EOF
    )
) else (
    if exist "%_NTTREE%\KtlTest" (
        set BINARIES_DIR=%_NTTREE%\KtlTest
    ) else (
        echo "Default binaries dir does not exist.  Please build or specify the binaries dir with the /bindir option."
        GOTO :EOF
    )
)


if "%SKIP_SETUP%"=="false" (
    REM  Do setup tasks
    REM  TODO: change tests not to rely on these specific directories/files
    if not exist "%windir%\rvd" (
        md "%windir%\rvd"
    )
    if errorlevel 1 (
        echo "Failed to create rvd directory: %windir%\rvd"
        GOTO :EOF
    )

    if exist "%BASEDIR%\services\winfab\prod\shared\ktl\tools\tests\scripts\hosts.txt" (
        copy "%BASEDIR%\services\winfab\prod\shared\ktl\tools\tests\scripts\hosts.txt" "%windir%\rvd\hosts.txt"
    ) else if exist "%BASEDIR%\services\ktl\prod\ktl\tools\tests\scripts\hosts.txt" (
        copy "%BASEDIR%\services\ktl\prod\ktl\tools\tests\scripts\hosts.txt" "%windir%\rvd\hosts.txt"
    ) else (
        echo "Failed to find hosts.txt necessary to run rvd tests."
        GOTO :EOF
    )
    if errorlevel 1 (
        echo "Failed to copy hosts.txt for rvd tests."
        GOTO :EOF
    )
    echo "Copied hosts.txt to %windir%\rvd for rvd tests"


    if not exist "C:\temp\Ktl\XmlTest" (
        md "C:\temp\Ktl\XmlTest"
    )
    if errorlevel 1 (
        echo "Failed to create xml test directory: C:\temp\Ktl\XmlTest"
        GOTO :EOF
    )

    if exist "%_NTDRIVE%%_NTROOT%\services\winfab\prod\shared\ktl\tools\tests\xml\TestFile0.xml" (
        copy "%_NTDRIVE%%_NTROOT%\services\winfab\prod\shared\ktl\tools\tests\xml\TestFile0.xml" "C:\temp\Ktl\XmlTest\testfile0.xml"
    ) else if exist "%_NTDRIVE%%_NTROOT%\services\ktl\prod\ktl\tools\tests\xml\TestFile0.xml" (
        copy "%_NTDRIVE%%_NTROOT%\services\ktl\prod\ktl\tools\tests\xml\TestFile0.xml" "C:\temp\Ktl\XmlTest\testfile0.xml"
    ) else (
        echo "Failed to find TestFile0.xml necessary to run xml tests."
        GOTO :EOF
    )
    if errorlevel 1 (
        echo "Failed to copy testfile0.xml for xml tests."
        GOTO :EOF
    )
    echo "Copied TestFile0.xml to C:\temp\Ktl\XmlTest for xml tests"
)


echo Executing tests: %Tests%
echo in %BINARIES_DIR%
pushd %BINARIES_DIR%

if "%SKIP_UM%"=="false" (
    for %%i in (%Tests%) do (
        CALL :DoUMTest %%i
        
        if errorlevel 1 (
            GOTO :EndRun
        )
    )
)

if "%SKIP_KM%"=="false" (
    for %%i in (%Tests%) do (
        CALL :DoKMTest %%i
        
        if errorlevel 1 (
            GOTO :EndRun
        )
    )
)

:EndRun
popd
goto :EOF


REM   Given a test name (e.g. KBlockFile) run the user-mode test
:DoUmTest
    echo User mode test for %1:
    echo "CommandLine: %TE_DIR%\te %BINARIES_DIR%\testloader.dll /inproc /p:target=user /p:dll=%1User /p:configfile=Table:%BINARIES_DIR%\%1TestInput.xml#Test /p:c=RunTests"
    "%TE_DIR%\te" "%BINARIES_DIR%\testloader.dll" /inproc /p:target=user /p:dll=%1User /p:configfile=Table:%BINARIES_DIR%\%1TestInput.xml#Test /p:c=RunTests
goto :EOF


REM   Given a test name (e.g. KBlockFile) run the kernel-mode test
:DoKmTest
    echo Kernel mode test for %1:
    echo "CommandLine: %TE_DIR%\te %BINARIES_DIR%\testloader.dll /inproc /p:target=kernel /p:driver=%1Kernel /p:configfile=Table:%BINARIES_DIR%\%1TestInput.xml#Test /p:c=RunTests"
    "%TE_DIR%\te" "%BINARIES_DIR%\testloader.dll" /inproc /p:target=kernel /p:driver=%1Kernel /p:configfile=Table:%BINARIES_DIR%\%1TestInput.xml#Test /p:c=RunTests 
goto :EOF