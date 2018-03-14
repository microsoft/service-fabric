// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Reliability::ReconfigurationAgentComponent;

ErrorCode ComProxyStatefulServiceFactory::CreateReplica(
    wstring const & serviceType, 
    wstring const & serviceName, 
    vector<byte> const & initParams, 
    Guid const & partitionId,
    const FABRIC_REPLICA_ID replicaId, 
    Common::ComPointer<IFabricStatefulServiceReplica> & service)
{
    HRESULT hr;
    ComPointer<IFabricStatefulServiceReplica> comService;

    hr = factory_->CreateReplica(
        serviceType.c_str(),
        serviceName.c_str(),
        (ULONG)initParams.size(),
        initParams.data(),
        partitionId.AsGUID(),
        replicaId,
        comService.InitializationAddress());

    if(SUCCEEDED(hr))
    {
        ASSERT_IFNOT(comService, "Replica create returned a null service.");

        service = comService;
    }
        
    return ErrorCode::FromHResult(hr, true);
}
