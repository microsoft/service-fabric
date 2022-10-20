// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Query;
using namespace ServiceModel;
using namespace Transport;

class FMQueryHelper::MovePrimaryAsyncOperation : public AsyncOperation
{
public:
    MovePrimaryAsyncOperation(
                FMQueryHelper & owner,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent
                ) : AsyncOperation(callback, parent)
                , owner_(owner)
                , queryArgs_(queryArgs)
                , activityId_(activityId)
                , timeoutHelper_(timeout)
                , newPrimary_()
                , desiredPartitionId_()
                , newPrimaryNodeId_()
                , force_(false)
                , chooseRandom_(false)
    {
    }

    static ErrorCode MovePrimaryAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<MovePrimaryAsyncOperation>(operation);
        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        std::wstring force;
        force_ = false;


        queryArgs_.TryGetValue(QueryResourceProperties::Node::Name, newPrimary_);
        queryArgs_.TryGetValue(QueryResourceProperties::Partition::PartitionId, desiredPartitionId_);
        queryArgs_.TryGetValue(QueryResourceProperties::QueryMetadata::ForceMove, force); StringUtility::ToLower(force);

        force_ = (force == L"true") ? true : false;
        chooseRandom_ = newPrimary_.empty();

        Federation::NodeIdGenerator::GenerateFromString(newPrimary_, newPrimaryNodeId_);

        auto error = GetParamsAndStartMovePrimary();
        if (error.IsError(ErrorCodeValue::AlreadyPrimaryReplica)
            || error.IsError(ErrorCodeValue::InvalidOperation)
            || error.IsError(ErrorCodeValue::InvalidPartitionOperation)
            || error.IsError(ErrorCodeValue::Timeout)
            || error.IsError(ErrorCodeValue::NodeNotFound))
        {
            TryComplete(thisSPtr, error);
            return;
        }
        ValidateMovePrimaryInternalOperation(thisSPtr, !error.IsSuccess());
    }


private:

    Common::ErrorCode GetParamsAndStartMovePrimary(bool isRetry = false)
    {
        wstring serviceName;
        Federation::NodeId currentPrimaryNodeId;
        FailoverUnitId failoverUnitId = FailoverUnitId(Common::Guid(desiredPartitionId_));
        // scope is to release lock pointer once service name is recovered.
        ErrorCode error = owner_.GetServiceNameAndCurrentPrimaryNode(failoverUnitId, activityId_, serviceName, currentPrimaryNodeId);
        if (!error.IsSuccess())
        {
            return error;
        }

        owner_.fm_.WriteInfo(Constants::QuerySource,
            "{0}: Node Name {1}, Service Name {2}, Partition Id {3}, MovePrimaryAction",
            activityId_,
            newPrimary_,
            serviceName,
            failoverUnitId.Guid);

        return TriggerMovePrimaryOperation(serviceName, failoverUnitId.Guid, currentPrimaryNodeId, newPrimaryNodeId_, isRetry);
    }

    void ValidateMovePrimaryInternalOperation(AsyncOperationSPtr const & thisSPtr, bool needsFullRestart, ErrorCode error = ErrorCodeValue::Timeout)
    {
        bool success = VerifyMovePrimaryReplicaMovement(Common::Guid(desiredPartitionId_), newPrimaryNodeId_, activityId_);
        if (!timeoutHelper_.IsExpired && !success)
        {
            error = GetParamsAndStartMovePrimary(!needsFullRestart);
            owner_.fm_.WriteInfo(Constants::QuerySource,"MovePrimaryError: {0}", error);

            if (error.IsSuccess()
                || error.IsError(ErrorCodeValue::PLBNotReady)
                || error.IsError(ErrorCodeValue::ServiceNotFound)
                || error.IsError(ErrorCodeValue::REReplicaDoesNotExist)
                || error.IsError(ErrorCodeValue::FMFailoverUnitNotFound)
                || error.IsError(ErrorCodeValue::FMNotReadyForUse))
            {
                Common::Threadpool::Post([this, thisSPtr, error]
                {
                    ValidateMovePrimaryInternalOperation(thisSPtr, !error.IsSuccess(), error);
                }, TimeSpan::FromSeconds(delay_));
                return;
            }
        }

        if (error.IsError(ErrorCodeValue::AlreadyPrimaryReplica)
            || (success && (error.IsError(ErrorCodeValue::Timeout) || error.IsError(ErrorCodeValue::PLBNotReady))))
        {
            error.Reset(ErrorCodeValue::Success);
        }

        TryComplete(thisSPtr, error);

    }

    Common::ErrorCode TriggerMovePrimaryOperation(std::wstring const & serviceName, Guid const& failoverUnitId, Federation::NodeId const & currentPrimary, __inout Federation::NodeId & newPrimary, bool isRetry)
    {
        if (!timeoutHelper_.IsExpired)
        {
            if (!chooseRandom_ || isRetry)
            {
                auto nodeError = owner_.VerifyNodeExistsAndIsUp(newPrimary);
                if (!nodeError.IsError(ErrorCodeValue::Success))
                {
                    return nodeError;
                }
            }

            return owner_.fm_.PLB.TriggerSwapPrimary(serviceName, failoverUnitId, currentPrimary, newPrimary, force_, chooseRandom_ && !isRetry);
        }
        else
        {
            return ErrorCodeValue::Timeout;
        }
    }

    bool VerifyMovePrimaryReplicaMovement(Guid const & partitionId,
        Federation::NodeId const & nodeId,
        ActivityId const & activityId)
    {
            FailoverUnitId failoverUnitId = FailoverUnitId(partitionId);
            LockedFailoverUnitPtr failoverUnitPtr;
            if (!owner_.TryGetLockedFailoverUnit(failoverUnitId, activityId, failoverUnitPtr))
            {
                return false;
            }

            auto replica = failoverUnitPtr->GetReplica(nodeId);
            if (replica)
            {
                if (replica->IsCurrentConfigurationPrimary || replica->IsToBePromoted)
                {
                    owner_.fm_.WriteInfo(Constants::QuerySource,
                        "{0}: PartitionId: {1} Replica Role change succeeded.",
                        activityId,
                        partitionId);
                    return true;
                }
                else{
                    owner_.fm_.WriteInfo(Constants::QuerySource,
                        "{0}: PartitionId: {1} Replica Role {2}, IsCurrentConfigurationPrimary {3}, IsPrimaryToBePlaced {4}, IsPrimaryToBeSwappedout {5} ",
                        activityId,
                        partitionId,
                        replica->CurrentConfigurationRole,
                        replica->IsCurrentConfigurationPrimary,
                        replica->IsPrimaryToBePlaced,
                        replica->IsPrimaryToBeSwappedOut);
                }
            }
            else{
                owner_.fm_.WriteInfo(Constants::QuerySource,
                    "{0}: PartitionId: {1} node id {2} replica not found.",
                    activityId,
                    partitionId,
                    nodeId);
            }

        return false;
    }

    FMQueryHelper & owner_;
    ServiceModel::QueryArgumentMap queryArgs_;
    Common::ActivityId activityId_;
    Common::TimeoutHelper timeoutHelper_;
    std::wstring newPrimary_;
    std::wstring desiredPartitionId_;
    Federation::NodeId newPrimaryNodeId_;
    bool force_;
    bool chooseRandom_;
    Transport::MessageUPtr reply_;
    double const delay_ = 2.5;
};

class FMQueryHelper::MoveSecondaryAsyncOperation : public AsyncOperation
{
public:
    MoveSecondaryAsyncOperation(
        FMQueryHelper & owner,
        ServiceModel::QueryArgumentMap const & queryArgs,
        Common::ActivityId const & activityId,
        Common::TimeSpan timeout,
        Common::AsyncCallback callback,
        Common::AsyncOperationSPtr const & parent
        ) : AsyncOperation(callback, parent)
        , owner_(owner)
        , queryArgs_(queryArgs)
        , activityId_(activityId)
        , timeoutHelper_(timeout)
        , currentSecondary_()
        , newSecondary_()
        , desiredPartitionId_()
        , currentSecondaryNodeId_()
        , newSecondaryNodeId_()
        , force_(false)
        , chooseRandom_(false)
    {
    }

    static ErrorCode MoveSecondaryAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<MoveSecondaryAsyncOperation>(operation);
        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        std::wstring force;
        force_ = false;
        chooseRandom_ = false;

        queryArgs_.TryGetValue(QueryResourceProperties::Node::CurrentNodeName, currentSecondary_);
        queryArgs_.TryGetValue(QueryResourceProperties::Node::NewNodeName, newSecondary_);

        chooseRandom_ = newSecondary_.empty();

        Federation::NodeIdGenerator::GenerateFromString(currentSecondary_, currentSecondaryNodeId_);
        Federation::NodeIdGenerator::GenerateFromString(newSecondary_, newSecondaryNodeId_);
        queryArgs_.TryGetValue(QueryResourceProperties::Partition::PartitionId, desiredPartitionId_);

        queryArgs_.TryGetValue(QueryResourceProperties::QueryMetadata::ForceMove, force); StringUtility::ToLower(force);

        force_ = (force == L"true") ? true : false;

        auto error = GetParamsAndStartMoveSecondary();

        if (error.IsError(ErrorCodeValue::AlreadySecondaryReplica) ||
            error.IsError(ErrorCodeValue::InvalidOperation) ||
            error.IsError(ErrorCodeValue::REReplicaDoesNotExist) ||
            error.IsError(ErrorCodeValue::AlreadyPrimaryReplica) ||
            error.IsError(ErrorCodeValue::InvalidReplicaStateForReplicaOperation) ||
            error.IsError(ErrorCodeValue::InvalidPartitionOperation) ||
            error.IsError(ErrorCodeValue::Timeout) ||
            error.IsError(ErrorCodeValue::NodeNotFound))
        {
            TryComplete(thisSPtr, error);
            return;
        }
        ValidateMoveSecondaryInternalOperation(thisSPtr, !error.IsSuccess());
    }

private:

    Common::ErrorCode GetParamsAndStartMoveSecondary(bool isRetry = false)
    {
        wstring serviceName;
        Federation::NodeId currentPrimaryNodeId;
        FailoverUnitId failoverUnitId = FailoverUnitId(Common::Guid(desiredPartitionId_));
        // scope to get service name and release the failoverunitLockPtr
        ErrorCode error = owner_.GetServiceNameAndCurrentPrimaryNode(failoverUnitId, activityId_, serviceName, currentPrimaryNodeId);
        if (!error.IsSuccess())
        {
            return error;
        }

        owner_.fm_.WriteInfo(Constants::QuerySource,
            "{0}: current Node Name {1} , new Node name {2} , Service Name {3}, Partition Id {4}, MoveSecondaryAction",
            activityId_,
            currentSecondary_,
            newSecondary_,
            serviceName,
            failoverUnitId.Guid);

        return TriggerMoveSecondaryOperation(serviceName, failoverUnitId.Guid, currentSecondaryNodeId_, newSecondaryNodeId_, isRetry);
    }

    Common::ErrorCode TriggerMoveSecondaryOperation(std::wstring const & serviceName,
        Common::Guid const & failoverUnitId,
        Federation::NodeId const & currentSecondaryNodeId,
        __inout Federation::NodeId & newSecondaryNodeId,
        bool isRetry)
    {
        if (!timeoutHelper_.IsExpired)
        {
            if (!chooseRandom_ || isRetry)
            {
                auto nodeError = owner_.VerifyNodeExistsAndIsUp(newSecondaryNodeId);
                if (!nodeError.IsError(ErrorCodeValue::Success))
                {
                    return nodeError;
                }
            }

            return owner_.fm_.PLB.TriggerMoveSecondary(serviceName, failoverUnitId, currentSecondaryNodeId, newSecondaryNodeId, force_, chooseRandom_ && !isRetry);
        }
        else
        {
            return ErrorCodeValue::Timeout;
        }
    }

    void ValidateMoveSecondaryInternalOperation(AsyncOperationSPtr const & thisSPtr, bool needsFullRestart, ErrorCode error = ErrorCodeValue::Timeout)
    {
        bool success = VerifyMoveSecondaryReplicaMovement(
            Common::Guid(desiredPartitionId_),
            currentSecondaryNodeId_,
            newSecondaryNodeId_,
            activityId_);
        if (!timeoutHelper_.IsExpired && !success)
        {
            error = GetParamsAndStartMoveSecondary(!needsFullRestart);
            owner_.fm_.WriteInfo(Constants::QuerySource,"MoveSecondaryError: {0}", error);

            if (error.IsSuccess()
                || error.IsError(ErrorCodeValue::PLBNotReady)
                || error.IsError(ErrorCodeValue::ServiceNotFound)
                || error.IsError(ErrorCodeValue::REReplicaDoesNotExist)
                || error.IsError(ErrorCodeValue::FMFailoverUnitNotFound)
                || error.IsError(ErrorCodeValue::FMNotReadyForUse))
            {
                Common::Threadpool::Post([this, thisSPtr, error]
                {
                    ValidateMoveSecondaryInternalOperation(thisSPtr, !error.IsSuccess(), error);
                },
                    TimeSpan::FromSeconds(delay_));

                return;
            }
        }

        if (error.IsError(ErrorCodeValue::AlreadySecondaryReplica)
            || (success && (error.IsError(ErrorCodeValue::Timeout) || error.IsError(ErrorCodeValue::PLBNotReady))))
        {
            error.Reset(ErrorCodeValue::Success);
        }

        TryComplete(thisSPtr, error);
    }

    bool VerifyMoveSecondaryReplicaMovement(
        Guid const & partitionId,
        Federation::NodeId const & currentNodeId,
        Federation::NodeId const & newNodeId,
        ActivityId const & activityId)
    {
            FailoverUnitId failoverUnitId = FailoverUnitId(partitionId);
            LockedFailoverUnitPtr failoverUnitPtr;
            if (!owner_.TryGetLockedFailoverUnit(failoverUnitId, activityId, failoverUnitPtr))
            {
                return false;
            }

            auto newReplica = failoverUnitPtr->GetReplica(newNodeId);
            if (newReplica)
            {
                if ((newReplica->CurrentConfigurationRole == ReplicaRole::Secondary) || newReplica->IsCurrentConfigurationSecondary)
                {
                    owner_.fm_.WriteInfo(Constants::QuerySource,
                        "{0}: PartitionId: {1} Replica Role change succeeded.",
                        activityId,
                        partitionId);
                    return true;
                }
                else
                {
                    owner_.fm_.WriteInfo(Constants::QuerySource,
                        "{0}: Partition {1} Replica role {2}",
                        activityId,
                        partitionId,
                        newReplica->CurrentConfigurationRole);
                }

            }
            else
            {
                owner_.fm_.WriteInfo(Constants::QuerySource,
                    "{0}: PartitionId: {1} node id {2} replica not found.",
                    activityId,
                    partitionId,
                    newNodeId);
            }

            auto prevReplica = failoverUnitPtr->GetReplica(currentNodeId);
            if (prevReplica)
            {
                if (newReplica != nullptr &&
                    (prevReplica->IsDropped || prevReplica->IsPendingRemove) &&
                    ((newReplica && newReplica->IsInBuild) || newReplica->IsCreating))
                {
                    owner_.fm_.WriteInfo(Constants::QuerySource,
                        "{0}: PartitionId: {1} Replica Role change succeeded.",
                        activityId,
                        partitionId);
                    return true;
                }
                else
                {
                    owner_.fm_.WriteInfo(Constants::QuerySource,
                        "{0}: Partition {1} Replica role {2}",
                        activityId,
                        partitionId,
                        prevReplica->CurrentConfigurationRole);
                }
            }
            else
            {
                owner_.fm_.WriteInfo(Constants::QuerySource,
                    "{0}: PartitionId: {1} node id {2} replica is dropped.",
                    activityId,
                    partitionId,
                    currentNodeId);
            }
        return false;
    }

    FMQueryHelper & owner_;
    ServiceModel::QueryArgumentMap queryArgs_;
    Common::ActivityId activityId_;
    Common::TimeoutHelper timeoutHelper_;
    std::wstring currentSecondary_;
    std::wstring newSecondary_;
    std::wstring desiredPartitionId_;
    Federation::NodeId currentSecondaryNodeId_;
    Federation::NodeId newSecondaryNodeId_;
    bool force_;
    bool chooseRandom_;
    Transport::MessageUPtr reply_;
    double const delay_ = 2.5;
};

class FMQueryHelper::ProcessQueryAsyncOperation : public TimedAsyncOperation
{
public:
    ProcessQueryAsyncOperation(
        __in FMQueryHelper &owner,
        Query::QueryNames::Enum queryName,
        ServiceModel::QueryArgumentMap const & queryArgs,
        Common::ActivityId const & activityId,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , queryName_(queryName)
        , queryArgs_(queryArgs)
        , activityId_(activityId)
    {
    }

    static Common::ErrorCode End(
        Common::AsyncOperationSPtr const & asyncOperation,
        __out Transport::MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<ProcessQueryAsyncOperation>(asyncOperation);
        if (casted->reply_)
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

protected:
    // This method is chained to the FM message processor.
    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        QueryResult queryResult;
        ErrorCode errorCode = ErrorCodeValue::InvalidConfiguration;
        bool isAsyncOperation = false;
        switch (queryName_)
        {
        case QueryNames::GetNodeList:
            errorCode = owner_.GetNodesList(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetSystemServicesList:
            errorCode = owner_.GetSystemServicesList(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetApplicationServiceList:
            errorCode = owner_.GetServicesList(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetApplicationServiceGroupMemberList:
            errorCode = owner_.GetServiceGroupsMembersList(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetServicePartitionList:
            errorCode = owner_.GetServicePartitionList(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetServicePartitionReplicaList:
            errorCode = owner_.GetServicePartitionReplicaList(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetClusterLoadInformation:
            errorCode = owner_.GetClusterLoadInformation(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetPartitionLoadInformation:
            errorCode = owner_.GetPartitionLoadInformation(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetNodeLoadInformation:
            errorCode = owner_.GetNodeLoadInformation(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetReplicaLoadInformation:
            errorCode = owner_.GetReplicaLoadInformation(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::MovePrimary:
            HandleMovePrimaryAsync(thisSPtr);
            isAsyncOperation = true;
            break;
        case QueryNames::MoveSecondary:
            HandleMoveSecondaryAsync(thisSPtr);
            isAsyncOperation = true;
            break;
        case QueryNames::GetUnplacedReplicaInformation:
            errorCode = owner_.GetUnplacedReplicaInformation(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetApplicationLoadInformation:
            errorCode = owner_.GetApplicationLoadInformation(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetServiceName:
            errorCode = owner_.GetServiceName(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetReplicaListByServiceNames:
            errorCode = owner_.GetReplicaListByServiceNames(queryArgs_, queryResult, activityId_);
            break;
        case QueryNames::GetNetworkList:
        case QueryNames::GetNetworkApplicationList:
        case QueryNames::GetNetworkNodeList:
        case QueryNames::GetApplicationNetworkList:
        case QueryNames::GetDeployedNetworkList:
            errorCode = owner_.ProcessNIMQuery(queryName_, queryArgs_, queryResult, activityId_);
            break;
            
        default:
            errorCode = ErrorCodeValue::InvalidArgument;
        }

        if (!isAsyncOperation)
        {
            reply_ = RSMessage::GetQueryReply().CreateMessage(queryResult);
            TryComplete(thisSPtr, errorCode);
        }
    }

private:

    void HandleMovePrimaryAsync(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.BeginMovePrimaryOperation(
            queryArgs_,
            activityId_,
            this->RemainingTime,
            [this, thisSPtr](AsyncOperationSPtr const & operation){ OnMovePrimaryCompleted(operation, false); },
            thisSPtr);
        OnMovePrimaryCompleted(operation, true);
    }

    void OnMovePrimaryCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EndMovePrimaryOperation(operation);
        QueryResult result;
        reply_ = RSMessage::GetQueryReply().CreateMessage(result);
        TryComplete(operation->Parent, error);
    }

    void HandleMoveSecondaryAsync(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.BeginMoveSecondaryOperation(
            queryArgs_,
            activityId_,
            this->RemainingTime,
            [this, thisSPtr](AsyncOperationSPtr const & operation){ OnMoveSecondaryCompleted(operation, false); },
            thisSPtr);
        OnMoveSecondaryCompleted(operation, true);
    }

    void OnMoveSecondaryCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EndMoveSecondaryOperation(operation);
        QueryResult result;
        reply_ = RSMessage::GetQueryReply().CreateMessage(result);
        TryComplete(operation->Parent, error);
    }

    FMQueryHelper & owner_;
    Query::QueryNames::Enum queryName_;
    ServiceModel::QueryArgumentMap queryArgs_;
    Common::ActivityId activityId_;
    Transport::MessageUPtr reply_;
};

FMQueryHelper::FMQueryHelper(__in FailoverManager & fm)
    : fm_(fm)
    , queryMessageHandler_(make_unique<QueryMessageHandler>(fm_, fm_.IsMaster ? QueryAddresses::FMMAddressSegment : QueryAddresses::FMAddressSegment))
{
}

FMQueryHelper::~FMQueryHelper(){}

ErrorCode FMQueryHelper::OnOpen()
{
    queryMessageHandler_->RegisterQueryHandler(
        [this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, Common::ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
    {
        return this->BeginProcessQuery(queryName, queryArgs, activityId, timeout, callback, parent);
    },
        [this](Common::AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        return this->EndProcessQuery(operation, reply);
    });

    return queryMessageHandler_->Open();
}

Common::AsyncOperationSPtr FMQueryHelper::BeginProcessQueryMessage(
    Transport::Message & message,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return queryMessageHandler_->BeginProcessQueryMessage(message, timeout, callback, parent);
}

Common::ErrorCode FMQueryHelper::EndProcessQueryMessage(
    Common::AsyncOperationSPtr const & asyncOperation,
    _Out_ Transport::MessageUPtr & reply)
{
    return queryMessageHandler_->EndProcessQueryMessage(asyncOperation, reply);
}

Common::AsyncOperationSPtr FMQueryHelper::BeginProcessQuery(
    Query::QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    Common::ActivityId const & activityId,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessQueryAsyncOperation>(
        *this,
        queryName,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode FMQueryHelper::EndProcessQuery(
    Common::AsyncOperationSPtr const & operation,
    __out Transport::MessageUPtr & reply)
{
    return ProcessQueryAsyncOperation::End(operation, reply);
}

ErrorCode FMQueryHelper::OnClose()
{
    return queryMessageHandler_->Close();
}

void FMQueryHelper::OnAbort()
{
    queryMessageHandler_->Abort();
}

ErrorCode FMQueryHelper::GetNodesList(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring desiredNodeName;
    bool filter = queryArgs.TryGetValue(QueryResourceProperties::Node::Name, desiredNodeName);

    vector<NodeInfoSPtr> nodes;

    wstring continuationToken;
    if (queryArgs.TryGetValue(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken))
    {
        Federation::NodeId lastNodeId;
        auto error = PagingStatus::GetContinuationTokenData<Federation::NodeId>(continuationToken, lastNodeId);
        if (!error.IsSuccess())
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: error parsing continuation token {1}: {2}",
                activityId,
                continuationToken,
                error);
            return error;
        }

        fm_.NodeCacheObj.GetNodes(lastNodeId, nodes);
    }
    else
    {
        fm_.NodeCacheObj.GetNodes(nodes);
    }

    wstring nodeStatusFilterString;
    DWORD nodeStatusFilter = (DWORD)FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT;
    if (queryArgs.TryGetValue(QueryResourceProperties::Node::NodeStatusFilter, nodeStatusFilterString))
    {
        if (!StringUtility::TryFromWString<DWORD>(nodeStatusFilterString, nodeStatusFilter))
        {
            return ErrorCodeValue::InvalidArgument;
        }
    }

    fm_.WriteInfo(Constants::QuerySource, "{0}: Cache nodes: continuation token={1}, count={2}", activityId, continuationToken, nodes.size());

    // Get max results value
    int64 maxResults;
    auto error = QueryPagingDescription::TryGetMaxResultsFromArgumentMap(queryArgs, maxResults, activityId.ToString(), L"FMQueryHelper::GetNodesList");
    if (!error.IsSuccess())
    {
        return error;
    }

    ListPager<NodeQueryResult> nodeQueryResultList;
    nodeQueryResultList.SetMaxResults(maxResults);

    for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
    {
        NodeInfoSPtr const& nodeInfo = *nodeIt;

        if (!IsMatch(nodeInfo->NodeStatus, nodeStatusFilter))
        {
            continue;
        }

        if ((!filter || (filter && nodeInfo->NodeName == desiredNodeName)))
        {
            int64 nodeUpTimeInSecs = 0;
            if (nodeInfo->IsUp && (nodeInfo->NodeUpTime != DateTime::Zero))
            {
                nodeUpTimeInSecs = (DateTime::Now() - nodeInfo->NodeUpTime).TotalSeconds();
            }

            int64 nodeDownTimeInSecs = 0;
            if (!nodeInfo->IsUp && (nodeInfo->NodeDownTime != DateTime::Zero))
            {
                nodeDownTimeInSecs = (DateTime::Now() - nodeInfo->NodeDownTime).TotalSeconds();
            }

            error = nodeQueryResultList.TryAdd(
                ServiceModel::NodeQueryResult(
                    nodeInfo->NodeName,
                    nodeInfo->IpAddressOrFQDN,
                    nodeInfo->NodeType,
                    nodeInfo->VersionInstance.Version.CodeVersion.ToString(),
                    nodeInfo->VersionInstance.Version.ConfigVersion.ToString(),
                    nodeInfo->NodeStatus,
                    nodeUpTimeInSecs,
                    nodeDownTimeInSecs,
                    nodeInfo->NodeUpTime,
                    nodeInfo->NodeDownTime,
                    fm_.Federation.IsSeedNode((*nodeIt)->Id),
                    nodeInfo->ActualUpgradeDomainId,
                    nodeInfo->FaultDomainId,
                    nodeInfo->Id,
                    nodeInfo->NodeInstance.InstanceId,
                    nodeInfo->DeactivationInfo.GetQueryResult(),
                    nodeInfo->Description.HttpGatewayPort,
                    nodeInfo->Description.ClusterConnectionPort,
                    false));
            if (!error.IsSuccess() && nodeQueryResultList.IsBenignError(error))
            {
                fm_.WriteInfo(Constants::QuerySource,
                    "{0}: reached max message size or page limit with {1} ({2}): {3}",
                    activityId,
                    nodeInfo->Id,
                    nodeInfo->NodeName,
                    error.Message);
                break;
            }
            else if (!error.IsSuccess())
            {
                return error;
            }
        }
    }

    queryResult = ServiceModel::QueryResult(move(nodeQueryResultList));
    return ErrorCode::Success();
}

ErrorCode FMQueryHelper::GetSystemServicesList(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring desiredServiceName;
    bool serviceNameFilter = queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, desiredServiceName);

    wstring desiredServiceTypeName;
    bool serviceTypeNameFilter = queryArgs.TryGetValue(QueryResourceProperties::ServiceType::ServiceTypeName, desiredServiceTypeName);

    wstring continuationToken;
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);

    // Get max results value
    int64 maxResults;
    auto error = QueryPagingDescription::TryGetMaxResultsFromArgumentMap(queryArgs, maxResults, activityId.ToString(), L"FMQueryHelper::GetSystemServicesList");
    if (!error.IsSuccess())
    {
        return(move(error));
    }

    if (serviceNameFilter && checkContinuationToken && desiredServiceName <= continuationToken)
    {
        // The desired service doesn't respect the continuation token
        return ErrorCode::Success();
    }

    vector<ServiceQueryResult> serviceQueryResultList;

    // Add query result for FMService
    if ((!serviceNameFilter || (serviceNameFilter && desiredServiceName == *ServiceModel::SystemServiceApplicationNameHelper::PublicFMServiceName))
        && (!serviceTypeNameFilter || (serviceTypeNameFilter && desiredServiceTypeName == (*ServiceModel::ServiceTypeIdentifier::FailoverManagerServiceTypeId).ServiceTypeName)))
    {
        Uri serviceName;
        Uri::TryParse(SystemServiceApplicationNameHelper::PublicFMServiceName, serviceName);

        serviceQueryResultList.push_back(ServiceQueryResult::CreateStatefulServiceQueryResult(
            serviceName,
            (*ServiceModel::ServiceTypeIdentifier::FailoverManagerServiceTypeId).ServiceTypeName,
            fm_.FabricUpgradeManager.CurrentVersionInstance.Version.CodeVersion.ToString(),
            true,
            FABRIC_QUERY_SERVICE_STATUS_ACTIVE));
    }

    // Add query result for other system services
    fm_.ServiceCacheObj.GetServiceQueryResults(
        [serviceNameFilter, serviceTypeNameFilter, &desiredServiceName, &desiredServiceTypeName](ServiceInfoSPtr const & serviceInfo)
        {
            if (serviceNameFilter)
            {
                return serviceInfo->ServiceType->Type.IsSystemServiceType() &&
                    serviceInfo->Name == SystemServiceApplicationNameHelper::GetInternalServiceName(desiredServiceName);
            }
            else if (serviceTypeNameFilter)
            {
                return serviceInfo->ServiceType->Type.IsSystemServiceType() && serviceInfo->ServiceDescription.Type.ServiceTypeName == desiredServiceTypeName;
            }

            return serviceInfo->ServiceType->Type.IsSystemServiceType();
        },
        serviceQueryResultList);

    map<wstring, ServiceQueryResult> sortedServices;
    for (auto it = serviceQueryResultList.begin(); it != serviceQueryResultList.end(); it++)
    {
        wstring publicServiceName = it->ServiceName.ToString();
        if (checkContinuationToken && (publicServiceName <= continuationToken))
        {
            // Doesn't respect the continuation token, continue
            continue;
        }

        sortedServices.insert(make_pair(move(publicServiceName), move(*it)));
    }

    // Add the sorted services to result
    ListPager<ServiceQueryResult> pager;
    pager.SetMaxResults(maxResults);

    for (auto it = sortedServices.begin(); it != sortedServices.end(); ++it)
    {
        error = pager.TryAdd(move(it->second));
        if (!error.IsSuccess() && pager.IsBenignError(error))
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: reached max message size or page limit with {1}: {2}",
                activityId,
                it->first,
                error.Message);
            break;
        }
    }

    queryResult = ServiceModel::QueryResult(move(pager));
    return ErrorCode::Success();
}

ErrorCode FMQueryHelper::GetServicesList(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring desiredAppName;
    bool appFilter = queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, desiredAppName);

    wstring desiredServiceName;
    bool serviceFilter = queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, desiredServiceName);

    wstring serviceTypeNameFilter;
    bool serviceTypeFilter = queryArgs.TryGetValue(QueryResourceProperties::ServiceType::ServiceTypeName, serviceTypeNameFilter);

    wstring continuationToken;
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);

    // Get max results value
    int64 maxResults;
    auto error = QueryPagingDescription::TryGetMaxResultsFromArgumentMap(queryArgs, maxResults, activityId.ToString(), L"FMQueryHelper::GetServicesList");
    if (!error.IsSuccess())
    {
        return(move(error));
    }

    if (serviceFilter && checkContinuationToken)
    {
        if (desiredServiceName <= continuationToken)
        {
            // The desired service doesn't respect the continuation token
            return ErrorCode::Success();
        }
    }

    vector<ServiceQueryResult> serviceQueryResultList;
    fm_.ServiceCacheObj.GetServiceQueryResults(
        [appFilter, serviceFilter, serviceTypeFilter, &desiredAppName, &desiredServiceName, &serviceTypeNameFilter, &continuationToken](ServiceInfoSPtr const & serviceInfo)
        {
            if (!continuationToken.empty() && serviceInfo->Name <= continuationToken)
            {
                return false;
            }

            if (appFilter)
            {
                if (serviceFilter)
                {
                    return serviceInfo->ServiceDescription.ApplicationName == desiredAppName && serviceInfo->Name == desiredServiceName;
                }
                else if (serviceTypeFilter)
                {
                    return serviceInfo->ServiceDescription.ApplicationName == desiredAppName && serviceInfo->ServiceDescription.Type.ServiceTypeName == serviceTypeNameFilter;
                }

                return serviceInfo->ServiceDescription.ApplicationName == desiredAppName;
            }
            else
            {
                if (serviceFilter)
                {
                    return serviceInfo->ServiceType->Type.IsAdhocServiceType() && serviceInfo->Name == desiredServiceName;
                }
                else if (serviceTypeFilter)
                {
                    return serviceInfo->ServiceType->Type.IsAdhocServiceType() && serviceInfo->ServiceDescription.Type.ServiceTypeName == serviceTypeNameFilter;
                }

                return serviceInfo->ServiceType->Type.IsAdhocServiceType();
            }
        },
        serviceQueryResultList);

    // Sort the services by the public name; should be NOP, since then internal and external name matches.
    sort(serviceQueryResultList.begin(), serviceQueryResultList.end(), [](ServiceQueryResult const & l, ServiceQueryResult const & r)
    {
        return l.ServiceName.ToString() < r.ServiceName.ToString();
    });

    // Add the services to the result.
    ListPager<ServiceQueryResult> pager;
    pager.SetMaxResults(maxResults);

    // Add the services to the result.
    for (auto it = serviceQueryResultList.begin(); it != serviceQueryResultList.end(); ++it)
    {
        error = pager.TryAdd(move(*it));
        if (!error.IsSuccess() && pager.IsBenignError(error))
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: reached max message size or page limit with {1}: {2}",
                activityId,
                it->ServiceName,
                error.Message);
            break;
        }
    }

    queryResult = ServiceModel::QueryResult(move(pager));
    return ErrorCode::Success();
}

ErrorCode FMQueryHelper::GetServiceGroupsMembersList(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    UNREFERENCED_PARAMETER(activityId);

    wstring desiredAppName;
    bool appFilter = queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, desiredAppName);

    wstring desiredServiceName;
    bool serviceFilter = queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, desiredServiceName);

    function<bool(ServiceInfoSPtr const &)> filterFunction =
        [appFilter, serviceFilter, &desiredAppName, &desiredServiceName](ServiceInfoSPtr const & serviceInfo)
        {
            if (appFilter)
            {
                if (serviceFilter)
                {
                    return serviceInfo->ServiceDescription.IsServiceGroup == true &&
                        serviceInfo->ServiceDescription.ApplicationName == desiredAppName &&
                        serviceInfo->Name == desiredServiceName;
                }

                return serviceInfo->ServiceDescription.IsServiceGroup == true  &&
                    serviceInfo->ServiceDescription.ApplicationName == desiredAppName;
            }
            else
            {
                if (serviceFilter)
                {
                    return serviceInfo->ServiceDescription.IsServiceGroup == true &&
                        serviceInfo->Name == desiredServiceName;
                }

                return serviceInfo->ServiceDescription.IsServiceGroup == true;
            }
        };

    vector<ServiceGroupMemberQueryResult> serviceGroupMemberQueryResultList;
    fm_.ServiceCacheObj.GetServiceGroupMemberQueryResults(filterFunction, serviceGroupMemberQueryResultList);
    queryResult = ServiceModel::QueryResult(move(serviceGroupMemberQueryResultList));

    return ErrorCode::Success();
}

ServicePartitionInformation FMQueryHelper::CheckPartitionInformation(Reliability::ConsistencyUnitDescription const & consistencyUnitDescription)
{
    if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
    {
        return ServicePartitionInformation(consistencyUnitDescription.ConsistencyUnitId.Guid);
    }
    else if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
    {
        return ServicePartitionInformation(
            consistencyUnitDescription.ConsistencyUnitId.Guid,
            static_cast<int64>(consistencyUnitDescription.LowKey),
            static_cast<int64>(consistencyUnitDescription.HighKey));
    }
    else if (consistencyUnitDescription.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_NAMED)
    {
        return ServicePartitionInformation(
            consistencyUnitDescription.ConsistencyUnitId.Guid,
            consistencyUnitDescription.PartitionName);
    }
    else
    {
        Assert::CodingError("Invalid Partition Kind");
    }
}

void FMQueryHelper::InsertPartitionIntoQueryResultList(LockedFailoverUnitPtr const & failoverUnitPtr, ServicePartitionInformation const & servicePartitionInformation, vector<ServicePartitionQueryResult> & partitionQueryResultList)
{
    if (failoverUnitPtr->IsStateful)
    {
        partitionQueryResultList.push_back(ServicePartitionQueryResult::CreateStatefulServicePartitionQueryResult(
            servicePartitionInformation,
            failoverUnitPtr->TargetReplicaSetSize,
            failoverUnitPtr->MinReplicaSetSize,
            failoverUnitPtr->PartitionStatus,
            failoverUnitPtr->LastQuorumLossDuration.TotalSeconds(),
            failoverUnitPtr->CurrentConfigurationEpoch.ToPrimaryEpoch()));
    }
    else
    {
        partitionQueryResultList.push_back(ServicePartitionQueryResult::CreateStatelessServicePartitionQueryResult(
            servicePartitionInformation,
            failoverUnitPtr->TargetReplicaSetSize,
            failoverUnitPtr->PartitionStatus));
    }
}

void FMQueryHelper::InsertReplicaInfoForPartition(LockedFailoverUnitPtr const &failoverUnitPtr, bool isFilterByPartitionName, wstring const & partitionNameFilter, ReplicasByServiceQueryResult &replicasByServiceResult)
{
    auto const & consistencyUnitDescription = failoverUnitPtr->FailoverUnitDescription.ConsistencyUnitDescription;
    for (auto replica = failoverUnitPtr->BeginIterator; replica != failoverUnitPtr->EndIterator; ++replica)
    {
        if (isFilterByPartitionName && consistencyUnitDescription.PartitionName != partitionNameFilter) 
        {
            continue;
        }

        replicasByServiceResult.ReplicaInfos.push_back(move(ReplicaInfoResult(
            consistencyUnitDescription.ConsistencyUnitId.Guid,
            consistencyUnitDescription.PartitionName,
            replica->ReplicaDescription.ReplicaId,
            replica->NodeInfoObj->NodeName)));
    }
}

ErrorCode FMQueryHelper::GetServicePartitionList(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    vector<ServicePartitionQueryResult> partitionQueryResultList;
    wstring desiredServiceName;
    wstring desiredPartitionId;

    bool filter = queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, desiredPartitionId);
    bool serviceNamePresent = queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, desiredServiceName);

    if (!serviceNamePresent && !filter)
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetServicePartitionList: Argument {1} and {2} not found", activityId, QueryResourceProperties::Service::ServiceName, QueryResourceProperties::Partition::PartitionId);
        return ErrorCodeValue::InvalidArgument;
    }

    wstring continuationToken;
    Guid continuationTokenPartitionId = Guid::Empty();
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);
    if (checkContinuationToken)
    {
        auto error = PagingStatus::GetContinuationTokenData<Guid>(continuationToken, continuationTokenPartitionId);
        if (!error.IsSuccess())
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: error parsing continuation token {1}: {2}",
                activityId,
                continuationToken,
                error);
            return error;
        }
    }

    if (!serviceNamePresent) {

        FailoverUnitId desiredFailoverUnitId = FailoverUnitId(Common::Guid(desiredPartitionId));

        LockedFailoverUnitPtr failoverUnitPtr;
        if (!fm_.FailoverUnitCacheObj.TryGetLockedFailoverUnit(desiredFailoverUnitId, failoverUnitPtr))
        {
            fm_.WriteInfo(Constants::QuerySource, "{0}: GetServicePartitionList failed because of timeout while aquiring lock", activityId);
            return ErrorCodeValue::UpdatePending;
        }

        if (failoverUnitPtr)
        {
            auto const & consistencyUnitDescription = failoverUnitPtr->FailoverUnitDescription.ConsistencyUnitDescription;
            auto partitionInformation = CheckPartitionInformation(consistencyUnitDescription);
            InsertPartitionIntoQueryResultList(failoverUnitPtr, partitionInformation, partitionQueryResultList);
        }

    }
    else
    {
        desiredServiceName = SystemServiceApplicationNameHelper::GetInternalServiceName(desiredServiceName);

        ErrorCode error = fm_.ServiceCacheObj.IterateOverFailoverUnits(
            desiredServiceName,
            [&filter, &desiredPartitionId](FailoverUnitId const& failoverUnitId) -> bool
        {
            return (!filter || (filter && failoverUnitId.Guid.ToString() == desiredPartitionId));
        },
            [&checkContinuationToken, &continuationTokenPartitionId, &partitionQueryResultList, this](LockedFailoverUnitPtr & failoverUnit)->ErrorCode
        {
            if (!failoverUnit->IsOrphaned)
            {
                auto const & consistencyUnitDescription = failoverUnit->FailoverUnitDescription.ConsistencyUnitDescription;
                if (checkContinuationToken && (consistencyUnitDescription.ConsistencyUnitId.Guid <= continuationTokenPartitionId))
                {
                    // Continuation token is not respected
                    return ErrorCodeValue::Success;
                }

                InsertPartitionIntoQueryResultList(failoverUnit, CheckPartitionInformation(consistencyUnitDescription), partitionQueryResultList);
            }

            return ErrorCodeValue::Success;
        });

        if (!error.IsSuccess())
        {
            return error;
        }

        // Sort by partition id
        sort(partitionQueryResultList.begin(), partitionQueryResultList.end(), [](ServicePartitionQueryResult const & l, ServicePartitionQueryResult const & r)
        {
            return l.PartitionId < r.PartitionId;
        });

    }

    // Add the sorted partitions to result
    ListPager<ServicePartitionQueryResult> pager;
    for (auto it = partitionQueryResultList.begin(); it != partitionQueryResultList.end(); ++it)
    {
        auto error = pager.TryAdd(move(*it));
        if (error.IsError(ErrorCodeValue::EntryTooLarge))
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: reached max message size with {1}: {2}",
                activityId,
                it->PartitionId,
                error.Message);
            break;
        }
    }

    queryResult = ServiceModel::QueryResult(move(pager));
    return ErrorCode::Success();
}

ErrorCode FMQueryHelper::GetPartitionLoadInformation(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring desiredPartitionId;

    queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, desiredPartitionId);

    FailoverUnitId desiredFailoverUnitId = FailoverUnitId(Common::Guid(desiredPartitionId));

    if (!fm_.FailoverUnitCacheObj.FailoverUnitExists(desiredFailoverUnitId))
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetServicePartitionInformation failed. Partition {1} not found", activityId, desiredPartitionId);
        return ErrorCodeValue::PartitionNotFound;
    }

    LoadInfoSPtr loadInfo = fm_.LoadCacheObj.GetLoads(desiredFailoverUnitId);
    std::vector<ServiceModel::LoadMetricReport> primaryLoads;
    std::vector<ServiceModel::LoadMetricReport> secondaryLoads;

    if(loadInfo)
    {
        LoadBalancingComponent::LoadOrMoveCostDescription const & loadDescription = loadInfo->LoadDescription;
        if (loadDescription.IsStateful)
        {
            ConvertToLoadMetricReport(loadDescription.PrimaryEntries, primaryLoads);
            ConvertToLoadMetricReport(loadDescription.SecondaryEntries, secondaryLoads);
        }
        else
        {
            // our code stores load of stateless service in secondaryEntries
            ConvertToLoadMetricReport(loadDescription.SecondaryEntries, primaryLoads);
        }
    }
    else
    {
        LockedFailoverUnitPtr failoverUnitPtr;
        if (!fm_.FailoverUnitCacheObj.TryGetLockedFailoverUnit(desiredFailoverUnitId, failoverUnitPtr))
        {
            fm_.WriteInfo(Constants::QuerySource, "{0}: GetServicePartitionInformation failed because of timeout while aquiring lock", activityId);
            return ErrorCodeValue::UpdatePending;
        }

        for (ServiceLoadMetricDescription const & metric : failoverUnitPtr->ServiceInfoObj->ServiceDescription.Metrics)
        {
            primaryLoads.push_back(LoadMetricReport(
                metric.Name,
                static_cast<uint>(metric.PrimaryDefaultLoad),
                metric.PrimaryDefaultLoad,
                DateTime::Zero));
            if (failoverUnitPtr->IsStateful)
            {
                secondaryLoads.push_back(LoadMetricReport(
                    metric.Name,
                    static_cast<uint>(metric.SecondaryDefaultLoad),
                    metric.SecondaryDefaultLoad,
                    DateTime::Zero));
            }
        }
    }

    unique_ptr<PartitionLoadInformationQueryResult> partitionLoadQueryResult(new PartitionLoadInformationQueryResult(
        desiredFailoverUnitId.Guid,
        std::move(primaryLoads),
        std::move(secondaryLoads)
        ));

    queryResult = ServiceModel::QueryResult(move(partitionLoadQueryResult));

    return ErrorCode::Success();
}

ErrorCode FMQueryHelper::GetServicePartitionReplicaList(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring desiredPartitionId;
    if (!queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, desiredPartitionId))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    FailoverUnitId desiredFailoverUnitId = FailoverUnitId(Common::Guid(desiredPartitionId));
    LockedFailoverUnitPtr failoverUnitPtr;
    if (!fm_.FailoverUnitCacheObj.TryGetLockedFailoverUnit(desiredFailoverUnitId, failoverUnitPtr))
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetPartitionReplicaList failed because of timeout while aquiring lock", activityId);
        return ErrorCodeValue::UpdatePending;
    }

    if (!failoverUnitPtr)
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetPartitionReplicaList failed. Partition {1} not found", activityId, desiredPartitionId);
        return ErrorCodeValue::PartitionNotFound;
    }

    vector<ServiceReplicaQueryResult> replicaQueryResultList;

    // check if we need to filter on the replicaId
    wstring desiredReplicaId;
    bool filterReplicaId = queryArgs.TryGetValue(QueryResourceProperties::Replica::ReplicaId, desiredReplicaId);
    if (!filterReplicaId)
    {
        filterReplicaId = queryArgs.TryGetValue(QueryResourceProperties::Replica::InstanceId, desiredReplicaId);
    }

    wstring replicaStatusFilterString;
    DWORD replicaStatusFilter = (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT;
    if (queryArgs.TryGetValue(QueryResourceProperties::Replica::ReplicaStatusFilter, replicaStatusFilterString))
    {
        if (!StringUtility::TryFromWString<DWORD>(replicaStatusFilterString, replicaStatusFilter))
        {
            return ErrorCodeValue::InvalidArgument;
        }
    }

    wstring continuationToken;
    FABRIC_REPLICA_ID continuationTokenReplicaId = FABRIC_INVALID_REPLICA_ID;
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);
    if (checkContinuationToken)
    {
        auto error = PagingStatus::GetContinuationTokenData<FABRIC_REPLICA_ID>(continuationToken, continuationTokenReplicaId);
        if (!error.IsSuccess())
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: error parsing continuation token {1}: {2}",
                activityId,
                continuationToken,
                error);
            return error;
        }
    }

    for (auto replica = failoverUnitPtr->BeginIterator; replica != failoverUnitPtr->EndIterator; ++replica)
    {
        if (filterReplicaId && (StringUtility::ToWString(replica->ReplicaDescription.ReplicaId) != desiredReplicaId))
        {
            continue;
        }

        if (!IsMatch(replica->ReplicaStatus, replicaStatusFilter))
        {
            continue;
        }

        if (checkContinuationToken && (replica->ReplicaDescription.ReplicaId <= continuationTokenReplicaId))
        {
            continue;
        }

        if (failoverUnitPtr->IsStateful)
        {
            replicaQueryResultList.push_back(ServiceReplicaQueryResult::CreateStatefulServiceReplicaQueryResult(
                replica->ReplicaDescription.ReplicaId,
                ReplicaRole::ConvertToPublicReplicaRole(replica->ReplicaDescription.CurrentConfigurationRole),
                replica->ReplicaStatus,
                replica->ServiceLocation,
                replica->NodeInfoObj->NodeName,
                replica->LastBuildDuration.TotalSeconds()));
        }
        else
        {
            replicaQueryResultList.push_back(ServiceReplicaQueryResult::CreateStatelessServiceInstanceQueryResult(
                replica->ReplicaDescription.ReplicaId,
                replica->ReplicaStatus,
                replica->ServiceLocation,
                replica->NodeInfoObj->NodeName,
                replica->LastBuildDuration.TotalSeconds()));
        }
    }

    // Replicas must be sorted by replica id
    sort(replicaQueryResultList.begin(), replicaQueryResultList.end(), [](ServiceReplicaQueryResult const & l, ServiceReplicaQueryResult const & r)
        {
            return l.ReplicaOrInstanceId < r.ReplicaOrInstanceId;
        });

    fm_.WriteInfo(Constants::QuerySource, "{0}: GetPartitionReplicaList on FM: {1} results", activityId, replicaQueryResultList.size());

    // Add the sorted replicas to result
    ListPager<ServiceReplicaQueryResult> pager;
    for (auto it = replicaQueryResultList.begin(); it != replicaQueryResultList.end(); ++it)
    {
        auto error = pager.TryAdd(move(*it));
        if (error.IsError(ErrorCodeValue::EntryTooLarge))
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: reached max message size with {1}: {2}",
                activityId,
                it->ReplicaOrInstanceId,
                error.Message);
            break;
        }
    }

    queryResult = ServiceModel::QueryResult(move(pager));
    return ErrorCode::Success();
}

ErrorCode FMQueryHelper::GetReplicaListByServiceNames(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring serviceNamesString;
    if (!queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceNames, serviceNamesString))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    // check if we need to filter on the replicaId
    wstring desiredReplicaId;
    bool filterReplicaId = queryArgs.TryGetValue(QueryResourceProperties::Replica::ReplicaId, desiredReplicaId);
    if (!filterReplicaId)
    {
        filterReplicaId = queryArgs.TryGetValue(QueryResourceProperties::Replica::InstanceId, desiredReplicaId);
    }

    NamesArgument serviceNames;
    auto error = JsonHelper::Deserialize(serviceNames, serviceNamesString);
    if (!error.IsSuccess())
    {
        fm_.WriteInfo(Constants::QuerySource,
            "{0}: ReplicasByService Error when deserializing service names {1}",
            activityId,
            error);
        return error;
    }

    ListPager<ReplicasByServiceQueryResult> replicasByService;

    for (auto const &serviceName : serviceNames.Names)
    {
        ReplicasByServiceQueryResult replicasByServiceResult(serviceName);

        // Get replicas in all partitions in the service.
        error = fm_.ServiceCacheObj.IterateOverFailoverUnits(
            serviceName,
            [](FailoverUnitId const&) -> bool
            {
                return true;
            },
            [&replicasByServiceResult, filterReplicaId, &desiredReplicaId, this](LockedFailoverUnitPtr & failoverUnit)->ErrorCode
            {
                if (!failoverUnit->IsOrphaned)
                {
                    InsertReplicaInfoForPartition(failoverUnit, filterReplicaId, desiredReplicaId, replicasByServiceResult);
                }

                return ErrorCodeValue::Success;
            });

        if (!error.IsSuccess())
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: ReplicasByService Error when iterating Failover Unit's for service {1}: {2}",
                activityId,
                serviceName,
                error);
            return error;
        }

        error = replicasByService.TryAdd(move(replicasByServiceResult));
        if (error.IsError(ErrorCodeValue::EntryTooLarge))
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: ReplicasByService reached max message size with {1}: {2}",
                activityId,
                serviceName,
                error);
            break;
        }
        else if (!error.IsSuccess())
        {
            fm_.WriteInfo(Constants::QuerySource,
                "{0}: ReplicasByService error when adding to list pager {1}: {2}",
                activityId,
                serviceName,
                error);
            return error;
        }
    }

    queryResult = ServiceModel::QueryResult(move(replicasByService));
    return ErrorCodeValue::Success;
}

ErrorCode FMQueryHelper::GetClusterLoadInformation(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    UNREFERENCED_PARAMETER(activityId);
    UNREFERENCED_PARAMETER(queryArgs);

    ClusterLoadInformationQueryResult plbQueryResult;

    ErrorCode retValue = fm_.PLB.GetClusterLoadInformationQueryResult(plbQueryResult);

    unique_ptr<ClusterLoadInformationQueryResult> queryResultUPtr(new ClusterLoadInformationQueryResult(move(plbQueryResult)));
    queryResult = ServiceModel::QueryResult(move(queryResultUPtr));

    return ErrorCode::Success();
}

ErrorCode FMQueryHelper::GetNodeLoadInformation(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring nodeName;

    queryArgs.TryGetValue(QueryResourceProperties::Node::Name, nodeName);

    Federation::NodeId nodeId;

    ErrorCode returnValue = Federation::NodeIdGenerator::GenerateFromString(nodeName, nodeId);
    if (returnValue.IsSuccess())
    {
        NodeLoadInformationQueryResult result;
        returnValue = fm_.PLB.GetNodeLoadInformationQueryResult(nodeId, result);
        if (returnValue.IsSuccess())
        {
            result.put_NodeName(nodeName);
            unique_ptr<NodeLoadInformationQueryResult> queryResultUPtr(new NodeLoadInformationQueryResult(move(result)));
            queryResult = ServiceModel::QueryResult(move(queryResultUPtr));
            return returnValue;
        }
    }

    fm_.WriteInfo(Constants::QuerySource, "{0}: GetNodeLoadInformation failed. Node: {1} not found with error {2}", activityId, nodeName, returnValue);

    return returnValue;
}

ErrorCode FMQueryHelper::GetReplicaLoadInformation(QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring desiredPartitionId;
    wstring desiredReplicaId;

    queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, desiredPartitionId);
    queryArgs.TryGetValue(QueryResourceProperties::Replica::ReplicaId, desiredReplicaId);

    FailoverUnitId desiredFailoverUnitId = FailoverUnitId(Common::Guid(desiredPartitionId));

    if (!fm_.FailoverUnitCacheObj.FailoverUnitExists(desiredFailoverUnitId))
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetReplicaLoadInformation failed. Partition {1} not found", activityId, desiredPartitionId);
        return ErrorCodeValue::PartitionNotFound;
    }

    LockedFailoverUnitPtr failoverUnitPtr;
    if (!fm_.FailoverUnitCacheObj.TryGetLockedFailoverUnit(desiredFailoverUnitId, failoverUnitPtr))
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetReplicaLoadInformation failed because of timeout while aquiring lock", activityId);
        return ErrorCodeValue::UpdatePending;
    }

    if (!failoverUnitPtr)
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetReplicaLoadInformation failed. Partition {1} not found", activityId, desiredPartitionId);
        return ErrorCodeValue::PartitionNotFound;
    }

    for (auto replica = failoverUnitPtr->BeginIterator; replica != failoverUnitPtr->EndIterator; ++replica)
    {
        if (StringUtility::ToWString(replica->ReplicaDescription.ReplicaId) == desiredReplicaId)
        {
            ReplicaRole::Enum role = replica->CurrentConfigurationRole;
            LoadInfoSPtr loadInfo = fm_.LoadCacheObj.GetLoads(desiredFailoverUnitId);
            std::vector<ServiceModel::LoadMetricReport>  loadMetricReports;

            if (loadInfo)
            {
                LoadBalancingComponent::LoadOrMoveCostDescription const & loadDescription = loadInfo->LoadDescription;
                if(loadDescription.IsStateful && role == ReplicaRole::Enum::Primary)
                {
                    ConvertToLoadMetricReport(loadDescription.PrimaryEntries, loadMetricReports);
                }
                else
                {
                    auto it = loadDescription.SecondaryEntriesMap.find(replica->FederationNodeId);
                    if (it != loadDescription.SecondaryEntriesMap.end())
                    {
                        ConvertToLoadMetricReport(it->second, loadMetricReports);
                    }
                    else
                    {
                        ConvertToLoadMetricReport(loadDescription.SecondaryEntries, loadMetricReports);
                    }
                }
            }
            else
            {
                // no need to divide and check as services cannot define system metrics (Cpu and Memory)
                bool useSecondaryLoad = replica->FailoverUnitObj.IsStateful && role == ReplicaRole::Enum::Secondary;

                for (ServiceLoadMetricDescription metric : failoverUnitPtr->ServiceInfoObj->ServiceDescription.Metrics)
                {
                    if (useSecondaryLoad)
                    {
                        loadMetricReports.push_back(LoadMetricReport(
                            metric.Name,
                            static_cast<uint>(metric.SecondaryDefaultLoad),
                            metric.SecondaryDefaultLoad,
                            DateTime::Zero));
                    }
                    else
                    {
                        loadMetricReports.push_back(LoadMetricReport(
                            metric.Name,
                            static_cast<uint>(metric.PrimaryDefaultLoad),
                            metric.PrimaryDefaultLoad,
                            DateTime::Zero));
                    }
                }
            }

            unique_ptr<ReplicaLoadInformationQueryResult> replicaLoadQueryResult(new ReplicaLoadInformationQueryResult(
                desiredFailoverUnitId.Guid,
                replica->ReplicaDescription.ReplicaId,
                std::move(loadMetricReports)
                ));

            queryResult = ServiceModel::QueryResult(move(replicaLoadQueryResult));

            return ErrorCode::Success();
        }
    }

    fm_.WriteInfo(Constants::QuerySource, "{0}: GetReplicaLoadInformation failed. Replica {1} not found", activityId, desiredReplicaId);

    return ErrorCodeValue::REReplicaDoesNotExist;
}

Common::ErrorCode FMQueryHelper::GetUnplacedReplicaInformation(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId)
{
    wstring servicePath;
    wstring partitionId;
    wstring onlyQueryPrimariesString;
    NamingUri serviceUri;
    wstring plbServiceName;

    queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, servicePath);
    queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, partitionId);
    queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::OnlyQueryPrimaries, onlyQueryPrimariesString);

    StringUtility::ToLower(onlyQueryPrimariesString);
    Common::Guid partitionGuid(partitionId);

    bool onlyPrimaries = !(onlyQueryPrimariesString.compare(StringUtility::ToWString("true"))) ? true : false;

    if (servicePath == *ServiceModel::SystemServiceApplicationNameHelper::PublicFMServiceName)
    {
        //This is the FMM case
        plbServiceName = *SystemServiceApplicationNameHelper::InternalFMServiceName;
    }
    else if (ServiceModel::SystemServiceApplicationNameHelper::IsSystemServiceName(servicePath))
    {
        //This is the System Services on FM case
        plbServiceName = ServiceModel::SystemServiceApplicationNameHelper::IsInternalServiceName(servicePath) ? move(servicePath) : ServiceModel::SystemServiceApplicationNameHelper::GetInternalServiceName(servicePath);

    }
    else
    {
        //This is the User Services on FM case
        NamingUri::TryParse(servicePath, serviceUri);
        plbServiceName = serviceUri.Name;
    }

    if (!fm_.ServiceCacheObj.ServiceExists(plbServiceName))
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetUnplacedReplicaInformation failed. Service {1} not found", activityId, plbServiceName);
        return ErrorCodeValue::ServiceNotFound;
    }

    if (partitionGuid != Common::Guid::Empty())
    {
        FailoverUnitId desiredFailoverUnitId = FailoverUnitId(partitionGuid);

        if (!fm_.FailoverUnitCacheObj.FailoverUnitExists(desiredFailoverUnitId))
        {
            fm_.WriteInfo(Constants::QuerySource, "{0}: GetUnplacedReplicaInformation failed. Partition {1} not found", activityId, partitionGuid);
            return ErrorCodeValue::PartitionNotFound;
        }
    }

    ServiceModel::UnplacedReplicaInformationQueryResult result = fm_.PLB.GetUnplacedReplicaInformationQueryResult(plbServiceName, partitionGuid, onlyPrimaries);
    unique_ptr<ServiceModel::UnplacedReplicaInformationQueryResult> queryResultUPtr(new ServiceModel::UnplacedReplicaInformationQueryResult(move(result)));
    queryResult = ServiceModel::QueryResult(move(queryResultUPtr));

    return ErrorCode::Success();
}

Common::ErrorCode FMQueryHelper::GetApplicationLoadInformation(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const &)
{
    wstring applicationName;
    queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationName);

    ApplicationLoadInformationQueryResult result;

    ErrorCode returnValue = fm_.PLB.GetApplicationLoadInformationResult(applicationName, result);
    unique_ptr<ApplicationLoadInformationQueryResult> queryResultUPtr(new ApplicationLoadInformationQueryResult(move(result)));
    queryResult = ServiceModel::QueryResult(move(queryResultUPtr));

    return returnValue;
}

Common::ErrorCode FMQueryHelper::GetServiceName(
    ServiceModel::QueryArgumentMap const & queryArgs,
    ServiceModel::QueryResult & queryResult,
    Common::ActivityId const & activityId)
{
    wstring partitionId;
    queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, partitionId);
    if (partitionId.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    FailoverUnitId ftId = FailoverUnitId(Common::Guid(partitionId));

    LockedFailoverUnitPtr failoverUnit;
    if (!fm_.FailoverUnitCacheObj.TryGetLockedFailoverUnit(ftId, failoverUnit))
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetServiceName failed because of timeout while aquiring lock", activityId);
        return ErrorCodeValue::UpdatePending;
    }
    if (!failoverUnit)
    {
        return ErrorCode(ErrorCodeValue::PartitionNotFound);
    }
    wstring const & serviceNameStr = SystemServiceApplicationNameHelper::GetPublicServiceName(failoverUnit->ServiceName);

    NamingUri serviceName;
    bool success = NamingUri::TryParse(serviceNameStr, serviceName);
    if (!success)
    {
        return ErrorCode(ErrorCodeValue::InvalidNameUri);
    }

    ServiceNameQueryResult result;
    unique_ptr<ServiceNameQueryResult> queryResultUPtr(new ServiceNameQueryResult(move(serviceName)));
    queryResult = ServiceModel::QueryResult(move(queryResultUPtr));

    return ErrorCode::Success();
}

ErrorCode FMQueryHelper::ProcessNIMQuery(Query::QueryNames::Enum queryname, QueryArgumentMap const & queryArgs, QueryResult & queryResult, Common::ActivityId const & activityId)
{
    if(!Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
    {
        return ErrorCode(ErrorCodeValue::InvalidConfiguration);
    }

    return fm_.NIS.ProcessNIMQuery(queryname, queryArgs, queryResult, activityId);
}

bool FMQueryHelper::IsMatch(FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus, DWORD replicaStatusFilter)
{
    if (replicaStatusFilter == (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT)
    {
        return (replicaStatus != FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED);
    }
    if (replicaStatusFilter == (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_ALL)
    {
        return true;
    }
    else
    {
        switch(replicaStatus)
        {
            case FABRIC_QUERY_SERVICE_REPLICA_STATUS_INBUILD:
                return ((replicaStatusFilter & FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_INBUILD) == FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_INBUILD);
            case FABRIC_QUERY_SERVICE_REPLICA_STATUS_STANDBY:
                return ((replicaStatusFilter & FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_STANDBY) == FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_STANDBY);
            case FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY:
                return ((replicaStatusFilter & FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_READY) == FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_READY);
            case FABRIC_QUERY_SERVICE_REPLICA_STATUS_DOWN:
                return ((replicaStatusFilter & FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DOWN) == FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DOWN);
            case FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED:
                return ((replicaStatusFilter & FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DROPPED) == FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DROPPED);
            default:
                return false;
        }
    }
}

bool FMQueryHelper::IsMatch(FABRIC_QUERY_NODE_STATUS nodeStatus, DWORD nodeStatusFilter)
{
    if (nodeStatusFilter == (DWORD)FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT)
    {
        return (nodeStatus != FABRIC_QUERY_NODE_STATUS_UNKNOWN && nodeStatus != FABRIC_QUERY_NODE_STATUS_REMOVED);
    }
    if (nodeStatusFilter == (DWORD)FABRIC_QUERY_NODE_STATUS_FILTER_ALL)
    {
        return true;
    }
    else
    {
        switch (nodeStatus)
        {
        case FABRIC_QUERY_NODE_STATUS_UP:
            return ((nodeStatusFilter & FABRIC_QUERY_NODE_STATUS_FILTER_UP) == FABRIC_QUERY_NODE_STATUS_FILTER_UP);
        case FABRIC_QUERY_NODE_STATUS_DOWN:
            return ((nodeStatusFilter & FABRIC_QUERY_NODE_STATUS_FILTER_DOWN) == FABRIC_QUERY_NODE_STATUS_FILTER_DOWN);
        case FABRIC_QUERY_NODE_STATUS_ENABLING:
            return ((nodeStatusFilter & FABRIC_QUERY_NODE_STATUS_FILTER_ENABLING) == FABRIC_QUERY_NODE_STATUS_FILTER_ENABLING);
        case FABRIC_QUERY_NODE_STATUS_DISABLING:
            return ((nodeStatusFilter & FABRIC_QUERY_NODE_STATUS_FILTER_DISABLING) == FABRIC_QUERY_NODE_STATUS_FILTER_DISABLING);
        case FABRIC_QUERY_NODE_STATUS_DISABLED:
            return ((nodeStatusFilter & FABRIC_QUERY_NODE_STATUS_FILTER_DISABLED) == FABRIC_QUERY_NODE_STATUS_FILTER_DISABLED);
        case FABRIC_QUERY_NODE_STATUS_UNKNOWN:
            return ((nodeStatusFilter & FABRIC_QUERY_NODE_STATUS_FILTER_UNKNOWN) == FABRIC_QUERY_NODE_STATUS_FILTER_UNKNOWN);
        case FABRIC_QUERY_NODE_STATUS_REMOVED:
            return ((nodeStatusFilter & FABRIC_QUERY_NODE_STATUS_FILTER_REMOVED) == FABRIC_QUERY_NODE_STATUS_FILTER_REMOVED);
        default:
            return false;
        }
    }
}

void FMQueryHelper::ConvertToLoadMetricReport(
    std::vector<LoadBalancingComponent::LoadMetricStats> const & loadMetricStats,
    __inout vector<LoadMetricReport> & loadMetricReport)
{
    for(auto iter = loadMetricStats.begin(); iter != loadMetricStats.end();++iter)
    {
        uint loadValue = static_cast<uint>(iter->Value);
        double currentloadValue = iter->Value;
        if (iter->Name == ServiceModel::Constants::SystemMetricNameCpuCores)
        {
            loadValue = static_cast<uint>(loadValue / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor);
            currentloadValue = currentloadValue / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
        }
        loadMetricReport.push_back(LoadMetricReport(iter->Name, loadValue, currentloadValue, StopwatchTime::ToDateTime(iter->LastReportTime)));
    }
}

Common::ErrorCode FMQueryHelper::GetServiceNameAndCurrentPrimaryNode(
    FailoverUnitId const & failoverUnitId,
    ActivityId const & activityId,
    wstring & serviceName,
    Federation::NodeId & currentPrimaryNodeId)
{
    if (!fm_.FailoverUnitCacheObj.FailoverUnitExists(failoverUnitId))
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetServicePartitionInformation failed. Partition {1} not found", activityId, failoverUnitId.Guid);
        return ErrorCodeValue::PartitionNotFound;
    }

    LockedFailoverUnitPtr failoverUnitPtr;
    if(!this->TryGetLockedFailoverUnit(failoverUnitId, activityId, failoverUnitPtr))
    {
        return ErrorCodeValue::UpdatePending;
    }

    if (!failoverUnitPtr->IsStateful || failoverUnitPtr->CurrentConfiguration.IsEmpty)
    {
        return ErrorCodeValue::InvalidPartitionOperation;
    }

    serviceName = failoverUnitPtr->ServiceName;
    currentPrimaryNodeId = failoverUnitPtr->Primary->ReplicaDescription.FederationNodeId;
    return ErrorCodeValue::Success;
}

AsyncOperationSPtr FMQueryHelper::BeginMovePrimaryOperation(
    QueryArgumentMap const & queryArgs,
    ActivityId const & activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<MovePrimaryAsyncOperation>(
        *this,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode FMQueryHelper::EndMovePrimaryOperation(
    Common::AsyncOperationSPtr const & operation
    )
{
    return MovePrimaryAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr FMQueryHelper::BeginMoveSecondaryOperation(
    ServiceModel::QueryArgumentMap const & queryArgs,
    Common::ActivityId const & activityId,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<MoveSecondaryAsyncOperation>(
        *this,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode FMQueryHelper::EndMoveSecondaryOperation(
    Common::AsyncOperationSPtr const & operation)
{
    return MoveSecondaryAsyncOperation::End(operation);
}

bool FMQueryHelper::TryGetLockedFailoverUnit(
    FailoverUnitId const& failoverUnitId,
    ActivityId const & activityId,
    __out LockedFailoverUnitPtr & failoverUnitPtr) const
{
    if (!fm_.FailoverUnitCacheObj.TryGetLockedFailoverUnit(failoverUnitId, failoverUnitPtr))
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetPartitionReplicaList failed because of timeout while acquiring lock", activityId);
        return false;
    }

    if (!failoverUnitPtr)
    {
        fm_.WriteInfo(Constants::QuerySource, "{0}: GetServicePartitionInformation failed. Partition {1} not found", activityId, failoverUnitId.Guid);
        return false;
    }
    return true;
}

Common::ErrorCode FMQueryHelper::VerifyNodeExistsAndIsUp(Federation::NodeId const & nodeId) const
{
    auto nodeInfoSPtr = fm_.NodeCacheObj.GetNode(nodeId);
    if (!nodeInfoSPtr || !nodeInfoSPtr->IsUp)
    {
        return ErrorCodeValue::NodeNotFound;
    }

    return ErrorCodeValue::Success;
}
