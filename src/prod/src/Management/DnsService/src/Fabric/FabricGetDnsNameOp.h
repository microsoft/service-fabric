// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IDnsCache.h"
#include "OperationCallback.h"

namespace DNS
{
    using ::_delete;

    //
    // Forward decl.
    //
    class FabricAsyncOperationContext;

    class FabricGetDnsNameOp :
        public KAsyncContextBase
    {
        K_FORCE_SHARED(FabricGetDnsNameOp);

    public:
        static void Create(
            __out FabricGetDnsNameOp::SPtr& spNotification,
            __in KAllocator& allocator,
            __in IDnsCache& cache,
            __in KString& serviceName,
            __in IFabricServiceManagementClient& _fabricServiceClient
            );

    private:
        FabricGetDnsNameOp(
            __in IDnsCache& cache,
            __in KString& serviceName,
            __in IFabricServiceManagementClient& _fabricServiceClient
            );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;

    public:
        void StartResolve(
            __in KAsyncContextBase* const parent
        );

    private:
        void OnFabricGetServiceDescriptionCompleted(
            __in IFabricAsyncOperationContext* pResolveContext
        );

    private:
        IDnsCache& _cache;
        KString::SPtr _spServiceName;
        ComPointer<IFabricAsyncOperationContext> _spContext;
        OperationCallback::SPtr _spSync;
        ComPointer<IInternalFabricServiceManagementClient2> _spInternalFabricServiceManagementClient2;
    };
}
