// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetServicePartitionReplicaListQuerySpecification
        : public AggregateHealthParallelQuerySpecificationBase<ServiceModel::ServiceReplicaQueryResult, int64>
    {
        DENY_COPY(GetServicePartitionReplicaListQuerySpecification)
    public:

        explicit GetServicePartitionReplicaListQuerySpecification(bool FMMQuery);
        static std::vector<QuerySpecificationSPtr> CreateSpecifications();
        static std::wstring GetSpecificationId(ServiceModel::QueryArgumentMap const & queryArgs);
    protected:

        virtual bool IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification);
        Common::ErrorCode AddEntityKeyFromHealthResult(__in ServiceModel::QueryResult & queryResult, __inout std::map<int64, FABRIC_HEALTH_STATE> & healthStateMap);
        virtual int64 GetEntityKeyFromEntityResult(ServiceModel::ServiceReplicaQueryResult const & healthInformation);
    private:

        static bool ShouldSendToFMM(ServiceModel::QueryArgumentMap const & queryArgs);
        static std::wstring GetInternalSpecificationId(bool shouldSendToFMM);

    };
}
