// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsServiceSynchronizer.h"

DnsServiceSynchronizer::DnsServiceSynchronizer() :
    _event(false),
    _size(0)
{
    _callback = DnsServiceCallback(this, &DnsServiceSynchronizer::OnCompletion);
    _udpWriteCallback = UdpWriteOpCompletedCallback(this, &DnsServiceSynchronizer::OnUdpWriteCompletion);
    _udpReadCallback = UdpReadOpCompletedCallback(this, &DnsServiceSynchronizer::OnUdpReadCompletion);
}

DnsServiceSynchronizer::~DnsServiceSynchronizer()
{
}

bool DnsServiceSynchronizer::Wait(
    __in ULONG timeoutInMilliseconds
)
{
    Common::ErrorCode error = _event.Wait(timeoutInMilliseconds);
    if (error.IsSuccess())
    {
        return true;
    }

    return false;
}

void DnsServiceSynchronizer::Reset()
{
    _event.Reset();
}

void DnsServiceSynchronizer::OnCompletion(
    __in NTSTATUS
)
{
    _event.Set();
}

void DnsServiceSynchronizer::OnUdpWriteCompletion(
    __in NTSTATUS,
    __in ULONG
)
{
    _event.Set();
}

void DnsServiceSynchronizer::OnUdpReadCompletion(
    __in NTSTATUS,
    __in KBuffer&,
    __in ULONG size
)
{
    _size = size;
    _event.Set();
}
