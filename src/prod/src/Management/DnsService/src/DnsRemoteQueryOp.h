// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class DnsRemoteQueryOp :
        public StateMachineBase
    {
        K_FORCE_SHARED(DnsRemoteQueryOp);

        BEGIN_STATEMACHINE_DEFINITION
            DECLARE_STATES_11(InitNetworkParams, NextDnsServer, \
                TransportOpen, TransportClose, StartReceiveResponseOp, SendQuery, \
                SuccessfullyReceivedResponse, FailedToGetResponse, ShouldTryNextDnsServer, \
                RemoteQuerySucceeded, RemoteQueryFailed)
            BEGIN_TRANSITIONS
                TRANSITION(Start, InitNetworkParams)
                TRANSITION_BOOL(InitNetworkParams, NextDnsServer, RemoteQueryFailed)
                TRANSITION_BOOL(NextDnsServer, TransportOpen, RemoteQueryFailed)
                TRANSITION_BOOL(TransportOpen, SendQuery, FailedToGetResponse)
                TRANSITION_BOOL(SendQuery, StartReceiveResponseOp, FailedToGetResponse)
                TRANSITION(StartReceiveResponseOp, SuccessfullyReceivedResponse)
                TRANSITION(SuccessfullyReceivedResponse, TransportClose)
                TRANSITION(FailedToGetResponse, TransportClose)
                TRANSITION(TransportClose, ShouldTryNextDnsServer)
                TRANSITION_BOOL(ShouldTryNextDnsServer, NextDnsServer, RemoteQuerySucceeded)
                TRANSITION(RemoteQuerySucceeded, End)
                TRANSITION(RemoteQueryFailed, End)
            END_TRANSITIONS
        END_STATEMACHINE_DEFINITION

        virtual void OnBeforeStateChange(
            __in LPCWSTR fromState,
            __in LPCWSTR toState
            ) override;

    public:
        typedef KDelegate<void(__in KBuffer&, __in ULONG, __in_opt PVOID)> DnsRemoteQueryOpCallback;

    public:
        static void Create(
            __out DnsRemoteQueryOp::SPtr& spRemoteQueryOp,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer,
            __in INetIoManager& netIoManager,
            __in IDnsParser& dnsParser,
            __in INetworkParams& networkParams,
            __in const DnsServiceParams& params
        );

    private:
        DnsRemoteQueryOp(
            __in IDnsTracer& tracer,
            __in INetIoManager& netIoManager,
            __in IDnsParser& dnsParser,
            __in INetworkParams& networkParams,
            __in const DnsServiceParams& params
        );

    public:
        void StartRemoteQuery(
            __in_opt KAsyncContextBase* const parent,
            __in KStringView& activityId,
            __in KBuffer& buffer,
            __in IDnsMessage& message,
            __in DnsRemoteQueryOpCallback callback,
            __in_opt PVOID context = nullptr
        );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCompleted() override;
        virtual void OnReuse() override;
        virtual void OnCancel() override;

    private:
        void OnRecurseCompleted(
            __in IDnsRecord& question,
            __in KArray<KUri::SPtr>& arrResults
        );

        void OnRemoteDnsQueryCompleted(
            __in NTSTATUS status,
            __in KBuffer& buffer,
            __in ULONG bytesRead
        );

        void OnDnsQueryCompleted(
            __in NTSTATUS status,
            __in ULONG bytesSent
        );

        void OnUdpListenerClosed(
            __in NTSTATUS status
        );

    private:
        IDnsTracer& _tracer;
        IDnsParser& _dnsParser;
        INetworkParams& _networkParams;
        const DnsServiceParams& _params;

        KStringView _activityId;
        IDnsMessage::SPtr _spQueryMessage;
        IDnsMessage::SPtr _spResultMessage;
        DnsRemoteQueryOpCallback _callback;
        KBuffer::SPtr _spBuffer;
        ULONG _numberOfBytesInBuffer;
        PVOID _context;

        ULONG _currentDnsServerIndex;
        ULONG _currentDnsServiceIp;
        IUdpListener::SPtr _spUdpListener;
    };
}
