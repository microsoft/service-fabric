/*++

Copyright (c) Microsoft Corporation

Module Name:

    KHttpTests.h

Abstract:

    Contains unit test case routine declarations.

    To add a new unit test case:
    1. Declare your test case routine in KWebSocketTests.h.
    2. Add an entry to the gs_KuTestCases table in KWebSocketTestCases.cpp.
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
KWebSocketTest(
    int argc, WCHAR* args[]
    );
