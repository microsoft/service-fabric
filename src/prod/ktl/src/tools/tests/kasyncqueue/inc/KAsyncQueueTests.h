/*++

Copyright (c) Microsoft Corporation

Module Name:

    KAsyncQueueTests.h

Abstract:

    Contains unit test case routine declarations.

    To add a new unit test case:
    1. Declare your test case routine in KAsyncQueueTests.h.
    2. Add an entry to the gs_KuTestCases table in KAsyncQueueTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/

#pragma once
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif

struct StaticTestCallback {

    PVOID _EventPtr;

    StaticTestCallback(PVOID EventPtr);

    void Callback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);
};

NTSTATUS
KAsyncHelperTests(int argc, WCHAR* args[]);

NTSTATUS
KAsyncEventTest(int argc, WCHAR* args[]);

NTSTATUS
KTimerTest(int argc, WCHAR* args[]);

NTSTATUS
KQuotaGateTest(int argc, WCHAR* args[]);

NTSTATUS
KAsyncServiceTest(int argc, WCHAR* args[]);
