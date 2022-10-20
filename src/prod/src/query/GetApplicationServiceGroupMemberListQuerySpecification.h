// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetApplicationServiceGroupMemberListQuerySpecification
        : public AggregateHealthParallelQuerySpecificationBase<ServiceModel::ServiceGroupMemberQueryResult, std::wstring>
    {
        DENY_COPY(GetApplicationServiceGroupMemberListQuerySpecification)

    public:
        explicit GetApplicationServiceGroupMemberListQuerySpecification(bool adhocService);
        static std::vector<QuerySpecificationSPtr> CreateSpecifications();
        static std::wstring GetSpecificationId(ServiceModel::QueryArgumentMap const & queryArgs);

    protected:
        virtual bool IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification);
        virtual Common::ErrorCode AddEntityKeyFromHealthResult(__in ServiceModel::QueryResult & queryResult, __inout std::map<std::wstring, FABRIC_HEALTH_STATE> & healthStateMap);
        virtual std::wstring GetEntityKeyFromEntityResult(ServiceModel::ServiceGroupMemberQueryResult const & entityInformation);

        virtual Common::ErrorCode OnParallelQueryExecutionComplete(
            Common::ActivityId const & activityId,
            std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> & queryResults,
            __out Transport::MessageUPtr & replyMessage);

    private:
        static bool IsAdhocServicesRequest(ServiceModel::QueryArgumentMap const & queryArgs);
        static std::wstring GetInternalSpecificationId(bool isAdhoc);

        std::map<std::wstring, ServiceModel::ServiceGroupMemberQueryResult> CreateServiceGroupMemberResultMap(
            std::vector<ServiceModel::ServiceGroupMemberQueryResult> && ServiceGroupMemberQueryResults);
        std::map<std::wstring, ServiceModel::ServiceQueryResult> GetApplicationServiceGroupMemberListQuerySpecification::CreateServiceResultMap(
            std::vector<ServiceModel::ServiceQueryResult> && serviceQueryResults);

        bool isAdhocServicesSpecification_;
    };
}
