// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetServicePartitionListQuerySpecification
        : public AggregateHealthParallelQuerySpecificationBase<ServiceModel::ServicePartitionQueryResult, Common::Guid>
    {
        DENY_COPY(GetServicePartitionListQuerySpecification)
    public:

        explicit GetServicePartitionListQuerySpecification(bool FMMQuery);
        static std::vector<QuerySpecificationSPtr> CreateSpecifications();
        static std::wstring GetSpecificationId(ServiceModel::QueryArgumentMap const & queryArgs);
    protected:

        virtual bool IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification);
        virtual Common::ErrorCode AddEntityKeyFromHealthResult(__in ServiceModel::QueryResult & queryResult, __inout std::map<Common::Guid, FABRIC_HEALTH_STATE> & healthStateMap);
        virtual Common::Guid GetEntityKeyFromEntityResult(ServiceModel::ServicePartitionQueryResult const & healthInformation);
    private:

        static bool ShouldSendToFMM(ServiceModel::QueryArgumentMap const & queryArgs);
        static std::wstring GetInternalSpecificationId(bool shouldSendToFMM);
    };
}
