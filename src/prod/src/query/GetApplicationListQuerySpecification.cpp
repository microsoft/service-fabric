// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

GetApplicationListQuerySpecification::GetApplicationListQuerySpecification()
    : AggregateHealthParallelQuerySpecificationBase<ApplicationQueryResult, wstring>(
        Query::QueryNames::GetApplicationList,
        Query::QueryAddresses::GetCM(),
        QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
        QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, false),
        QueryArgument(Query::QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false))
{
    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetApplicationList,
            QueryAddresses::GetCM(),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, false),
            QueryArgument(Query::QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetAggregatedApplicationHealthList,
            Query::QueryAddresses::GetHMViaCM(),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false),
            QueryArgument(Query::QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
}

bool GetApplicationListQuerySpecification::IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification)
{
    return querySpecification->QueryName == QueryNames::GetApplicationList;
}

Common::ErrorCode GetApplicationListQuerySpecification::AddEntityKeyFromHealthResult(
    __in ServiceModel::QueryResult & queryResult,
    __inout std::map<wstring, FABRIC_HEALTH_STATE> & healthStateMap)
{
    vector<ApplicationAggregatedHealthState> healthQueryResult;
    auto error = queryResult.MoveList(healthQueryResult);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itHealthInfo = healthQueryResult.begin(); itHealthInfo != healthQueryResult.end(); ++itHealthInfo)
    {
        healthStateMap.insert(std::make_pair(move(itHealthInfo->ApplicationName), itHealthInfo->AggregatedHealthState));
    }

    return error;
}

wstring GetApplicationListQuerySpecification::GetEntityKeyFromEntityResult(ServiceModel::ApplicationQueryResult const & entityInformation)
{
    return entityInformation.ApplicationName.ToString();
}
