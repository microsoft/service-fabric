/*++

Copyright (c) Microsoft Corporation

Module Name:

    KUriTests.h

Abstract:

    Contains unit test case routine declarations.

    To add a new unit test case:
    1. Declare your test case routine in KUriTests.h.
    2. Add an entry to the gs_KuTestCases table in KUriTestCases.cpp.
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
KUriTest(
    int argc, WCHAR* args[]
    );
