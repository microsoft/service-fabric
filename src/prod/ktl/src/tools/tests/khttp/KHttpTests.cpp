/*++

Copyright (c) Microsoft Corporation

Module Name:

    KHttpTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KHttpTests.h.
    2. Add an entry to the gs_KuTestCases table in KHttpTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KHttpTests.h"
#include <CommandLineParser.h>

KAllocator* g_Allocator = nullptr;

NTSTATUS
DirectGet(
    __in KStringView& Url,
    __in LPCWSTR FilePath,
    __in_opt PCCERT_CONTEXT clientCert = NULL,
    __in_opt KHttpCliRequest::ServerCertValidationHandler validateServerCert = NULL
    );



VOID
DumpEntityBodyToFile(
    __in PUCHAR Body,
    __in ULONG Len
    )
{
    UNREFERENCED_PARAMETER(Body);
    UNREFERENCED_PARAMETER(Len);
    
#ifdef DUMP_POST_BODY_TO_FILE

#endif
}


BOOLEAN
FilePathToKBuffer(
    __in LPCWSTR FilePath,
    __out KBuffer::SPtr& Buffer
    )
{
    HANDLE h = CreateFile(FilePath, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    ULONG Length = SetFilePointer(h, 0, 0, FILE_END);
    if (Length == 0)
    {
        CloseHandle(h);
        return FALSE;
    }

    NTSTATUS Res = KBuffer::Create(Length, Buffer, *g_Allocator);
    if (!NT_SUCCESS(Res))
    {
        CloseHandle(h);
        return FALSE;
    }

    SetFilePointer(h, 0, 0, FILE_BEGIN);
    ULONG TotalRead = 0;
    ReadFile(h,  Buffer->GetBuffer(), Length, &TotalRead, 0);

    if (TotalRead != Length)
    {
        CloseHandle(h);
        return FALSE;
    }
    CloseHandle(h);
    return TRUE;
}


VOID
MainPage(
    __in KHttpServerRequest::SPtr Request
    )
{
    static char* Str =
        "http://./test                      // This page\r\n"
        "http://./test/echo                 // For POST, this returns whatever was posted\r\n"
        "http://./test/big                  // GET; returns 1 meg of text\r\n"
        "http://./test/file?filepath        // Returns the specified file to the client\r\n";

    KMemRef mem((ULONG)strlen(Str), Str, (ULONG)strlen(Str));
    Request->SetResponseContent(mem);
    KStringView value(L"text/plain");
    Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    Request->SendResponse(KHttpUtils::Ok);
}

VOID
BigText(
    __in KHttpServerRequest::SPtr Request
    )
{
    const ULONG BufSize = 0x100000;

    KHttpUtils::OperationType Op = Request->GetOpType();
    if (Op != KHttpUtils::eHttpGet)
    {
        Request->SendResponse(KHttpUtils::BadRequest, "Op type should be GET!");
        return;
    }

    KBuffer::SPtr Text;
    NTSTATUS Res = KBuffer::Create(BufSize, Text, *g_Allocator);
    if (!NT_SUCCESS(Res))
    {
        Request->SendResponse(KHttpUtils::InternalServerError, "Could not allocate memory!");
        return;
    }

    char* Buf = (char *) Text->GetBuffer();
    char* Trace;
    ULONG Count;
    char Alpha;

    for (Trace = Buf, Count = 0, Alpha = 'A'; Count < BufSize; Count++)
    {
        *Trace++ = Alpha;
        Alpha++;
        if (Alpha > 'Z')
        {
            Alpha = 'A';
        }
    }

    Request->SetResponseContent(Text);
    KStringView value(L"text/plain");
    Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    Request->SendResponse(KHttpUtils::Ok);
}

//
//  Echo the content and content type back to the poster
//
VOID
Echo(
    __in KHttpServerRequest::SPtr Request
    )
{
    KHttpUtils::OperationType Op = Request->GetOpType();
    if (Op != KHttpUtils::eHttpPost)
    {
        Request->SendResponse(KHttpUtils::BadRequest, "Op type should be POST");
    }
    // Get the content
    //
    KBuffer::SPtr EntityBody;
    NTSTATUS Res = Request->GetRequestContent(EntityBody);

    if (!NT_SUCCESS(Res))
    {
        Request->SendResponse(KHttpUtils::BadRequest, "This URL requires a POST with content to be echoed back.  No content detected.");
        return;
    }

    // Get the content type

    KString::SPtr ContType;

    Res = Request->GetRequestHeaderByCode(KHttpUtils::HttpHeaderContentType, ContType);
    if (!NT_SUCCESS(Res))
    {
        Request->SendResponse(KHttpUtils::BadRequest, "Content-Type header is required for this operation.");
        return;
    }

    KBuffer::SPtr ResponseContent;
    Res = KBuffer::Create(Request->GetRequestEntityBodyLength(), ResponseContent, *g_Allocator);
    if (!NT_SUCCESS(Res))
    {
        Request->SendResponse(KHttpUtils::InternalServerError, "Could not allocate memory!");
        return;
    }

    KMemCpySafe(
        ResponseContent->GetBuffer(), 
        ResponseContent->QuerySize(), 
        EntityBody->GetBuffer(),
        Request->GetRequestEntityBodyLength());

    Request->SetResponseContent(ResponseContent);

    // Send it back

    Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, *ContType);
    KStringView headerId(L"KTL.Test.Server");
    KStringView value(L"Echoing original content");
    Request->SetResponseHeader(headerId, value);
    Request->SendResponse(KHttpUtils::Ok);
}


VOID
GetFile(
    __in KHttpServerRequest::SPtr Request
    )
{
    KHttpUtils::OperationType Op = Request->GetOpType();
    if (Op != KHttpUtils::eHttpGet)
    {
        Request->SendResponse(KHttpUtils::BadRequest, "Op type should be GET");
        return;
    }

    // Get the required file from the URL

    KUri::SPtr Url;
    Request->GetUrl(Url);
    KStringView& FilePath = Url->Get(KUri::eQueryString);

    if (FilePath.Length() == 0)
    {
        Request->SendResponse(KHttpUtils::BadRequest, "Missing file path from URL fragment");
        return;
    }

    KLocalString<MAX_PATH+1> Path;

    Path.CopyFrom(FilePath);
    Path.UnencodeEscapeChars();
    Path.SetNullTerminator();

    KBuffer::SPtr ReturnBuf;
    BOOLEAN Res = FilePathToKBuffer(Path, ReturnBuf);
    Res;
    if (FilePath.Length() == 0)
    {
        Request->SendResponse(KHttpUtils::InternalServerError, "Could find or allocate the file");
        return;
    }

    Request->SetResponseContent(ReturnBuf);

    // Send it back
    KStringView value(L"random-binary-file");
    Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    KStringView headerId(L"KTL.Test.Server");
    value = L"Got the file you requested";
    Request->SetResponseHeader(headerId, value);
    Request->SendResponse(KHttpUtils::Ok);
}

VOID
SpecialHandler1(
    __in KHttpServerRequest::SPtr Request
    )
{
    static char* Str =
        "SpecialHandler1";

    KMemRef mem((ULONG)strlen(Str), Str, (ULONG)strlen(Str));
    Request->SetResponseContent(mem);
    KStringView value(L"text/plain");
    Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    Request->SendResponse(KHttpUtils::Ok);

    // CONSIDER: WHy is it sending a second response ? The one below will only error out

    // If here, send back a 404

    KStringView headerId(L"KTL.Test.Server.Message");
    value = L"URL not found!";
    Request->SetResponseHeader(headerId, value);
    Request->SendResponse(KHttpUtils::NotFound, "You sent a really bad URL to me");
}

VOID
SpecialHandler2(
    __in KHttpServerRequest::SPtr Request
    )
{
    static char* Str =
        "SpecialHandler2";

    KMemRef mem((ULONG)strlen(Str), Str, (ULONG)strlen(Str));
    Request->SetResponseContent(mem);
    KStringView value(L"text/plain");
    Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    Request->SendResponse(KHttpUtils::Ok);
}

VOID
SpecialHandler3(
    __in KHttpServerRequest::SPtr Request
)
{
    static char* Str =
        "SpecialHandler3";
    KUri::SPtr Url;
    Request->GetUrl(Url);

    // Last segment is the UTF-8 encoded value corresponding to ijådkåælæmænopekuæræsteuve
    KStringView comparand(L"testurl/globalizedpath/ij%C3%A5dk%C3%A5%C3%A6l%C3%A6m%C3%A6nopeku%C3%A6r%C3%A6steuve");
    if (Url->Get(KUri::ePath).Compare(comparand) == 0)
    {
        KMemRef mem((ULONG)strlen(Str), Str, (ULONG)strlen(Str));
        Request->SetResponseContent(mem);
        KStringView value(L"text/plain");
        Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
        Request->SendResponse(KHttpUtils::Ok);
        return;
    }

    KStringView headerId(L"KTL.Test.Server.Message");
    KStringView value(L"URL not found!");
    Request->SetResponseHeader(headerId, value);
    Request->SendResponse(KHttpUtils::NotFound, "You sent a really bad URL to me");
}

VOID
LargeQueryStringHandler(
    __in KHttpServerRequest::SPtr Request
)
{
    static char* Str =
        "LargeQueryStringHandler";
    KUri::SPtr Url;
    Request->GetUrl(Url);

    KStringView& QueryString = Url->Get(KUri::eQueryString);

    if (QueryString.Length() == 0)
    {
        Request->SendResponse(KHttpUtils::BadRequest, "Missing query string");
        return;
    }

    Request->SetResponseContent(QueryString);
    KStringView value(L"text/plain");
    Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    Request->SendResponse(KHttpUtils::Ok);
    return;
}

VOID
CustomVerbHandler(
    __in KHttpServerRequest::SPtr Request
)
{
    static char* Str =
        "CustomVerbHandler";

    KUri::SPtr Url;
    Request->GetUrl(Url);

    KString::SPtr VerbStr;
    BOOLEAN Ret = Request->GetOpString(VerbStr);

    if (Ret)
    {
        // If path is testpatch and verb is PATCH, return SUCCESS
        // Or If the path is testxxyy and verb is XXYY, return SUCCESS
        if ((Url->Get(KUri::ePath).Compare(KStringView(L"testurl/testpatch")) == 0
            && VerbStr->CompareNoCase(L"PATCH") == 0)
            || (Url->Get(KUri::ePath).Compare(KStringView(L"testurl/testxxyy")) == 0
                && VerbStr->CompareNoCase(L"XXYY") == 0))
        {
            KMemRef mem((ULONG)strlen(Str), Str, (ULONG)strlen(Str));
            Request->SetResponseContent(mem);
            KStringView value(L"text/plain");
            Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
            Request->SendResponse(KHttpUtils::Ok);
            return;
        }
    }

    KStringView headerId(L"KTL.Test.Server.Message");
    KStringView value(L"Incorrect VERB!");
    Request->SetResponseHeader(headerId, value);
    Request->SendResponse(KHttpUtils::BadRequest, "Incorrect VERB!");
    return;
}

VOID
DefaultServerHandler(
    __in KHttpServerRequest::SPtr Request

    )
{
    NTSTATUS Res;

    // Dump useful info to the console

    KHttpUtils::OperationType Op = Request->GetOpType();
    KStringView OpString;
    KHttpUtils::OpToString(Op, OpString);

    KTestPrintf("***************Server Operation = %S\n", PWSTR(OpString));

    KString::SPtr OpStr;
    Request->GetOpString(OpStr);
    KTestPrintf("***************Server Operation from GetOpString = %S\n", PWSTR(*OpStr));

    KUri::SPtr Url;
    Request->GetUrl(Url);
    KTestPrintf("URL = %S\n", LPCWSTR(*Url));
    KTestPrintf("Inbound data length = %u\n", Request->GetRequestEntityBodyLength());
    KString::SPtr Val;

    Res = Request->GetAllHeaders(Val);
    KTestPrintf("-----------All headers---------:\n%S\n", PWSTR(*Val));
    KTestPrintf("\n----------End Headers------------\n");


    if (Url->Get(KUri::ePath).Compare(KStringView(L"test")) == 0)
    {
        MainPage(Request);
        return;
    }

    if (Url->Get(KUri::ePath).Compare(KStringView(L"test/echo")) == 0)
    {
        Echo(Request);
        return;
    }

    if (Url->Get(KUri::ePath).Compare(KStringView(L"test/big")) == 0)
    {
        BigText(Request);
        return;
    }

    if (Url->Get(KUri::ePath).Compare(KStringView(L"test/file")) == 0)
    {
        GetFile(Request);
        return;
    }

    // If here, send back a 404
    KStringView headerId(L"KTL.Test.Server.Message");
    KStringView value(L"URL not found!");
    Request->SetResponseHeader(headerId, value);
    Request->SendResponse(KHttpUtils::NotFound, "You sent a really bad URL to me");
}

VOID
OtherServerHandler(
    __in KHttpServerRequest::SPtr Request
    )
{
    NTSTATUS Res;


    KTestPrintf("*** Other server handler\n");

    // Dump useful info to the console

    KHttpUtils::OperationType Op = Request->GetOpType();
    KStringView OpString;
    KHttpUtils::OpToString(Op, OpString);

    KTestPrintf("***************Server Operation = %S\n", PWSTR(OpString));
    KUri::SPtr Url;
    Request->GetUrl(Url);
    KTestPrintf("URL = %S\n", LPCWSTR(*Url));
    KTestPrintf("Inbound data length = %u\n", Request->GetRequestEntityBodyLength());
    KString::SPtr Val;

    Res = Request->GetAllHeaders(Val);
    KTestPrintf("-----------All headers---------:\n%S\n", PWSTR(*Val));
    KTestPrintf("\n----------End Headers------------\n");


    if (Url->Get(KUri::ePath).Compare(KStringView(L"secondary")) == 0)
    {
        static char* Str = "The secondary has responded";
        KMemRef mem((ULONG)strlen(Str), Str, (ULONG)strlen(Str));
        Request->SetResponseContent(mem);
        KStringView value(L"text/plain");
        Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
        Request->SendResponse(KHttpUtils::Ok);
        return;
    }


    // If here, send back a 404
    KStringView headerId(L"KTL.Test.Server.Message");
    KStringView value(L"URL not found!");
    Request->SetResponseHeader(headerId, value);
    Request->SendResponse(KHttpUtils::NotFound, "You sent a really bad URL to me");
}

KEvent* p_ServerShutdownEvent = 0;
BOOLEAN g_ShutdownServer = FALSE;

VOID
ServerShutdownCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS CloseStatus
    )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Service);
    
    KTestPrintf("Shutting down: Status: 0X%08X\n", CloseStatus);
    g_ShutdownServer = TRUE;
    p_ServerShutdownEvent->SetEvent();
}

KEvent* p_ServerOpenEvent = 0;

VOID
ServerOpenCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS OpenStatus
    )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Service);
    
    KTestPrintf("Open complete: Status: 0X%08X\n", OpenStatus);
    p_ServerOpenEvent->SetEvent();
}


KEvent* p_OtherShutdownEvent = 0;

VOID
OtherShutdownCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS CloseStatus
    )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Service);
    
    KTestPrintf("Shutting down: Status: 0X%08X\n", CloseStatus);
    p_OtherShutdownEvent->SetEvent();
}

KEvent* p_OtherOpenEvent = 0;

VOID
OtherOpenCallback(
    __in KAsyncContextBase* Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS OpenStatus
    )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Service);
    
    KTestPrintf("Open complete: Status: 0X%08X\n", OpenStatus);
    p_OtherOpenEvent->SetEvent();
}


NTSTATUS
RunServer(
    __in LPCWSTR Url,
    __in ULONG Seconds
    )
{
    QuickTimer t;

    // Run two servers.  The 'OtherServer' is hard wired and is there to prove
    // multiple instances can be executed in the same process.

    KHttpServer::SPtr Server;
    KHttpServer::SPtr OtherServer;

    KEvent OpenEvent;
    KEvent OtherOpenEvent;
    KEvent ShutdownEvent;
    KEvent OtherShutdownEvent;
    p_ServerOpenEvent = &OpenEvent;
    p_ServerShutdownEvent = &ShutdownEvent;
    p_OtherShutdownEvent = &OtherShutdownEvent;
    p_OtherOpenEvent = &OtherOpenEvent;

    NTSTATUS Res = KHttpServer::Create(*g_Allocator, Server);
    if (!NT_SUCCESS(Res))
    {
        if (Res == STATUS_ACCESS_DENIED)
        {
            KTestPrintf("ERROR: Server must run under admin privileges.\n");
        }
        return Res;
    }

    Res = KHttpServer::Create(*g_Allocator, OtherServer);
    if (!NT_SUCCESS(Res))
    {
        if (Res == STATUS_ACCESS_DENIED)
        {
            KTestPrintf("ERROR: Server must run under admin privileges.\n");
        }
        return Res;
    }

    KUriView ListenUrl(Url);
    KTestPrintf("Running server on URL = %S\n", Url);

    Res = Server->StartOpen(ListenUrl, DefaultServerHandler, 3, 8192, 8192, 0, ServerOpenCallback, nullptr, KHttpUtils::HttpAuthType::AuthNone, 65536);
    if (!NT_SUCCESS(Res))
    {
        if (Res == STATUS_INVALID_ADDRESS)
        {
            KTestPrintf("ERROR: URL is bad or missing the port number, which is required for HTTP.SYS based servers\n");
        }
        return Res;
    }

    OpenEvent.WaitUntilSet();
    KInvariant(Server->Status() == STATUS_PENDING);

    KUriView suffix(L"suffix1");
    Res = Server->RegisterHandler(suffix, FALSE, SpecialHandler1);
    suffix = L"suffix2/suffix3";
    Res = Server->RegisterHandler(suffix, TRUE, SpecialHandler2);
    suffix = L"globalizedpath";
    Res = Server->RegisterHandler(suffix, FALSE, SpecialHandler3);
    suffix = L"testpatch";
    Res = Server->RegisterHandler(suffix, FALSE, CustomVerbHandler);
    suffix = L"testxxyy";
    Res = Server->RegisterHandler(suffix, FALSE, CustomVerbHandler);
    suffix = L"largequery";
    Res = Server->RegisterHandler(suffix, FALSE, LargeQueryStringHandler);

    // Initialize the 'other' server

    KUriView OtherUrl(L"http://*:80/secondary");
    Res = OtherServer->StartOpen(OtherUrl, OtherServerHandler, 3, 8192, 8192, 0, OtherOpenCallback, nullptr, KHttpUtils::HttpAuthType::AuthNone, 65536);
    if (!NT_SUCCESS(Res))
    {
        KTestPrintf("Unable to initialize other server!\n");
        return STATUS_INTERNAL_ERROR;
    }

    OtherOpenEvent.WaitUntilSet();
    KInvariant(OtherServer->Status() == STATUS_PENDING);

    for (;;)
    {
        KNt::Sleep(5000);
        if (t.HasElapsed(Seconds))
        {
            break;
        }
    }

    KTestPrintf("...Deactivating server\n");
    Server->StartClose(0, ServerShutdownCallback);
    OtherServer->StartClose(0, OtherShutdownCallback);

    ShutdownEvent.WaitUntilSet();
    OtherShutdownEvent.WaitUntilSet();
    KInvariant(Server->Status() == STATUS_SUCCESS);
    KInvariant(Server->Status() == STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

//
//  KHttpWebSocket Tests
//
#if NTDDI_VERSION >= NTDDI_WIN8


class WebSocketEchoHandler : public KAsyncContextBase
{
    K_FORCE_SHARED(WebSocketEchoHandler);

public:

    static CHAR const * const ShutdownString;
    static CHAR const * const AbortString;

    static NTSTATUS
    Create(
        __out WebSocketEchoHandler::SPtr& Handler
        )
    {
        Handler = nullptr;

        WebSocketEchoHandler::SPtr handler = _new(KTL_TAG_TEST, *g_Allocator) WebSocketEchoHandler();

        if (handler == nullptr)
        {
            return STATUS_UNSUCCESSFUL;
        }

        Handler = handler;
        return STATUS_SUCCESS;
    }

    VOID
    StartEchoWebSocket(
        __in KAsyncContextBase::CompletionCallback CloseCompletionCallback,
        __in KHttpServerRequest& ServerRequest,
        __in KHttpServer& ParentServer
        )
    {
        _ServerRequest = &ServerRequest;
        _ParentServer = &ParentServer;

        Start(&ParentServer, CloseCompletionCallback);
    }

    VOID
    CloseReceived(
        __in KWebSocket& WebSocket
        )
    {
        KInvariant(&WebSocket == _WebSocket.RawPtr());

        NTSTATUS status;

        status = _WebSocket->StartCloseWebSocket(
            this,
            KAsyncServiceBase::CloseCompletionCallback(this, &WebSocketEchoHandler::WebSocketCloseCallback)
            );
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed call to StartCloseWebSocket in CloseReceived;  It has already been called.");
            return;
        }

        return;
    }
     
    VOID
    WebSocketCloseCallback(
        KAsyncContextBase* const ParentAsync,
        KAsyncServiceBase& Service,
        NTSTATUS CloseStatus
        )
    {
        KInvariant(&Service == _WebSocket.RawPtr());
        KInvariant(ParentAsync == this);

        KTestPrintf("Websocket close callback received.  Status: 0X%08X", CloseStatus);
        KTestPrintf("Canceling the timer.");
        _Timer->Cancel();
        Complete(CloseStatus);
    }

    VOID
    OnStart() override
    {
        NTSTATUS status;

        status = KHttpServerWebSocket::Create(_WebSocket, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create KHttpServerWebSocket.");
            Complete(status);
            return;
        }

        status = _WebSocket->CreateReceiveFragmentOperation(_ReceiveFragmentOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create receive fragment operation.");
            Complete(status);
            return;
        }

        status = _WebSocket->CreateReceiveMessageOperation(_ReceiveMessageOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create receive message operation.");
            Complete(status);
            return;
        }

        status = _WebSocket->CreateSendFragmentOperation(_SendFragmentOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create send fragment operation.");
            Complete(status);
            return;
        }

        status = _WebSocket->CreateSendMessageOperation(_SendMessageOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create send message operation.");
            Complete(status);
            return;
        }

        status = KBuffer::Create(1024, _ReceiveBuffer, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create receive buffer.");
            Complete(status);
            return;
        }

        status = KBuffer::Create(1024, _SendBuffer, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create send buffer.");
            Complete(status);
            return;
        }

        KStringA::SPtr supportedSubprotocols;
        status = KStringA::Create(supportedSubprotocols, GetThisAllocator(), KStringViewA("  test , \t echo\r\n   , echo1, echo2\r\n"), TRUE);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create supported subprotocols string.");
            Complete(status);
            return;
        }
        KInvariant(supportedSubprotocols);
        KInvariant(supportedSubprotocols->IsNullTerminated());

        status = KTimer::Create(_Timer, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create send message timer.");
            Complete(status);
            return;
        }
        _Timer->StartTimer(100000, this, KAsyncContextBase::CompletionCallback(this, &WebSocketEchoHandler::TimerCallback));


        //
        //  Randomly enable/disable the various timers
        //
        if (rand() % 2)
        {
            KInvariant(NT_SUCCESS(_WebSocket->SetTimingConstant(KWebSocket::TimingConstant::OpenTimeoutMs, KWebSocket::TimingConstantValue::None)));
        }

        if (rand() % 2)
        {
            KInvariant(NT_SUCCESS(_WebSocket->SetTimingConstant(KWebSocket::TimingConstant::CloseTimeoutMs, KWebSocket::TimingConstantValue::None)));
        }

        if (rand() % 2)
        {
            KInvariant(NT_SUCCESS(_WebSocket->SetTimingConstant(KWebSocket::TimingConstant::PingQuietChannelPeriodMs, static_cast<KWebSocket::TimingConstantValue>(200))));
        }

        if (rand() % 2)
        {
            KInvariant(NT_SUCCESS(_WebSocket->SetTimingConstant(KWebSocket::TimingConstant::PongTimeoutMs, static_cast<KWebSocket::TimingConstantValue>(2000))));
        }

        if (rand() % 2)
        {
            KInvariant(NT_SUCCESS(_WebSocket->SetTimingConstant(KWebSocket::TimingConstant::PongKeepalivePeriodMs, static_cast<KWebSocket::TimingConstantValue>(200))));
        }

        status = _WebSocket->StartOpenWebSocket(
            *_ServerRequest,
            KWebSocket::WebSocketCloseReceivedCallback(this, &WebSocketEchoHandler::CloseReceived),
            this,
            KAsyncServiceBase::OpenCompletionCallback(this, &WebSocketEchoHandler::WebSocketOpenCallback),
            4096UL,
            4096UL,
            supportedSubprotocols
            );
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed call to StartOpenWebSocket.");
            KTestPrintf("Canceling the timer.");
            
            _Timer->Cancel();
            Complete(status);
            return;
        }

        return;
    }

    VOID
    WebSocketOpenCallback(
        KAsyncContextBase* const ParentAsync,
        KAsyncServiceBase& Service,
        NTSTATUS OpenStatus
        )
    {
        KInvariant(&Service == _WebSocket.RawPtr());
        KInvariant(ParentAsync == this);

        if (! NT_SUCCESS(OpenStatus))
        {
            KTestPrintf("server WebSocket open completed with failure status.");
            KTestPrintf("Canceling the timer.");
            _Timer->Cancel();
            Complete(OpenStatus);

            return;
        }

        KTestPrintf("Web socket is open.  Starting the receive operation.");
        StartReceive();
    }

    VOID
    StartReceive()
    {
        if (rand() % 2)
        {
            KTestPrintf("Starting server message receive.");
            _ReceiveMessageOperation->StartReceiveMessage(
                *_ReceiveBuffer,
                this,
                KAsyncContextBase::CompletionCallback(this, &WebSocketEchoHandler::ReceiveMessageCallback)
                );
        }
        else
        {
            KTestPrintf("Starting server fragment receive(s)");
            _ReceiveOffset = 0;

            DoFragmentReceive();
        }
    }

    VOID
    ProcessReceivedMessage()
    {
        //
        //  Server should initiate shutdown
        //
        if (KStringViewA(static_cast<PCHAR>(_ReceiveBuffer->GetBuffer()), _MessageLength, _MessageLength).Compare(ShutdownString) == 0)
        {
            KTestPrintf("Received shutdown string.  Closing the WebSocket");

            KSharedBufferStringA::SPtr closeReason = nullptr;
            NTSTATUS status = KSharedBufferStringA::Create(closeReason, "Received shutdown string", *g_Allocator);
            KWebSocket::CloseStatusCode closeStatus = NT_SUCCESS(status) ? KWebSocket::CloseStatusCode::NormalClosure : KWebSocket::CloseStatusCode::GoingAway;
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, 0, 0);
            }

            _WebSocket->StartCloseWebSocket(
                this,
                KAsyncServiceBase::CloseCompletionCallback(this, &WebSocketEchoHandler::WebSocketCloseCallback),
                closeStatus,
                closeReason
                );
            return;
        }
        else if (KStringViewA(static_cast<PCHAR>(_ReceiveBuffer->GetBuffer()), _MessageLength, _MessageLength).Compare(AbortString) == 0)
        {
            KTestPrintf("Receive abort string.  Aborting and closing the WebSocket");

            _WebSocket->Abort();

            _WebSocket->StartCloseWebSocket(
                this,
                KAsyncServiceBase::CloseCompletionCallback(this, &WebSocketEchoHandler::WebSocketCloseCallback),
                KWebSocket::CloseStatusCode::GoingAway
                );
            return;
        }
        else
        {
            StartSend();
        }
    }

    VOID
    TimerCallback(
        KAsyncContextBase* const ParentAsync,
        KAsyncContextBase& SubOp
        )
    {
        KInvariant(ParentAsync == this);
        KInvariant(&SubOp == _Timer.RawPtr());

        KInvariant(! NT_SUCCESS(_Timer->Status()));
    }

    VOID
    StartSend()
    {
        KInvariant(IsInApartment());
        if (rand() % 2)
        {
            KTestPrintf("Message received.  Starting the send message operation to echo it back to the client.");

            _SendMessageOperation->StartSendMessage(
                *_SendBuffer,
                _MessageContentType,
                this,
                KAsyncContextBase::CompletionCallback(this, &WebSocketEchoHandler::WebSocketSendMessageCallback),
                0,
                _MessageLength
                );
        }
        else
        {
            KTestPrintf("Message received.  Starting the send fragment operation(s) to echo it back to the client.");
            _SendOffset = 0;

            DoFragmentSend();
        }
    }

    VOID
    DoFragmentReceive()
    {
        ULONG const remaining = _ReceiveBuffer->QuerySize() - _ReceiveOffset;
        ULONG const receiveFragmentSize = (rand() % remaining) + 1;
        KInvariant(receiveFragmentSize > 0 && receiveFragmentSize <= remaining);

        _ReceiveFragmentOperation->StartReceiveFragment(
            *_ReceiveBuffer,
            this,
            KAsyncContextBase::CompletionCallback(this, &WebSocketEchoHandler::ReceiveFragmentCallback),
            _ReceiveOffset,
            receiveFragmentSize
            );
    }

    VOID
    ReceiveFragmentCallback(
        KAsyncContextBase* const ParentAsync,
        KAsyncContextBase& SubOp
        )
    {
        KInvariant(&SubOp == _ReceiveFragmentOperation.RawPtr());
        KInvariant(_ReceiveFragmentOperation->GetBuffer() == _ReceiveBuffer);
        KInvariant(_ReceiveBuffer->QuerySize() == _SendBuffer->QuerySize());
        KInvariant(ParentAsync == this);

        KTestPrintf("WebSocketEchoHandler::ReceiveFragmentCallback called");

        NTSTATUS status = SubOp.Status();

        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Server receive fragment operation failed, calling StartCloseWebSocket()");
            _WebSocket->StartCloseWebSocket(
                this,
                KAsyncServiceBase::CloseCompletionCallback(this, &WebSocketEchoHandler::WebSocketCloseCallback)
                );
            return;
        }

        _ReceiveOffset += _ReceiveFragmentOperation->GetBytesReceived();
        KInvariant(_ReceiveOffset <= _ReceiveBuffer->QuerySize());

        _MessageLength = _ReceiveOffset;
        _MessageContentType = _ReceiveFragmentOperation->GetContentType();

        _ReceiveFragmentOperation->Reuse();

        if (_ReceiveFragmentOperation->IsFinalFragment())
        {
            KMemCpySafe(_SendBuffer->GetBuffer(), _SendBuffer->QuerySize(), _ReceiveBuffer->GetBuffer(), _MessageLength);

            ProcessReceivedMessage();

            return;
        }
        else
        {
            DoFragmentReceive();

            return;
        }
    }

    VOID
    ReceiveMessageCallback(
        KAsyncContextBase* const ParentAsync,
        KAsyncContextBase& SubOp
        )
    {
        KInvariant(&SubOp == _ReceiveMessageOperation.RawPtr());
        KInvariant(_ReceiveMessageOperation->GetBuffer() == _ReceiveBuffer);
        KInvariant(_ReceiveBuffer->QuerySize() == _SendBuffer->QuerySize());
        KInvariant(ParentAsync == this);

        KTestPrintf("WebSocketEchoHandler::ReceiveMessageCallback called");

        NTSTATUS status = SubOp.Status();

        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Server receive message operation failed, calling StartCloseWebSocket()");
            _WebSocket->StartCloseWebSocket(
                this,
                KAsyncServiceBase::CloseCompletionCallback(this, &WebSocketEchoHandler::WebSocketCloseCallback)
                );
            return;
        }

        KInvariant(_ReceiveMessageOperation->GetBytesReceived() <= _SendBuffer->QuerySize());

        _MessageLength = _ReceiveMessageOperation->GetBytesReceived();
        _MessageContentType = _ReceiveMessageOperation->GetContentType();
        KMemCpySafe(_SendBuffer->GetBuffer(), _SendBuffer->QuerySize(), _ReceiveBuffer->GetBuffer(), _MessageLength);

        _ReceiveMessageOperation->Reuse();

        ProcessReceivedMessage();
    }

    VOID
    DoFragmentSend()
    {
        ULONG const remaining = _MessageLength - _SendOffset;
        ULONG const sendFragmentSize = (rand() % remaining) + 1;
        KInvariant(sendFragmentSize > 0 && sendFragmentSize <= remaining);

        _SendFragmentOperation->StartSendFragment(
            *_SendBuffer,
            sendFragmentSize == remaining,
            _MessageContentType,
            this,
            KAsyncContextBase::CompletionCallback(this, &WebSocketEchoHandler::WebSocketSendFragmentCallback),
            _SendOffset,
            sendFragmentSize
            );

        _SendOffset += sendFragmentSize;
    }

    VOID
    WebSocketSendFragmentCallback(
        KAsyncContextBase* const ParentAsync,
        KAsyncContextBase& SubOp
        )
    {
        KInvariant(&SubOp == _SendFragmentOperation.RawPtr());
        KInvariant(ParentAsync == this);

        NTSTATUS status = SubOp.Status();

        if (! NT_SUCCESS(status))  //  A send should never fail under normal circumstances
        {
            KTestPrintf("Server send fragment operation failed, calling StartCloseWebSocket()");
            KTraceFailedAsyncRequest(status, this, NULL, NULL);
            _WebSocket->StartCloseWebSocket(
                this,
                KAsyncServiceBase::CloseCompletionCallback(this, &WebSocketEchoHandler::WebSocketCloseCallback)
                );
            return;
        }

        _SendFragmentOperation->Reuse();

        if (_SendOffset < _MessageLength)
        {
            DoFragmentSend();
            return;
        }
        else
        {
            KTestPrintf("Final fragment sent.  Posting the next receive operation.");
            StartReceive();
        }
    }

    VOID
    WebSocketSendMessageCallback(
        KAsyncContextBase* const ParentAsync,
        KAsyncContextBase& SubOp
        )
    {
        KInvariant(&SubOp == _SendMessageOperation.RawPtr());
        KInvariant(ParentAsync == this);

        NTSTATUS status = SubOp.Status();

        if (! NT_SUCCESS(status))  //  A send should never fail under normal circumstances
        {
            KTestPrintf("Server send message operation failed, calling StartCloseWebSocket()");
            KTraceFailedAsyncRequest(status, this, NULL, NULL);
            _WebSocket->StartCloseWebSocket(
                this,
                KAsyncServiceBase::CloseCompletionCallback(this, &WebSocketEchoHandler::WebSocketCloseCallback)
                );
            return;
        }

        _SendMessageOperation->Reuse();

        KTestPrintf("Echo message successfully sent. Posting the next receive operation.");
        StartReceive();
    }

private:

    KWebSocket::MessageContentType _MessageContentType;
    ULONG _MessageLength;
    ULONG _SendOffset;
    ULONG _ReceiveOffset;
    KBuffer::SPtr _ReceiveBuffer;
    KBuffer::SPtr _SendBuffer;
    KWebSocket::ReceiveFragmentOperation::SPtr _ReceiveFragmentOperation;
    KWebSocket::ReceiveMessageOperation::SPtr _ReceiveMessageOperation;
    KWebSocket::SendMessageOperation::SPtr _SendMessageOperation;
    KWebSocket::SendFragmentOperation::SPtr _SendFragmentOperation;
    KHttpServerWebSocket::SPtr _WebSocket;
    KHttpServer::SPtr _ParentServer;
    KHttpServerRequest::SPtr _ServerRequest;
    KTimer::SPtr _Timer;
};

CHAR const * const WebSocketEchoHandler::ShutdownString = "Server should shutdown";
CHAR const * const WebSocketEchoHandler::AbortString = "Server should Abort()";

WebSocketEchoHandler::WebSocketEchoHandler()
{
}
WebSocketEchoHandler::~WebSocketEchoHandler()
{
}

struct WebSocketRequestHandler
{
public:
    WebSocketRequestHandler(KHttpServer::SPtr ParentServer, KStringView QueryStr = L"")
    {
        _ParentServer = ParentServer;
        _ExpectedQueryStr = QueryStr;
    }

    VOID
    CloseCallback(
        KAsyncContextBase* const ParentAsync,    // Parent; can be nullptr
        KAsyncContextBase& SubOp                 // CompletingSubOp
        )
    {
        UNREFERENCED_PARAMETER(SubOp);
        KInvariant(ParentAsync == _ParentServer.RawPtr());
    }

    VOID
    Handler(
        __in KHttpServerRequest::SPtr Request
        )
    {
        NTSTATUS status;
        KString::SPtr upgradeValue;
        WebSocketEchoHandler::SPtr echoHandler;

        status = Request->GetRequestHeaderByCode(KHttpUtils::HttpHeaderType::HttpHeaderUpgrade, upgradeValue);
        if (!NT_SUCCESS(status) || upgradeValue->CompareNoCase(KStringView(L"websocket")) != 0)
        {
            Request->SendResponse(KHttpUtils::HttpResponseCode::Forbidden, "Only WebSocket connections are supported on this server.");
            return;
        }
        
        if (!_ExpectedQueryStr.IsEmpty())
        {
            KUri::SPtr Url;
            Request->GetUrl(Url);
            KStringView& QueryStr = Url->Get(KUri::eQueryString);

            if (QueryStr.Length() == 0 || _ExpectedQueryStr.Compare(QueryStr) != 0)
            {
                Request->SendResponse(KHttpUtils::HttpResponseCode::BadRequest, "Query parameters not found in the request.");
                return;
            }
        }

        status = WebSocketEchoHandler::Create(echoHandler);
        if (! NT_SUCCESS(status))
        {
            KStringView headerId(L"Error.OOM");
            KStringView value(L"OOM when creating WebSocket handler");
            Request->SetResponseHeader(headerId, value);
            Request->SendResponse(KHttpUtils::HttpResponseCode::InternalServerError, "Out of memory when calling WebSocketEchoHandler::Create");
            return;
        }

        echoHandler->StartEchoWebSocket(
            KAsyncContextBase::CompletionCallback(this, &WebSocketRequestHandler::CloseCallback),
            *Request,
            *_ParentServer
            );
    }

private:
    KHttpServer::SPtr _ParentServer;
    KStringView _ExpectedQueryStr;
};

NTSTATUS
RunTimedWebSocketEchoServer(
    __in LPCWSTR Url,
    __in ULONG Seconds
    )
{
    QuickTimer t;
    KHttpServer::SPtr Server;
    

    KEvent OpenEvent;
    KEvent ShutdownEvent;
    p_ServerOpenEvent = &OpenEvent;
    p_ServerShutdownEvent = &ShutdownEvent;

    NTSTATUS Res = KHttpServer::Create(*g_Allocator, Server);
    if (!NT_SUCCESS(Res))
    {
        if (Res == STATUS_ACCESS_DENIED)
        {
            KTestPrintf("ERROR: Server must run under admin privileges.\n");
        }
        return Res;
    }

    KUriView ListenUrl(Url);
    KTestPrintf("Running server on URL = %S\n", Url);

    WebSocketRequestHandler webSocketRequestHandler(Server);

    Res = Server->StartOpen(
        ListenUrl,
        KHttpServer::RequestHandler(&webSocketRequestHandler, &WebSocketRequestHandler::Handler),
        3,
        8192,
        8192,
        0,
        ServerOpenCallback,
        nullptr,
        KHttpUtils::HttpAuthType::AuthNone,
        65536
        );
    if (! NT_SUCCESS(Res))
    {
        if (Res == STATUS_INVALID_ADDRESS)
        {
            KTestPrintf("ERROR: URL is bad or missing the port number, which is required for HTTP.SYS based servers\n");
        }
        return Res;
    }

    OpenEvent.WaitUntilSet();
    KInvariant(Server->Status() == STATUS_PENDING);
    KTestPrintf("Server is running.\n");

    for (;;)
    {
        KNt::Sleep(5000);
        if (t.HasElapsed(Seconds))
        {
            break;
        }
    }

    KTestPrintf("...Deactivating server\n");
    Server->StartClose(0, ServerShutdownCallback);

    ShutdownEvent.WaitUntilSet();

    return Server->Status();
}

class KWebSocketReceiveCloseSynchronizer : private KSynchronizer
{
public:

    using KSynchronizer::WaitForCompletion;

    KWebSocketReceiveCloseSynchronizer()
    {
        _CloseReceivedCallback.Bind(this, &KWebSocketReceiveCloseSynchronizer::CloseReceived);
    }

    operator KWebSocket::WebSocketCloseReceivedCallback()
    {
        return _CloseReceivedCallback;
    }

private:

    VOID
    CloseReceived(
        __in_opt KWebSocket& WebSocket
        )
    {
        AsyncCompletion(nullptr, WebSocket);
    }

private:

    KWebSocket::WebSocketCloseReceivedCallback _CloseReceivedCallback;
};

class TestKHttpServer : public KHttpServer
{
    K_FORCE_SHARED(TestKHttpServer);

public:

    static NTSTATUS
    Create(
        __out KHttpServer::SPtr& Server,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_TEST
        )
    {
        KHttpServer::SPtr server;

        Server = nullptr;

        server = _new(AllocationTag, Allocator) TestKHttpServer();
        if (! server)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Server = Ktl::Move(server);
        return STATUS_SUCCESS;
    }

    using KAsyncServiceBase::StartOpen;
    using KAsyncServiceBase::StartClose;

private:

    VOID
    OnServiceOpen() override
    {
        KAsyncServiceBase::OnServiceOpen();
    }

    VOID
    OnServiceClose() override
    {
        KAsyncServiceBase::OnServiceClose();
    }

    VOID
    OnCompleted() override
    {
        KAsyncServiceBase::OnCompleted();
    }

    VOID
    ReadCompletion(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        ) override
    {
        KInvariant(Parent == this);
        UNREFERENCED_PARAMETER(Op);
        //  No-op
    }

    VOID
    WebSocketCompletion(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        ) override
    {
        KInvariant(Parent == this);
        UNREFERENCED_PARAMETER(Op);
        ReleaseActivities(); // Release activities acquired in StartOpenWebSocket()
    }
};

TestKHttpServer::TestKHttpServer()
{
}

TestKHttpServer::~TestKHttpServer()
{
}

class TestKHttpServerRequest : public KHttpServerRequest
{
    K_FORCE_SHARED(TestKHttpServerRequest);

public:

    static KDefineSingleArgumentCreate(KHttpServerRequest, TestKHttpServerRequest, ServerRequest, KSharedPtr<KHttpServer>, ParentServer);

    using KAsyncContextBase::Start;
    using KAsyncContextBase::Complete;

    virtual VOID
    OnStart() override
    {
        KAsyncContextBase::OnStart();
    }

    VOID
    SetPHttpRequest(
        __in PHTTP_REQUEST_V2 PHttpRequest
        )
    {
        _PHttpRequest = PHttpRequest;
    }

private:

    FAILABLE
    TestKHttpServerRequest(
        __in KSharedPtr<KHttpServer> ParentServer
        );
};

TestKHttpServerRequest::TestKHttpServerRequest(
    __in KSharedPtr<KHttpServer> ParentServer
    )
    :
        KHttpServerRequest(ParentServer)
{
}

TestKHttpServerRequest::~TestKHttpServerRequest()
{
}


class InMemoryTransport;

class TestKHttpServerWebSocket : public KHttpServerWebSocket
{
    K_FORCE_SHARED(TestKHttpServerWebSocket);
    friend class InMemoryTransport;

public:

    static NTSTATUS
    Create(
        __out TestKHttpServerWebSocket::SPtr& WebSocket,
        __in InMemoryTransport& Transport,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag
        );

private:

    TestKHttpServerWebSocket(
        __in KAsyncQueue<WebSocketReceiveOperation>& ReceiveOperationQueue,
        __in KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation& ReceiveRequestDequeueOperation,
        __in KAsyncQueue<WebSocketOperation>& SendOperationQueue,
        __in KAsyncQueue<WebSocketOperation>::DequeueOperation& SendRequestDequeueOperation,
        __in HttpReceiveEntityBodyOperation& TransportReceiveOperation,
        __in HttpSendEntityBodyOperation& TransportSendOperation,
        __in HttpSendResponseOperation& SendResponseOperation,
        __in KHttpServerSendCloseOperation& SendCloseOperation,
        __in KHttpUtils::CallbackAsync& InformOfClosureAsync,
        __in KHttpUtils::CallbackAsync& AbortAsync,
        __in CloseTransportOperation& CloseTransportOperation,
        __in KStringA& ActiveSubprotocol,
        __in KBuffer& RemoteCloseReasonBuffer
        )
        :
            KHttpServerWebSocket(
                ReceiveOperationQueue,
                ReceiveRequestDequeueOperation,
                SendOperationQueue,
                SendRequestDequeueOperation,
                TransportReceiveOperation,
                TransportSendOperation,
                SendResponseOperation,
                SendCloseOperation,
                InformOfClosureAsync,
                AbortAsync,
                CloseTransportOperation,
                ActiveSubprotocol,
                RemoteCloseReasonBuffer
            )
    {
    }
};


template<class IType, ULONG NPriorities = 1L>
class TestSharedAsyncQueue : public KAsyncQueue<IType, NPriorities>
{
    K_FORCE_SHARED(TestSharedAsyncQueue);

public:

    class TestSharedDequeueOperation;

    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __in ULONG const ITypeLinkOffset,
        __out KSharedPtr<TestSharedAsyncQueue<IType, NPriorities>>& Context
        )
    {
        Context = _new(AllocationTag, Allocator) TestSharedAsyncQueue<IType, NPriorities>(AllocationTag, ITypeLinkOffset);
        if (!Context)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NTSTATUS status = Context->Status();
        if (!NT_SUCCESS(status))
        {
            Context.Reset();
        }
        return status;
    }

    NTSTATUS
    Enqueue(
        __in IType& ToEnqueue,
        __in ULONG Priority = (NPriorities - 1)
        )
    {
        NTSTATUS status;

        status = KAsyncQueue<IType, NPriorities>::Enqueue(ToEnqueue, Priority);

        if (NT_SUCCESS(status))
        {
            ToEnqueue.AddRef();
        }

        return status;
    }

    NTSTATUS
    CreateDequeueOperation(__out KSharedPtr<TestSharedDequeueOperation>& Context)
    {
        NTSTATUS status = STATUS_SUCCESS;

        Context = _new(_AllocationTag, GetThisAllocator()) TestSharedDequeueOperation(*this, _ThisVersion);
        if (!Context)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = Context->Status();
        if (!NT_SUCCESS(status))
        {
            Context.Reset();
        }

        return status;
    }

public:

    class TestSharedDequeueOperation : public TestSharedAsyncQueue<IType, NPriorities>::DequeueOperation
    {
        K_FORCE_SHARED(TestSharedDequeueOperation);

        friend class TestSharedAsyncQueue;

    public:

        KSharedPtr<IType>
        GetDequeuedSharedItem()
        {
            KSharedPtr<IType> sp;
            sp.Attach(_Result);
            _Result = nullptr;
            return sp;
        }

    private:

        TestSharedDequeueOperation(
            __in TestSharedAsyncQueue<IType, NPriorities>& Queue,
            __in ULONG Version
            )
            :
                KAsyncQueue<IType, NPriorities>::DequeueOperation(Queue, Version)
        {
        }

        using KAsyncQueue<IType, NPriorities>::DequeueOperation::GetDequeuedItem;
    };

private:

    TestSharedAsyncQueue(
        __in ULONG AllocationTag,
        __in ULONG ITypeLinkOffset
        )
        :
            KAsyncQueue<IType, NPriorities>(AllocationTag, ITypeLinkOffset)
    {
    }

    
};

template <class IType, ULONG NPriorities>
TestSharedAsyncQueue<IType, NPriorities>::~TestSharedAsyncQueue()
{
}


template <class IType, ULONG NPriorities>
TestSharedAsyncQueue<IType, NPriorities>::TestSharedDequeueOperation::~TestSharedDequeueOperation()
{
}


class InMemoryTransport : public KShared<InMemoryTransport>, public KObject<InMemoryTransport>
{
    K_FORCE_SHARED(InMemoryTransport);

public:

    class DataChunk : public KShared<DataChunk>, public KObject<DataChunk>
    {
        K_FORCE_SHARED(DataChunk);

    public:

        KDeclareDefaultCreate(DataChunk, Chunk, KTL_TAG_TEST);

        KBuffer::SPtr _Data;
        ULONG _Consumed;
        KListEntry _Links;
    };

    class CloseTransportOperation : public TestKHttpServerWebSocket::CloseTransportOperation
    {
        K_FORCE_SHARED(CloseTransportOperation);

    public:

        static KDefineSingleArgumentCreate(
            TestKHttpServerWebSocket::CloseTransportOperation,
            InMemoryTransport::CloseTransportOperation,
            Operation,
            InMemoryTransport&,
            Transport
            );

        VOID
        StartCloseTransport(
            __in KHttpServer& ParentServer,
            __in BOOLEAN IsGracefulClose,
            __in HANDLE RequestQueueHandle,
            __in HTTP_REQUEST_ID RequestId
            ) override;

        using KAsyncContextBase::Complete;

    private:

        CloseTransportOperation(
            __in InMemoryTransport& Transport
            )
            :
                _Transport(Transport)
        {
        }

        InMemoryTransport& _Transport;
    };

    class SendResponseOperation : public TestKHttpServerWebSocket::HttpSendResponseOperation
    {
        K_FORCE_SHARED(SendResponseOperation);

    public:

        static KDefineSingleArgumentCreate(
            HttpSendResponseOperation,
            SendResponseOperation,
            Operation,
            InMemoryTransport&,
            Transport
            );

        using KAsyncContextBase::Complete;

        VOID
        StartSend(
            __in KHttpServerWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HANDLE const & RequestQueueHandle,
            __in HTTP_REQUEST_ID const & HttpRequestId,
            __in HTTP_RESPONSE& HttpResponse
            ) override;

    private:

        SendResponseOperation(
            __in InMemoryTransport& Transport
            )
            :
                _Transport(Transport)
        {
        }

        InMemoryTransport& _Transport;
    };

    class ReceiveOperation : public TestKHttpServerWebSocket::HttpReceiveEntityBodyOperation
    {
        K_FORCE_SHARED(ReceiveOperation);

    public:

        static KDefineSingleArgumentCreate(
            HttpReceiveEntityBodyOperation,
            ReceiveOperation,
            Operation,
            InMemoryTransport&,
            Transport
            );

        VOID
        StartReceive(
            __in KHttpServerWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HANDLE const & RequestQueueHandle,
            __in HTTP_REQUEST_ID const & HttpRequestId,
            __in PBYTE const & Buffer,
            __in ULONG const & BufferSizeInBytes,
            __in PVOID const & ActionContext
            ) override
        {
            UNREFERENCED_PARAMETER(RequestQueueHandle);
            UNREFERENCED_PARAMETER(HttpRequestId);

            KInvariant(ParentWebSocket.IsInApartment());
            KInvariant(CallbackPtr);

            _ActionContext = ActionContext;

            Start(&ParentWebSocket, CallbackPtr);
            SetActive(TRUE);

            _Buffer = Buffer;
            _BufferSizeInBytes = BufferSizeInBytes;

            _Transport._ServerReceiveRequestQueue->Enqueue(*this);
        }

        using KAsyncContextBase::Complete;

        ReceiveOperation(
            __in InMemoryTransport& Transport
            )
            :
                _Transport(Transport)
        {
        }

        PBYTE _Buffer;
        ULONG _BufferSizeInBytes;
        InMemoryTransport& _Transport;
        KListEntry _Links;
    };

    class SendOperation : public TestKHttpServerWebSocket::HttpSendEntityBodyOperation
    {
        K_FORCE_SHARED(SendOperation);

    public:

        static KDefineSingleArgumentCreate(
            HttpSendEntityBodyOperation,
            SendOperation,
            Operation,
            InMemoryTransport&,
            Transport
            );

        VOID
        HandleSend()
        {
            InMemoryTransport::DataChunk::SPtr chunk;
            NTSTATUS status = InMemoryTransport::DataChunk::Create(chunk, GetThisAllocator());
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, 0, 0);
                Complete(status);
                return;
            }

            status = KBuffer::Create(_BufferSizeInBytes, chunk->_Data, GetThisAllocator());
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, 0, 0);
                Complete(status);
                return;
            }

            KInvariant(chunk->_Data->QuerySize() >= _BufferSizeInBytes);
            KMemCpySafe(chunk->_Data->GetBuffer(), chunk->_Data->QuerySize(), _Buffer, _BufferSizeInBytes);

            status = _Transport._ServerDataQueue->Enqueue(*chunk);
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, 0, 0);
                Complete(status);
                return;
            }

            Complete(STATUS_SUCCESS);
        }

        VOID
        StartSend(
            __in KHttpServerWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HANDLE RequestQueueHandle,
            __in HTTP_REQUEST_ID HttpRequestId,
            __in PBYTE Buffer,
            __in ULONG BufferSizeInBytes,
            __in PVOID ActionContext
            ) override
        {
            KInvariant(ParentWebSocket.IsInApartment());

            UNREFERENCED_PARAMETER(RequestQueueHandle);
            UNREFERENCED_PARAMETER(HttpRequestId);

            KInvariant(! IsActive());
            SetActive(TRUE);
            Start(&ParentWebSocket, CallbackPtr);

            _Buffer = Buffer;
            _BufferSizeInBytes = BufferSizeInBytes;
            _ActionContext = ActionContext;
            
            if (_Transport._TransportSendDelay != 0)
            {
                KTimer::SPtr timer;
                KInvariant(NT_SUCCESS(KTimer::Create(timer, GetThisAllocator(), KTL_TAG_TEST)));

                timer->StartTimer(
                    _Transport._TransportSendDelay,
                    nullptr,
                    KAsyncContextBase::CompletionCallback(this, &InMemoryTransport::SendOperation::SendDelayTimerCallback)
                    );

                return;
            }
            else
            {
                HandleSend();
            }
        }

        VOID
        SendDelayTimerCallback(
            __in KAsyncContextBase*,
            __in KAsyncContextBase&
            )
        {
            HandleSend();
        }


        SendOperation(
            __in InMemoryTransport& Transport
            )
            :
                _Transport(Transport)
        {
        }

        ULONG _BufferSizeInBytes;
        PBYTE _Buffer;

        InMemoryTransport& _Transport;
    };
    
    VOID
    HandleServerResponse(
        __in HTTP_RESPONSE& Response,
        __in SendResponseOperation& Op
        )
    {
        UNREFERENCED_PARAMETER(Response);

        if (_TransportSendDelay != 0)
        {
            KTimer::SPtr timer;
            KInvariant(NT_SUCCESS(KTimer::Create(timer, GetThisAllocator(), KTL_TAG_TEST)));

            _SendResponseOp = &Op;
            timer->StartTimer(
                _TransportSendDelay,
                nullptr,
                KAsyncContextBase::CompletionCallback(this, &InMemoryTransport::SendResponseDelayTimerCallback)
                );

            return;
        }

        Op.Complete(_SendResponseSuccess ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
    }

    VOID
    SendResponseDelayTimerCallback(
        __in KAsyncContextBase*,
        __in KAsyncContextBase&
        )
    {
        _SendResponseOp->Complete(_SendResponseSuccess ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
    }

    VOID
    CloseTransport(
        __in BOOLEAN IsGracefulClose
        )
    {
        if (! _IsServerDataQueueDeactivated)
        {
            _ServerDataQueue->Deactivate();
            _IsServerDataQueueDeactivated = TRUE;
        }
        _ServerDataQueue->CancelAllEnqueued(KAsyncQueue<DataChunk>::DroppedItemCallback(&InMemoryTransport::HandleDroppedDataChunk));

        if (! _IsServerReceiveOperationQueueDeactivated)
        {
            _ServerReceiveRequestQueue->Deactivate();
            _IsServerReceiveOperationQueueDeactivated = TRUE;
        }
        if (! IsGracefulClose)
        {
            _ServerReceiveRequestQueue->CancelAllEnqueued(KAsyncQueue<ReceiveOperation>::DroppedItemCallback(&InMemoryTransport::HandleDroppedReceiveOperation));
        }
    }

    static VOID
    HandleDroppedDataChunk(
        __in KAsyncQueue<DataChunk>&,
        __in DataChunk& Chunk
        )
    {
        Chunk.Release();
    }

    static VOID
    HandleDroppedReceiveOperation(
        __in KAsyncQueue<ReceiveOperation>&,
        __in ReceiveOperation& Operation
        )
    {
        Operation.Complete(STATUS_REQUEST_ABORTED);
    }

    static NTSTATUS
    Create(
        __out InMemoryTransport::SPtr& Transport,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_TEST
        )
    {
        NTSTATUS status;
        InMemoryTransport::SPtr transport;

        Transport = nullptr;

        TestSharedAsyncQueue<DataChunk>::SPtr serverDataQueue;
        status = TestSharedAsyncQueue<DataChunk>::Create(
            AllocationTag,
            Allocator,
            FIELD_OFFSET(DataChunk, _Links),
            serverDataQueue
            );
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
            return status;
        }

        KAsyncQueue<ReceiveOperation>::SPtr serverReceiveRequestQueue;
        status = KAsyncQueue<ReceiveOperation>::Create(
            AllocationTag,
            Allocator,
            FIELD_OFFSET(ReceiveOperation, _Links),
            serverReceiveRequestQueue
            );
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
            return status;
        }

        transport = _new(AllocationTag, Allocator) InMemoryTransport(*serverDataQueue, *serverReceiveRequestQueue);

        if (! transport)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
            return status;
        }

        status = transport->_ServerDataQueue->Activate(nullptr, nullptr);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        transport->_ServerReceiveRequestQueue->Activate(nullptr, nullptr);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        Transport = Ktl::Move(transport);

        return STATUS_SUCCESS;
    }

    InMemoryTransport(
        __in TestSharedAsyncQueue<DataChunk>& ServerDataQueue,
        __in KAsyncQueue<ReceiveOperation>& ServerReceiveRequestQueue
        )
        :
            _ServerDataQueue(&ServerDataQueue),
            _ServerReceiveRequestQueue(&ServerReceiveRequestQueue),

            _SendResponseSuccess(TRUE),
            _SendResponseOp(nullptr),
            _TransportSendDelay(0),

            _IsServerDataQueueDeactivated(FALSE),
            _IsServerReceiveOperationQueueDeactivated(FALSE)
    {
    }

    TestSharedAsyncQueue<DataChunk>::SPtr _ServerDataQueue;
    KAsyncQueue<ReceiveOperation>::SPtr _ServerReceiveRequestQueue;

    BOOLEAN _SendResponseSuccess;
    SendResponseOperation* _SendResponseOp;
    ULONG _TransportSendDelay;

    BOOLEAN _IsServerDataQueueDeactivated;
    BOOLEAN _IsServerReceiveOperationQueueDeactivated;
};

VOID
InMemoryTransport::SendResponseOperation::StartSend(
    __in KHttpServerWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HANDLE const & RequestQueueHandle,
    __in HTTP_REQUEST_ID const & HttpRequestId,
    __in HTTP_RESPONSE& HttpResponse
    )
{
    UNREFERENCED_PARAMETER(RequestQueueHandle);
    UNREFERENCED_PARAMETER(HttpRequestId);

    Start(&ParentWebSocket, CallbackPtr);
    _Transport.HandleServerResponse(HttpResponse, *this);
}

VOID
InMemoryTransport::CloseTransportOperation::StartCloseTransport(
    __in KHttpServer& ParentServer,
    __in BOOLEAN IsGracefulClose,
    __in HANDLE RequestQueueHandle,
    __in HTTP_REQUEST_ID RequestId
    )
{
    UNREFERENCED_PARAMETER(RequestQueueHandle);
    UNREFERENCED_PARAMETER(RequestId);

    Start(&ParentServer, nullptr);
    _Transport.CloseTransport(IsGracefulClose);
    Complete(STATUS_SUCCESS);
}

InMemoryTransport::~InMemoryTransport()
{
    CloseTransport(FALSE);
}

KDefineDefaultCreate(InMemoryTransport::DataChunk, Chunk);

InMemoryTransport::DataChunk::DataChunk()
    :
        _Data(nullptr),
        _Consumed(0)
{
}

InMemoryTransport::DataChunk::~DataChunk()
{
}

InMemoryTransport::SendOperation::~SendOperation()
{
}

InMemoryTransport::ReceiveOperation::~ReceiveOperation()
{
}

InMemoryTransport::CloseTransportOperation::~CloseTransportOperation()
{
}


InMemoryTransport::SendResponseOperation::~SendResponseOperation()
{
}


NTSTATUS
TestKHttpServerWebSocket::Create(
    __out TestKHttpServerWebSocket::SPtr& WebSocket,
    __in InMemoryTransport& Transport,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag = KTL_TAG_TEST
    )
{
    NTSTATUS status;
    TestKHttpServerWebSocket::SPtr webSocket;

    WebSocket = nullptr;

    KAsyncQueue<WebSocketReceiveOperation>::SPtr receiveOperationQueue;
    status = KAsyncQueue<WebSocketReceiveOperation>::Create(
        AllocationTag,
        Allocator,
        FIELD_OFFSET(WebSocketReceiveOperation, _Links),
        receiveOperationQueue
        );
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation::SPtr receiveDequeueOperation;
    status = receiveOperationQueue->CreateDequeueOperation(receiveDequeueOperation);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    HttpReceiveEntityBodyOperation::SPtr transportReceiveOperation;
    status = InMemoryTransport::ReceiveOperation::Create(transportReceiveOperation, Transport, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KAsyncQueue<WebSocketOperation>::SPtr sendOperationQueue;
    status = KAsyncQueue<WebSocketOperation>::Create(
        AllocationTag,
        Allocator,
        FIELD_OFFSET(WebSocketOperation, _Links),
        sendOperationQueue
        );
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KAsyncQueue<WebSocketOperation>::DequeueOperation::SPtr sendDequeueOperation;
    status = sendOperationQueue->CreateDequeueOperation(sendDequeueOperation);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    HttpSendEntityBodyOperation::SPtr transportSendOperation;
    status = InMemoryTransport::SendOperation::Create(transportSendOperation, Transport, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    HttpSendResponseOperation::SPtr sendResponseOperation;
    status = InMemoryTransport::SendResponseOperation::Create(sendResponseOperation, Transport, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KHttpServerSendCloseOperation::SPtr sendCloseOperation;
    status = KHttpServerSendCloseOperation::Create(sendCloseOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KHttpUtils::CallbackAsync::SPtr informOfClosureAsync;
    status = KHttpUtils::CallbackAsync::Create(informOfClosureAsync, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KHttpUtils::CallbackAsync::SPtr abortAsync;
    status = KHttpUtils::CallbackAsync::Create(abortAsync, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    CloseTransportOperation::SPtr closeTransportOperation;
    status = InMemoryTransport::CloseTransportOperation::Create(closeTransportOperation, Transport, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }


    KStringA::SPtr selectedSubprotocol;
    status = KStringA::Create(selectedSubprotocol, Allocator, "", TRUE);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KBuffer::SPtr remoteCloseReasonBuffer;
    status = KBuffer::Create(MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES, remoteCloseReasonBuffer, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    webSocket = _new(AllocationTag, Allocator) TestKHttpServerWebSocket(
        *receiveOperationQueue,
        *receiveDequeueOperation,
        *sendOperationQueue,
        *sendDequeueOperation,
        *transportReceiveOperation,
        *transportSendOperation,
        *sendResponseOperation,
        *sendCloseOperation,
        *informOfClosureAsync,
        *abortAsync,
        *closeTransportOperation,
        *selectedSubprotocol,
        *remoteCloseReasonBuffer
        );
  
    if (! webSocket)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    if (! NT_SUCCESS(webSocket->Status()))
    {
        return webSocket->Status();
    }

    WebSocket = Ktl::Move(webSocket);

    return STATUS_SUCCESS;
}

TestKHttpServerWebSocket::~TestKHttpServerWebSocket()
{
}

static NTSTATUS
InitializeRequestHeaders(
    __out HTTP_REQUEST_V2& HttpRequest,
    __out KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>>& UnknownHeaders
    )
{
    CHAR const * hostHeader = "localhost";
    CHAR const * upgradeHeader = "websocket";
    CHAR const * connectionHeader = "Upgrade";
    CHAR const * websocketKeyHeader = "x3JJHMbDL1EzLkh9GBhXDw==";  //  Arbitrary value
    CHAR const * websocketSubprotocolHeader = "echo";
    CHAR const * webSocketVersionHeader = "13";

    RtlZeroMemory(&HttpRequest, sizeof HttpRequest);
    UnknownHeaders = nullptr;

    UnknownHeaders = _newArray<HTTP_UNKNOWN_HEADER>(KTL_TAG_TEST, *g_Allocator, 3);
    if (! UnknownHeaders)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    HttpRequest.Headers.KnownHeaders[KHttpUtils::HttpHeaderType::HttpHeaderHost].pRawValue = hostHeader;
    HttpRequest.Headers.KnownHeaders[KHttpUtils::HttpHeaderType::HttpHeaderHost].RawValueLength = static_cast<USHORT>(strlen(hostHeader));

    HttpRequest.Headers.KnownHeaders[KHttpUtils::HttpHeaderType::HttpHeaderUpgrade].pRawValue = upgradeHeader;
    HttpRequest.Headers.KnownHeaders[KHttpUtils::HttpHeaderType::HttpHeaderUpgrade].RawValueLength = static_cast<USHORT>(strlen(upgradeHeader));

    HttpRequest.Headers.KnownHeaders[KHttpUtils::HttpHeaderType::HttpHeaderConnection].pRawValue = connectionHeader;
    HttpRequest.Headers.KnownHeaders[KHttpUtils::HttpHeaderType::HttpHeaderConnection].RawValueLength = static_cast<USHORT>(strlen(connectionHeader));

        
    UnknownHeaders[0].pName = WebSocketHeaders::KeyA;
    UnknownHeaders[0].NameLength = static_cast<USHORT>(strlen(WebSocketHeaders::KeyA));
    UnknownHeaders[0].pRawValue = websocketKeyHeader;
    UnknownHeaders[0].RawValueLength = static_cast<USHORT>(strlen(websocketKeyHeader));

    UnknownHeaders[1].pName = WebSocketHeaders::ProtocolA;
    UnknownHeaders[1].NameLength = static_cast<USHORT>(strlen(WebSocketHeaders::ProtocolA));
    UnknownHeaders[1].pRawValue = websocketSubprotocolHeader;
    UnknownHeaders[1].RawValueLength = static_cast<USHORT>(strlen(websocketSubprotocolHeader));

    UnknownHeaders[2].pName = WebSocketHeaders::VersionA;
    UnknownHeaders[2].NameLength = static_cast<USHORT>(strlen(WebSocketHeaders::VersionA));
    UnknownHeaders[2].pRawValue = webSocketVersionHeader;
    UnknownHeaders[2].RawValueLength = static_cast<USHORT>(strlen(webSocketVersionHeader));

    HttpRequest.Headers.pUnknownHeaders = UnknownHeaders;
    HttpRequest.Headers.UnknownHeaderCount = 3;

    return STATUS_SUCCESS;
}

static NTSTATUS
WriteControlFrame(
    __inout KBuffer& Buffer,
    __in BYTE OpCode,
    __out ULONG& FrameLength
    )
{
    FrameLength = 6; // One for opcode and FIN, one for MASK and payload lenght, four for masking key
    if(Buffer.QuerySize() < FrameLength)
    {
        return STATUS_UNSUCCESSFUL;
    }

    PBYTE bytePtr = static_cast<PBYTE>(Buffer.GetBuffer());

    bytePtr[0] = 0x80 | OpCode; // FIN=1, connection close opcode
    bytePtr[1] = 0x80; // MASK=1, no payload
    //  let masking key be whatever the contents of memory are, we aren't using it

    return STATUS_SUCCESS;
}

static NTSTATUS
InMemTestSetup(
    __out KHttpServer::SPtr& Server,
    __out KHttpServerRequest::SPtr& ServerRequest,
    __out HTTP_REQUEST_V2& HttpRequest,
    __out KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>>& UnknownHeaders,
    __out InMemoryTransport::SPtr& Transport,
    __out TestKHttpServerWebSocket::SPtr& WebSocket
    )
{
    KServiceSynchronizer serviceSync;
    NTSTATUS status;

    KHttpServer::SPtr server;
    status = TestKHttpServer::Create(server, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    status = ((TestKHttpServer&) *server).StartOpen(nullptr, serviceSync.OpenCompletionCallback());
    if (! NT_SUCCESS(status))
    {
        return status;
    }
    
    status = serviceSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    //
    //  Set the request headers
    //
    status = InitializeRequestHeaders(HttpRequest, UnknownHeaders);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to initialize request headers.");
        return status;
    }

    KHttpServerRequest::SPtr serverRequest;
    status = TestKHttpServerRequest::Create(serverRequest, server, *g_Allocator, KTL_TAG_TEST);
    if (! NT_SUCCESS(status))
    {
        return status;
    }
    ((TestKHttpServerRequest&) *serverRequest).SetPHttpRequest(&HttpRequest);
    ((TestKHttpServerRequest&) *serverRequest).Start(nullptr, nullptr);


    InMemoryTransport::SPtr transport;
    status = InMemoryTransport::Create(transport, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    TestKHttpServerWebSocket::SPtr serverWebSocket;
    status = TestKHttpServerWebSocket::Create(serverWebSocket, *transport, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    Server = Ktl::Move(server);
    ServerRequest = Ktl::Move(serverRequest);
    Transport = Ktl::Move(transport);
    WebSocket = Ktl::Move(serverWebSocket);

    return STATUS_SUCCESS;
}

//
//  Read the next control frame from the in-memory transport.  ONLY supports reading control frames and assumes the given buffer
//  is big enough to hold a max-size control frame (to simplify)
//
static NTSTATUS
GetNextControlFrame(
    __in TestSharedAsyncQueue<InMemoryTransport::DataChunk>::TestSharedDequeueOperation& DequeueOp,
    __in KBuffer& Buffer,
    __out ULONG& FrameLength,
    __inout_opt InMemoryTransport::DataChunk::SPtr& PartialDataChunk
    )
{
    KSynchronizer sync;
    NTSTATUS status;

    PBYTE bytePtr = static_cast<PBYTE>(Buffer.GetBuffer());

    if (Buffer.QuerySize() < 127)  //  maximum server-side frame: 2 byte header and 125-byte payload
    {
        return K_STATUS_BUFFER_TOO_SMALL;
    }

    if (!PartialDataChunk || PartialDataChunk->_Consumed == PartialDataChunk->_Data->QuerySize())
    {
        DequeueOp.StartDequeue(nullptr, sync);
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("InMemory server data dequeue failed after 20 seconds.");
            return status;
        }

        PartialDataChunk = DequeueOp.GetDequeuedSharedItem();
        KInvariant(PartialDataChunk);
        KInvariant(PartialDataChunk->_Consumed == 0);
        KInvariant(PartialDataChunk->_Data);
        KInvariant(PartialDataChunk->_Data->QuerySize() >= 1);

        DequeueOp.Reuse();
    }

    bytePtr[0] = static_cast<PBYTE>(PartialDataChunk->_Data->GetBuffer())[PartialDataChunk->_Consumed];
    PartialDataChunk->_Consumed++;

    if (PartialDataChunk->_Consumed == PartialDataChunk->_Data->QuerySize())
    {
        DequeueOp.StartDequeue(nullptr, sync);
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("InMemory server data dequeue failed after 20 seconds.");
            return status;
        }

        PartialDataChunk = DequeueOp.GetDequeuedSharedItem();
        KInvariant(PartialDataChunk);
        KInvariant(PartialDataChunk->_Consumed == 0);
        KInvariant(PartialDataChunk->_Data);
        KInvariant(PartialDataChunk->_Data->QuerySize() >= 1);

        DequeueOp.Reuse();
    }

    bytePtr[1] = static_cast<PBYTE>(PartialDataChunk->_Data->GetBuffer())[PartialDataChunk->_Consumed];
    PartialDataChunk->_Consumed++;
    
    ULONG payloadSize = bytePtr[1] & 0x7F; // 0b01111111
    FrameLength = 2 + payloadSize;
    ULONG payloadConsumed = 0;
    while (payloadConsumed < payloadSize)
    {
        if (PartialDataChunk->_Consumed == PartialDataChunk->_Data->QuerySize())
        {
            DequeueOp.StartDequeue(nullptr, sync);
            status = sync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("InMemory server data dequeue failed after 20 seconds.");
                return status;
            }

            PartialDataChunk = DequeueOp.GetDequeuedSharedItem();
            KInvariant(PartialDataChunk);
            KInvariant(PartialDataChunk->_Consumed == 0);
            KInvariant(PartialDataChunk->_Data);
            KInvariant(PartialDataChunk->_Data->QuerySize() >= 1);

            DequeueOp.Reuse();
        }

        ULONG toConsume = __min(PartialDataChunk->_Data->QuerySize() - PartialDataChunk->_Consumed, payloadSize - payloadConsumed);
        KMemCpySafe(
            bytePtr + 2 + payloadConsumed, 
            Buffer.QuerySize() - (2 + payloadConsumed), 
            static_cast<PBYTE>(PartialDataChunk->_Data->GetBuffer()) + PartialDataChunk->_Consumed, 
            toConsume);

        PartialDataChunk->_Consumed += toConsume;
        payloadConsumed += toConsume;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DoPingPongTest(
    __in ULONG PingPeriod,
    __in ULONG PongTimeout
    )
{
    NTSTATUS status;
    KServiceSynchronizer serviceSync;
    KSynchronizer operationSync;
    KWebSocketReceiveCloseSynchronizer closeReceivedSync;

    KTestPrintf("\nStarting PING + PONG Timeout test.");

    KHttpServer::SPtr server;
    HTTP_REQUEST_V2 httpRequest;
    KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>> unknownHeaders;
    KHttpServerRequest::SPtr serverRequest;
    InMemoryTransport::SPtr transport;
    TestKHttpServerWebSocket::SPtr serverWebSocket;

    status = InMemTestSetup(server, serverRequest, httpRequest, unknownHeaders, transport, serverWebSocket);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    KTimer::SPtr timer;
    status = KTimer::Create(timer, *g_Allocator, KTL_TAG_TEST);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    KAsyncQueue<InMemoryTransport::ReceiveOperation>::DequeueOperation::SPtr receiveDequeueOp;
    status = transport->_ServerReceiveRequestQueue->CreateDequeueOperation(receiveDequeueOp);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    KBuffer::SPtr receiveBuffer;
    status = KBuffer::Create(256, receiveBuffer, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    TestSharedAsyncQueue<InMemoryTransport::DataChunk>::TestSharedDequeueOperation::SPtr dataDequeueOp;
    status = transport->_ServerDataQueue->CreateDequeueOperation(dataDequeueOp);
    if (! NT_SUCCESS(status))
    {
        return status;
    }


    KBuffer::SPtr pongFrameBuffer;
    status = KBuffer::Create(256, pongFrameBuffer, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    KInvariant(NT_SUCCESS(serverWebSocket->SetTimingConstant(KWebSocket::TimingConstant::PingQuietChannelPeriodMs, static_cast<KWebSocket::TimingConstantValue>(PingPeriod))));
    KInvariant(NT_SUCCESS(serverWebSocket->SetTimingConstant(KWebSocket::TimingConstant::PongTimeoutMs, static_cast<KWebSocket::TimingConstantValue>(PongTimeout))));

    status = serverWebSocket->StartOpenWebSocket(
        *serverRequest,
        closeReceivedSync,
        nullptr,
        serviceSync.OpenCompletionCallback()
        );

    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to start the server WebSocket open.");
        return status;
    }

    status = serviceSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Server WebSocket failed to open.");
        return status;
    }


    //
    //  Respond to a number of pings, to demonstrate that the PONG timeout will not occur if PONG frames
    //  are sent in a timely manner
    //
    ULONG frameLength = 0;
    InMemoryTransport::DataChunk::SPtr partialChunk = nullptr;
    for (ULONG i = 0; i < 15; i++)
    {
        status = GetNextControlFrame(*dataDequeueOp, *receiveBuffer, frameLength, partialChunk);
        KTestPrintf("Responding to ping #%u", i);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to retreive frame from the server.  Status: %X", status);
            KInvariant(NT_SUCCESS(serverWebSocket->StartCloseWebSocket(nullptr, serviceSync.CloseCompletionCallback())));
            status = serviceSync.WaitForCompletion();
            KInvariant(FALSE);
            return status;
        }

        PBYTE bytePtr = (PBYTE) receiveBuffer->GetBuffer();
        if (bytePtr[0] != 0x89)
        {
            KTestPrintf("Default server websocket sent something other than PING (First byte: %X)", bytePtr[0]);
            return STATUS_UNSUCCESSFUL;
        }

        KInvariant((bytePtr[1] & 0x80) == 0);  //  Mask = 0
        BYTE payloadLength = bytePtr[1] & 0x7F;
        KInvariant(payloadLength >= 0);
        KInvariant(frameLength == 2 + (ULONG)payloadLength);

        ULONG pongFrameSize = 6 + payloadLength;


        static_cast<PBYTE>(pongFrameBuffer->GetBuffer())[0] = 0x8A;
        static_cast<PBYTE>(pongFrameBuffer->GetBuffer())[1] = 0x80 | payloadLength;
        // [2..5] is the masking key, we will use whatever's in memory
        KInvariant(pongFrameBuffer->QuerySize() >= pongFrameSize);
        KInvariant(receiveBuffer->QuerySize() >= frameLength);
        KMemCpySafe(
            static_cast<PBYTE>(pongFrameBuffer->GetBuffer()) + 6, 
            pongFrameBuffer->QuerySize() - 6, 
            bytePtr + 2, 
            payloadLength);

        //
        //  Mask the payload data
        //
        for(ULONG j = 0; j < payloadLength; j++)
        {
            static_cast<PBYTE>(pongFrameBuffer->GetBuffer())[6 + j] ^= static_cast<PBYTE>(pongFrameBuffer->GetBuffer())[2 + (j % 4)];
            KInvariant((static_cast<PBYTE>(pongFrameBuffer->GetBuffer())[6 + j] ^ static_cast<PBYTE>(pongFrameBuffer->GetBuffer())[2 + (j % 4)])
                == bytePtr[2 + j]);
        }

        //
        //  Inject the PONG frame into the server
        //
        ULONG written = 0;
        while (written < pongFrameSize)
        {
            receiveDequeueOp->StartDequeue(nullptr, operationSync);
            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("Dequeue completed with error status.");
                return status;
            }

            InMemoryTransport::ReceiveOperation& op = receiveDequeueOp->GetDequeuedItem();
            ULONG toWrite = __min(op._BufferSizeInBytes, pongFrameSize - written);
            KInvariant(toWrite > 0);
            KInvariant(op._BufferSizeInBytes >= toWrite);
            KMemCpySafe(op._Buffer, op._BufferSizeInBytes, static_cast<PBYTE>(pongFrameBuffer->GetBuffer()) + written, toWrite);
            written += toWrite;

            op.IOCP_Completion(NO_ERROR, toWrite);

            timer->StartTimer(1000, nullptr, operationSync);
            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("Timer completed with error status.");
                return status;
            }

            timer->Reuse();
            receiveDequeueOp->Reuse();
        }
    }

    KTestPrintf("\nStopping sending PONGS to allow timeout.");

    //
    //  Stop sending PONGs to allow the pong timer to timeout and the websocket to fail
    //
    do
    {
        status = GetNextControlFrame(*dataDequeueOp, *receiveBuffer, frameLength, partialChunk);
    } while (NT_SUCCESS(status));

    status = serverWebSocket->StartCloseWebSocket(nullptr, serviceSync.CloseCompletionCallback());
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to start server WebSocket close.");
        return status;
    }

    status = serviceSync.WaitForCompletion();
    if (NT_SUCCESS(status))
    {
        KTestPrintf("Server close completed with success status despite being in an error state (due to PONG timeout).");
        return STATUS_UNSUCCESSFUL;
    }
    else if (status != K_STATUS_PROTOCOL_TIMEOUT)
    {
        KTestPrintf("Server close failed with status other than K_STATUS_PROTOCOL_TIMEOUT.");
        return status;
    }

    KInvariant(NT_SUCCESS(server->StartClose(nullptr, serviceSync.CloseCompletionCallback())));
    status = serviceSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to close khttp server.");
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RunInMemoryWebSocketTest()
{
    NTSTATUS status;
    KServiceSynchronizer serviceSync;
    KSynchronizer operationSync;
    KWebSocketReceiveCloseSynchronizer closeReceivedSync;

    
    //
    //  Verify the PONG keepalive timer behavior
    //
    {
        KTestPrintf("\nStarting PONG keepalive test.");

        KHttpServer::SPtr server;
        HTTP_REQUEST_V2 httpRequest;
        KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>> unknownHeaders;
        KHttpServerRequest::SPtr serverRequest;
        InMemoryTransport::SPtr transport;
        TestKHttpServerWebSocket::SPtr serverWebSocket;

        status = InMemTestSetup(server, serverRequest, httpRequest, unknownHeaders, transport, serverWebSocket);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        CHAR const * supportedSubprotocolsString = "echo2,echo1, echo";
        KStringA::SPtr supportedSubProtocols;
        status = KStringA::Create(supportedSubProtocols, *g_Allocator, supportedSubprotocolsString, TRUE);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        KBuffer::SPtr closeFrameBuffer;
        status = KBuffer::Create(256, closeFrameBuffer, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        ULONG closeFrameSize;
        status = WriteControlFrame(*closeFrameBuffer, 0x8, closeFrameSize);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Unable to write close frame into buffer.");
            return status;
        }
        

        //
        //  Set the timing pong keepliave period to one second (1000 ms)
        //
        serverWebSocket->SetTimingConstant(KWebSocket::TimingConstant::PongKeepalivePeriodMs, static_cast<KWebSocket::TimingConstantValue>(1000));
        
        status = serverWebSocket->StartOpenWebSocket(
            *serverRequest,
            closeReceivedSync,
            nullptr,
            serviceSync.OpenCompletionCallback(),
            256,
            256,
            supportedSubProtocols
            );

        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to start the server WebSocket open.");
            return status;
        }

        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Server WebSocket failed to open.");
            return status;
        }

        


        KAsyncQueue<InMemoryTransport::ReceiveOperation>::DequeueOperation::SPtr receiveDequeueOp;
        status = transport->_ServerReceiveRequestQueue->CreateDequeueOperation(receiveDequeueOp);
        if (! NT_SUCCESS(status))
        {
            return status;
        }


        KBuffer::SPtr receiveBuffer;
        status = KBuffer::Create(256, receiveBuffer, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        TestSharedAsyncQueue<InMemoryTransport::DataChunk>::TestSharedDequeueOperation::SPtr dataDequeueOp;
        status = transport->_ServerDataQueue->CreateDequeueOperation(dataDequeueOp);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        InMemoryTransport::DataChunk::SPtr partialChunk = nullptr;
        ULONG frameLength = 0;
        
        //
        //  Parse some PONGs from the server
        //
        for (int i = 0; i < 15; i++)
        {
            status = GetNextControlFrame(*dataDequeueOp, *receiveBuffer, frameLength, partialChunk);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("Failed to retreive frame from the server.");
                return status;
            }

            PBYTE bytePtr = (PBYTE) receiveBuffer->GetBuffer();
            if (bytePtr[0] != 0x8A)
            {
                KTestPrintf("Default server websocket sent something other than PONG (Opcode: %X)", bytePtr[0]);
                return STATUS_UNSUCCESSFUL;
            }
        }

        status = serverWebSocket->StartCloseWebSocket(nullptr, serviceSync.CloseCompletionCallback());
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to start server WebSocket close.");
            return status;
        }

        //
        //  Inject a close frame to the server
        //
        ULONG written = 0;
        while (written < closeFrameSize)
        {
            receiveDequeueOp->StartDequeue(nullptr, operationSync);
            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                return status;
            }

            InMemoryTransport::ReceiveOperation& op = receiveDequeueOp->GetDequeuedItem();
            ULONG toWrite = __min(op._BufferSizeInBytes, closeFrameSize - written);
            
            KInvariant(toWrite > 0);
            KMemCpySafe(op._Buffer, op._BufferSizeInBytes, closeFrameBuffer->GetBuffer(), toWrite);
            written += toWrite;

            op.IOCP_Completion(NO_ERROR, toWrite);

            receiveDequeueOp->Reuse();
        }

        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Server WebSocket close failed.");
            return status;
        }

        KInvariant(NT_SUCCESS(server->StartClose(nullptr, serviceSync.CloseCompletionCallback())));
        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to close khttp server.");
            return status;
        }
    }

    //
    //  Test failing open due to timeout
    //
    {
        KTestPrintf("\nStarting open timeout test.");

        KHttpServer::SPtr server;
        HTTP_REQUEST_V2 httpRequest;
        KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>> unknownHeaders;
        KHttpServerRequest::SPtr serverRequest;
        InMemoryTransport::SPtr transport;
        TestKHttpServerWebSocket::SPtr serverWebSocket;

        status = InMemTestSetup(server, serverRequest, httpRequest, unknownHeaders, transport, serverWebSocket);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        //
        //  Cause the open to timeout by delaying the server's response
        //
        transport->_TransportSendDelay = 10000;
        KInvariant(NT_SUCCESS(serverWebSocket->SetTimingConstant(KWebSocket::TimingConstant::OpenTimeoutMs, static_cast<KWebSocket::TimingConstantValue>(5000))));

        status = serverWebSocket->StartOpenWebSocket(
            *serverRequest,
            closeReceivedSync,
            nullptr,
            serviceSync.OpenCompletionCallback()
            );
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        status = serviceSync.WaitForCompletion();
        if (NT_SUCCESS(status))
        {
            KTestPrintf("Server open succeeded despite send being delayed for longer than the open timeout period.");
            return STATUS_UNSUCCESSFUL;
        }
        else if (status != K_STATUS_PROTOCOL_TIMEOUT)
        {
            KTestPrintf("Server open failed with status other than K_STATUS_PROTOCOL_TIMEOUT.");
            return status;
        }

        KInvariant(NT_SUCCESS(server->StartClose(nullptr, serviceSync.CloseCompletionCallback())));
        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to close khttp server.");
            return status;
        }
    }


    //
    //  Test CLOSE failing due to timeout
    //
    {
        KTestPrintf("\nStarting close timeout test.");

        KHttpServer::SPtr server;
        HTTP_REQUEST_V2 httpRequest;
        KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>> unknownHeaders;
        KHttpServerRequest::SPtr serverRequest;
        InMemoryTransport::SPtr transport;
        TestKHttpServerWebSocket::SPtr serverWebSocket;

        status = InMemTestSetup(server, serverRequest, httpRequest, unknownHeaders, transport, serverWebSocket);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        KInvariant(NT_SUCCESS(serverWebSocket->SetTimingConstant(KWebSocket::TimingConstant::CloseTimeoutMs, static_cast<KWebSocket::TimingConstantValue>(5000))));
    
        status = serverWebSocket->StartOpenWebSocket(
            *serverRequest,
            closeReceivedSync,
            nullptr,
            serviceSync.OpenCompletionCallback()
            );

        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to start the server WebSocket open.");
            return status;
        }

        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Server WebSocket failed to open.");
            return status;
        }

        //
        //  Cause the close timeout by delaying the server's sending of the close frame
        //
        transport->_TransportSendDelay = 10000;

        status = serverWebSocket->StartCloseWebSocket(nullptr, serviceSync.CloseCompletionCallback());
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to start server WebSocket close.");
            return status;
        }

        status = serviceSync.WaitForCompletion();
        if (NT_SUCCESS(status))
        {
            KTestPrintf("Server close succeeded despite send being delayed for longer than the close timeout period.");
            return STATUS_UNSUCCESSFUL;
        }
        else if (status != K_STATUS_PROTOCOL_TIMEOUT)
        {
            KTestPrintf("Server close failed with status other than K_STATUS_PROTOCOL_TIMEOUT.");
            return status;
        }

        KInvariant(NT_SUCCESS(server->StartClose(nullptr, serviceSync.CloseCompletionCallback())));
        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to close khttp server.");
            return status;
        }
    }

    //
    //  Test PING without PONG timeout
    //
    {
        KTestPrintf("\nStarting PING (no PONG) test.");

        KHttpServer::SPtr server;
        HTTP_REQUEST_V2 httpRequest;
        KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>> unknownHeaders;
        KHttpServerRequest::SPtr serverRequest;
        InMemoryTransport::SPtr transport;
        TestKHttpServerWebSocket::SPtr serverWebSocket;

        status = InMemTestSetup(server, serverRequest, httpRequest, unknownHeaders, transport, serverWebSocket);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        KTimer::SPtr timer;
        status = KTimer::Create(timer, *g_Allocator, KTL_TAG_TEST);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        KAsyncQueue<InMemoryTransport::ReceiveOperation>::DequeueOperation::SPtr receiveDequeueOp;
        status = transport->_ServerReceiveRequestQueue->CreateDequeueOperation(receiveDequeueOp);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        KBuffer::SPtr receiveBuffer;
        status = KBuffer::Create(256, receiveBuffer, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        TestSharedAsyncQueue<InMemoryTransport::DataChunk>::TestSharedDequeueOperation::SPtr dataDequeueOp;
        status = transport->_ServerDataQueue->CreateDequeueOperation(dataDequeueOp);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        KBuffer::SPtr closeFrameBuffer;
        status = KBuffer::Create(6, closeFrameBuffer, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        ULONG closeFrameSize;
        status = WriteControlFrame(*closeFrameBuffer, 0x8, closeFrameSize);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Unable to write close frame into buffer.");
            return status;
        }



        KBuffer::SPtr pongFrameBuffer;
        status = KBuffer::Create(6, pongFrameBuffer, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        ULONG pongFrameSize;
        status = WriteControlFrame(*pongFrameBuffer, 0xA, pongFrameSize);
        if (! NT_SUCCESS(status))
        {
            return status;
        }


        KInvariant(NT_SUCCESS(serverWebSocket->SetTimingConstant(KWebSocket::TimingConstant::PingQuietChannelPeriodMs, static_cast<KWebSocket::TimingConstantValue>(5000))));
        KInvariant(NT_SUCCESS(serverWebSocket->SetTimingConstant(KWebSocket::TimingConstant::PongTimeoutMs, KWebSocket::TimingConstantValue::None)));

        status = serverWebSocket->StartOpenWebSocket(
            *serverRequest,
            closeReceivedSync,
            nullptr,
            serviceSync.OpenCompletionCallback()
            );

        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to start the server WebSocket open.");
            return status;
        }

        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Server WebSocket failed to open.");
            return status;
        }

        //
        //  Suppress PINGs by sending unsolicited PONGs to verify that PINGs are not sent unless the channel is quiet
        //  Send PONG frames every second for 15 seconds, while the PING quiet channel period is 5 seconds.
        //  Nothing should be sent from the server during this time.
        //
        for (int i = 0; i < 15; i++)
        {
            //
            //  Inject a PONG frame into the server
            //
            ULONG written = 0;
            while (written < pongFrameSize)
            {
                receiveDequeueOp->StartDequeue(nullptr, operationSync);
                status = operationSync.WaitForCompletion();
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("Dequeue completed with error status.");
                    return status;
                }

                InMemoryTransport::ReceiveOperation& op = receiveDequeueOp->GetDequeuedItem();
                ULONG toWrite = __min(op._BufferSizeInBytes, pongFrameSize - written);
                
                KInvariant(toWrite > 0);
                KMemCpySafe(op._Buffer, op._BufferSizeInBytes, pongFrameBuffer->GetBuffer(), toWrite);
                written += toWrite;

                op.IOCP_Completion(NO_ERROR, toWrite);

                timer->StartTimer(1000, nullptr, operationSync);
                status = operationSync.WaitForCompletion();
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("Timer completed with error status.");
                    return status;
                }

                timer->Reuse();
                receiveDequeueOp->Reuse();
            }
        }

        //
        //  Verify that the server didn't send anything
        //
        if (transport->_ServerDataQueue->GetNumberOfQueuedItems() != 0)
        {
            KTestPrintf("Sever data queue is nonempty (meaning server sent data despite channel not being quiet).");
            return STATUS_UNSUCCESSFUL;
        }

        InMemoryTransport::DataChunk::SPtr partialChunk = nullptr;
        ULONG frameLength = 0;

        //
        //  Now let the channel go quiet, and read some PING frames
        //
        for (int i = 0; i < 15; i++)
        {
            status = GetNextControlFrame(*dataDequeueOp, *receiveBuffer, frameLength, partialChunk);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("Failed to retreive frame from the server.");
                return status;
            }

            PBYTE bytePtr = (PBYTE) receiveBuffer->GetBuffer();
            if (bytePtr[0] != 0x89)
            {
                KTestPrintf("Server websocket sent something other than PING (Opcode: %X)", bytePtr[0]);
                return STATUS_UNSUCCESSFUL;
            }
        }

        status = serverWebSocket->StartCloseWebSocket(nullptr, serviceSync.CloseCompletionCallback());
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to start server WebSocket close.");
            return status;
        }

        //
        //  Inject a close frame to the server
        //
        ULONG written = 0;
        while (written < closeFrameSize)
        {
            receiveDequeueOp->StartDequeue(nullptr, operationSync);
            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                return status;
            }

            InMemoryTransport::ReceiveOperation& op = receiveDequeueOp->GetDequeuedItem();
            ULONG toWrite = __min(op._BufferSizeInBytes, closeFrameSize - written);
            
            KInvariant(toWrite > 0);
            KMemCpySafe(op._Buffer, op._BufferSizeInBytes,static_cast<PBYTE>(closeFrameBuffer->GetBuffer()) + written, toWrite);
            written += toWrite;

            op.IOCP_Completion(NO_ERROR, toWrite);

            receiveDequeueOp->Reuse();
        }

        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Server WebSocket close failed.");
            return status;
        }

        KInvariant(NT_SUCCESS(server->StartClose(nullptr, serviceSync.CloseCompletionCallback())));
        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to close khttp server.");
            return status;
        }
    }


    //
    //  Test PING with PONG timeout
    //
    {
        status = DoPingPongTest(5000, 5000);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("PingPongTest failed with ping period %u and pong timeout %u", 4000, 4000);
            return status;
        }

        status = DoPingPongTest(5000, 10000);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("PingPongTest failed with ping period %u and pong timeout %u", 2000, 4000);
            return status;
        }

        status = DoPingPongTest(2000, 1000);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("PingPongTest failed with ping period %u and pong timeout %u", 5000, 500);
            return status;
        }
    }

    return STATUS_SUCCESS;
}



NTSTATUS
RunWebSocketClientEchoTest(
    __in LPCWSTR Url
    )
{
    NTSTATUS status;

    KBufferString receiveString;
    KStringView messageString(L"This is a message string to be sent along the WebSocket.  A longer message provides more variance for the randomized fragment send/receive tests.");
    KBuffer::SPtr message0;

    KUri::SPtr clientUrl;

    status = KBuffer::Create(messageString.LengthInBytes(), message0, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("ERROR: failed to create message buffer");
        return status;
    }

    KInvariant(message0->QuerySize() >= messageString.LengthInBytes());
    KMemCpySafe(message0->GetBuffer(), message0->QuerySize(), messageString, messageString.LengthInBytes());



    status = KUri::Create(KStringView(Url), *g_Allocator, clientUrl);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("ERROR: failed to create client URI");
        return status;
    }

    KBuffer::SPtr receiveBuffer;
	HRESULT hr;
	ULONG result;
	hr = ULongAdd(messageString.LengthInBytes(), 2, &result);
	KInvariant(SUCCEEDED(hr));
	status = KBuffer::Create(result, receiveBuffer, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("ERROR: failed to create receive buffer");
        return status;
    }

    
    //
    //  Test some happy path scenarios
    //
    #pragma region happyPath
    {
        KServiceSynchronizer serviceSync;
        KSynchronizer operationSync;
        KWebSocketReceiveCloseSynchronizer receiveCloseSynchronizer;

        KHttpClientWebSocket::SPtr clientWebSocket;
        KWebSocket::ReceiveFragmentOperation::SPtr receiveFragmentOperation;
        KWebSocket::ReceiveMessageOperation::SPtr receiveMessageOperation;
        KWebSocket::SendFragmentOperation::SPtr sendFragmentOperation;
        KWebSocket::SendMessageOperation::SPtr sendMessageOperation;
        KStringA::SPtr requestedSubprotocols;

        status = KHttpClientWebSocket::Create(clientWebSocket, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to create client web socket. Status: 0X%08X", status);
            return status;
        }

        //
        //  Randomly enable/disable the client-supported timers
        //
        if (rand() % 2)
        {
            KInvariant(NT_SUCCESS(clientWebSocket->SetTimingConstant(KWebSocket::TimingConstant::OpenTimeoutMs, KWebSocket::TimingConstantValue::None)));
        }

        if (rand() % 2)
        {
            KInvariant(NT_SUCCESS(clientWebSocket->SetTimingConstant(KWebSocket::TimingConstant::CloseTimeoutMs, KWebSocket::TimingConstantValue::None)));
        }

        if (rand() % 2)
        {
            KInvariant(NT_SUCCESS(clientWebSocket->SetTimingConstant(KWebSocket::TimingConstant::PongKeepalivePeriodMs, static_cast<KWebSocket::TimingConstantValue>(1000))));
        }

        status = clientWebSocket->CreateSendMessageOperation(sendMessageOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to create send message operation");
            return status;
        }

        status = clientWebSocket->CreateSendFragmentOperation(sendFragmentOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to create send fragment operation");
            return status;
        }

        status = clientWebSocket->CreateReceiveFragmentOperation(receiveFragmentOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to create receive Fragment operation");
            return status;
        }

        status = clientWebSocket->CreateReceiveMessageOperation(receiveMessageOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to create receive message operation");
            return status;
        }

        status = KStringA::Create(requestedSubprotocols, *g_Allocator, KStringViewA("\techo2,\techo1,\t echo \r"), TRUE);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to create requested subprotocols");
            return status;
        }


        status = clientWebSocket->StartOpenWebSocket(
            *clientUrl,
            receiveCloseSynchronizer,
            nullptr,
            serviceSync.OpenCompletionCallback(),
            4096L,
            requestedSubprotocols
            );
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to start client web socket.  Status: 0X%08X", status);
            return status;
        }
        if (clientWebSocket->GetConnectionStatus() != KWebSocket::ConnectionStatus::OpenStarted)
        {
            KTestPrintf("ERROR: WebSocket connection status is not OpenStarted after StartOpenWebSocket is called.");
            return STATUS_UNSUCCESSFUL;
        }

        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: WebSocket open failed.");
            return status;
        }
        if (clientWebSocket->GetConnectionStatus() != KWebSocket::ConnectionStatus::Open)
        {
            KTestPrintf("ERROR: WebSocket connection status is not Open after open calls back successfully.");
            return STATUS_UNSUCCESSFUL;
        }

        KTestPrintf("Testing the selected subprotocol");
        KStringViewA activeSubProtocol = clientWebSocket->GetActiveSubProtocol();
        KInvariant(activeSubProtocol.IsNullTerminated());
        if (activeSubProtocol.Compare("echo") != 0)
        {
            KTestPrintf("ERROR: Wrong subprotocol selected.  \nExpected: echo\nActual: %s", (LPCSTR) clientWebSocket->GetActiveSubProtocol());
            return STATUS_UNSUCCESSFUL;
        }
        KTestPrintf("Selected subprotocol matches expected value: %s", (LPCSTR) activeSubProtocol);

        //
        //  Test simple text message send/receive
        //
        KTestPrintf("Starting simple text message send/receive");
        {
            KTestPrintf("Starting message send.");
            sendMessageOperation->StartSendMessage(
                *message0,
                KWebSocket::MessageContentType::Text,
                nullptr,
                operationSync
                );
    
            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Message send failed.");
                return status;
            }
            KTestPrintf("Message sent.");
            sendMessageOperation->Reuse();

            KTestPrintf("Starting message receive.");
            receiveMessageOperation->StartReceiveMessage(
                *receiveBuffer,
                nullptr,
                operationSync
                );

            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Message receive failed.");
                return status;
            }
            KTestPrintf("Message received.");
            receiveMessageOperation->Reuse();

            status = receiveString.MapBackingBuffer(*receiveBuffer, messageString.Length());
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to map received string.");
                return status;
            }
            KInvariant(receiveString.SetNullTerminator());

            if (messageString.Compare(receiveString) != 0)
            {
                KTestPrintf("Message received does not match sent message: \"%S\" != \"%S\"", (LPCWSTR) receiveString, (LPCWSTR) messageString);
                return STATUS_UNSUCCESSFUL;
            
            }
            KTestPrintf("Message received matches sent message: %S", (LPCWSTR) messageString);

            RtlZeroMemory(receiveBuffer->GetBuffer(), receiveBuffer->QuerySize());
        }

        //
        //  Test fragmented message send/receive
        //
        {
            //
            //  Fragment send, message receive.  Also test binary content type, and starting the receive 
            //  before any of the fragment sends (to show that message receive does not prematurely complete)
            //
            KTestPrintf("Starting fragment send, message receive.");
            {
                ULONG offset = 0;
                ULONG chunkLength = (messageString.Length() / 3) * 2;
                KSynchronizer receiveSync;

                KTestPrintf("Starting message receive.");
                receiveMessageOperation->StartReceiveMessage(
                    *receiveBuffer,
                    nullptr,
                    receiveSync
                    );

                KTestPrintf("Starting the first fragment send.");
                sendFragmentOperation->StartSendFragment(
                    *message0,
                    FALSE,
                    KWebSocket::MessageContentType::Binary,
                    nullptr,
                    operationSync,
                    offset,
                    chunkLength
                    );
                offset += chunkLength;


                status = operationSync.WaitForCompletion();
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: First fragment send failed.");
                    return status;
                }
                KTestPrintf("First fragment send completed.");
                sendFragmentOperation->Reuse();

                KTestPrintf("Starting the second fragment send.");
                sendFragmentOperation->StartSendFragment(
                    *message0,
                    FALSE,
                    KWebSocket::MessageContentType::Binary,
                    nullptr,
                    operationSync,
                    offset,
                    chunkLength
                    );
                offset += chunkLength;

                status = operationSync.WaitForCompletion();
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: Second fragment send failed.");
                    return status;
                }
                KTestPrintf("Second fragment send completed.");
                sendFragmentOperation->Reuse();

                KTestPrintf("Starting the third (and final) fragment send.");
                sendFragmentOperation->StartSendFragment(
                    *message0,
                    TRUE,
                    KWebSocket::MessageContentType::Binary,
                    nullptr,
                    operationSync,
                    offset,
                    messageString.LengthInBytes() - offset
                    );
                offset = messageString.LengthInBytes();

                status = operationSync.WaitForCompletion();
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: Third fragment send failed.");
                    return status;
                }
                KTestPrintf("Third fragment send completed.");
                sendFragmentOperation->Reuse();

            
                KTestPrintf("Waiting for accumulated message receive to complete.");
                status = receiveSync.WaitForCompletion();
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: Message receive failed.");
                    return status;
                }
                KTestPrintf("Message received.");

                if (receiveMessageOperation->GetContentType() != KWebSocket::MessageContentType::Binary)
                {
                    KTestPrintf("ERROR: Received message content type does not match sent (binary).");
                    return STATUS_UNSUCCESSFUL;
                }

                receiveMessageOperation->Reuse();


                status = receiveString.MapBackingBuffer(*receiveBuffer, messageString.Length());
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: failed to map received string.");
                    return status;
                }
                KInvariant(receiveString.SetNullTerminator());

                if (messageString.Compare(receiveString) != 0)
                {
                    KTestPrintf("Message received does not match sent message: \"%S\" != \"%S\"", (LPCWSTR) receiveString, (LPCWSTR) messageString);
                    return STATUS_UNSUCCESSFUL;
                }
                KTestPrintf("Message received matches sent message: %S", (LPCWSTR) messageString);

                RtlZeroMemory(receiveBuffer->GetBuffer(), receiveBuffer->QuerySize());
            }

            //
            //  Message send, fragment receive
            //
            KTestPrintf("Starting message send, fragment receive.");
            {
                ULONG offset = 0;
                ULONG const messageSize = message0->QuerySize();
                ULONG receiveFragmentBufferSize;

                sendMessageOperation->StartSendMessage(
                    *message0,
                    KWebSocket::MessageContentType::Text,
                    nullptr,
                    operationSync
                    );
                status = operationSync.WaitForCompletion();
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: Message send failed.");
                    return status;
                }
                KTestPrintf("Message sent.");
                sendMessageOperation->Reuse();

                while (offset < messageSize)
                {
                    ULONG remaining = messageSize - offset;
                    receiveFragmentBufferSize = (rand() % remaining) + 1;
                    KInvariant(receiveFragmentBufferSize > 0 && receiveFragmentBufferSize <= remaining);

                    receiveFragmentOperation->StartReceiveFragment(
                        *receiveBuffer,
                        nullptr,
                        operationSync,
                        offset,
                        receiveFragmentBufferSize
                        );
                    status = operationSync.WaitForCompletion();
                    if (! NT_SUCCESS(status))
                    {
                        KTestPrintf("ERROR: Fragment receive failed.");
                        return status;
                    }
                    KTestPrintf("Fragment received.");

                    if (receiveFragmentOperation->GetContentType() != KWebSocket::MessageContentType::Text)
                    {
                        KTestPrintf("ERROR: Received fragment content type does not match sent (text).");
                        return STATUS_UNSUCCESSFUL;
                    }

                    if (receiveFragmentOperation->GetBytesReceived() == 0)
                    {
                        KTestPrintf("ERROR: Received 0 bytes");
                        return STATUS_UNSUCCESSFUL;
                    }

                    offset += receiveFragmentOperation->GetBytesReceived();

                    receiveFragmentOperation->Reuse();
                }

                status = receiveString.MapBackingBuffer(*receiveBuffer, messageString.Length());
                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: failed to map received string.");
                    return status;
                }
                KInvariant(receiveString.SetNullTerminator());

                if (messageString.Compare(receiveString) != 0)
                {
                    KTestPrintf("Message accumulated does not match sent message: \"%S\" != \"%S\"", (LPCWSTR) receiveString, (LPCWSTR) messageString);
                    return STATUS_UNSUCCESSFUL;
                }
                KTestPrintf("Message accumulated matches sent message: %S", (LPCWSTR) messageString);

                RtlZeroMemory(receiveBuffer->GetBuffer(), receiveBuffer->QuerySize());
            }
        }

        //
        //  Test client-initiated shutdown with a close reason
        //
        KTestPrintf("Starting client-initiated shutdown.");
        {
            CHAR const * const closeReasonString = "Test close reason";
            KSharedBufferStringA::SPtr clientCloseReason;
            status = KSharedBufferStringA::Create(clientCloseReason, closeReasonString, *g_Allocator);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Failed to create client close reason string.");
                return status;
            }
            KInvariant(clientCloseReason->Compare(closeReasonString) == 0);

            status = clientWebSocket->StartCloseWebSocket(
                nullptr,
                serviceSync.CloseCompletionCallback(),
                KWebSocket::CloseStatusCode::NormalClosure,
                clientCloseReason
                );
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Failed to start the close procedure from the client.");
                return status;
            }

            status = receiveCloseSynchronizer.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: WebSocket had error status when receiving a close frame.");
                return status;
            }

            status = serviceSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Client-initiated WebSocket close failed.");
                return status;
            }
            KTestPrintf("Client-initiated WebSocket graceful close complete.");
        }
        clientWebSocket = nullptr;


        //
        //  Test server-initiated shutdown
        //
        KTestPrintf("Starting server-initiated shutdown.");
        {
            status = KHttpClientWebSocket::Create(clientWebSocket, *g_Allocator);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to create client web socket. Status: 0X%08X", status);
                return status;
            }

            status = clientWebSocket->CreateSendMessageOperation(sendMessageOperation);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to create send message operation");
                return status;
            }
            
            status = clientWebSocket->CreateReceiveMessageOperation(receiveMessageOperation);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to create receive message operation");
                return status;
            }

            status = clientWebSocket->StartOpenWebSocket(
                *clientUrl,
                receiveCloseSynchronizer,
                nullptr,
                serviceSync.OpenCompletionCallback()
                );
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to start client web socket.  Status: 0X%08X", status);
                return status;
            }
            if (clientWebSocket->GetConnectionStatus() != KWebSocket::ConnectionStatus::OpenStarted)
            {
                KTestPrintf("ERROR: WebSocket connection status is not OpenStarted after StartOpenWebSocket is called.");
                return STATUS_UNSUCCESSFUL;
            }

            status = serviceSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: WebSocket open failed.");
                return status;
            }

            KTestPrintf("Starting message send.");
            sendMessageOperation->StartSendMessage(
                *message0,
                KWebSocket::MessageContentType::Text,
                nullptr,
                operationSync
                );
    
            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Message send failed.");
                return status;
            }
            KTestPrintf("Message sent.");
            sendMessageOperation->Reuse();

            KTestPrintf("Starting message receive.");
            receiveMessageOperation->StartReceiveMessage(
                *receiveBuffer,
                nullptr,
                operationSync
                );

            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Message receive failed.");
                return status;
            }
            KTestPrintf("Message received.");
            receiveMessageOperation->Reuse();

            status = receiveString.MapBackingBuffer(*receiveBuffer, messageString.Length());
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to map received string.");
                return status;
            }
            KInvariant(receiveString.SetNullTerminator());

            if (messageString.Compare(receiveString) != 0)
            {
                KTestPrintf("Message received does not match sent message: \"%S\" != \"%S\"", (LPCWSTR) receiveString, (LPCWSTR) messageString);
                return STATUS_UNSUCCESSFUL;
            
            }
            KTestPrintf("Message received matches sent message: %S", (LPCWSTR) messageString);


            KSharedBufferStringA::SPtr shutdownString;
            status = KSharedBufferStringA::Create(shutdownString, WebSocketEchoHandler::ShutdownString, *g_Allocator);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to create shutdown string.");
                return status;
            }

            sendMessageOperation->StartSendMessage(
                *shutdownString->GetBackingBuffer(),
                KWebSocket::MessageContentType::Text,
                nullptr,
                operationSync,
                0,
                shutdownString->LengthInBytes()
                );

            status = operationSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Shutdown message send failed.");
                return status;
            }
            KTestPrintf("Shutdown message sent.");
            sendMessageOperation->Reuse();

            status = receiveCloseSynchronizer.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: WebSocket had error status when receiving a close frame.");
                return status;
            }

            status = clientWebSocket->StartCloseWebSocket(
                nullptr,
                serviceSync.CloseCompletionCallback()
                );
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Failed to close the client web socket after receiving a close frame.");
                return status;
            }

            status = serviceSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Server-initiated WebSocket close failed.");
                return status;
            }
            KTestPrintf("Server-initiated WebSocket graceful close complete.");
        }


    }
    #pragma endregion happyPath


    //
    //  Test some Abort() paths
    //
    #pragma region abortPath
    {

        //
        //  Abort before any ops
        //
        {
            KNt::Sleep(100);
            KTestPrintf("\n\nStarting test (client abort before any ops)");

            KServiceSynchronizer serviceSync;
            KSynchronizer operationSync;
            KWebSocketReceiveCloseSynchronizer closeSync;

            KHttpClientWebSocket::SPtr clientWebSocket;
            KWebSocket::SendMessageOperation::SPtr sendMessageOperation;

            status = KHttpClientWebSocket::Create(clientWebSocket, *g_Allocator);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to recreate client web socket. Status: 0X%08X", status);
                return status;
            }

            status = clientWebSocket->CreateSendMessageOperation(sendMessageOperation);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to create send message operation");
                return status;
            }

            status = clientWebSocket->StartOpenWebSocket(
                *clientUrl,
                closeSync,
                nullptr,
                serviceSync.OpenCompletionCallback()
                );
            if (! NT_SUCCESS(status)) 
            {
                KTestPrintf("ERROR: failed to start client web socket.  Status: 0X%08X", status);
                return status;
            }

            KNt::Sleep(rand() % 20);
            clientWebSocket->Abort();

            status = serviceSync.WaitForCompletion();
            if (NT_SUCCESS(status))
            {
                KTestPrintf("Open succeeded before Abort() failed the WebSocket.  Running send operations until failure.");
                do
                {
                    sendMessageOperation->StartSendMessage(
                        *message0,
                        KWebSocket::MessageContentType::Text,
                        nullptr,
                        operationSync
                        );
                    status = operationSync.WaitForCompletion();
                    sendMessageOperation->Reuse();
                } while (NT_SUCCESS(status));

                KTestPrintf("Send correctly failed due to Abort().  Status: 0X%08X", status);

                KTestPrintf("Starting a client-initiated close after operation failure (open succeeded).");
                status = clientWebSocket->StartCloseWebSocket(
                    nullptr,
                    serviceSync.CloseCompletionCallback(),
                    KWebSocket::CloseStatusCode::GenericClosure
                    );

                if (! NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: Failed to start the close procedure from the client after an operation failed due to Abort().");
                    return status;
                }

                status = serviceSync.WaitForCompletion();
                if (NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: WebSocket close succeeded after WebSocket transitioned to Error state.");
                    return STATUS_UNSUCCESSFUL;
                }
                KTestPrintf("WebSocket closed with error status due to having failed.");
            }
            else
            {
                if (status != STATUS_REQUEST_ABORTED)
                {
                    KTestPrintf("Open status not STATUS_REQUEST_ABORTED after failing open due to abort.");
                    return STATUS_UNSUCCESSFUL;
                }
                KTestPrintf("Open correctly failed due to Abort().");

                KTestPrintf("Starting a client-initiated close after open failed.");
                status = clientWebSocket->StartCloseWebSocket(
                    nullptr,
                    serviceSync.CloseCompletionCallback(),
                    KWebSocket::CloseStatusCode::GenericClosure
                    );

                if (NT_SUCCESS(status))
                {
                    KTestPrintf("ERROR: StartCloseWebSocket returned success status after open failed due to Abort().");
                    return STATUS_UNSUCCESSFUL;
                }
                if (status != STATUS_REQUEST_ABORTED)
                {
                    KTestPrintf("ERROR: Close status not STATUS_REQUEST_ABORTED after failing open due to abort.");
                    return STATUS_UNSUCCESSFUL;
                }
                KTestPrintf("Call to StartCloseWebSocket correctly returned failure status after open failed due to Abort().");
            }
        }

        //
        //  Test server-initiated abort
        //
        {
            KTestPrintf("\n\n Starting test (server-initiated abort)");

            KServiceSynchronizer serviceSync;
            KSynchronizer operationSync;
            KWebSocketReceiveCloseSynchronizer closeSync;

            KHttpClientWebSocket::SPtr clientWebSocket;
            KWebSocket::SendMessageOperation::SPtr sendMessageOperation;
            KWebSocket::ReceiveMessageOperation::SPtr receiveMessageOperation;

            status = KHttpClientWebSocket::Create(clientWebSocket, *g_Allocator);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to create client web socket. Status: 0X%08X", status);
                return status;
            }

            status = clientWebSocket->CreateSendMessageOperation(sendMessageOperation);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to create send message operation");
                return status;
            }

            status = clientWebSocket->CreateReceiveMessageOperation(receiveMessageOperation);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to create receive message operation");
                return status;
            }

            status = clientWebSocket->StartOpenWebSocket(
                *clientUrl,
                closeSync,
                nullptr,
                serviceSync.OpenCompletionCallback()
                );
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: failed to start client web socket.  Status: 0X%08X", status);
                return status;
            }

            status = serviceSync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: WebSocket client open (without subprotocol) failed.");
                return status;
            }
            KTestPrintf("Client webSocket successfully opened.");

            if (! clientWebSocket->GetActiveSubProtocol().IsEmpty())
            {
                KTestPrintf("ERROR: Client has non-null active subprotocol after open, when no subprotocol was requested");
                return status;
            }

            KSharedBufferStringA::SPtr abortMessage;
            status = KSharedBufferStringA::Create(abortMessage, WebSocketEchoHandler::AbortString, *g_Allocator);
            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Failed to create abort message.");
                return status;
            }
            sendMessageOperation->StartSendMessage(
                *abortMessage->GetBackingBuffer(),
                KWebSocket::MessageContentType::Text,
                nullptr,
                operationSync,
                0,
                abortMessage->LengthInBytes()
                );
            status = operationSync.WaitForCompletion();
            sendMessageOperation->Reuse();

            if (! NT_SUCCESS(status))
            {
                KTestPrintf("Abort message send failed due to remote transport closure racing ahead of send success.");
            }
            else
            {
                KTestPrintf("Abort message send completed.");

                KNt::Sleep(rand() % 20);
                KTestPrintf("Executing message send and receives until failure occurs due to the remote Abort()");
                while (true)
                {
                    KTestPrintf("Starting client send after abort message sent.");
                    sendMessageOperation->StartSendMessage(
                        *message0,
                        KWebSocket::MessageContentType::Text,
                        nullptr,
                        operationSync,
                        0,
                        abortMessage->LengthInBytes()
                        );
                    status = operationSync.WaitForCompletion();
                    if (! NT_SUCCESS(status))
                    {
                        KTestPrintf("Send correctly failed due to remote Abort().  Status: 0X%08X", status);
                        break;
                    }
                    sendMessageOperation->Reuse();

                    KTestPrintf("Starting client receive after abort message sent.");
                    receiveMessageOperation->StartReceiveMessage(
                        *receiveBuffer,
                        nullptr,
                        operationSync
                        );
                    status = operationSync.WaitForCompletion();
                    if (! NT_SUCCESS(status))
                    {
                        KTestPrintf("Receive correctly failed due to remote Abort().  Status: 0X%08X", status);
                        break;
                    }
                    receiveMessageOperation->Reuse();
                
                    // skip verifying the message contents
                }
            }


            status = clientWebSocket->StartCloseWebSocket(
                nullptr,
                serviceSync.CloseCompletionCallback(),
                KWebSocket::CloseStatusCode::GenericClosure
                );

            if (! NT_SUCCESS(status))
            {
                KTestPrintf("ERROR: Failed to start the close procedure from the client after an operation failed due to Abort().");
                return status;
            }

            status = serviceSync.WaitForCompletion();
            if (NT_SUCCESS(status))
            {
                KTestPrintf("WebSocket close succeeded despite remote abort; close procedure won the race with abort.");
            }
            else
            {
                KTestPrintf("WebSocket closed with error status due to having failed.");
            }
        }
    }
    #pragma endregion abortPath

    //
    //  Test mismatching content types
    //
    {
        KServiceSynchronizer serviceSync;
        KSynchronizer operationSync;
        KWebSocketReceiveCloseSynchronizer closeSync;

        KHttpClientWebSocket::SPtr clientWebSocket;
        KWebSocket::SendFragmentOperation::SPtr sendFragmentOperation;

        status = KHttpClientWebSocket::Create(clientWebSocket, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to create client web socket. Status: 0X%08X", status);
            return status;
        }

        status = clientWebSocket->CreateSendFragmentOperation(sendFragmentOperation);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to create send fragment operation. Status: 0X%08X", status);
            return status;
        }

        status = clientWebSocket->StartOpenWebSocket(
            *clientUrl,
            closeSync,
            nullptr,
            serviceSync.OpenCompletionCallback()
            );
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: failed to start client web socket.  Status: 0X%08X", status);
            return status;
        }

        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: WebSocket client open (without subprotocol) failed.  Status: 0X%08X", status);
            return status;
        }
        KTestPrintf("Client webSocket successfully opened.");

        if (! clientWebSocket->GetActiveSubProtocol().IsEmpty())
        {
            KTestPrintf("ERROR: Client has non-null active subprotocol after open, when no subprotocol was requested");
            return status;
        }

        sendFragmentOperation->StartSendFragment(
            *message0,
            FALSE,
            KWebSocket::MessageContentType::Binary,
            nullptr,
            operationSync
            );

        status = operationSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: First fragment send failed.");
            return status;
        }
        KTestPrintf("First fragment send completed.");
        sendFragmentOperation->Reuse();

        KTestPrintf("Starting mismatched fragment send.");
        sendFragmentOperation->StartSendFragment(
            *message0,
            FALSE,
            KWebSocket::MessageContentType::Text,
            nullptr,
            operationSync
            );
        status = operationSync.WaitForCompletion();
        if (NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: Fragment send with mismatched content type completed successfully.");
            return STATUS_UNSUCCESSFUL;
        }
        KTestPrintf("Fragment send with mismatched content type correctly failed.");
        sendFragmentOperation->Reuse();
        
        if (clientWebSocket->GetConnectionStatus() != KWebSocket::ConnectionStatus::Error)
        {
            KTestPrintf("ERROR: WebSocket->GetConnectionStatus() is not Error after failing an operation.");
            return STATUS_UNSUCCESSFUL;
        }

        KTestPrintf("Testing starting an operation after the WebSocket has transitioned to Error state.");
        sendFragmentOperation->StartSendFragment(
            *message0,
            FALSE,
            KWebSocket::MessageContentType::Binary,
            nullptr,
            operationSync
            );

        status = operationSync.WaitForCompletion();
        if (NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: An operation succeeded after the WebSocket transitioned to Error state.");
            return STATUS_UNSUCCESSFUL;
        }
        KTestPrintf("Fragment send after the WebSocket has transitioned to Error state correctly failed.");

        KTestPrintf("Starting a client-initiated close after failure.");
        status = clientWebSocket->StartCloseWebSocket(
            nullptr,
            serviceSync.CloseCompletionCallback(),
            KWebSocket::CloseStatusCode::GenericClosure
            );
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: Failed to start the close procedure from the client. Status: 0X%08X", status);
            return status;
        }

        status = serviceSync.WaitForCompletion();
        if (NT_SUCCESS(status))
        {
            KTestPrintf("ERROR: WebSocket close succeeded after WebSocket transitioned to Error state.");
            return STATUS_UNSUCCESSFUL;
        }
        KTestPrintf("WebSocket closed with error status due to having failed.");
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RunInProcWebSocketTest(
    __in ULONG ServerIterations,
    __in ULONG ClientIterations
)
{
    NTSTATUS status;

    srand(static_cast<ULONG>(KNt::GetSystemTime()));

    ULONG machineNameSize = 0;
    KDynString serverUrlString(*g_Allocator);
    KDynString clientUrlString(*g_Allocator);
    KStringView serverScheme(L"http://");
    KStringView clientScheme(L"ws://");
    KStringView portAndPath(L":80/inproc");
    KStringView clientPathSuffix(L"/TestQuery");
    KStringView queryString(L"field1=value1&field2=value2");

    GetComputerNameExW(ComputerNameNetBIOS, NULL, &machineNameSize);
    if (machineNameSize == 0)
    {
        KDbgPrintf("ERROR: GetComputerNameExW yielded 0 for machine name size.");
        return NTSTATUS_FROM_WIN32(GetLastError());
    }
    if (! serverUrlString.Resize(machineNameSize + serverScheme.Length() + portAndPath.Length()))
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, NULL, NULL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (! GetComputerNameExW(ComputerNameNetBIOS, serverUrlString, &machineNameSize))
    {
        KTestPrintf("ERROR: failed to retrieve computer name");
        return STATUS_UNSUCCESSFUL;
    }
    serverUrlString.SetLength(machineNameSize);

    if (! serverUrlString.Concat(portAndPath))
    {
        KTestPrintf("ERROR: failed to set url port and path");
        return STATUS_UNSUCCESSFUL;
    }

    if (! clientUrlString.CopyFrom(serverUrlString))
    {
        KTestPrintf("ERROR: failed to copy to client url string");
        return STATUS_UNSUCCESSFUL;
    }

    if (! serverUrlString.Prepend(serverScheme))
    {
        KTestPrintf("ERROR: failed to set server url scheme");
        return STATUS_UNSUCCESSFUL;
    }

    if (! clientUrlString.Prepend(clientScheme))
    {
        KTestPrintf("ERROR: failed to set client url scheme");
        return STATUS_UNSUCCESSFUL;
    }

    if (!clientUrlString.Concat(clientPathSuffix))
    {
        KTestPrintf("ERROR: failed to set client path suffix string");
        return STATUS_UNSUCCESSFUL;
    }

    if (!clientUrlString.Concat(L"?"))
    {
        KTestPrintf("ERROR: failed to set query separator");
        return STATUS_UNSUCCESSFUL;
    }

    if (! clientUrlString.Concat(queryString))
    {
        KTestPrintf("ERROR: failed to set query string");
        return STATUS_UNSUCCESSFUL;
    }

    KUri::SPtr serverUrl;
    status = KUri::Create(serverUrlString, *g_Allocator, serverUrl);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("ERROR: failed to create server URI");
        return status;
    }

    if (! serverUrl->IsValid())
    {
        KTestPrintf("ERROR: url invalid");
        return STATUS_UNSUCCESSFUL;
    }

    KTestPrintf("Running servers on URL = %S", (LPCWSTR)serverUrlString);
    for (ULONG serverIteration = 0; serverIteration < ServerIterations; serverIteration++)
    {
        QuickTimer t;
        KHttpServer::SPtr Server;

        KEvent OpenEvent;
        KEvent ShutdownEvent;
        p_ServerOpenEvent = &OpenEvent;
        p_ServerShutdownEvent = &ShutdownEvent;

        status = KHttpServer::Create(*g_Allocator, Server);
        if (! NT_SUCCESS(status))
        {
            if (status == STATUS_ACCESS_DENIED)
            {
                KTestPrintf("ERROR: Server must run under admin privileges.");
            }
            return status;
        }

        WebSocketRequestHandler webSocketRequestHandler(Server, queryString);

        status = Server->StartOpen(
            *serverUrl,
            KHttpServer::RequestHandler(&webSocketRequestHandler, &WebSocketRequestHandler::Handler),
            3,
            8192,
            8192,
            0,
            ServerOpenCallback,
            nullptr,
            KHttpUtils::HttpAuthType::AuthNone,
            65536
            );
        if (! NT_SUCCESS(status))
        {
            if (status == STATUS_INVALID_ADDRESS)
            {
                KTestPrintf("ERROR: URL is bad or missing the port number, which is required for HTTP.SYS based servers");
            }
            return status;
        }

        OpenEvent.WaitUntilSet();
        KInvariant(Server->Status() == STATUS_PENDING);
        KTestPrintf("Server is running.");

        for (ULONG clientIteration = 0; clientIteration < ClientIterations; clientIteration++)
        {
            KTestPrintf("Starting iteration %u,%u / %u,%u\n", serverIteration, clientIteration, ServerIterations, ClientIterations);
            KTestPrintf("Starting iteration %u,%u / %u,%u", serverIteration, clientIteration, ServerIterations, ClientIterations);
            status = RunWebSocketClientEchoTest(clientUrlString);
            if (! NT_SUCCESS(status))
            {
                return status;
            }
        }

        KTestPrintf("...Deactivating server");
        Server->StartClose(0, ServerShutdownCallback);

        ShutdownEvent.WaitUntilSet();

        status = Server->Status();
        if (! NT_SUCCESS(status))
        {
            return status;
        }
    }

    return STATUS_SUCCESS;
}
#endif //#if NTDDI_VERSION >= NTDDI_WIN8


//-------------- CLIENT TESTS-------------------------------------------------------------------------

static KAutoResetEvent* pResponseEvent = nullptr;

VOID
GetOpCallback(
   __in KAsyncContextBase* const Parent,
   __in KAsyncContextBase& Op
   )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Op);
    
    // Called at first phase.
    pResponseEvent->SetEvent();

}


VOID
DumpUnicodeBuffer(ULONG BufSize, wchar_t* Buf)
{
    for (ULONG i = 0; i < BufSize; i++)
    {
        KTestPrintf("%C", Buf[i]);
    }
}


NTSTATUS RunGetTest(
    __in KHttpClient::SPtr Client,
    __in KNetworkEndpoint::SPtr Url
    )
{
    // Create a new request and address
    //
    KHttpCliRequest::SPtr Req;
    NTSTATUS Res = Client->CreateRequest(Req);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetTarget(Url, KHttpUtils::eHttpGet);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Execute and block until the response is incoming
    //
    KHttpCliRequest::Response::SPtr Resp;

    KAutoResetEvent ResponseEvent;
    pResponseEvent = &ResponseEvent;

    Res = Req->Execute(Resp, 0, GetOpCallback);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ResponseEvent.WaitUntilSet();

    // When here, we have a response or timeout
    //
    ULONG Code;
    NTSTATUS Status;

    Status = Resp->Status();

    Code = Resp->GetHttpResponseCode();

    KTestPrintf("HTTP Response code is %u\n", Code);

    KString::SPtr Header;
    Resp->GetHeaderByCode(KHttpUtils::HttpHeaderContentType, Header);

    ULONG ContLength = Resp->GetContentLengthHeader();

    KTestPrintf("Content type is        %S\n",   Header ? PWSTR(*Header) : L"(null)");
    KTestPrintf("Content Length is      %u\n", ContLength);
    KTestPrintf("Entity Body Length is  %u\n", Resp->GetEntityBodyLength());


    KString::SPtr AllHeaders;
    Resp->GetAllHeaders(AllHeaders);
    if (AllHeaders)
    {
        KTestPrintf("\n-------------Begin headers:\n%S\n", PWSTR(*AllHeaders));
    }
    KTestPrintf("\n--------End headers------------\n");

    KBuffer::SPtr Content;
    Status = Resp->GetContent(Content);
    if (!NT_SUCCESS(Status) && ContLength > 0)
    {
        KTestPrintf("Failed to get content\n");
        return Status;
    }
    if (!Content)
    {
        KTestPrintf("Content buffer is NULL\n");
    }
    else
    {
        ULONG BufSize = Resp->GetEntityBodyLength();
        wchar_t* Buf = (wchar_t *) Content->GetBuffer();
        KTestPrintf("Buffer [%u bytes]:{\n", BufSize);
        DumpUnicodeBuffer(BufSize, Buf);
        KTestPrintf("\n}\n");
    }

    return STATUS_SUCCESS;
}


NTSTATUS RunPostTest(
    __in KHttpClient::SPtr Client,
    __in KNetworkEndpoint::SPtr Url
    )
{
    // Create a new request and address
    //
    KHttpCliRequest::SPtr Req;
    NTSTATUS Res = Client->CreateRequest(Req);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetTarget(Url, KHttpUtils::eHttpPost);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KStringView content(L"<XmlTest>input content is here</XmlTest>");
    Res = Req->SetContent(content);
    KStringView value(L"text/xml");
    Res = Req->AddHeaderByCode(KHttpUtils::HttpHeaderContentType, value);

    // Execute and block until the response is incoming
    //
    KHttpCliRequest::Response::SPtr Resp;

    KAutoResetEvent ResponseEvent;
    pResponseEvent = &ResponseEvent;

    Res = Req->Execute(Resp, nullptr, GetOpCallback);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ResponseEvent.WaitUntilSet();

    // When here, we have a response or timeout
    //
    ULONG Code;
    NTSTATUS Status;

    Status = Resp->Status();

    Code = Resp->GetHttpResponseCode();

    KTestPrintf("HTTP Response code is %u\n", Code);

    KString::SPtr Header;
    Resp->GetHeaderByCode(KHttpUtils::HttpHeaderContentType, Header);

    ULONG ContLength = Resp->GetContentLengthHeader();

    KTestPrintf("Content type is        %S\n",   Header ? PWSTR(*Header) : L"(null)");
    KTestPrintf("Content Length is      %u\n", ContLength);
    KTestPrintf("Entity Body Length is  %u\n", Resp->GetEntityBodyLength());


    KString::SPtr AllHeaders;
    Resp->GetAllHeaders(AllHeaders);
    if (AllHeaders)
    {
        KTestPrintf("\n-------------Begin headers:\n%S\n", PWSTR(*AllHeaders));
    }
    KTestPrintf("\n--------End headers------------\n");

    KBuffer::SPtr Content;
    Status = Resp->GetContent(Content);
    if (!NT_SUCCESS(Status) && ContLength > 0)
    {
        KTestPrintf("Failed to get content\n");
        return Status;
    }
    if (!Content)
    {
        KTestPrintf("Content buffer is NULL\n");
    }
    else
    {
        ULONG BufSize = Resp->GetEntityBodyLength();
        wchar_t* Buf = (wchar_t *) Content->GetBuffer();
        KTestPrintf("Buffer [%u bytes]:{\n", BufSize);
        DumpUnicodeBuffer(BufSize, Buf);
        KTestPrintf("\n}\n");
    }

    return STATUS_SUCCESS;
}


VOID
RunClientOps(
    __in KStringView& ServerUrl
    )
{
    KLocalString<512> FinalUrl;
    FinalUrl.CopyFrom(ServerUrl);
    FinalUrl.Concat(KStringView(L"/big"));
    DirectGet(FinalUrl, 0);
}

WCHAR g_UrlBuf[MAX_PATH];
ULONG g_Seconds = 0;

DWORD WINAPI ServerThread(LPVOID Param)
{
    UNREFERENCED_PARAMETER(Param);
    
    RunServer(g_UrlBuf, g_Seconds);
    return 0;
}

NTSTATUS RunSelfTest(
    __in KStringView Url,
    __in ULONG Seconds
    )
{
    *g_UrlBuf = 0;
    KStringView OtherUrl(g_UrlBuf, MAX_PATH);
    OtherUrl.CopyFrom(Url);
    OtherUrl.SetNullTerminator();
    g_Seconds = Seconds;

    // Spawn server on a different thread

    DWORD ThreadId;
    CreateThread(0, 0, ServerThread, 0, 0, &ThreadId);

    // Now do some client ops

    QuickTimer t;

    for (;;)
    {
        RunClientOps(Url);
        if (t.HasElapsed(Seconds))
        {
            break;
        }
    }

    while (!g_ShutdownServer)
    {
        KNt::Sleep(1000);
    }
    return STATUS_SUCCESS;
}


NTSTATUS Post(
    __in LPCWSTR Url,
    __in LPCWSTR FilePath,
    __in LPCWSTR ContentType,
    __in LPCWSTR Dest
    )
{
    KBuffer::SPtr FileContent;

    if (!FilePathToKBuffer(FilePath, FileContent))
    {
        KTestPrintf("Unable to read file %S\n", FilePath);
        return STATUS_UNSUCCESSFUL;
    }

    KTestPrintf("Posting %u bytes of type %S\n", FileContent->QuerySize(), ContentType);

    KHttpClient::SPtr Client;
    NTSTATUS Result;

    KStringView userAgent(L"KTL HTTP Test Agent");
    Result = KHttpClient::Create(userAgent, *g_Allocator, Client);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    // Convert Url to a network endpoint.
    //
    KNetworkEndpoint::SPtr Ep;
    KUriView httpUri(Url);
    NTSTATUS Res = KHttpNetworkEndpoint::Create(httpUri, *g_Allocator, Ep);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    // Create a new request and address
    //
    KHttpCliRequest::SPtr Req;
    Res = Client->CreateRequest(Req);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetTarget(Ep, KHttpUtils::eHttpPost);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetContent(FileContent);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KStringView value(ContentType);
    Res = Req->AddHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    // Execute and block until the response is incoming
    //
    KHttpCliRequest::Response::SPtr Resp;

    KAutoResetEvent ResponseEvent;
    pResponseEvent = &ResponseEvent;

    Res = Req->Execute(Resp, nullptr, GetOpCallback);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ResponseEvent.WaitUntilSet();


    // When here, we have a response or timeout
    //
    ULONG Code;
    NTSTATUS Status;

    Status = Resp->Status();

    Code = Resp->GetHttpResponseCode();

    KTestPrintf("HTTP Response code is %u\n", Code);

    KString::SPtr Header;
    Resp->GetHeaderByCode(KHttpUtils::HttpHeaderContentType, Header);

    ULONG ContLength = Resp->GetContentLengthHeader();

    KTestPrintf("Content type is        %S\n",   Header ? PWSTR(*Header) : L"(null)");
    KTestPrintf("Content Length is      %u\n", ContLength);
    KTestPrintf("Entity Body Length is  %u\n", Resp->GetEntityBodyLength());


    KString::SPtr AllHeaders;
    Resp->GetAllHeaders(AllHeaders);
    if (AllHeaders)
    {
        KTestPrintf("\n-------------Begin headers:\n%S\n", PWSTR(*AllHeaders));
    }
    KTestPrintf("\n--------End headers------------\n");

    KBuffer::SPtr Content;
    Status = Resp->GetContent(Content);
    if (!NT_SUCCESS(Status) && ContLength > 0)
    {
        KTestPrintf("Failed to get content\n");
        return Status;
    }
    if (!Content)
    {
        KTestPrintf("Content buffer is NULL\n");
    }
    else
    {
        ULONG BufSize = Resp->GetEntityBodyLength();
        char* Buf = (char *) Content->GetBuffer();

        if (FilePath)
        {
            FILE* f;
            f = _wfopen(Dest, L"wb");
            if (!f)
            {
               return STATUS_UNSUCCESSFUL;
            }
            fwrite(Buf, 1, BufSize, f);
            fclose(f);
        }
    }

    return STATUS_SUCCESS;
}

PCCERT_CONTEXT GetClientCertificate(
    __in LPCWSTR certCNName,
    __in LPCWSTR certSystemStoreName)
{
    // Open the store the certificate is in.
    HCERTSTORE hStore = CertOpenSystemStore(
        0,
        certSystemStoreName);
    PCCERT_CONTEXT pClientCert = NULL;

    if (hStore)
    {
        pClientCert = CertFindCertificateInStore(hStore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0,
            CERT_FIND_SUBJECT_STR,
            certCNName, //Subject string in the certificate.
            NULL);
        DWORD error = GetLastError();

        if (pClientCert == NULL)
        {
            KTestPrintf("Failed to find certificate in store, GetLastError = %u\n", error);
        }
        CertCloseStore(hStore, 0);
    }
    return pClientCert;
}

BOOLEAN AllowServerCert(
    __in PCCERT_CONTEXT pCertContext
)
{
    if (!pCertContext)
    {
        KTestPrintf("Server Cert is NULL");
    }

    // succeed the server cert validation.
    return TRUE;
}

BOOLEAN DenyServerCert(
    __in PCCERT_CONTEXT pCertContext
)
{
    if (!pCertContext)
    {
        KTestPrintf("Server Cert is NULL");
    }

    // fail the server cert validation.
    return FALSE;
}

NTSTATUS SendRequest(
    __in KStringView& Url,
    __in LPCWSTR FilePath,
    __in KStringView OpString,
    __in_opt PCCERT_CONTEXT clientCert = NULL,
    __in_opt KHttpCliRequest::ServerCertValidationHandler serverCertValidationHandler = NULL
    )
{
    KHttpClient::SPtr Client;
    NTSTATUS Res;

    KStringView userAgent(L"KTL HTTP Test Agent");
    Res = KHttpClient::Create(userAgent, *g_Allocator, Client);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Convert Url to a network endpoint.
    //
    KNetworkEndpoint::SPtr Ep;
    KUriView httpUri(Url);
    Res = KHttpNetworkEndpoint::Create(httpUri, *g_Allocator, Ep);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    // Create a new request and address
    //
    KHttpCliRequest::SPtr Req;
    Res = Client->CreateRequest(Req);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetTarget(Ep, OpString);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KStringView headerId(L"Accept");
    KStringView value(L"text/html");
    Res = Req->AddHeader(headerId, value);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    if (clientCert != NULL)
    {
        Res = Req->SetClientCertificate(clientCert);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }
    }

    if (serverCertValidationHandler != NULL)
    {
        Res = Req->SetServerCertValidationHandler(serverCertValidationHandler);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }
    }

    // Execute and block until the response is incoming
    //
    KHttpCliRequest::Response::SPtr Resp;

    KAutoResetEvent ResponseEvent;
    pResponseEvent = &ResponseEvent;

    Res = Req->Execute(Resp, nullptr, GetOpCallback);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ResponseEvent.WaitUntilSet();


    // When here, we have a response or timeout
    //
    ULONG Code;

    Res = Req->Status();
    KTestPrintf("Request Status = 0x%X   WinHttpErrorCode = %u\n", Req->Status(), Req->GetWinHttpError());
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Code = Resp->GetHttpResponseCode();

    KTestPrintf("SendRequest ------------ HTTP Response code is %u\n", Code);

    KString::SPtr Header;
    Resp->GetHeaderByCode(KHttpUtils::HttpHeaderContentType, Header);

    ULONG ContLength = Resp->GetContentLengthHeader();

    KTestPrintf("Content type is        %S\n", Header ? PWSTR(*Header) : L"(null)");
    KTestPrintf("Content Length is      %u\n", ContLength);
    KTestPrintf("Entity Body Length is  %u\n", Resp->GetEntityBodyLength());


    KString::SPtr AllHeaders;
    Resp->GetAllHeaders(AllHeaders);
    if (AllHeaders)
    {
        KTestPrintf("\n-------------Begin headers:\n%S\n", PWSTR(*AllHeaders));
    }
    KTestPrintf("\n--------End headers------------\n");

    KBuffer::SPtr Content;
    Res = Resp->GetContent(Content);
    if (!NT_SUCCESS(Res) && ContLength > 0)
    {
        KTestPrintf("Failed to get content\n");
        return Res;
    }
    if (!Content)
    {
        KTestPrintf("Content buffer is NULL\n");
    }
    else
    {
        ULONG BufSize = Resp->GetEntityBodyLength();
        char* Buf = (char *)Content->GetBuffer();

        if (FilePath)
        {
            FILE* f;
            f = _wfopen(FilePath, L"wb");
            if (!f)
            {
                return STATUS_UNSUCCESSFUL;
            }
            fwrite(Buf, 1, BufSize, f);
            fclose(f);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS DirectGet(
    __in KStringView& Url,
    __in LPCWSTR FilePath,
    __in_opt PCCERT_CONTEXT clientCert,
    __in_opt KHttpCliRequest::ServerCertValidationHandler serverCertValidationHandler
    )
{
    return SendRequest(Url, FilePath, KHttpUtils::HttpVerb::GET, clientCert, serverCertValidationHandler);
}

LPWSTR WebSites[] =
{
    L"http://www.google.com",
    L"http://www.cnn.com",
    L"http://www.bing.com",
    L"http://www.drudgereport.com",
    L"http://www.time.gov",
    L"http://www.whitehouse.gov",
    L"http://www.faz.net",
    L"http://www.lemonde.fr",
    L"http://www.pravda.ru",
    L"http://www.telegraph.co.uk",
    L"http://www.amazon.com",
    L"http://www.amazon.de",
    L"http://www.amazon.fr"
};

ULONG WebSiteCount = sizeof(WebSites)/sizeof(LPWSTR);

NTSTATUS WebTest(
    __in ULONG Seconds
    )
{
    QuickTimer t;
    NTSTATUS Res;

    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    for (;;)
    {
        for (ULONG i = 0; i < WebSiteCount; i++)
        {
            KStringView url(WebSites[i]);
            Res = DirectGet(url, nullptr);
            if (!NT_SUCCESS(Res))
            {
                return Res;
            }

            if (t.HasElapsed(Seconds))
            {
                goto done;
            }
        }
    }

done:
    return STATUS_SUCCESS;
}

NTSTATUS SSLTestSendClientCert(
    __in LPCWSTR secureUrl,
    __in LPCWSTR certCNName,
    __in LPCWSTR certSystemStoreName
)
{
    NTSTATUS Res = STATUS_SUCCESS;

    PCCERT_CONTEXT pCertContext = GetClientCertificate(certCNName, certSystemStoreName);

    KStringView url(secureUrl);
    Res = DirectGet(url, nullptr, pCertContext, AllowServerCert);
    if (!NT_SUCCESS(Res))
    {
        KTestPrintf("SSLTestSendClientCert failed for %S \r\n", secureUrl);
    }

    CertFreeCertificateContext(pCertContext);

    return Res;
}


LPWSTR SSLValidCertWebSites[] =
{
    L"https://www.microsoft.com",
    L"https://sha256.badssl.com/"
};

ULONG SSLValidCertWebSiteCount = sizeof(SSLValidCertWebSites) / sizeof(LPWSTR);

LPWSTR SSLInvalidCertWebSites[] =
{
    L"https://expired.badssl.com/",
    L"https://wrong.host.badssl.com/",
    L"https://self-signed.badssl.com/",
    L"https://untrusted-root.badssl.com/"
};

ULONG SSLInvalidCertWebSiteCount = sizeof(SSLInvalidCertWebSites) / sizeof(LPWSTR);

NTSTATUS SSLTestSecureClient()
{
    QuickTimer t;
    NTSTATUS Ret = STATUS_SUCCESS;
    NTSTATUS Res;

    // HTTP Get to websites with valid certificates
    for (ULONG i = 0; i < SSLValidCertWebSiteCount; i++)
    {
        KTestPrintf("SSLTestSecureClient GET %S\n", SSLValidCertWebSites[i]);
        KStringView url(SSLValidCertWebSites[i]);
        Res = DirectGet(url, nullptr);
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("SSLTestSecureClient GET %S failed with %u", SSLValidCertWebSites[i], Res);
            Ret = STATUS_UNSUCCESSFUL;
        }
    }

    // HTTP GET to SSL websites with incorrectly configured certificates and verify the request fails. Do not provide any validate certificate handler.
    for (ULONG i = 0; i < SSLInvalidCertWebSiteCount; i++)
    {
        KTestPrintf("SSLTestSecureClient GET %S\n", SSLInvalidCertWebSites[i]);
        KStringView url(SSLInvalidCertWebSites[i]);
        Res = DirectGet(url, nullptr);
        if (NT_SUCCESS(Res))
        {
            KTestPrintf("SSLTestSecureClient GET %S succeeded, Expected : Failure.", SSLInvalidCertWebSites[i]);
            Ret = STATUS_UNSUCCESSFUL;
        }
    }

    // Use validate cert handler to succeed requests to websites with incorrectly configured certificates.
    for (ULONG i = 0; i < SSLInvalidCertWebSiteCount; i++)
    {
        KTestPrintf("SSLTestSecureClient GET %S\n", SSLInvalidCertWebSites[i]);
        KStringView url(SSLInvalidCertWebSites[i]);
        Res = DirectGet(url, nullptr, FALSE, AllowServerCert);
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("SSLTestSecureClient GET %S failed with %u.", SSLInvalidCertWebSites[i], Res);
            Ret = STATUS_UNSUCCESSFUL;
        }
    }

    // Use validate cert handler to fail requests to websites with incorrectly configured certificates.
    for (ULONG i = 0; i < SSLInvalidCertWebSiteCount; i++)
    {
        KTestPrintf("SSLTestSecureClient GET %S\n", SSLInvalidCertWebSites[i]);
        KStringView url(SSLInvalidCertWebSites[i]);
        Res = DirectGet(url, nullptr, FALSE, DenyServerCert);
        if (NT_SUCCESS(Res))
        {
            KTestPrintf("SSLTestSecureClient GET %S succeeded, Expected : Failure.", SSLInvalidCertWebSites[i]);
            Ret = STATUS_UNSUCCESSFUL;
        }
    }

    return Ret;
}

NTSTATUS SendRequest(
    __in KHttpClient::SPtr Client,
    __in LPCWSTR RequestUrl,
    __in BOOLEAN EnableCookies,
    __in BOOLEAN AllowRedirects,
    __in BOOLEAN EnableAutoLogonForAllRequests,
    __out KHttpCliRequest::Response::SPtr& Resp)
{
    // Convert Url to a network endpoint.
    //
    KNetworkEndpoint::SPtr Ep;
    KUriView httpUri(RequestUrl);
    NTSTATUS Res = KHttpNetworkEndpoint::Create(httpUri, *g_Allocator, Ep);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Create a new request and address
    //
    KHttpCliRequest::SPtr Req;
    Res = Client->CreateRequest(Req);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetTarget(Ep, KHttpUtils::eHttpGet);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetEnableCookies(EnableCookies);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetAllowRedirects(AllowRedirects);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetAutoLogonForAllRequests(EnableAutoLogonForAllRequests);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Execute and block until the response is incoming
    //

    KAutoResetEvent ResponseEvent;
    pResponseEvent = &ResponseEvent;

    Res = Req->Execute(Resp, nullptr, GetOpCallback);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ResponseEvent.WaitUntilSet();

    Res = Req->Status();
    KTestPrintf("Request Status = 0x%X   WinHttpErrorCode = %u\n", Req->Status(), Req->GetWinHttpError());
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    return STATUS_SUCCESS;
}

NTSTATUS DisableRedirectsTest(
)
{
    KHttpClient::SPtr Client;
    NTSTATUS Res;

    KStringView userAgent(L"KTL HTTP Test Agent");
    Res = KHttpClient::Create(userAgent, *g_Allocator, Client);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KHttpCliRequest::Response::SPtr Resp;
    Res = SendRequest(Client, L"http://httpbin.org/redirect-to?url=http://www.microsoft.com", FALSE, FALSE, FALSE, Resp);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ULONG Code = Resp->GetHttpResponseCode();
    if (Code != KHttpUtils::HttpResponseCode::Found)
    {
        KTestPrintf("ERROR: SendRequest HTTP Response code is %u\n", Code);
        return STATUS_UNSUCCESSFUL;
    }

    KString::SPtr LocationVal;
    Resp->GetHeader(KHttpUtils::HttpHeaderName::Location, LocationVal);

    if (!LocationVal ||
        LocationVal->CompareNoCase(L"http://www.microsoft.com") != 0)
    {
        KTestPrintf("ERROR: Location Header not found or incorrect.\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS TestCookie(
    __in KHttpClient::SPtr Client,
    __in LPCWSTR RequestUrl,
    __in LPCSTR CookieName,
    __in BOOLEAN EnableCookie)
{
    NTSTATUS Res;
    KHttpCliRequest::Response::SPtr Resp;
    Res = SendRequest(Client, RequestUrl, EnableCookie, TRUE, FALSE, Resp);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KBuffer::SPtr Content;
    ULONG BufSize = Resp->GetEntityBodyLength();
    Res = Resp->GetContent(Content);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KStringViewA ContentView((PCHAR)Content->GetBuffer(), BufSize, BufSize);
    ULONG CookiePos;
    BOOLEAN CookieFound = ContentView.Search(CookieName, CookiePos);
    if ((EnableCookie && !CookieFound) || (!EnableCookie && CookieFound))
    {
        KTestPrintf("ERROR: EnableCookie: %d, CookieFound: %d.\n", EnableCookie, CookieFound);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS CookiesTest(
)
{
    KHttpClient::SPtr Client;
    NTSTATUS Res;
    KStringView userAgent(L"KTL HTTP Test Agent");
    Res = KHttpClient::Create(userAgent, *g_Allocator, Client);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Set a simple cookie
    KHttpCliRequest::Response::SPtr Resp;
    Res = SendRequest(Client, L"http://httpbin.org/cookies/set?KHttpClientCookie1=12345", TRUE, TRUE, FALSE, Resp);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Test cookies returned while Cookies are enabled
    Res = TestCookie(Client, L"http://httpbin.org/cookies", "KHttpClientCookie1", TRUE);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Test cookies returned while Cookies are disabled 
    Res = TestCookie(Client, L"http://httpbin.org/cookies", "KHttpClientCookie1", FALSE);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    return STATUS_SUCCESS;
}

NTSTATUS LargeQueryTest(
    __in LPCWSTR RequestUrl
)
{
    KHttpClient::SPtr Client;
    NTSTATUS Res;
    KStringView userAgent(L"KTL HTTP Test Agent");
    Res = KHttpClient::Create(userAgent, *g_Allocator, Client);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KDynString clientUrlString(*g_Allocator);
    clientUrlString.Concat(RequestUrl);

    const int largeQueryValLength = 16 * 1024;
    wchar_t queryStringVal[largeQueryValLength];
    wmemset(queryStringVal, L'a', largeQueryValLength);
    queryStringVal[largeQueryValLength - 1] = L'\0';
    KDynString queryString(*g_Allocator);
    queryString.Concat(L"?queryname=");
    queryString.Concat(queryStringVal);

    clientUrlString.Concat(queryString);
    KHttpCliRequest::Response::SPtr Resp;
    Res = SendRequest(Client, clientUrlString, TRUE, TRUE, FALSE, Resp);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ULONG Code = Resp->GetHttpResponseCode();
    if (Code != KHttpUtils::HttpResponseCode::Ok)
    {
        KTestPrintf("ERROR: LargeQuery SendRequest HTTP Response code is %u\n", Code);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS TestWinauthClient(
    __in LPCWSTR testUrl
)
{
    KHttpClient::SPtr Client;
    NTSTATUS Res;

    KStringView userAgent(L"KTL HTTP Test Agent");
    Res = KHttpClient::Create(userAgent, *g_Allocator, Client);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KHttpCliRequest::Response::SPtr Resp;
    Res = SendRequest(Client, testUrl, FALSE, FALSE, TRUE, Resp);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ULONG Code = Resp->GetHttpResponseCode();
    if (Code != KHttpUtils::HttpResponseCode::Ok)
    {
        KTestPrintf("ERROR: TestWinauthClient HTTP Response code is %u\n", Code);
        return STATUS_UNSUCCESSFUL;
    }

    return Res;
}

NTSTATUS
ExecCommandLine(
    int argc,
    WCHAR** args
    )
{
    CmdLineParser Parser(*g_Allocator);
    Parameter *Param;

    //
    //  Process command line
    //
    if (Parser.Parse(argc, args))
    {
        if (!Parser.ParameterCount())
        {
            KTestPrintf("No switches specified\n");
            return STATUS_INVALID_PARAMETER;
        }

        Param = Parser.GetParam(0);

        if (wcscmp(Param->_Name, L"websites") == 0
            && Param->_ValueCount == 1)
        {
            KTestPrintf("Running client test against standard web sites for %S seconds\n", Param->_Values[0]);

            KStringView Secs(Param->_Values[0]);
            ULONG Seconds = 0;
            Secs.ToULONG(Seconds);
            return WebTest(Seconds);
        }
        else if (wcscmp(Param->_Name, L"server") == 0
            && Param->_ValueCount == 2)
        {
            KTestPrintf("Running server operation mix [url=%S] for %S seconds. Waiting for clients...\n", Param->_Values[0], Param->_Values[1]);

            KStringView Secs(Param->_Values[1]);
            ULONG Seconds = 0;
            Secs.ToULONG(Seconds);
            LPCWSTR Url = Param->_Values[0];
            return RunServer(Url, Seconds);
        }
        #if NTDDI_VERSION >= NTDDI_WIN8
        else if (wcscmp(Param->_Name, L"ws") == 0
            && Param->_ValueCount > 0)
        {
            KStringView test(Param->_Values[0]);
            if (test.CompareNoCase(L"InMem") == 0)
            {
                KTestPrintf("Running KHttpWebSocket in-memory tests\n");

                return RunInMemoryWebSocketTest();
            }
            else if (test.CompareNoCase(L"InProc") == 0)
            {
                KTestPrintf("Running KHttpWebSocket in-process tests\n");

                KStringView serverIterationsString(Param->_Values[1]);
                ULONG serverIterations = 0;
                serverIterationsString.ToULONG(serverIterations);

                KStringView clientIterationsString(Param->_Values[2]);
                ULONG clientIterations = 0;
                clientIterationsString.ToULONG(clientIterations);

                return RunInProcWebSocketTest(serverIterations, clientIterations);
            }
            else if (test.CompareNoCase(L"Server") == 0)
            {
                KTestPrintf("Running KHttpWebSocket echo server [url=%S] for %S seconds.  Waiting for clients...\n", Param->_Values[1], Param->_Values[2]);

                LPCWSTR url = Param->_Values[1];

                KStringView secs(Param->_Values[2]);
                ULONG seconds = 0;
                secs.ToULONG(seconds);

                return RunTimedWebSocketEchoServer(url, seconds);
            }
            else if (test.CompareNoCase(L"Client") == 0)
            {
                KTestPrintf("Running KHttpWebSocket client againt echo server [url=%S] for %S iterations\n", Param->_Values[1], Param->_Values[2]);

                LPCWSTR url = Param->_Values[1];

                ULONG iterations;
                KStringView iterationsString(Param->_Values[2]);
                iterationsString.ToULONG(iterations);

                NTSTATUS status = STATUS_SUCCESS;
                for (ULONG i = 0; i < iterations; i++)
                {
                    KTestPrintf("Starting client iteration %u/%u\n", i, iterations);
                    status = RunWebSocketClientEchoTest(url);
                    if (! NT_SUCCESS(status))
                    {
                        KTestPrintf("Client iteration failed.\n");
                        return status;
                    }
                }
                return status;
            }
            else
            {
                return STATUS_INVALID_PARAMETER;
            }
        }
        #endif
        else if (wcscmp(Param->_Name, L"advancedserver") == 0
            && Param->_ValueCount == 3)
        {
            KTestPrintf("Running advanced server operation mix [url=%S] for %S seconds with %S preposted requests. Waiting for clients...\n", Param->_Values[0], Param->_Values[1], Param->_Values[2]);

            KStringView Secs(Param->_Values[1]);
            ULONG Seconds = 0;
            Secs.ToULONG(Seconds);

            KStringView Requests(Param->_Values[2]);
            ULONG OutstandingRequests = 0;
            Requests.ToULONG(OutstandingRequests);

            LPCWSTR Url = Param->_Values[0];
            return RunAdvancedServer(Url, Seconds, OutstandingRequests);
        }
        else if (wcscmp(Param->_Name, L"advancedclient") == 0
            && Param->_ValueCount == 2)
        {
            KTestPrintf("Running advanced client against URL %S with %S requests\n", Param->_Values[0], Param->_Values[1]);

            KStringView Reqs(Param->_Values[1]);
            ULONG Requests = 0;
            Reqs.ToULONG(Requests);

            LPCWSTR Url = Param->_Values[0];
            return RunAdvancedClient(Url, Requests);
        }
        else if (wcscmp(Param->_Name, L"selftest") == 0
            && Param->_ValueCount == 2)
        {
            KTestPrintf("Running inproc client/server self test [url=%S] for %S seconds\n", Param->_Values[0], Param->_Values[1]);

            KStringView Secs(Param->_Values[1]);
            ULONG Seconds = 0;
            Secs.ToULONG(Seconds);
            LPCWSTR Url = Param->_Values[0];
            return RunSelfTest(Url, Seconds);
        }
        else if (wcscmp(Param->_Name, L"post") == 0
            && Param->_ValueCount == 4)
        {
            LPCWSTR Url = Param->_Values[0];
            LPCWSTR ContentType = Param->_Values[1];
            LPCWSTR Path = Param->_Values[2];
            LPCWSTR Dest = Param->_Values[3];

            KTestPrintf("Running a Post to %S of file content at %S with content-type %S, response written to %S\n", Url, Path, ContentType, Dest);

            return Post(Url, Path, ContentType, Dest);
        }
        else if (wcscmp(Param->_Name, L"mppost") == 0
            && Param->_ValueCount == 4)
        {
            LPCWSTR Url = Param->_Values[0];
            LPCWSTR ContentType = Param->_Values[1];
            LPCWSTR Path = Param->_Values[2];
            LPCWSTR Dest = Param->_Values[3];

            KTestPrintf("Running a MultiPart Post to %S of file content at %S with content-type %S, response written to %S\n", Url, Path, ContentType, Dest);

            return MultiPartPost(Url, Path, ContentType, Dest);
        }
        else if (wcscmp(Param->_Name, L"get") == 0
            && Param->_ValueCount > 0)
        {
            LPCWSTR File = nullptr;
            if (Param->_ValueCount == 1)
            {
                KTestPrintf("Get '%S' \n", Param->_Values[0]);
            }
            else if (Param->_ValueCount == 2)
            {
                KTestPrintf("Get '%S' to file %S\n", Param->_Values[0],Param->_Values[1]);
                File = Param->_Values[1];
            }
            KStringView uri(Param->_Values[0]);
            return DirectGet(uri, File);
        }
        else if (wcscmp(Param->_Name, L"patch") == 0
            || wcscmp(Param->_Name, L"xxyy") == 0)
        {
            LPCWSTR File = nullptr;
            if (Param->_ValueCount == 1)
            {
                KTestPrintf("PATCH '%S' \n", Param->_Values[0]);
            }
            else if (Param->_ValueCount == 2)
            {
                KTestPrintf("Get '%S' to file %S\n", Param->_Values[0], Param->_Values[1]);
                File = Param->_Values[1];
            }
            KStringView VerbStr(Param->_Name);
            VerbStr.ToUpper();
            KStringView url(Param->_Values[0]);
            return SendRequest(url, File, VerbStr);
        }
        else if (wcscmp(Param->_Name, L"secureclient") == 0)
        {
            KTestPrintf("Running secure client tests against known SSL websites\n");
            return SSLTestSecureClient();
        }
        else if (wcscmp(Param->_Name, L"winauthclient") == 0
            && Param->_ValueCount == 1)
        {
            LPCWSTR url = Param->_Values[0];
            KTestPrintf("Running client tests with windows auth against %S\n",url);
            return TestWinauthClient(url);
        }
        else if (wcscmp(Param->_Name, L"secureclientwithcert") == 0
            && Param->_ValueCount == 3)
        {
            LPCWSTR url = Param->_Values[0];
            LPCWSTR certCNName = Param->_Values[1];
            LPCWSTR certStoreName = Param->_Values[2];
            KTestPrintf("Running secure client tests against %S, using certificate Common Name %S from system store %S\n", url, certCNName, certStoreName);
            return SSLTestSendClientCert(url, certCNName, certStoreName);
        }
        else if (wcscmp(Param->_Name, L"disableredirects") == 0)
        {
            KTestPrintf("Run HTTP GET with redirect disabled\n");
            return DisableRedirectsTest();
        }
        else if (wcscmp(Param->_Name, L"cookiestest") == 0)
        {
            KTestPrintf("Run HTTP GET with cookies enabled and disabled\n");
            return CookiesTest();
        }
        else if (wcscmp(Param->_Name, L"largequerytest") == 0
            && Param->_ValueCount == 1)
        {
            KTestPrintf("Get '%S' \n", Param->_Values[0]);

            KStringView uri(Param->_Values[0]);
            return LargeQueryTest(uri);
        }
        else
        {
            KTestPrintf("Unknown switch or missing arguments\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
KHttpTest(
    int argc, WCHAR* args[]
    )
{
    if (argc < 1)
    {
        KTestPrintf("\n\nKTL HTTP Tests.   Usage:\n");
        KTestPrintf("  -websites nSeconds                      // Runs a client GET against a set of well-known web sites for nSeconds\n");
        KTestPrintf("  -server testUrl nSeconds                // Runs a server-only echo server using the testURL for nSeconds.  The testURL MUST have the port number:  http://myserver:80/testurl\n");
        #if NTDDI_VERSION >= NTDDI_WIN8
        KTestPrintf("  -ws InMem                               // Runs KHttpServerWebSocket tests using in-memory transport");
        KTestPrintf("  -ws InProc serverIterations clientIterations   // Runs an in-process test between KHttpClientWebSocket and KHttpServerWebSocket for (serverIterations * clientIterations) iterations \n");
        KTestPrintf("  -ws Server testUrl nSeconds              // Runs a websocket-only echo server using the testURL for nSeconds.");
        KTestPrintf("  -ws Client testUrl  nIterations                      // Runs a websocket client against a websocket echo server using the testUrl for nIterations iterations\n");
        #endif
        KTestPrintf("  -get serverURL filePath                            // Does a one-time Get from the specified URL and places the response content in the specified filePath\r\n");
        KTestPrintf("  -post serverURL contentType SrcfilePath DestFile   // Does a one-time Post of the specified file to the server, using the content-type specified, response written to DestFile\r\n");
        KTestPrintf("  -mppost serverURL contentType SrcfilePath DestFile   // Does a multipart Post of the specified file to the server, using the content-type specified, response written to DestFile\r\n");
        KTestPrintf("  -selftest serverURL nSeconds                       // Runs an internal server/client in the same process for nSeconds uing various test sequences\n");
        KTestPrintf("  -advancedserver serverURL NumberPrepostedRequests NumberSeconds // Runs a server-only echo server using the testURL for nSeconds.  The testURL MUST have the port number:  http://myserver:80/testurl/<ServerTest> \r\n");
        KTestPrintf("                                                                     <ServerTest> may be StreamInStreamOutEcho, LegacyGet, LegacyGetStreamOut\n");
        KTestPrintf("  -advancedclient serverURL NumberConcurrentRequests   // Runs a client that sends multiple concurrent requests at the advancedserver\n");
        KTestPrintf("  -secureclient // HTTPS tests: Runs a client GET against known HTTPS servers. \n");
        KTestPrintf("  -secureclientwithcert serverURL ClientAuthCertCommonName CertSystemStoreName // HTTPS tests: Runs a client GET against specified server URL with the given certificate. The certificate is obtained from the specified system store (Eg: MY) \r\n");
        KTestPrintf("  -cookiestest // Runs a client GET against a known server with cookies disabled. \n");
        KTestPrintf("  -largequery // Runs a client GET against a known server with a large query param. Pass the server URL as first param Usage: -largequerytest http://myserver:80/testurl/largequery. \n");
        KTestPrintf("                                                                     The test server can be started with the -server option: -server http://myserver:80/testurl 1800\n");

        KTestPrintf("\n");
        KTestPrintf("\n");
        KTestPrintf("  How to run the basic client/server tests:\n\n");
        KTestPrintf("      On machineA, disable the firewall and run the command:\n");
        KTestPrintf("              khttptests -server http://machineA:8081/testurl 10000\n\n");
        KTestPrintf("      On machineB, run the commands:\n");
        KTestPrintf("              khttptests -get http://MachineA:8081/testurl/suffix1 foo1\n");
        KTestPrintf("              khttptests -get http://MachineA:8081/testurl/suffix2/suffix3 foo2\n");
        KTestPrintf("              khttptests -post http://MachineA:8081/testurl/suffix1 text/plain <random input file> bar1\n");
        KTestPrintf("              khttptests -post http://MachineA:8081/testurl/suffix2/suffix3 text/plain <random input file>  bar2\n");
        KTestPrintf("              khttptests -mppost http://MachineA:8081/testurl/suffix1 text/plain <random input file> bar1\n");
        KTestPrintf("              khttptests -patch http://MachineA:8081/testurl/testpatch foo4\n");
        KTestPrintf("              or From a browser, connect to  http://MachineA:8081/testurl/globalizedpath/ijådkåælæmænopekuæræsteuve foo3\n");
        KTestPrintf("              khttptests -mppost http://MachineA:8081/testurl/suffix1 text/plain <random input file> bar1\n");
        KTestPrintf("          NOTE: <random input file> should be less than 64K\n\n");     
        KTestPrintf("      Verify that foo1 and bar1 contains the message: SpecialHandler1\n");
        KTestPrintf("      Verify that foo2 and bar2 contains the message: SpecialHandler2\n");
        KTestPrintf("      Verify that foo3 contains the message: SpecialHandler3\n\n");
        KTestPrintf("      Verify that foo4 contains the message: CustomVerbHandler\n\n");
        KTestPrintf("              khttptests -post http://MachineA:8081/testurl/suffix2/suffix3 text/plain <large input file>  bar3\n");
        KTestPrintf("          NOTE: <large input file> should be greater than 64K\n\n");       
        KTestPrintf("      Verify error 413 is returned\n");
        KTestPrintf("\n");
        KTestPrintf("  How to run the advanced client/server tests:\n\n");
        KTestPrintf("      On machineA, disable the firewall and run the command:\n");
        KTestPrintf("              khttptests -advancedserver http://machineA:8081/testurl 256 10000\n\n");
        KTestPrintf("      On machineB, run the commands:\n");
        KTestPrintf("              khttptests -advancedclient http://machineA:8081/testurl 512\n\n");
        KTestPrintf("      NOTE: advanced client can only be run against advanced server\n");

        return STATUS_INVALID_PARAMETER;
    }

    EventRegisterMicrosoft_Windows_KTL();

    NTSTATUS status;
    KtlSystem* underlyingSystem;
    KDbgMirrorToDebugger = TRUE;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    status = KtlSystem::Initialize(&underlyingSystem);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return status;
    }

    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);


    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    //  Run the test
    status = ExecCommandLine(argc, args);
    KTestPrintf("ExecCommandLine status: 0X%08X\n", status);

    // Sleep to give time for asyncs to be cleaned up before checking for leaks.
    KNt::Sleep(500);
    
    //
    //  Check for leaks
    //
    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - StartingAllocs;
    if (Leaks)
    {
        KTestPrintf("Leaks = %u\n", Leaks);
        KTestPrintf("Leaks = %u", Leaks);
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
        KTestPrintf("No leaks.");
    }
    

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return status;
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

    return KHttpTest(argc, args);
}
#endif
