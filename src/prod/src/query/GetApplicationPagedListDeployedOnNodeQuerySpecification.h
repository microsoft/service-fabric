// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetApplicationPagedListDeployedOnNodeQuerySpecification
        : public AggregateHealthParallelQuerySpecificationBase<ServiceModel::DeployedApplicationQueryResult, std::wstring>
    {
        DENY_COPY(GetApplicationPagedListDeployedOnNodeQuerySpecification)

    public:
        explicit GetApplicationPagedListDeployedOnNodeQuerySpecification(bool includeHealthState);
        static std::vector<QuerySpecificationSPtr> CreateSpecifications();
        static std::wstring GetSpecificationId(ServiceModel::QueryArgumentMap const & queryArgs);

    protected:
        virtual bool IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification);
        virtual Common::ErrorCode AddEntityKeyFromHealthResult(__in ServiceModel::QueryResult & healthQueryResult, __inout std::map<std::wstring, FABRIC_HEALTH_STATE> & healthStateMap);
        virtual std::wstring GetEntityKeyFromEntityResult(ServiceModel::DeployedApplicationQueryResult const & entityResult);

    private:
        static std::wstring GetInternalSpecificationId(bool includeHealthState);
        bool includeHealthState_;
    };
}
