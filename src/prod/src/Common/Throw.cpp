// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/StringUtility.h"

#include "Common/StackTrace.h"
#include "Common/Throw.h"

namespace Common
{
    StringLiteral TraceType = "throw";

    __declspec(nothrow) DWORD __stdcall DebugThread(void *p) 
    {
        //
        // volatile here is just an attempt to make compiler
        // not to optimize out this variable so it would be 
        // easier to debug
        // 
        volatile void * assertingThreadId = p;
        UNREFERENCED_PARAMETER(assertingThreadId);
        DebugBreak(); //break in debugger or create crash dump

        // TODO: determine proper error codes to use in the assert statements
        TerminateProcess(GetCurrentProcess(), ERROR_ASSERTION_FAILURE);
        return 0;
    }

    __declspec(noreturn) void fail_coding_error(char const * message)
    {
        //
        // Fail with a coding error - coding errors indicate that the system is in an unrecoverable state and MUST crash in production.  Since the system is corrupted, only literal
        // strings may be used.  Note that the memory to which message points shall not be stack allocated.
        //

        try
        {
            Trace.WriteError(TraceType, "coding error {0}", message);
            StackTrace currentStack;
            currentStack.CaptureCurrentPosition();
            Trace.WriteError(TraceType, "{0}", currentStack);
        }
        catch(std::exception const&)
        {
            // silently swallowing exceptions to ensure nothing interferes with final behavior
        }
        throw std::system_error(microsoft::MakeWindowsErrorCode(ERROR_ASSERTION_FAILURE), message);
    }

    __declspec(noreturn) void fail_coding_error(std::string const & message)
    {
        fail_coding_error(message.c_str());
    }

    __declspec(noreturn) void fail_coding_error(std::wstring const & message)
    {
        std::string ansiVersion;
        try
        {
            StringUtility::UnicodeToAnsi(message, ansiVersion);
        }
        catch(std::exception const&)
        {
             // silently swallowing exceptions to ensure nothing interferes with final behavior
        }

        fail_coding_error(ansiVersion);
    }

    void LogStackTraceText(StackTrace const & currentStack)
    {        
        std::wstring stackText;
        StringWriter writer(stackText);
        writer << currentStack;
        Trace.WriteError(TraceType, "{0}", stackText);
    }

    __declspec(noreturn) void throw_system_error(char const * message, std::error_code error)
    {
        try
        {
            Trace.WriteError(TraceType, "system error {0} {1}", message, error);
            StackTrace currentStack;
            currentStack.CaptureCurrentPosition();
            LogStackTraceText(currentStack);   
        }
        catch(std::exception const&)
        {
            // silently swallowing exceptions to ensure nothing interferes with final behavior
        }

        throw std::system_error(error, message);
    }

    __declspec(noreturn) void throw_system_error(std::string const & message, std::error_code error)
    {
        throw_system_error(message.c_str(), error);
    }

    __declspec(noreturn) void throw_system_error(std::wstring const & message, std::error_code error)
    {    
        std::string ansiVersion;
        try
        {
            StringUtility::UnicodeToAnsi(message, ansiVersion);
        }
        catch(std::exception const&)
        {
             // silently swallowing exceptions to ensure nothing interferes with final behavior
        }

        throw_system_error(ansiVersion, error);
    }

    __declspec(nothrow) void log_system_error(char const * message, std::error_code error)
    {
        try
        {
            Trace.WriteWarning(TraceType, "system error {0} {1}", std::string(message), error);
            StackTrace currentStack;
            currentStack.CaptureCurrentPosition();
            LogStackTraceText(currentStack);
        }
        catch(std::exception const&)
        {
            // silently swallowing exceptions to ensure nothing interferes with final behavior
        }
    }    
}
