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

Common::ErrorCode ServiceModel::QueryPagingDescription::FromPublicApi(FABRIC_QUERY_PAGING_DESCRIPTION const & pagingDescription)
{
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(pagingDescription.ContinuationToken, continuationToken_);
    maxResults_ = pagingDescription.MaxResults;

    return ErrorCode::Success();
}

void QueryPagingDescription::SetQueryArguments(__out QueryArgumentMap & argMap) const
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
    int64 maxResults;

    // Try to find the string value of ContinuationToken and MaxResults
    bool hasContinuationToken = argMap.TryGetValue(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);
    if (hasContinuationToken)
    {
        continuationToken_ = move(continuationToken);
    }

    auto error = argMap.GetInt64(Query::QueryResourceProperties::QueryMetadata::MaxResults, maxResults);

    wstring maxResultsString;

    if (error.IsSuccess())
    {
        // Max results cannot be negative
        // Max results can also not be 0, because it should have not been included in arg maps if that is the case
        if (maxResults <= 0)
        {
            argMap.TryGetValue(Query::QueryResourceProperties::QueryMetadata::MaxResults, maxResultsString);
            Trace.WriteWarning(TraceSource, "The provided MaxResults value \"{0}\" is invalid", maxResultsString);
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResultsString));
        }

        maxResults_ = maxResults;
        return ErrorCodeValue::Success;
    }
    else if (error.IsError(ErrorCodeValue::NotFound)) // If MaxResults wasn't given in arg maps
    {
        maxResults_ = MaxResultsDefaultValue;
        return ErrorCodeValue::Success;
    }
    else
    {
        Trace.WriteWarning(TraceSource, "The provided MaxResults value \"{0}\" cannot be parsed", maxResultsString);
        argMap.TryGetValue(Query::QueryResourceProperties::QueryMetadata::MaxResults, maxResultsString);
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResultsString));
    }
}
