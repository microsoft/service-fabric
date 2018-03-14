// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

GetApplicationPagedListDeployedOnNodeQuerySpecification::GetApplicationPagedListDeployedOnNodeQuerySpecification(bool includeHealthState)
    : AggregateHealthParallelQuerySpecificationBase < DeployedApplicationQueryResult, wstring >(
        Query::QueryNames::GetApplicationPagedListDeployedOnNode,
        Query::QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Node::Name, true),
        QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::IncludeHealthState, false))
    , includeHealthState_(includeHealthState)
{
    // This sets the member in the parent class QuerySpecification. You need to set this because this is how QuerySpecificationStore
    // checks for uniqueness of the specificationID.
    // The check doesn't call GetSpecificationId because it's a static method.
    querySpecificationId_ = GetInternalSpecificationId(includeHealthState);

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetApplicationPagedListDeployedOnNode,
            QueryAddresses::GetHosting(),
            QueryArgument(Query::QueryResourceProperties::Node::Name, true),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));

    if (includeHealthState)
    {
        // Note: the below is using GetHM instead of GetHMViaCM because they come to the same place, but getHM requires less steps
        // as it doesn't go through CM
        ParallelQuerySpecifications.push_back(
            make_shared<QuerySpecification>(
                QueryNames::GetAggregatedDeployedApplicationsOnNodeHealthPagedList, // This is paged
                Query::QueryAddresses::GetHM(),
                QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
    }
}

vector<QuerySpecificationSPtr> GetApplicationPagedListDeployedOnNodeQuerySpecification::CreateSpecifications()
{
    vector<QuerySpecificationSPtr> resultSPtr;

    // The true and false is to set includeHealthState
    resultSPtr.push_back(make_shared<GetApplicationPagedListDeployedOnNodeQuerySpecification>(false));
    resultSPtr.push_back(make_shared<GetApplicationPagedListDeployedOnNodeQuerySpecification>(true));

    return move(resultSPtr);
}

bool GetApplicationPagedListDeployedOnNodeQuerySpecification::IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification)
{
    return querySpecification->QueryName == QueryNames::GetApplicationPagedListDeployedOnNode;
}

Common::ErrorCode GetApplicationPagedListDeployedOnNodeQuerySpecification::AddEntityKeyFromHealthResult(
    __in ServiceModel::QueryResult & queryResult,
    __inout std::map<wstring, FABRIC_HEALTH_STATE> & healthStateMap)
{
    vector<DeployedApplicationAggregatedHealthState> healthQueryResult;
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

wstring GetApplicationPagedListDeployedOnNodeQuerySpecification::GetEntityKeyFromEntityResult(ServiceModel::DeployedApplicationQueryResult const & entityInformation)
{
    return entityInformation.ApplicationName;
}

wstring GetApplicationPagedListDeployedOnNodeQuerySpecification::GetSpecificationId(ServiceModel::QueryArgumentMap const& queryArgs)
{
    bool includeHealthState = false;
    queryArgs.TryGetBool(Query::QueryResourceProperties::QueryMetadata::IncludeHealthState, includeHealthState);

    return move(GetInternalSpecificationId(includeHealthState));
}

wstring GetApplicationPagedListDeployedOnNodeQuerySpecification::GetInternalSpecificationId(bool includeHealthState)
{
    if (includeHealthState)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetApplicationPagedListDeployedOnNode), L"/IncludeHealth");
    }

    return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetApplicationPagedListDeployedOnNode), L"/ExcludeHealth");
}
