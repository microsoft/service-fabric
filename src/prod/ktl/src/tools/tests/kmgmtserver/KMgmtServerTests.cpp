/*++

Copyright (c) Microsoft Corporation

Module Name:

    KMgmtServerTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KMgmtServerTests.h.
    2. Add an entry to the gs_KuTestCases table in KMgmtServerTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include "KMgmtServerTests.h"
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
KEvent*  g_MgmtStartupEvent = 0;
KEvent*  g_HttpShutdownEvent = 0;
KEvent*  g_MgmtShutdownEvent = 0;
NTSTATUS g_StartupStatus = 0;

////////////// BEGIN SAMPLE PROVIDER ///////////////////////////////

class TestMgmtProviderA : public KIManagementOps
{
public:
    typedef KSharedPtr<TestMgmtProviderA> SPtr;

    static

    KSharedPtr<TestMgmtProviderA>
    Create(
        __in KAllocator& Allocator
        )
    {
        return _new(KTL_TAG_MGMT_SERVER, Allocator) TestMgmtProviderA;
    }

    struct MgmtOpCompletion : public KAsyncContextBase
    {
        typedef KSharedPtr<MgmtOpCompletion> SPtr;

        MgmtOpCompletion(){}
       ~MgmtOpCompletion(){}

        VOID OnStart()
        {
            Complete(STATUS_SUCCESS);
        }


        VOID Finish(
            __in_opt KAsyncContextBase* Parent,
            __in  KAsyncContextBase::CompletionCallback Callback
            )
        {
            Start(Parent, Callback);
        }
    };


    virtual NTSTATUS
    StartGetAsync(
        __in  KUriView& Target,
        __in  KIDomNode::SPtr RequestHeaders,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in  KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        )
        override
    {
        MgmtOpCompletion::SPtr NewOp = _new(KTL_TAG_MGMT_SERVER, *g_Allocator) MgmtOpCompletion;
        if (!NewOp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KStringView ResponseText(L"<GetResponse>Data</GetResponse>");

        NTSTATUS Res = KDom::FromString(ResponseText, GetThisAllocator(), ResponseBody);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }


        KStringView ResponseHeaderTemplate(
            L"<ResponseMessageHeader>\n"
            L"    <ActivityId xsi:type=\"ktl:ULONGLONG\">99</ActivityId>\n"
            L"    <Status xsi:type=\"ktl:ULONG\">33</Status>\n"
            L"    <StatusText xsi:type=\"ktl:STRING\">The result is thirty three</StatusText>\n"
            L"</ResponseMessageHeader>\n"
            );

        Res = KDom::FromString(ResponseHeaderTemplate, GetThisAllocator(), ResponseHeaders);
        if (!NT_SUCCESS(Res))
        {

        }


        NewOp->Finish(Parent, Callback);
        return STATUS_PENDING;
    }

    virtual NTSTATUS
    StartCreateAsync(
        __in KUriView& Target,
        __in KIDomNode::SPtr RequestHeaders,
        __in KIDomNode::SPtr RequestBody,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        )
        override
    {
        MgmtOpCompletion::SPtr NewOp = _new(KTL_TAG_MGMT_SERVER, *g_Allocator) MgmtOpCompletion;
        if (!NewOp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NewOp->Finish(Parent, Callback);
        return STATUS_PENDING;
    }

    virtual NTSTATUS
    StartUpdateAsync(
        __in KUriView& Target,
        __in KIDomNode::SPtr RequestHeaders,
        __in KIDomNode::SPtr RequestBody,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        )
        override
    {
        MgmtOpCompletion::SPtr NewOp = _new(KTL_TAG_MGMT_SERVER, *g_Allocator) MgmtOpCompletion;
        if (!NewOp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NewOp->Finish(Parent, Callback);
        return STATUS_PENDING;
    }

    virtual NTSTATUS
    StartDeleteAsync(
        __in KUriView& Target,
        __in KIDomNode::SPtr RequestHeaders,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        )
        override
    {
        MgmtOpCompletion::SPtr NewOp = _new(KTL_TAG_MGMT_SERVER, *g_Allocator) MgmtOpCompletion;
        if (!NewOp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NewOp->Finish(Parent, Callback);
        return STATUS_PENDING;
    }

    virtual NTSTATUS
    StartQueryAsync(
        __in  KUriView& Target,
        __in  KIDomNode::SPtr Query,
        __in  KIDomNode::SPtr RequestHeaders,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KArray<KIMutableDomNode::SPtr>& ResponseBodies,
        __out BOOLEAN& MoreData,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        )
        override
     {
        MgmtOpCompletion::SPtr NewOp = _new(KTL_TAG_MGMT_SERVER, *g_Allocator) MgmtOpCompletion;
        if (!NewOp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NewOp->Finish(Parent, Callback);
        return STATUS_PENDING;
    }

    virtual NTSTATUS
    StartInvokeAsync(
        __in  KUriView& Target,
        __in  KUriView& Verb,
        __in  KIDomNode::SPtr RequestHeaders,
        __in  KIDomNode::SPtr RequestBody,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        )
        override
     {
        MgmtOpCompletion::SPtr NewOp = _new(KTL_TAG_MGMT_SERVER, *g_Allocator) MgmtOpCompletion;
        if (!NewOp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NewOp->Finish(Parent, Callback);
        return STATUS_PENDING;
    }
};


//////////////////////// END PROVIDER //////////////////////////////////

VOID
MgmtDeactivateCompletion(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
     KTestPrintf("Deactivation Completion");
    KMgmtServer& MgmtServer = (KMgmtServer&) Op;
    NTSTATUS Res = MgmtServer.Status();
    KTestPrintf("  Status = %d 0x%X\n", Res, Res);
    g_MgmtShutdownEvent->SetEvent();
}



VOID
MgmtStartupCompletion(
    __in ULONG StatusCode
    )
{
    g_StartupStatus = StatusCode;
    KTestPrintf("Server startup completed. Status = 0x%x\n", StatusCode);

    g_MgmtStartupEvent->SetEvent();
}



VOID HttpServerShutdownCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncContextBase& Op
    )
{
    KTestPrintf("Shutting down KHttpServer!\n");
    g_HttpShutdownEvent->SetEvent();
}

VOID HttpDefaultServerHandler(
    __in KHttpServerRequest::SPtr Request
    )
{
    // If here, send back a 404.  The KMgmtServer should have intercepted all the actual requests.

    Request->SetResponseHeader(KStringView(L"KTL.Test.Server.Message"), KStringView(L"URL not found!"));
    Request->SendResponse(KHttpUtils::NotFound, "You sent a really bad URL to me");
}



NTSTATUS
StartHttpServer(
    __in KUriView& HttpUrl,
    __out KHttpServer::SPtr& HttpServer
    )
{
    NTSTATUS Res = KHttpServer::Create(*g_Allocator, HttpServer);
    if (!NT_SUCCESS(Res))
    {
        if (Res == STATUS_ACCESS_DENIED)
        {
            KTestPrintf("ERROR: Server must run under admin privileges.\n");
        }
        return Res;
    }

    Res = HttpServer->Activate(HttpUrl, HttpDefaultServerHandler, 3, 8192, 8192, 0, HttpServerShutdownCallback);
    if (!NT_SUCCESS(Res))
    {
        if (Res == STATUS_INVALID_ADDRESS)
        {
            KTestPrintf("ERROR: URL is bad or missing the port number, which is required for HTTP.SYS based servers\n");
        }
        return Res;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
TestSequence(
    __in KUriView& HttpUrl
    )
{
    KMgmtServer::SPtr Server;
    KHttpServer::SPtr HttpServer;

    KEvent HttpShutdownEvent;
    KEvent MgmtShutdownEvent;
    KEvent MgmtStartupEvent;

    g_HttpShutdownEvent = &HttpShutdownEvent;

    g_MgmtStartupEvent = &MgmtStartupEvent;
    g_MgmtShutdownEvent = &MgmtShutdownEvent;

    NTSTATUS Res;

    // Start up the HTTP server.
    //
    Res = StartHttpServer(HttpUrl, HttpServer);
    if (!NT_SUCCESS(Res))
    {
        KTestPrintf("Failed to start HTTP server\n");
        return Res;
    }

    Res = KMgmtServer::Create(*g_Allocator, Server);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Activate and wait for callback.
    //

#if KTL_USER_MODE
    Res = Server->Activate(HttpServer, KUriView(L"mgmt"), KStringView(L"KMgmtServer Unit Test"), MgmtStartupCompletion, nullptr, MgmtDeactivateCompletion, nullptr);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }
#else
    Res = Server->Activate(StartupCompletion, nullptr, DeactivateCompletion, nullptr);
    if (Res != STATUS_PENDING)
    {
        return Res;
    }

#endif

    MgmtStartupEvent.WaitUntilSet();
    if (!NT_SUCCESS(g_StartupStatus))
    {
        return g_StartupStatus;
    }


    TestMgmtProviderA::SPtr Prov;
    Prov = TestMgmtProviderA::Create(*g_Allocator);
    if (!Prov)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Res = Server->RegisterProvider(KUriView(L"foo:/bar"), KStringView(L"TestProviderA"), down_cast<KIManagementOps, TestMgmtProviderA>(Prov), KMgmtServer::eMatchPrefix);

    // Now sit around for 100 seconds and service requests.
    //
    KNt::Sleep(100000);

    Res = Server->UnregisterProvider(KUriView(L"foo:/bar"));

    Server->Deactivate();
    HttpServer->Deactivate();

    // Wait for the various shutdown events

    MgmtShutdownEvent.WaitUntilSet();
    HttpShutdownEvent.WaitUntilSet();


    return STATUS_SUCCESS;

}

NTSTATUS
KMgmtServerTest(
    int argc, WCHAR* args[]
    )
{
    NTSTATUS Result;

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
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
       Result = TestSequence(KUriView(args[0]));
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

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return Result;
}

#if CONSOLE_TEST
int
wmain(int argc, WCHAR* args[])
{
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KMgmtServerTest(argc, args);
}
#endif
