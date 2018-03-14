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

StringLiteral const TraceComponent("ProcessRepairTaskContextsAsyncOperation");

ProcessRepairTaskContextsAsyncOperation::ProcessRepairTaskContextsAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    vector<shared_ptr<RepairTaskContext>> && tasks,
    Store::ReplicaActivityId const & activityId,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , ReplicaActivityTraceComponent(activityId)
    , owner_(owner)
    , tasks_(move(tasks))
    , currentTaskIndex_(0)
    , currentNestedActivityId_(this->ActivityId)
{
}

void ProcessRepairTaskContextsAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto error = owner_.BeginOperation(this->ReplicaActivityId);
    if (error.IsSuccess())
    {
        WriteInfo(TraceComponent,
            "{0} Repair tasks in current batch: {1}",
            this->ReplicaActivityId,
            tasks_.size());

        this->ScheduleProcessNextTask(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void ProcessRepairTaskContextsAsyncOperation::ScheduleProcessNextTask(AsyncOperationSPtr const & thisSPtr)
{
    timer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer)
    {
        timer->Cancel();
        this->ProcessNextTask(thisSPtr);
    },
    true); // allow concurrency

    timer_->Change(TimeSpan::Zero);
}

void ProcessRepairTaskContextsAsyncOperation::ProcessNextTask(AsyncOperationSPtr const & thisSPtr)
{
    auto managerState = owner_.State;
    if (managerState != RepairManagerState::Started)
    {
        WriteInfo(TraceComponent,
            "{0} Halting processing because repair manager state is {1}",
            this->ReplicaActivityId,
            managerState);

        owner_.EndOperation(this->ReplicaActivityId);
        this->TryComplete(thisSPtr, ErrorCodeValue::NotPrimary);
    }
    else if (currentTaskIndex_ == tasks_.size())
    {
        WriteInfo(TraceComponent,
            "{0} Finished processing all tasks in current batch",
            this->ReplicaActivityId);

        owner_.EndOperation(this->ReplicaActivityId);
        this->TryComplete(thisSPtr, ErrorCode::Success());
    }
    else
    {
        RepairTaskContext & currentContext = *tasks_[currentTaskIndex_];
        currentNestedActivityId_ = currentNestedActivityId_.GetNestedActivity();

        ++currentTaskIndex_;

        WriteInfo(TraceComponent,
            "{0} Processing task {1} of {2}",
            this->ReplicaActivityId,
            currentTaskIndex_,
            tasks_.size());

        auto operation = AsyncOperation::CreateAndStart<ProcessRepairTaskContextAsyncOperation>(
            owner_,
            currentContext,
            Store::ReplicaActivityId(owner_.PartitionedReplicaId, currentNestedActivityId_),
            [this] (AsyncOperationSPtr const & operation) { this->OnProcessNextTaskComplete(operation, false); },
            thisSPtr);
        this->OnProcessNextTaskComplete(operation, true);
    }
}

void ProcessRepairTaskContextsAsyncOperation::OnProcessNextTaskComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = AsyncOperation::End<ProcessRepairTaskContextAsyncOperation>(operation)->Error;

    WriteInfo(TraceComponent,
        "{0} Finished processing task {1} of {2}, error = {3}",
        this->ReplicaActivityId,
        currentTaskIndex_,
        tasks_.size(),
        error);

    this->ScheduleProcessNextTask(thisSPtr);
}
