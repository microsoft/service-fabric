// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Hosting2;
using namespace Management::RepairManager;

StringLiteral const TraceType_RestartManagerClient("NodeRestartManagerClient");

class NodeRestartManagerClient::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation(NodeRestartManagerClient & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode OpenAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "Opening NodeRestartManagerClient: {0}",
            timeoutHelper_.GetRemainingTime());

        owner_.processId_ = ::GetCurrentProcessId();

        // Step 1: Open IPC Client
        OpenIPC(thisSPtr);
    }

private:
    void OpenIPC(AsyncOperationSPtr const & thisSPtr)
    {
        if(owner_.ipcClient_)
        {
            SecuritySettings ipcClientSecuritySettings;
            auto error = SecuritySettings::CreateNegotiateClient(SecurityConfig::GetConfig().FabricHostSpn, ipcClientSecuritySettings);
            owner_.Client.SecuritySettings = ipcClientSecuritySettings;

            WriteTrace(
                error.ToLogLevel(),
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "SecuritySettings.CreateNegotiate: ErrorCode={0}",
                error);

            if (!error.IsSuccess())
            {
                this->TryComplete(thisSPtr, error);
                return;
            }

            owner_.RegisterIpcMessageHandler();
            error = owner_.Client.Open();
            WriteTrace(
                error.ToLogLevel(),
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "IPC Client Open: ErrorCode={0}",
                error);
            if(!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
        }

        RegisterClientWithActivator(thisSPtr);
    }

    void RegisterClientWithActivator(AsyncOperationSPtr thisSPtr)
    {
        MessageUPtr request = CreateRegisterClientRequest();
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteInfo(
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "Register FabricActivatorClient timeout: {0}",
            timeout);

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnRegistrationCompleted(operation, false); },
            thisSPtr);
        OnRegistrationCompleted(operation, true);
    }

    void OnRegistrationCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "End(RegisterFabricActivatorClient): ErrorCode={0}",
            error);
        TryComplete(operation->Parent, error);
        return;
    }

    MessageUPtr CreateRegisterClientRequest()
    {
        RegisterFabricActivatorClientRequest registerRequest(owner_.processId_, owner_.nodeId_);
        MessageUPtr request = make_unique<Message>(registerRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::RestartManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::RegisterFabricActivatorClient));

        WriteNoise(TraceType_RestartManagerClient, owner_.Root.TraceId, "RegisterFabricActivatorClient: Message={0}, Body={1}", *request, registerRequest);
        return move(request);
    }

private:
    TimeoutHelper timeoutHelper_;
    NodeRestartManagerClient & owner_;
};

class NodeRestartManagerClient::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        __in NodeRestartManagerClient & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode CloseAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if(owner_.ipcClient_)
        {
            UnregisterFabricActivatorClient(thisSPtr);
        }
        else
        {
            WriteNoise(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "NodeRestartManagerClient: closed");
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

private:
    void UnregisterFabricActivatorClient(AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr request = owner_.CreateUnregisterClientRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ OnUnregisterCompleted(operation, false); },
            thisSPtr);
        OnUnregisterCompleted(operation, true);
    }

    void OnUnregisterCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "End(RegisterFabricActivatorClient): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        owner_.UnregisterIpcMessageHandler();
        error = owner_.ipcClient_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "FabricActivatorClient: Failed to close ipcclient. Error {0}",
                error);
        }

        TryComplete(operation->Parent, error);
    }

private:
    TimeoutHelper timeoutHelper_;
    NodeRestartManagerClient & owner_;
};

// Disable Node Operation
class NodeRestartManagerClient::DisableNodeRequestAsyncOperation : public AsyncOperation
{
public:
    DisableNodeRequestAsyncOperation(
        __in NodeRestartManagerClient & owner,
        wstring const & disableReason,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , disableReason_(disableReason)
        , lock_()
        , completedOrCancelled_(false)
    {
    }

    static ErrorCode DisableNodeRequestAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DisableNodeRequestAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->GetCurrentNodeActiveRepairTask(thisSPtr);
    }

    void OnCompleted()
    {
        AcquireExclusiveLock rwlock(lock_);
        this->completedOrCancelled_ = true;
        if (pollingTimer_)
        {
            pollingTimer_->Cancel();
        }

        AsyncOperation::OnCompleted();
    }

private:
    void ScheduleTimer(AsyncOperationSPtr const & thisSPtr, TimeSpan dueTime)
    {
        AcquireExclusiveLock rwlock(lock_);
        if (!this->completedOrCancelled_)
        {
            if (!pollingTimer_)
            {
                pollingTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const&) { PollingTimerCallback(thisSPtr); });
            }

            pollingTimer_->Change(dueTime);
        }
    }

    void GetCurrentNodeActiveRepairTask(AsyncOperationSPtr const & thisSPtr)
    {
        ClusterRepairScopeIdentifier scope;
        AsyncOperationSPtr operation = owner_.hosting_.RepairManagementClient->BeginGetRepairTaskList(
            scope,
            L"",
            RepairTaskState::Claimed | RepairTaskState::Executing | RepairTaskState::Approved | RepairTaskState::Preparing,
            owner_.executor_,
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const& operation) { this->OnGettingCurrentNodeActiveRepairTask(operation, false); },
            thisSPtr);

        this->OnGettingCurrentNodeActiveRepairTask(operation, true);
    }

    void OnGettingCurrentNodeActiveRepairTask(AsyncOperationSPtr operation, bool expectedCompleteSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        vector<Management::RepairManager::RepairTask> repairTaskList;
        auto error = owner_.hosting_.RepairManagementClient->EndGetRepairTaskList(operation, repairTaskList);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "OnGettingCurrentNodeActiveRepairTask: ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            this->ScheduleTimer(operation->Parent, TimeSpan::FromSeconds(Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds));
            return;
        }

        if (repairTaskList.size() == 0)
        {
            CreateRepairTaskForCurrentNode(operation->Parent);
        }
        else
        {
            int approvedTaskIndex = -1;
            int executingTaskIndex = -1;
            int index = -1;
            for (std::vector<RepairTask>::iterator it = repairTaskList.begin(); it != repairTaskList.end(); ++it)
            {
                index++;
                if (it->State == RepairTaskState::Approved)
                {
                    approvedTaskIndex = index;
                }
                else if (it->State == RepairTaskState::Executing)
                {
                    executingTaskIndex = index;
                    break;
                }
            }

            if (executingTaskIndex != -1)
            {
                SendNodeDisabledNotification(operation->Parent, repairTaskList[executingTaskIndex].TaskId);
            }
            else if (approvedTaskIndex != -1)
            {
                MoveCurrentNodeTaskToExecutingState(operation->Parent, repairTaskList[approvedTaskIndex]);
            }
            else
            {
                ScheduleTimer(operation->Parent, TimeSpan::FromSeconds(Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds));
            }
        }
    }

    void CreateRepairTaskForCurrentNode(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "CreateRepairTaskForCurrentNode: No active task. So create one."
            );

        // Add current node as the target
        const int size = 1;
        LPCWSTR targets[size];
        wstring nodeName = owner_.hosting_.FabricNodeConfigObj.InstanceNameEntry.GetValue();
        targets[0] = nodeName.c_str();

        FABRIC_STRING_LIST targetsFabricStringList;
        targetsFabricStringList.Count = static_cast<ULONG>(size);
        targetsFabricStringList.Items = targets;

        FABRIC_REPAIR_TARGET_DESCRIPTION target;
        target.Kind = FABRIC_REPAIR_TARGET_KIND_NODE;
        target.Value = &targetsFabricStringList;

        // Scope
        FABRIC_REPAIR_SCOPE_IDENTIFIER scope;
        scope.Kind = FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER;
        scope.Value = NULL;

        // Executor
        FABRIC_REPAIR_EXECUTOR_STATE state = { 0 };
        state.Executor = owner_.executor_.c_str();
        state.ExecutorData = owner_.upgradeDomainId_.c_str();

        FABRIC_REPAIR_TASK task;
        SecureZeroMemory(&task, sizeof(task));

        task.Scope = &scope;
        task.State = FABRIC_REPAIR_TASK_STATE_CLAIMED;
        task.Action = this->disableReason_.c_str();

        task.ExecutorState = &state;
        task.TaskId = (L"RestartManager." + Guid::NewGuid().ToString()).c_str();
        task.Target = &target;
        task.Description = L"Reboot";
        RepairTask repairTask;
        ErrorCode error = repairTask.FromPublicApi(task);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "Could not create repair task: TaskId:{0} ErrorCode={1}",
                task.TaskId,
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        AsyncOperationSPtr operation = owner_.hosting_.RepairManagementClient->BeginCreateRepairTask(
            repairTask, 
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const& operation) { this->OnCurrentNodeRepairTaskCreated(operation, false); },
            thisSPtr);

        this->OnCurrentNodeRepairTaskCreated(operation, true);
    }

    void OnCurrentNodeRepairTaskCreated(AsyncOperationSPtr const& operation, bool expectedCompleteSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        int64 commitVersion;
        ErrorCode error = owner_.hosting_.RepairManagementClient->EndCreateRepairTask(operation, commitVersion);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "OnCurrentNodeRepairTaskCreated: CreateRepairTaskFailed with error: ErrorCode={0}",
                error);
            ScheduleTimer(operation->Parent, TimeSpan::FromSeconds(Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds));
            return;
        }

        GetCurrentNodeActiveRepairTask(operation->Parent);
    }
    
    void MoveCurrentNodeTaskToExecutingState(AsyncOperationSPtr const & thisSPtr, RepairTask & repairTask)
    {
        FABRIC_REPAIR_TASK task;
        ScopedHeap heap;
        repairTask.ToPublicApi(heap, task);
        task.Reserved = NULL;
        task.State = FABRIC_REPAIR_TASK_STATE_EXECUTING;
        RepairTask repairTask1;
        ErrorCode error = repairTask1.FromPublicApi(task);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "MoveCurrentNodeTaskToExecutingState: ErrorCode={0}",
                error);
            TryComplete(thisSPtr, error);
        }

        AsyncOperationSPtr operation = owner_.hosting_.RepairManagementClient->BeginUpdateRepairExecutionState(
            repairTask1,
            HostingConfig::GetConfig().RequestTimeout,
            [this, repairTask1](AsyncOperationSPtr const& operation) { this->OnTaskMovedToExecutingState(operation, false, repairTask1.TaskId); },
            thisSPtr);

        this->OnTaskMovedToExecutingState(operation, true, repairTask1.TaskId);
    }

    void OnTaskMovedToExecutingState(AsyncOperationSPtr const& operation, bool expectedCompleteSynchronously, wstring const & taskId)
    {
        if (operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        int64 commitVersion;
        auto error = owner_.hosting_.RepairManagementClient->EndUpdateRepairExecutionState(operation, commitVersion);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "OnTaskMovedToExecutingState: Prepare task of Id:{0} failed with ErrorCode={1}",
            taskId,
            error);
        if (error.IsSuccess())
        {
            this->SendNodeDisabledNotification(operation->Parent, taskId);
        }
        else
        {
            this->ScheduleTimer(operation->Parent, TimeSpan::FromSeconds(Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds));
        }
    }

    void SendNodeDisabledNotification(AsyncOperationSPtr const &thisSPtr, std::wstring const &taskId)
    {
        NodeDisabledNotification disabledNotification(taskId);
        MessageUPtr request = make_unique<Message>(disabledNotification);
        request->Headers.Add(Transport::ActorHeader(Actor::RestartManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::NodeDisabledNotification));

        WriteNoise(TraceType_RestartManagerClient, owner_.Root.TraceId, "SendNodeDisabledNotification: Message={0}, Body={1}", *request, disabledNotification);

        auto operation = owner_.Client.BeginRequest(
            move(request),
            TimeSpan::FromMinutes(1),
            [this](AsyncOperationSPtr const & operation){ OnNodeDisabledNotification(operation, false); },
            thisSPtr);
        OnNodeDisabledNotification(operation, true);
    }

    void OnNodeDisabledNotification(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "End(OnNodeDisabledNotification): ErrorCode={0}",
                error);

            this->ScheduleTimer(operation->Parent, TimeSpan::FromSeconds(Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds));
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }

    void PollingTimerCallback(AsyncOperationSPtr const & thisSPtr)
    {
        GetCurrentNodeActiveRepairTask(thisSPtr);
    }

private:
    NodeRestartManagerClient & owner_;
    wstring disableReason_;
    Common::TimerSPtr pollingTimer_;
    Common::RwLock lock_;
    bool completedOrCancelled_;
};

// Enable node operation
class NodeRestartManagerClient::EnableNodeRequestAsyncOperation : public AsyncOperation
{
public:
    EnableNodeRequestAsyncOperation(
        __in NodeRestartManagerClient & owner,
        wstring const & disableReason,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , disableReason_(disableReason)
        , pollingTimer_()
        , lock_()
        , completedOrCancelled_(false)
    {
    }

    static ErrorCode EnableNodeRequestAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<EnableNodeRequestAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        GetCurrentNodeTask(thisSPtr);
    }

    void OnCancel()
    {
        CancelTimer();
        AsyncOperation::OnCancel();
    }

    void OnComleted()
    {
        CancelTimer();
        AsyncOperation::OnCompleted();
    }

private:
    void CancelTimer()
    {
        AcquireWriteLock writeLock(lock_);
        this->completedOrCancelled_ = true;
        if (pollingTimer_)
        {
            pollingTimer_->Cancel();
        }
    }

    void ScheduleTimer(AsyncOperationSPtr const & thisSPtr, TimeSpan dueTime)
    {
        AcquireWriteLock writeLock(lock_);
        if (!this->completedOrCancelled_)
        {
            if (!pollingTimer_)
            {
                pollingTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const&) { PollingTimerCallback(thisSPtr); });
            }

            pollingTimer_->Change(dueTime);
        }
    }

    void GetCurrentNodeTask(AsyncOperationSPtr const& thisSPtr)
    {
        ClusterRepairScopeIdentifier scope;
        AsyncOperationSPtr operation = owner_.hosting_.RepairManagementClient->BeginGetRepairTaskList(
            scope,
            L"",
            RepairTaskState::Claimed | RepairTaskState::Preparing | RepairTaskState::Approved | RepairTaskState::Executing | RepairTaskState::Restoring | RepairTaskState::Completed,
            owner_.executor_,
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const& operation) { this->OnGettingCurrentTaskState(operation, false); },
            thisSPtr);
        this->OnGettingCurrentTaskState(operation, true);
    }

    void OnGettingCurrentTaskState(AsyncOperationSPtr const& operation, bool expectedCompleteSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        vector<Management::RepairManager::RepairTask> repairTaskList;
        ErrorCode error = owner_.hosting_.RepairManagementClient->EndGetRepairTaskList(operation, repairTaskList);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "OnGettingCurrentTaskState: ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            this->ScheduleTimer(operation->Parent, TimeSpan::FromSeconds(Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds));
            return;
        }

        if (repairTaskList.size() != 0)
        {
            bool anyTaskInNotCompleteState = false;
            for (std::vector<RepairTask>::iterator it = repairTaskList.begin(); it != repairTaskList.end(); ++it)
            {
                if (it->State != RepairTaskState::Completed)
                {
                    anyTaskInNotCompleteState = true;
                }

                if (it->State == RepairTaskState::Claimed || it->State == RepairTaskState::Preparing)
                {
                    CancelRepairTask(operation->Parent, *it);
                }
                else if (it->State == RepairTaskState::Approved || it->State == RepairTaskState::Executing)
                {
                    MoveTaskToRestoringState(operation->Parent, *it);
                }
            }

            if (anyTaskInNotCompleteState)
            {
                // Check after sometime again
                this->ScheduleTimer(operation->Parent, TimeSpan::FromSeconds(Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds));
            }
            else
            {
                SendNodeEnabledNotification(operation->Parent);
            }
        }
        else
        {
            SendNodeEnabledNotification(operation->Parent);
        }
    }

    void CancelRepairTask(AsyncOperationSPtr const& thisSPtr, RepairTask const & repairTask)
    {
        FABRIC_REPAIR_CANCEL_DESCRIPTION description;
        description.RepairTaskId = repairTask.TaskId.c_str();
        description.RequestAbort = TRUE;
        description.Version = repairTask.Version;
        FABRIC_REPAIR_SCOPE_IDENTIFIER scope;
        if (repairTask.Scope->Kind == RepairScopeIdentifierKind::Cluster)
        {
            scope.Kind = FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER;
        }
        else
        {
            scope.Kind = FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_INVALID;
        }

        scope.Value = NULL;
        description.Scope = &scope;
        CancelRepairTaskMessageBody messageBody;
        ErrorCode error = messageBody.FromPublicApi(description);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "CancelRepairTask: Could not create repair task: TaskId:{0} ErrorCode={1}",
                repairTask.TaskId,
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        auto currentOperation = owner_.hosting_.RepairManagementClient->BeginCancelRepairTask(
            messageBody,
            HostingConfig::GetConfig().RequestTimeout,
            [this, description](AsyncOperationSPtr const& operation) { this->OnRepairTaskCanceled(operation, false, description.RepairTaskId); },
            thisSPtr);

        this->OnRepairTaskCanceled(currentOperation, true, description.RepairTaskId);
    }

    void OnRepairTaskCanceled(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously, wstring const & taskId)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = owner_.hosting_.RepairManagementClient->EndDeleteRepairTask(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "OnRepairTaskCanceled: Cancelled TaskId:{0} failed with ErrorCode={1}",
            taskId,
            error);
    }

    void MoveTaskToRestoringState(AsyncOperationSPtr const& thisSPtr, RepairTask const & repairTask)
    {
        FABRIC_REPAIR_TASK task;
        ScopedHeap heap;
        repairTask.ToPublicApi(heap, task);
        task.Reserved = NULL;
        task.State = FABRIC_REPAIR_TASK_STATE_RESTORING;
        FABRIC_REPAIR_RESULT_DESCRIPTION resultDesc;
        resultDesc.Reserved = NULL;
        resultDesc.ResultCode = S_OK;
        resultDesc.ResultDetails = L"Restarted";
        resultDesc.ResultStatus = FABRIC_REPAIR_TASK_RESULT_SUCCEEDED;
        task.Result = &resultDesc;

        RepairTask repairTask1;
        ErrorCode error = repairTask1.FromPublicApi(task);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "MoveTaskToRestoringState: Could not create repair task: TaskId:{0} ErrorCode={1}",
                repairTask.TaskId,
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        AsyncOperationSPtr operation = owner_.hosting_.RepairManagementClient->BeginUpdateRepairExecutionState(
            repairTask1,
            HostingConfig::GetConfig().RequestTimeout,
            [this, repairTask1](AsyncOperationSPtr const& operation) { this->OnUpdateRepairExecutionState(operation, false, repairTask1.TaskId); },
            thisSPtr);

        this->OnUpdateRepairExecutionState(operation, true, repairTask1.TaskId);
    }

    void OnUpdateRepairExecutionState(AsyncOperationSPtr const& operation, bool expectedCompleteSynchronously, wstring const & taskId)
    {
        if (operation->CompletedSynchronously != expectedCompleteSynchronously)
        {
            return;
        }

        int64 commitVersion;
        auto error = owner_.hosting_.RepairManagementClient->EndUpdateRepairExecutionState(operation, commitVersion);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_RestartManagerClient,
            owner_.Root.TraceId,
            "OnUpdateRepairExecutionState: Prepare task of Id:{0} failed with ErrorCode={1}",
            taskId,
            error);
    }

    void SendNodeEnabledNotification(AsyncOperationSPtr thisSPtr)
    {
        NodeEnabledNotification enabledNotification(ErrorCodeValue::Success);
        MessageUPtr request = make_unique<Message>(enabledNotification);
        request->Headers.Add(Transport::ActorHeader(Actor::RestartManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::NodeEnabledNotification));

        WriteNoise(TraceType_RestartManagerClient, owner_.Root.TraceId, "SendNodeEnabledNotification: Message={0}, Body={1}", *request, enabledNotification);

        auto operation = owner_.Client.BeginRequest(
            move(request),
            TimeSpan::FromMinutes(1),
            [this](AsyncOperationSPtr const & operation){ OnNodeEnabledNotification(operation, false); },
            thisSPtr);
        OnNodeEnabledNotification(operation, true);
    }

    void OnNodeEnabledNotification(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                owner_.Root.TraceId,
                "End(OnNodeEnabledNotification): ErrorCode={0}",
                error);

            this->ScheduleTimer(operation->Parent, TimeSpan::FromSeconds(Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds));
        }
        else
        {
            TryComplete(operation->Parent, ErrorCode::Success());
        }
    }

    void PollingTimerCallback(AsyncOperationSPtr const & thisSPtr)
    {
        GetCurrentNodeTask(thisSPtr);
    }


private:
    NodeRestartManagerClient & owner_;
    const wstring & disableReason_;
    Common::TimerSPtr pollingTimer_;
    Common::RwLock lock_;
    bool completedOrCancelled_;
};

NodeRestartManagerClient::NodeRestartManagerClient(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting,
    wstring const & nodeId)
    : RootedObject(root)
    , hosting_(hosting)
    , nodeId_(nodeId)
    , ipcClient_()
    , clientId_()
    , executor_()
    , executorPrefix_(L"RestartManager.")
    , upgradeDomainId_()
{
    wstring activatorAddress;
    this->enableNodeRequestOperation_ = nullptr;
    this->disableNodeRequestOperation_ = nullptr;
    
    if(Environment::GetEnvironmentVariable(Constants::FabricActivatorAddressEnvVariable, activatorAddress, NOTHROW()))
    {
        clientId_ = Guid::NewGuid().ToString();
        this->executor_ = executorPrefix_;
        this->executor_.append(nodeId_);
        this->upgradeDomainId_ = this->hosting_.FabricNodeConfigObj.UpgradeDomainId.c_str();

        WriteInfo(TraceType_RestartManagerClient,
            root.TraceId,
            "FabricHost address retrieved:{0} Executor:{1} UpgradeId:{2}",
            activatorAddress,
            this->executor_,
            this->upgradeDomainId_);

        auto ipcClient = make_unique<IpcClient>(this->Root, 
            clientId_,
            activatorAddress,
            false /* disallow use of unreliable transport */,
            L"NodeRestartManagerClient");
        ipcClient_ = move(ipcClient);
    }
    else
    {
        WriteWarning(
            TraceType_RestartManagerClient,
            root.TraceId,
            "IPCServer address not found in env. IPC client not initialized");
    }
}

NodeRestartManagerClient::~NodeRestartManagerClient()
{
}

Common::AsyncOperationSPtr NodeRestartManagerClient::OnBeginOpen(
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

Common::ErrorCode NodeRestartManagerClient::OnEndOpen(Common::AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr NodeRestartManagerClient::OnBeginClose(
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);  
}

Common::ErrorCode NodeRestartManagerClient::OnEndClose(Common::AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void NodeRestartManagerClient::OnAbort()
{
    if(this->ipcClient_)
    {
        this->UnregisterFabricActivatorClient();
        this->UnregisterIpcMessageHandler();
        this->ipcClient_->Abort();
    }
}

void NodeRestartManagerClient::UnregisterFabricActivatorClient()
{
    AsyncOperationWaiterSPtr cleanupWaiter = make_shared<AsyncOperationWaiter>();
    TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;
    MessageUPtr request = CreateUnregisterClientRequest();
    auto operation = Client.BeginRequest(
        move(request),
        HostingConfig::GetConfig().RequestTimeout,
        [this, cleanupWaiter](AsyncOperationSPtr const & operation)
    {
        MessageUPtr reply;
        auto error = Client.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_RestartManagerClient,
                Root.TraceId,
                "Failed UnregisterFabricActivatorClient: ErrorCode={0}",
                error);
        }
        cleanupWaiter->SetError(error);
        cleanupWaiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    if (cleanupWaiter->WaitOne(timeout))
    {
        auto error = cleanupWaiter->GetError();
        WriteTrace(
            error.ToLogLevel(),
            TraceType_RestartManagerClient,
            Root.TraceId,
            "Unregister fabricactivatorclient request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType_RestartManagerClient,
            Root.TraceId,
            "Unregister fabricactivatorclient request cleanupWaiter failed on Wait");
    }
}

void NodeRestartManagerClient::RegisterIpcMessageHandler()
{
    auto root = this->Root.CreateComponentRoot();
    ipcClient_->RegisterMessageHandler(
        Actor::RestartManagerClient,
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & context)
    {
        this->IpcMessageHandler(*message, context);
    },
        false/*dispatchOnTransportThread*/);
}

void NodeRestartManagerClient::UnregisterIpcMessageHandler()
{
    ipcClient_->UnregisterMessageHandler(Actor::RestartManagerClient);
}

bool NodeRestartManagerClient::IsIpcClientInitialized()
{
    return (ipcClient_ != nullptr);
}

void NodeRestartManagerClient::IpcMessageHandler(__in Message & message, __in IpcReceiverContextUPtr & context)
{
    WriteNoise(TraceType_RestartManagerClient, Root.TraceId, "Received message: actor={0}, action={1}", message.Actor, message.Action);

    if (message.Actor == Actor::RestartManagerClient)
    {
        ProcessIpcMessage(message, context);
    }
    else
    {
        WriteWarning(TraceType_RestartManagerClient, Root.TraceId, "Unexpected message: actor={0}, action={1}", message.Actor, message.Action);
    }
}

void NodeRestartManagerClient::ProcessIpcMessage(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context)
{
    if (message.Action == Hosting2::Protocol::Actions::DisableNodeRequest)
    {
        ProcessDisableNodeRequest(message, context);
        return;
    }

    if (message.Action == Hosting2::Protocol::Actions::EnableNodeRequest)
    {
        ProcessEnableNodeRequest(message, context);
        return;
    }
}

void NodeRestartManagerClient::ProcessDisableNodeRequest(__in Message & message, __in IpcReceiverContextUPtr & context)
{
    DisableNodeRequest disableNodeRequest;

    if (!message.GetBody<DisableNodeRequest>(disableNodeRequest))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_RestartManagerClient,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    //Just return message to ack reception
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    MessageUPtr response = make_unique<Message>(disableNodeRequest);
    context->Reply(move(response));

    // Start the process if node disable operation is not running
    if (this->disableNodeRequestOperation_ == NULL || this->disableNodeRequestOperation_ == nullptr)
    {
        this->CancelRunningEnableOperation();

        WriteInfo(TraceType_RestartManagerClient,
            Root.TraceId,
            "starting disableNodeRequestAsync operation");

        this->disableNodeRequestOperation_ = AsyncOperation::CreateAndStart<DisableNodeRequestAsyncOperation>(
            *this,
            disableNodeRequest.DisableReason, 
            [this](AsyncOperationSPtr const& operation) { this->OnDisableNodeRequestOperationComplete(operation); },
            this->Root.CreateAsyncOperationRoot());
    }
}

void NodeRestartManagerClient::OnDisableNodeRequestOperationComplete(AsyncOperationSPtr const & operation)
{
    WriteInfo(TraceType_RestartManagerClient,
        Root.TraceId,
        "disableNodeRequestAsync operation ended");

    DisableNodeRequestAsyncOperation::End(operation);
    this->disableNodeRequestOperation_ = nullptr;
}

void NodeRestartManagerClient::ProcessEnableNodeRequest(__in Message & message, __in IpcReceiverContextUPtr & context)
{
    EnableNodeRequest enableNodeRequest;

    if (!message.GetBody<EnableNodeRequest>(enableNodeRequest))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_RestartManagerClient,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    //Just return message to ack reception
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    MessageUPtr response = make_unique<Message>(enableNodeRequest);
    context->Reply(move(response));

    // Start the process if node disable operation is not running
    if (this->enableNodeRequestOperation_ == nullptr || this->enableNodeRequestOperation_ == NULL || this->enableNodeRequestOperation_->IsCompleted)
    {
        WriteInfo(TraceType_RestartManagerClient,
            Root.TraceId,
            "starting enableNodeRequestAsync operation");

        this->enableNodeRequestOperation_ = AsyncOperation::CreateAndStart<EnableNodeRequestAsyncOperation>(
            *this,
            enableNodeRequest.DisableReason,
            [this](AsyncOperationSPtr const& operation) { this->OnEnableNodeRequestOperationComplete(operation); },
            this->Root.CreateAsyncOperationRoot());
    }
}

void NodeRestartManagerClient::CancelRunningEnableOperation()
{
    if (this->enableNodeRequestOperation_ != NULL && !this->enableNodeRequestOperation_->IsCompleted)
    {
        this->enableNodeRequestOperation_->Cancel(true);
    }
}

void NodeRestartManagerClient::OnEnableNodeRequestOperationComplete(AsyncOperationSPtr const & operation)
{
    WriteInfo(TraceType_RestartManagerClient,
        Root.TraceId,
        "enableNodeRequestAsync operation ended");

    EnableNodeRequestAsyncOperation::End(operation);
    this->enableNodeRequestOperation_ = nullptr;
}

MessageUPtr NodeRestartManagerClient::CreateUnregisterClientRequest()
{
    RegisterFabricActivatorClientRequest registerRequest(processId_, nodeId_);
    MessageUPtr request = make_unique<Message>(registerRequest);
    request->Headers.Add(Transport::ActorHeader(Actor::RestartManager));
    request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::UnregisterFabricActivatorClient));
    WriteNoise(TraceType_RestartManagerClient, Root.TraceId, "RegisterFabricActivatorClient: Message={0}, Body={1}", *request, registerRequest);
    return move(request);
}
