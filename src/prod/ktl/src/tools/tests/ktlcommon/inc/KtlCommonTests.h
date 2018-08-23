/*++

Copyright (c) Microsoft Corporation

Module Name:

    KtlCommonTests.h

Abstract:

    Contains unit test case routine declarations.

    To add a new unit test case:
    1. Declare your test case routine in KtlCommonTests.h.
    2. Add an entry to the gs_KuTestCases table in KtlCommonTestCases.cpp.
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
KtlCommonTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    );

NTSTATUS
KtlTextTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    );

#if !defined(PLATFORM_UNIX)
NTSTATUS
KNtMemoryMapTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    );
#endif

