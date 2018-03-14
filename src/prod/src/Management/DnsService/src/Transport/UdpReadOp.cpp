// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "UdpReadOp.h"

/*static*/
void UdpReadOp::Create(
    __out UdpReadOp::SPtr& spReadOp,
    __in KAllocator& allocator,
    __in SOCKET socket,
    __in INetIoManager& netIoManager
)
{
    spReadOp = _new(TAG, allocator) UdpReadOp(socket, netIoManager);
    KInvariant(spReadOp != nullptr);
}

UdpReadOp::UdpReadOp(
    __in SOCKET socket,
    __in INetIoManager& netIoManager
) : _socket(socket),
_netIoManager(netIoManager),
_bytesTransferred(0),
_flags(0),
_timeoutInMs(0)
{
#if !defined(PLATFORM_UNIX)
    RtlZeroMemory(&_wsaBuf, sizeof(WSABUF));
#endif

    RtlZeroMemory(&_olap, sizeof(UDP_OVERLAPPED));
    _olap._pOverlapOp = this;
    _olap._opType = SOCKET_OP_TYPE_READ;

    KTimer::Create(/*out*/_spTimer, GetThisAllocator(), KTL_TAG_TIMER);
    if (_spTimer == nullptr)
    {
        KInvariant(false);
    }
}

UdpReadOp::~UdpReadOp()
{
}

void UdpReadOp::StartRead(
    __in_opt KAsyncContextBase* const parent,
    __out KBuffer& buffer,
    __out ISocketAddress& address,
    __in UdpReadOpCompletedCallback completionCallback,
    __in ULONG timeoutInMs
)
{
    _callback = completionCallback;
    _spBuffer = &buffer;
    _spAddress = &address;
    _timeoutInMs = timeoutInMs;
    _bytesTransferred = 0;

    Start(parent, nullptr/*callback*/);
}

void UdpReadOp::OnStart()
{
    if (_timeoutInMs != INFINITE)
    {
        KAsyncContextBase::CompletionCallback timerCallback(this, &UdpReadOp::OnTimer);
        _spTimer->StartTimer(_timeoutInMs, this/*parent*/, timerCallback);
    }

    int res = 0;
#if !defined(PLATFORM_UNIX)
    _wsaBuf.len = _spBuffer->QuerySize();
    _wsaBuf.buf = static_cast<char*>(_spBuffer->GetBuffer());

    res = WSARecvFrom(
        _socket,
        &_wsaBuf,
        1,
        &_bytesTransferred,
        &_flags,
        reinterpret_cast<SOCKADDR*>(_spAddress->Address()),
        _spAddress->SizePtr(),
        &_olap,
        NULL // Completion routine; not used for IOCP
    );

    if ((res == 0) || ((res = WSAGetLastError()) == WSA_IO_PENDING))
    {
        return;
    }
#else
    res = _netIoManager.ReadAsync(_socket, *this, /*out*/_bytesTransferred);

    if ((res == 0) || (res == WSA_IO_PENDING))
    {
        return;
    }
#endif

    Complete(res);
}

void UdpReadOp::OnCancel()
{
    closesocket(_socket);

#if defined(PLATFORM_UNIX)
    _netIoManager.UnregisterSocket(_socket);
#endif

    // No need to call Complete
    // Operation will be completed with WSA_OPERATION_ABORTED
}

void UdpReadOp::OnCompleted()
{
    _callback(Status(), *_spBuffer, _bytesTransferred);
}

void UdpReadOp::IOCP_Completion(
    __in ULONG status,
    __in ULONG bytesTransferred
)
{
    _bytesTransferred = bytesTransferred;

    _spTimer->Cancel();

    Complete(status);
}

void UdpReadOp::OnTimer(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& context
)
{
    if (context.Status() == STATUS_SUCCESS)
    {
        // Timer has fired. Cancel this operation.
        Cancel();
    }
    else
    {
        // Timer was cancelled
        KInvariant(context.Status() == STATUS_CANCELLED);
    }
}
