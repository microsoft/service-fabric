/*++

Copyright (c) Microsoft Corporation

Module Name:

    KNetFaultInjectorTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KNetFaultInjectorTests.h.
    2. Add an entry to the gs_KuTestCases table in KNetFaultInjectorTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KNetFaultInjectorTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

// {0FA792A1-E63F-432D-922E-E5283FEE78F1}
static GUID TestGuid1 =
{ 0xfa792a1, 0xe63f, 0x432d, { 0x92, 0x2e, 0xe5, 0x28, 0x3f, 0xee, 0x78, 0xf1 } };

// {0FA792A1-E63F-432D-922E-E5283FEE78F2}
static GUID TestGuid2 =
{ 0xfa792a1, 0xe63f, 0x432d, { 0x92, 0x2e, 0xe5, 0x28, 0x3f, 0xee, 0x78, 0xf2 } };


// {0FA792A1-E63F-432D-922E-E5283FEE78F3}
static GUID TestGuid3 =
{ 0xfa792a1, 0xe63f, 0x432d, { 0x92, 0x2e, 0xe5, 0x28, 0x3f, 0xee, 0x78, 0xf3 } };


NTSTATUS
Test1(
    __in KNetFaultInjector* Faulter
    )
{
    KTestPrintf("Test1()\n");

    // Test for drop given message type & destination uri

    KWString Uri1(KtlSystem::GlobalNonPagedAllocator(), L"local/Test1");

    Faulter->RegisterMessageDrop(Uri1, &TestGuid1, 2); // Every other message
    Faulter->Activate(0, 1000);

    KWString TestUri(KtlSystem::GlobalNonPagedAllocator(), L"local/Test1");

    ULONG Delay = 0;
    KWString Uri(KtlSystem::GlobalNonPagedAllocator(), L"local/Test1");
    GUID MessageId = TestGuid1;

    NTSTATUS Res = Faulter->TestMessage(Uri, MessageId, Delay);

    // First call should not trip a drop, beacause of the Ratio of 2
    //
    if (Res != STATUS_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Faulter->TestMessage(Uri, MessageId, Delay);
    if (Res != STATUS_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }


    Res =  Faulter->TestMessage(Uri, MessageId, Delay);
    if (Res != STATUS_CANCELLED)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // And now it should go through again
    Res =  Faulter->TestMessage(Uri, MessageId, Delay);
    if (Res != STATUS_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Faulter->Deactivate();
    Faulter->UnregisterAll();

    return STATUS_SUCCESS;
}

NTSTATUS
Test2(
    __in KNetFaultInjector* Faulter
    )
{
    KTestPrintf("Test2()\n");

    // Test for delay given message type, any URI


    KWString Uri1(KtlSystem::GlobalNonPagedAllocator());

    Faulter->RegisterMessageDrop(Uri1, &TestGuid1, 2); // Every other message
    Faulter->Activate(0, 1000);

    KWString TestUri(KtlSystem::GlobalNonPagedAllocator(), L"local/Test1");

    ULONG Delay = 0;
    KWString Uri(KtlSystem::GlobalNonPagedAllocator(), L"local/Test1");
    KWString Uri2(KtlSystem::GlobalNonPagedAllocator(), L"other");
    GUID MessageId = TestGuid1;

    // First 2 calls should not trip a drop, beacause of the Ratio of 2:1
    //
    NTSTATUS Res = Faulter->TestMessage(Uri, MessageId, Delay);
    if (Res != STATUS_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res =  Faulter->TestMessage(Uri2, MessageId, Delay);
    if (Res != STATUS_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }


    Res =  Faulter->TestMessage(Uri2, MessageId, Delay);
    if (Res != STATUS_CANCELLED)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // And now it should go through again
    Res =  Faulter->TestMessage(Uri2, MessageId, Delay);
    if (Res != STATUS_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Faulter->Deactivate();
    Faulter->UnregisterAll();

    return STATUS_SUCCESS;
}





NTSTATUS
Test3(
    __in KNetFaultInjector* Faulter
    )
{
    KTestPrintf("Test3()\n");

    // Test for drop of a given URI, irrespective of message
    KWString Uri(KtlSystem::GlobalNonPagedAllocator(), L"local/blackhole");

    Faulter->RegisterMessageDrop(Uri, nullptr, 0); // Every message
    Faulter->Activate(0, 1000);

    ULONG Delay = 0;
    KWString TestUri(KtlSystem::GlobalNonPagedAllocator(), L"local/Test1");
    GUID MessageId = TestGuid1;

    NTSTATUS Res = Faulter->TestMessage(TestUri, MessageId, Delay);

    // First call should not trip a drop, beacause of unrelated URI
    //
    if (Res != STATUS_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    TestUri = L"local/blackhole";
    MessageId = TestGuid1;

    Res = Faulter->TestMessage(TestUri, MessageId, Delay);

    // This should drop
    //
    if (Res != STATUS_CANCELLED)
    {
        return STATUS_UNSUCCESSFUL;

    }

    Res = Faulter->TestMessage(TestUri, MessageId, Delay);

    // This should drop
    //
    if (Res != STATUS_CANCELLED)
    {
        return STATUS_UNSUCCESSFUL;

    }

    MessageId = TestGuid2;

    Res = Faulter->TestMessage(TestUri, MessageId, Delay);

    // Should drop anyway because of URI and ratio of 0
    //
    if (Res != STATUS_CANCELLED)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Faulter->Deactivate();
    Faulter->UnregisterAll();    

    return STATUS_SUCCESS;
}



NTSTATUS
Test4(
    __in KNetFaultInjector* Faulter
    )
{
    UNREFERENCED_PARAMETER(Faulter);

    // Test for delay given message type & destination uri
    KTestPrintf("Test4()\n");


    return STATUS_SUCCESS;
}

NTSTATUS
Test5(
    __in KNetFaultInjector* Faulter
    )
{
    UNREFERENCED_PARAMETER(Faulter);

    // Test for delay given message type, any URI

    KTestPrintf("Test5()\n");

    return STATUS_SUCCESS;
}


NTSTATUS
Test6(
    __in KNetFaultInjector* Faulter
    )
{
    KTestPrintf("Test6()\n");

    // Test for drop of a given URI, irrespective of message
    KWString Uri(KtlSystem::GlobalNonPagedAllocator(), L"local/blackhole");

    Faulter->RegisterMessageDelay(Uri, nullptr, 2500, 0); // Every message
    Faulter->Activate(0, 1000);

    ULONG Delay = 0;
    KWString TestUri(KtlSystem::GlobalNonPagedAllocator(), L"local/Test1");
    GUID MessageId = TestGuid1;

    NTSTATUS Res = Faulter->TestMessage(TestUri, MessageId, Delay);

    // First call should not trip a drop, beacause of unrelated URI
    //
    if (Res != STATUS_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    TestUri = L"local/blackhole";
    MessageId = TestGuid1;

    Res = Faulter->TestMessage(TestUri, MessageId, Delay);

    // This should drop
    //
    if (Res != STATUS_WAIT_1 || Delay  != 2500)
    {
        return STATUS_UNSUCCESSFUL;

    }

    Res = Faulter->TestMessage(TestUri, MessageId, Delay);

    // This should drop
    //
    if (Res != STATUS_WAIT_1 || Delay != 2500)
    {
        return STATUS_UNSUCCESSFUL;

    }

    MessageId = TestGuid2;

    Res = Faulter->TestMessage(TestUri, MessageId, Delay);

    // Should drop anyway because of URI and ratio of 0
    //
    if (Res != STATUS_WAIT_1 || Delay != 2500)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Faulter->Deactivate();
    Faulter->UnregisterAll();    

    return STATUS_SUCCESS;
}

NTSTATUS
Test7(
    __in KNetFaultInjector* Faulter
    )
{
    KTestPrintf("Test7()\n");

    Faulter->Randomize(3, 3, 2000);

    return STATUS_SUCCESS;
}



NTSTATUS
SetupFaultTypes(
    __in KNetFaultInjector* Faulter
    )
{
    KWString Uri1(KtlSystem::GlobalNonPagedAllocator(), L"local/Test1");
    KWString Uri2(KtlSystem::GlobalNonPagedAllocator(), L"local/Test2");

    Faulter->RegisterMessageDrop(Uri1, &TestGuid1, 2);
    Faulter->RegisterMessageDelay(Uri2, &TestGuid2, 3000, 3);


    return STATUS_SUCCESS;
}


VOID
DumpStatus(
    NTSTATUS Res,
    ULONG Msec
    )
{
    switch (Res)
    {
        case STATUS_SUCCESS: KTestPrintf("No impact\n"); break;
        case STATUS_WAIT_1:  KTestPrintf("Delay by %u\n", Msec); break;
        case STATUS_CANCELLED: KTestPrintf("Cancel the message\n"); break;
        default:
        KTestPrintf("Invalid return code\n");
    }
}

NTSTATUS
VerifyFaultTypes(
    __in KNetFaultInjector* Faulter
    )
{
    KWString Uri(KtlSystem::GlobalNonPagedAllocator(), L"local/Test2");
    GUID MessageId = TestGuid2;

    ULONG Delay = 0;

    NTSTATUS Res = Faulter->TestMessage(Uri, MessageId, Delay);
    DumpStatus(Res, Delay);

    Res = Faulter->TestMessage(Uri, MessageId, Delay);
    DumpStatus(Res, Delay);

    Res = Faulter->TestMessage(Uri, MessageId, Delay);
    DumpStatus(Res, Delay);

    Res = Faulter->TestMessage(Uri, MessageId, Delay);
    DumpStatus(Res, Delay);

    return STATUS_SUCCESS;
}

static KEvent* g_pEvent = nullptr;

VOID
Callback(PVOID Message)
{
    KTestPrintf("Delay value is %u\n", ULONG(ULONGLONG(Message)));
    g_pEvent->SetEvent();
}


NTSTATUS
DelayTest()
{
    KTestPrintf("Entering delay test...\n");

    KEvent ev;
    g_pEvent = &ev;

    _ObjectDelayEngine De(KtlSystem::GlobalNonPagedAllocator(), Callback);

    De.DelayThis(PVOID(33), 2000);

    ev.WaitUntilSet();
    KTestPrintf("Event set - exiting\n");
    return STATUS_SUCCESS;
}


NTSTATUS TestSequence()
{
    NTSTATUS Result;

    DelayTest();

    KUniquePtr<KNetFaultInjector> Faulter = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KNetFaultInjector(KtlSystem::GlobalNonPagedAllocator());
    if (!Faulter)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Result = Test1(Faulter);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test1() failed. 0x%08x\n", Result);
        return Result;
    }

    Result = Test2(Faulter);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test2() failed. 0x%08x\n", Result);        
        return Result;
    }

    Result = Test3(Faulter);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test3() failed. 0x%08x\n", Result);        
        return Result;
    }

    Result = Test6(Faulter);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test6() failed. 0x%08x\n", Result);        
        return Result;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KNetFaultInjectorTest(
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
	
    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KNetFaultInjectorTest test");

    NTSTATUS Result = KtlSystem::Initialize();

    NTSTATUS Status = TestSequence();
    #if CONSOLE_TEST
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KNetFaultInjectorTest() failed. 0x%08x\n", Result);        
    }    
    else
    {
        KTestPrintf("KNetFaultInjectorTest() succeeded\n");
    }
    #endif

    KtlSystem::Shutdown();

    if (KAllocatorSupport::gs_AllocsRemaining != 0)
    {
        KTestPrintf("Outstanding allocations\n");
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    EventUnregisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return Status;
}




#if CONSOLE_TEST
int
main(int argc, WCHAR* args[])
{
   return KNetFaultInjectorTest(argc, args);
}
#endif
