//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Query
{
    // This specification needs to constructed explicitly as its query specification list is dynamic.
    class GetDeployedCodePackageParallelQuerySpecification
        : public ParallelQuerySpecification
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
        DENY_COPY(GetDeployedCodePackageParallelQuerySpecification)

    public:
        GetDeployedCodePackageParallelQuerySpecification(
            std::wstring const & applicationNameString,
            std::vector<std::wstring> const & nodeNames);

        Common::ErrorCode OnParallelQueryExecutionComplete(
            Common::ActivityId const & activityId,
            std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> & queryResults,
            __out Transport::MessageUPtr & replyMessage) override;

        ServiceModel::QueryArgumentMap GetQueryArgumentMap(
            size_t index,
            ServiceModel::QueryArgumentMap const & baseArg) override;

    private:
        Common::ErrorCode MergeDeployedCodePackageResults(
            __inout std::vector<ServiceModel::DeployedCodePackageQueryResult> & mergedQueryResult,
            __in ServiceModel::QueryResult & results);

        std::vector<ServiceModel::QueryArgumentMap> queryArgumentMaps_;
    };
}
