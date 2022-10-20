// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricResolve.h"
#include "FabricResolveOp.h"
#include "FabricGetDnsNameOp.h"

void DNS::CreateFabricResolve(
    __out IFabricResolve::SPtr& spResolve,
    __in KAllocator& allocator,
    __in const KString::SPtr& spServiceName,
    __in const IDnsTracer::SPtr& spTracer,
    __in const IFabricData::SPtr& spData,
    __in const IDnsCache::SPtr& spCache,
    __in IFabricServiceManagementClient* pFabricServiceClient,
    __in IFabricPropertyManagementClient* pFabricPropertyClient
)
{
    FabricResolve::SPtr spOp;
    FabricResolve::Create(/*out*/spOp, allocator, spServiceName, spTracer, spData, spCache, pFabricServiceClient, pFabricPropertyClient);

    spResolve = spOp.RawPtr();
}

/*static*/
void FabricResolve::Create(
    __out FabricResolve::SPtr& spResolveOp,
    __in KAllocator& allocator,
    __in const KString::SPtr& spServiceName,
    __in const IDnsTracer::SPtr& spTracer,
    __in const IFabricData::SPtr& spData,
    __in const IDnsCache::SPtr& spCache,
    __in IFabricServiceManagementClient* pFabricServiceClient,
    __in IFabricPropertyManagementClient* pFabricPropertyClient
)
{
    spResolveOp = _new(TAG, allocator) FabricResolve(spServiceName, spTracer, spData, spCache, pFabricServiceClient, pFabricPropertyClient);
    KInvariant(spResolveOp != nullptr);
}

FabricResolve::FabricResolve(
    __in const KString::SPtr& spServiceName,
    __in const IDnsTracer::SPtr& spTracer,
    __in const IFabricData::SPtr& spData,
    __in const IDnsCache::SPtr& spCache,
    __in IFabricServiceManagementClient* pFabricServiceClient,
    __in IFabricPropertyManagementClient* pFabricPropertyClient
) : _fActive(false),
_spServiceName(spServiceName),
_spTracer(spTracer),
_spData(spData),
_spCache(spCache),
_spFabricServiceClient(pFabricServiceClient),
_spFabricPropertyClient(pFabricPropertyClient)
{
}

FabricResolve::~FabricResolve()
{
    Tracer().Trace(DnsTraceLevel_Noise, "Destructing FabricResolve.");
}

void FabricResolve::Open(
    __in KAsyncContextBase* const parent
)
{
    Start(parent, nullptr/*callback*/);
}

void FabricResolve::CloseAsync()
{
    K_LOCK_BLOCK(_lock)
    {
        _fActive = false;
        Cancel();
    }
}

IFabricResolveOp::SPtr FabricResolve::CreateResolveOp(
    __in ULONG fabricQueryTimeoutInSeconds,
    __in const KString::SPtr& spStrPartitionPrefix,
    __in const KString::SPtr& spStrPartitionSuffix,
    __in bool fIsPartitionedQueryEnabled
) const
{
    FabricResolveOp::SPtr spOp;
    FabricResolveOp::Create(/*out*/spOp, GetThisAllocator(),
        *_spServiceName, *_spTracer.RawPtr(), *_spData.RawPtr(), *_spCache.RawPtr(),
        *_spFabricServiceClient.RawPtr(), *_spFabricPropertyClient.RawPtr(), fabricQueryTimeoutInSeconds,
        spStrPartitionPrefix, spStrPartitionSuffix, fIsPartitionedQueryEnabled);

    IFabricResolveOp::SPtr spOut = spOp.RawPtr();
    return spOut;
}

void FabricResolve::NotifyServiceChanged(
    __in KString& serviceName,
    __in bool fServiceDeleted
)
{
    if (fServiceDeleted)
    {
        _spCache->Remove(serviceName);
    }
    else if (!_spCache->IsServiceKnown(serviceName))
    {
        K_LOCK_BLOCK(_lock)
        {
            if (_fActive)
            {
                FabricGetDnsNameOp::SPtr spOp;
                FabricGetDnsNameOp::Create(/*out*/spOp, GetThisAllocator(),
                    *_spCache, serviceName, *_spFabricServiceClient.RawPtr());
                spOp->StartResolve(this);
            }
        }
    }
}

void FabricResolve::OnStart()
{
    _fActive = true;
}

void FabricResolve::OnCompleted()
{
    Tracer().Trace(DnsTraceLevel_Info, "FabricResolve OnCompleted called.");
}

void FabricResolve::OnReuse()
{
    Tracer().Trace(DnsTraceLevel_Noise, "FabricResolve OnReuse called.");
}

void FabricResolve::OnCancel()
{
    Tracer().Trace(DnsTraceLevel_Info, "FabricResolve OnCancel called.");
    Complete(STATUS_CANCELLED);
}
