// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class QueryPagingDescription;
    typedef std::unique_ptr<QueryPagingDescription> QueryPagingDescriptionUPtr;

    class QueryPagingDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
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
        void set_ContinuationToken(std::wstring const & continuationToken) { continuationToken_ = continuationToken; }

        __declspec(property(get = get_MaxResults, put = set_MaxResults)) int64 MaxResults;
        int64 get_MaxResults() const { return maxResults_; }
        void set_MaxResults(int64 maxResults) { if (maxResults > 0) { maxResults_ = maxResults; } } // Do not allow setting of negative numbers

        Common::ErrorCode FromPublicApi(FABRIC_QUERY_PAGING_DESCRIPTION const & pagingDescription);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_QUERY_PAGING_DESCRIPTION &) const;

        void SetQueryArguments(__inout QueryArgumentMap & argMap) const;

        // Fills out this object with values from argMap
        Common::ErrorCode GetFromArgumentMap(QueryArgumentMap const & argMap);

        // Return success if maxResults not found, or found and successfully parsed (is a positive number)
        // Otherwise, return ErrorCodeValue::InvalidArgument
        // If a maxResults has been found and successfully parsed, then add an entry into argMap.
        static Common::ErrorCode TryGetMaxResultsFromArgumentMap(
            QueryArgumentMap const & argMap,
            __out int64 & maxResultsOut,
            std::wstring const & activityId = std::wstring(),
            std::wstring const & operationName = std::wstring());

        static const int64 MaxResultsDefaultValue = std::numeric_limits<int64>::max();

        FABRIC_FIELDS_02(
            continuationToken_,
            maxResults_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ContinuationToken, continuationToken_);
        SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxResults, maxResults_);
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring continuationToken_;
        int64 maxResults_;
    };
}
