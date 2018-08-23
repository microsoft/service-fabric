//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ComStatefulGuestServiceTypeFactory");

ComStatefulGuestServiceTypeFactory::ComStatefulGuestServiceTypeFactory(
    IGuestServiceTypeHost & typeHost)
    : typeHost_(typeHost)
{
    traceId_ = wformatString("{0}", static_cast<void*>(this));

    WriteNoise(TraceType, traceId_, "ctor");
}

ComStatefulGuestServiceTypeFactory::~ComStatefulGuestServiceTypeFactory()
{
    WriteNoise(TraceType, traceId_, "dtor");
}

HRESULT ComStatefulGuestServiceTypeFactory::CreateReplica(
    /* [in] */ LPCWSTR serviceTypeName,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [retval][out] */ IFabricStatefulServiceReplica **serviceReplica)
{
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);

    wstring serviceTypeStr(serviceTypeName);
    wstring serviceNameStr(serviceName);
    Guid partitionIdGuid(partitionId);

    WriteNoise(
        TraceType, 
        traceId_, 
        "CreateInstance: ServiceType={0}, ServiceName={1}, PartitionId={2}, ReplicaId={3}",
        serviceTypeStr,
        serviceNameStr,
        partitionIdGuid,
        replicaId);

    auto comServiceReplica = 
        make_com<ComGuestServiceReplica, IFabricStatefulServiceReplica>(
            typeHost_,
            serviceNameStr,
            serviceTypeStr,
            partitionIdGuid,
            replicaId);

    *serviceReplica = comServiceReplica.DetachNoRelease();
    
    return S_OK;
}
