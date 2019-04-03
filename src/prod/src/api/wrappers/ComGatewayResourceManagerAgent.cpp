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
// ComGatewayResourceManagerAgent::ComGatewayResourceManagerAgent Implementation
//

ComGatewayResourceManagerAgent::ComGatewayResourceManagerAgent(IGatewayResourceManagerAgentPtr const & impl)
    : IFabricGatewayResourceManagerAgent()
    , ComUnknownBase()
    , impl_(impl)
{
}

ComGatewayResourceManagerAgent::~ComGatewayResourceManagerAgent()
{
    // Break lifetime cycle between TSAgent and ServiceRoutingAgentProxy
    impl_->Release();
}

HRESULT STDMETHODCALLTYPE ComGatewayResourceManagerAgent::RegisterGatewayResourceManager(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    IFabricGatewayResourceManager * comInterface,
    IFabricStringResult ** serviceAddress)
{
    if (serviceAddress == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring serviceAddressResult;

    impl_->RegisterGatewayResourceManager(
        partitionId, 
        replicaId, 
        WrapperFactory::create_rooted_com_proxy(comInterface),
        serviceAddressResult);

    auto result = make_com<ComStringResult, IFabricStringResult>(serviceAddressResult);
    *serviceAddress = result.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComGatewayResourceManagerAgent::UnregisterGatewayResourceManager(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId)
{
    impl_->UnregisterGatewayResourceManager(partitionId, replicaId);

    return ComUtility::OnPublicApiReturn(S_OK);
}
