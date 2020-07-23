@echo off
if "%1"=="" GOTO USAGE

REM Debug build is the default build type
set _buildType=debug-amd64

if "%2"=="retail" (
set _buildType=retail-amd64
)

setlocal
set __NugetPath=%USERPROFILE%\.nuget\packages\microsoft.servicefabric.reliablecollectiontestinterop\%1\runtimes\win10-x64\native

@echo "Copying reliable collection native files of build type %_buildType% to %__NugetPath%..."
copy %REPOROOT%\out\%_buildType%\bin\FabricDrop\bin\Fabric\Fabric.Code\FabricCommon.dll %__NugetPath% /Y
copy %REPOROOT%\out\%_buildType%\bin\FabricDrop\bin\Fabric\Fabric.Code\FabricResources.dll %__NugetPath% /Y
copy %REPOROOT%\out\%_buildType%\ReliableCollectionRuntime\ReliableCollectionRuntime.dll %__NugetPath% /Y
copy %REPOROOT%\out\%_buildType%\symbols\ReliableCollectionRuntime.pdb %__NugetPath% /Y
copy %REPOROOT%\out\%_buildType%\ReliableCollectionServiceStandalone\ReliableCollectionServiceStandalone.dll %__NugetPath% /Y
copy %REPOROOT%\out\%_buildType%\ReliableCollectionServiceStandalone\ReliableCollectionServiceStandalone.pdb %__NugetPath% /Y
GOTO END

:USAGE
@echo UpdateNuget.bat nuget-version [BuildType]
@echo BuildType : Type of build. Default is debug build. Use retail for release build.

:END

