// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "DnsRemoteQueryOp.h"
#include "DnsResolveOp.h"

namespace DNS
{
    using ::_delete;

    class DnsExchangeOp :
        public StateMachineBase
    {
        K_FORCE_SHARED(DnsExchangeOp);

        BEGIN_STATEMACHINE_DEFINITION
            DECLARE_STATES_15(ReadQuestion, ReadQuestionSucceeded, ReadQuestionFailed, \
                FabricResolve, FabricResolveSucceeded, FabricResolveFailed, \
                IsRemoteResolveEnabled, RemoteResolve, RemoteResolveSucceeded, RemoteResolveFailed, \
                SerializeFabricAnswer, CreateBadRequestAnswer, CreateInternalErrorAnswer,\
                WriteAnswers, DropMessage)
            BEGIN_TRANSITIONS
                TRANSITION(Start, ReadQuestion)
                TRANSITION_BOOL(ReadQuestion, ReadQuestionSucceeded, ReadQuestionFailed)
                TRANSITION(ReadQuestionSucceeded, FabricResolve)
                TRANSITION_BOOL(ReadQuestionFailed, CreateBadRequestAnswer, DropMessage)
                TRANSITION_BOOL(FabricResolve, FabricResolveSucceeded, FabricResolveFailed)
                TRANSITION(FabricResolveSucceeded, SerializeFabricAnswer)
                TRANSITION(FabricResolveFailed, IsRemoteResolveEnabled)
                TRANSITION_BOOL(IsRemoteResolveEnabled, RemoteResolve, CreateInternalErrorAnswer)
                TRANSITION_BOOL(RemoteResolve, RemoteResolveSucceeded, RemoteResolveFailed)
                TRANSITION(RemoteResolveSucceeded, WriteAnswers)
                TRANSITION(RemoteResolveFailed, CreateInternalErrorAnswer)
                TRANSITION(CreateBadRequestAnswer, WriteAnswers)
                TRANSITION(CreateInternalErrorAnswer, SerializeFabricAnswer)
                TRANSITION_BOOL(SerializeFabricAnswer, WriteAnswers, End)
                TRANSITION(DropMessage, End)
                TRANSITION(WriteAnswers, End)
            END_TRANSITIONS
        END_STATEMACHINE_DEFINITION

        virtual void OnBeforeStateChange(
            __in LPCWSTR fromState,
            __in LPCWSTR toState
        ) override;

    public:
        static void Create(
            __out DnsExchangeOp::SPtr& spExchangeOp,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer,
            __in IDnsParser& dnsParser,
            __in INetIoManager& netIoManager,
            __in IUdpListener& udpServer,
            __in IFabricResolve& fabricResolve,
            __in INetworkParams& networkParams,
            __in const DnsServiceParams& params
        );

    private:
        DnsExchangeOp(
            __in IDnsTracer& tracer,
            __in IDnsParser& dnsParser,
            __in INetIoManager& netIoManager,
            __in IUdpListener& udpServer,
            __in IFabricResolve& fabricResolve,
            __in INetworkParams& networkParams,
            __in const DnsServiceParams& params
        );

    public:
        void StartExchange(
            __in_opt KAsyncContextBase* const parent,
            __in_opt KAsyncContextBase::CompletionCallback callbackPtr
        );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnReuse() override;
        virtual void OnCompleted() override;
        virtual void OnCancel() override;

    private:
        void OnUdpReadCompleted(
            __in NTSTATUS status,
            __in KBuffer& buffer,
            __in ULONG bytesRead
        );

        void OnDnsResolveCompleted(
            __in IDnsMessage& message
        );

        void OnDnsRemoteQueryCompleted(
            __in KBuffer& buffer,
            __in ULONG numberOfBytesInBuffer,
            __in_opt PVOID context
            );

        void OnUdpWriteCompleted(
            __in NTSTATUS status,
            __in ULONG bytesSent
        );

    private:
        IDnsTracer& _tracer;
        IDnsParser& _dnsParser;
        INetIoManager& _netIoManager;
        IUdpListener& _udpServer;
        IFabricResolve& _fabricResolve;
        const DnsServiceParams& _params;

        KLocalString<64> _activityId;
        IDnsMessage::SPtr _spMessage;
        KBuffer::SPtr _spBuffer;
        ULONG _bytesWrittenToBuffer;

        DnsResolveOp::SPtr _spDnsResolveOp;
        DnsRemoteQueryOp::SPtr _spDnsRemoteQueryOp;

        ISocketAddress::SPtr _spAddressFrom;
    };
}
