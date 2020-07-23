/*++

Copyright (c) Microsoft Corporation

Module Name:

    KHttpTests.h

Abstract:

    Contains unit test case routine declarations.

    To add a new unit test case:
    1. Declare your test case routine in KHttpTests.h.
    2. Add an entry to the gs_KuTestCases table in KHttpTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/

#pragma once
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif

#if KTL_USER_MODE
 #define _printf(...)   printf(__VA_ARGS__)
// #define _printf(...)   KDbgPrintf(__VA_ARGS__)

 extern volatile LONGLONG gs_AllocsRemaining;
#else
 #define _printf(...)   KDbgPrintf(__VA_ARGS__)
 #define wprintf(...)    KDbgPrintf(__VA_ARGS__)
#endif

extern KAllocator* g_Allocator;

#define KHTTP_TAG 'PHHK'

#if KTL_USER_MODE

extern volatile LONGLONG gs_AllocsRemaining;

#endif

class QuickTimer
{
public:
    QuickTimer()
    {
        _Start =  KNt::GetTickCount64();
    }

    BOOLEAN
    HasElapsed(
        __in ULONG Seconds
        )
    {
            ULONGLONG Now = KNt::GetTickCount64();
            if ((Now - _Start)/1000 > Seconds)
            {
                return TRUE;
            }
            return FALSE;
    }

private:
    ULONGLONG _Start;
};

extern KEvent* p_ServerShutdownEvent;;
extern BOOLEAN g_ShutdownServer;

VOID ServerShutdownCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS CloseStatus
    );

extern KEvent* p_ServerOpenEvent;

VOID ServerOpenCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS OpenStatus
    );

extern KEvent* p_OtherShutdownEvent;

VOID OtherShutdownCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS CloseStatus
    );

extern KEvent* p_OtherOpenEvent;

VOID OtherOpenCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS OpenStatus
    );


BOOLEAN
FilePathToKBuffer(
    __in LPCWSTR FilePath,
    __out KBuffer::SPtr& Buffer
    );

NTSTATUS MultiPartPost(
    __in LPCWSTR Url,
    __in LPCWSTR FilePath,
    __in LPCWSTR ContentType,
    __in LPCWSTR Dest
    );

BOOLEAN AllowServerCert(
    __in PCCERT_CONTEXT cert_context
    );

BOOLEAN DenyServerCert(
    __in PCCERT_CONTEXT cert_context
    );

NTSTATUS
RunAdvancedServer(
    __in LPCWSTR Url,
    __in ULONG Seconds,
	__in ULONG NumberPrepostedRequests
    );

NTSTATUS
RunAdvancedClient(
    __in LPCWSTR Url,
	__in ULONG NumberOutstandingRequests
    );

#if NTDDI_VERSION >= NTDDI_WIN8
NTSTATUS
RunTimedWebSocketEchoServer(
    __in LPCWSTR Url,
    __in ULONG Seconds
    );

NTSTATUS
RunWebSocketClientEchoTest(
    __in LPCWSTR Url
    );

NTSTATUS
RunInProcWebSocketTest(
    __in ULONG ServerIterations,
    __in ULONG ClientIterations
    );
#endif

NTSTATUS
KHttpTest(
    int argc, WCHAR* args[]
    );
