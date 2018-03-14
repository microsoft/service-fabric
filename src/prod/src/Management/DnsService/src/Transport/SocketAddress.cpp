// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "SocketAddress.h"

void DNS::CreateSocketAddress(
    __out ISocketAddress::SPtr& spAddress,
    __in KAllocator& allocator
)
{
    SocketAddress::SPtr sp;
    SocketAddress::Create(/*out*/sp, allocator);

    spAddress = sp.RawPtr();
}

void DNS::CreateSocketAddress(
    __out ISocketAddress::SPtr& spAddress,
    __in KAllocator& allocator,
    __in ULONG address,
    __in USHORT port
)
{
    SocketAddress::SPtr sp;
    SocketAddress::Create(/*out*/sp, allocator);

    sp->Set(address, port);

    spAddress = sp.RawPtr();
}

/*static*/
void SocketAddress::Create(
    __out SocketAddress::SPtr& spAddress,
    __in KAllocator& allocator
)
{
    spAddress = _new(TAG, allocator) SocketAddress();
    KInvariant(spAddress != nullptr);
}

SocketAddress::SocketAddress()
{
    RtlZeroMemory(&_address, sizeof(_address));
    _size = sizeof(_address);
}

SocketAddress::~SocketAddress()
{
}

void SocketAddress::Set(
    __in ULONG address,
    __in USHORT port
)
{
    SOCKADDR_IN address4;
    RtlZeroMemory(&address4, sizeof(address4));
    address4.sin_family = AF_INET;
    address4.sin_addr.s_addr = address;
    address4.sin_port = port;

    KMemCpySafe(&_address, sizeof(_address), &address4, sizeof(address4));
}
