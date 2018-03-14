// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetSystemServiceListQuerySpecification
        : public AggregateHealthParallelQuerySpecificationBase<ServiceModel::ServiceQueryResult, std::wstring>
    {
    public:
         GetSystemServiceListQuerySpecification();
         virtual bool IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification);
         virtual Common::ErrorCode AddEntityKeyFromHealthResult(__in ServiceModel::QueryResult & queryResult, __inout std::map<std::wstring, FABRIC_HEALTH_STATE> & healthStateMap);
         virtual std::wstring GetEntityKeyFromEntityResult(ServiceModel::ServiceQueryResult const & entityInformation);
    };
}
