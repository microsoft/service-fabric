/*++

Copyright (c) Microsoft Corporation

Module Name:

    KPerfCounterTests.cpp

Abstract:

See this link for perf counter types:

    http://technet.microsoft.com/en-us/library/cc785636(WS.10).aspx

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
#include "KPerfCounterTests.h"
#include <CommandLineParser.h>

KAllocator* g_Allocator = nullptr;

#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif


// Provider & Counter Set GUIDs
//
// Because of manifest registration conflicts, user & kernel cannot have the same GUIDs
//
#if KTL_USER_MODE

// {FFFFFFFF-F41B-42e6-8D58-84FA32174792}
static const GUID GUID_RVD_TEST_PROVIDER =
{ 0xFFFFFFFF, 0xf41b, 0x42e6, { 0x8d, 0x58, 0x84, 0xfa, 0x32, 0x17, 0x47, 0x92 } };


// {12F98692-8B08-408c-A93B-BB582673DCFA}
static const GUID GUID_SINGLETON_TEST =
{ 0x12f98692, 0x8b08, 0x408c, { 0xa9, 0x3b, 0xbb, 0x58, 0x26, 0x73, 0xdc, 0xfa } };

// {76A0AFC1-F41B-42e6-8D58-84FA32174791}
static const GUID GUID_MULTI_INST_TEST =
{ 0x76a0afc1, 0xf41b, 0x42e6, { 0x8d, 0x58, 0x84, 0xfa, 0x32, 0x17, 0x47, 0x91 } };

#else

// {6041B507-3C86-40fd-93CD-CDF762487E17}
static const GUID GUID_RVD_TEST_PROVIDER =
{ 0x6041b507, 0x3c86, 0x40fd, { 0x93, 0xcd, 0xcd, 0xf7, 0x62, 0x48, 0x7e, 0x17 } };

// {F1930498-93EE-431d-A9A9-E48CC7BC4FCC}
static const GUID GUID_SINGLETON_TEST =
{ 0xf1930498, 0x93ee, 0x431d, { 0xa9, 0xa9, 0xe4, 0x8c, 0xc7, 0xbc, 0x4f, 0xcc } };


// {DF9175F2-20A6-4bc3-85E5-A71E14B28BB5}
static const GUID GUID_MULTI_INST_TEST =
{ 0xdf9175f2, 0x20a6, 0x4bc3, { 0x85, 0xe5, 0xa7, 0x1e, 0x14, 0xb2, 0x8b, 0xb5 } };

#endif



NTSTATUS
DoCounters(
    __in KPerfCounterSetInstance::SPtr Singleton,
    __in KPerfCounterSetInstance::SPtr MultiOne,
    __in KPerfCounterSetInstance::SPtr MultiTwo
    )
{
    // Singleton values

    ULONG s1 = 10;
    ULONGLONG s2 = 20;
    ULONG s3 = 30;


    // Multi instance values

    ULONGLONG m1_v1 = 15;
    ULONG     m1_v2 = 25;
    ULONG     m1_v3 = 35;
    ULONGLONG m1_v4 = 45;

    ULONGLONG m2_v1 = 50;
    ULONG     m2_v2 = 60;
    ULONG     m2_v3 = 70;
    ULONGLONG m2_v4 = 80;


    // Map them

    Singleton->SetCounterAddress(1, &s1);
    Singleton->SetCounterAddress(2, &s2);
    Singleton->SetCounterAddress(3, &s3);


    MultiOne->SetCounterAddress(1, &m1_v1);
    MultiOne->SetCounterAddress(2, &m1_v2);
    MultiOne->SetCounterAddress(3, &m1_v3);
    MultiOne->SetCounterAddress(4, &m1_v4);

    MultiTwo->SetCounterAddress(1, &m2_v1);
    MultiTwo->SetCounterAddress(2, &m2_v2);
    MultiTwo->SetCounterAddress(3, &m2_v3);
    MultiTwo->SetCounterAddress(4, &m2_v4);


    Singleton->Start();


    MultiOne->Start();
    MultiTwo->Start();


    // Run a simple loop for a minute to watch them

    ULONG Cycles = 10;

    while (Cycles--)
    {
        KNt::Sleep(5000);

        s1 += 1;
        s2 += 1;
        s3 += 1;

        m1_v1 += 10;
        m1_v2 += 11;
        m1_v3 += 12;
        m1_v4 += 13;

        m2_v1++;
        m2_v2++;
        m2_v3++;
        m2_v4++;

    }


    Singleton->Stop();
    MultiOne->Stop();
    MultiTwo->Stop();

    KNt::Sleep(10000);

    Singleton->Start();
    MultiTwo->Start();

    Cycles = 3;
    while (Cycles--)
    {
        KNt::Sleep(5000);

        s1 += 1;
        s2 += 1;
        s3 += 1;

        m2_v1++;
        m2_v2++;
        m2_v3++;
        m2_v4++;
    }


    return STATUS_SUCCESS;
}


NTSTATUS
BaseTest(KPerfCounterManager::SPtr Mgr)
{
    NTSTATUS Res;
    KPerfCounterSet::SPtr Singleton;
    KPerfCounterSet::SPtr Multi;

    // Create two distinct counter set definitions.
    //
    Res = Mgr->CreatePerfCounterSet(
        KWString(*g_Allocator, L"RVD Kernel Singleton"),
        GUID_SINGLETON_TEST,
        KPerfCounterSet::CounterSetSingleton,
        Singleton
        );

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Mgr->CreatePerfCounterSet(
        KWString(*g_Allocator, L"RVD Kernel Multi"),
        GUID_MULTI_INST_TEST,
        KPerfCounterSet::CounterSetMultiInstance,
        Multi
        );

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Define the counters
    //

    Res = Singleton->DefineCounter(1, KPerfCounterSet::Type_ULONG);
    Res = Singleton->DefineCounter(2, KPerfCounterSet::Type_ULONGLONG);
    Res = Singleton->DefineCounter(3, KPerfCounterSet::Type_ULONG);

    Res = Singleton->CommitDefinition();   // Lock the definition in place; no more changes

    Res = Multi->DefineCounter(1,  KPerfCounterSet::Type_ULONGLONG);
    Res = Multi->DefineCounter(2,  KPerfCounterSet::Type_ULONG);
    Res = Multi->DefineCounter(3,  KPerfCounterSet::Type_ULONG);
    Res = Multi->DefineCounter(4,  KPerfCounterSet::Type_ULONGLONG);

    Res = Multi->CommitDefinition();

    for (ULONG i = 0; i < 5; i++)
    {
        // Spawn Instances

        KPerfCounterSetInstance::SPtr SingletonInst;
        KPerfCounterSetInstance::SPtr MultiOne;
        KPerfCounterSetInstance::SPtr MultiTwo;

        Singleton->SpawnInstance(nullptr, SingletonInst);

        Multi->SpawnInstance(L"TestInst_1", MultiOne);
        Multi->SpawnInstance(L"TestInst_2", MultiTwo);

        // Run a loop to update the values for a while so we can see them in perfmon
        //
        DoCounters(SingletonInst, MultiOne, MultiTwo);
        KTestPrintf("Loop %d completed\n", i);
    }

    return STATUS_SUCCESS;

}


NTSTATUS
TestSequence()
{
    NTSTATUS Res;
    KPerfCounterManager::SPtr Mgr;

    Res = KPerfCounterManager::Create(GUID_RVD_TEST_PROVIDER, *g_Allocator, Mgr);

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    BaseTest(Mgr);
    return STATUS_SUCCESS;
}

NTSTATUS
KPerfCounterTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS Result;

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif

    EventRegisterMicrosoft_Windows_KTL();

    Result = KtlSystem::Initialize();

    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }

    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    if (NT_SUCCESS(Result))
    {
       Result = TestSequence();
    }

    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test failure\n");
    }

    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - StartingAllocs;
    if (Leaks)
    {
        KTestPrintf("Leaks = %u\n", Leaks);
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }
    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return Result;
}

#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
wmain(int argc, WCHAR* args[])
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
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KPerfCounterTest(argc, args);
}
#endif

