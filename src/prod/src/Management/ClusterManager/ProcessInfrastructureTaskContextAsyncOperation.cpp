// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace Store;
using namespace ServiceModel;
using namespace Transport;
using namespace Management;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("InfrastructureTaskContextAsyncOperation");

//
// Inner Async Operation classes
//

class ProcessInfrastructureTaskContextAsyncOperation::ProcessNodesAsyncOperationBase : public TimedAsyncOperation
{
public:
    ProcessNodesAsyncOperationBase(
        __in ProcessInfrastructureTaskContextAsyncOperation & owner,
        vector<NodeTaskDescription> const & tasks,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , tasks_(tasks)
    {
    }

protected:
    ErrorCode GetNodeId(wstring &nodeName, __out NodeId & nodeId)
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
                owner_.WriteWarning(
                    TraceComponent, 
                    "{0} failed to generate NodeId from NodeName: {1} ({2})", 
                    owner_.TraceId, 
                    nodeName,
                    error);

                return error; 
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode CreateDeactivateNodesRequest(__out map<NodeId, NodeDeactivationIntent::Enum> & request)
    {
        map<NodeId, NodeDeactivationIntent::Enum> nodesMap;

        for (auto const & task : tasks_)
        {
            NodeDeactivationIntent::Enum fmIntent = NodeDeactivationIntent::None;

            switch (task.TaskType)
            {
            case NodeTask::Restart:
                fmIntent = NodeDeactivationIntent::Restart;
                break;

            case NodeTask::Relocate:
                fmIntent = NodeDeactivationIntent::RemoveData;
                break;

            case NodeTask::Remove:
                fmIntent = NodeDeactivationIntent::RemoveNode;
                break;

            default:
                owner_.WriteWarning(
                    TraceComponent, 
                    "{0} invalid node task: {1}", 
                    owner_.TraceId, 
                    task.TaskType);

                return ErrorCodeValue::InvalidArgument;
            }

            NodeId nodeId;
            wstring nodeName = task.NodeName;
            auto error = this->GetNodeId(nodeName, nodeId);

            if (!error.IsSuccess())
            {
                return error;
            }

            nodesMap.insert(pair<NodeId, NodeDeactivationIntent::Enum>(nodeId, fmIntent));

        } // for each NodeTask

        request = move(nodesMap);

        return ErrorCodeValue::Success;
    }

    ProcessInfrastructureTaskContextAsyncOperation & owner_;
    vector<NodeTaskDescription> const & tasks_;
};

class ProcessInfrastructureTaskContextAsyncOperation::PreProcessNodesAsyncOperation : public ProcessNodesAsyncOperationBase
{
public:
    PreProcessNodesAsyncOperation(
        __in ProcessInfrastructureTaskContextAsyncOperation & owner,
        vector<NodeTaskDescription> const & tasks,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ProcessNodesAsyncOperationBase(owner, tasks, timeout, callback, parent)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<PreProcessNodesAsyncOperation>(operation)->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        // Don't start base, just pass the remaining time along

        owner_.WriteInfo(
            TraceComponent, 
            "{0} PreProcessing Nodes (StatusOnly={1}): {2}",
            owner_.TraceId,
            owner_.getStatusOnly_,
            tasks_);

        if (tasks_.empty())
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else if (owner_.getStatusOnly_)
        {
            this->GetDeactivationStatus(thisSPtr);
        }
        else
        {
            this->SendDeactivationRequest(thisSPtr);
        }
    }

private:

    void SendDeactivationRequest(AsyncOperationSPtr const & thisSPtr)
    {
        map<NodeId, NodeDeactivationIntent::Enum> request;

        auto error = this->CreateDeactivateNodesRequest(request);

        if (error.IsSuccess())
        {
            // Note that the FM will add a dummy node entry for nodes that
            // do no exist in its node cache in order to persist the node's deactivation
            // status.
            //
            auto operation = owner_.Client.BeginDeactivateNodesBatch(
                move(request),
                owner_.context_.Description.TaskId,
                owner_.ActivityId,
                this->RemainingTime,
                [this] (AsyncOperationSPtr const & operation) { this->OnDeactivationRequestComplete(operation, false); },
                thisSPtr);
            this->OnDeactivationRequestComplete(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void OnDeactivationRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = owner_.Client.EndDeactivateNodesBatch(operation);

        if (error.IsSuccess())
        {
            this->GetDeactivationStatus(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void GetDeactivationStatus(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.Client.BeginGetNodeDeactivationStatus(
            owner_.context_.Description.TaskId,
            owner_.ActivityId,
            this->RemainingTime,
            [this] (AsyncOperationSPtr const & operation) { this->OnDeactivationStatusComplete(operation, false); },
            thisSPtr);
        this->OnDeactivationStatusComplete(operation, true);
    }

    void OnDeactivationStatusComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        NodeDeactivationStatus::Enum status;
        auto error = owner_.Client.EndGetNodeDeactivationStatus(operation, status);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
        else if (status != NodeDeactivationStatus::DeactivationComplete)
        {
            // Once deactivation has succeeded, we only need to poll the FM for
            // status. This is only tracked in-memory. On failover, the CM will
            // send another deactivation request to kickstart the process, which
            // FM will handle indempotently.
            //
            owner_.getStatusOnly_ = true;
            this->TryComplete(thisSPtr, ErrorCodeValue::NodeDeactivationInProgress);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }
};

class ProcessInfrastructureTaskContextAsyncOperation::PostProcessNodesAsyncOperation : public ProcessNodesAsyncOperationBase
{
public:
    PostProcessNodesAsyncOperation(
        __in ProcessInfrastructureTaskContextAsyncOperation & owner,
        vector<NodeTaskDescription> const & tasks,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ProcessNodesAsyncOperationBase(owner, tasks, timeout, callback, parent)
        , pendingNodeStateRemovedRequests_(0)
        , error_(ErrorCodeValue::Success)
        , errorLock_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<PostProcessNodesAsyncOperation>(operation)->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        // Don't start base, just pass the remaining time along

        owner_.WriteInfo(
            TraceComponent, 
            "{0} PostProcessing Nodes: {1}",
            owner_.TraceId,
            tasks_);

        if (tasks_.empty())
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            map<wstring, NodeId> removedNodesMap;

            for (auto it = tasks_.begin(); it != tasks_.end(); ++it)
            {
                if (it->TaskType != NodeTask::Remove)
                {
                    continue;
                }

                NodeId nodeId;
                wstring nodeName = it->NodeName;
                auto error = this->GetNodeId(nodeName, nodeId);

                if (!error.IsSuccess())
                {
                    this->TryComplete(thisSPtr, error);
                    return;
                }

                removedNodesMap.insert(pair<wstring, NodeId>(nodeName, nodeId));
            }

            if (removedNodesMap.empty())
            {
                this->SendRemoveNodeDeactivationRequest(thisSPtr);
            }
            else
            {
                pendingNodeStateRemovedRequests_.store(static_cast<LONG>(removedNodesMap.size()));

                for (auto it=removedNodesMap.begin(); it!=removedNodesMap.end(); ++it)
                {
                    auto operation = owner_.Client.BeginNodeStateRemoved(
                        it->first,
                        it->second,
                        owner_.ActivityId,
                        this->RemainingTime,
                        [this] (AsyncOperationSPtr const & operation) { this->OnNodeStateRemovedComplete(operation, false); },
                        thisSPtr);
                    this->OnNodeStateRemovedComplete(operation, true);
                }
            }
        }
    }

private:

    void OnNodeStateRemovedComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = owner_.Client.EndNodeStateRemoved(operation);

        switch (error.ReadValue())
        {
        // After #1337444 is resolved, a node entry should have been created by the FM
        // already. But this can still happen if the FM loses data.
        //
        case ErrorCodeValue::NodeNotFound:
        // Azure may not actually stop a VM when it's supposed to. Avoid blocking
        // the MR job in these cases.
        //
        case ErrorCodeValue::NodeIsUp:
            error = ErrorCodeValue::Success;
            break;
        }

        {
            AcquireExclusiveLock lock(errorLock_);

            error_ = ErrorCode::FirstError(error_, error);
        }

        if (--pendingNodeStateRemovedRequests_ == 0)
        {
            if (error_.IsSuccess())
            {
                this->SendRemoveNodeDeactivationRequest(thisSPtr);
            }
            else
            {
                this->TryComplete(thisSPtr, error_);
            }
        }
    }

    void SendRemoveNodeDeactivationRequest(AsyncOperationSPtr const & thisSPtr)
    {
        // NodeStateRemoved does not remove the node from the NodeCache, which
        // still contains a deactivated entry. Re-activate the node so it can
        // re-join the ring if needed.
        //
        auto operation = owner_.Client.BeginRemoveNodeDeactivations(
            owner_.context_.Description.TaskId,
            owner_.ActivityId,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnRemoveNodeDeactivationComplete(operation, false); },
            thisSPtr);
        this->OnRemoveNodeDeactivationComplete(operation, true);
    }

    void OnRemoveNodeDeactivationComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = owner_.Client.EndRemoveNodeDeactivations(operation);

        this->TryComplete(operation->Parent, error);
    }

private:
    Common::atomic_long pendingNodeStateRemovedRequests_;
    ErrorCode error_;
    ExclusiveLock errorLock_;
};

class ProcessInfrastructureTaskContextAsyncOperation::HealthCheckAsyncOperation : public ProcessNodesAsyncOperationBase
{
public:
    HealthCheckAsyncOperation(
        __in ProcessInfrastructureTaskContextAsyncOperation & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ProcessNodesAsyncOperationBase(owner, vector<NodeTaskDescription>(), TimeSpan::MaxValue, callback, parent)
        , healthCheckPerformed_(false)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<HealthCheckAsyncOperation>(operation)->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.WriteInfo(
            TraceComponent, 
            "{0} cluster health Check: elapsed time = persisted:{1} + stopwatch:{2}",
            owner_.TraceId,
            owner_.context_.HealthCheckElapsedTime,
            owner_.healthCheckStopwatch_.Elapsed);

        owner_.context_.UpdateHealthCheckElapsedTime(owner_.healthCheckStopwatch_.Elapsed);

        this->UpdateAndCommitInfrastructureTaskContext(thisSPtr);
    }

private:
    TimeSpan GetTotalElapsedTime()
    {
        return owner_.context_.HealthCheckElapsedTime.AddWithMaxAndMinValueCheck(owner_.healthCheckStopwatch_.Elapsed);
    }

    TimeSpan GetHealthCheckRetryTimeout()
    {
        auto waitDuration = ManagementConfig::GetConfig().InfrastructureTaskHealthCheckWaitDuration;
        auto retryTimeout = ManagementConfig::GetConfig().InfrastructureTaskHealthCheckRetryTimeout;

        return waitDuration.AddWithMaxAndMinValueCheck(retryTimeout);
    }

    TimeSpan GetHealthStableDuration()
    {
        auto waitDuration = ManagementConfig::GetConfig().InfrastructureTaskHealthCheckWaitDuration;
        auto stableDuration = ManagementConfig::GetConfig().InfrastructureTaskHealthCheckStableDuration;

        return waitDuration.AddWithMaxAndMinValueCheck(stableDuration);
    }

    void UpdateAndCommitInfrastructureTaskContext(AsyncOperationSPtr const & thisSPtr)
    {
        Stopwatch updateStopwatch;
        updateStopwatch.Start();

        auto storeTx = owner_.CreateTransaction();

        auto error = storeTx.Update(owner_.context_);

        if (error.IsSuccess())
        {
            auto operation = StoreTransaction::BeginCommit(
                move(storeTx),
                owner_.context_,
                [this, updateStopwatch] (AsyncOperationSPtr const & operation) { this->OnCommitComplete(updateStopwatch, operation, false); },
                thisSPtr);
            this->OnCommitComplete(updateStopwatch, operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void OnCommitComplete(Stopwatch updateStopwatch, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = StoreTransaction::EndCommit(operation);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            owner_.healthCheckStopwatch_ = updateStopwatch;

            auto totalElapsed = this->GetTotalElapsedTime();
            auto waitDuration = ManagementConfig::GetConfig().InfrastructureTaskHealthCheckWaitDuration;

            if (totalElapsed >= waitDuration)
            {
                this->PerformHealthCheck(thisSPtr);
            }
            else
            {
                owner_.WriteInfo(
                    TraceComponent, 
                    "{0} health check pending: {1} < {2}",
                    owner_.TraceId,
                    totalElapsed,
                    waitDuration);

                this->TryComplete(thisSPtr, ErrorCodeValue::HealthCheckPending);
            }
        }
    }

    void PerformHealthCheck(AsyncOperationSPtr const & thisSPtr)
    {
        bool isHealthy = owner_.context_.LastHealthCheckResult;

        if (!healthCheckPerformed_)
        {
            HealthManager::IHealthAggregatorSPtr healthAggregator;
            auto error = owner_.Replica.GetHealthAggregator(healthAggregator);
            if (!error.IsSuccess())
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} error getting health aggregator: {1}",
                    owner_.TraceId,
                    error);
                this->TryComplete(thisSPtr, error);
                return;
            }

            vector<HealthEvaluation> unhealthyEvaluations;
            // TODO: 4240470: pass in correct baseline and upgrade domain list
            HealthManager::ClusterUpgradeStateSnapshot baseline;
            vector<wstring> applicationsWithoutAppType;
            error = healthAggregator->IsClusterHealthy(
                owner_.ActivityId,
                vector<wstring>(),
                baseline,
                isHealthy,
                unhealthyEvaluations,
                applicationsWithoutAppType);

            if (error.IsError(ErrorCodeValue::ApplicationTypeNotFound))
            {
                owner_.Replica.ReportApplicationsType(TraceComponent, owner_.ActivityId, thisSPtr, applicationsWithoutAppType, this->RemainingTime);
            }

            if (!error.IsSuccess())
            {
                owner_.WriteInfo(
                    TraceComponent, 
                    "{0} Health Manager not ready for health check: {1}",
                    owner_.TraceId,
                    error);
                this->TryComplete(thisSPtr, ErrorCodeValue::HealthCheckPending);
                return;
            }

            healthCheckPerformed_ = true;
        }

        if (owner_.context_.LastHealthCheckResult != isHealthy)
        {
            owner_.WriteInfo(
                TraceComponent, 
                "{0} health check result changed: previous={1} current={2}",
                owner_.TraceId,
                owner_.context_.LastHealthCheckResult,
                isHealthy);

            // effectively sets both stable duration and retry timeout back to zero
            //
            owner_.context_.SetHealthCheckElapsedTime(ManagementConfig::GetConfig().InfrastructureTaskHealthCheckWaitDuration);

            owner_.context_.SetLastHealthCheckResult(isHealthy);

            // Persist the new health check result and update elapsed time.
            // Workflow will come back into PerformHealthCheck() after
            // successful replication, but the check with HM will not actually
            // be done again (healthCheckPerformed_ is set).
            //
            // Instead we will proceed to evaluate the new health check result
            // that we already have saved in the context.
            //
            this->UpdateAndCommitInfrastructureTaskContext(thisSPtr);

            return;
        }

        auto totalElapsed = this->GetTotalElapsedTime();

        if (isHealthy)
        {
            auto healthCheckDuration = this->GetHealthStableDuration(); 

            if (totalElapsed < healthCheckDuration)
            {
                owner_.WriteInfo(
                    TraceComponent, 
                    "{0} passed cluster health check - waiting for stable duration: {1} < {2}",
                    owner_.TraceId,
                    totalElapsed,
                    healthCheckDuration);

                this->TryComplete(thisSPtr, ErrorCodeValue::HealthCheckPending);
            }
            else
            {
                owner_.WriteInfo(
                    TraceComponent, 
                    "{0} cluster health check succeeded: {1} >= {2}",
                    owner_.TraceId,
                    totalElapsed,
                    healthCheckDuration);

                this->ReportFinishTaskSuccess(thisSPtr);
            }
        }
        else
        {
            auto healthCheckTimeout = this->GetHealthCheckRetryTimeout();

            if (totalElapsed < healthCheckTimeout)
            {
                owner_.WriteInfo(
                    TraceComponent, 
                    "{0} failed cluster health check - scheduling retry: {1} < {2}",
                    owner_.TraceId,
                    totalElapsed,
                    healthCheckTimeout);

                this->TryComplete(thisSPtr, ErrorCodeValue::HealthCheckPending);
            }
            else
            {
                owner_.WriteWarning(
                    TraceComponent, 
                    "{0} failed cluster health check: {1} >= {2}",
                    owner_.TraceId,
                    totalElapsed,
                    healthCheckTimeout);

                this->TryComplete(thisSPtr, ErrorCodeValue::InvestigationRequired);
            }
        }
    }

    void ReportFinishTaskSuccess(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.Client.BeginReportFinishTaskSuccess(
            owner_.context_.Description.Task,
            owner_.context_.Description.SourcePartitionId,
            owner_.ActivityId,
            owner_.context_.CommunicationTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnReportFinishTaskSuccessComplete(operation, false); },
            thisSPtr);
        this->OnReportFinishTaskSuccessComplete(operation, true);
    }

    void OnReportFinishTaskSuccessComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.Client.EndReportFinishTaskSuccess(operation);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} ReportFinishTaskSuccess failed with {1}",
                owner_.TraceId,
                error);

            error = ErrorCodeValue::OperationCanceled;
        }

        this->TryComplete(operation->Parent, error);
    }

private:

    bool healthCheckPerformed_;
};

//
// ProcessInfrastructureTaskContextAsyncOperation
//

ProcessInfrastructureTaskContextAsyncOperation::ProcessInfrastructureTaskContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in InfrastructureTaskContext & context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessRolloutContextAsyncOperationBase(
        rolloutManager, 
        context.ReplicaActivityId, 
        timeout,
        callback, 
        root)
    , context_(context)
    , timerSPtr_()
    , timerLock_()
    , healthCheckStopwatch_()
    , getStatusOnly_(false)
{
}

void ProcessInfrastructureTaskContextAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    // Process infrastructure task in background
    //
    context_.CompleteClientRequest();

    this->ProcessTask(thisSPtr);
}

void ProcessInfrastructureTaskContextAsyncOperation::ScheduleProcessTask(AsyncOperationSPtr const & thisSPtr, TimeSpan const delay)
{
    if (!this->IsRolloutManagerActive())
    {
        WriteInfo(
            TraceComponent, 
            "{0} rollout manager no longer active",
            this->TraceId);

        this->TryComplete(thisSPtr, ErrorCodeValue::CMRequestAborted);
    }
    else
    {
        AcquireExclusiveLock lock(timerLock_);
        
        if (!this->InternalIsCompleted)
        {
            timerSPtr_ = Timer::Create(TimerTagDefault, [this, thisSPtr] (TimerSPtr const & timer) 
            { 
                timer->Cancel();
                this->ProcessTask(thisSPtr);
            },
            true); // allow concurrency
            timerSPtr_->Change(delay);
        }
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::ScheduleProcessTaskRetry(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    if (this->IsReconfiguration(error))
    {
        this->TryComplete(thisSPtr, error);
    }
    else if (this->IsRetryable(error))
    {
        WriteInfo(
            TraceComponent, 
            "{0} scheduling retry on {1}",
            this->TraceId,
            error);

        this->ScheduleProcessTask(thisSPtr, ManagementConfig::GetConfig().InfrastructureTaskProcessingInterval);
    }
    else
    {
        if (context_.Description.DoAsyncAck)
        {
            this->ReportTaskFailure(thisSPtr, error);
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} skipping async fail",
                this->TraceId);

            this->TryComplete(thisSPtr, error);
        }
    }
}

ErrorCode ProcessInfrastructureTaskContextAsyncOperation::RefreshContext()
{
    auto storeTx = this->CreateTransaction();

    return context_.Refresh(storeTx);
}

void ProcessInfrastructureTaskContextAsyncOperation::ProcessTask(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        TraceComponent, 
        "{0} ({1}) '{2}'",
        this->TraceId,
        context_.State,
        context_.Description.Task);

    ErrorCode error = this->RefreshContext();

    if (!error.IsSuccess())
    {
        this->ScheduleProcessTaskRetry(thisSPtr, error);
        return;
    }

    switch (context_.State)
    {
    case InfrastructureTaskState::PreProcessing:
        this->PreProcessNodes(thisSPtr);
        break;

    case InfrastructureTaskState::PreAckPending:
        this->ReportPreProcessSuccess(thisSPtr);
        break;

    case InfrastructureTaskState::PreAcked:
        this->OnPreAcked(thisSPtr);
        break;

    case InfrastructureTaskState::PostProcessing:
        this->PostProcessNodes(thisSPtr);
        break;

    case InfrastructureTaskState::PostAckPending:
        this->PerformClusterHealthCheck(thisSPtr);
        break;

    case InfrastructureTaskState::PostAcked:
        this->OnPostAcked(thisSPtr);
        break;

    default:
        this->Replica.TestAssertAndTransientFault(wformatString("{0} invalid infrastructure task state: {1}", this->TraceId, context_.State));

        // Stop processing since the current state is corrupt. A new incoming task will be
        // allowed to start.
        //
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        break;
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::PreProcessNodes(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<PreProcessNodesAsyncOperation>(
        *this,
        context_.Description.NodeTasks,
        context_.CommunicationTimeout,
        [this](AsyncOperationSPtr const & operation) { this->OnPreProcessNodesComplete(operation, false); },
        thisSPtr);
    this->OnPreProcessNodesComplete(operation, true);
}

void ProcessInfrastructureTaskContextAsyncOperation::OnPreProcessNodesComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ErrorCode error = PreProcessNodesAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        context_.UpdateState(InfrastructureTaskState::PreAckPending);

        this->UpdateContext(thisSPtr);
    }
    else
    {
        this->ScheduleProcessTaskRetry(thisSPtr, error);
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::ReportPreProcessSuccess(AsyncOperationSPtr const & thisSPtr)
{
    if (context_.Description.DoAsyncAck)
    {
        auto operation = this->Client.BeginReportStartTaskSuccess(
            context_.Description.Task,
            context_.Description.SourcePartitionId,
            this->ActivityId,
            context_.CommunicationTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnReportPreProcessSuccessComplete(operation, false); },
            thisSPtr);
        this->OnReportPreProcessSuccessComplete(operation, true);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} skipping ack",
            this->TraceId);

        context_.UpdateState(InfrastructureTaskState::PreAcked);

        this->UpdateContext(thisSPtr);
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::OnReportPreProcessSuccessComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ErrorCode error = this->Client.EndReportStartTaskSuccess(operation);

    if (error.IsSuccess())
    {
        context_.UpdateState(InfrastructureTaskState::PreAcked);

        this->UpdateContext(thisSPtr);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} ReportStartTaskSuccess failed with {1}",
            this->TraceId,
            error);

        // For all notifications going to the Infrastructure Service, do not depend
        // on the IS to return a known error code since it can just throw arbitrary
        // exceptions that will convert to non-retryable HRESULTs. Always retry on
        // errors communicating with the IS instead of failing the operation context.
        //
        this->ScheduleProcessTaskRetry(thisSPtr, ErrorCodeValue::OperationCanceled);
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::OnPreAcked(AsyncOperationSPtr const & thisSPtr)
{
    if (context_.IsComplete)
    {
        WriteInfo(
            TraceComponent, 
            "{0} PreAcked complete",
            this->TraceId);

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        auto storeTx = this->CreateTransaction();

        ErrorCode error = context_.Complete(storeTx);

        if (error.IsSuccess())
        {
            this->Commit(thisSPtr, move(storeTx));
        }
        else
        {
            this->ScheduleProcessTaskRetry(thisSPtr, error);
        }
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::PostProcessNodes(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<PostProcessNodesAsyncOperation>(
        *this,
        context_.Description.NodeTasks,
        context_.CommunicationTimeout,
        [this](AsyncOperationSPtr const & operation) { this->OnPostProcessNodesComplete(operation, false); },
        thisSPtr);
    this->OnPostProcessNodesComplete(operation, true);
}

void ProcessInfrastructureTaskContextAsyncOperation::OnPostProcessNodesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ErrorCode error = PostProcessNodesAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        context_.UpdateState(InfrastructureTaskState::PostAckPending);

        context_.SetHealthCheckElapsedTime(TimeSpan::Zero);

        healthCheckStopwatch_.Restart();

        this->UpdateContext(thisSPtr);
    }
    else
    {
        this->ScheduleProcessTaskRetry(thisSPtr, error);
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::PerformClusterHealthCheck(AsyncOperationSPtr const & thisSPtr)
{
    // Skip health check and post ack if we are reversing work that we may have done during PreProcess
    //
    if (context_.IsInterrupted || !context_.Description.DoAsyncAck)
    {
        WriteInfo(
            TraceComponent, 
            "{0} skipping health check",
            this->TraceId);

        context_.UpdateState(InfrastructureTaskState::PostAcked);

        this->UpdateContext(thisSPtr);
    }
    else
    {
        auto operation = AsyncOperation::CreateAndStart<HealthCheckAsyncOperation>(
            *this,
            [this](AsyncOperationSPtr const & operation) { this->OnPerformClusterHealthCheckComplete(operation, false); },
            thisSPtr);
        this->OnPerformClusterHealthCheckComplete(operation, true);
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::OnPerformClusterHealthCheckComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ErrorCode error = HealthCheckAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        context_.UpdateState(InfrastructureTaskState::PostAcked);

        this->UpdateContext(thisSPtr);
    }
    else
    {
        this->ScheduleProcessTaskRetry(thisSPtr, error);
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::OnPostAcked(AsyncOperationSPtr const & thisSPtr)
{
    if (context_.IsComplete)
    {
        WriteInfo(
            TraceComponent, 
            "{0} PostAcked complete",
            this->TraceId);

        // Do not delete completed infrastructure task contexts.
        // Leave them for auditing purposes.
        //
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        auto storeTx = this->CreateTransaction();

        ErrorCode error = context_.Complete(storeTx);

        if (error.IsSuccess())
        {
            this->Commit(thisSPtr, move(storeTx));
        }
        else
        {
            this->ScheduleProcessTaskRetry(thisSPtr, error);
        }
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::UpdateContext(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    ErrorCode error = storeTx.Update(context_);

    if (error.IsSuccess())
    {
        this->Commit(thisSPtr, move(storeTx));
    }
    else
    {
        this->ScheduleProcessTaskRetry(thisSPtr, error);
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::Commit(AsyncOperationSPtr const & thisSPtr, StoreTransaction && storeTx)
{
    auto operation = StoreTransaction::BeginCommit(
        move(storeTx),
        context_,
        [this] (AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
        thisSPtr);
    this->OnCommitComplete(operation, true);
}

void ProcessInfrastructureTaskContextAsyncOperation::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    if (error.IsSuccess())
    {
        // Process next state immediately
        this->ScheduleProcessTask(operation->Parent, TimeSpan::Zero);
    }
    else
    {
        this->ScheduleProcessTaskRetry(operation->Parent, error);
    }
}

void ProcessInfrastructureTaskContextAsyncOperation::ReportTaskFailure(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    WriteInfo(
        TraceComponent, 
        "{0} reporting task failure: {1}",
        this->TraceId,
        error);

    auto operation = this->Client.BeginReportTaskFailure(
        context_.Description.Task,
        context_.Description.SourcePartitionId,
        this->ActivityId,
        context_.CommunicationTimeout,
        [this, error](AsyncOperationSPtr const & operation) { this->OnReportTaskFailureComplete(operation, error, false); },
        thisSPtr);
    this->OnReportTaskFailureComplete(operation, error, true);
}

void ProcessInfrastructureTaskContextAsyncOperation::OnReportTaskFailureComplete(
    AsyncOperationSPtr const & operation,
    ErrorCode const & taskError,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ErrorCode error = this->Client.EndReportTaskFailure(operation);

    if (error.IsSuccess())
    {
        if (this->IsHealthCheckFailure(taskError))
        {
            // The IS has been notified of the health check failure and given an opportunity to act.
            // Unblock the IS by "completing" PostProcessing. Ideally, the IS should be
            // handling failed tasks and not continue attempting to finish them.
            //
            context_.UpdateState(InfrastructureTaskState::PostAcked);

            this->UpdateContext(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, taskError);
        }
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} ReportFinishTaskFailure failed with {1}",
            this->TraceId,
            error);

        this->ScheduleProcessTaskRetry(thisSPtr, ErrorCodeValue::OperationCanceled);
    }
}

bool ProcessInfrastructureTaskContextAsyncOperation::IsRetryable(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::OperationFailed:
            return false;

        default:
            return (this->IsHealthCheckFailure(error) ? false : !this->IsReconfiguration(error));
    }
}

bool ProcessInfrastructureTaskContextAsyncOperation::IsHealthCheckFailure(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        // Report failure to the infrastructure service to quickly escalate
        // situations potentially requiring human investigation rather
        // than waiting for a timeout.
        //  
        // If reporting failure does not succeed, we will just retry the 
        // operation that caused the failure, which will likely (but not 
        // necessarily) fail again.
        //
        case ErrorCodeValue::InvestigationRequired:
            return true;

        // Don't retry within this context, but the RolloutManager
        // may still retry at its level (schedule with context processing interval)
        //
        default:
            return false;
    }
}
