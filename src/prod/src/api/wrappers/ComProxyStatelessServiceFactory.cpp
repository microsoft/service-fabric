// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyStatelessServiceFactory Implementation
//
ComProxyStatelessServiceFactory::ComProxyStatelessServiceFactory(
    ComPointer<IFabricStatelessServiceFactory> const & comImpl)
    : ComponentRoot(),
    IStatelessServiceFactory(),
    comImpl_(comImpl)
{
}

ComProxyStatelessServiceFactory::~ComProxyStatelessServiceFactory()
{
}
        
ErrorCode ComProxyStatelessServiceFactory::CreateInstance(
    wstring const & serviceType, 
    wstring const & serviceName, 
    vector<byte> const & initializationData,
    Guid const & partitionId,
    const FABRIC_INSTANCE_ID instanceId, 
    __out IStatelessServiceInstancePtr & )
{
    HRESULT hr;
    ComPointer<IFabricStatelessServiceInstance> comService;

    hr = comImpl_->CreateInstance(
        serviceType.c_str(),
        serviceName.c_str(),
        (ULONG)initializationData.size(),
        initializationData.data(),
        partitionId.AsGUID(),
        instanceId,
        comService.InitializationAddress());

    if(SUCCEEDED(hr))
    {
        ASSERT_IFNOT(comService, "Instance create returned a null service.");
        // TODO: create proxy for the stateless service
        // service = make_unique<ComProxyStatelessService>(comService);
        //
    }
    
    return ErrorCode::FromHResult(hr);
}
