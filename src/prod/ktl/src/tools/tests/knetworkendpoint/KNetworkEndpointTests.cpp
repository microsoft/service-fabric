/*++

Copyright (c) Microsoft Corporation

Module Name:

    KNetworkEndpointTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KNetworkEndpointTests.h.
    2. Add an entry to the gs_KuTestCases table in KNetworkEndpointTestCases.cpp.
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
#include "KNetworkEndpointTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

KAllocator* g_Allocator = nullptr;

NTSTATUS
TestSequence()
{
    KNetworkEndpoint::SPtr Nep;
    KUriView Uri(L"http://bing.com");
    NTSTATUS Res = KHttpNetworkEndpoint::Create(Uri, *g_Allocator, Nep);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KUri::SPtr UrlPtr = Nep->GetUri();
    if (UrlPtr)
    {
#ifdef PLATFORM_UNIX
        KTestPrintf("The URL is %s\n", Utf16To8(PWSTR((KStringView&)(*UrlPtr))).c_str());
#else
        KTestPrintf("The URL is %S\n", PWSTR(KStringView(*UrlPtr)));
#endif
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!is_type<KHttpNetworkEndpoint>(Nep.RawPtr()))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!is_type<KHttpNetworkEndpoint>(Nep))
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
KNetworkEndpointTest(
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
	
    KTestPrintf("KNetworkEndpointTest: STARTED\n");

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

    Result = TestSequence();

    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Failures...\n");
    }
    else
    {
        KTestPrintf("No errors\n");
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

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KNetworkEndpointTest: COMPLETED\n");

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

    return KNetworkEndpointTest(argc, args);
}
#endif



