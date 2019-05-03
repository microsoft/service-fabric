// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("DeployedNetworkQueryResult");

DeployedNetworkQueryResult::DeployedNetworkQueryResult()
{
}

DeployedNetworkQueryResult::DeployedNetworkQueryResult(std::wstring const & networkName)
    : networkName_(networkName)
{
}

DeployedNetworkQueryResult::DeployedNetworkQueryResult(DeployedNetworkQueryResult && other)
    : networkName_(move(other.networkName_))
{
}

DeployedNetworkQueryResult const & DeployedNetworkQueryResult::operator = (DeployedNetworkQueryResult && other)
{
    if (this != &other)
    {
        networkName_ = move(other.networkName_);
    }

    return *this;
}

Common::ErrorCode DeployedNetworkQueryResult::FromPublicApi(__in FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_ITEM const & deployedNetworkQueryResult)
{    
    auto hr = StringUtility::LpcwstrToWstring(deployedNetworkQueryResult.NetworkName, false, networkName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    return ErrorCode::Success();
}

void DeployedNetworkQueryResult::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_ITEM & publicDeployedNetworkQueryResult) const
{
    publicDeployedNetworkQueryResult.NetworkName = heap.AddString(networkName_);
    publicDeployedNetworkQueryResult.Reserved = nullptr;
}

std::wstring DeployedNetworkQueryResult::ToString() const
{
    return wformatString(
        "NetworkName=[{0}]\n",
        networkName_);
}
