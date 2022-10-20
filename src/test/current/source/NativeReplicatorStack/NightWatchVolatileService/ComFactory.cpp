// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace V1ReplPerf;

HRESULT ComFactory::CreateReplica(
    LPCWSTR serviceType,
    FABRIC_URI,
    ULONG ,
    const byte *,
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    IFabricStatefulServiceReplica **service)
{
    wstring serviceTypeString(serviceType);

    ASSERT_IFNOT(
        serviceTypeString == Constants::ServiceTypeName,
        "Unsupported service type {0}. Only Service type {1} is supported in this factory",
        serviceTypeString,
        Constants::ServiceTypeName);

    auto serviceSPtr = factory_.CreateReplica(partitionId, replicaId, root_);
    auto comPointer = make_com<ComService>(*serviceSPtr);
    return comPointer->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(service));
}
