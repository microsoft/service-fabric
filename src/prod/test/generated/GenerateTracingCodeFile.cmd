@set @__GenerateTracingCodeFile=1 /* [[ Begin batch script
REM ------------------------------------------------------------
REM Copyright (c) Microsoft Corporation.  All rights reserved.
REM Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
REM ------------------------------------------------------------

@echo off
set @__GenerateTracingCodeFile=
setlocal

set TOOL_NAME=%~nx0
if "%1" == "/?" goto :Usage

echo //------------------------------------------------------------
echo // Copyright (c) Microsoft Corporation.  All rights reserved.
echo //------------------------------------------------------------
echo.
echo namespace MS.Test.WinFabric.Common.Events
echo {
echo     using System;
echo     using System.CodeDom.Compiler;
echo     using System.Diagnostics.CodeAnalysis;
echo.
echo     [SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Justification = "Matches underlying data type")]
echo     [CLSCompliant(false)]
echo     [GeneratedCode("%TOOL_NAME%", "1.0")]
echo     public enum WinFabricStructuredTracingCode : ushort
echo     {
echo         None = 0,

cscript.exe //nologo //E:JSCRIPT "%~dpnx0" %1

echo     }
echo }

goto :eof

:Usage
echo %TOOL_NAME% [manifest-file]
echo.
echo Outputs a C# source file containing structured tracing codes from the provided manifest.
goto :eof

REM End batch script ]]
*/
// [[ Begin JScript

var document = new ActiveXObject("Msxml2.DOMDocument.3.0");
document.preserveWhiteSpace = true;
document.async = false;
document.load(WScript.Arguments(0));

var error = document.parseError;
if (error.errorCode != 0)
{
    var message = "An error occurred while loading '" + file + "': ";
    var location =  "(" + error.line + ":" + error.linepos + ") ";
    throw new Error(error.errorCode, message + location + error.reason);
}

document.setProperty("SelectionLanguage", "XPath");
var assemblyNamespace = "xmlns:a='urn:schemas-microsoft-com:asm.v3'";
var eventsNamespace = "xmlns:e='http://schemas.microsoft.com/win/2004/08/events'";
document.setProperty("SelectionNamespaces", assemblyNamespace + " " + eventsNamespace);
var eventNodes = document.selectNodes("/a:assembly/a:instrumentation/e:events/e:provider/e:events/e:event");

for (var i = 0; i < eventNodes.length; ++i)
{
    var eventNode = eventNodes.item(i);
    var eventName = eventNode.attributes.getNamedItem("symbol").value;
    var eventId = eventNode.attributes.getNamedItem("value").value;
    WScript.Echo("        " + eventName + " = " + eventId + ",");
}

// End JScript ]]