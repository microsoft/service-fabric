// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "OperationCallback.h"

namespace DNS
{
    using ::_delete;

    class FabricResolveOp :
        public StateMachineBase,
        public IFabricResolveOp
    {
        K_FORCE_SHARED(FabricResolveOp);
        K_SHARED_INTERFACE_IMP(IFabricResolveOp);

        BEGIN_STATEMACHINE_DEFINITION
            DECLARE_STATES_11(CacheResolve, IsUnknownDnsName, \
                IsFabricName, MapDnsNameToFabricName, MapDnsNameToFabricNameSucceeded, \
                MapDnsNameToFabricNameFailed, ResolveFabricName, ResolveFabricNameSucceeded, \
                ResolveFabricNameFailed, FabricResolveSucceeded, FabricResolveFailed)
            BEGIN_TRANSITIONS
                TRANSITION(Start, IsFabricName)
                TRANSITION_BOOL(IsFabricName, ResolveFabricName, CacheResolve)
                TRANSITION_BOOL(CacheResolve, ResolveFabricName, IsUnknownDnsName)
                TRANSITION_BOOL(IsUnknownDnsName, MapDnsNameToFabricName, FabricResolveFailed)
                TRANSITION_BOOL(MapDnsNameToFabricName, MapDnsNameToFabricNameSucceeded, MapDnsNameToFabricNameFailed)
                TRANSITION(MapDnsNameToFabricNameSucceeded, ResolveFabricName)
                TRANSITION(MapDnsNameToFabricNameFailed, FabricResolveFailed)
                TRANSITION_BOOL(ResolveFabricName, ResolveFabricNameSucceeded, ResolveFabricNameFailed)
                TRANSITION(ResolveFabricNameSucceeded, FabricResolveSucceeded)
                TRANSITION(ResolveFabricNameFailed, FabricResolveFailed)
                TRANSITION(FabricResolveSucceeded, End)
                TRANSITION(FabricResolveFailed, End)
            END_TRANSITIONS
        END_STATEMACHINE_DEFINITION

        virtual void OnBeforeStateChange(
            __in LPCWSTR fromState,
            __in LPCWSTR toState
            ) override;

    public:
        static void Create(
            __out FabricResolveOp::SPtr& spResolveOp,
            __in KAllocator& allocator,
            __in KString& serviceName,
            __in IDnsTracer& tracer,
            __in IFabricData& data,
            __in IDnsCache& cache,
            __in IFabricServiceManagementClient& fabricServiceClient,
            __in IFabricPropertyManagementClient& fabricPropertyClient,
            __in ULONG fabricQueryTimeoutInSeconds
        );

    private:
        FabricResolveOp(
            __in KString& serviceName,
            __in IDnsTracer& tracer,
            __in IFabricData& data,
            __in IDnsCache& cache,
            __in IFabricServiceManagementClient& fabricServiceClient,
            __in IFabricPropertyManagementClient& fabricPropertyClient,
            __in ULONG fabricQueryTimeoutInSeconds
        );

    public:
        // IFabricResolveOp Impl.
        virtual void StartResolve(
            __in_opt KAsyncContextBase* const parent,
            __in KStringView& activityId,
            __in IDnsRecord& record,
            __in FabricResolveCompletedCallback callback
        ) override;

        virtual void CancelResolve() override;

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;
        using KAsyncContextBase::Cancel;

        virtual void OnStart() override;
        virtual void OnCompleted() override;
        virtual void OnCancel() override;

    private:
        void OnFabricResolveCompleted(
            __in IFabricAsyncOperationContext* pResolveContext
        );

        void OnFabricGetPropertyCompleted(
            __in IFabricAsyncOperationContext* pResolveContext
        );

    private:
        KString& _serviceName;
        IDnsTracer& _tracer;
        IFabricData& _data;
        IDnsCache& _cache;
        IFabricServiceManagementClient& _fabricServiceClient;
        IFabricPropertyManagementClient& _fabricPropertyClient;
        const ULONG _fabricQueryTimeoutInSeconds;

        KStringView _activityId;
        IDnsRecord::SPtr _spQuestion;
        FabricResolveCompletedCallback _callback;

        KString::SPtr _spFabricName;
        DnsNameType _nameType;
        OperationCallback::SPtr _spSync;
        ComPointer<IFabricAsyncOperationContext> _spResolveContext;
        KArray<KString::SPtr> _arrResults;
    };
}
