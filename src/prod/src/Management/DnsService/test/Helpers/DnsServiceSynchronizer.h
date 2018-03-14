// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS { namespace Test
{
    class DnsServiceSynchronizer
    {
        K_DENY_COPY(DnsServiceSynchronizer);
    public:
        DnsServiceSynchronizer();

        ~DnsServiceSynchronizer();

        operator DnsServiceCallback() { return _callback; }

        operator UdpWriteOpCompletedCallback() { return _udpWriteCallback; }

        operator UdpReadOpCompletedCallback() { return _udpReadCallback; }

        bool Wait(
            __in ULONG timeoutInMilliseconds = INFINITE
        );

        void Reset();

        DWORD Size() const { return _size; }

    private:
        void OnCompletion(
            __in NTSTATUS
        );

        void OnUdpWriteCompletion(
            __in NTSTATUS,
            __in ULONG
        );

        void OnUdpReadCompletion(
            __in NTSTATUS,
            __in KBuffer&,
            __in ULONG
        );

    private:
        Common::ManualResetEvent _event;
        DnsServiceCallback _callback;
        UdpWriteOpCompletedCallback _udpWriteCallback;
        UdpReadOpCompletedCallback _udpReadCallback;

        DWORD _size;
    };
}}
