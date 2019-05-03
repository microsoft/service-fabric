
/*++

Copyright (c) Microsoft Corporation

Module Name:

    KRttTests.cpp

Abstract:

    This file contains test case implementations of KRTT.


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
#include "KRTTTests.h"
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

class Base : public KRTT
{
public:
    int x;
    K_RUNTIME_TYPED(Base);

    Base()
    {
        x = 0;
    }
};


class Other : public KRTT, public KObject<Other>, KShared<Other>
{
    K_FORCE_SHARED_WITH_INHERITANCE(Other);
    K_RUNTIME_TYPED(Other);

public:
    int x;

};

Other::Other()
{
    x = 900;
}

Other::~Other()
{
}

class Derived1 : public Base
{
    K_RUNTIME_TYPED(Derived1);
public:

    Derived1()
    {
        x = 1;
    }
};

class Derived2 : public Derived1
{
    K_RUNTIME_TYPED(Derived2);

public:

    Derived2()
    {
        x = 2;
    }
};

class Derived3 : public Derived2
{
    K_RUNTIME_TYPED(Derived3);


public:
    Derived3()
    {
        x = 3;
    }
};


BOOLEAN
TestForBase(KRTT* pCandidate)
{
    if (is_type<Base>(pCandidate))
    {
        return TRUE;
    }
    return FALSE;
}

BOOLEAN
TestForDerived1(KRTT* pCandidate)
{
    if (is_type<Derived1>(pCandidate))
    {
        return TRUE;
    }
    return FALSE;
}

BOOLEAN
TestForDerived2(KRTT* pCandidate)
{
    if (is_type<Derived2>(pCandidate))
    {
        return TRUE;
    }
    return FALSE;
}

BOOLEAN
TestForDerived3(KRTT* pCandidate)
{
    if (is_type<Derived3>(pCandidate))
    {
        return TRUE;
    }
    return FALSE;
}

BOOLEAN
TestForOther(KRTT* pCandidate)
{
    if (is_type<Other>(pCandidate))
    {
        return TRUE;
    }
    return FALSE;
}


NTSTATUS
TestSequence()
{
    Derived3 obj;
    Derived2 obj2;

    if (TestForBase(&obj2) == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (TestForDerived2(&obj2) == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (TestForDerived3(&obj2) == TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (TestForBase(&obj) == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (TestForDerived2(&obj) == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (TestForDerived1(&obj) == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (TestForDerived3(&obj) == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (TestForOther(&obj) == TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
KRTTTest(
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
	
    KTestPrintf("KRTTTest: STARTED\n");

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

    KTestPrintf("KRTTTest: COMPLETED\n");

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
    std::vector<WCHAR*> args_vec(argc);
    WCHAR** args = (WCHAR**)args_vec.data();
    std::vector<std::wstring> wargs(argc);
    for (int iter = 0; iter < argc; iter++)
    {
        wargs[iter] = Utf8To16(cargs[iter]);
        args[iter] = (WCHAR*)(wargs[iter].data());
    }
#endif
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KRTTTest(argc, args);
}
#endif





