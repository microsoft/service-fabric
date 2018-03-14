// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Federation;

StringLiteral const TraceSource("PagingStatus");

PagingStatus::PagingStatus()
    : continuationToken_()
{
}

PagingStatus::PagingStatus(wstring && continuationToken)
    : continuationToken_(move(continuationToken))
{
}

PagingStatus::PagingStatus(PagingStatus && other)
    : continuationToken_(move(other.continuationToken_))
{
}

PagingStatus & PagingStatus::operator =(PagingStatus && other)
{
    if (this != &other)
    {
        continuationToken_ = move(other.continuationToken_);
    }

    return *this;
}

PagingStatus::~PagingStatus()
{
}

void PagingStatus::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_PAGING_STATUS & publicPagingStatus) const
{
    publicPagingStatus.ContinuationToken = heap.AddString(continuationToken_);
}

ErrorCode PagingStatus::FromPublicApi(FABRIC_PAGING_STATUS const & publicPagingStatus)
{
    auto hr = StringUtility::LpcwstrToWstring(publicPagingStatus.ContinuationToken, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, continuationToken_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing continuation token in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    return ErrorCode::Success();
}

//
// Templated methods
//

// This will return an error if the continuation token is empty (same as other specializations of this template's base template)
template <> ErrorCode PagingStatus::GetContinuationTokenData<wstring>(
    std::wstring const & continuationToken,
    __inout wstring & data)
{
    if (continuationToken.empty())
    {
        return ErrorCode(
            ErrorCodeValue::ArgumentNull,
            wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationToken));
    }
    data = continuationToken;
    return ErrorCode::Success();
}

template <> std::wstring PagingStatus::CreateContinuationToken<Uri>(Uri const & data)
{
    return data.ToString();
}

template <> std::wstring PagingStatus::CreateContinuationToken<NodeId>(NodeId const & nodeId)
{
    return wformatString("{0}", nodeId);
}

template <> ErrorCode PagingStatus::GetContinuationTokenData<NodeId>(
    std::wstring const & continuationToken,
    __inout NodeId & nodeId)
{
    if (!NodeId::TryParse(continuationToken, nodeId))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationToken));
    }

    return ErrorCode::Success();
}

template <> std::wstring PagingStatus::CreateContinuationToken<Guid>(Guid const & guid)
{
    return wformatString("{0}", guid);
}

template <> ErrorCode PagingStatus::GetContinuationTokenData<Guid>(
    std::wstring const & continuationToken,
    __inout Guid & guid)
{
    if (!Guid::TryParse(continuationToken, guid))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationToken));
    }

    return ErrorCode::Success();
}

template <> std::wstring PagingStatus::CreateContinuationToken<FABRIC_REPLICA_ID>(FABRIC_REPLICA_ID const & replicaId)
{
    return wformatString("{0}", replicaId);
}

template <> ErrorCode PagingStatus::GetContinuationTokenData<FABRIC_REPLICA_ID>(
    std::wstring const & continuationToken,
    __inout FABRIC_REPLICA_ID & replicaId)
{
    if (!StringUtility::TryFromWString<FABRIC_REPLICA_ID>(continuationToken, replicaId))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationToken));
    }

    return ErrorCode::Success();
}
