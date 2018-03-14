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

StringLiteral const TraceComponent("RepairRestorationAsyncOperation");

RepairRestorationAsyncOperation::RepairRestorationAsyncOperation(
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

void RepairRestorationAsyncOperation::OnStart(Common::AsyncOperationSPtr const & thisSPtr)
{
    if (context_.TaskState == RepairTaskState::Preparing)
    {
        this->CancelPreparation(thisSPtr);
    }
    else if (context_.TaskState == RepairTaskState::Restoring)
    {
        this->StartRestoration(thisSPtr);
    }
    else
    {
        Assert::CodingError("Invalid state to begin repair restoration: {0}", context_.TaskState);
    }
}

void RepairRestorationAsyncOperation::CancelPreparation(AsyncOperationSPtr const & thisSPtr)
{
    context_.Task.PrepareForSystemCancel();

    auto storeTx = StoreTransaction::Create(owner_.ReplicatedStore, owner_.PartitionedReplicaId, this->ActivityId);

    auto error = storeTx.Update(context_);
    if (error.IsSuccess())
    {
        // TODO replace with a structured event
        WriteInfo(TraceComponent,
            "{0} CancelPreparation: id = {1}",
            this->ReplicaActivityId,
            context_.TaskId);

        auto operation = storeTx.BeginCommit(
            move(storeTx),
            context_,
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnCancelPreparationComplete(operation, false); },
            thisSPtr);
        this->OnCancelPreparationComplete(operation, true);
    }
    else
    {
        WriteInfo(TraceComponent,
            "{0} CancelPreparation: update failed: id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error);

        this->TryComplete(thisSPtr, error);
    }
}

void RepairRestorationAsyncOperation::OnCancelPreparationComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);

    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        "{0} OnCancelPreparationComplete: id = {1}, error = {2}",
        this->ReplicaActivityId,
        context_.TaskId,
        error);

    if (error.IsSuccess())
    {
        this->StartRestoration(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void RepairRestorationAsyncOperation::StartRestoration(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto impact = context_.Task.Impact;
    CODING_ERROR_ASSERT(impact);

    // TODO Refactor prepare/restore async ops to be polymorphic based on impact type
    switch (impact->Kind)
    {
    case RepairImpactKind::Node:

        this->SendRemoveDeactivationRequest(thisSPtr);
        break;

    default:

        WriteWarning(TraceComponent,
            "{0} RepairRestorationAsyncOperation: unknown impact kind {1}",
            this->ReplicaActivityId,
            impact->Kind); 

        this->TryComplete(thisSPtr, ErrorCodeValue::InvalidState);
    }
}

void RepairRestorationAsyncOperation::SendRemoveDeactivationRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto operation = owner_.ClusterManagementClient->BeginRemoveNodeDeactivations(
        context_.ConstructDeactivationBatchId(),
        RepairManagerConfig::GetConfig().MaxCommunicationTimeout,
        [this] (AsyncOperationSPtr const & operation) { this->OnRemoveDeactivationRequestComplete(operation, false); },
        thisSPtr);
    this->OnRemoveDeactivationRequestComplete(operation, true);
}

void RepairRestorationAsyncOperation::OnRemoveDeactivationRequestComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = owner_.ClusterManagementClient->EndRemoveNodeDeactivations(operation);

    WriteInfo(TraceComponent,
        "{0} RemoveNodeDeactivations completed: id = {1}, error = {2}",
        this->ReplicaActivityId,
        context_.TaskId,
        error);

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    // This leads to a chain of method calls finally resulting in CompleteTask if everything was successful.
    // If at any point, there is an error, we return with TryComplete(thisSPtr, error);
    UpdateHealthCheckStartInfo(thisSPtr);
}

/// <summary>
/// Updates the restoring health check start time of the repair task in the replicated store.
/// </summary>
void RepairRestorationAsyncOperation::UpdateHealthCheckStartInfo(__in AsyncOperationSPtr const & thisSPtr)
{
    if (context_.Task.RestoringHealthCheckState == RepairTaskHealthCheckState::NotStarted)
    {
        context_.Task.PrepareForRestoringHealthCheckStart();

        auto storeTx = StoreTransaction::Create(owner_.ReplicatedStore, owner_.PartitionedReplicaId, this->ActivityId);

        // TODO, remove duplication on all store transactions
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

void RepairRestorationAsyncOperation::OnUpdateHealthCheckStartInfoComplete(
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

void RepairRestorationAsyncOperation::UpdateHealthCheckEndInfo(__in AsyncOperationSPtr const & thisSPtr)
{    
    if (context_.Task.RestoringHealthCheckState == RepairTaskHealthCheckState::InProgress)
    {
        // cancel flag is set when 
        // a. task timed out in Preparing where it doesn't make sense for the Restoring health check to run
        //    before moving the repair task to the terminal Completed state
        // b. When user explicitly canceled the task in Preparing
        // c. Task was canceled during RM processing

        bool cancel =
            context_.Task.PreparingHealthCheckState == RepairTaskHealthCheckState::TimedOut ||
            context_.Task.IsCancelRequested; // user requested a cancel
            
        bool needed = context_.Task.PerformRestoringHealthCheck && !cancel;

        WriteInfo(TraceComponent,
            "{0} PerformHealthCheckEnd start: id = {1}, health check computation needed: {2}, Task[PerformRestoringHealthCheck: {3}, IsCancelRequestedTask: {4}, PreparingHealthCheckState: {5}]",
            this->ReplicaActivityId, context_.TaskId, needed,
            context_.Task.PerformRestoringHealthCheck, context_.Task.IsCancelRequested, context_.Task.PreparingHealthCheckState);

        if (needed)
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

            // check if health check is done
            // if not return error InProgress or timed out
            //
            // for restoration, rebuild the stable window from scratch. this is because a repair task would typically
            // degrade the cluster health. So, we don't reuse any stored health data.

            auto healthCheckStart = context_.Task.History.RestoringHealthCheckStartTimestamp;
            auto windowStart = healthCheckStoreData.LastErrorAt > healthCheckStart ? healthCheckStoreData.LastErrorAt : healthCheckStart;

            RepairTaskHealthCheckState::Enum healthCheckState = RepairTaskHealthCheckHelper::ComputeHealthCheckState(
                this->ReplicaActivityId,
                context_.Task.TaskId,
                healthCheckStoreData.LastErrorAt,
                healthCheckStoreData.LastHealthyAt,
                healthCheckStart,
                windowStart,
                RepairManagerConfig::GetConfig().RestoringHealthCheckRetryTimeout);

            if (healthCheckState == RepairTaskHealthCheckState::InProgress)
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::HealthCheckPending);
                return;
            }

            // we either succeeded or timed out
            context_.Task.PrepareForRestoringHealthCheckEnd(healthCheckState);
        }
        else
        {
            context_.Task.PrepareForRestoringHealthCheckEnd(RepairTaskHealthCheckState::Skipped);
        }

        UpdateHealthCheckResultInStore(thisSPtr);
    }
    else
    {
        this->CompleteTask(thisSPtr);
    }
}

void RepairRestorationAsyncOperation::UpdateHealthCheckResultInStore(__in AsyncOperationSPtr const & thisSPtr)
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

void RepairRestorationAsyncOperation::OnHealthCheckEndComplete(
    __in AsyncOperationSPtr const & operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);

    if (error.IsSuccess())
    {
        this->CompleteTask(thisSPtr);
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

void RepairRestorationAsyncOperation::CompleteTask(AsyncOperationSPtr const & thisSPtr)
{
    context_.Task.PrepareForSystemCompletion();

    auto storeTx = StoreTransaction::Create(owner_.ReplicatedStore, owner_.PartitionedReplicaId, this->ActivityId);

    auto error = storeTx.Update(context_);
    if (error.IsSuccess())
    {
        // TODO replace with a structured event
        WriteInfo(TraceComponent,
            "{0} CompleteTask: id = {1}",
            this->ReplicaActivityId,
            context_.TaskId);

        owner_.CreditCompletedTask(this->ReplicaActivityId);

        auto operation = storeTx.BeginCommit(
            move(storeTx),
            context_,
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnCompleteTaskComplete(operation, false); },
            thisSPtr);
        this->OnCompleteTaskComplete(operation, true);
    }
    else
    {
        WriteInfo(TraceComponent,
            "{0} CompleteTask: update failed: id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error);

        this->TryComplete(thisSPtr, error);
    }
}

void RepairRestorationAsyncOperation::OnCompleteTaskComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = StoreTransaction::EndCommit(operation);

    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        "{0} OnCompleteTaskComplete: id = {1}, error = {2}",
        this->ReplicaActivityId,
        context_.TaskId,
        error);

    this->TryComplete(thisSPtr, error);
}
