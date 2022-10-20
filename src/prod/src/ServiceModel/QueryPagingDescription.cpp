// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("QueryPagingDescription");

QueryPagingDescription::QueryPagingDescription()
    : continuationToken_()
    , maxResults_()
{
}

Common::ErrorCode QueryPagingDescription::FromPublicApi(FABRIC_QUERY_PAGING_DESCRIPTION const & pagingDescription)
{
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(pagingDescription.ContinuationToken, continuationToken_);
    maxResults_ = pagingDescription.MaxResults;

    if (maxResults_ < 0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResults_));
    }

    return ErrorCode::Success();
}

void QueryPagingDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_QUERY_PAGING_DESCRIPTION & result) const
{
    result.ContinuationToken = heap.AddString(continuationToken_);
    result.MaxResults = (long)maxResults_;
}

void QueryPagingDescription::SetQueryArguments(__inout QueryArgumentMap & argMap) const
{
    if (!continuationToken_.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::QueryMetadata::ContinuationToken,
            continuationToken_);
    }

    if (maxResults_ != 0)
    {
        argMap.Insert(
            Query::QueryResourceProperties::QueryMetadata::MaxResults,
            StringUtility::ToWString(maxResults_));
    }
}

Common::ErrorCode QueryPagingDescription::GetFromArgumentMap(QueryArgumentMap const & argMap)
{
    wstring continuationToken;

    bool hasContinuationToken = argMap.TryGetValue(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);
    if (hasContinuationToken)
    {
        continuationToken_ = move(continuationToken);
    }

    return TryGetMaxResultsFromArgumentMap(argMap, maxResults_);
}

Common::ErrorCode QueryPagingDescription::TryGetMaxResultsFromArgumentMap(
    QueryArgumentMap const & argMap,
    int64 & maxResultsOut,
    wstring const & activityId,
    wstring const & operationName)
{
    int64 maxResults;
    auto error = argMap.GetInt64(Query::QueryResourceProperties::QueryMetadata::MaxResults, maxResults);

    if (error.IsSuccess())
    {
        // Max results cannot be negative
        // Max results can also not be 0, because it should have not been included in arg maps if that is the case
        if (maxResults <= 0)
        {
            Trace.WriteWarning(TraceSource, "{0} - {1}: The provided MaxResults value \"{2}\" is invalid", activityId, operationName, maxResults);
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResults));
        }

        maxResultsOut = maxResults;
        return ErrorCodeValue::Success;
    }
    else if (error.IsError(ErrorCodeValue::NotFound)) // If MaxResults wasn't given in arg maps
    {
        maxResultsOut = MaxResultsDefaultValue;
        return ErrorCodeValue::Success;
    }
    else
    {
        wstring maxResultsString;
        argMap.TryGetValue(Query::QueryResourceProperties::QueryMetadata::MaxResults, maxResultsString);
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResultsString));
    }
}