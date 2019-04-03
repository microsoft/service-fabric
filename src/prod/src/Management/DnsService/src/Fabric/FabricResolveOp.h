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
            DECLARE_STATES_14(CacheResolve, IsUnknownDnsName, IsFabricName, \
                GetServiceDescription, GetServiceDescriptionSucceeded, GetServiceDescriptionFailed, \
                MapDnsNameToFabricName, MapDnsNameToFabricNameSucceeded, MapDnsNameToFabricNameFailed, \
                ResolveFabricName, ResolveFabricNameSucceeded, ResolveFabricNameFailed, \
                FabricResolveSucceeded, FabricResolveFailed)
            BEGIN_TRANSITIONS
                TRANSITION(Start, IsFabricName)
                TRANSITION_BOOL(IsFabricName, GetServiceDescription, CacheResolve)
                TRANSITION_BOOL(CacheResolve, GetServiceDescription, IsUnknownDnsName)
                TRANSITION_BOOL(IsUnknownDnsName, MapDnsNameToFabricName, FabricResolveFailed)
                TRANSITION_BOOL(MapDnsNameToFabricName, MapDnsNameToFabricNameSucceeded, MapDnsNameToFabricNameFailed)
                TRANSITION(MapDnsNameToFabricNameSucceeded, GetServiceDescription)
                TRANSITION_BOOL(GetServiceDescription, GetServiceDescriptionSucceeded, GetServiceDescriptionFailed)
                TRANSITION(GetServiceDescriptionSucceeded, ResolveFabricName)
                TRANSITION(GetServiceDescriptionFailed, FabricResolveFailed)
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
            __in ULONG fabricQueryTimeoutInSeconds,
            __in const KString::SPtr& spStrPartitionPrefix,
            __in const KString::SPtr& spStrPartitionSuffix,
            __in bool fIsPartitionedQueryEnabled
        );

    private:
        FabricResolveOp(
            __in KString& serviceName,
            __in IDnsTracer& tracer,
            __in IFabricData& data,
            __in IDnsCache& cache,
            __in IFabricServiceManagementClient& fabricServiceClient,
            __in IFabricPropertyManagementClient& fabricPropertyClient,
            __in ULONG fabricQueryTimeoutInSeconds,
            __in const KString::SPtr& spStrPartitionPrefix,
            __in const KString::SPtr& spStrPartitionSuffix,
            __in bool fIsPartitionedQueryEnabled
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
        void OnFabricGetServiceDescriptionCompleted(
            __in IFabricAsyncOperationContext* pResolveContext
        );

        void OnFabricResolveCompleted(
            __in IFabricAsyncOperationContext* pResolveContext
        );

        void OnFabricGetPropertyCompleted(
            __in IFabricAsyncOperationContext* pResolveContext
        );

        static void ExtractPartition(
            __in KString& strQuestion,
            __in KAllocator& allocator,
            __in const KString::SPtr& spStrPartitionPrefix,
            __in const KString::SPtr& spStrPartitionSuffix,
            __out KString::SPtr& spStrPartition,
            __out KString::SPtr& spStrDnsName
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

        KString::SPtr _spStrPartitionName;
        KString::SPtr _spStrDnsName;

        KString::SPtr _spFabricName;
        DnsNameType _nameType;
        OperationCallback::SPtr _spSync;
        ComPointer<IFabricAsyncOperationContext> _spResolveContext;
        ComPointer<IFabricAsyncOperationContext> _spServiceDescriptionContext;
        ComPointer<IInternalFabricServiceManagementClient2> _spInternalFabricServiceManagementClient2;
        KArray<KString::SPtr> _arrResults;

        KDateTime _timeGetServiceDescription;
        KDateTime _timeResolveServicePartition;
        KDateTime _timeGetProperty;

        FABRIC_SERVICE_DESCRIPTION_KIND _serviceKind;
        FABRIC_PARTITION_SCHEME _partitionKind;
        KString::SPtr _spStrPartitionPrefix;
        KString::SPtr _spStrPartitionSuffix;

        bool _fIsPartitionedQueryEnabled;
    };
}
