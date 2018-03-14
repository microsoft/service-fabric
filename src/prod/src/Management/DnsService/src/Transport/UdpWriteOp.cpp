// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "UdpWriteOp.h"

/*static*/
void UdpWriteOp::Create(
    __out UdpWriteOp::SPtr& spWriteOp,
    __in KAllocator& allocator,
    __in SOCKET socket,
    __in INetIoManager& netIoManager
)
{
    spWriteOp = _new(TAG, allocator) UdpWriteOp(socket, netIoManager);
    KInvariant(spWriteOp != nullptr);
}

UdpWriteOp::UdpWriteOp(
    __in SOCKET socket,
    __in INetIoManager& netIoManager
) : _socket(socket),
_netIoManager(netIoManager),
_numberOfBytesToWrite(0),
_bytesTransferred(0)
{
#if !defined(PLATFORM_UNIX)
    RtlZeroMemory(&_wsaBuf, sizeof(_wsaBuf));
#endif
    RtlZeroMemory(&_olap, sizeof(_olap));
    _olap._pOverlapOp = this;
    _olap._opType = SOCKET_OP_TYPE_WRITE;
}

UdpWriteOp::~UdpWriteOp()
{
}

void UdpWriteOp::StartWrite(
    __in_opt KAsyncContextBase* const parent,
    __in KBuffer& buffer,
    __in ULONG numberOfBytesToWrite,
    __in ISocketAddress& address,
    __in UdpWriteOpCompletedCallback callback
)
{
    _bytesTransferred = 0;
    _callback = callback;
    _spBuffer = &buffer;
    _numberOfBytesToWrite = numberOfBytesToWrite;
    _spAddress = &address;

    Start(parent, nullptr/*callback*/);
}

void UdpWriteOp::OnStart()
{
    int res = 0;

#if !defined(PLATFORM_UNIX)
    _wsaBuf.len = min(_numberOfBytesToWrite, _spBuffer->QuerySize());
    _wsaBuf.buf = static_cast<PCHAR>(_spBuffer->GetBuffer());

    DWORD dwFlags = 0;
    res = WSASendTo(
        _socket,
        &_wsaBuf,
        1,
        &_bytesTransferred,
        dwFlags,
        reinterpret_cast<SOCKADDR*>(_spAddress->Address()),
        _spAddress->Size(),
        &_olap,
        NULL // Completion routine; not used for IOCP
    );

    if ((res == 0) || ((res = WSAGetLastError()) == WSA_IO_PENDING))
    {
        return;
    }
#else
    res = _netIoManager.WriteAsync(
        _socket,
        *_spBuffer,
        _numberOfBytesToWrite,
        *_spAddress,
        *this,
        /*out*/_bytesTransferred
    );

    if ((res == 0) || (res == WSA_IO_PENDING))
    {
        return;
    }
#endif

    Complete(res);
}

void UdpWriteOp::OnCompleted()
{
    _callback(Status(), _bytesTransferred);
}

void UdpWriteOp::IOCP_Completion(
    __in ULONG status,
    __in ULONG bytesTransferred
)
{
    _bytesTransferred = bytesTransferred;
    Complete(status);
}
