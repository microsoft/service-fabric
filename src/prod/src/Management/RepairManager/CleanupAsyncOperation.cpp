// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ClientServerTransport;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ServiceModel;
using namespace Store;
using namespace Management;
using namespace Management::RepairManager;

StringLiteral const TraceComponent("CleanupAsyncOperation");

CleanupAsyncOperation::CleanupAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    Store::ReplicaActivityId const & activityId,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , ReplicaActivityTraceComponent(activityId)
    , owner_(owner)
    , currentNestedActivityId_(this->ActivityId)
{
}

void CleanupAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (RepairManagerConfig::GetConfig().CompletedTaskCleanupEnabled)
    {
        auto error = owner_.BeginOperation(this->ReplicaActivityId);
        if (error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} Starting cleanup pass", this->ReplicaActivityId);
            this->RunCleanup(thisSPtr);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} Unable to start cleanup pass", this->ReplicaActivityId);
            this->TryComplete(thisSPtr, error);
        }
    }
    else
    {
        WriteInfo(TraceComponent, "{0} Automatic cleanup is disabled", this->ReplicaActivityId);
        this->TryComplete(thisSPtr, ErrorCode::Success());
    }
}

void CleanupAsyncOperation::RunCleanup(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = StoreTransaction::Create(owner_.ReplicatedStore, owner_.PartitionedReplicaId, this->ActivityId);

    // Read from the store, filtering by task ID prefix.
    vector<RepairTaskContext> contexts;
    auto error = storeTx.ReadPrefix<RepairTaskContext>(
        Constants::StoreType_RepairTaskContext,
        L"",
        contexts);

    if (!error.IsSuccess())
    {
        storeTx.Rollback();
        owner_.EndOperation(this->ReplicaActivityId);
        this->TryComplete(thisSPtr, error);
        return;
    }

    storeTx.CommitReadOnly();

    size_t originalTotalTaskCount = contexts.size();

    // Remove all active tasks
    contexts.erase(
        std::remove_if(
            contexts.begin(),
            contexts.end(),
            [](RepairTaskContext const & c) { return c.TaskState != RepairTaskState::Completed; }),
        contexts.end());

    WriteInfo(TraceComponent,
        "{0} Task counts: active = {1}, completed = {2}, total = {3}",
        this->ReplicaActivityId,
        originalTotalTaskCount - contexts.size(),
        contexts.size(),
        originalTotalTaskCount);

    // Sort from oldest to newest completion timestamp
    std::sort(contexts.begin(), contexts.end(), [](RepairTaskContext & a, RepairTaskContext & b) {
        return a.Task.History.CompletedUtcTimestamp < b.Task.History.CompletedUtcTimestamp;
    });

    auto now = DateTime::Now();
    size_t totalTaskCount = originalTotalTaskCount;

    auto& config = RepairManagerConfig::GetConfig();
    int maxDesiredTaskCount = config.DesiredTotalTasks;
    TimeSpan minAge = config.MinCompletedTaskAge;
    TimeSpan maxAge = config.MaxCompletedTaskAge;

    // Enumerate from oldest to newest
    for (auto it = contexts.begin(); it != contexts.end(); ++it)
    {
        auto & completedTimestamp = it->Task.History.CompletedUtcTimestamp;
        auto taskAge = now - completedTimestamp;

        WriteInfo(TraceComponent,
            "{0} Evaluating task: id = {1}, completed time = {2}, age = {3}, "
            "min age = {4}, max age = {5}, max count = {6}, current count = {7}",
            this->ReplicaActivityId,
            it->TaskId,
            completedTimestamp,
            taskAge,
            minAge,
            maxAge,
            maxDesiredTaskCount,
            totalTaskCount);

        // Does the task meet the deletion criteria, based on its age and the total task count?
        if ((taskAge > minAge) && ((taskAge > maxAge) || (totalTaskCount > maxDesiredTaskCount)))
        {
            currentNestedActivityId_ = currentNestedActivityId_.GetNestedActivity();

            DeleteRepairTaskMessageBody requestBody(
                it->Task.get_ScopeMutable(),
                it->Task.TaskId,
                it->Task.Version);

            WriteInfo(TraceComponent,
                "{0} Starting delete request: id = {1}, new activity id = {2}",
                this->ReplicaActivityId,
                it->TaskId,
                currentNestedActivityId_);

            --totalTaskCount;

            // Attempt to delete it asynchronously, result does not matter
            AsyncOperation::CreateAndStart<DeleteRepairRequestAsyncOperation>(
                owner_,
                move(requestBody),
                currentNestedActivityId_,
                TimeSpan::MaxValue,
                [this](AsyncOperationSPtr const & operation) { this->OnDeleteTaskComplete(operation); },
                thisSPtr);
        }
    }

    WriteInfo(TraceComponent,
        "{0} Completed cleanup pass; deleted {1} of {2} tasks",
        this->ReplicaActivityId,
        originalTotalTaskCount - totalTaskCount,
        originalTotalTaskCount);

    owner_.BeginGetNodeList(
        [this](AsyncOperationSPtr const & operation) { this->OnGetNodeListComplete(operation); },
        thisSPtr);
}

void CleanupAsyncOperation::OnGetNodeListComplete(AsyncOperationSPtr const & operation)
{
    auto error = owner_.OnGetNodeListComplete(this->ReplicaActivityId, operation);

    owner_.EndOperation(this->ReplicaActivityId);
    this->TryComplete(operation->Parent, error);
}

void CleanupAsyncOperation::OnDeleteTaskComplete(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<DeleteRepairRequestAsyncOperation>(operation);
    
    WriteInfo(TraceComponent,
        "{0} Delete request finished with result {1}",
        casted->ReplicaActivityId,
        casted->Error);
}
