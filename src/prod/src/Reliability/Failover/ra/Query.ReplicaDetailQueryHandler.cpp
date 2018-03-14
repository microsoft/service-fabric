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

ReplicaDetailQueryHandler::ReplicaDetailQueryHandler(
    ReconfigurationAgent & ra,
    QueryUtility queryUtility,
    DeployedServiceReplicaUtility deployedServiceReplicaUtility)
    : ra_(ra)
    , queryUtility_(queryUtility)
    , deployedServiceReplicaUtility_(deployedServiceReplicaUtility)
{
}

ReplicaDetailQueryHandler::ReplicaDetailQueryHandler(const ReplicaDetailQueryHandler & handler)
    : ra_(handler.ra_)
    , queryUtility_(handler.queryUtility_)
    , deployedServiceReplicaUtility_(handler.deployedServiceReplicaUtility_)
{
}

void ReplicaDetailQueryHandler::ProcessQuery(
    RequestReceiverContextUPtr && context,
    QueryArgumentMap const & queryArgs,
    wstring const & activityId)
{
    wstring serviceManifestName = L"";
    FailoverUnitId failoverUnitId;
    int64 replicaId = 0;
    auto parameterValidationError = ValidateAndGetParameters(queryArgs, serviceManifestName, failoverUnitId, replicaId);
    if (!parameterValidationError.IsSuccess())
    {
        queryUtility_.SendErrorResponseForQuery(ra_, activityId, parameterValidationError, *context);
        return;
    }

    auto entityEntries = queryUtility_.GetEntityEntriesByFailoverUnitId(ra_, failoverUnitId);
    if (entityEntries.empty())
    {
        queryUtility_.SendErrorResponseForQuery(ra_, activityId, ErrorCodeValue::InvalidArgument, *context);
        return;
    }

    wstring runtimeId = L"";
    FailoverUnitDescription failoverUnitDescription;
    ReplicaDescription replicaDescription;
    wstring hostId = L"";
    DeployedServiceReplicaQueryResult replicaQueryResult;
    auto replicaQueryResultError = deployedServiceReplicaUtility_.GetReplicaQueryResult(
        entityEntries.front(),
        serviceManifestName,
        replicaId,
        runtimeId,
        failoverUnitDescription,
        replicaDescription,
        hostId,
        replicaQueryResult);
    if (!replicaQueryResultError.IsSuccess())
    {
        queryUtility_.SendErrorResponseForQuery(ra_, activityId, replicaQueryResultError, *context);
        return;
    }

    shared_ptr<Federation::RequestReceiverContext> contextSPtr = move(context);
    function<void(AsyncOperationSPtr const &)> callback = [this, activityId, contextSPtr, replicaQueryResult](Common::AsyncOperationSPtr const & inner) mutable
    {
        OnCompleteSendResponse(activityId, *contextSPtr, inner, replicaQueryResult);
    };

    deployedServiceReplicaUtility_.BeginGetReplicaDetailQuery(
        ra_,
        activityId,
        entityEntries.front(),
        runtimeId,
        failoverUnitDescription,
        replicaDescription,
        hostId,
        callback,
        ra_.Root.CreateAsyncOperationRoot());
}

ErrorCode ReplicaDetailQueryHandler::ValidateAndGetParameters(
    QueryArgumentMap const & queryArgs,
    __out wstring & serviceManifestName,
    __out FailoverUnitId & failoverUnitId,
    __out int64 & replicaId)
{
    queryArgs.TryGetValue(QueryResourceProperties::ServiceType::ServiceManifestName, serviceManifestName);

    Guid partitionId;
    auto isPartitionIdProvided = queryArgs.TryGetPartitionId(partitionId);

    if (!isPartitionIdProvided || partitionId == Guid::Empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    failoverUnitId = FailoverUnitId(partitionId);

    queryArgs.TryGetReplicaId(replicaId);

    return ErrorCodeValue::Success;
}

void ReplicaDetailQueryHandler::OnCompleteSendResponse(
    wstring const & activityId,
    RequestReceiverContext & context,
    AsyncOperationSPtr const& operation,
    DeployedServiceReplicaQueryResult & replicaQueryResult)
{
    if (!operation->Error.IsSuccess())
    {
        queryUtility_.SendErrorResponseForQuery(ra_, activityId, operation->Error, context);
        return;
    }

    DeployedServiceReplicaDetailQueryResult deployedServiceReplicaDetailQueryResult;
    auto error = deployedServiceReplicaUtility_.EndGetReplicaDetailQuery(operation, deployedServiceReplicaDetailQueryResult);

    if (!error.IsSuccess())
    {
        queryUtility_.SendErrorResponseForQuery(ra_, activityId, error, context);
        return;
    }

    auto replicaQueryResultSPtr = std::make_shared<ServiceModel::DeployedServiceReplicaQueryResult>(replicaQueryResult);
    deployedServiceReplicaDetailQueryResult.SetDeployedServiceReplicaQueryResult(move(replicaQueryResultSPtr));

    queryUtility_.SendResponseForGetDeployedServiceReplicaDetailQuery(ra_, deployedServiceReplicaDetailQueryResult, context);
}
