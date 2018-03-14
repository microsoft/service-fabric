// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace 
{
    StringLiteral TraceCategory("Assert"); 
}

Assert::DisableDebugBreakInThisScope::DisableDebugBreakInThisScope() 
    : saved_(false)
{
    saved_ = get_DebugBreakEnabled();
    set_DebugBreakEnabled(false);
}

Assert::DisableDebugBreakInThisScope::~DisableDebugBreakInThisScope()
{
    set_DebugBreakEnabled(saved_);
}

Assert::DisableTestAssertInThisScope::DisableTestAssertInThisScope() 
    : saved_(false)
{
    saved_ = get_TestAssertEnabled();
    set_TestAssertEnabled(false);
}

Assert::DisableTestAssertInThisScope::~DisableTestAssertInThisScope()
{
    set_TestAssertEnabled(saved_);
}

void Assert::SetCrashLeasingApplicationCallback(void(*callback) (void))
{
    ::FabricSetCrashLeasingApplicationCallback((void*)callback);
}

bool * Assert::static_TestAssertEnabled()
{
    static bool testAssertEnabled = false;
    return & testAssertEnabled;
}

bool * Assert::static_DebugBreakEnabled()
{
    static bool debugBreakEnabled = true;
    return & debugBreakEnabled;
}

bool * Assert::static_StackTraceCaptureEnabled()
{
    static bool stackTraceCaptureEnabled = true;
    return &stackTraceCaptureEnabled;
}

void Assert::set_DebugBreakEnabled(bool value)
{
    *Assert::static_DebugBreakEnabled() = value;
}

void Assert::set_TestAssertEnabled(bool value)
{
    *Assert::static_TestAssertEnabled() = value;
}

void Assert::set_StackTraceCaptureEnabled(bool value)
{
    *Assert::static_StackTraceCaptureEnabled() = value;
}

bool Assert::get_DebugBreakEnabled()
{
    return *Assert::static_DebugBreakEnabled();
}

bool Assert::get_TestAssertEnabled()
{
    return *Assert::static_TestAssertEnabled();
}

bool Assert::get_StackTraceCaptureEnabled()
{
    return *Assert::static_StackTraceCaptureEnabled();
}

bool Assert::IsDebugBreakEnabled()
{
    return get_DebugBreakEnabled();
}

bool Assert::IsTestAssertEnabled()
{
    return get_TestAssertEnabled();
}

bool Assert::IsStackTraceCaptureEnabled()
{
    return get_StackTraceCaptureEnabled();
}

void Assert::LoadConfiguration(Config & config)
{
    bool debugBreakEnabled = true;
    if (config.ReadUnencryptedConfig<bool>(L"Common", L"DebugBreakEnabled", debugBreakEnabled, true))
    {
        Assert::set_DebugBreakEnabled(debugBreakEnabled);
    }
    
    bool testAssertEnabled = false;
    if (config.ReadUnencryptedConfig<bool>(L"Common", L"TestAssertEnabled", testAssertEnabled, false))
    {
        Assert::set_TestAssertEnabled(testAssertEnabled);
    }

    bool stackTraceCaptureEnabled = true;
    if (config.ReadUnencryptedConfig<bool>(L"Common", L"StackTraceCaptureEnabled", stackTraceCaptureEnabled, true))
    {
        Assert::set_StackTraceCaptureEnabled(stackTraceCaptureEnabled);
    }
}

void Assert::DoTestAssert(string const & message)
{
    if (Assert::IsTestAssertEnabled())
    {
        Assert::DoFailFast(message);
    }
}

void Assert::DoFailFast(string const & message)
{
    // Compilers we use have thread safe static initialization.
    static LONG hasFailFastBeenInvoked = 0;
    void * crashLeasingApplicationPtr = nullptr;
    ::FabricGetCrashLeasingApplicationCallback(&crashLeasingApplicationPtr);
    if (crashLeasingApplicationPtr)
    {
        void (*crashLeasingApplicationCallback) ();
        crashLeasingApplicationCallback = (void(*) (void)) crashLeasingApplicationPtr;
        (*crashLeasingApplicationCallback)();
    }

    string failFastMessage = message;
    wstring stackTraceMessage = L"Stack trace capture disabled. Enable using StackTraceCaptureEnabled configuration";

    if (Assert::IsStackTraceCaptureEnabled())
    {
        Common::StackTrace currentStack;
        currentStack.CaptureCurrentPosition();
        stackTraceMessage = currentStack.ToString();
    }

    GeneralEventSource eventSource;
    eventSource.Assert(failFastMessage, stackTraceMessage);

    if (!Assert::IsDebugBreakEnabled())
    {
        // some CITs disable debug break within a scope to test negative conditions.  These calls should be let through.
        throw std::system_error(microsoft::MakeWindowsErrorCode(ERROR_ASSERTION_FAILURE), failFastMessage);
    }

    if (::IsDebuggerPresent())
    {
        ::DebugBreak();
    }
    else
    {
        // WER may have problems collecting the dump if there are multiple fail fasts.  This is a workaround.
        InterlockedIncrement(&hasFailFastBeenInvoked);

        if (hasFailFastBeenInvoked > 1)
        {
            // Sleep for a limited time and then call fail fast.  We are still calling fail fast in case there's a problem with the first fail fast.
            Trace.WriteWarning(
                TraceCategory,
                "Another assert has already been observed, sleeping 30 minutes");

            Sleep(60*30*1000);
        }

        ::RaiseFailFastException(NULL, NULL, 1);
    }
}

void Assert::CodingError(
    StringLiteral format)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5,
    VariableArgument const & arg6)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5,
    VariableArgument const & arg6,
    VariableArgument const & arg7)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5,
    VariableArgument const & arg6,
    VariableArgument const & arg7,
    VariableArgument const & arg8)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);

    DoFailFast(message);
}

void Assert::CodingError(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5,
    VariableArgument const & arg6,
    VariableArgument const & arg7,
    VariableArgument const & arg8,
    VariableArgument const & arg9)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);

    DoFailFast(message);
}


void Assert::TestAssert(
    StringLiteral format)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5,
    VariableArgument const & arg6)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5,
    VariableArgument const & arg6,
    VariableArgument const & arg7)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5,
    VariableArgument const & arg6,
    VariableArgument const & arg7,
    VariableArgument const & arg8)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);

    DoTestAssert(message);
}

void Assert::TestAssert(
    StringLiteral format,
    VariableArgument const & arg0,
    VariableArgument const & arg1,
    VariableArgument const & arg2,
    VariableArgument const & arg3,
    VariableArgument const & arg4,
    VariableArgument const & arg5,
    VariableArgument const & arg6,
    VariableArgument const & arg7,
    VariableArgument const & arg8,
    VariableArgument const & arg9)
{
    string message;
    StringWriterA writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);

    DoTestAssert(message);
}
