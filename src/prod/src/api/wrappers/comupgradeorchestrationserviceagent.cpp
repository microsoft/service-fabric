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
// ComUpgradeOrchestrationServiceAgent::ComUpgradeOrchestrationServiceAgent Implementation
//

ComUpgradeOrchestrationServiceAgent::ComUpgradeOrchestrationServiceAgent(IUpgradeOrchestrationServiceAgentPtr const & impl)
    : IFabricUpgradeOrchestrationServiceAgent()
    , ComUnknownBase()
    , impl_(impl)
{
}

ComUpgradeOrchestrationServiceAgent::~ComUpgradeOrchestrationServiceAgent()
{
    // Break lifetime cycle between TSAgent and ServiceRoutingAgentProxy
    impl_->Release();
}

HRESULT STDMETHODCALLTYPE ComUpgradeOrchestrationServiceAgent::RegisterUpgradeOrchestrationService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    IFabricUpgradeOrchestrationService * comInterface,
    IFabricStringResult ** serviceAddress)
{
    if (serviceAddress == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring serviceAddressResult;

    impl_->RegisterUpgradeOrchestrationService(
        partitionId, 
        replicaId, 
        WrapperFactory::create_rooted_com_proxy(comInterface),
        serviceAddressResult);

    auto result = make_com<ComStringResult, IFabricStringResult>(serviceAddressResult);
    *serviceAddress = result.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComUpgradeOrchestrationServiceAgent::UnregisterUpgradeOrchestrationService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId)
{
    impl_->UnregisterUpgradeOrchestrationService(partitionId, replicaId);

    return ComUtility::OnPublicApiReturn(S_OK);
}
