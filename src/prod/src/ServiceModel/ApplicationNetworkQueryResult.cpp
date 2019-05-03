// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("ApplicationNetworkQueryResult");

ApplicationNetworkQueryResult::ApplicationNetworkQueryResult()
    : networkName_()
{
}

ApplicationNetworkQueryResult::ApplicationNetworkQueryResult(std::wstring const & networkName)
    : networkName_(networkName)
{
}

ApplicationNetworkQueryResult::ApplicationNetworkQueryResult(ApplicationNetworkQueryResult && other)
    : networkName_(move(other.networkName_))
{
}

ApplicationNetworkQueryResult const & ApplicationNetworkQueryResult::operator = (ApplicationNetworkQueryResult && other)
{
    if (this != &other)
    {
        networkName_ = move(other.networkName_);
    }

    return *this;
}

Common::ErrorCode ApplicationNetworkQueryResult::FromPublicApi(__in FABRIC_APPLICATION_NETWORK_QUERY_RESULT_ITEM const & applicationNetworkQueryResult)
{    
    auto hr = StringUtility::LpcwstrToWstring(applicationNetworkQueryResult.NetworkName, false, networkName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    return ErrorCode::Success();
}

void ApplicationNetworkQueryResult::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_APPLICATION_NETWORK_QUERY_RESULT_ITEM & publicApplicationNetworkQueryResult) const
{
    publicApplicationNetworkQueryResult.NetworkName = heap.AddString(networkName_);
    publicApplicationNetworkQueryResult.Reserved = nullptr;
}
