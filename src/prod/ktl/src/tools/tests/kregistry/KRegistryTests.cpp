/*++

Copyright (c) Microsoft Corporation

Module Name:

    KRegistryTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KRegistryTests.h.
    2. Add an entry to the gs_KuTestCases table in KRegistryTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>
#include "KRegistryTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif


void CleanupPreviousRun()
{
    KRegistry Reg(KtlSystem::GlobalNonPagedAllocator());
    Reg.Open(KWString(KtlSystem::GlobalNonPagedAllocator(), L"Software\\Microsoft\\Windows\\CurrentVersion"))   ;

    Reg.DeleteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyString"));
    Reg.DeleteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyULONG"));
    Reg.DeleteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"ExtraLong"));
}

NTSTATUS Test1()
{
    KTestPrintf("KRegistry Test1()\n");

    CleanupPreviousRun();

    KRegistry Reg(KtlSystem::GlobalNonPagedAllocator());
    NTSTATUS Result = Reg.Open(KWString(KtlSystem::GlobalNonPagedAllocator(), L"Software\\Microsoft\\Windows\\CurrentVersion"))   ;

    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Failed with code %d (%x)\n", Result, Result);
        return Result;
    }

    // Try to read a non-existent value

    ULONG Val = 0;

    Result = Reg.ReadValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyULONG"), Val);

    if (Result != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        KTestPrintf("Error in test; the value should not be there\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Now create it
    Val = 33;
    Result = Reg.WriteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyULONG"), Val);

    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Unable to write a ULONG Value to the key\n");
        return STATUS_UNSUCCESSFUL;
    }

    Val = 0;

    // Now write a string value
    KWString StrMyStr(KtlSystem::GlobalNonPagedAllocator(), L"MY_STRING");

    Result = Reg.WriteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyString"), StrMyStr);

    // Now write a ULONGLONG Value
    ULONGLONG Longer = 144;

    Result = Reg.WriteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"ExtraLong"), Longer);

    // Read them all back
    ULONGLONG LongerReceiver  = 0;
    ULONG ValReceiver = 0;
    KWString StrReceiver(KtlSystem::GlobalNonPagedAllocator());

    Result = Reg.ReadValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"ExtraLong"), LongerReceiver);
    Result = Reg.ReadValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyULONG"), ValReceiver);
    Result = Reg.ReadValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyString"), StrReceiver);


    if (LongerReceiver != 144)
    {
        KTestPrintf("Failed to retrieve correct ULONGLONG value\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (ValReceiver != 33)
    {
        KTestPrintf("Failed to retrieve correct ULONG value\n");
        return STATUS_UNSUCCESSFUL;
    }

    LONG CmpResult = StrReceiver.CompareTo(L"MY_STRING");

    if (CmpResult != 0)
    {
        KTestPrintf("Failed to retrieve correct KWString value\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Delete them all

    Reg.DeleteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyString"));
    Reg.DeleteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"MyULONG"));
    Reg.DeleteValue(KWString(KtlSystem::GlobalNonPagedAllocator(), L"ExtraLong"));

    KTestPrintf("KRegistry Test1(): COMPLETED\n");
    return Result;
}


NTSTATUS TestSequence()
{
    NTSTATUS Result = 0;
    LONGLONG InitialAllocations = KAllocatorSupport::gs_AllocsRemaining;

    KTestPrintf("Starting TestSequence...\n");

    Result = Test1();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test1() failure\n");
        return Result;
    }

    if (KAllocatorSupport::gs_AllocsRemaining != InitialAllocations)
    {
        KTestPrintf("Outstanding allocations\n");
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    KTestPrintf("SUCCESS\n");
    return STATUS_SUCCESS;
}


NTSTATUS
KRegistryTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    KTestPrintf("KRegistryTest: STARTED\n");

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KRegistryTest test");

    NTSTATUS Result = KtlSystem::Initialize();
    KFatal(NT_SUCCESS(Result));

    Result = TestSequence();

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KRegistryTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return Result;
}

#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    WCHAR** args = new WCHAR *[argc]();
    auto wargs = new std::vector<std::wstring>(argc);
    for (int arg_iter = 0; arg_iter < argc; arg_iter++)
    {
        (*wargs)[arg_iter] = Utf8To16(cargs[arg_iter]);
        args[arg_iter] = (WCHAR*)((*wargs)[arg_iter].data());
    }
#endif
    return KRegistryTest(argc, args);
}
#endif
