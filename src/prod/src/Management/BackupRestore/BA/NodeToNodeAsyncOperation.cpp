// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Transport;
using namespace Management;
using namespace Naming;
using namespace Api;
using namespace BackupRestoreAgentComponent;

StringLiteral const TraceComponent("NodeToNodeAsyncOperation");

NodeToNodeAsyncOperation::NodeToNodeAsyncOperation(
    MessageUPtr && message,
    IpcReceiverContextUPtr && receiverContext,
    ServiceResolver& serviceResolver,
    FederationSubsystem & federation,
    IQueryClientPtr queryClientPtr,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : RequestResponseAsyncOperationBase(move(message), timeout, callback, parent)
    , serviceResolver_(serviceResolver)
    , federation_(federation)
    , queryClientPtr_(queryClientPtr)
    , receiverContext_(move(receiverContext))
{
}

NodeToNodeAsyncOperation::~NodeToNodeAsyncOperation()
{
}

void NodeToNodeAsyncOperation::OnStart(Common::AsyncOperationSPtr const & thisSPtr)
{
    RequestResponseAsyncOperationBase::OnStart(thisSPtr);

    if (receivedMessage_->Headers.Action == BAMessage::ForwardToBRSAction)
    {
        ConsistencyUnitId cuidOfService = ConsistencyUnitId::CreateFirstReservedId(ConsistencyUnitId::BackupRestoreServiceIdRange);

        auto operation = this->serviceResolver_.BeginResolveServicePartition(
            cuidOfService,
            Transport::FabricActivityHeader(),
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnResolveServicePartitionCompleted(operation, false);
            },
            thisSPtr);
        this->OnResolveServicePartitionCompleted(operation, true);
    }
    else if (receivedMessage_->Headers.Action == BAMessage::ForwardToBAPAction)
    {
        wstring continuationToken;

        // Get the PartitionTarget header
        PartitionTargetHeader partitionHeader;
        if (!receivedMessage_->Headers.TryReadFirst(partitionHeader))
        {
            // Header not found. Send error
            WriteWarning(TraceComponent, "{0} Partition Header not found", this->ActivityId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }
        
        auto operation = queryClientPtr_->BeginGetReplicaList(
            partitionHeader.PartitionId, 
            0, 
            0, 
            continuationToken, 
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetReplicaListCompleted(operation, false);
            },
            thisSPtr);

        this->OnGetReplicaListCompleted(operation, true);
    }
}

void NodeToNodeAsyncOperation::OnGetReplicaListCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    std::vector<ServiceModel::ServiceReplicaQueryResult> replicaResult;
    ServiceModel::PagingStatusUPtr pagingStatus;
    wstring nodeName, continuationToken;
    
    auto error = this->queryClientPtr_->EndGetReplicaList(operation, replicaResult, pagingStatus);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent, "{0} GetReplicaList failed with error {1}", this->ActivityId, error);
        this->TryComplete(thisSPtr, error);
        return;
    }
    
    for (std::vector<ServiceModel::ServiceReplicaQueryResult>::iterator it = replicaResult.begin(); it != replicaResult.end(); ++it)
    {
        if (it->ReplicaRole == FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_PRIMARY)
        {
            ScopedHeap heap; 
            FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM result;
            it->ToPublicApi(heap, result);
            auto statefulResult = (FABRIC_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM*)result.Value;
            nodeName = statefulResult->NodeName;
            break;
        }
    }

    if (nodeName.empty())
    {
        error = ErrorCode(ErrorCodeValue::Enum::ServiceOffline);
        WriteWarning(TraceComponent, "{0} GetReplicaList returned with no primary replica, failing with error {1}", this->ActivityId, error);
        this->TryComplete(thisSPtr, error);
        return;
    }

    continuationToken.clear();
    
    auto operation1 = this->queryClientPtr_->BeginGetNodeList(
        nodeName,
        continuationToken,
        this->RemainingTime,
        [this](AsyncOperationSPtr const & getNodeOperation)
        {
            this->OnGetNodeListCompleted(getNodeOperation, false);
        },
        thisSPtr);
    this->OnGetNodeListCompleted(operation1, true);
}

void NodeToNodeAsyncOperation::OnGetNodeListCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    std::vector<ServiceModel::NodeQueryResult> nodeResult;
    ServiceModel::PagingStatusUPtr pagingStatus;
    ServiceModel::NodeQueryResult nodeInstance;

    auto error = this->queryClientPtr_->EndGetNodeList(operation, nodeResult, pagingStatus);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent, "{0} GetNodeList failed with error {1}", this->ActivityId, error);
        this->TryComplete(thisSPtr, error);
        return;
    }
    
    BeginRouteToNode(nodeResult.front().NodeId, nodeResult.front().InstanceId, operation);
}

void NodeToNodeAsyncOperation::OnResolveServicePartitionCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    std::vector<ServiceTableEntry> serviceTableEntry;
    GenerationNumber generation;
    
    ErrorCode error = this->serviceResolver_.EndResolveServicePartition(operation, serviceTableEntry, generation);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent, "{0} Service resolution failed with {1}", this->ActivityId, error);
        this->TryComplete(thisSPtr, error);
        return;
    }

    auto first = serviceTableEntry.front();
    if (!first.ServiceReplicaSet.IsPrimaryLocationValid)
    {
        WriteWarning(TraceComponent, "{0} Service resolve entry doesnt have a valid primary location", this->ActivityId);
        this->TryComplete(thisSPtr, ErrorCodeValue::SystemServiceNotFound);
        return;
    }

    SystemServices::SystemServiceLocation primaryLocation;

    if (!SystemServices::SystemServiceLocation::TryParse(first.ServiceReplicaSet.PrimaryLocation, true, primaryLocation))
    {
        WriteWarning(TraceComponent, "{0} Service resolve parse failed {1}", this->ActivityId, first.ServiceReplicaSet.PrimaryLocation);
        this->TryComplete(thisSPtr, ErrorCodeValue::SystemServiceNotFound);
        return;
    }
    
    NodeInstance brsInstance = primaryLocation.NodeInstance;
    BeginRouteToNode(brsInstance.Id, brsInstance.InstanceId, operation);
}

void NodeToNodeAsyncOperation::BeginRouteToNode(NodeId const nodeId, uint64 instanceId, AsyncOperationSPtr const & operation)
{
    // Update the timeoutheader
    receivedMessage_->Headers.Replace(TimeoutHeader(this->RemainingTime));

    WriteInfo(TraceComponent, "{0} Routing message to node {1}:{2}", this->ActivityId, nodeId, instanceId);

    auto innerOperation = federation_.BeginRouteRequest(
        move(receivedMessage_),
        nodeId,
        instanceId,
        true,
        this->RemainingTime,            // TODO: Need to fix retry timeout
        this->RemainingTime,
        [this](AsyncOperationSPtr const& routeOperation)
        {
            this->OnRouteToNodeRequestComplete(routeOperation, false /* expectedCompletedSynchronously */);
        },
        operation->Parent);

    this->OnRouteToNodeRequestComplete(innerOperation, true /* expectedCompletedSynchronously */);
}

void NodeToNodeAsyncOperation::OnRouteToNodeRequestComplete(
    Common::AsyncOperationSPtr const& asyncOperation,
    bool expectedCompletedSynchronously)
{
    if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }
    
    Transport::MessageUPtr reply;
    ErrorCode error = federation_.EndRouteRequest(asyncOperation, reply);

    if (error.IsSuccess())
    {
        OnRouteToNodeSuccessful(asyncOperation->Parent, reply);
    }
    else
    {
        OnRouteToNodeFailed(asyncOperation->Parent, error);
    }
}

void NodeToNodeAsyncOperation::OnRouteToNodeSuccessful(
    Common::AsyncOperationSPtr const & thisSPtr,
    Transport::MessageUPtr & reply)
{
    this->SetReplyAndComplete(thisSPtr, std::move(reply), ErrorCode::Success());
}

void NodeToNodeAsyncOperation::OnRouteToNodeFailed(
    Common::AsyncOperationSPtr const & thisSPtr,
    Common::ErrorCode const & error)
{
    TryComplete(thisSPtr, error);
}

void NodeToNodeAsyncOperation::OnCompleted()
{
    RequestResponseAsyncOperationBase::OnCompleted();
}

Common::ErrorCode NodeToNodeAsyncOperation::End(
    Common::AsyncOperationSPtr const & asyncOperation,
    __out Transport::MessageUPtr & reply,
    __out Transport::IpcReceiverContextUPtr & receiverContext)
{
    auto casted = AsyncOperation::End<NodeToNodeAsyncOperation>(asyncOperation);
    auto error = casted->Error;

    receiverContext = move(casted->ReceiverContext);
    if (error.IsSuccess())
    {
        reply = std::move(casted->reply_);
    }
    
    return error;
}
