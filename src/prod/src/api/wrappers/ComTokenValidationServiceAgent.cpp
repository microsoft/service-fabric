// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;


// ********************************************************************************************************************
// ComTokenValidationServiceAgent::ComTokenValidationServiceAgent Implementation
//

ComTokenValidationServiceAgent::ComTokenValidationServiceAgent(ITokenValidationServiceAgentPtr const & impl)
    : IFabricTokenValidationServiceAgent()
    , ComUnknownBase()
    , impl_(impl)
{
}

ComTokenValidationServiceAgent::~ComTokenValidationServiceAgent()
{
    // Break lifetime cycle between TokenValidationServiceAgent and ServiceRoutingAgentProxy
    impl_->Release();
}


HRESULT STDMETHODCALLTYPE ComTokenValidationServiceAgent::RegisterTokenValidationService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    IFabricTokenValidationService * comInterface,
    IFabricStringResult ** serviceAddress)
{
    if (serviceAddress == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring serviceAddressResult;

    impl_->RegisterTokenValidationService(
        partitionId, 
        replicaId, 
        WrapperFactory::create_rooted_com_proxy(comInterface),
        serviceAddressResult);

    auto result = make_com<ComStringResult, IFabricStringResult>(serviceAddressResult);
    *serviceAddress = result.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComTokenValidationServiceAgent::UnregisterTokenValidationService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId)
{
    impl_->UnregisterTokenValidationService(partitionId, replicaId);

    return ComUtility::OnPublicApiReturn(S_OK);
}
