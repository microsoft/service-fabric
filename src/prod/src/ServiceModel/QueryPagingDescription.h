// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class QueryPagingDescription
    {
        DENY_COPY(QueryPagingDescription)

    public:
        QueryPagingDescription();

        QueryPagingDescription(QueryPagingDescription && other) = default;
        QueryPagingDescription & operator = (QueryPagingDescription && other) = default;

        ~QueryPagingDescription() = default;

        __declspec(property(get = get_ContinuationToken, put = set_ContinuationToken)) std::wstring const & ContinuationToken;
        std::wstring const & get_ContinuationToken() const { return continuationToken_; }
        void set_ContinuationToken(std::wstring && continuationToken) { continuationToken_ = std::move(continuationToken); }

        __declspec(property(get = get_MaxResults, put = set_MaxResults)) int64 MaxResults;
        int64 get_MaxResults() const { return maxResults_; }
        void set_MaxResults(int64 maxResults) { maxResults_ = maxResults; }

        Common::ErrorCode FromPublicApi(FABRIC_QUERY_PAGING_DESCRIPTION const & pagingDescription);

        void SetQueryArguments(__out QueryArgumentMap & argMap) const;

        // Fills out this object with values from argMap
        Common::ErrorCode GetFromArgumentMap(QueryArgumentMap const & argMap);

        static const int64 MaxResultsDefaultValue = std::numeric_limits<int64>::max();

    private:
        std::wstring continuationToken_;
        int64 maxResults_;
    };
}

