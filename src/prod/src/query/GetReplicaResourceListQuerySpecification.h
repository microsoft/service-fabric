//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Query
{
    class GetReplicaResourceListQuerySpecification
        : public SequentialQuerySpecification
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
        DENY_COPY(GetReplicaResourceListQuerySpecification)
    public:
        GetReplicaResourceListQuerySpecification();

        Common::ErrorCode GetNext(
            QuerySpecificationSPtr const & previousQuerySpecification,
            __in std::unordered_map<Query::QueryNames::Enum, ServiceModel::QueryResult> & queryResults,
            __inout ServiceModel::QueryArgumentMap & queryArgs,
            __out Query::QuerySpecificationSPtr & nextQuery,
            __out Transport::MessageUPtr & replyMessage) override;

        bool ShouldContinueSequentialQuery(
            Query::QuerySpecificationSPtr const & lastExecutedQuerySpecification,
            Common::ErrorCode const & error) override;
    };
}
