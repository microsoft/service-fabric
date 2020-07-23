// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("NetworkNodeQueryResult");

NetworkNodeQueryResult::NetworkNodeQueryResult()
    : nodeName_()
{
}

NetworkNodeQueryResult::NetworkNodeQueryResult(std::wstring const & nodeName)
    : nodeName_(nodeName)
{
}

NetworkNodeQueryResult::NetworkNodeQueryResult(NetworkNodeQueryResult && other)
    : nodeName_(move(other.nodeName_))
{
}

NetworkNodeQueryResult const & NetworkNodeQueryResult::operator = (NetworkNodeQueryResult && other)
{
    if (this != &other)
    {
        nodeName_ = move(other.nodeName_);
    }

    return *this;
}

Common::ErrorCode NetworkNodeQueryResult::FromPublicApi(__in FABRIC_NETWORK_NODE_QUERY_RESULT_ITEM const &networkNodeQueryResult)
{    
    auto hr = StringUtility::LpcwstrToWstring(networkNodeQueryResult.NodeName, false, nodeName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    return ErrorCode::Success();
}

void NetworkNodeQueryResult::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_NETWORK_NODE_QUERY_RESULT_ITEM & publicNetworkNodeQueryResult) const
{
    publicNetworkNodeQueryResult.NodeName = heap.AddString(nodeName_);
    publicNetworkNodeQueryResult.Reserved = nullptr;
}
