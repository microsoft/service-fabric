/*++

Copyright (c) Microsoft Corporation

Module Name:

    ProcessAffinityTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in ProcessAffinityTests.h.
    2. Add an entry to the gs_KuTestCases table in ProcessAffinityTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "ProcessAffinityTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif


class DoNothing : public KAsyncContextBase
{
    K_FORCE_SHARED(DoNothing);

    public:
        static NTSTATUS Create(__out DoNothing::SPtr& Context,
                                __in KAllocator& Allocator,
                                __in ULONG AllocationTag);

        VOID StartIt(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr    
                    );

    protected:
        
        VOID
        OnStart() override;
};

DoNothing::DoNothing()
{
}

DoNothing::~DoNothing()
{
}

NTSTATUS DoNothing::Create(__out DoNothing::SPtr& Context,
                                __in KAllocator& Allocator,
                                __in ULONG AllocationTag)
{
    NTSTATUS status;
    DoNothing::SPtr context;

    context = _new(AllocationTag, Allocator) DoNothing();
    if (! context)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context;
    
    return(status);
}

VOID DoNothing::OnStart()
{
    Complete(STATUS_SUCCESS);
}

VOID DoNothing::StartIt(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr    
    )
{
    Start(ParentAsyncContext, CallbackPtr);
}

NTSTATUS Test1()
{
    NTSTATUS status = STATUS_SUCCESS;
    const ULONG count = 256;
    DoNothing::SPtr doNothing[count];
    KSynchronizer sync[count];

    KTestPrintf("ProcessAffinity Test1()\n");

    for (ULONG i = 0; i < count; i++)
    {
        status = DoNothing::Create(doNothing[i], KtlSystem::GlobalNonPagedAllocator(), 72);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("DoNothing::Create(%d) failed %x\n", i, status);
            return(status);
        }
    }

    for (ULONG i = 0; i < count; i++)
    {
        doNothing[i]->StartIt(NULL, sync[i]);
    }

    for (ULONG i = 0; i < count; i++)
    {
        status = sync[i].WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("WaitForCompletion(%d) failed %x\n", i, status);
            return(status);
        }
    }

    return(status);
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

    // Sleep to give time for asyncs to be cleaned up before allocation
    // check. It requires a context switch and a little time on UP machines.
    KNt::Sleep(500);

    LONGLONG FinalAllocations = KAllocatorSupport::gs_AllocsRemaining;
    if (FinalAllocations != InitialAllocations)
    {
        KTestPrintf("Outstanding allocations.  Expected: %d.  Actual: %d\n", InitialAllocations, FinalAllocations);
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
ProcessAffinityTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    ULONG i;    
    BOOL b;
    DWORD_PTR processMask, systemMask;

    KTestPrintf("NOTE: This must be run as an admin\n");

    //
    // Restrict process affinity to a single proc.
    //
    b = GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask);
    if (! b)
    {
        KTestPrintf("GetProcessAffinityMask failed %d\n", GetLastError());
        return(STATUS_UNSUCCESSFUL);
    }

    KTestPrintf("System mask is %x, Process mask is %x\n", systemMask, processMask);

    for (i = 0; i < 32; i++)
    {
        DWORD_PTR mask = (DWORD_PTR)((DWORD_PTR)1 << (DWORD_PTR)i);
        if (systemMask & mask)
        {
            b = SetProcessAffinityMask(GetCurrentProcess(), mask);
            if (! b)
            {
                KTestPrintf("SetProcessAffinityMask(%x) failed %d\n", mask, GetLastError());
                return(STATUS_UNSUCCESSFUL);
            }
            
            KTestPrintf("SetProcessAffinityMask(%x) succeeded\n", mask);
            
            break;
        }
    }

    if (i == 32)
    {
        KTestPrintf("Could not find processor to mask\n");
        return(STATUS_UNSUCCESSFUL);
    }

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting ProcessAffinityTest test");

    KtlSystem* system = nullptr;
    NTSTATUS Result = KtlSystem::Initialize(&system);
    KFatal(NT_SUCCESS(Result));

    system->SetStrictAllocationChecks(TRUE);

    Result = TestSequence();

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();
    return Result;
}

#if CONSOLE_TEST
int
main(int argc, WCHAR* args[])
{
    return ProcessAffinityTest(argc, args);
}
#endif
