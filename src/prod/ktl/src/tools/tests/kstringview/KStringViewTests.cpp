/*++

Copyright (c) Microsoft Corporation

Module Name:

    KStringViewTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KStringViewTests.h.
    2. Add an entry to the gs_KuTestCases table in KStringViewTestCases.cpp.
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
#include "KStringViewTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

KAllocator* g_Allocator = nullptr;

#if KTL_USER_MODE
extern volatile LONGLONG gs_AllocsRemaining;
#endif

// {77D752E7-213D-406F-848C-A73B3E0A33D2}
static const GUID TestGUID =
{ 0x77d752e7, 0x213d, 0x406f, { 0x84, 0x8c, 0xa7, 0x3b, 0x3e, 0xa, 0x33, 0xd2 } };



// Generate Ansi version of unit tests
#define K$AnsiStringTarget
#include "KStringViewTests.proto.cpp"

// Generate WCHAR version of unit tests
#undef K$AnsiStringTarget
#include "KStringViewTests.proto.cpp"



NTSTATUS
TestSequence()
{
    NTSTATUS Status;

    CompareEdgeTests();
    CompareEdgeTestsA();

    Status = DynReplace();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = DynReplaceA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = StripWhitespace();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = StripWhitespaceA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = PrependTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = PrependTestsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KWStringInterop();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = UnencodeEscapeTest();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = UnencodeEscapeTestA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ConstructorsTest();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ConstructorsTestA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    Status = SearchTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = SearchTestsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = InsertRemoveConcat();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = InsertRemoveConcatA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = FindAndMatchTest();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = FindAndMatchTestA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = GuidConvert();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = GuidConvertA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = LeftRightSub();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = LeftRightSubA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Conversions();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ConversionsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Conversions2();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Conversions2A();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Tokenizer();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = TokenizerA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KLocalStringTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KLocalStringTestsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KDynStringTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KDynStringTestsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KStringTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KStringTestsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KBufferStringTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KBufferStringTestsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KStringPoolTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KStringPoolTestsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = DateConversions();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = DateConversionsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = AnsiConversion();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = SearchAndReplace();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = SearchAndReplaceA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = UnicodeStringTest();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = MeasureThisTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = MeasureThisTestsA();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
KStringViewTest(
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

    KTestPrintf("KStringViewTest: STARTED\n");

    NTSTATUS Result;
    Result = KtlSystem::Initialize();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

    if (NT_SUCCESS(Result))
    {
       Result = TestSequence();
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

    if (NT_SUCCESS(Result))
    {
        KTestPrintf("Completed. Success.\n");
    }
    else
    {
        KTestPrintf("Completed. Failure.\n");
    }

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KStringViewTest: COMPLETED\n");

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
    CONVERT_TO_ARGS(argc, cargs)
#endif
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KStringViewTest(argc, args);
}
#endif


