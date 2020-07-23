// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IDnsTracer.h"
#include "IDnsCache.h"

namespace DNS
{
    typedef KDelegate<void(
        __in IDnsRecord& question,
        __in KArray<KString::SPtr>&)> FabricResolveCompletedCallback;

    interface IFabricResolveOp
    {
        K_SHARED_INTERFACE(IFabricResolveOp);

    public:
        virtual void StartResolve(
            __in_opt KAsyncContextBase* const parent,
            __in KStringView& activityId,
            __in IDnsRecord& question,
            __in FabricResolveCompletedCallback callback
        ) = 0;

        virtual void CancelResolve() = 0;
    };

    interface IFabricResolve
    {
        K_SHARED_INTERFACE(IFabricResolve);

    public:
        virtual void Open(
            __in KAsyncContextBase* const parent
        ) = 0;

        virtual void CloseAsync() = 0;

        virtual IFabricResolveOp::SPtr CreateResolveOp(
            __in ULONG fabricQueryTimeoutInSeconds,
            __in const KString::SPtr& spStrPartitionPrefix,
            __in const KString::SPtr& spStrPartitionSuffix,
            __in bool fIsPartitionedQueryEnabled
        ) const = 0;

        virtual void NotifyServiceChanged(
            __in KString& serviceName,
            __in bool fServiceDeleted
        ) = 0;
    };

    interface IFabricData
    {
        K_SHARED_INTERFACE(IFabricData);

    public:
        virtual bool DeserializeServiceEndpoints(
            __in IFabricResolvedServicePartitionResult&,
            __out KArray<KString::SPtr>&
        ) const = 0;

        virtual bool DeserializePropertyValue(
            __in IFabricPropertyValueResult&,
            __out KString::SPtr&
        ) const = 0;
    };

    void CreateFabricResolve(
        __out IFabricResolve::SPtr& spResolve,
        __in KAllocator& allocator,
        __in const KString::SPtr& spServiceName,
        __in const IDnsTracer::SPtr& spTracer,
        __in const IFabricData::SPtr& spData,
        __in const IDnsCache::SPtr& spCache,
        __in IFabricServiceManagementClient* pFabricServiceClient,
        __in IFabricPropertyManagementClient* pFabricPropertyClient
    );
}
