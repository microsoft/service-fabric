// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricGetDnsNameOp.h"

/*static*/
void FabricGetDnsNameOp::Create(
    __out FabricGetDnsNameOp::SPtr& spNotification,
    __in KAllocator& allocator,
    __in IDnsCache& cache,
    __in KString& serviceName,
    __in IFabricServiceManagementClient& fabricServiceClient
)
{
    spNotification = _new(TAG, allocator) FabricGetDnsNameOp(cache, serviceName, fabricServiceClient);
    KInvariant(spNotification != nullptr);
}

FabricGetDnsNameOp::FabricGetDnsNameOp(
    __in IDnsCache& cache,
    __in KString& serviceName,
    __in IFabricServiceManagementClient& fabricServiceClient
) : _cache(cache),
_spServiceName(&serviceName)
{
    fabricServiceClient.QueryInterface(
        IID_IInternalFabricServiceManagementClient2,
        _spInternalFabricServiceManagementClient2.VoidInitializationAddress());

    KInvariant(_spInternalFabricServiceManagementClient2 != nullptr);
}

FabricGetDnsNameOp::~FabricGetDnsNameOp()
{
}

void FabricGetDnsNameOp::OnStart()
{
    static ULONG timeout = 5000;

    FabricAsyncOperationCallback callback(this, &FabricGetDnsNameOp::OnFabricGetServiceDescriptionCompleted);
    OperationCallback::Create(/*out*/_spSync, GetThisAllocator(), callback);
    
   HRESULT hr = _spInternalFabricServiceManagementClient2->BeginGetCachedServiceDescription(
        static_cast<LPCWSTR>(*_spServiceName),
        timeout,
        static_cast<IFabricAsyncOperationCallback*>(_spSync.RawPtr()),
        _spContext.InitializationAddress()
    );

    if (hr != S_OK)
    {
        Complete(STATUS_SUCCESS);
    }
}

void FabricGetDnsNameOp::StartResolve(
    __in KAsyncContextBase* const parent
)
{
    Start(parent, nullptr/*completionCallback*/);
}

void FabricGetDnsNameOp::OnFabricGetServiceDescriptionCompleted(
    __in IFabricAsyncOperationContext* pContext
)
{
    KString::SPtr spDnsName;
    ComPointer<IFabricServiceDescriptionResult> spResult;

    HRESULT hr = _spInternalFabricServiceManagementClient2->EndGetCachedServiceDescription(
        pContext, 
        spResult.InitializationAddress());

    if (hr == S_OK)
    {
        const FABRIC_SERVICE_DESCRIPTION* pDesc = spResult->get_Description();
        if (pDesc->Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL)
        {
            FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1* p1 = nullptr;
            FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2* p2 = nullptr;
            FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3* p3 = nullptr;
            FABRIC_STATEFUL_SERVICE_DESCRIPTION* p = (FABRIC_STATEFUL_SERVICE_DESCRIPTION*)pDesc->Value;
            if (p->Reserved != nullptr)
            {
                p1 = (FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1*)p->Reserved;
            }
            if (p1 != nullptr)
            {
                p2 = (FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2*)p1->Reserved;
            }
            if (p2 != nullptr)
            {
                p3 = (FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3*)p2->Reserved;
            }
            if (p3 != nullptr && p3->ServiceDnsName != nullptr)
            {
                spDnsName = KString::Create(p3->ServiceDnsName, GetThisAllocator());
            }
        }
        else if (pDesc->Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS)
        {
            FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1* p1 = nullptr;
            FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2* p2 = nullptr;
            FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3* p3 = nullptr;
            FABRIC_STATELESS_SERVICE_DESCRIPTION* p = (FABRIC_STATELESS_SERVICE_DESCRIPTION*)pDesc->Value;
            if (p->Reserved != nullptr)
            {
                p1 = (FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1*)p->Reserved;
            }
            if (p1 != nullptr)
            {
                p2 = (FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2*)p1->Reserved;
            }
            if (p2 != nullptr)
            {
                p3 = (FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3*)p2->Reserved;
            }
            if (p3 != nullptr && p3->ServiceDnsName != nullptr)
            {
                spDnsName = KString::Create(p3->ServiceDnsName, GetThisAllocator());
            }
        }
    }

    if (spDnsName != nullptr && !spDnsName->IsEmpty())
    {
        _cache.Put(*spDnsName, *_spServiceName);
    }

    Complete(STATUS_SUCCESS);
}
