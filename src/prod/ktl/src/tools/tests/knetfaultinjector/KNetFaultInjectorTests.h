/*++

Copyright (c) Microsoft Corporation

Module Name:

    KNetFaultInjectorTests.h

Abstract:

    Contains unit test case routine declarations.

    To add a new unit test case:
    1. Declare your test case routine in KNetFaultInjectorTests.h.
    2. Add an entry to the gs_KuTestCases table in KNetFaultInjectorTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/

#pragma once
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif

NTSTATUS
KNetFaultInjectorTest(
    int argc, WCHAR* args[]
    );
