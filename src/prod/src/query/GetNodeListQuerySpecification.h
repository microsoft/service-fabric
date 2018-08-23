// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetNodeListQuerySpecification
        : public ParallelQuerySpecification
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
    public:
         explicit GetNodeListQuerySpecification(bool);
         static std::vector<QuerySpecificationSPtr> CreateSpecifications();

         virtual Common::ErrorCode OnParallelQueryExecutionComplete(
             Common::ActivityId const & activityId,
             std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> & queryResults,
             __out Transport::MessageUPtr & replyMessage);

         virtual Common::TimeSpan GetParallelQueryExecutionTimeout(
             QuerySpecificationSPtr const& querySpecification,
             Common::TimeSpan const& totalRemainingTime);

         static std::wstring GetSpecificationId(ServiceModel::QueryArgumentMap const & queryArgs);

    private:
        static const double CMGetNodeListTimeoutPercentage;
        std::map<Federation::NodeId, ServiceModel::NodeQueryResult> CreateNodeResultMap(
            std::vector<ServiceModel::NodeQueryResult> && nodeQueryResults);
        static bool ExcludeStoppedNodeInfo(ServiceModel::QueryArgumentMap const & queryArgs);
        static std::wstring GetInternalSpecificationId(bool excludeFAS);
    };
}
