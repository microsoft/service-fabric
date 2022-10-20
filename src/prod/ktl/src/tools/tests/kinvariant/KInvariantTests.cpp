/*++

Copyright (c) Microsoft Corporation

Module Name:

    KInvariant.cpp

Abstract:

    This file contains test case implementations for KInvariant
    functionality

    To add a new unit test case:
    1. Declare your test case routine in KInvariantTests.h.
    2. Add an entry to the gs_KuTestCases table in KInvariantTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <ktl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <strsafe.h>
#include <ktrace.h>
#include "KInvariantTests.h"
#include <CommandLineParser.h>
#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#if KTL_USER_MODE
extern volatile LONGLONG gs_AllocsRemaining;
#endif

#define TextConditionSize 256
#define TextFileSize 512
CHAR KInvariantTextCondition[TextConditionSize];
CHAR KInvariantTextFile[TextFileSize];
ULONG KInvariantTextLine;
BOOLEAN KInvariantBoolean;

KInvariantCalloutType PreviousKInvariantCallout;

BOOLEAN MyKInvariantCallout(
    __in LPCSTR Condition,
    __in LPCSTR File,
    __in ULONG Line
    )
{
    //
    // Remember the info passed
    //
    StringCchCopyA(KInvariantTextCondition, TextConditionSize, Condition);
    StringCchCopyA(KInvariantTextFile, TextFileSize, File);
    KInvariantTextLine = Line;
    
    //
    // Forward on to the previous callout and remember result
    //
    KInvariantBoolean = (*PreviousKInvariantCallout)(Condition, File, Line);

    //
    // Do not cause process to crash - we are testing after all.
    // KInvariant callouts in non-test code should almost NEVER return
    // FALSE.
    //
    return(FALSE);
}

NTSTATUS
KInvariantTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    KTestPrintf("KInvariantTest: STARTED\n");

    NTSTATUS status = STATUS_SUCCESS;

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KInvariantTest test\n");

    KtlSystem* underlyingSystem;
    status = KtlSystem::Initialize(&underlyingSystem);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("%s: %i: KtlSystem::Initialize failed\n", __FUNCTION__, __LINE__);
        return status;
    }

    //
    // Setup our own KInvariant callout to hook the callout
    //
    PreviousKInvariantCallout = SetKInvariantCallout(MyKInvariantCallout);

    //
    // NOTE: This all needs to be on the same line so that the line
    // numbers will match when comparing the results
    //
    CHAR *expectedKInvariantTextCondition = "0"; CHAR *expectedKInvariantTextFile = __FILE__;   ULONG expectedKInvariantTextLine = __LINE__; KInvariant(FALSE);

    if ((strcmp(KInvariantTextCondition, expectedKInvariantTextCondition) != 0) ||
        (strcmp(KInvariantTextFile, expectedKInvariantTextFile) != 0) ||
        (KInvariantTextLine != expectedKInvariantTextLine))
    {
        KTestPrintf("FAILED: Results are not correct Condition: %s, File: %s, Line: %d\n"
                    "                                Expecting: %s, File: %s, Line: %d\n",
                    KInvariantTextCondition, KInvariantTextFile, KInvariantTextLine,
                    expectedKInvariantTextCondition, expectedKInvariantTextFile, expectedKInvariantTextLine);
    }
    
    if (! KInvariantBoolean)
    {
        KTestPrintf("FAILED: Expected default KInvariant callout to return TRUE\n");
        status = STATUS_UNSUCCESSFUL;
    }

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KInvariantTest: COMPLETED\n");
    return(status);
}

#if CONSOLE_TEST
int __cdecl
#if !defined(PLATFORM_UNIX)
wmain(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
)
{
#else
main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs)
#endif
    NTSTATUS status;

    //
    // Adjust for the test name so CmdParseLine works.
    //

    if (argc > 0)
    {
        argc--;
        args++;
    }

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif

    status = KInvariantTest(argc, args);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("KInvariant tests failed Status = 0X%X\n", status);
        return RtlNtStatusToDosError(status);
    }

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return RtlNtStatusToDosError(status);
}
#endif
