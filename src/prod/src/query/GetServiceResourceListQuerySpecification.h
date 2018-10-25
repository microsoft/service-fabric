// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetServiceResourceListQuerySpecification
        : public AggregateHealthParallelQuerySpecificationBase<ServiceModel::ModelV2::ContainerServiceQueryResult, std::wstring>
    {
        DENY_COPY(GetServiceResourceListQuerySpecification)

    public:
        GetServiceResourceListQuerySpecification();

    protected:
        virtual bool IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification);
        virtual Common::ErrorCode AddEntityKeyFromHealthResult(__in ServiceModel::QueryResult & queryResult, __inout std::map<std::wstring, FABRIC_HEALTH_STATE> & healthStateMap);
        virtual std::wstring GetEntityKeyFromEntityResult(ServiceModel::ModelV2::ContainerServiceQueryResult const & entityInformation);

        virtual Common::ErrorCode OnParallelQueryExecutionComplete(
            Common::ActivityId const & activityId,
            std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> & queryResults,
            __out Transport::MessageUPtr & replyMessage);

    private:

        std::map<std::wstring, ServiceModel::ServiceQueryResult> CreateServiceResultMap(
            std::vector<ServiceModel::ServiceQueryResult> && serviceQueryResults);

    };
}
