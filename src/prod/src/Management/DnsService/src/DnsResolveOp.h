// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "DnsResolveFqdnOp.h"

namespace DNS
{
    using ::_delete;

    class DnsResolveOp :
        public StateMachineBase
    {
        K_FORCE_SHARED(DnsResolveOp);

        BEGIN_STATEMACHINE_DEFINITION
            DECLARE_STATES_10(StartFabricResolveOps, WaitForFabricResolveOpsToFinish, ZeroFabricResolveOpsStarted, \
                ReceivedValidResponse, ReceivedEmptyResponse, \
                StartResolveHostnameOps, WaitForResolveHostnameOpsToFinish, ZeroResolveHostnameOpsStarted, ResolveHostnamesFinished, \
                AggregateResults)
            BEGIN_TRANSITIONS
                TRANSITION(Start, StartFabricResolveOps)
                TRANSITION_BOOL(StartFabricResolveOps, WaitForFabricResolveOpsToFinish, ZeroFabricResolveOpsStarted)
                TRANSITION_BOOL(WaitForFabricResolveOpsToFinish, ReceivedValidResponse, ReceivedEmptyResponse)
                TRANSITION(ReceivedValidResponse, StartResolveHostnameOps)
                TRANSITION_BOOL(StartResolveHostnameOps, WaitForResolveHostnameOpsToFinish, ZeroResolveHostnameOpsStarted)
                TRANSITION(WaitForResolveHostnameOpsToFinish, ResolveHostnamesFinished)
                TRANSITION(ZeroResolveHostnameOpsStarted, ResolveHostnamesFinished)
                TRANSITION(ResolveHostnamesFinished, AggregateResults)
                TRANSITION(ReceivedEmptyResponse, End)
                TRANSITION(AggregateResults, End)
                TRANSITION(ZeroFabricResolveOpsStarted, End)
            END_TRANSITIONS
        END_STATEMACHINE_DEFINITION

    virtual void OnBeforeStateChange(
        __in LPCWSTR fromState,
        __in LPCWSTR toState
    ) override;

    public:
        typedef KDelegate<void(__in IDnsMessage&)> DnsResolveCallback;

    public:
        static void Create(
            __out DnsResolveOp::SPtr& spResolveOp,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer,
            __in INetIoManager& netIoManager,
            __in IDnsParser& dnsParser,
            __in IFabricResolve& fabricResolve,
            __in INetworkParams& networkParams,
            __in const DnsServiceParams& params
        );

    private:
        DnsResolveOp(
            __in IDnsTracer& tracer,
            __in INetIoManager& netIoManager,
            __in IDnsParser& dnsParser,
            __in IFabricResolve& fabricResolve,
            __in INetworkParams& networkParams,
            __in const DnsServiceParams& params
        );

    public:
        void StartResolve(
            __in_opt KAsyncContextBase* const parent,
            __in KStringView& activityId,
            __in IDnsMessage& message,
            __in DnsResolveCallback resolveCallback
        );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCompleted() override;
        virtual void OnReuse() override;
        virtual void OnCancel() override;

    private:
        void OnFabricResolveCompleted(
            __in IDnsRecord& question,
            __in KArray<KString::SPtr>& arrResults
        );

        void OnDnsResolveCompleted(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase&
        );

        void OnDnsResolveFqdnCompleted(
            __in_opt IDnsMessage* message,
            __in_opt PVOID context
        );

        static void Shuffle(
            __inout KArray<KString::SPtr>& arr
        );

    private:
        IDnsTracer& _tracer;
        INetIoManager& _netIoManager;
        IDnsParser& _dnsParser;
        IFabricResolve& _fabricResolve;
        INetworkParams& _networkParams;
        const DnsServiceParams& _params;
        KStringView _activityId;
        IDnsMessage::SPtr _spMessage;
        DnsResolveCallback _resolveCallback;

    private:
        // Fabric Ops and answers, has to cleaned up on Reuse
        KArray<IFabricResolveOp::SPtr> _arrFabricOps;
        KSpinLock _lockFabricResolveOps;
        ULONG _pendingFabricResolveOps;

    private:
        // FQDN Ops, has to be cleaned up on Reuse
        KArray<DnsResolveFqdnOp::SPtr> _arrFqdnOps;
        KSpinLock _lockFqdnOps;
        ULONG _pendingFqdnOps;

    private:
        class DnsAnswer :
            public KShared<DnsAnswer>
        {
            K_FORCE_SHARED(DnsAnswer);

        public:
            static void Create(
                __out DnsAnswer::SPtr& spAnswer,
                __in KAllocator& allocator,
                __in IDnsRecord& question,
                __in KString& rawAnswer
            );

        private:
            DnsAnswer(
                __in IDnsRecord& question,
                __in KString& rawAnswer
            );

        public:
            bool IsFqdnHost() const { return _fIsFqdnHost; }
            IDnsRecord& Question() const { return *_spQuestion; }
            KString& RawAnswer() const { return *_spRawAnswer; }
            KString& Host() const { return *_spHostStr; }
            ULONG HostIpAddress() const { return _hostIpAddress; } // network byte order
            ULONG Port() const { return _port; }

            void SetHostIpAddress(__in ULONG address) { _hostIpAddress = address; }

        private:
            static void FindHostAndPort(
                __in KStringView strRawUri,
                __out KStringView& strHost,
                __out ULONG& port
            );

        private:
            bool _fIsFqdnHost;
            IDnsRecord::SPtr _spQuestion;
            KString::SPtr _spRawAnswer;
            KString::SPtr _spHostStr;
            ULONG _hostIpAddress; // network byte order
            ULONG _port;
        };

        // Has to be cleaned up on Reuse
        KHashTable<KString::SPtr, KSharedArray<DnsAnswer::SPtr>::SPtr> _htAnswers;
    };
}
