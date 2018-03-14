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

StringLiteral const TraceComponent("ProcessRepairTaskContextAsyncOperation");

ProcessRepairTaskContextAsyncOperation::ProcessRepairTaskContextAsyncOperation(
    RepairManagerServiceReplica & owner,
    RepairTaskContext & context,
    Store::ReplicaActivityId const & activityId,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , ReplicaActivityTraceComponent(activityId)
    , owner_(owner)
    , context_(context)
{
}

void ProcessRepairTaskContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->ProcessTask(thisSPtr);
}

void ProcessRepairTaskContextAsyncOperation::ProcessTask(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = StoreTransaction::Create(owner_.ReplicatedStore, owner_.PartitionedReplicaId, this->ActivityId);

    context_.NeedsRefresh = false;

    auto error = storeTx.ReadExact(context_);

    if (error.IsSuccess())
    {
        WriteInfo(TraceComponent,
            "{0} Processing repair task: id = {1}, state = {2}, cancelled = {3}",
            this->ReplicaActivityId,
            context_.TaskId,
            context_.TaskState,
            context_.Task.IsCancelRequested);

        switch (context_.TaskState)
        {
        case RepairTaskState::Preparing:
            if (context_.Task.IsCancelRequested || context_.Task.PreparingHealthCheckState == RepairTaskHealthCheckState::TimedOut)
            {
                this->RestoreTask(thisSPtr);
            }
            else
            {
                this->PrepareTask(thisSPtr);
            }
            break;

        case RepairTaskState::Restoring:
            this->RestoreTask(thisSPtr);
            break;

        default:
            // Nothing to do right now; finish the task
            this->FinishProcessTask(thisSPtr, error);
            break;
        }
    }
    else
    {
        WriteWarning(TraceComponent,
            "{0} Failed to read repair task: id = {1}, error = {2}",
            this->ReplicaActivityId,
            context_.TaskId,
            error);

        this->FinishProcessTask(thisSPtr, error);
    }
}

void ProcessRepairTaskContextAsyncOperation::PrepareTask(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<RepairPreparationAsyncOperation>(
        owner_,
        context_,
        this->ReplicaActivityId,
        [this] (AsyncOperationSPtr const & operation) { this->OnPrepareTaskComplete(operation, false); },
        thisSPtr);
    this->OnPrepareTaskComplete(operation, true);
}

void ProcessRepairTaskContextAsyncOperation::OnPrepareTaskComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = AsyncOperation::End<RepairPreparationAsyncOperation>(operation)->Error;

    WriteInfo(TraceComponent,
        "{0} Prepare operation completed: task id = {1}, error = {2}",
        this->ReplicaActivityId,
        context_.TaskId,
        error);

    this->FinishProcessTask(thisSPtr, error);
}

void ProcessRepairTaskContextAsyncOperation::RestoreTask(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<RepairRestorationAsyncOperation>(
        owner_,
        context_,
        this->ReplicaActivityId,
        [this] (AsyncOperationSPtr const & operation) { this->OnRestoreTaskComplete(operation, false); },
        thisSPtr);
    this->OnRestoreTaskComplete(operation, true);
}

void ProcessRepairTaskContextAsyncOperation::OnRestoreTaskComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = AsyncOperation::End<RepairRestorationAsyncOperation>(operation)->Error;

    WriteInfo(TraceComponent,
        "{0} Restore operation completed: task id = {1}, error = {2}",
        this->ReplicaActivityId,
        context_.TaskId,
        error);

    this->FinishProcessTask(thisSPtr, error);
}

void ProcessRepairTaskContextAsyncOperation::FinishProcessTask(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    if (error.IsSuccess())
    {
        owner_.TryRemoveTask(context_, this->ReplicaActivityId);
    }

    this->TryComplete(thisSPtr, error);
}
