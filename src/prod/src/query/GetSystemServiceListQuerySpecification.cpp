// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace Query;

GetSystemServiceListQuerySpecification::GetSystemServiceListQuerySpecification()
    : AggregateHealthParallelQuerySpecificationBase<ServiceQueryResult, wstring>(
        QueryNames::GetSystemServicesList,
        QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
        QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false))
{
    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetSystemServicesList,
            QueryAddresses::GetFM(),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetAggregatedServiceHealthList,
            QueryAddresses::GetHM(),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
}

bool GetSystemServiceListQuerySpecification::IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification)
{
    return querySpecification->QueryName == QueryNames::GetSystemServicesList;
}

Common::ErrorCode GetSystemServiceListQuerySpecification::AddEntityKeyFromHealthResult(
    __in ServiceModel::QueryResult & queryResult,
    __inout std::map<std::wstring, FABRIC_HEALTH_STATE> & healthStateMap)
{
    vector<ServiceAggregatedHealthState> healthQueryResult;
    auto error = queryResult.MoveList(healthQueryResult);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itHealthInfo = healthQueryResult.begin(); itHealthInfo != healthQueryResult.end(); ++itHealthInfo)
    {
        healthStateMap.insert(std::make_pair(itHealthInfo->ServiceName, itHealthInfo->AggregatedHealthState));
    }

    return error;
}

wstring GetSystemServiceListQuerySpecification::GetEntityKeyFromEntityResult(ServiceModel::ServiceQueryResult const & entityInformation)
{
    return entityInformation.ServiceName.ToString();
}
