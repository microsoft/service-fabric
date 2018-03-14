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
// ComFaultAnalysisServiceAgent::ComFaultAnalysisServiceAgent Implementation
//

ComFaultAnalysisServiceAgent::ComFaultAnalysisServiceAgent(IFaultAnalysisServiceAgentPtr const & impl)
    : IFabricFaultAnalysisServiceAgent()
    , ComUnknownBase()
    , impl_(impl)
{
}

ComFaultAnalysisServiceAgent::~ComFaultAnalysisServiceAgent()
{
    // Break lifetime cycle between TSAgent and ServiceRoutingAgentProxy
    impl_->Release();
}

HRESULT STDMETHODCALLTYPE ComFaultAnalysisServiceAgent::RegisterFaultAnalysisService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    IFabricFaultAnalysisService * comInterface,
    IFabricStringResult ** serviceAddress)
{
    if (serviceAddress == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring serviceAddressResult;

    impl_->RegisterFaultAnalysisService(
        partitionId, 
        replicaId, 
        WrapperFactory::create_rooted_com_proxy(comInterface),
        serviceAddressResult);

    auto result = make_com<ComStringResult, IFabricStringResult>(serviceAddressResult);
    *serviceAddress = result.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComFaultAnalysisServiceAgent::UnregisterFaultAnalysisService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId)
{
    impl_->UnregisterFaultAnalysisService(partitionId, replicaId);

    return ComUtility::OnPublicApiReturn(S_OK);
}
