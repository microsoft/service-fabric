// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "UdpListener.h"
#include "UdpReadOp.h"
#include "UdpWriteOp.h"

/*static*/
void UdpListener::Create(
    __out UdpListener::SPtr& spServer,
    __in KAllocator& allocator,
    __in const IDnsTracer::SPtr& spTracer,
    __in const INetIoManager::SPtr& spManager,
    __in bool fEnableSocketAddressReuse
)
{
    spServer = _new(TAG, allocator) UdpListener(spTracer, spManager, fEnableSocketAddressReuse);
    KInvariant(spServer != nullptr);
}

UdpListener::UdpListener(
    __in const IDnsTracer::SPtr& spTracer,
    __in const INetIoManager::SPtr& spManager,
    __in bool fEnableSocketAddressReuse
) : _spTracer(spTracer),
    _spManager(spManager),
    _port(0),
    _ioRegistrationContext(0),
    _socket(INVALID_SOCKET),
    _fEnableSocketAddressReuse(fEnableSocketAddressReuse)
{
}

UdpListener::~UdpListener()
{
    Tracer().Trace(DnsTraceLevel_Noise, "Destructing DNS UdpListener.");
}


//***************************************
// BEGIN KAsyncContext region
//***************************************

void UdpListener::OnStart()
{
}

void UdpListener::OnCancel()
{
    Tracer().Trace(DnsTraceLevel_Info, "DNS UdpListener OnCancel called.");

    Cleanup();

    Complete(STATUS_CANCELLED);
}

void UdpListener::OnCompleted()
{
    Tracer().Trace(DnsTraceLevel_Info, "DNS UdpListener OnCompleted called.");

#if !defined(PLATFORM_UNIX)
    if (_ioRegistrationContext != 0)
    {
        GetThisAllocator().GetKtlSystem().DefaultThreadPool().UnregisterIoCompletionCallback(_ioRegistrationContext);
        _ioRegistrationContext = 0;
    }
#endif    

    _closeCallback(STATUS_SUCCESS);
}

//***************************************
// END KAsyncContext region
//***************************************

bool UdpListener::StartListener(
    __in_opt KAsyncContextBase* const parent,
    __inout USHORT& port,
    __in UdpListenerCallback completionCallback
)
{
    _closeCallback = completionCallback;

    Reuse();

    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET)
    {
        int error = 0;
#if defined(PLATFORM_UNIX)
        error = errno;
#else
        error = WSAGetLastError();
#endif
        Tracer().Trace(DnsTraceLevel_Warning, "DNS UdpListener, failed to create socket, error {0}", static_cast<LONG>(error));
        return false;
    }

    if (_fEnableSocketAddressReuse)
    {
        int enable = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) == SOCKET_ERROR)
        {
            int error = 0;
#if defined(PLATFORM_UNIX)
            error = errno;
#else
            error = WSAGetLastError();
#endif
            Tracer().Trace(DnsTraceLevel_Warning,
                "DNS UdpListener, failed to set socket option SO_REUSEADDR, error {0}",
                static_cast<LONG>(error));

            return false;
        }
    }

    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

#if defined(PLATFORM_UNIX)
    if (fcntl(s, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
    {
        Tracer().Trace(DnsTraceLevel_Warning, "DNS UdpListener, failed to set socket option, error {0}", errno);
        return false;
    }
#endif

    if (::bind(s, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        int error = 0;
#if defined(PLATFORM_UNIX)
        error = errno;
#else
        error = WSAGetLastError();
#endif
        Tracer().Trace(DnsTraceLevel_Warning, "DNS UdpListener, failed to bind socket, error {0}", static_cast<LONG>(error));
        Cleanup();

        return false;
    }

    socklen_t addrlen = sizeof(sin);
    if (getsockname(s, (struct sockaddr *)&sin, &addrlen) == 0)
    {
        port = ntohs(sin.sin_port);
    }

    _port = port;
    _socket = s;

#if defined(PLATFORM_UNIX)
    _spManager->RegisterSocket(_socket);
#else
    // Overlap
    GetThisAllocator().GetKtlSystem().DefaultThreadPool().RegisterIoCompletionCallback(
        HANDLE(s),
        UdpListener::Raw_IOCP_Completion,
        nullptr,
        &_ioRegistrationContext
    );
#endif

    Start(parent, nullptr);

    Tracer().Trace(DnsTraceLevel_Noise, "DNS UdpListener StartListener");

    return true;
}

bool UdpListener::CloseAsync()
{
    Tracer().Trace(DnsTraceLevel_Noise, "DNS UdpListener CloseAsync");

    if (Status() == STATUS_PENDING)
    {
        Cancel();
        return true;
    }

    return false;
}

void UdpListener::ReadAsync(
    __out KBuffer& buffer,
    __out ISocketAddress& addressFrom,
    __in UdpReadOpCompletedCallback readCompletionCallback,
    __in ULONG timeoutInMs
)
{
    UdpReadOp::SPtr spReadOp;
    UdpReadOp::Create(/*out*/spReadOp, GetThisAllocator(), _socket, *_spManager);
    spReadOp->StartRead(this/*parent*/, buffer, addressFrom, readCompletionCallback, timeoutInMs);
}

void UdpListener::WriteAsync(
    __in KBuffer& buffer,
    __in ULONG numberOfBytesToWrite,
    __in ISocketAddress& addressTo,
    __in UdpWriteOpCompletedCallback writeCompletionCallback
)
{
    UdpWriteOp::SPtr spWriteOp;
    UdpWriteOp::Create(/*out*/spWriteOp, GetThisAllocator(), _socket, *_spManager);

    spWriteOp->StartWrite(this/*parent*/, buffer, numberOfBytesToWrite, addressTo, writeCompletionCallback);
}

void UdpListener::Cleanup()
{
    if (_socket != INVALID_SOCKET)
    {
        int error = closesocket(_socket);

        if (error == SOCKET_ERROR)
        {
#if !defined(PLATFORM_UNIX)
            error = WSAGetLastError();
#else
            error = errno;
#endif
            Tracer().Trace(DnsTraceLevel_Noise, "UdpListener failed to close the socket cleanly, error {0}",
                static_cast<LONG>(error));
        }
        
#if defined(PLATFORM_UNIX)
        _spManager->UnregisterSocket(_socket);
#endif 
        _socket = INVALID_SOCKET;
    }
}

/*static*/
void UdpListener::Raw_IOCP_Completion(
    __in_opt void* pContext,
    __inout OVERLAPPED* pOverlapped,
    __in ULONG status,
    __in ULONG bytesTransferred
)
{
    UNREFERENCED_PARAMETER(pContext);

    UDP_OVERLAPPED* pUdpOverlapped = static_cast<UDP_OVERLAPPED*>(pOverlapped);
    pUdpOverlapped->_pOverlapOp->IOCP_Completion(status, bytesTransferred);
}
