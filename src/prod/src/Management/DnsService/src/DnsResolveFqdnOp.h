// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "DnsRemoteQueryOp.h"

namespace DNS
{
    using ::_delete;

    class DnsResolveFqdnOp :
        public StateMachineBase
    {
        K_FORCE_SHARED(DnsResolveFqdnOp);

        BEGIN_STATEMACHINE_DEFINITION
            DECLARE_STATES_9(IsFqdnRelative, AppendSuffix, IsFqdnRelativeAndHasMoreSuffixes, \
                SendQuery, WaitForResponse, ReceivedValidResponse, ReceivedEmptyResponse, \
                ResolveSucceeded, ResolveFailed)
            BEGIN_TRANSITIONS
                TRANSITION(Start, IsFqdnRelative)
                TRANSITION_BOOL(IsFqdnRelative, AppendSuffix, SendQuery)
                TRANSITION(AppendSuffix, SendQuery)
                TRANSITION(SendQuery, WaitForResponse)
                TRANSITION_BOOL(WaitForResponse, ReceivedValidResponse, ReceivedEmptyResponse)
                TRANSITION(ReceivedValidResponse, ResolveSucceeded)
                TRANSITION(ReceivedEmptyResponse, IsFqdnRelativeAndHasMoreSuffixes)
                TRANSITION_BOOL(IsFqdnRelativeAndHasMoreSuffixes, AppendSuffix, ResolveFailed)
                TRANSITION(ResolveSucceeded, End)
                TRANSITION(ResolveFailed, End)
            END_TRANSITIONS
        END_STATEMACHINE_DEFINITION

        virtual void OnBeforeStateChange(
            __in LPCWSTR fromState,
            __in LPCWSTR toState
        ) override;

    public:
        typedef KDelegate<void(__in_opt IDnsMessage*, __in_opt PVOID)> DnsResolveFqdnOpCallback;

    public:
        static void Create(
            __out DnsResolveFqdnOp::SPtr& spResolveOp,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer,
            __in INetIoManager& netIoManager,
            __in IDnsParser& dnsParser,
            __in INetworkParams& networkParams,
            __in const DnsServiceParams& params
        );

    private:
        DnsResolveFqdnOp(
            __in IDnsTracer& tracer,
            __in INetIoManager& netIoManager,
            __in IDnsParser& dnsParser,
            __in INetworkParams& networkParams,
            __in const DnsServiceParams& params
        );

    public:
        void StartResolve(
            __in_opt KAsyncContextBase* const parent,
            __in KStringView& activityId,
            __in KString& strQuestion,
            __in DnsResolveFqdnOpCallback resolveCallback,
            __in_opt PVOID context
        );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;
        using KAsyncContextBase::Reuse;

        virtual void OnStart() override;
        virtual void OnCompleted() override;
        virtual void OnCancel() override;

    private:
        void OnDnsRemoteQueryCompleted(
            __in KBuffer& buffer,
            __in ULONG numberOfBytesInBuffer,
            __in_opt PVOID context
        );

    private:
        IDnsTracer& _tracer;
        INetIoManager& _netIoManager;
        IDnsParser& _dnsParser;
        INetworkParams& _networkParams;
        const DnsServiceParams& _params;

        KStringView _activityId;
        KString::SPtr _spQuestionStr;
        DnsResolveFqdnOpCallback _resolveCallback;

        KString::SPtr _spModifiedQuestionStr;
        DnsRemoteQueryOp::SPtr _spQuery;
        IDnsMessage::SPtr _spMessage;

        bool _fIsFqdnRelative;
        ULONG _currentSuffixIndex;
        PVOID _context;
    };
}
