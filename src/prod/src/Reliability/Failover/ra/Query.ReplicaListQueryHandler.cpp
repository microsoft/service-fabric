// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ::Query;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponent::Query;
using namespace Infrastructure;
using namespace ServiceModel;

const wstring MissingParameterMessage = L"Query parameter {0} not found.";
const wstring InvalidParameterValueMessage = L"Query parameter {0} has invalid value {1}.";

ReplicaListQueryHandler::ReplicaListQueryHandler(
    ReconfigurationAgent & ra,
    QueryUtility queryUtility,
    DeployedServiceReplicaUtility deployedServiceReplicaUtility)
    : ra_(ra)
    , queryUtility_(queryUtility)
    , deployedServiceReplicaUtility_(deployedServiceReplicaUtility)
{
}

ReplicaListQueryHandler::ReplicaListQueryHandler(const ReplicaListQueryHandler & handler)
    : ra_(handler.ra_)
    , queryUtility_(handler.queryUtility_)
    , deployedServiceReplicaUtility_(handler.deployedServiceReplicaUtility_)
{
}

void ReplicaListQueryHandler::ProcessQuery(
    RequestReceiverContextUPtr && context,
    QueryArgumentMap const & queryArgs,
    wstring const & activityId)
{
    wstring applicationName = L"";
    wstring serviceManifestName = L"";
    FailoverUnitId failoverUnitId;
    auto isFailoverUnitProvided = false;
    auto parameterValidationError = ValidateAndGetParameters(
        queryArgs,
        applicationName,
        serviceManifestName,
        failoverUnitId,
        isFailoverUnitProvided);
    
    if(!parameterValidationError.IsSuccess())
    {
        queryUtility_.SendErrorResponseForQuery(ra_, activityId, parameterValidationError, *context);
        return;
    }

    vector<EntityEntryBaseSPtr> entityEntries;
    if(isFailoverUnitProvided)
    {
        entityEntries = queryUtility_.GetEntityEntriesByFailoverUnitId(ra_, failoverUnitId);
    }
    else
    {
        entityEntries = queryUtility_.GetAllEntityEntries(ra_);
    }

    vector<DeployedServiceReplicaQueryResult> results;
    for (auto entityEntry : entityEntries)
    {
        DeployedServiceReplicaQueryResult replicaQueryResult;
        auto error = deployedServiceReplicaUtility_.GetReplicaQueryResult(entityEntry, applicationName, serviceManifestName, replicaQueryResult);
        if (error.IsSuccess())
        {
            results.push_back(move(replicaQueryResult));
        }
    }

    queryUtility_.SendResponseForGetServiceReplicaListDeployedOnNodeQuery(ra_, activityId, move(results), *context);
}

ErrorCode ReplicaListQueryHandler::ValidateAndGetParameters(
    QueryArgumentMap const & queryArgs,
    __out wstring & applicationName,
    __out wstring & serviceManifestName,
    __out FailoverUnitId & failoverUnitId,
    __out bool & isFailoverUnitProvided)
{
    queryArgs.TryGetValue(QueryResourceProperties::ServiceType::ServiceManifestName, serviceManifestName);

    auto appNameFound = queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationName);
    if (!appNameFound)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(MissingParameterMessage, L"applicationName"));
    }

    Guid partitionId;
    isFailoverUnitProvided = queryArgs.TryGetPartitionId(partitionId);
    if (isFailoverUnitProvided)
    {
        if(partitionId == Guid::Empty())
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(InvalidParameterValueMessage, L"partitionId", partitionId));
        }

        failoverUnitId = FailoverUnitId(partitionId);
    }

    return ErrorCodeValue::Success;
}
