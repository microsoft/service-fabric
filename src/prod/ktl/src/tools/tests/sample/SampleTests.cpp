/*++

Copyright (c) Microsoft Corporation

Module Name:

    SampleTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in SampleTests.h.
    2. Add an entry to the gs_KuTestCases table in SampleTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
        this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#include "SampleTests.h"
#include <CommandLineParser.h>

NTSTATUS
SamplePass(
    int argc, WCHAR* args[]
    )
{
    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting samplepass test");

    NTSTATUS status = KtlSystem::Initialize();
    KFatal(NT_SUCCESS(status));

    CmdLineParser Parser(KtlSystem::GlobalNonPagedAllocator());
    BOOLEAN Res = Parser.Parse(argc -1, &args[1]);
    
    if (! Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG i = 0; i < Parser.ParameterCount(); i++)
    {
       Parameter* param = Parser.GetParam(i);
       KTestPrintf("Parameter name = %S, Value Count = %u\n", param->_Name, param->_ValueCount);
       for (ULONG ValueIndex = 0; ValueIndex < param->_ValueCount; ValueIndex++)
       {
           KTestPrintf("Value[%u]: %S\n", ValueIndex, param->_Values[ValueIndex]);
       }
    }

    KtlSystem::Shutdown();
    return STATUS_SUCCESS;
}

NTSTATUS
SampleFail(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    if(FALSE)
    {
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
SampleSecondFail(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    int i = 5;
    if(i<5)
    {
        return STATUS_SUCCESS;
    }
    else
    {
        i = 6;
    }
    return STATUS_UNSUCCESSFUL;
}

#if CONSOLE_TEST
int
wmain(    int argc, WCHAR* args[] )
{
#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif

    SamplePass( argc, args);
    SampleFail( argc, args);

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  	
}
#endif
