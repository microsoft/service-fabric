// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("NetworkApplicationQueryResult");

NetworkApplicationQueryResult::NetworkApplicationQueryResult()
    : applicationName_()
{
}

NetworkApplicationQueryResult::NetworkApplicationQueryResult(std::wstring const & applicationName)
    : applicationName_(applicationName)
{
}

NetworkApplicationQueryResult::NetworkApplicationQueryResult(NetworkApplicationQueryResult && other)
    : applicationName_(move(other.applicationName_))
{
}

NetworkApplicationQueryResult const & NetworkApplicationQueryResult::operator = (NetworkApplicationQueryResult && other)
{
    if (this != &other)
    {
        applicationName_ = move(other.applicationName_);
    }

    return *this;
}

Common::ErrorCode NetworkApplicationQueryResult::FromPublicApi(__in FABRIC_NETWORK_APPLICATION_QUERY_RESULT_ITEM const &networkApplicationQueryResult)
{    
    auto hr = StringUtility::LpcwstrToWstring(networkApplicationQueryResult.ApplicationName, false, applicationName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    return ErrorCode::Success();
}

void NetworkApplicationQueryResult::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_NETWORK_APPLICATION_QUERY_RESULT_ITEM & publicNetworkApplicationQueryResult) const
{
    publicNetworkApplicationQueryResult.ApplicationName = heap.AddString(applicationName_);
    publicNetworkApplicationQueryResult.Reserved = nullptr;
}
