@set @__GenerateServiceModelFile=1 /* [[ Begin batch script
REM ------------------------------------------------------------
REM Copyright (c) Microsoft Corporation.  All rights reserved.
REM Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
REM ------------------------------------------------------------
@echo off
set @__GenerateServiceModelFile=
setlocal

set DestPath=
set DestFile=
set FabricRoot=
:BeginArgs
set Arg=%~1
if "%Arg%" == "" goto :EndArgs
if /i "%Arg%" == "/?" goto :Usage
if /i "%Arg:~0,9%" == "/destPath" set DestPath=%Arg:~10%
if /i "%Arg:~0,9%" == "/destFile" set DestFile=%Arg:~10%
if /i "%Arg:~0,11%" == "/fabricRoot" set FabricRoot=%Arg:~12%

shift /1
goto :BeginArgs
:EndArgs

set SchemaFile=%FabricRoot%\src\ServiceModel\xsd\ServiceFabricServiceModel.xsd
cd %~dp0
set XsdGeneratedServiceModelFile=ServiceFabricServiceModel.cs

:: Generate a new schema file...
call xsd.exe %SchemaFile% /c /o:%DestPath% /n:MS.Test.WinFabric.Common
cscript.exe //nologo //E:JSCRIPT "%~dpnx0" "FixCLSCompliant" %DestPath%\%XsdGeneratedServiceModelFile% %DestPath%\%DestFile%

goto :eof

:Usage
echo Regenerates the generated service model from the management xsd.
echo.
echo Usage: %~nx0 [options]
echo.
echo Options:
echo  /destPath:^<destination path^>
echo    The destination path for generated files
echo  /destFile:^<destination file^>
echo    The name of the generated file
goto :eof

REM End batch script ]]
*/
// [[ Begin JScript

function Main()
{
    var mode = WScript.Arguments(0);
    if (mode === "FixCLSCompliant")
    {
        var fileToFix = WScript.Arguments(1);
        var fileToWrite = WScript.Arguments(2);
        FixCLSCompliant(fileToFix, fileToWrite);
    }
}

function FixCLSCompliant(fileToFix, fileToWrite)
{
    var fso = new ActiveXObject("Scripting.FileSystemObject");
    var input = fso.OpenTextFile(fileToFix, 1, true);

    var content = input.ReadAll();
    input.Close();

    content = content.replace(/public uint DefaultLoad/g, "[System.CLSCompliant(false)]public uint DefaultLoad");
    content = content.replace(/public uint PrimaryDefaultLoad/g, "[System.CLSCompliant(false)]public uint PrimaryDefaultLoad");
    content = content.replace(/public uint SecondaryDefaultLoad/g, "[System.CLSCompliant(false)]public uint SecondaryDefaultLoad");

    var output = fso.OpenTextFile(fileToWrite, 2, true);
    output.Write(content);
    output.Close();
}

try
{
    Main();
}
catch (e)
{
    WScript.Echo(e.description);
    WScript.Quit(e.number & 0xFFFF);
}

// End JScript ]]