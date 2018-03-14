// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ClientServerTransport;
using namespace Common;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace Transport;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Management;
using namespace Management::RepairManager;

StringLiteral const TraceComponent("RepairPreparationAsyncOperation");

RepairPreparationAsyncOperation::RepairPreparationAsyncOperation(
    RepairManagerServiceReplica & owner,
    RepairTaskContext & context,
    Store::ReplicaActivityId const & activityId,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , ReplicaActivityTraceComponent(activityId)
    , owner_(owner)
    , context_(context)
{
}

void RepairPreparationAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    auto impact = context_.Task.Impact;
    CODING_ERROR_ASSERT(impact);

    // TODO Refactor prepare/restore async ops to be polymorphic based on impact type
    switch (impact->Kind)
    {
    case RepairImpactKind::Node:

        if (context_.GetStatusOnly)
        {
            this->GetDeactivationStatus(thisSPtr);
        }
        else
        {            
            // This leads to a chain of method calls finally resulting in SendDeactivationRequest if everything was successful.
            // if at any point, there is an error, we return with TryComplete(thisSPtr, error);
            UpdateHealthCheckStartInfo(thisSPtr);
        }
        break;

    default:

        WriteWarning(TraceComponent,
            "{0} RepairPreparationAsyncOperation: unknown impact kind {1}",
            this->ReplicaActivityId,
            impact->Kind);

        this->TryComplete(thisSPtr, ErrorCodeValue::InvalidState);
    }
}

/// <summary>
/// Updates the preparing health check start time of the repair task in the replicated store.
/// </summary>
void RepairPreparationAsyncOperation::UpdateHealthCheckStartInfo(__in AsyncOperationSPtr const & thisSPtr)
{
    if (context_.Task.PreparingHealthCheckState == RepairTaskHealthCheckState::NotStarted)
    {
        // do this whether Task.PerformPreparingHealthCheck is enabled or not

        context_.Task.PrepareForPreparingHealthCheckStart();

        auto storeTx = StoreTransaction::Create(owner_.ReplicatedStore, owner_.PartitionedReplicaId, this->ActivityId);

        auto error = storeTx.Update(context_);
        if (error.IsSuccess())
        {
            auto operation = storeTx.BeginCommit(
                move(storeTx),
                context_,
                TimeSpan::MaxValue, 
                [this](AsyncOperationSPtr const & operation) { this->OnUpdateHealthCheckStartInfoComplete(operation, false); },
                thisSPtr);
            this->OnUpdateHealthCheckStartInfoComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent,
                "{0} PerformHealthCheckStart: update failed: id = {1}, error = {2}",
                this->ReplicaActivityId,
                context_.TaskId,
                error);

            this->TryComplete(thisSPtr, error);
        }
    }
    else
    {
        UpdateHealthCheckEndInfo(thisSPtr);
    }
}

void RepairPreparationAsyncOperation::OnUpdateHealthCheckStartInfoComplete(
    __in AsyncOperationSPtr const & operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent,
            "{0} OnHealthCheckStartComplete: update failed: id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error);

        this->TryComplete(thisSPtr, error);
        return;
    }

    UpdateHealthCheckEndInfo(thisSPtr);
}

void RepairPreparationAsyncOperation::UpdateHealthCheckEndInfo(__in AsyncOperationSPtr const & thisSPtr)
{    
    if (context_.Task.PreparingHealthCheckState == RepairTaskHealthCheckState::InProgress)
    {
        if (context_.Task.PerformPreparingHealthCheck)
        {
            HealthCheckStoreData healthCheckStoreData;
            ErrorCode error = owner_.GetHealthCheckStoreData(this->ReplicaActivityId, healthCheckStoreData);
            if (!error.IsSuccess())
            {
                WriteWarning(TraceComponent,
                    "{0} PerformHealthCheckEnd: getting health check store data failed, returning: id = {1}, error = {2}",
                    this->ReplicaActivityId,
                    context_.TaskId,
                    error);
                this->TryComplete(thisSPtr, error);
                return;
            }

            auto healthCheckStart = context_.Task.History.PreparingHealthCheckStartTimestamp;
            auto windowStart = healthCheckStoreData.LastErrorAt;

            if (!RepairManagerConfig::GetConfig().PreparingHealthCheckAllowLookback &&
                (windowStart < healthCheckStart))
            {
                windowStart = healthCheckStart;
            }

            RepairTaskHealthCheckState::Enum healthCheckState = RepairTaskHealthCheckHelper::ComputeHealthCheckState(
                this->ReplicaActivityId,
                context_.Task.TaskId,
                healthCheckStoreData.LastErrorAt,
                healthCheckStoreData.LastHealthyAt,
                healthCheckStart,
                windowStart,
                RepairManagerConfig::GetConfig().PreparingHealthCheckRetryTimeout);

            if (healthCheckState == RepairTaskHealthCheckState::InProgress)
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::HealthCheckPending);
                return;
            }

            // we either succeeded or timed out
            context_.Task.PrepareForPreparingHealthCheckEnd(healthCheckState);
        }
        else
        {
            context_.Task.PrepareForPreparingHealthCheckEnd(RepairTaskHealthCheckState::Skipped);

            WriteInfo(TraceComponent,
                "{0} PerformHealthCheckEnd: health check not required, returning: id = {1}",
                this->ReplicaActivityId,
                context_.TaskId);
        }

        UpdateHealthCheckResultInStore(thisSPtr);
    }
    else
    {
        SendDeactivationRequest(thisSPtr);
    }
}

void RepairPreparationAsyncOperation::UpdateHealthCheckResultInStore(__in AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = StoreTransaction::Create(owner_.ReplicatedStore, owner_.PartitionedReplicaId, this->ActivityId);

    auto error = storeTx.Update(context_);
    if (error.IsSuccess())
    {
        auto operation = storeTx.BeginCommit(
            move(storeTx),
            context_,
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnHealthCheckEndComplete(operation, false); },
            thisSPtr);
        this->OnHealthCheckEndComplete(operation, true);
    }
    else
    {
        WriteInfo(TraceComponent,
            "{0} PerformHealthCheckEnd: update failed: id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error);

        this->TryComplete(thisSPtr, error);
    }
}

void RepairPreparationAsyncOperation::OnHealthCheckEndComplete(
    __in AsyncOperationSPtr const & operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);
    
    if (error.IsSuccess())
    {
        SendDeactivationRequest(thisSPtr);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0} OnHealthCheckEndComplete: id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error);

        this->TryComplete(thisSPtr, error);
    }
}

void RepairPreparationAsyncOperation::SendDeactivationRequest(AsyncOperationSPtr const& thisSPtr)
{
    if (context_.Task.PreparingHealthCheckState == RepairTaskHealthCheckState::TimedOut)
    {
        auto error1 = ErrorCode(ErrorCodeValue::InvestigationRequired);

        WriteWarning(
            TraceComponent,
            "{0} SendDeactivationRequest: cannot deactivate since Preparing health check timed out, id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error1);

        this->TryComplete(thisSPtr, error1);
        return;
    }

    map<NodeId, NodeDeactivationIntent::Enum> request;

    auto error = this->CreateDeactivateNodesRequest(request);

    if (error.IsSuccess())
    {
        // Note that the FM will add a dummy node entry for nodes that
        // do no exist in its node cache in order to persist the node's deactivation
        // status.
        //
        auto operation = owner_.ClusterManagementClient->BeginDeactivateNodesBatch(
            move(request),
            context_.ConstructDeactivationBatchId(),
            RepairManagerConfig::GetConfig().MaxCommunicationTimeout,
            [this] (AsyncOperationSPtr const & operation) { this->OnDeactivationRequestComplete(operation, false); },
            thisSPtr);
        this->OnDeactivationRequestComplete(operation, true);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void RepairPreparationAsyncOperation::OnDeactivationRequestComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = owner_.ClusterManagementClient->EndDeactivateNodesBatch(operation);

    WriteInfo(TraceComponent,
        "{0} DeactivateNodesBatch completed: id = {1}, error = {2}",
        this->ReplicaActivityId,
        context_.TaskId,
        error);

    if (error.IsSuccess())
    {
        this->GetDeactivationStatus(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void RepairPreparationAsyncOperation::GetDeactivationStatus(AsyncOperationSPtr const& thisSPtr)
{
    auto operation = owner_.ClusterManagementClient->BeginGetNodeDeactivationStatus(
        context_.ConstructDeactivationBatchId(),
        RepairManagerConfig::GetConfig().MaxCommunicationTimeout,
        [this] (AsyncOperationSPtr const & operation) { this->OnDeactivationStatusComplete(operation, false); },
        thisSPtr);
    this->OnDeactivationStatusComplete(operation, true);
}

void RepairPreparationAsyncOperation::OnDeactivationStatusComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    NodeDeactivationStatus::Enum status;
    auto error = owner_.ClusterManagementClient->EndGetNodeDeactivationStatus(operation, status);

    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent,
            "{0} GetNodeDeactivationStatus failed: id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error);

        this->TryComplete(thisSPtr, error);
    }
    else if (status != NodeDeactivationStatus::DeactivationComplete)
    {
        WriteInfo(TraceComponent,
            "{0} GetNodeDeactivationStatus succeeded: id = {1}, status = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            status);

        // Once deactivation has succeeded, we only need to poll the FM for
        // status. This is only tracked in-memory. On failover, the CM will
        // send another deactivation request to kickstart the process, which
        // FM will handle idempotently.
        //
        context_.GetStatusOnly = true;
        this->TryComplete(thisSPtr, ErrorCodeValue::NodeDeactivationInProgress);
    }
    else
    {
        WriteInfo(TraceComponent,
            "{0} Deactivation completed: id = {1}",
            this->ReplicaActivityId,
            context_.TaskId);

        this->ApproveTask(thisSPtr);
    }
}

void RepairPreparationAsyncOperation::ApproveTask(AsyncOperationSPtr const & thisSPtr)
{
    context_.Task.PrepareForSystemApproval();

    auto storeTx = StoreTransaction::Create(owner_.ReplicatedStore, owner_.PartitionedReplicaId, this->ActivityId);

    auto error = storeTx.Update(context_);
    if (error.IsSuccess())
    {        
        WriteInfo(TraceComponent,
            "{0} ApproveTask: id = {1}",
            this->ReplicaActivityId,
            context_.TaskId);

        auto operation = storeTx.BeginCommit(
            move(storeTx),
            context_,
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnApprovalComplete(operation, false); },
            thisSPtr);
        this->OnApprovalComplete(operation, true);
    }
    else
    {
        WriteInfo(TraceComponent,
            "{0} ApproveTask: update failed: id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error);

        this->TryComplete(thisSPtr, error);
    }
}

void RepairPreparationAsyncOperation::OnApprovalComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);

    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        "{0} OnApprovalComplete: id = {1}, error = {2}",
        this->ReplicaActivityId,
        context_.TaskId,
        error);

    this->TryComplete(thisSPtr, error);
}

ErrorCode RepairPreparationAsyncOperation::CreateDeactivateNodesRequest(
    __out map<NodeId, NodeDeactivationIntent::Enum> & request) const
{
    map<NodeId, NodeDeactivationIntent::Enum> nodesMap;

    auto nodeImpactDescription = dynamic_pointer_cast<NodeRepairImpactDescription const>(context_.Task.Impact);

    for (auto const & nodeImpact : nodeImpactDescription->NodeImpactList)
    {
        NodeDeactivationIntent::Enum fmIntent;

        auto error = this->GetDeactivationIntent(nodeImpact.ImpactLevel, fmIntent);
        if (!error.IsSuccess())
        {
            return error;
        }

        NodeId nodeId;
        wstring nodeName = nodeImpact.NodeName;

        error = this->GetNodeId(nodeName, nodeId);
        if (!error.IsSuccess())
        {
            return error;
        }

        nodesMap.insert(pair<NodeId, NodeDeactivationIntent::Enum>(nodeId, fmIntent));
    }

    request = move(nodesMap);

    return ErrorCode::Success();
}

ErrorCode RepairPreparationAsyncOperation::GetDeactivationIntent(
    NodeImpactLevel::Enum const impactLevel,
    __out NodeDeactivationIntent::Enum & intent) const
{
    switch (impactLevel)
    {
    case NodeImpactLevel::None:
        intent = NodeDeactivationIntent::None;
        break;

    case NodeImpactLevel::Restart:
        intent = NodeDeactivationIntent::Restart;
        break;

    case NodeImpactLevel::RemoveData:
        intent = NodeDeactivationIntent::RemoveData;
        break;

    case NodeImpactLevel::RemoveNode:
        intent = NodeDeactivationIntent::RemoveNode;
        break;

    default:
        WriteWarning(TraceComponent, "{0} invalid node impact level: {1}", this->ReplicaActivityId, impactLevel);
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCode::Success();
}

ErrorCode RepairPreparationAsyncOperation::GetNodeId(wstring & nodeName, __out NodeId & nodeId) const
{
    if (NodeId::TryParse(nodeName, nodeId))
    {
        nodeName = *NodeIdGenerator::NodeIdNamePrefix + nodeName;
    }
    else
    {
        auto error = NodeIdGenerator::GenerateFromString(nodeName, nodeId);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} failed to generate NodeId from NodeName: {1} ({2})",
                this->ReplicaActivityId,
                nodeName,
                error);

            return error;
        }
    }

    return ErrorCodeValue::Success;
}
