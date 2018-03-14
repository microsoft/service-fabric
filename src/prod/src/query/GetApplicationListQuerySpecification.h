// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetApplicationListQuerySpecification
        : public AggregateHealthParallelQuerySpecificationBase<ServiceModel::ApplicationQueryResult, std::wstring>
    {
    public:
        GetApplicationListQuerySpecification();

    protected:
        virtual bool IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification);
        virtual Common::ErrorCode AddEntityKeyFromHealthResult(__in ServiceModel::QueryResult & healthQueryResult, __inout std::map<std::wstring, FABRIC_HEALTH_STATE> & healthStateMap);
        virtual std::wstring GetEntityKeyFromEntityResult(ServiceModel::ApplicationQueryResult const & entityResult);
    };
}
