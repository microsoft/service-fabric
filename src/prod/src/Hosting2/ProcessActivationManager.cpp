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
using namespace Management;
using namespace ImageModel;
using namespace Management::ResourceMonitor;
using namespace Management::NetworkInventoryManager;

StringLiteral const TraceType_ActivationManager("ProcessActivationManager");

class ProcessActivationManager::NotifyServiceTerminationAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    NotifyServiceTerminationAsyncOperation(
        ProcessActivationManager & owner,
        ActivityDescription const & activityDescription,
        wstring const & parentId,
        vector<wstring> const & appServiceId,
        DWORD exitCode,
        bool isRootContainer,
        AsyncCallback const & callback,
        AsyncOperationSPtr parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        activityDescription_(activityDescription),
        parentId_(parentId),
        appServiceId_(appServiceId),
        exitCode_(exitCode),
        isRootContainer_(isRootContainer)
    {
    }
    static ErrorCode NotifyServiceTerminationAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<NotifyServiceTerminationAsyncOperation>(operation);
        return thisPtr->Error;
    }
protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SendTerminationNotification(thisSPtr);
    }

    void SendTerminationNotification(AsyncOperationSPtr const & thisSPtr)
    {
        ActivatorRequestorSPtr requestor;
        auto error = owner_.requestorMap_.Get(parentId_, requestor);
        if (error.IsSuccess())
        {
            ServiceTerminatedNotification notification(activityDescription_, parentId_, appServiceId_, exitCode_, isRootContainer_);

            MessageUPtr request = make_unique<Message>(notification);
            request->Headers.Add(Transport::ActorHeader(Actor::FabricActivatorClient));
            request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ServiceTerminatedNotificationRequest));

            WriteInfo(TraceType_ActivationManager, owner_.Root.TraceId, "ServiceTerminatedNotification: Message={0}, Body={1}", *request, notification);
            auto operation = owner_.activationManager_.IpcServerObj.BeginRequest(
                move(request),
                requestor->CallbackAddress,
                HostingConfig::GetConfig().RequestTimeout,
                [this](AsyncOperationSPtr const & operation) { FinishSendServiceTerminationMessage(operation, false); },
                thisSPtr);
            FinishSendServiceTerminationMessage(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

    void FinishSendServiceTerminationMessage(AsyncOperationSPtr operation, bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }
        MessageUPtr reply;
        auto error = owner_.activationManager_.IpcServerObj.EndRequest(operation, reply);
        if (!error.IsSuccess() &&
            error.IsError(ErrorCodeValue::Timeout))
        {
            WriteInfo(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Sending notification for AppHost down timedout. AppHost {0}, Node {1}. Retrying",
                appServiceId_,
                parentId_);
            SendTerminationNotification(operation->Parent);
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }
private:
    ProcessActivationManager & owner_;
    ActivityDescription activityDescription_;
    wstring parentId_;
    vector<wstring> appServiceId_;
    DWORD exitCode_;
    bool isRootContainer_;
};

class ProcessActivationManager::NotifyContainerHealthCheckStatusAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    NotifyContainerHealthCheckStatusAsyncOperation(
        ProcessActivationManager & owner,
        wstring const & nodeId,
        vector<ContainerHealthStatusInfo> const & healthStatusList,
        AsyncCallback const & callback,
        AsyncOperationSPtr parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , nodeId_(nodeId)
        , healthStatusList_(healthStatusList)
    {
    }

    static ErrorCode NotifyContainerHealthCheckStatusAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<NotifyContainerHealthCheckStatusAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SendHealthNotification(thisSPtr);
    }

    void SendHealthNotification(AsyncOperationSPtr const & thisSPtr)
    {
        ActivatorRequestorSPtr requestor;

        auto error = owner_.requestorMap_.Get(nodeId_, requestor);
        if (error.IsSuccess())
        {
            ContainerHealthCheckStatusChangeNotification notification(nodeId_, healthStatusList_);

            auto request = make_unique<Message>(move(notification));

            request->Headers.Add(Transport::ActorHeader(Actor::FabricActivatorClient));
            request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ContainerHealthCheckStatusChangeNotificationRequest));

            WriteNoise(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "ContainerHealthCheckStatusChangeNotification: Message={0}, Body={1}", *request, notification);

            auto operation = owner_.activationManager_.IpcServerObj.BeginRequest(
                move(request),
                requestor->CallbackAddress,
                HostingConfig::GetConfig().RequestTimeout,
                [this](AsyncOperationSPtr const & operation) { FinishSendHealthNotification(operation, false); },
                thisSPtr);

            FinishSendHealthNotification(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

    void FinishSendHealthNotification(AsyncOperationSPtr operation, bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }

        MessageUPtr reply;

        auto error = owner_.activationManager_.IpcServerObj.EndRequest(operation, reply);
        if (!error.IsSuccess() && error.IsError(ErrorCodeValue::Timeout))
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Sending notification for container health check status change timed out. AppHost {0}, Node {1}. Retrying...",
                healthStatusList_,
                nodeId_);

            SendHealthNotification(operation->Parent);
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }

private:
    ProcessActivationManager & owner_;
    wstring nodeId_;
    vector<ContainerHealthStatusInfo> healthStatusList_;
};

class ProcessActivationManager::NotifyDockerProcessTerminationAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    NotifyDockerProcessTerminationAsyncOperation(
        ProcessActivationManager & owner,
        DWORD exitCode,
        ULONG continuousFailurecount,
        TimeSpan const& nextStartTime,
        AsyncCallback const & callback,
        AsyncOperationSPtr parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        exitCode_(exitCode),
        continuousFailurecount_(continuousFailurecount),
        nextStartTime_(nextStartTime)
    {
    }
    static ErrorCode NotifyDockerProcessTerminationAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<NotifyDockerProcessTerminationAsyncOperation>(operation);
        return thisPtr->Error;
    }
protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SendDockerProcessTerminationNotification(thisSPtr);
    }

    void SendDockerProcessTerminationNotification(AsyncOperationSPtr const & thisSPtr)
    {
        auto requestors = owner_.requestorMap_.GetAll();
        auto nodeCount = requestors.size();
        pendingOperationCount_.store(nodeCount);

        for (auto const& requestor : requestors)
        {
            auto nodeId = requestor->RequestorId;
            DockerProcessTerminatedNotification notification(exitCode_, continuousFailurecount_, nextStartTime_);

            MessageUPtr request = make_unique<Message>(move(notification));
            request->Headers.Add(Transport::ActorHeader(Actor::FabricActivatorClient));
            request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DockerProcessTerminatedNotificationRequest));

            WriteInfo(TraceType_ActivationManager, owner_.Root.TraceId, "DockerProcessTerminatedNotificationRequest: NodeId={0}", nodeId);
            auto operation = owner_.activationManager_.IpcServerObj.BeginRequest(
                move(request),
                requestor->CallbackAddress,
                HostingConfig::GetConfig().RequestTimeout,
                [this, nodeId](AsyncOperationSPtr const & operation) { FinishSendServiceTerminationMessage(operation, nodeId, false); },
                thisSPtr);
            FinishSendServiceTerminationMessage(operation, nodeId, true);
        }

        CheckPendingOperations(thisSPtr, nodeCount);
    }

    void FinishSendServiceTerminationMessage(AsyncOperationSPtr operation, wstring nodeId, bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }
        MessageUPtr reply;
        auto error = owner_.activationManager_.IpcServerObj.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Sending notification for DockerProcessManager failed. This notification is best effort and won't be retried. NodeId:{0}, Error:{1}",
                nodeId,
                error);
        }

        uint64 pendingOperationCount = --pendingOperationCount_;
        CheckPendingOperations(operation->Parent, pendingOperationCount);
    }

    void CheckPendingOperations(AsyncOperationSPtr const & thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }
    }

private:
    ProcessActivationManager & owner_;
    atomic_uint64 pendingOperationCount_;
    DWORD exitCode_;
    ULONG continuousFailurecount_;
    TimeSpan nextStartTime_;
};

class ProcessActivationManager::OpenAsyncOperation :
    public AsyncOperation,
        TextTraceComponent<TraceTaskCodes::Hosting>
    {
    public:
        OpenAsyncOperation(
            ProcessActivationManager & owner,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            timeoutHelper_(timeout),
            lastError_(ErrorCodeValue::Success)
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
            owner_.RegisterIpcRequestHandler();
            auto error = owner_.principalsProvider_->Open();
            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "SecurityPrincipalsProvider open returned error {0}",
                error);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
            error = owner_.endpointProvider_->Open();
            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "HttpEndpointSecurityProvider open returned error {0}",
                error);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
            error = owner_.crashDumpConfigurationManager_->Open();
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }

            SidUPtr currentSid;
            error = BufferedSid::GetCurrentUserSid(currentSid);
            if (error.IsSuccess())
            {
#if !defined(PLATFORM_UNIX)
                owner_.isSystem_ = ::IsWellKnownSid(currentSid->PSid, WinLocalSystemSid);
#else
                owner_.isSystem_ = currentSid->IsLocalSystem();
#endif
            }
            else
            {
                WriteError(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to obtain current user SID. Error {0}",
                    error);
                TryComplete(thisSPtr, error);
                return;
            }

            error = owner_.firewallProvider_->Open();
            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "FirewallSecurityProvider open returned error {0}",
                error);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
#if defined(PLATFORM_UNIX)
            //This will delete the whole fabric cgroup hierarchy in case there are leftovers from previous FabricHost cras
            //Also this will initialize the libcgoup library
            SetupCgroups();
#endif
            auto operation = owner_.containerActivator_->BeginOpen(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const& operation)
            {
                this->OnContainerActivatorOpenCompleted(operation, false);
            },
                thisSPtr);
            OnContainerActivatorOpenCompleted(operation, true);
        }
#if defined(PLATFORM_UNIX)

        void SetupCgroups()
        {
            struct cgroup * cgroupObj;
            std::string cgroupName;
            StringUtility::Utf16ToUtf8(Constants::CgroupPrefix, cgroupName);
            int error = 0;

            error = cgroup_init(); // This is needed before we can use cgroups

            if (error)
            {
                WriteError(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Cgroup initialization failed with error {0}, check cgroup mount point",
                    error);
                return;
            }

            cgroupObj = cgroup_new_cgroup(cgroupName.c_str());
            if (cgroupObj)
            {
                auto memoryController = cgroup_add_controller(cgroupObj, "memory");
                if (!memoryController)
                {
                    error = ECGFAIL;
                    goto CleanupSetupCgroup;
                }
                auto cpuController = cgroup_add_controller(cgroupObj, "cpu");
                if (!cpuController)
                {
                    error = ECGFAIL;
                    goto CleanupSetupCgroup;
                }
                //remove all the fabric cgroups if we have some leftovers
                error = cgroup_delete_cgroup_ext(cgroupObj, CGFLAG_DELETE_RECURSIVE);
            }
            else
            {
                error = ECGFAIL;
                goto CleanupSetupCgroup;
            }

CleanupSetupCgroup:
            if (cgroupObj)
            {
                cgroup_free(&cgroupObj);
            }
            WriteInfo(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Cgroup setup finished with error {0}",
                error);
        }

#endif
        void OnContainerActivatorOpenCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSyncronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSyncronously)
            {
                return;
            }
            auto error = owner_.containerActivator_->EndOpen(operation);
            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "ContainerActivator open returned error {0}",
                error);

            // Schedule a timer to Post Statistics.
            owner_.ScheduleTimerForStats(HostingConfig::GetConfig().InitialStatisticsInterval);

            // Create marker files for all container log directory so that FabricDCA can delete them.
            // If FabricHost crashes and comes back up this will ensure that we are not leaving any
            // container log directories behind.
            ContainerHelper::GetContainerHelper().MarkContainerLogFolder(L"", false /*forProcessing*/,  true /*MarkAllLogDirectories*/);

            TryComplete(operation->Parent, error);
        }

    private:
        TimeoutHelper timeoutHelper_;
        ProcessActivationManager & owner_;
        atomic_uint64 pendingOperationCount_;
        ErrorCode lastError_;
    };

class ProcessActivationManager::CloseAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    CloseAsyncOperation(
        ProcessActivationManager & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        lastError_(ErrorCodeValue::Success)
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
        CancelStatisticsTimer(thisSPtr);
    }

private:

    void CancelStatisticsTimer(AsyncOperationSPtr const& thisSPtr)
    {
        if (owner_.statisticsIntervalTimerSPtr_)
        {
            owner_.statisticsIntervalTimerSPtr_->Cancel();
        }

        DeactivateApplicationServices(thisSPtr);
    }

    void DeactivateApplicationServices(AsyncOperationSPtr const & thisSPtr)
    {
        auto requestors = owner_.requestorMap_.Close();
        for (auto it = requestors.begin(); it != requestors.end(); it++)
        {
            (*it)->Close();
        }
        vector<ApplicationServiceSPtr> applicationServices = owner_.applicationServiceMap_->Close();
        if (applicationServices.size() > 0)
        {
            pendingOperationCount_.store(applicationServices.size());
            for (auto iter = applicationServices.begin(); iter != applicationServices.end(); iter++)
            {
                ApplicationServiceSPtr applicationService = (*iter);
                auto operation = applicationService->BeginDeactivate(
                    true,
                    FabricHostConfig::GetConfig().StopTimeout,
                    [this, applicationService](AsyncOperationSPtr const & operation)
                {
                    this->FinishDeactivate(operation, applicationService, false);
                },
                    thisSPtr);
                FinishDeactivate(operation, applicationService, true);
            }
        }
        else
        {
            CheckPendingOperations(thisSPtr, 0);
        }
    }

    void FinishDeactivate(AsyncOperationSPtr operation,
        ApplicationServiceSPtr const & applicationService,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        ErrorCode error = applicationService->EndDeactivate(operation);
        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }
        uint64 pendingOperationCount = --pendingOperationCount_;
        CheckPendingOperations(operation->Parent, pendingOperationCount);
    }

    void CheckPendingOperations(AsyncOperationSPtr const & thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            auto error = owner_.crashDumpConfigurationManager_->Close();
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to close crash dump configuration manager. Error {0}",
                    error);
                lastError_.Overwrite(error);
            }

            auto err = owner_.principalsProvider_->Close();
            if (!err.IsSuccess())
            {
                WriteError(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to close principalsprovider. Error {0}",
                    err);
                lastError_.Overwrite(err);
            }
            err = owner_.endpointProvider_->Close();
            if (!err.IsSuccess())
            {
                WriteError(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to close endpointsecurity provider. Error {0}",
                    err);
                lastError_.Overwrite(err);
            }
            err = owner_.firewallProvider_->Close();
            if (!err.IsSuccess())
            {
                WriteError(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to close firewall provider. Error {0}",
                    err);
                lastError_.Overwrite(err);
            }

            auto operation = owner_.containerActivator_->BeginClose(
                FabricHostConfig::GetConfig().StopTimeout,
                [this](AsyncOperationSPtr const & operation)
            {
                this->ContainerActivatorDeactivated(operation, false);
            },
                thisSPtr);

            ContainerActivatorDeactivated(operation, true);
        }
    }

    void ContainerActivatorDeactivated(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        ErrorCode error = owner_.containerActivator_->EndClose(operation);

        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to close ContainerActivator. Error {0}",
                error);
            lastError_.Overwrite(error);
        }

        lastError_.ReadValue();
        TryComplete(operation->Parent, lastError_);
    }

private:
    TimeoutHelper timeoutHelper_;
    ProcessActivationManager & owner_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
};

class ProcessActivationManager::ActivateServiceAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ActivateServiceAsyncOperation(
        ProcessActivationManager & owner,
        ActivateProcessRequest const & request,
        TimeSpan const timeout,
        Transport::IpcReceiverContextUPtr & context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        nodeId_(request.NodeId),
        applicationId_(request.ApplicationId),
        appServiceId_(request.ApplicationServiceId),
        appService_(),
        owner_(owner),
        processDescription_(request.ProcDescription),
        securityUserId_(request.SecurityUserId),
        fabricBinFolder_(request.FabricBinFolder),
        isContainerHost_(request.IsContainerHost),
        containerDescription_(request.ContainerDescriptionObj),
        timeoutHelper_(timeout),
        context_(move(context)),
        processId_(0)
    {
    }

    static ErrorCode ActivateServiceAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateServiceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        bool appHostPresent;
        ErrorCode success(ErrorCodeValue::Success);
        auto error = owner_.applicationServiceMap_->Contains(nodeId_, appServiceId_, appHostPresent);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to check if apphost is already present for Node {0}, Application {1}, AppHostId {2}. Error {3}",
                nodeId_,
                applicationId_,
                appServiceId_,
                error);
            SendActivateProcessReplyAndComplete(error, thisSPtr);
            return;
        }
        if (appHostPresent)
        {
            WriteInfo(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Apphost is already present for Node {0}, Application {1}, AppHostId {2}.",
                nodeId_,
                applicationId_,
                appServiceId_);
            SendActivateProcessReplyAndComplete(ErrorCode(ErrorCodeValue::Success), thisSPtr);
            return;
        }

        SecurityUserSPtr secUser;
        //this means we should run this process as member of win fab admin group
        //this is needed for resource monitor service in order to communicate with fabric host
        //and backup restore service which needs to access the encryption cert ACLed for winfabadmin group
        //should be only used for members of system app package
        bool isWinFabAdminMember = securityUserId_ == *Constants::WindowsFabricAdministratorsGroupAllowedUser;

        if (!securityUserId_.empty() && !isWinFabAdminMember)
        {
            error = owner_.principalsProvider_->GetSecurityUser(nodeId_, applicationId_, securityUserId_, secUser);
            if (!error.IsSuccess())
            {
                WriteWarning(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to get security user for Node {0}, Application {1}, AppHostId {2} securityuserid {3}. Error {4}",
                    nodeId_,
                    applicationId_,
                    appServiceId_,
                    securityUserId_,
                    error);
                SendActivateProcessReplyAndComplete(error, thisSPtr);
                return;
            }
        }
        else
        {
            ActivatorRequestorSPtr activatorRequestor;
            error = owner_.requestorMap_.Get(nodeId_, activatorRequestor);
            if (!error.IsSuccess())
            {
                WriteWarning(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to get requestor for Node {0}. Error {1}",
                    nodeId_,
                    error);
                SendActivateProcessReplyAndComplete(error, thisSPtr);
                return;
            }
#if !defined(PLATFORM_UNIX)
            error = activatorRequestor->GetDefaultSecurityUser(secUser, isWinFabAdminMember);
#else
            if (applicationId_.compare(ServiceModel::ApplicationIdentifier::FabricSystemAppId->ToString()) == 0)
            {
                error = activatorRequestor->GetDefaultSecurityUser(secUser, isWinFabAdminMember);
            }
            else
            {
                error = activatorRequestor->GetDefaultAppSecurityUser(secUser);
            }
#endif
            if (!error.IsSuccess())
            {
                WriteWarning(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to get default security user for Node {0}. Error {1}",
                    nodeId_,
                    error);
                SendActivateProcessReplyAndComplete(error, thisSPtr);
                return;
            }
        }
        appService_ = make_shared<ApplicationService>(ProcessActivationManagerHolder(
            owner_,
            owner_.Root.CreateComponentRoot()),
            nodeId_,
            appServiceId_,
            processDescription_,
            fabricBinFolder_,
            secUser,
            isContainerHost_,
            containerDescription_);

        auto err = owner_.applicationServiceMap_->Add(appService_);
        if (!err.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to add application service to map Node {0}, Application {1}, AppHostId {2}. Error {3}",
                nodeId_,
                applicationId_,
                appServiceId_,
                error);
            SendActivateProcessReplyAndComplete(err, thisSPtr);
            return;
        }
        ActivateApplicationService(thisSPtr);
    }

private:

    void ActivateApplicationService(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Activation applicationservice for  Node {0}, Application {1}, AppHostId {2}",
            nodeId_,
            applicationId_,
            appServiceId_);
        auto operation = appService_->BeginActivate(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->FinishActivate(operation, false);
        },
            thisSPtr);
        FinishActivate(operation, true);
    }

    void FinishActivate(AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        ErrorCode error = appService_->EndActivate(operation, processId_);
        if (!error.IsSuccess())
        {
            auto err = owner_.applicationServiceMap_->Remove(nodeId_, appServiceId_);
            WriteTrace(err.ToLogLevel(),
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Removing failed AppServiceId {0}, for parent {1} returned {2}",
                appServiceId_,
                nodeId_,
                err);
        }

        SendActivateProcessReplyAndComplete(error, operation->Parent);

    }

    void SendActivateProcessReplyAndComplete(ErrorCode error, AsyncOperationSPtr const & thisSPtr)
    {
        unique_ptr<ActivateProcessReply> replyBody;
        replyBody = make_unique<ActivateProcessReply>(appServiceId_, processId_, error, error.Message);
        MessageUPtr reply = make_unique<Message>(*replyBody);
        WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Sent ActivateProcessReply: for AppServiceId {0}, error {1}",
            appServiceId_,
            error);
        context_->Reply(move(reply));
        TryComplete(thisSPtr, error);
    }

private:
    TimeoutHelper timeoutHelper_;
    wstring const nodeId_;
    wstring const applicationId_;
    wstring const appServiceId_;
    ApplicationServiceSPtr appService_;
    ProcessActivationManager & owner_;
    ProcessDescription const & processDescription_;
    wstring const fabricBinFolder_;
    wstring const securityUserId_;
    Transport::IpcReceiverContextUPtr context_;
    DWORD processId_;
    bool isContainerHost_;
    ContainerDescription const containerDescription_;
};

class ProcessActivationManager::DeactivateServiceAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    DeactivateServiceAsyncOperation(
        ProcessActivationManager & owner,
        wstring const & parentId,
        wstring const & appServiceId,
        TimeSpan const timeout,
        Transport::IpcReceiverContextUPtr & context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        parentId_(parentId),
        appServiceId_(appServiceId),
        owner_(owner),
        timeoutHelper_(timeout),
        context_(move(context))
    {
    }

    static ErrorCode DeactivateServiceAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateServiceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ApplicationServiceSPtr appService;
        ErrorCode err = owner_.applicationServiceMap_->Get(parentId_, appServiceId_, appService);
        if (!err.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to get application service {0} from services map. Error {1}",
                appServiceId_,
                err);

            SendDeactivationReplyAndComplete(err, thisSPtr);
            return;
        }
        if (!appService)
        {
            SendDeactivationReplyAndComplete(ErrorCode(ErrorCodeValue::UserServiceNotFound), thisSPtr);
            return;
        }
        else
        {
            err = owner_.applicationServiceMap_->Remove(parentId_, appServiceId_);
            if (!err.IsSuccess())
            {
                WriteWarning(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to remove application service {0} from services map. Error {1}",
                    appServiceId_,
                    err);
                SendDeactivationReplyAndComplete(err, thisSPtr);
                return;
            }

            DeactivateApplicationService(appService, thisSPtr);
        }
    }

private:

    void DeactivateApplicationService(ApplicationServiceSPtr appService, AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = appService->BeginDeactivate(
            true,
            timeoutHelper_.GetRemainingTime(),
            [this, appService](AsyncOperationSPtr const & operation)
        {
            this->FinishDeactivate(appService, operation, false);
        },
            thisSPtr);
        FinishDeactivate(appService, operation, true);
    }

    void FinishDeactivate(ApplicationServiceSPtr appService,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        auto error = appService->EndDeactivate(operation);
        DWORD exitCode = error.IsSuccess() ? ProcessActivator::ProcessDeactivateExitCode : ProcessActivator::ProcessAbortExitCode;

        owner_.SendApplicationServiceTerminationNotification(parentId_, appServiceId_, exitCode, ActivityDescription(ActivityId(), ActivityType::Enum::ServicePackageEvent), appService->IsContainerRoot);
        SendDeactivationReplyAndComplete(error, operation->Parent);
    }

    void SendDeactivationReplyAndComplete(ErrorCode const & error, AsyncOperationSPtr const & thisSPtr)
    {
        unique_ptr<DeactivateProcessReply> replyBody;
        replyBody = make_unique<DeactivateProcessReply>(appServiceId_, error);
        MessageUPtr reply = make_unique<Message>(*replyBody);
        WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Sent DeactivateProcessReply: AppServiceId {0}, error {1}",
            appServiceId_,
            error);
        context_->Reply(move(reply));
        TryComplete(thisSPtr, error);
    }

private:
    TimeoutHelper timeoutHelper_;
    wstring const parentId_;
    wstring const appServiceId_;
    ProcessActivationManager & owner_;
    Transport::IpcReceiverContextUPtr context_;
};

class ProcessActivationManager::TerminateServicesAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    TerminateServicesAsyncOperation(
        ProcessActivationManager & owner,
        wstring const & parentId,
        vector<ApplicationServiceSPtr> const & appServices,
        vector<wstring> const & cgroupsToCleanup,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        parentId_(parentId),
        appServices_(appServices),
        cgroupsToCleanup_(cgroupsToCleanup),
        owner_(owner),
        timeoutHelper_(HostingConfig::GetConfig().ContainerTerminationTimeout),
        pendingOperationCount_(appServices.size()),
        lastError_(ErrorCodeValue::Success)
    {
    }

    static ErrorCode TerminateServicesAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<TerminateServicesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto it = appServices_.begin(); it != appServices_.end(); it++)
        {
            DeactivateApplicationService(*it, thisSPtr);
        }
    }

private:

    void DeactivateApplicationService(ApplicationServiceSPtr appService, AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = appService->BeginDeactivate(
            false,
            timeoutHelper_.GetRemainingTime(),
            [this, appService](AsyncOperationSPtr const & operation)
        {
            this->FinishDeactivate(appService, operation, false);
        },
            thisSPtr);
        FinishDeactivate(appService, operation, true);
    }

    void FinishDeactivate(ApplicationServiceSPtr appService,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        auto error = appService->EndDeactivate(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivationManager,
            "Terminate application service error {0}", error);
        DecrementAndCheckPendingOperations(operation->Parent, error);
    }

    void DecrementAndCheckPendingOperations(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        if (!error.IsSuccess())
        {
            lastError_.ReadValue();
            lastError_ = error;
        }
        if (pendingOperationCount_.load() == 0)
        {
            Assert::CodingError("Pending operation count is already 0");
        }
        uint64 pendingOperationCount = --pendingOperationCount_;

        if (pendingOperationCount == 0)
        {
            owner_.containerActivator_->CleanupAssignedIpsToNode(parentId_);
            RemoveCGroups();
            RemoveAssignedOverlayNetworkResourcesToNode(thisSPtr, parentId_);
        }
    }
   
    void RemoveAssignedOverlayNetworkResourcesToNode(AsyncOperationSPtr const & thisSPtr, wstring nodeId)
    {
        auto operation = owner_.containerActivator_->BeginCleanupAssignedOverlayNetworkResourcesToNode(
            nodeId,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnCleanupAssignedOverlayNetworkResourcesToNodeCompleted(operation, false);
        },
            thisSPtr);
        
        this->OnCleanupAssignedOverlayNetworkResourcesToNodeCompleted(operation, true);
    }

    void OnCleanupAssignedOverlayNetworkResourcesToNodeCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.containerActivator_->EndCleanupAssignedOverlayNetworkResourcesToNode(operation);
        
        WriteInfo(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Clean up assigned overlay network resources to node completed for Node={0} with error={1}.",
            parentId_,
            error);

        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        TryComplete(operation->Parent, lastError_);
    }

    void RemoveCGroups()
    {
#if defined(PLATFORM_UNIX)
        //once we are done with stopping the containers try and cleanup cgroups
        for (auto it = cgroupsToCleanup_.begin(); it != cgroupsToCleanup_.end(); ++it)
        {
            string cgroupName;
            StringUtility::Utf16ToUtf8(*it, cgroupName);
            auto error = ProcessActivator::RemoveCgroup(cgroupName, true);
            WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Cgroup cleanup of {0} completed with error code {1}",
                *it, error);
        }
#endif
    }

private:
    TimeoutHelper timeoutHelper_;
    vector<ApplicationServiceSPtr> appServices_;
    vector<wstring> cgroupsToCleanup_;
    wstring parentId_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
    ProcessActivationManager & owner_;
};

class ProcessActivationManager::ActivateContainerGroupRootAsyncOperation
    : public AsyncOperation
    , TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ActivateContainerGroupRootAsyncOperation(
        ProcessActivationManager & owner,
        SetupContainerGroupRequest & request,
#if defined(PLATFORM_UNIX)
        ContainerPodDescription const &podDesc,
#endif
        TimeSpan const timeout,
        Transport::IpcReceiverContextUPtr && context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        nodeId_(request.NodeId),
        networkType_(request.NetworkType),
        openNetworkAssignedIp_(request.OpenNetworkAssignedIp),
        overlayNetworkResources_(request.OverlayNetworkResources),
        dnsServers_(request.DnsServers),
        servicePackageId_(request.ServicePackageId),
        appId_(request.AppId),
        appFolder_(request.AppFolder),
        appName_(request.AppName), 
        partitionId_(request.PartitionId),
        servicePackageActivationId_(request.ServicePackageActivationId),
        containerName_(GetContainerName(request.AppId)),
        context_(move(context)),
        owner_(owner),
#if defined(PLATFORM_UNIX)
        podDescription_(podDesc),
#endif
        spRg_(request.ServicePackageRG),
        timeoutHelper_(timeout),
        processId_(0)
    {
    }

    static ErrorCode ActivateContainerGroupRootAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateContainerGroupRootAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        bool appHostPresent;
        ErrorCode success(ErrorCodeValue::Success);
        auto error = owner_.applicationServiceMap_->Contains(nodeId_, servicePackageId_, appHostPresent);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to check if root container is already present for container group Node {0}, servicepackageId {1}. Error {2}",
                nodeId_,
                servicePackageId_,
                error);
            SendActivateProcessReplyAndComplete(thisSPtr, error);
            return;
        }
        if (appHostPresent)
        {
            WriteInfo(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Rootcontainer is already present for Node {0}, servicepackageId {1}.",
                nodeId_,
                servicePackageId_);
            SendActivateProcessReplyAndComplete(thisSPtr, error);
            return;
        }

        ActivatorRequestorSPtr activatorRequestor;
        error = owner_.requestorMap_.Get(nodeId_, activatorRequestor);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to get requestor for Node {0}. Error {1}",
                nodeId_,
                error);
            SendActivateProcessReplyAndComplete(thisSPtr, error);
            return;
        }

#if !defined(PLATFORM_UNIX)
        ProcessDescription processDescription(
            HostingConfig::GetConfig().ContainerGroupRootImageName,
            L"",
            appFolder_,
            EnvironmentMap(),
            appFolder_,
            appFolder_,
            appFolder_,
            appFolder_,
            false,
            false,
            false,
            false);
#else
        wstring cgroupName = L"";
        if (spRg_.IsGoverned)
        {
            cgroupName = L"/" + Constants::CgroupPrefix + L"/" + servicePackageId_;
        }
        ProcessDescription processDescription(
            HostingConfig::GetConfig().ContainerGroupRootImageName,
            L"",
            appFolder_,
            EnvironmentMap(),
            appFolder_,
            appFolder_,
            appFolder_,
            appFolder_,
            false,
            false,
            false,
            false,
            L"",
            0L,
            0L,
            ProcessDebugParameters(),
            ResourceGovernancePolicyDescription(),
            spRg_,
            cgroupName);
#endif

        auto entryPoint = HostingConfig::GetConfig().ContainerGroupEntrypoint;
#if defined(PLATFORM_UNIX)
        if (entryPoint.empty())
        {
            entryPoint = L"tail,-f,/dev/null";
        }
#endif

        ContainerNetworkConfigDescription containerNetworkConfig(
            openNetworkAssignedIp_,
            overlayNetworkResources_,
            map<wstring, wstring>(), // port bindings
            nodeId_,
            L"", // nodeName
            L"", // nodeIpAddress
            networkType_);

        ContainerDescription containerDescription(
            appName_,
            containerName_,
            L"-1",
            containerName_,
            HostingConfig::GetConfig().ContainerGroupEntrypoint,
            ContainerIsolationMode::Enum::process,
            L"",
            L"",
            L"",
            map<wstring, wstring>(),
            LogConfigDescription(),
            vector<ContainerVolumeDescription>(),
            vector<ContainerLabelDescription>(),
            dnsServers_,
            RepositoryCredentialsDescription(),
            ContainerHealthConfigDescription(),
            containerNetworkConfig,
            vector<wstring>(),
#if defined(PLATFORM_UNIX)
            podDescription_,
#endif
            true, // TODO: This should be propagated based on the requireSFRuntime flag.
            wstring(),
            false,
            false,
            true,
            true,
            true,
            wstring(),
            servicePackageActivationId_,
            partitionId_);

        appService_ = make_shared<ApplicationService>(ProcessActivationManagerHolder(
            owner_,
            owner_.Root.CreateComponentRoot()),
            nodeId_,
            servicePackageId_,
            processDescription,
            L"",
            nullptr,
            true,
            containerDescription);

        auto err = owner_.applicationServiceMap_->Add(appService_);
        if (!err.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to add application service to map Node {0}, servicepackageId {1}. Error {2}",
                nodeId_,
                servicePackageId_,
                error);
            SendActivateProcessReplyAndComplete(thisSPtr, error);
            return;
        }
        ActivateApplicationService(thisSPtr);
    }

private:

    void ActivateApplicationService(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Activation root container applicationservice for  Node {0}, servicepackage {1}",
            nodeId_,
            servicePackageId_);
        auto operation = appService_->BeginActivate(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->FinishActivate(operation, false);
        },
            thisSPtr);
        FinishActivate(operation, true);
    }

    void FinishActivate(AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        ErrorCode error = appService_->EndActivate(operation, processId_);
        if (!error.IsSuccess())
        {
            auto err = owner_.applicationServiceMap_->Remove(nodeId_, servicePackageId_);
            WriteTrace(err.ToLogLevel(),
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Removing failed AppServiceId {0}, for parent {1} returned {2}",
                servicePackageId_,
                nodeId_,
                err);
        }

        SendActivateProcessReplyAndComplete(operation->Parent, error);
        return;
    }

    void SendActivateProcessReplyAndComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        unique_ptr<FabricHostOperationReply> replyBody;
        replyBody = make_unique<FabricHostOperationReply>(error, error.Message, containerName_);
        MessageUPtr reply = make_unique<Message>(*replyBody);
        WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Sent ActivateProcessReply: for ContainerName {0}, error {1}",
            containerName_,
            error);
        context_->Reply(move(reply));
        TryComplete(thisSPtr, error);
    }

    wstring GetContainerName(wstring appId)
    {
       return wformatString("SF-SP-{0}-{1}", appId, Guid::NewGuid().ToString());
    }

private:
    TimeoutHelper timeoutHelper_;
    wstring nodeId_;
    ServiceModel::NetworkType::Enum networkType_;
    wstring openNetworkAssignedIp_;
    std::map<std::wstring, std::wstring> overlayNetworkResources_;
    std::vector<std::wstring> dnsServers_;
    wstring servicePackageId_;
    wstring appFolder_;
    ApplicationServiceSPtr appService_;
    wstring containerName_;
    wstring appId_;
    wstring appName_;
    wstring partitionId_;
    wstring servicePackageActivationId_;
    ServicePackageResourceGovernanceDescription spRg_;
#if defined(PLATFORM_UNIX)
    ContainerPodDescription podDescription_;
#endif
    ProcessActivationManager & owner_;
    DWORD processId_;
    Transport::IpcReceiverContextUPtr context_;
};

class ProcessActivationManager::FabricUpgradeAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    FabricUpgradeAsyncOperation(
        ProcessActivationManager & owner,
        bool const useFabricInstallerService,
        wstring const & programToRun,
        wstring const & arguments,
        wstring const & downloadedFabricPackageLocation,
        TimeSpan const timeout,
        Transport::IpcReceiverContextUPtr && context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        useFabricInstallerService_(useFabricInstallerService),
        programToRun_(programToRun),
        arguments_(arguments),
        downloadedFabricPackageLocation_(downloadedFabricPackageLocation),
        owner_(owner),
        timeoutHelper_(timeout),
        context_(move(context))
    {
    }

    static ErrorCode FabricUpgradeAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<FabricUpgradeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!useFabricInstallerService_)
        {
            wstring workingDir = programToRun_;
            StringUtility::Replace<wstring>(workingDir, Path::GetFileName(workingDir), L"");

            EnvironmentMap environmentMap;

#if defined(PLATFORM_UNIX)
            // Environment map is only needed in linux since the environment is not
            // passed in by default in linux
            auto getEnvResult = Environment::GetEnvironmentMap(environmentMap);
            if (!getEnvResult)
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to read EnvironmentBlock for FabricUpgradeAsyncOperation");

                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                return;
            }
#endif

            ProcessDescription processDesc(
                programToRun_,
                arguments_,
                workingDir,
                environmentMap,
                workingDir,
                workingDir,
                workingDir,
                workingDir,
                true,
                false,
                false,
                true);

            auto operation = owner_.ProcessActivatorObj->BeginActivate(
                nullptr,
                processDesc,
                workingDir,
                [this, thisSPtr](pid_t, Common::ErrorCode const & error, DWORD exitCode) { this->OnProcessExit(thisSPtr, error, exitCode); },
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->FinishActivate(operation, false); },
                thisSPtr);
            FinishActivate(operation, true);
        }
        else
        {
            this->SetupFabricInstallerService(thisSPtr);
        }
    }

private:
    void SetupFabricInstallerService(AsyncOperationSPtr const & thisSPtr)
    {
        //LINUXTODO
#if !defined(PLATFORM_UNIX)
        ServiceController sc(Constants::FabricUpgrade::FabricInstallerServiceName);
        DWORD serviceState = SERVICE_STOPPED;
        auto error = sc.GetState(serviceState);
        if (!error.IsSuccess())
        {
            if (error.IsWin32Error(ERROR_SERVICE_DOES_NOT_EXIST))
            {
                if (Path::GetExtension(downloadedFabricPackageLocation_) == L".msi")
                {
                    WriteError(TraceType_ActivationManager,
                        owner_.Root.TraceId,
                        "MSI {0} cannot install FabricInstallerSvc, and it is not installed. To bypass. delete UseFabricInstallerSvc registry value.",
                        downloadedFabricPackageLocation_);

                    this->SendReplyAndCompleteOperation(thisSPtr, ErrorCode(error.ReadValue(), wformatString(Constants::FabricUpgrade::ErrorMessageFormat, StringResource::Get(IDS_HOSTING_FabricInstallerService_Setup_Failed), error)));
                    return;
                }

                // Install the service
                error = this->ConfigureFabricInstallerService(false/*copyOnly*/);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        owner_.Root.TraceId,
                        "ConfigureFabricInstallerService returned error {0}",
                        error);

                    this->SendReplyAndCompleteOperation(thisSPtr, ErrorCode(error.ReadValue(), wformatString(Constants::FabricUpgrade::ErrorMessageFormat, StringResource::Get(IDS_HOSTING_FabricInstallerService_Setup_Failed), error)));
                    return;
                }
            }
            else
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Error {0} while getting the state of Fabric installer service",
                    error);

                this->SendReplyAndCompleteOperation(thisSPtr, ErrorCode(error.ReadValue(), wformatString(Constants::FabricUpgrade::ErrorMessageFormat, StringResource::Get(IDS_HOSTING_FabricInstallerService_Setup_Failed), error)));
                return;
            }
        }
        else
        {
            if (SERVICE_STOPPED != serviceState)
            {
                error = sc.WaitForServiceStop(timeoutHelper_.GetRemainingTime());
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        owner_.Root.TraceId,
                        "Error {0} while waiting for Fabric installer service to stop",
                        error);

                    this->SendReplyAndCompleteOperation(thisSPtr, ErrorCode(error.ReadValue(), wformatString(Constants::FabricUpgrade::ErrorMessageFormat, StringResource::Get(IDS_HOSTING_FabricInstallerService_Setup_Failed), error)));
                    return;
                }
            }

            // For MSI path, reuse old FabricInstallerSvc. CAB path installs new bits before upgrading.
            if (Path::GetExtension(downloadedFabricPackageLocation_) == L".cab")
            {
                error = this->ConfigureFabricInstallerService(true/*copyOnly*/);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        owner_.Root.TraceId,
                        "ConfigureFabricInstallerService returned error {0}",
                        error);

                    this->SendReplyAndCompleteOperation(thisSPtr, ErrorCode(error.ReadValue(), wformatString(Constants::FabricUpgrade::ErrorMessageFormat, StringResource::Get(IDS_HOSTING_FabricInstallerService_Setup_Failed), error)));
                    return;
                }
            }
        }

        // Start the service
        error = sc.Start();
        if (error.IsWin32Error(ERROR_SERVICE_ALREADY_RUNNING))
        {
            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Error {0} while starting Fabric installer service",
                error);

            this->SendReplyAndCompleteOperation(thisSPtr, ErrorCode(error.ReadValue(), wformatString(Constants::FabricUpgrade::ErrorMessageFormat, StringResource::Get(IDS_HOSTING_FabricInstallerService_Start_Failed), error)));
        }
        else
        {
            this->SendReplyAndCompleteOperation(thisSPtr, error);
        }
#endif
    }

    ErrorCode ConfigureFabricInstallerService(bool copyOnly)
    {
        //LINUXTODO
#if !defined(PLATFORM_UNIX)
        wstring fabricRoot(L"");
        auto error = FabricEnvironment::GetFabricRoot(fabricRoot);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Error {0} while getting fabric root",
                error);

            return error;
        }

        auto fabricInstallerServiceDestDir = Path::Combine(fabricRoot, Constants::FabricUpgrade::FabricInstallerServiceDirectoryName);
        auto fabricInstallerServiceExeDestPath = Path::Combine(fabricInstallerServiceDestDir, Constants::FabricUpgrade::FabricInstallerServiceExeName);
        if (Directory::Exists(downloadedFabricPackageLocation_))
        {
            wstring fabricInstallerServiceSourceDir = L"";
            fabricInstallerServiceSourceDir = Path::Combine(downloadedFabricPackageLocation_, Constants::FabricUpgrade::FabricInstallerServiceDirectoryName);
            // Assert because these should have been validated already during provision
            TESTASSERT_IF(!Directory::Exists(fabricInstallerServiceSourceDir), "Fabric Installation folder does not have Fabric installer service binaries.");
            auto fabricInstallerServiceExeSrcPath = Path::Combine(fabricInstallerServiceSourceDir, Constants::FabricUpgrade::FabricInstallerServiceExeName);
            TESTASSERT_IF(!File::Exists(fabricInstallerServiceExeSrcPath), "Fabric Installation folder does not have the installer service exe");

            if (!Directory::Exists(fabricInstallerServiceDestDir))
            {
                error = Directory::Create2(fabricInstallerServiceDestDir);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        owner_.Root.TraceId,
                        "Error {0} while creating directory {1}",
                        error,
                        fabricInstallerServiceDestDir);

                    return error;
                }
            }

            error = Directory::Copy(fabricInstallerServiceSourceDir, fabricInstallerServiceDestDir, true);
            if (!error.IsSuccess())
            {
                // [subiswal]TODO: Handle sharing violation errors and schedule copy at reboot
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Error {0} while copying file from {1} to {2}",
                    error,
                    fabricInstallerServiceSourceDir,
                    fabricInstallerServiceDestDir);

                return error;
            }
        }
        else // CAB
        {
            TESTASSERT_IF(Path::GetExtension(downloadedFabricPackageLocation_) != L".cab", "Package location does not point to CAB file.");
            TESTASSERT_IF(!File::Exists(downloadedFabricPackageLocation_), "CAB package does not exist at expected location.");

            // Delete old service directory if it exists
            if (Directory::Exists(fabricInstallerServiceDestDir))
            {
                error = Directory::Delete(fabricInstallerServiceDestDir, true); // recursive
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        owner_.Root.TraceId,
                        "Error {0} while deleting directory {1}",
                        error,
                        fabricInstallerServiceDestDir);

                    return error;
                }
            }

            // Extract Service directory directly to FabricRoot
            wstring filters[] = { Constants::FabricUpgrade::FabricInstallerServiceDirectoryName };
            vector<wstring> extractServiceFilter(filters, filters + ARRAYSIZE(filters));
            int errorCode = CabOperations::ExtractFiltered(downloadedFabricPackageLocation_, fabricRoot, extractServiceFilter, true);
            if (errorCode != S_OK)
            {
                error = ErrorCode::FromWin32Error(errorCode);
                WriteTrace(
                    error.ToLogLevel(),
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "ConfigureFabricInstallerService CabOperations::ExtractFiltered : Error:{0}",
                    error);

                return error;
            }

            // Asserts to verify post-installation contents
            bool directoryExists = Directory::Exists(fabricInstallerServiceDestDir);
            bool serviceExeExists = File::Exists(fabricInstallerServiceExeDestPath);
            TESTASSERT_IFNOT(directoryExists, "FabricInstallerSvc destination folder does not exist.");
            TESTASSERT_IFNOT(serviceExeExists, "Fabric Installation folder does not have the installer service exe");
            if (!directoryExists)
            {
                WriteError(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "ConfigureFabricInstallerService Service Directory does not exist at: {0}",
                    fabricInstallerServiceDestDir);

                return ErrorCodeValue::DirectoryNotFound;
            }
            if (!serviceExeExists)
            {
                WriteError(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "ConfigureFabricInstallerService Service Exe does not exist at: {0}",
                    fabricInstallerServiceExeDestPath);

                return ErrorCodeValue::FileNotFound;
            }
        }

        if (!copyOnly)
        {
            error = InstallFabricInstallerService(fabricInstallerServiceExeDestPath);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Error {0} while installing Fabric installer service",
                    error);
            }
        }

        return error;
#endif
    }

    ErrorCode InstallFabricInstallerService(wstring const & servicePath)
    {
        //LINUXTODO
#if !defined(PLATFORM_UNIX)
        WriteInfo(
            TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Installing Fabric installer service");
        wstring servicePathWithQuotes = wformatString("\"{0}\"", servicePath);
        ServiceController scm(Constants::FabricUpgrade::FabricInstallerServiceName);
        auto error = scm.InstallAuto(Constants::FabricUpgrade::FabricInstallerServiceDisplayName, servicePathWithQuotes);
        if (error.IsWin32Error(ERROR_SERVICE_EXISTS))
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Fabric installer service is already installed");
            return ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Error {0} while installing Fabric installer service",
                error);
            return error;
        }

        WriteInfo(
            TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Setting Fabric installer service description to '{0}'", Constants::FabricUpgrade::FabricInstallerServiceDescription);
        error = scm.SetServiceDescription(Constants::FabricUpgrade::FabricInstallerServiceDescription);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Error {0} while updating description of Fabric installer service",
                error);
            return error;
        }

        error = scm.SetStartupTypeDelayedAuto();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Error {0} while setting the start up type of Fabric installer service to 'Delayed Auto'",
                error);
            return error;
        }

        WriteInfo(
            TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Setting failure actions to restart for Fabric installer service");
        error = scm.SetServiceFailureActionsForRestart(
            Constants::FabricUpgrade::FabricInstallerServiceDelayInSecondsBeforeRestart,
            Constants::FabricUpgrade::FabricUpgradeFailureCountResetPeriodInDays);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Error {0} setting failure actions to restart for Fabric installer service",
                error);
        }

        return error;
#endif
    }

    void FinishActivate(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.ProcessActivatorObj->EndActivate(operation, activationContext_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "ProcessActivatorObj EndActivate failed with {0}",
                error);

            this->SendReplyAndCompleteOperation(operation->Parent, error);
        }
    }

    void OnProcessExit(AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & waitError, DWORD processExitCode)
    {
        if (!waitError.IsSuccess())
        {
            WriteInfo(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "OnProcessExit: The error passed in is {0}",
                waitError);

            this->SendReplyAndCompleteOperation(thisSPtr, waitError);
            return;
        }

        WriteInfo(
            TraceType_ActivationManager,
            owner_.Root.TraceId,
            "OnProcessExit: The process exit code is {0}",
            processExitCode);

        auto result = ErrorCode::FromWin32Error(processExitCode);
        this->SendReplyAndCompleteOperation(thisSPtr, result);
    }

    void SendReplyAndCompleteOperation(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        if (TryStartComplete())
        {
            auto replyBody = make_unique<FabricHostOperationReply>(error);
            MessageUPtr reply = make_unique<Message>(*replyBody);
            context_->Reply(move(reply));

            FinishComplete(thisSPtr, error);
        }
    }

private:
    IProcessActivationContextSPtr activationContext_;
    TimeoutHelper timeoutHelper_;
    bool const useFabricInstallerService_;
    wstring const programToRun_;
    wstring const arguments_;
    wstring const downloadedFabricPackageLocation_;
    ProcessActivationManager & owner_;
    IpcReceiverContextUPtr context_;
};

class ProcessActivationManager::DownloadContainerImagesAsyncOperation : public AsyncOperation
{
public:
    DownloadContainerImagesAsyncOperation(
        vector<ContainerImageDescription> const & images,
        ProcessActivationManager & owner,
        TimeSpan const timeout,
        Transport::IpcReceiverContextUPtr && context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) :
        AsyncOperation(callback, parent),
        images_(images),
        owner_(owner),
        timeoutHelper_(timeout),
        timer_(),
        context_(move(context))
    {
    }

    static ErrorCode DownloadContainerImagesAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadContainerImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        DownloadContainerImages(thisSPtr);
    }

    void DownloadContainerImages(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.containerActivator_->BeginDownloadImages(
            images_,
            HostingConfig::GetConfig().ContainerImageDownloadTimeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDownloadContainerImagesCompleted(operation, false);
        },
            thisSPtr);
        OnDownloadContainerImagesCompleted(operation, true);
    }

    void OnDownloadContainerImagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.containerActivator_->EndDownloadImages(operation);
        owner_.SendFabricActivatorOperationReply(error, context_);
        TryComplete(operation->Parent, error);

    }

private:
    ProcessActivationManager & owner_;
    vector<ContainerImageDescription> images_;
    TimeoutHelper timeoutHelper_;
    Common::TimerSPtr timer_;
    Transport::IpcReceiverContextUPtr context_;
};

class ProcessActivationManager::DeleteContainerImagesAsyncOperation : public AsyncOperation
{
public:
    DeleteContainerImagesAsyncOperation(
        vector<wstring> const & images,
        ProcessActivationManager & owner,
        TimeSpan const timeout,
        Transport::IpcReceiverContextUPtr && context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) :
        AsyncOperation(callback, parent),
        images_(images),
        owner_(owner),
        timeoutHelper_(timeout),
        timer_(),
        context_(move(context))
    {
    }

    static ErrorCode DeleteContainerImagesAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeleteContainerImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        DeleteContainerImages(thisSPtr);
    }

    void DeleteContainerImages(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.containerActivator_->BeginDeleteImages(
            images_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeleteContainerImagesCompleted(operation, false);
        },
            thisSPtr);
        OnDeleteContainerImagesCompleted(operation, true);
    }

    void OnDeleteContainerImagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.containerActivator_->EndDeleteImages(operation);
        owner_.SendFabricActivatorOperationReply(error, context_);
        TryComplete(operation->Parent, error);

    }

private:
    ProcessActivationManager & owner_;
    vector<wstring> images_;
    TimeoutHelper timeoutHelper_;
    Common::TimerSPtr timer_;
    Transport::IpcReceiverContextUPtr context_;
};

class ProcessActivationManager::GetResourceUsageAsyncOperation : public AsyncOperation
{
public:
    GetResourceUsageAsyncOperation(
        std::wstring const & nodeId,
        std::vector<wstring> const & hosts,
        ProcessActivationManager & owner,
        Transport::IpcReceiverContextUPtr && context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) :
        AsyncOperation(callback, parent),
        nodeId_(nodeId),
        hosts_(hosts),
        count_(),
        lastError_(ErrorCodeValue::Success),
        owner_(owner),
        context_(move(context))
        {
            count_.store((uint64)hosts_.size());
        }

    static ErrorCode GetResourceUsageAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<GetResourceUsageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto & hostId : hosts_)
        {
            ApplicationServiceSPtr appService;
            ErrorCode err = owner_.applicationServiceMap_->Get(nodeId_, hostId, appService);
            if (err.IsSuccess())
            {
                auto operation = appService->BeginMeasureResourceUsage(
                    [this, appService](AsyncOperationSPtr const & operation)
                    {
                        this->FinishMeasurement(appService, operation, false);
                    },
                    thisSPtr);
                FinishMeasurement(appService, operation, true);
            }
            else
            {
                uint64 pendingCount = --count_;
                if (pendingCount == 0)
                {
                    SendResponse();
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::UserServiceNotFound));
                }
            }
        }
    }

    void FinishMeasurement(ApplicationServiceSPtr appService,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        ResourceMeasurement resourceMeasurement;
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        auto error = appService->EndMeasureResourceUsage(operation, resourceMeasurement);
        DecrementAndCheckPendingOperations(appService, resourceMeasurement, operation->Parent, error);
    }

    void DecrementAndCheckPendingOperations(
        ApplicationServiceSPtr appService,
        ResourceMeasurement const & resourceMeasurement,
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        WriteNoise(
            TraceType_ActivationManager,
            owner_.Root.TraceId,
            "ApplicationService {0} resource usage {1} error {2}",
            appService->ApplicationServiceId,
            resourceMeasurement,
            error);

        AcquireWriteLock lock(lock_);

        uint64 pendingCount = --count_;

        if (!error.IsSuccess())
        {
            lastError_.ReadValue();
            lastError_ = error;
        }
        else
        {
            resources_[appService->ApplicationServiceId] = resourceMeasurement;
        }

        if (pendingCount == 0)
        {
            SendResponse();
            TryComplete(thisSPtr, lastError_);
        }
    }

    void SendResponse()
    {
        unique_ptr<ResourceMeasurementResponse> replyBody;
        replyBody = make_unique<ResourceMeasurementResponse>(move(resources_));
        MessageUPtr reply = make_unique<Message>(*replyBody);
        context_->Reply(move(reply));
    }

private:
    ProcessActivationManager & owner_;
    wstring nodeId_;
    atomic_uint64 count_;
    std::vector<wstring> hosts_;
    std::map<std::wstring, ResourceMeasurement> resources_;
    Common::RwLock lock_;
    ErrorCode lastError_;
    Transport::IpcReceiverContextUPtr context_;
};

class ProcessActivationManager::GetContainerInfoAsyncOperation : public AsyncOperation
{
public:
    GetContainerInfoAsyncOperation(
        wstring const & nodeId,
        wstring const & appServiceId,
        bool isServicePackageActivationModeExclusive,
        wstring const & containerInfoTypeString,
        wstring const & containerInfoArgsString,
        ProcessActivationManager & owner,
        TimeSpan const timeout,
        Transport::IpcReceiverContextUPtr && context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) :
        AsyncOperation(callback, parent),
        nodeId_(nodeId),
        appServiceId_(appServiceId),
        isServicePackageActivationModeExclusive_(isServicePackageActivationModeExclusive),
        containerInfoTypeString_(containerInfoTypeString),
        containerInfoArgsString_(containerInfoArgsString),
        owner_(owner),
        timeoutHelper_(timeout),
        timer_(),
        context_(move(context))
    {
    }

    static ErrorCode GetContainerInfoAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<GetContainerInfoAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ApplicationServiceSPtr appService;
        ErrorCode err = owner_.applicationServiceMap_->Get(nodeId_, appServiceId_, appService);
        if (!err.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to get application service {0} from services map. Error {1}",
                appServiceId_,
                err);

            SendGetContainerInfoReplyAndComplete(err, L"", thisSPtr);
            return;
        }

        ServiceModel::ContainerInfoType::Enum containerInfoType;
        if (!ServiceModel::ContainerInfoType::FromWString(containerInfoTypeString_, containerInfoType))
        {
            SendGetContainerInfoReplyAndComplete(ErrorCode(ErrorCodeValue::InvalidArgument), L"", thisSPtr);
            return;
        }

        if (!appService)
        {
            SendGetContainerInfoReplyAndComplete(ErrorCode(ErrorCodeValue::UserServiceNotFound), L"", thisSPtr);
            return;
        }
        else
        {
            switch (containerInfoType)
            {
                case ServiceModel::ContainerInfoType::Logs:
                    GetContainerLogs(appService->ContainerDescriptionObj, isServicePackageActivationModeExclusive_, thisSPtr);
                    break;
                case ServiceModel::ContainerInfoType::RawAPI:
                    CallContainerApi(appService->ContainerDescriptionObj, thisSPtr);
                    break;
                case ServiceModel::ContainerInfoType::Events:
                case ServiceModel::ContainerInfoType::Inspect:
                case ServiceModel::ContainerInfoType::Stats:
                    SendGetContainerInfoReplyAndComplete(ErrorCode(ErrorCodeValue::NotImplemented), L"", thisSPtr);
                    break;
                default:
                    SendGetContainerInfoReplyAndComplete(ErrorCode(ErrorCodeValue::InvalidArgument), L"", thisSPtr);
                    break;
            }
        }
    }

    void CallContainerApi(ContainerDescription containerDescription, AsyncOperationSPtr const & thisSPtr)
    {
        ServiceModel::ContainerInfoArgMap containerInfoArgMap;
        auto error = JsonHelper::Deserialize(containerInfoArgMap, containerInfoArgsString_);
        TESTASSERT_IF(!error.IsSuccess(), "Deserializing containerInfoArgsString failed - {0}", error);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "CallContainerApi({0}): ContainerInfoArgMap deserialization failed with {1}, input='{2}'",
                containerDescription.ContainerName,
                error,
                containerInfoArgsString_);

            SendGetContainerInfoReplyAndComplete(error, wstring(), thisSPtr);
            return;
        }

        auto httpVerb = containerInfoArgMap.GetValue(*StringConstants::HttpVerb);
        auto uriSuffix = containerInfoArgMap.GetValue(*StringConstants::UriPath);
        auto requestContentType = containerInfoArgMap.GetValue(*StringConstants::HttpContentType);
        auto requestBody = containerInfoArgMap.GetValue(*StringConstants::HttpRequestBody);
        auto operation = owner_.containerActivator_->BeginInvokeContainerApi(
            containerDescription,
            httpVerb,
            uriSuffix,
            requestContentType,
            requestBody,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                OnContainerApiCallCompleted(operation, false);
            },
            thisSPtr);
        OnContainerApiCallCompleted(operation, true);
    }

    void OnContainerApiCallCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) return;

        wstring result;
        auto error = owner_.containerActivator_->EndInvokeContainerApi(operation, result);
        SendGetContainerInfoReplyAndComplete(error, result, operation->Parent);
    }

    void GetContainerLogs(ContainerDescription containerDescriptionObj, bool isServicePackageActivationModeExclusive, AsyncOperationSPtr const & thisSPtr)
    {
        ServiceModel::ContainerInfoArgMap containerInfoArgMap;
        auto error = JsonHelper::Deserialize(containerInfoArgMap, containerInfoArgsString_);
        TESTASSERT_IF(!error.IsSuccess(), "Deserializing containerInfoArgsString failed - {0}", error);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "GetContainerLogs(): ContainerInfoArgMap deserialization failed: {0}",
                error);

            SendGetContainerInfoReplyAndComplete(error, wstring(), thisSPtr);
            return;
        }

        wstring tailArgString = containerInfoArgMap.GetValue(ContainerInfoArgs::Tail);
        int previous;
        StringUtility::ParseIntegralString<int, true>::Try(containerInfoArgMap.GetValue(ContainerInfoArgs::Previous), previous, 0);

        auto operation = owner_.containerActivator_->BeginGetLogs(
            containerDescriptionObj,
            isServicePackageActivationModeExclusive,
            tailArgString,
            previous,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetContainerInfoCompleted(operation, false);
        },
            thisSPtr);
        OnGetContainerInfoCompleted(operation, true);
    }

    void OnGetContainerInfoCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring containerInfo;
        auto error = owner_.containerActivator_->EndGetLogs(operation, containerInfo);
        SendGetContainerInfoReplyAndComplete(error, containerInfo, operation->Parent);
    }

    void SendGetContainerInfoReplyAndComplete(ErrorCode const & error, wstring const & containerInfo, AsyncOperationSPtr const & thisSPtr)
    {
        unique_ptr<GetContainerInfoReply> replyBody;
        replyBody = make_unique<GetContainerInfoReply>(containerInfo, error);
        MessageUPtr reply = make_unique<Message>(*replyBody);
        WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Sent GetContainerInfoReply: ContainerInfo {0}, error {1}",
            containerInfo,
            error);
        context_->Reply(move(reply));
        TryComplete(thisSPtr, error);
    }

private:
    ProcessActivationManager & owner_;
    wstring nodeId_;
    wstring appServiceId_;
    bool isServicePackageActivationModeExclusive_;
    wstring containerInfoTypeString_;
    wstring containerInfoArgsString_;
    TimeoutHelper timeoutHelper_;
    Common::TimerSPtr timer_;
    Transport::IpcReceiverContextUPtr context_;
};

class ProcessActivationManager::GetImagesAsyncOperation : public AsyncOperation
{
public:
    GetImagesAsyncOperation(
        ProcessActivationManager & owner,
        Transport::IpcReceiverContextUPtr && context,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        context_(move(context))
    {
    }

    static ErrorCode GetImagesAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<GetImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ContainerDescription containerDescription;

        auto operation = owner_.containerActivator_->BeginInvokeContainerApi(
            containerDescription,
            L"GET",
            L"/images/json",
            L"application/json",
            L"",
            HostingConfig::GetConfig().NodeAvailableContainerImagesTimeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnContainerApiGetImagesCompleted(operation, false);
        },
            thisSPtr);
        this->OnContainerApiGetImagesCompleted(operation, true);
    }

    void OnContainerApiGetImagesCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        wstring result;
        auto error = owner_.containerActivator_->EndInvokeContainerApi(operation, result);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "GetImages(): Failed communication with docker with error: {0}",
                error);
            SendGetImagesReplyAndComplete(std::vector<wstring>(), error, operation->Parent);
            return;
        }
        else
        {
            ContainerApiResponse containerApiResponse;
            error = JsonHelper::Deserialize(containerApiResponse, result);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "GetImages(): ContainerApiResponse deserialization failed with error: {0}",
                    error);

                SendGetImagesReplyAndComplete(std::vector<wstring>(), error, operation->Parent);
                return;
            }

            auto & containerApiResult = containerApiResponse.Result();
            std::vector<NodeAvailableImage> nodeAvailableImages;
            error = JsonHelper::Deserialize(nodeAvailableImages, containerApiResult.Body());
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "GetImages(): NodeAvailableImages deserialization failed with error: {0}",
                    error);

                SendGetImagesReplyAndComplete(std::vector<wstring>(), error, operation->Parent);
                return;
            }

            std::vector<wstring> images;
            for (auto & image : nodeAvailableImages)
            {
                for (auto & name : image.AvailableImages_)
                {
                    images.push_back(name);
                }
            }

            SendGetImagesReplyAndComplete(images, error, operation->Parent);
        }
    }

    void SendGetImagesReplyAndComplete(std::vector<wstring> const & images, ErrorCode const & error, AsyncOperationSPtr const & thisSPtr)
    {
        unique_ptr<GetImagesReply> replyBody;
        replyBody = make_unique<GetImagesReply>(images, error);
        MessageUPtr reply = make_unique<Message>(*replyBody);
        WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Sent GetImagesReply: Available node images {0}, error {1}",
            images,
            error);
        context_->Reply(move(reply));
        TryComplete(thisSPtr, error);
    }

private:
    ProcessActivationManager & owner_;
    Transport::IpcReceiverContextUPtr context_;
};

class ProcessActivationManager::GetOverlayNetworkDefinitionAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    GetOverlayNetworkDefinitionAsyncOperation(
        ProcessActivationManager & owner,
        wstring const & networkName,
        wstring const & parentId,
        wstring const & nodeName,
        wstring const & nodeIpAddress,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        networkName_(networkName),
        parentId_(parentId),
        nodeName_(nodeName),
        nodeIpAddress_(nodeIpAddress),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode GetOverlayNetworkDefinitionAsyncOperation::End(AsyncOperationSPtr const & operation, __out OverlayNetworkDefinitionSPtr & networkDefinition)
    {
        auto thisPtr = AsyncOperation::End<GetOverlayNetworkDefinitionAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            networkDefinition = move(thisPtr->networkDefinition_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ActivatorRequestorSPtr requestor;
        auto error = owner_.requestorMap_.Get(parentId_, requestor);
        if (error.IsSuccess())
        {
            MessageUPtr request = CreateNetworkAllocationRequest();
            auto operation = owner_.activationManager_.IpcServerObj.BeginRequest(
                move(request),
                requestor->CallbackAddress,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { OnGetOverlayNetworkDefinitionCompleted(operation, false); },
                thisSPtr);

            OnGetOverlayNetworkDefinitionCompleted(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    void OnGetOverlayNetworkDefinitionCompleted(AsyncOperationSPtr operation, bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.activationManager_.IpcServerObj.EndRequest(operation, reply);
        WriteInfo(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Get overlay network definition completed for Node={0} Network={1} with error={2}.",
            parentId_,
            networkName_,
            error);

        if (error.IsSuccess())
        {
            // process the reply
            Management::NetworkInventoryManager::NetworkAllocationResponseMessage networkAllocationResponse;
            if (!reply->GetBody<Management::NetworkInventoryManager::NetworkAllocationResponseMessage>(networkAllocationResponse))
            {
                error = ErrorCode::FromNtStatus(reply->Status);
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "GetBody<Management::NetworkInventoryManager::NetworkAllocationResponseMessage> failed: Message={0}, ErrorCode={1}",
                    *reply,
                    error);
                TryComplete(operation->Parent, error);
                return;
            }

            error = networkAllocationResponse.ErrorCode;

            if (error.IsSuccess())
            {
                // convert to network definition
                wstring subnet = wformatString("{0}/{1}", networkAllocationResponse.Subnet, networkAllocationResponse.Prefix);

                this->networkDefinition_ = make_shared<OverlayNetworkDefinition>(
                    networkAllocationResponse.NetworkName,
                    networkAllocationResponse.NetworkId,
                    Common::ContainerEnvironment::ContainerOverlayNetworkTypeName,
                    subnet,
                    networkAllocationResponse.Gateway,
                    networkAllocationResponse.Prefix,
                    networkAllocationResponse.OverlayId,
                    networkAllocationResponse.StartMacPoolAddress,
                    networkAllocationResponse.EndMacPoolAddress,
                    networkAllocationResponse.NodeIpAddress,
                    parentId_,
                    this->nodeName_,
                    networkAllocationResponse.IpMacAddressMapToBeAdded,
                    networkAllocationResponse.IpMacAddressMapToBeRemoved);

                std::wstring isolatedNetworkInterfaceName;
                error = FabricEnvironment::GetFabricIsolatedNetworkInterfaceName(isolatedNetworkInterfaceName);
                if (error.IsSuccess())
                {
                    this->networkDefinition_->NetworkAdapterName = isolatedNetworkInterfaceName;
                }
                else
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        owner_.Root.TraceId,
                        "GetFabricIsolatedNetworkInterfaceName failed: ErrorCode={0}",
                        error);
                }
            }
        }

        TryComplete(operation->Parent, error);
    }

    MessageUPtr CreateNetworkAllocationRequest()
    {
        NetworkAllocationRequestMessage networkAllocationRequest(
            this->networkName_,
            this->nodeName_,
            this->nodeIpAddress_);

        MessageUPtr request = make_unique<Message>(networkAllocationRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivatorClient));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::GetOverlayNetworkDefinition));

        WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Get overlay network definition request: Message={0}, Body={1}", *request, networkAllocationRequest);
            
        return move(request);
    }

private:
    ProcessActivationManager & owner_;
    OverlayNetworkDefinitionSPtr networkDefinition_;
    TimeoutHelper timeoutHelper_;
    wstring parentId_;
    wstring networkName_;
    wstring nodeName_;
    wstring nodeIpAddress_;
};

class ProcessActivationManager::DeleteOverlayNetworkDefinitionAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    DeleteOverlayNetworkDefinitionAsyncOperation(
        ProcessActivationManager & owner,
        wstring const & networkName,
        wstring const & parentId,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        networkName_(networkName),
        parentId_(parentId),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode DeleteOverlayNetworkDefinitionAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeleteOverlayNetworkDefinitionAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ActivatorRequestorSPtr requestor;
        auto error = owner_.requestorMap_.Get(parentId_, requestor);
        if (error.IsSuccess())
        {
            MessageUPtr request = CreateNetworkRemoveRequestMessage();
            auto operation = owner_.activationManager_.IpcServerObj.BeginRequest(
                move(request),
                requestor->CallbackAddress,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { OnDeleteOverlayNetworkDefinitionCompleted(operation, false); },
                thisSPtr);

            OnDeleteOverlayNetworkDefinitionCompleted(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    void OnDeleteOverlayNetworkDefinitionCompleted(AsyncOperationSPtr operation, bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.activationManager_.IpcServerObj.EndRequest(operation, reply);
        WriteInfo(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Delete overlay network definition completed for Node={0} Network={1} with error={2}.",
            parentId_,
            networkName_,
            error);

        if (error.IsSuccess())
        {
            // process the reply
            Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage networkRemoveResponse;
            if (!reply->GetBody<Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage>(networkRemoveResponse))
            {
                error = ErrorCode::FromNtStatus(reply->Status);
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "GetBody<Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage> failed: Message={0}, ErrorCode={1}",
                    *reply,
                    error);
                TryComplete(operation->Parent, error);
                return;
            }

            error = networkRemoveResponse.ErrorCode;
        }

        TryComplete(operation->Parent, error);
    }

    MessageUPtr CreateNetworkRemoveRequestMessage()
    {
        NetworkRemoveRequestMessage networkRemoveRequest(
            this->networkName_,
            this->parentId_);

        MessageUPtr request = make_unique<Message>(networkRemoveRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivatorClient));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DeleteOverlayNetworkDefinition));

        WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Delete overlay network definition request: Message={0}, Body={1}", *request, networkRemoveRequest);

        return move(request);
    }

private:
    ProcessActivationManager & owner_;
    TimeoutHelper timeoutHelper_;
    wstring parentId_;
    wstring networkName_;
};

class ProcessActivationManager::PublishNetworkTablesAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    PublishNetworkTablesAsyncOperation(
        ProcessActivationManager & owner,
        wstring const & networkName,
        wstring const & parentId,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        networkName_(networkName),
        parentId_(parentId),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode PublishNetworkTablesAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<PublishNetworkTablesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ActivatorRequestorSPtr requestor;
        auto error = owner_.requestorMap_.Get(parentId_, requestor);
        if (error.IsSuccess())
        {
            MessageUPtr request = CreatePublishNetworkTablesRequestMessage();
            auto operation = owner_.activationManager_.IpcServerObj.BeginRequest(
                move(request),
                requestor->CallbackAddress,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { OnPublishNetworkTablesRequestCompleted(operation, false); },
                thisSPtr);

            OnPublishNetworkTablesRequestCompleted(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    void OnPublishNetworkTablesRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.activationManager_.IpcServerObj.EndRequest(operation, reply);
        WriteInfo(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Publish network tables request completed for Node={0} Network={1} with error={2}.",
            parentId_,
            networkName_,
            error);

        if (error.IsSuccess())
        {
            // process the reply
            Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage publishNetworkTablesResponse;
            if (!reply->GetBody<Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage>(publishNetworkTablesResponse))
            {
                error = ErrorCode::FromNtStatus(reply->Status);
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "GetBody<Reliability::PublishNetworkTablesResponseMessage> failed: Message={0}, ErrorCode={1}",
                    *reply,
                    error);
                TryComplete(operation->Parent, error);
                return;
            }

            error = publishNetworkTablesResponse.ErrorCode;
        }

        TryComplete(operation->Parent, error);
    }

    MessageUPtr CreatePublishNetworkTablesRequestMessage()
    {
        PublishNetworkTablesMessageRequest publishNetworkTablesRequest(
            this->networkName_,
            this->parentId_);

        MessageUPtr request = make_unique<Message>(publishNetworkTablesRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivatorClient));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::PublishNetworkTablesRequest));

        WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Publish network tables request: Message={0}, Body={1}", *request, publishNetworkTablesRequest);

        return move(request);
    }

private:
    ProcessActivationManager & owner_;
    TimeoutHelper timeoutHelper_;
    wstring parentId_;
    wstring networkName_;
};

ProcessActivationManager::ProcessActivationManager(
    ComponentRoot const & root,
    FabricActivationManager const & activationManager)
    : RootedObject(root)
    , activationManager_(activationManager)
    , isSystem_(false)
    , principalsProvider_()
    , endpointProvider_()
    , requestorMap_()
    , crashDumpConfigurationManager_()
    , registryAccessLock_()
    , firewallProvider_()
    , statisticsIntervalTimerSPtr_()
{
    auto map = make_shared<ApplicationServiceMap>();
    applicationServiceMap_ = move(map);
    auto principalsprovider = make_unique<SecurityPrincipalsProvider>(root);
    principalsProvider_ = move(principalsprovider);
    auto endpointProvider = make_unique<HttpEndpointSecurityProvider>(root);
    endpointProvider_ = move(endpointProvider);
    auto crashDumpConfigurationManager = make_unique<CrashDumpConfigurationManager>(root);
    crashDumpConfigurationManager_ = move(crashDumpConfigurationManager);
    auto firewallProvider = make_unique<FirewallSecurityProvider>(root);
    firewallProvider_ = move(firewallProvider);

    auto containerActivator = make_unique<ContainerActivator>(
        root,
        *this,
        [this](wstring const & appHostId, wstring const & nodeId, DWORD exitCode)
        {
            this->OnContainerTerminated(appHostId, nodeId, exitCode);
        },
        [this](DWORD exitCode, ULONG continuousFailureCount, TimeSpan const& nextStartTime)
        {
            this->OnContainerEngineTerminated(exitCode, continuousFailureCount, nextStartTime);
        },
        [this](wstring const & nodeId, std::vector<ContainerHealthStatusInfo> const & healthStatusInfoList)
        {
            this->OnContainerHealthCheckStatusEvent(nodeId, healthStatusInfoList);
        });

    containerActivator_ = move(containerActivator);
}

ProcessActivationManager::~ProcessActivationManager()
{
}

Common::AsyncOperationSPtr ProcessActivationManager::OnBeginOpen(
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

Common::ErrorCode ProcessActivationManager::OnEndOpen(Common::AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ProcessActivationManager::OnBeginClose(
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

Common::ErrorCode ProcessActivationManager::OnEndClose(Common::AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void ProcessActivationManager::OnAbort()
{
    WriteInfo(TraceType_ActivationManager,
        Root.TraceId,
        "Aborting ProcessActivationManager");
    this->principalsProvider_->Abort();
    vector<ApplicationServiceSPtr> appServices = this->applicationServiceMap_->Close();
    for (auto iter = appServices.begin(); iter != appServices.end(); iter++)
    {
        (*iter)->AbortAndWaitForTermination();
    }
    this->endpointProvider_->Abort();
    this->crashDumpConfigurationManager_->Abort();
    this->firewallProvider_->Abort();
    this->containerActivator_->Abort();

    if (this->statisticsIntervalTimerSPtr_)
    {
        this->statisticsIntervalTimerSPtr_->Cancel();
    }
}

void ProcessActivationManager::OnApplicationServiceTerminated(
    DWORD processId,
    DateTime const & startTime,
    ErrorCode const & waitResult,
    DWORD exitCode,
    wstring const & parentId,
    wstring const & appServiceId,
    wstring const & exePath)
{
    UNREFERENCED_PARAMETER(waitResult);

    LONG state = this->State.Value;
    if (state > FabricComponentState::Opened)
    {
        WriteNoise(
            TraceType_ActivationManager,
            Root.TraceId,
            "Ignoring termination of ApplicationService: AppServiceId={0} for parent process {1} as current state is {2}",
            appServiceId,
            parentId,
            state);
        return;
    }

    WriteTrace(
        (exitCode == 0 || exitCode == ProcessActivator::ProcessDeactivateExitCode || exitCode == ERROR_CONTROL_C_EXIT || exitCode == STATUS_CONTROL_C_EXIT) ? LogLevel::Info : LogLevel::Warning,
        TraceType_ActivationManager,
        Root.TraceId,
        "Application service {0} with processId {1} for parent process {2} terminated with exit code {3}",
        appServiceId,
        processId,
        parentId,
        exitCode);

    if (!(exitCode == 0 || exitCode == ProcessActivator::ProcessDeactivateExitCode || exitCode == ERROR_CONTROL_C_EXIT))
    {
        hostingTrace.ProcessUnexpectedTermination(parentId, appServiceId, exitCode, Path::GetFileName(exePath.c_str()), processId, startTime);
    }

    ApplicationServiceSPtr appService = nullptr;
    ErrorCode err = applicationServiceMap_->Get(parentId, appServiceId, appService);
    WriteTrace(
        err.ToLogLevel(),
        TraceType_ActivationManager,
        this->Root.TraceId,
        "Check if servicemap contains service with Id {0}. error {1}",
        appServiceId,
        err);

    if (err.IsSuccess() &&
        appService)
    {
        if (HostingConfig::GetConfig().EnableProcessDebugging &&
            !appService->IsApplicationHostProcess(processId))
        {
            WriteInfo(
                TraceType_ActivationManager,
                this->Root.TraceId,
                " Ignoring termination notification for processId {0} and service {1} since its not apphost process",
                processId,
                appServiceId);
            appService->ResumeProcessIfNeeded();
            return;
        }
        err = applicationServiceMap_->Remove(parentId, appServiceId);
        WriteTrace(err.ToLogLevel(),
            TraceType_ActivationManager,
            this->Root.TraceId,
            "Removing service with Id {0} from servicemap returned error {1}",
            appServiceId,
            err);

        this->SendApplicationServiceTerminationNotification(parentId, appServiceId, exitCode, ActivityDescription(ActivityId(), ActivityType::Enum::ServicePackageEvent), appService->IsContainerRoot);
    }
    else if (!err.IsError(ErrorCodeValue::ObjectClosed))
    {
        WriteInfo(TraceType_ActivationManager,
            this->Root.TraceId,
            "Failed to get Service with Id {0} from servicemap. Error {1}, sending notification regardless",
            appServiceId,
            err);
        this->SendApplicationServiceTerminationNotification(parentId, appServiceId, exitCode, ActivityDescription(ActivityId(), ActivityType::Enum::ServicePackageEvent), false);
    }
}

void ProcessActivationManager::SendApplicationServiceTerminationNotification(
    wstring const & parentId,
    wstring const & appServiceId,
    DWORD exitCode,
    ActivityDescription const & activityDescription,
    bool isRootContainer)
{
    WriteInfo(TraceType_ActivationManager,
        this->Root.TraceId,
        "Sending ApplicationService host down notification for apphost {0}, node {1}, activityDescription {2}",
        appServiceId,
        parentId,
        activityDescription);

    vector<wstring> appIds;
    appIds.push_back(appServiceId);
    SendApplicationServiceTerminationNotification(
        activityDescription,
        parentId,
        appIds,
        exitCode,
        isRootContainer);
}

void ProcessActivationManager::SendApplicationServiceTerminationNotification(
    ActivityDescription const & activityDescription,
    wstring const & parentId,
    vector<wstring> const & appServiceId,
    DWORD exitCode,
    bool isRootContainer)
{
    auto operation = AsyncOperation::CreateAndStart<NotifyServiceTerminationAsyncOperation>(
        *this,
        activityDescription,
        parentId,
        appServiceId,
        exitCode,
        isRootContainer,
        [this](AsyncOperationSPtr const & operation) { this->FinishSendServiceTerminationMessage(operation, false); },
        this->Root.CreateAsyncOperationRoot());
    FinishSendServiceTerminationMessage(operation, true);
}

void ProcessActivationManager::FinishSendServiceTerminationMessage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }
    MessageUPtr reply;
    auto error = NotifyServiceTerminationAsyncOperation::End(operation);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "AppService termination notification failed. Error {0}", error);
    }
}

void ProcessActivationManager::OnContainerHealthCheckStatusEvent(
    wstring const & nodeId,
    vector<ContainerHealthStatusInfo> const & healthStatusInfoList)
{
    this->SendContainerHealthCheckStatusNotification(nodeId, healthStatusInfoList);
}

void ProcessActivationManager::OnContainerActivatorServiceTerminated()
{
    containerActivator_->OnContainerActivatorServiceTerminated();
}

void ProcessActivationManager::SendContainerHealthCheckStatusNotification(
    wstring const & nodeId,
    vector<ContainerHealthStatusInfo> const & healthStatusInfoList)
{
    auto operation = AsyncOperation::CreateAndStart<NotifyContainerHealthCheckStatusAsyncOperation>(
        *this,
        nodeId,
        healthStatusInfoList,
        [this](AsyncOperationSPtr const & operation) { this->FinishSendContainerHealthCheckStatusMessage(operation, false); },
        this->Root.CreateAsyncOperationRoot());

    FinishSendContainerHealthCheckStatusMessage(operation, true);
}

void ProcessActivationManager::FinishSendContainerHealthCheckStatusMessage(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = NotifyContainerHealthCheckStatusAsyncOperation::End(operation);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Container HealthCheck status notification failed. Error {0}", error);
    }
}

Common::AsyncOperationSPtr ProcessActivationManager::BeginGetOverlayNetworkDefinition(
    std::wstring const & networkName,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetOverlayNetworkDefinitionAsyncOperation>(
        *this,
        networkName,
        nodeId,
        nodeName,
        nodeIpAddress,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ProcessActivationManager::EndGetOverlayNetworkDefinition(
    Common::AsyncOperationSPtr const & operation,
    OverlayNetworkDefinitionSPtr & networkDefinition)
{
    return GetOverlayNetworkDefinitionAsyncOperation::End(operation, networkDefinition);
}

Common::AsyncOperationSPtr ProcessActivationManager::BeginDeleteOverlayNetworkDefinition(
    std::wstring const & networkName,
    std::wstring const & nodeId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeleteOverlayNetworkDefinitionAsyncOperation>(
        *this,
        networkName,
        nodeId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ProcessActivationManager::EndDeleteOverlayNetworkDefinition(
    Common::AsyncOperationSPtr const & operation)
{
    return DeleteOverlayNetworkDefinitionAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ProcessActivationManager::BeginPublishNetworkTablesRequest(
    std::wstring const & networkName,
    std::wstring const & nodeId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<PublishNetworkTablesAsyncOperation>(
        *this,
        networkName,
        nodeId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ProcessActivationManager::EndPublishNetworkTablesRequest(
    Common::AsyncOperationSPtr const & operation)
{
    return PublishNetworkTablesAsyncOperation::End(operation);
}

bool ProcessActivationManager::IsRequestorRegistered(wstring const & requestorId)
{
    bool contains;

    ErrorCode err = requestorMap_.Contains(requestorId, contains);
    if (!err.IsSuccess())
    {
        WriteWarning(TraceType_ActivationManager,
            Root.TraceId,
            "Failed to check if requestor Id {0} is present in map. Error {1}",
            requestorId,
            err);
        return false;
    }
    return contains;
}

ErrorCode ProcessActivationManager::CheckAddParentProcessMonitoring(
    DWORD parentId,
    wstring const & nodeId,
    wstring const & callbackAddress,
    uint64 nodeInstanceId)
{
    ActivatorRequestorSPtr requestor = nullptr;
    {
        AcquireWriteLock lock(lock_);

        ErrorCode err = requestorMap_.Get(nodeId, requestor);
        if (err.IsSuccess() &&
            requestor)
        {
            WriteWarning(TraceType_ActivationManager,
                Root.TraceId,
                "Process with Id {0} and node id {1} is already monitored. Current requestor process id is {2}",
                requestor->ProcessId,
                nodeId,
                parentId);
            return ErrorCode(ErrorCodeValue::AlreadyExists);
        }
        if (!err.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(TraceType_ActivationManager,
                Root.TraceId,
                "Failed to check if parentprocess Id {0} is present in map. Error {1}",
                parentId,
                err);
            return err;
        }
        else
        {
            WriteInfo(TraceType_ActivationManager,
                Root.TraceId,
                "Add process monitoring for node with Id {0} process id {1}",
                nodeId,
                parentId);
            requestor = make_shared<ActivatorRequestor>(
                ProcessActivationManagerHolder(*this, this->Root.CreateComponentRoot()),
                [this](DWORD parentId, wstring const & nodeId, ErrorCode waitResult, uint64 nodeInstanceId)
                {
                    this->OnParentProcessTerminated(parentId, nodeId, waitResult, nodeInstanceId);
                },
                ActivatorRequestorType::FabricNode,
                nodeId,
                callbackAddress,
                parentId,
                this->isSystem_,
                nodeInstanceId);

            err = requestorMap_.Add(nodeId, requestor);
            if (!err.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to add to requestorMap: Error {0} requestorId={1}",
                    err,
                    nodeId);
                return err;
            }
        }
    }
    auto err = requestor->Open();
    if (!err.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed to Open ActivatorRequestor: Error {0} requestorId={1}",
            err,
            nodeId);

        auto error = requestorMap_.Remove(nodeId, requestor);
        WriteTrace(error.ToLogLevel(),
            TraceType_ActivationManager,
            Root.TraceId,
            "Removing requestor with id {0} from map returned {1}",
            nodeId,
            error);
        //return the original error for open
        return err;
    }

    return ErrorCode(ErrorCodeValue::Success);
}


void ProcessActivationManager::OnParentProcessTerminated(
    DWORD parentId,
    wstring const & nodeId,
    ErrorCode const & waitResult,
    uint64 nodeInstanceId)
{
    LONG state = this->State.Value;
    if (state > FabricComponentState::Opened)
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Ignoring termination of Parent process: processId={0} for nodeId={1} as current state is {2}",
            parentId,
            nodeId,
            state);
        return;
    }

    WriteInfo(TraceType_ActivationManager,
        Root.TraceId,
        "Process with Id {0} and node Id {1} nodeInstanceId {2} terminated. Error {3}",
        parentId,
        nodeId,
        nodeInstanceId,
        waitResult);
    auto error = this->TerminateAndCleanup(nodeId, parentId, nodeInstanceId);

    WriteTrace(error.ToLogLevel(),
        TraceType_ActivationManager,
        Root.TraceId,
        "Termination and cleanup for processId {0} and node Id {1} nodeInstanceId {2} returned Error {3}",
        parentId,
        nodeId,
        nodeInstanceId,
        waitResult);
}

void ProcessActivationManager::RegisterIpcRequestHandler()
{
    auto root = this->Root.CreateComponentRoot();
    this->activationManager_.IpcServerObj.RegisterMessageHandler(
        Actor::FabricActivator,
        [this, root](MessageUPtr & message, IpcReceiverContextUPtr & context) { this->ProcessIpcMessage(*message, context); },
        false/*dispatchOnTransportThread*/);
}

void ProcessActivationManager::UnregisterIpcRequestHandler()
{
    this->activationManager_.IpcServerObj.UnregisterMessageHandler(Actor::FabricActivator);
}

void ProcessActivationManager::ProcessIpcMessage(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    wstring const & action = message.Action;
    WriteInfo(TraceType_ActivationManager,
        Root.TraceId,
        "Processing Ipc message with action {0}",
        action);
    if (action == Hosting2::Protocol::Actions::ActivateProcessRequest)
    {
        this->ProcessActivateProcessRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::DeactivateProcessRequest)
    {
        this->ProcessDeactivateProcessRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::TerminateProcessRequest)
    {
        this->ProcessTerminateProcessRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::RegisterFabricActivatorClient)
    {
        this->ProcessRegisterClientRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ConfigureSecurityPrincipalRequest ||
        action == Hosting2::Protocol::Actions::CleanupSecurityPrincipalRequest)
    {
        this->ProcessConfigureSecurityPrincipalRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::CleanupPortSecurity ||
        action == Hosting2::Protocol::Actions::SetupPortSecurity)
    {
        this->ProcessConfigureEndpointSecurityRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ConfigureCrashDumpRequest)
    {
        this->crashDumpConfigurationManager_->ProcessConfigureCrashDumpRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::UnregisterFabricActivatorClient)
    {
        this->ProcessUnregisterActivatorClientRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::FabricUpgradeRequest)
    {
        this->ProcessFabricUpgradeRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::SetupResourceACLs)
    {
        this->ProcessConfigureResourceACLsRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::CreateSymbolicLinkRequest)
    {
        this->ProcessCreateSymbolicLinkRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ConfigureSharedFolderACL)
    {
        this->ProcessConfigureSharedFolderRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ConfigureSMBShare)
    {
        this->ProcessConfigureSMBShareRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::CleanupApplicationLocalGroup)
    {
        this->ProcessRemoveApplicationGroupRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::AssignIpAddresses)
    {
        this->ProcessAssignIPAddressesRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ManageOverlayNetworkResources)
    {
        this->ProcessManageOverlayNetworkResourcesRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::UpdateOverlayNetworkRoutes)
    {
        this->ProcessUpdateOverlayNetworkRoutesRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::GetNetworkDeployedCodePackages)
    {
        this->ProcessGetDeployedNetworkCodePackagesRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::GetDeployedNetworks)
    {
        this->ProcessGetDeployedNetworksRequest(message, context);
    }
#if defined(PLATFORM_UNIX)
    else if (action == Hosting2::Protocol::Actions::DeleteApplicationFolder)
    {
        this->ProcessDeleteApplicationFolderRequest(message, context);
    }
#endif
    else if (action == Hosting2::Protocol::Actions::ConfigureContainerCertificateExport)
    {
        this->ProcessConfigureContainerCertificateExportRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::CleanupContainerCertificateExport)
    {
        this->ProcessCleanupContainerCertificateExportRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::DownloadContainerImages)
    {
        this->ProcessDownloadContainerImagesMessage(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::DeleteContainerImages)
    {
        this->ProcessDeleteContainerImagesMessage(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ConfigureNodeForDnsService)
    {
        this->ProcessConfigureNodeForDnsServiceMessage(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::SetupContainerGroup)
    {
        this->ProcessSetupContainerGroupRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ConfigureEndpointCertificatesAndFirewallPolicy)
    {
        this->ProcessConfigureEndpointCertificatesAndFirewallPolicyRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::GetContainerInfo)
    {
        this->ProcessGetContainerInfoMessage(message, context);
    }
    else if (action == Management::ResourceMonitor::ResourceMeasurementRequest::ResourceMeasureRequestAction)
    {
        this->ProcessGetResourceUsage(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::GetImages)
    {
        this->ProcessGetImagesMessage(message, context); // Process request to get available images on node
    }
    else
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Dropping unsupported message: {0}",
            message);
    }
}

void ProcessActivationManager::ProcessActivateProcessRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ActivateProcessRequest request;

    if (!message.GetBody<ActivateProcessRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    if (this->IsRequestorRegistered(request.NodeId))
    {
        wstring appServiceId = request.ApplicationServiceId;

        WriteNoise(
            TraceType_ActivationManager,
            Root.TraceId,
            "Processing activate request for NodeId {0}, application Id {1}, applicationService Id {2}, exe {3}",
            request.NodeId,
            request.ApplicationId,
            request.ApplicationServiceId,
            request.ProcDescription.ExePath);

        auto operation = AsyncOperation::CreateAndStart<ActivateServiceAsyncOperation>(
            *this,
            request,
            TimeSpan::FromTicks(request.TimeoutInTicks),
            context,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnActivateServiceCompleted(operation);
        },
            this->Root.CreateAsyncOperationRoot());
    }
    else
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Dropping activate process request for AppServiceId {0} for Application {1} from Node {2} since requestor registration check failed",
            request.ApplicationServiceId,
            request.ApplicationId,
            request.NodeId);
    }
}

void ProcessActivationManager::OnActivateServiceCompleted(AsyncOperationSPtr const & operation)
{
    auto error = ActivateServiceAsyncOperation::End(operation);
    WriteTrace(error.ToLogLevel(),
        TraceType_ActivationManager,
        "Activate application service returned {0}",
        error);
}

void ProcessActivationManager::OnContainerGroupSetupCompleted(AsyncOperationSPtr const & operation)
{
    auto error = ActivateContainerGroupRootAsyncOperation::End(operation);
    WriteTrace(error.ToLogLevel(),
        TraceType_ActivationManager,
        "Activate application service returned {0}",
        error);
}

void ProcessActivationManager::ProcessDeactivateProcessRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    DeactivateProcessRequest request;

    if (!message.GetBody<DeactivateProcessRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }
    if (this->IsRequestorRegistered(request.ParentId))
    {
        wstring appServiceId = request.ApplicationServiceId;
        TimeSpan timeout = TimeSpan::FromTicks(request.TimeoutInTicks);

        WriteNoise(
            TraceType_ActivationManager,
            Root.TraceId,
            "Processing deactivate request for NodeId {0}, applicationService Id {1}",
            request.ParentId,
            request.ApplicationServiceId);

        auto operation = AsyncOperation::CreateAndStart<DeactivateServiceAsyncOperation>(
            *this,
            request.ParentId,
            request.ApplicationServiceId,
            timeout,
            context,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeactivateServiceCompleted(operation);
        },
            this->Root.CreateAsyncOperationRoot());
    }
    else
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Dropping deactivate process request for AppServiceId {0} from Node {1} since requestor registration check failed",
            request.ApplicationServiceId,
            request.ParentId);
    }
}

void ProcessActivationManager::OnDeactivateServiceCompleted(AsyncOperationSPtr const & operation)
{
    auto error = DeactivateServiceAsyncOperation::End(operation);
    WriteTrace(error.ToLogLevel(),
        TraceType_ActivationManager,
        "Deactivate application service returned {0}",
        error);
}

void ProcessActivationManager::ProcessTerminateProcessRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    DeactivateProcessRequest request;

    if (!message.GetBody<DeactivateProcessRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }
    wstring appServiceId = request.ApplicationServiceId;

    SendDeactivateProcessReply(request.ApplicationServiceId, ErrorCode(ErrorCodeValue::Success), context);

    ApplicationServiceSPtr appService;
    auto err = applicationServiceMap_->Get(request.ParentId, appServiceId, appService);
    if (!err.IsSuccess())
    {
        WriteWarning(TraceType_ActivationManager,
            Root.TraceId,
            "Failed to get application service {0} from services map. Error {1}",
            appServiceId,
            err);
        return;
    }
    if (!appService)
    {
        WriteWarning(TraceType_ActivationManager,
            Root.TraceId,
            "Did not find application service {0} in services map.",
            appServiceId);
    }
    err = applicationServiceMap_->Remove(request.ParentId, appServiceId);
    if (!err.IsSuccess())
    {
        WriteWarning(TraceType_ActivationManager,
            Root.TraceId,
            "Failed to remove application service {0} from services map. Error {1}",
            appServiceId,
            err);
        return;
    }
    
    WriteInfo(
        TraceType_ActivationManager,
        Root.TraceId,
        "Aborting application service {0} associated with parent {1}",
        appServiceId,
        appService->ParentId);

    appService->AbortAndWaitForTermination();

    SendApplicationServiceTerminationNotification(
        request.ParentId,
        appServiceId, 
        ProcessActivator::ProcessAbortExitCode, 
        ActivityDescription(ActivityId(), ActivityType::Enum::ServicePackageEvent),
        appService->IsContainerRoot);
    return;
}

void ProcessActivationManager::ProcessRegisterClientRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    RegisterFabricActivatorClientRequest request;

    if (!message.GetBody<RegisterFabricActivatorClientRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    WriteInfo(TraceType_ActivationManager,
        Root.TraceId,
        "Register fabricactivatorclient with process id {0} and node id {1}",
        request.ParentProcessId,
        request.NodeId);

    auto error = this->CheckAddParentProcessMonitoring(request.ParentProcessId, request.NodeId, context->From, request.NodeInstanceId);

    SendRegisterClientReply(error, context);
}

void ProcessActivationManager::SendRegisterClientReply(
    ErrorCode const & error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    unique_ptr<RegisterFabricActivatorClientReply> replyBody;
    replyBody = make_unique<RegisterFabricActivatorClientReply>(error);
    MessageUPtr reply = make_unique<Message>(*replyBody);
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sending RegisterFabricActivatorClient: Error ={0}",
        error);
    context->Reply(move(reply));
}

void ProcessActivationManager::ProcessConfigureSecurityPrincipalRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    vector<SecurityPrincipalInformationSPtr> principalsInfo;
    ErrorCode error(ErrorCodeValue::Success);
    if (message.Action == Hosting2::Protocol::Actions::ConfigureSecurityPrincipalRequest)
    {
        ConfigureSecurityPrincipalRequest request;

        if (!message.GetBody<ConfigureSecurityPrincipalRequest>(request))
        {
            error = ErrorCode::FromNtStatus(message.Status);
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Invalid message received: {0}, dropping, error {1}",
                message,
                error);
            return;
        }
        if (this->IsRequestorRegistered(request.NodeId))
        {
            error = this->principalsProvider_->Setup(move(request), principalsInfo);
        }
        else
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Dropping configuresecurityprincipal request for Application {0} from Node {1} since requestor registration check failed",
                request.ApplicationId,
                request.NodeId);
            return;
        }
        SendConfigureSecurityPrincipalReply(principalsInfo, error, context);
    }
    else if (message.Action == Hosting2::Protocol::Actions::CleanupSecurityPrincipalRequest)
    {
        CleanupSecurityPrincipalRequest cleanupRequest;
        if (!message.GetBody<CleanupSecurityPrincipalRequest>(cleanupRequest))
        {
            error = ErrorCode::FromNtStatus(message.Status);
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Invalid message received: {0}, dropping, error {1}",
                message,
                error);
            return;
        }

        for (auto iter = cleanupRequest.ApplicationIds.begin(); iter != cleanupRequest.ApplicationIds.end(); ++iter)
        {
            auto cleanupError = this->principalsProvider_->Cleanup(cleanupRequest.NodeId, *iter, cleanupRequest.IsCleanupForNode);
            if (!cleanupError.IsSuccess())
            {
                error.ReadValue();    // read the previous error
                error = cleanupError; // save the last error
            }

        }
        SendFabricActivatorOperationReply(error, context);
    }
}

void ProcessActivationManager::SendConfigureSecurityPrincipalReply(
    vector<SecurityPrincipalInformationSPtr> const & principalInformation,
    ErrorCode const & error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    auto replyBody = make_unique<ConfigureSecurityPrincipalReply>(principalInformation, error);
    MessageUPtr reply = make_unique<Message>(*replyBody);
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sending ConfigureSecurityPrincipalReply: Error={0}",
        error);
    context->Reply(move(reply));
}

void ProcessActivationManager::ProcessConfigureResourceACLsRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ErrorCode error(ErrorCodeValue::Success);
    if (message.Action == Hosting2::Protocol::Actions::SetupResourceACLs)
    {
        ConfigureResourceACLsRequest request;
        if (!message.GetBody<ConfigureResourceACLsRequest>(request))
        {
            error = ErrorCode::FromNtStatus(message.Status);
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Invalid message received: {0}, dropping",
                message);
            return;
        }

        TimeSpan remainingTime = TimeSpan::FromTicks(request.TimeoutTicks);
        TimeoutHelper timeoutHelper(remainingTime);

        WriteInfo(TraceType_ActivationManager,
            Root.TraceId,
            "Recieved ConfigureResourceACLRequest with Timeout {0}",
            remainingTime);

        if (!request.CertificateAccessDescriptions.empty())
        {
            error = ConfigureCertificateACLs(request.CertificateAccessDescriptions, timeoutHelper);
            if (!error.IsSuccess())
            {
                error = ErrorCode(error.ReadValue(),
                    wformatString(StringResource::Get(IDS_HOSTING_CertACLFailed), error.ErrorCodeValueToString()));
            }
        }

        if(error.IsSuccess() &&
            !request.ApplicationFolders.empty())
        {
            error = this->ConfigureFolderACLs(request.ApplicationFolders,
                request.AccessMask,
                request.SIDs,
                timeoutHelper);
            if (!error.IsSuccess())
            {
                error = ErrorCode(error.ReadValue(),
                    wformatString(StringResource::Get(IDS_HOSTING_FolderACLFailed), error.ErrorCodeValueToString()));
            }

        }
        SendFabricActivatorOperationReply(error, context);

        WriteInfo(TraceType_ActivationManager,
            Root.TraceId,
            "Completed ConfigureResourceACLRequest and replied");
    }
}

ErrorCode ProcessActivationManager::ConfigureCertificateACLs(
    vector<CertificateAccessDescription> const & certAccessDescriptions,
    TimeoutHelper const & timeoutHelper)
{
    for (auto iter = certAccessDescriptions.begin(); iter != certAccessDescriptions.end(); iter++)
    {
        vector<SidSPtr> sids;
        CertificateAccessDescription certAccess = *iter;

        for (auto userSidsIter = certAccess.UserSids.begin(); userSidsIter != certAccess.UserSids.end(); ++userSidsIter)
        {
            SidSPtr sid;
            auto error = BufferedSid::CreateSPtrFromStringSid(*userSidsIter, sid);
            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivationManager,
                Root.TraceId,
                "Creating Sid object for string {0}. ErrorCode={1}",
                *userSidsIter,
                error);
            if (!error.IsSuccess())
            {
                return error;
            }
            sids.push_back(sid);
        }

        auto error = SecurityUtility::SetCertificateAcls(
            certAccess.SecretsCertificate.X509FindValue,
            certAccess.SecretsCertificate.X509StoreName,
            certAccess.SecretsCertificate.X509FindType,
            sids,
            certAccess.AccessMask,
            timeoutHelper.GetRemainingTime());

        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivationManager,
            Root.TraceId,
            "ACLing private key filename for thumbprint {0}. ErrorCode={1}",
            certAccess.SecretsCertificate.X509FindValue,
            error);
        if (!error.IsSuccess())
        {
            return error;
        }
    }
    return ErrorCode(ErrorCodeValue::Success);
}

void ProcessActivationManager::SendFabricActivatorOperationReply(
    ErrorCode const & error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    unique_ptr<FabricHostOperationReply> replyBody;
    replyBody = make_unique<FabricHostOperationReply>(error, error.Message);
    MessageUPtr reply = make_unique<Message>(*replyBody);
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sent FabricActivatorOperation reply : Error={0}",
        error);
    context->Reply(move(reply));
}

void ProcessActivationManager::SendCleanupApplicationPrincipalsReply(
    ErrorCode const & error,
    vector<wstring> const & deletedAppIds,
    __in Transport::IpcReceiverContextUPtr & context)
{
    unique_ptr<CleanupApplicationPrincipalsReply> replyBody;
    replyBody = make_unique<CleanupApplicationPrincipalsReply>(error, deletedAppIds);
    MessageUPtr reply = make_unique<Message>(*replyBody);
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sent CleanupApplicationPrincipals reply : Error={0}",
        error);
    context->Reply(move(reply));
}

void ProcessActivationManager::SendConfigureContainerCertificateExportReply(
    ErrorCode const & error,
    map<std::wstring, std::wstring> certificatePaths,
    map<std::wstring, std::wstring> certificatePasswordPaths,
    __in Transport::IpcReceiverContextUPtr & context)
{
    unique_ptr<ConfigureContainerCertificateExportReply> replyBody;
    replyBody = make_unique<ConfigureContainerCertificateExportReply>(error, error.Message, certificatePaths, certificatePasswordPaths);
    MessageUPtr reply = make_unique<Message>(*replyBody);
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sent ConfigureContainerCertificateExport reply : Error={0}",
        error);
    context->Reply(move(reply));
}

#if defined(PLATFORM_UNIX)
void ProcessActivationManager::SendDeleteApplicationFoldersReply(
    ErrorCode const & error,
    vector<wstring> const & deletedAppFolders,
    __in Transport::IpcReceiverContextUPtr & context)
{
    unique_ptr<DeleteApplicationFoldersReply> replyBody;
    replyBody = make_unique<DeleteApplicationFoldersReply>(error, deletedAppFolders);
    MessageUPtr reply = make_unique<Message>(*replyBody);
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sent DeleteApplicationFoldersReply reply : Error={0}",
        error);
    context->Reply(move(reply));
}
#endif

ErrorCode ProcessActivationManager::ConfigureFolderACLs(
    vector<wstring> const & folderNames,
    DWORD const accessMask,
    vector<wstring> const & additionalSids,
    TimeoutHelper const & timeoutHelper)
{
    vector<SidSPtr> sids;
    ErrorCode error;
    for (auto iter = additionalSids.begin(); iter != additionalSids.end(); ++iter)
    {
        SidSPtr sid;
        error = BufferedSid::CreateSPtrFromStringSid(*iter, sid);
        if (!error.IsSuccess())
        {
            return error;
        }
        sids.push_back(sid);
    }
    for (auto const& folderName : folderNames)
    {
        error = SecurityUtility::UpdateFolderAcl(sids, folderName, accessMask, timeoutHelper.GetRemainingTime(), true);
        if (!error.IsSuccess())
        {
            WriteNoise(TraceType_ActivationManager,
                Root.TraceId,
                "UpdateFolderAcl failed : Error={0}",
                error);
            return error;
        }
    }
    return error;
}

void ProcessActivationManager::ProcessRemoveApplicationGroupRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    CleanupSecurityPrincipalRequest cleanupRequest;
    if (!message.GetBody<CleanupSecurityPrincipalRequest>(cleanupRequest))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    ErrorCode lastError(ErrorCodeValue::Success);

#if defined(PLATFORM_UNIX)
    SendCleanupApplicationPrincipalsReply(lastError, cleanupRequest.ApplicationIds, context);
    return;
#endif

    vector<wstring> deletedAppIds;

    for (auto iter = cleanupRequest.ApplicationIds.begin(); iter != cleanupRequest.ApplicationIds.end(); ++iter)
    {
        ServiceModel::ApplicationIdentifier appId;
        auto error = ApplicationIdentifier::FromString(*iter, appId);
        if (!error.IsSuccess())
        {
            SendFabricActivatorOperationReply(error, context);
            return;
        }

        wstring applicationGroupName = ApplicationPrincipals::GetApplicationLocalGroupName(cleanupRequest.NodeId, appId.ApplicationNumber);
        error = SecurityPrincipalHelper::DeleteGroupAccount(applicationGroupName);

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
        {
            lastError.Overwrite(error);
            WriteInfo(
                TraceType_ActivationManager,
                Root.TraceId,
                "Deletion of Application local group: {0}, error {1}",
                applicationGroupName,
                lastError);
        }
        else
        {
            WriteInfo(
                TraceType_ActivationManager,
                Root.TraceId,
                "Deleted Application local group: {0} for applicationId {1} error {2}",
                applicationGroupName,
                appId,
                error);
            deletedAppIds.push_back(*iter);
        }
    }

    SendCleanupApplicationPrincipalsReply(lastError, deletedAppIds, context);
    return;
}

#if defined(PLATFORM_UNIX)
void ProcessActivationManager::ProcessDeleteApplicationFolderRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    DeleteFolderRequest deleteRequest;
    if (!message.GetBody<DeleteFolderRequest>(deleteRequest))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    ErrorCode lastError(ErrorCodeValue::Success);

    vector<wstring> deletedFolders;

    for (auto iter = deleteRequest.ApplicationFolders.begin(); iter != deleteRequest.ApplicationFolders.end(); ++iter)
    {
        auto error = Directory::Delete(*iter, true, true);

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
        {
            lastError.Overwrite(error);
            WriteInfo(
                TraceType_ActivationManager,
                Root.TraceId,
                "Deletion of Application folder: {0}, error {1}",
                *iter,
                lastError);
        }
        else
        {
            WriteInfo(
                TraceType_ActivationManager,
                Root.TraceId,
                "Deleted Application folder: {0} error {1}",
                *iter,
                error);
            deletedFolders.push_back(*iter);
        }
    }

    SendDeleteApplicationFoldersReply(lastError, deletedFolders, context);
    return;
}
#endif

void ProcessActivationManager::ProcessConfigureEndpointSecurityRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ConfigureEndpointSecurityRequest request;
    if (!message.GetBody<ConfigureEndpointSecurityRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }
    if (message.Action == Hosting2::Protocol::Actions::SetupPortSecurity)
    {
        auto error = this->endpointProvider_->ConfigurePortAcls(
            request.PrincipalSid,
            request.Port,
            request.IsHttps,
            request.Prefix,
            request.ServicePackageId,
            request.NodeId,
            request.IsSharedPort);
        if (!error.IsSuccess())
        {
            error = ErrorCode(error.ReadValue(),
                wformatString(StringResource::Get(IDS_HOSTING_EndpointFirewallSetupFailed), error.ErrorCodeValueToString()));
        }
        SendFabricActivatorOperationReply(error, context);
    }
    else if (message.Action == Hosting2::Protocol::Actions::CleanupPortSecurity)
    {
        SendFabricActivatorOperationReply(
            this->endpointProvider_->CleanupPortAcls(
                request.PrincipalSid,
                request.Port,
                request.IsHttps,
                request.Prefix,
                request.ServicePackageId,
                request.NodeId,
                request.IsSharedPort),
            context);
    }
}

ErrorCode ProcessActivationManager::ConfigureFirewallPolicy(
    bool removeRule,
    wstring const & servicePackageId,
    std::vector<LONG> const & ports,
    uint64 nodeInstanceId)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (removeRule)
    {
        error = firewallProvider_->RemoveFirewallRule(servicePackageId, ports, nodeInstanceId);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivationManager,
            Root.TraceId,
            "Remove FirewallPolicy for ServicePackageId {0} returned {1}",
            servicePackageId,
            error);
    }
    else
    {
        error = firewallProvider_->ConfigurePortFirewallPolicy(servicePackageId, ports, nodeInstanceId);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to configure firewall ports for servicepackage id : {0}, error {1} removing policies",
                servicePackageId, error);
            firewallProvider_->RemoveFirewallRule(servicePackageId, ports, nodeInstanceId).ReadValue();
        }
    }
    error = ErrorCode(error.ReadValue(),
        wformatString(StringResource::Get(IDS_HOSTING_EndpointFirewallSetupFailed), error.ErrorCodeValueToString()));
    return error;
}

ErrorCode ProcessActivationManager::ConfigureEndpointCertificates(
    std::vector<EndpointCertificateBinding> const & endpointCertificateBindings,
    std::wstring const & servicePackageId,
    bool cleanUp,
    std::wstring const & nodeId)
{
    vector<EndpointCertificateBinding> successConfigured;
    ErrorCode error(ErrorCodeValue::Success);

    for (auto it = endpointCertificateBindings.begin(); it != endpointCertificateBindings.end(); it++)
    {
        WriteInfo(
            TraceType_ActivationManager,
            Root.TraceId,
            "Processing configure request for port certificate for port: {0}, certificatefindvalue {1} cleanup {2}",
            it->Port,
            it->X509FindValue,
            cleanUp);
        error = endpointProvider_->ConfigurePortCertificate(
            it->Port,
            it->PrincipalSid,
            it->IsExplicitPort,
            cleanUp,
            it->X509FindValue,
            it->X509StoreName,
            it->CertFindType,
            servicePackageId,
            nodeId);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to configure port certificate for port: {0}, certificatefindvalue {1}, error {2}",
                it->Port,
                it->X509FindValue,
                error);
            if (error.Message.empty())
            {
                error = ErrorCode(error.ReadValue(),
                    wformatString(StringResource::Get(IDS_HOSTING_EndpointBindingSetupFailed), it->X509FindValue, error.ErrorCodeValueToString()));
            }
            break;
        }
        if (!cleanUp)
        {
            successConfigured.push_back(*it);
        }
    }
    if (!cleanUp &&
        !error.IsSuccess())
    {
        for (auto it = successConfigured.begin(); it != successConfigured.end(); it++)
        {
            auto err = endpointProvider_->ConfigurePortCertificate(
                it->Port,
                it->PrincipalSid,
                it->IsExplicitPort,
                true,
                it->X509FindValue,
                it->X509StoreName,
                it->CertFindType,
                servicePackageId,
                nodeId);
            WriteTrace(
                err.ToLogLevel(),
                TraceType_ActivationManager,
                Root.TraceId,
                "Cleanup of  port certificate on error for port: {0}, certificatefindvalue {1}, error {2}",
                it->Port,
                it->X509FindValue,
                err);
        }
    }

    return error;
}

void ProcessActivationManager::ProcessConfigureEndpointCertificatesAndFirewallPolicyRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ConfigureEndpointBindingAndFirewallPolicyRequest request;
    if (!message.GetBody<ConfigureEndpointBindingAndFirewallPolicyRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    ErrorCode error, lastError;

    if (!request.FirewallPorts.empty())
    {
        error = this->ConfigureFirewallPolicy(
            request.CleanupFirewallPolicy,
            request.ServicePackageId,
            request.FirewallPorts,
            request.NodeInstanceId);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "ConfigureFirewallPolicy failed: cleanup {0} with error {1}",
                request.CleanupFirewallPolicy,
                error);

            lastError.Overwrite(error);

            //If it is a cleaunup operation we still want to cleanup the endpoint certificates.
            if (!request.Cleanup)
            {
                SendFabricActivatorOperationReply(lastError, context);
                return;
            }
        }
    }

    error = this->ConfigureEndpointCertificates(
        request.EndpointCertificateBindings,
        request.ServicePackageId,
        request.Cleanup,
        request.NodeId);

    if (!error.IsSuccess())
    {
        lastError.Overwrite(error);
    }

    SendFabricActivatorOperationReply(lastError, context);
}

void ProcessActivationManager::ProcessAssignIPAddressesRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    AssignIpAddressesRequest request;
    if (!message.GetBody<AssignIpAddressesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }
    vector<wstring> results;
    ErrorCode error(ErrorCodeValue::Success);
    if (request.Cleanup)
    {
        this->containerActivator_->ReleaseIPAddresses(request.NodeId, request.ServicePackageId);
    }
    else
    {
        error = this->containerActivator_->AllocateIPAddresses(request.NodeId, request.ServicePackageId, request.CodePackageNames, results);
    }
    unique_ptr<AssignIpAddressesReply> replyBody;
    replyBody = make_unique<AssignIpAddressesReply>(error, results);

    MessageUPtr reply = make_unique<Message>(*replyBody);
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sending AssignIPAddressReply: for NodeId {0} ServicePackageId={1}",
        request.NodeId,
        request.ServicePackageId);
    context->Reply(move(reply));
}

void ProcessActivationManager::ProcessManageOverlayNetworkResourcesRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ManageOverlayNetworkResourcesRequest request;
    if (!message.GetBody<ManageOverlayNetworkResourcesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    if (request.Action == ManageOverlayNetworkAction::Assign)
    {
        this->containerActivator_->BeginAssignOverlayNetworkResources(
            request.NodeId,
            request.NodeName,
            request.NodeIpAddress,
            request.ServicePackageId,
            request.CodePackageNetworkNames,
            HostingConfig::GetConfig().RequestTimeout,
            [this, request, ctx = context.release()](AsyncOperationSPtr const & operation)
        {
            std::map<std::wstring, std::map<std::wstring, std::wstring>> assignedNetworkResources;
            ErrorCode error(ErrorCodeValue::Success);
            error = this->containerActivator_->EndAssignOverlayNetworkResources(operation, assignedNetworkResources);
            unique_ptr<ManageOverlayNetworkResourcesReply> replyBody;
            replyBody = make_unique<ManageOverlayNetworkResourcesReply>(request.Action, error, assignedNetworkResources);

            MessageUPtr reply = make_unique<Message>(*replyBody);
            WriteNoise(TraceType_ActivationManager,
                Root.TraceId,
                "Sending ManageOverlayNetworkResourcesReply: for NodeId={0} ServicePackageId={1} Action={2}",
                request.NodeId,
                request.ServicePackageId,
                request.Action);

            ctx->Reply(move(reply));
        },
            this->Root.CreateAsyncOperationRoot());
    }
    else
    {
        std::set<wstring> networkNamesSet;
        for (auto const & cpn : request.CodePackageNetworkNames)
        {
            for (auto const & networkName : cpn.second)
            {
                networkNamesSet.insert(networkName);
            }
        }

        std::vector<wstring> networkNames;
        networkNames.assign(networkNamesSet.begin(), networkNamesSet.end());

        this->containerActivator_->BeginReleaseOverlayNetworkResources(
            request.NodeId,
            request.NodeName,
            request.NodeIpAddress,
            request.ServicePackageId,
            networkNames,
            HostingConfig::GetConfig().RequestTimeout,
            [this, request, ctx = context.release()](AsyncOperationSPtr const & operation)
        {
            ErrorCode error(ErrorCodeValue::Success);
            error = this->containerActivator_->EndReleaseOverlayNetworkResources(operation);
            unique_ptr<ManageOverlayNetworkResourcesReply> replyBody;
            replyBody = make_unique<ManageOverlayNetworkResourcesReply>(request.Action, error, std::map<wstring, std::map<std::wstring, std::wstring>>());

            MessageUPtr reply = make_unique<Message>(*replyBody);
            WriteNoise(TraceType_ActivationManager,
                Root.TraceId,
                "Sending ManageOverlayNetworkResourcesReply: for NodeId={0} ServicePackageId={1} Action={2}",
                request.NodeId,
                request.ServicePackageId,
                request.Action);

            ctx->Reply(move(reply));
        },
            this->Root.CreateAsyncOperationRoot());
    }
}

void ProcessActivationManager::ProcessUpdateOverlayNetworkRoutesRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    UpdateOverlayNetworkRoutesRequest request;
    if (!message.GetBody<UpdateOverlayNetworkRoutesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    std::vector<OverlayNetworkRouteSPtr> overlayNetworkRoutes;
    for (auto const & r : request.NetworkMappingTable)
    {
        auto overlayNetworkRoute = make_shared<OverlayNetworkRoute>(r->IpAddress, r->MacAddress, r->InfraIpAddress);
        overlayNetworkRoutes.push_back(overlayNetworkRoute);
    }

    auto overlayNetworkRoutingInformation = make_shared<OverlayNetworkRoutingInformation>(
        request.NetworkName,
        request.NodeIpAddress,
        request.InstanceID,
        request.SequenceNumber,
        request.IsDelta,
        overlayNetworkRoutes);

    this->containerActivator_->BeginUpdateRoutes(
        overlayNetworkRoutingInformation,
        HostingConfig::GetConfig().RequestTimeout,
        [this, request, ctx = context.release()](AsyncOperationSPtr const & operation)
    {
        ErrorCode error(ErrorCodeValue::Success);
        error = this->containerActivator_->EndUpdateRoutes(operation);
        unique_ptr<UpdateOverlayNetworkRoutesReply> replyBody;
        replyBody = make_unique<UpdateOverlayNetworkRoutesReply>(request.NetworkName, request.NodeIpAddress, error);

        MessageUPtr reply = make_unique<Message>(*replyBody);
        WriteNoise(TraceType_ActivationManager,
            Root.TraceId,
            "Sending UpdateOverlayNetworkRoutesReply: for NodeIp={0} NetworkName={1}",
            request.NodeIpAddress,
            request.NetworkName);

        ctx->Reply(move(reply));
    },
        this->Root.CreateAsyncOperationRoot());
}

void ProcessActivationManager::ProcessGetDeployedNetworkCodePackagesRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    GetNetworkDeployedPackagesRequest request;
    if (!message.GetBody<GetNetworkDeployedPackagesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    std::map<std::wstring, std::vector<std::wstring>> networkReservedCodePackages;
    ErrorCode error(ErrorCodeValue::Success);
    this->containerActivator_->GetDeployedNetworkCodePackages(
        request.ServicePackageIds,
        request.CodePackageName,
        request.NetworkName,
        __out networkReservedCodePackages
    );

    std::map<std::wstring, std::wstring> containerInfoMap;
    for (auto const & cp : request.CodePackageInstanceAppHostMap)
    {
        ApplicationServiceSPtr appService;
        auto appServiceGetError = applicationServiceMap_->Get(request.NodeId, cp.second, appService);
        if (appServiceGetError.IsSuccess())
        {
            // key is of format: "codePackageInstanceId,containerId"
            auto key = wformatString("{0},{1}", cp.first, appService->ContainerId);

            // assigned ips
            wstring assignedIpAddresses = L"";
            if (!appService->ContainerDescriptionObj.NetworkConfig.OpenNetworkAssignedIp.empty())
            {
                assignedIpAddresses = appService->ContainerDescriptionObj.NetworkConfig.OpenNetworkAssignedIp;
            }

            for (auto const & onr : appService->ContainerDescriptionObj.NetworkConfig.OverlayNetworkResources)
            {
                wstring ipAddress;
                wstring macAddress;
                StringUtility::SplitOnce(onr.second, ipAddress, macAddress, L",");
                if (assignedIpAddresses.empty())
                {
                    assignedIpAddresses = ipAddress;
                }
                else
                {
                    assignedIpAddresses = wformatString("{0},{1}", assignedIpAddresses, ipAddress);
                }
            }

            containerInfoMap[key] = assignedIpAddresses;
        }
    }

    unique_ptr<GetNetworkDeployedPackagesReply> replyBody;
    replyBody = make_unique<GetNetworkDeployedPackagesReply>(networkReservedCodePackages, containerInfoMap);
    MessageUPtr reply = make_unique<Message>(*replyBody);

    wstring servicePackageIdsStr = L"";
    for (auto const & servicePackageId : request.ServicePackageIds)
    {
        if (servicePackageIdsStr.empty())
        {
            servicePackageIdsStr = servicePackageId;
        }
        else
        {
            servicePackageIdsStr = wformatString("{0},{1}", servicePackageIdsStr, servicePackageId);
        }
    }

    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sending GetNetworkDeployedPackagesReply: for ServicePackageIds={0} CodePackageName={1} NetworkName={2}",
        servicePackageIdsStr,
        request.CodePackageName,
        request.NetworkName);

    context->Reply(move(reply));
}

void ProcessActivationManager::ProcessGetDeployedNetworksRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    GetDeployedNetworksRequest request;
    if (!message.GetBody<GetDeployedNetworksRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    std::vector<std::wstring> results;
    this->containerActivator_->GetDeployedNetworkNames(request.NetworkType, results);

    unique_ptr<GetDeployedNetworksReply> replyBody;
    replyBody = make_unique<GetDeployedNetworksReply>(results);
    MessageUPtr reply = make_unique<Message>(*replyBody);

    WriteNoise(TraceType_ActivationManager, 
        Root.TraceId, 
        "Sending GetDeployedNetworksReply");

    context->Reply(move(reply));
}

void ProcessActivationManager::SendDeactivateProcessReply(
    wstring const & appServiceId,
    Common::ErrorCode const & error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    unique_ptr<DeactivateProcessReply> replyBody;
    replyBody = make_unique<DeactivateProcessReply>(appServiceId, error);
    MessageUPtr reply = make_unique<Message>(*replyBody);
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Sending DeactivateProcessReply: for appServiceId={0}",
        appServiceId);
    context->Reply(move(reply));
}

void ProcessActivationManager::ProcessConfigureContainerCertificateExportRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ConfigureContainerCertificateExportRequest request;

    if (!message.GetBody<ConfigureContainerCertificateExportRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }
    std::map<std::wstring, std::wstring> certificatePaths;
    std::map<std::wstring, std::wstring> certificatePasswordPaths;
    ErrorCode error(ErrorCodeValue::Success);
    srand((unsigned int)time(NULL));
    for (auto iter = request.CertificateRef.begin(); iter != request.CertificateRef.end(); iter++)
    {
        auto codePackageName = iter->first;
        std::vector<ServiceModel::ContainerCertificateDescription> certDescriptions = iter->second;
        for (auto it = certDescriptions.begin(); it != certDescriptions.end(); it++)
        {
            SecureString password;
            SecureString passwordFileName;
            error = CryptoUtility::GenerateRandomString(passwordFileName);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Error in GenerateRandomString: {0}",
                    error);
                return;
            }

#if !defined(PLATFORM_UNIX)
            auto certFilePath = Path::Combine(request.WorkDirectoryPath, codePackageName + L"_" + it->Name + L"_PFX.pfx");
            auto passwordFilePath = Path::Combine(request.WorkDirectoryPath, passwordFileName.GetPlaintext() + L".txt");
            vector<wstring> machineAccountNamesForACL = { L"SYSTEM", L"Administrators" };
#else
            auto certFilePath = Path::Combine(request.WorkDirectoryPath, codePackageName + L"_" + it->Name + L"_PEM.pem");
            auto passwordFilePath = Path::Combine(request.WorkDirectoryPath, passwordFileName.GetPlaintext() + L".key");
            vector<wstring> machineAccountNamesForACL = { L"root" };
#endif

            if (!it->X509FindValue.empty())
            {
#if !defined(PLATFORM_UNIX)
                error = CryptoUtility::GenerateRandomString(password);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Error in GenerateRandomString: {0}",
                        error);
                    return;
                }

                error = Common::CertificateManager::GenerateAndACLPFXFromCer(
                    X509StoreLocation::LocalMachine,
                    it->X509StoreName,
                    it->X509FindValue,
                    certFilePath,
                    password,
                    machineAccountNamesForACL);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Error in GenerateAndACLPFXFromCer: {0}",
                        error);
                    return;
                }
#else
                wstring privKeyPath;
                error = CopyAndACLPEMFromStore(
                    it->X509FindValue,
                    it->X509StoreName,
                    certFilePath,
                    machineAccountNamesForACL,
                    privKeyPath);

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Error in CopyAndACLPEMFromStore: {0}",
                        error);
                    return;
                }

                File file;
                error = file.TryOpen(privKeyPath, FileMode::Open, FileAccess::Read, FileShare::None);
                if(!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Failed to open {0}. Error={1}.",
                        privKeyPath,
                        error);
                    return;
                }

                vector<wchar_t> buffer(file.size());
                DWORD bytesRead;
                error = file.TryRead2(reinterpret_cast<void*>(buffer.data()), file.size(), bytesRead);
                if(!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Failed to read {0}. Error={1}.",
                        privKeyPath,
                        error);
                    return;
                }
                file.Close();
                password = SecureString(move(wstring(buffer.data())));
#endif
            }
            else if(!it->DataPackageRef.empty())
            {
                error = CopyAndACLCertificateFromDataPackage(
                                 it->DataPackageRef,
                                 it->DataPackageVersion,
                                 it->RelativePath,
                                 certFilePath,
                                 machineAccountNamesForACL);
                if (!error.IsSuccess())
                {
                    WriteError(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Error in CopyCertificateFromDataPackage: {0}",
                        error);
                    return;
                }

                if (it->IsPasswordEncrypted)
                {
                    error = CryptoUtility::DecryptText(it->Password, password);
                    if (!error.IsSuccess())
                    {
                        WriteError(
                            TraceType_ActivationManager,
                            "Decrypt text failed: Error {0}",
                            error);
                        return;
                    }
                }
                else
                {
                    password = SecureString(move(it->Password));
                }
            }

            error = CryptoUtility::CreateAndACLPasswordFile(passwordFilePath, password, machineAccountNamesForACL);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Error in CreateAndACLPasswordFile: {0}",
                    error);
                return;
            }

            certificatePaths[codePackageName + L"_" + it->Name] = certFilePath;
            certificatePasswordPaths[codePackageName + L"_" + it->Name] = passwordFilePath;
        }
    }
    SendConfigureContainerCertificateExportReply(error, certificatePaths, certificatePasswordPaths, context);
}

void ProcessActivationManager::ProcessCleanupContainerCertificateExportRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    CleanupContainerCertificateExportRequest request;
    if (!message.GetBody<CleanupContainerCertificateExportRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
       return;
    }

    ErrorCode error(ErrorCodeValue::Success);
    auto passwordMap = request.CertificatePasswordPaths;

    for (auto it = request.CertificatePaths.begin(); it != request.CertificatePaths.end(); it++)
    {
        error = File::Delete2(it->second, true);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FileNotFound))
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to remove CertificatePaths resource file={0}. Error={1}.",
                it->second,
                error);
        }
        else
        {
            WriteInfo(
                TraceType_ActivationManager,
                Root.TraceId,
                "Successfully removed CertificatePaths resource file={0}. Error={1}.",
                it->second,
                error);
        }

        error = File::Delete2(passwordMap[it->first], true);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FileNotFound))
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to remove CertificatePasswordPaths resource file={0}. Error={1}.",
                passwordMap[it->first],
                error);
        }
        else
        {
            WriteInfo(
                TraceType_ActivationManager,
                Root.TraceId,
                "Successfully removed CertificatePasswordPaths resource file={0}. Error={1}.",
                passwordMap[it->first],
                error);
        }
    }

    SendFabricActivatorOperationReply(error, context);
}

void ProcessActivationManager::ProcessUnregisterActivatorClientRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    RegisterFabricActivatorClientRequest request;

    if (!message.GetBody<RegisterFabricActivatorClientRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }
    WriteInfo(
        TraceType_ActivationManager,
        Root.TraceId,
        "Unregister fabric activator client  message received for node: {0}",
        request.NodeId);

    auto error = this->TerminateAndCleanup(request.NodeId, request.ParentProcessId, request.NodeInstanceId);
    SendRegisterClientReply(error, context);
}

void ProcessActivationManager::ProcessFabricUpgradeRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    FabricUpgradeRequest request;
    if (!message.GetBody<FabricUpgradeRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    WriteNoise(
        TraceType_ActivationManager,
        Root.TraceId,
        "Begin(FabricUpgradeRequest): ProgramToRun: {0}, Arguments: {1}",
        request.ProgramToRun,
        request.Arguments);

    auto operation = AsyncOperation::CreateAndStart<FabricUpgradeAsyncOperation>(
        *this,
        request.UseFabricInstallerService,
        request.ProgramToRun,
        request.Arguments,
        request.DownloadedFabricPackageLocation,
        HostingConfig::GetConfig().ActivationTimeout,
        move(context),
        [this](AsyncOperationSPtr const & operation) { this->FinishFabricUpgradeRequest(operation, false); },
        this->Root.CreateAsyncOperationRoot());
}

void ProcessActivationManager::FinishFabricUpgradeRequest(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = FabricUpgradeAsyncOperation::End(operation);
    WriteTrace(
        error.ToLogLevel(),
        TraceType_ActivationManager,
        Root.TraceId,
        "End(FabricUpgradeRequest): Error:{0}",
        error);
}

void ProcessActivationManager::ProcessSetupContainerGroupRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    SetupContainerGroupRequest request;

    if (!message.GetBody<SetupContainerGroupRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    if (this->IsRequestorRegistered(request.NodeId))
    {
        TimeSpan timeout = TimeSpan::FromTicks(request.TimeoutInTicks);

        WriteNoise(
            TraceType_ActivationManager,
            Root.TraceId,
            "Processing SetupContainerGroup request {0}",
            request);

        auto operation = AsyncOperation::CreateAndStart<ActivateContainerGroupRootAsyncOperation>(
            *this,
            request,
#if defined(PLATFORM_UNIX)
             request.PodDescription,
#endif
            timeout,
            move(context),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnContainerGroupSetupCompleted(operation);
        },
            this->Root.CreateAsyncOperationRoot());
    }
    else
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Dropping SetupContainerGroup request for AppServiceId {0} for Application {1} from Node {2} since requestor registration check failed",
            request.ServicePackageId,
            request.AppId,
            request.NodeId);
    }
}

void ProcessActivationManager::ProcessCreateSymbolicLinkRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    CreateSymbolicLinkRequest request;
    if (!message.GetBody<CreateSymbolicLinkRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    ErrorCode error(ErrorCodeValue::Success);
    vector<wstring> linksCreated;
    for (auto iter = request.SymbolicLinks.begin(); iter != request.SymbolicLinks.end(); iter++)
    {
        if (Directory::IsSymbolicLink(iter->key))
        {
            WriteInfo(
                TraceType_ActivationManager,
                Root.TraceId,
                "Directory {0} is already a symbolic link, skip setting symbolic link to location {1}",
                iter->key,
                iter->value);
        }
        else
        {
            if (!::CreateSymbolicLink(iter->key.c_str(), iter->value.c_str(), request.Flags))
            {
                error = ErrorCode::FromWin32Error();
                break;
            }
            linksCreated.push_back(iter->key);
        }
    }
    if (!error.IsSuccess())
    {
        for (auto iter = linksCreated.begin(); iter != linksCreated.end(); ++iter)
        {
            auto err = Directory::Delete(*iter, true);
            WriteTrace(err.ToLogLevel(),
                TraceType_ActivationManager,
                Root.TraceId,
                "Deleting symbolic link {0} returned error {1}",
                *iter,
                err);
        }
    }
    this->SendFabricActivatorOperationReply(error, context);
}

void ProcessActivationManager::ProcessDownloadContainerImagesMessage(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    DownloadContainerImagesRequest request;
    if (!message.GetBody<DownloadContainerImagesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    auto operation = AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
        request.Images,
        *this,
        TimeSpan::FromTicks(request.TimeoutInTicks),
        move(context),
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnContainerImagesOperationCompleted(operation, false);
        },
        this->Root.CreateAsyncOperationRoot());
}

void ProcessActivationManager::ProcessDeleteContainerImagesMessage(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    DeleteContainerImagesRequest request;
    if (!message.GetBody<DeleteContainerImagesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    auto operation = AsyncOperation::CreateAndStart<DeleteContainerImagesAsyncOperation>(
        request.Images,
        *this,
        TimeSpan::FromTicks(request.TimeoutInTicks),
        move(context),
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnContainerImagesOperationCompleted(operation, true);
    },
        this->Root.CreateAsyncOperationRoot());
}

void ProcessActivationManager::OnContainerImagesOperationCompleted(AsyncOperationSPtr const & operation, bool isDelete)
{
    ErrorCode error;
    if (isDelete)
    {
        error = DeleteContainerImagesAsyncOperation::End(operation);
    }
    else
    {
        error = DownloadContainerImagesAsyncOperation::End(operation);
    }
    WriteTrace(error.ToLogLevel(),
        TraceType_ActivationManager,
        "ContainerImagesOperation returned {0}, isDelete",
        error,
        isDelete);
}

void ProcessActivationManager::ProcessGetContainerInfoMessage(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    GetContainerInfoRequest request;
    if (!message.GetBody<GetContainerInfoRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    auto operation = AsyncOperation::CreateAndStart<GetContainerInfoAsyncOperation>(
        request.NodeId,
        request.ApplicationServiceId,
        request.IsServicePackageActivationModeExclusive,
        request.ContainerInfoType,
        request.ContainerInfoArgs,
        *this,
        HostingConfig::GetConfig().ActivationTimeout,
        move(context),
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnGetContainerInfoOperationCompleted(operation);
    },
        this->Root.CreateAsyncOperationRoot());
}

void ProcessActivationManager::OnGetContainerInfoOperationCompleted(AsyncOperationSPtr const & operation)
{
    ErrorCode error = GetContainerInfoAsyncOperation::End(operation);
    WriteTrace(error.ToLogLevel(),
        TraceType_ActivationManager,
        "GetContainerInfo Operation returned {0}",
        error);
}

void ProcessActivationManager::ProcessGetResourceUsage(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ResourceMeasurementRequest request;
    if (!message.GetBody<ResourceMeasurementRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    auto operation = AsyncOperation::CreateAndStart<GetResourceUsageAsyncOperation>(
        request.NodeId,
        request.Hosts,
        *this,
        move(context),
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnGetResourceUsageComplete(operation, false);
    },
        this->Root.CreateAsyncOperationRoot());

    this->OnGetResourceUsageComplete(operation, true);
}

void ProcessActivationManager::OnGetResourceUsageComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    ErrorCode error = GetResourceUsageAsyncOperation::End(operation);
    WriteTrace(error.ToLogLevel(),
        TraceType_ActivationManager,
        "GetResourceUsageAsyncOperation Operation returned {0}",
        error);
}

void ProcessActivationManager::ProcessGetImagesMessage(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    // no need to read message as body is empty
    UNREFERENCED_PARAMETER(message);

    auto operation = AsyncOperation::CreateAndStart<GetImagesAsyncOperation>(
        *this,
        move(context),
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnGetImagesCompleted(operation, false);
    },
        this->Root.CreateAsyncOperationRoot());
    this->OnGetImagesCompleted(operation, true);
}

void ProcessActivationManager::OnGetImagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    auto error = GetImagesAsyncOperation::End(operation);
}

void ProcessActivationManager::ProcessConfigureSharedFolderRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ConfigreSharedFolderACLRequest request;
    if (!message.GetBody<ConfigreSharedFolderACLRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    ErrorCode error(ErrorCodeValue::Success);

    vector<pair<SidSPtr, DWORD>> principalPermissions;

    SidSPtr winfabAdmin;
    error = BufferedSid::CreateSPtr(*FabricConstants::WindowsFabricAdministratorsGroupName, winfabAdmin);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed to get sid for winfabadmin, error {0}",
            error);
        SendFabricActivatorOperationReply(error, context);
        return;
    }
    principalPermissions.push_back(make_pair(winfabAdmin, GENERIC_ALL));

    SidSPtr allowedUsr;
    error = BufferedSid::CreateSPtr(*FabricConstants::WindowsFabricAllowedUsersGroupName, allowedUsr);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed to get sid for winfaballowed user, error {0}",
            error);
        SendFabricActivatorOperationReply(error, context);
        return;
    }
    principalPermissions.push_back(make_pair(allowedUsr, (GENERIC_READ | GENERIC_EXECUTE)));

    SidSPtr system;
    error = BufferedSid::CreateSPtr(WinLocalSystemSid, system);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed to get sid for system user, error {0}",
            error);
        SendFabricActivatorOperationReply(error, context);
        return;
    }
    principalPermissions.push_back(make_pair(system, GENERIC_ALL));

    SidSPtr ba;
    error = BufferedSid::CreateSPtr(WinBuiltinAdministratorsSid, ba);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed to get sid for builtin admin user, error {0}",
            error);
        SendFabricActivatorOperationReply(error, context);
        return;
    }
    principalPermissions.push_back(make_pair(ba, GENERIC_ALL));

    TimeSpan timeout = TimeSpan::FromTicks(request.TimeoutInTicks);

    for (auto iter = request.SharedFolders.begin(); iter != request.SharedFolders.end(); iter++)
    {

        error = SecurityUtility::OverwriteFolderACL(
            *iter,
            principalPermissions,
            false, //dont disable inheritence on shared folders
            false, //IgnoreInheritence flag, if inheritence is already set dont apply permissions.
            timeout);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to set shared folder permissions on folder {0}, error {1}",
                *iter,
                error);
            SendFabricActivatorOperationReply(error, context);
            return;
        }
    }
    this->SendFabricActivatorOperationReply(error, context);
}

void ProcessActivationManager::ProcessConfigureSMBShareRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ConfigureSMBShareRequest request;
    if (!message.GetBody<ConfigureSMBShareRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    WriteInfo(
        TraceType_ActivationManager,
        Root.TraceId,
        "ProcessConfigureSMBShareRequest called with LocalFullPath={0}, ShareName={1}, AccessMask={2}",
        request.LocalFullPath,
        request.ShareName,
        request.AccessMask);

    ErrorCode error(ErrorCodeValue::Success);

#if !defined(PLATFORM_UNIX)

    vector<SidSPtr> allowedSids;
    SidSPtr anonymousSid;
    for (auto iter = request.Sids.begin(); iter != request.Sids.end(); ++iter)
    {
        SidSPtr sid;
        error = BufferedSid::CreateSPtrFromStringSid(*iter, sid);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to set create Sid object for {0}, error {1}",
                *iter,
                error);
            SendFabricActivatorOperationReply(error, context);
            return;
        }

        allowedSids.push_back(sid);

        if (sid->IsAnonymous()) { anonymousSid = sid; }
    }

    TimeSpan timeout = TimeSpan::FromTicks(request.TimeoutInTicks);
    TimeoutHelper timoutHelper(timeout);

    SecurityDescriptorSPtr securityDescriptor;
    error = BufferedSecurityDescriptor::CreateAllowed(
        allowedSids,
        request.AccessMask,
        securityDescriptor);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "SetupShare: Failed to create SecurityDescriptor object. Error:{0}", error);
        SendFabricActivatorOperationReply(error, context);
        return;
    }

    if (!Directory::Exists(request.LocalFullPath))
    {
        error = Directory::Create2(request.LocalFullPath);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "SetupShare: Failed to create Directory '{0}'. Error:{1}",
                request.LocalFullPath,
                error);
            SendFabricActivatorOperationReply(error, context);
            return;
        }
    }

    error = SMBShareUtility::EnsureServerServiceIsRunning();
    if (!error.IsSuccess())
    {
        // This is best effort. If the service is not running, CreateShare will fail.
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "SetupShare: EnsureServerServiceIsRunning failed with Error:{0}",
            error);
    }

    error = SMBShareUtility::CreateShare(request.LocalFullPath, request.ShareName, securityDescriptor);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "SetupShare: Failed to create share. LocalFullPath:{0}, ShareName:{1}, AccessMask:{2}, Error:{3}",
            request.LocalFullPath,
            request.ShareName,
            request.AccessMask,
            error);
        SendFabricActivatorOperationReply(error, context);
        return;
    }

    SidSPtr fssGroupSid;
    error = BufferedSid::CreateSPtr(*Constants::FileStoreServiceUserGroup, fssGroupSid);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Unable to lookup for sid for {0}: Failed to acl share's local permission. LocalFullPath:{1}, ShareName:{2}, AccessMask:{3}, Error:{4}",
            *Constants::FileStoreServiceUserGroup,
            request.LocalFullPath,
            request.ShareName,
            request.AccessMask,
            error);
    }
    else
    {
        vector<SidSPtr> sids;
        sids.push_back(fssGroupSid);
        error = SecurityUtility::UpdateFolderAcl(sids, request.LocalFullPath, request.AccessMask, timoutHelper.GetRemainingTime(), true);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType_ActivationManager,
                Root.TraceId,
                "UpdateFolderAcl failed for FSS group {0} : Error={1}",
                *Constants::FileStoreServiceUserGroup,
                error);
        }
    }

    // Trace out ACLs that are applied to the share.
    // LookupAccount is an expensive operation, so delete the code block below when tracing is not needed anymore
    std::wstring strAclSids;
    Common::StringWriter writer(strAclSids);
    if (error.IsSuccess())
    {
        for (auto const &sid : allowedSids)
        {
            wstring sidString, domain, accountName;
            auto innerError = sid->ToString(sidString);
            if (!innerError.IsSuccess())
            {
                WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Unable to convert sid to string Error={0}",
                    innerError);

                continue;
            }

            innerError = Sid::LookupAccount(sid->PSid, domain, accountName);
            if (innerError.IsSuccess())
            {
                writer.Write("{0}|{1}|{2} ", sidString, accountName, domain);
            }
            else
            {
                WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Unable to lookup sid to account name Sid={0} Error={1}",
                    sidString,
                    innerError);

                writer.Write("{0}|<>|<> ", sidString);
            }
        }
    }

    if (anonymousSid)
    {
        error = SMBShareUtility::EnableAnonymousAccess(request.LocalFullPath, request.ShareName, request.AccessMask, anonymousSid, timoutHelper.GetRemainingTime());
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivationManager,
            "EnableAnonymousAccess. LocalPath:{0}, ShareName:{1}, Error:{2}",
            request.LocalFullPath, request.ShareName, error);
    }
    else
    {
        error = SMBShareUtility::DisableAnonymousAccess(request.LocalFullPath, request.ShareName, timoutHelper.GetRemainingTime());
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                "DisableAnonymousAccess: Failed to disable anonymous access. LocalFullPath:{0}, ShareName:{1}, Error:{2}",
                request.LocalFullPath,
                request.ShareName,
                error);

            SendFabricActivatorOperationReply(error, context);
            return;
        }
    }

    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceType_ActivationManager,
        Root.TraceId,
        "ProcessConfigureSMBShareRequest completed with LocalFullPath={0}, ShareName={1}, AccessMask={2}, Error={3}, SidsAcled(Sid|Account|Domain)={4}",
        request.LocalFullPath,
        request.ShareName,
        request.AccessMask,
        error,
        strAclSids);

    this->SendFabricActivatorOperationReply(error, context);

#else

    if (!Directory::Exists(request.LocalFullPath))
    {
        error = Directory::Create2(request.LocalFullPath);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "SetupShare: Failed to create Directory '{0}'. Error:{1}",
                request.LocalFullPath,
                error);
            SendFabricActivatorOperationReply(error, context);
            return;
        }
    }
    string dir;
    StringUtility::Utf16ToUtf8(request.LocalFullPath, dir);

    wstring str;
    SecurityUserSPtr securityUser = make_shared<BuiltinServiceAccount>(
                                        str,
                                        str,
                                        str,
                                        str,
                                        false,
                                        false,
                                        SecurityPrincipalAccountType::NetworkService,
                                        std::vector<std::wstring>(),
                                        std::vector<std::wstring>());
    securityUser->ConfigureAccount();

    string fabricuname;
    StringUtility::Utf16ToUtf8(securityUser->AccountName, fabricuname);
    string cmd = string("chown ") + fabricuname + " " + dir + " > /dev/null 2>&1";
    system(cmd.c_str());

#if defined(LINUX_ACL_ENABLED)
    cmd = string("setfacl -R ");
    for (auto iter = request.Sids.begin(); iter != request.Sids.end(); ++iter)
    {
        string access;
        if (request.AccessMask & GENERIC_READ)
            access += "rx";
        if (request.AccessMask & GENERIC_WRITE)
            access += "w";
        if (request.AccessMask & GENERIC_ALL)
            access = "rwx";

        string modifier(" -m \"");
        SidSPtr sid;
        error = BufferedSid::CreateSPtrFromStringSid(*iter, sid);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to set create Sid object for {0}, error {1}",
                *iter,
                error);
            SendFabricActivatorOperationReply(error, context);
            return;
        }

        if (sid->IsAnonymous())
        {
            modifier += "o:" + access;
        }
        else
        {
            PLSID psid = (PLSID)sid->PSid;
            if (psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_RID)
            {
                ULONG id = psid->SubAuthority[1];
                modifier += "u:" + std::to_string(id) + ":" + access;
                modifier += ",d:u:" + std::to_string(id) + ":" + access;
            }
            else if (psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_GROUP_RID)
            {
                ULONG id = psid->SubAuthority[1];
                modifier += "g:" + std::to_string(id) + ":" + access;
                modifier += ",d:g:" + std::to_string(id) + ":" + access;
            }
        }
        modifier += "\"";
        cmd += modifier + " ";
    }
    cmd += " " + dir + " > /dev/null 2>&1";
    system(cmd.c_str());
#else
    for (auto iter = request.Sids.begin(); iter != request.Sids.end(); ++iter)
    {
        SidSPtr sid;
        error = BufferedSid::CreateSPtrFromStringSid(*iter, sid);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to set create Sid object for {0}, error {1}",
                *iter,
                error);
            SendFabricActivatorOperationReply(error, context);
            return;
        }
        else if (!sid->IsAnonymous())
        {
            PLSID psid = (PLSID)sid->PSid;
            if (psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_GROUP_RID)
            {
                ULONG id = psid->SubAuthority[1];
                cmd = string("chown -R :") + std::to_string(id) + " " + dir + " > /dev/null 2>&1";
                system(cmd.c_str());
                if (request.AccessMask & GENERIC_ALL) {
                    cmd = string("chmod g+rwx ") + dir + " > /dev/null 2>&1";
                }
                else {
                    cmd = string("chmod g+rx ") + dir + " > /dev/null 2>&1";
                }
                system(cmd.c_str());
            }
        }
    }
#endif

    this->SendFabricActivatorOperationReply(error, context);
#endif
}

void ProcessActivationManager::ProcessConfigureNodeForDnsServiceMessage(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    ConfigureNodeForDnsServiceRequest request;
    if (!message.GetBody<ConfigureNodeForDnsServiceRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(TraceType_ActivationManager, Root.TraceId, "ProcessConfigureNodeForDnsServiceMessage Invalid message received: {0}, dropping", message);
        return;
    }

    WriteInfo(TraceType_ActivationManager, Root.TraceId,
        "ProcessConfigureNodeForDnsServiceMessage request.IsDnsServiceEnabled {0}.", request.IsDnsServiceEnabled);

    ConfigureNodeForDnsService dns(this->Root);
    ErrorCode error = dns.Configure(request.IsDnsServiceEnabled, request.SetAsPreferredDns);

    SendFabricActivatorOperationReply(error, context);
}

ErrorCode ProcessActivationManager::TerminateAndCleanup(
    wstring const & nodeId,
    DWORD processId,
    uint64 nodeInstanceId)
{
    ActivatorRequestorSPtr activatorRequestor;
    auto err = requestorMap_.Get(nodeId, activatorRequestor);
    WriteTrace(err.ToLogLevel(),
        TraceType_ActivationManager,
        "Get node {0} from requestor map. Error {1}",
        nodeId,
        err);
    if (err.IsSuccess())
    {
        if (activatorRequestor->ProcessId == processId)
        {
            activatorRequestor->Abort();

            vector<ApplicationServiceSPtr> appServices = this->applicationServiceMap_->RemoveAllByParentId(nodeId);
            vector<ApplicationServiceSPtr> containers;

            vector<wstring> cgroupsToCleanup;
            for (auto iter = appServices.begin(); iter != appServices.end(); ++iter)
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Application host {0} is not terminated. Aborting now",
                    (*iter)->ApplicationServiceId);
                if((*iter)->IsContainerHost)
                {
#if defined(PLATFORM_UNIX)
                    //only for container roots do a cleanup of cgroups
                    if ((*iter)->ContainerDescriptionObj.IsContainerRoot && !(*iter)->ProcessDescriptionObj.CgroupName.empty())
                    {
                        cgroupsToCleanup.push_back((*iter)->ProcessDescriptionObj.CgroupName);
                    }
#endif
                    containers.push_back(*iter);
                }
                else
                {
                    (*iter)->AbortAndWaitForTermination();
                }
            }
            if(!containers.empty())
            {
                auto operation = AsyncOperation::CreateAndStart<TerminateServicesAsyncOperation>(
                    *this,
                    nodeId,
                    containers,
                    cgroupsToCleanup,
                    [this](AsyncOperationSPtr const & operation)
                {
                    this->OnTerminateServicesCompleted(operation);
                },
                this->Root.CreateAsyncOperationRoot());
            }
            this->principalsProvider_->Cleanup(nodeId).ReadValue();

#if !defined(PLATFORM_UNIX)
            if (HostingConfig::GetConfig().EnableFirewallSecurityCleanup && HostingConfig::GetConfig().FirewallPolicyEnabled)
            {
                Threadpool::Post([nodeInstanceId]() {
                    vector<BSTR> rules;
                    auto error = FirewallSecurityProviderHelper::GetOrRemoveFirewallRules(
                        rules,
                        L"", /* groupname is empty. Donot remove all rules matching group name.*/
                        nodeInstanceId,
                        true);

                    if (!error.IsSuccess())
                    {
                        WriteInfo(TraceType_ActivationManager, "Failed to remove firewall policies for nodeInstance {0} error:{1}", nodeInstanceId, error);
                    }
                });
            }
#endif
            return requestorMap_.Remove(nodeId, activatorRequestor);
        }
        else
        {
            WriteInfo(TraceType_ActivationManager,
                "Requestor removal ignored for nodeid {0} since it is associated with process Id {1} while notification is for {2}.",
                nodeId,
                activatorRequestor->ProcessId,
                processId);
        }
    }
    WriteNoise(TraceType_ActivationManager,
        Root.TraceId,
        "Terminate and cleanup completed for process with Id {0} and node Id {1}.",
        processId,
        nodeId);
    return ErrorCode(ErrorCodeValue::Success);
}

void ProcessActivationManager::OnTerminateServicesCompleted(AsyncOperationSPtr const & operation)
{
    auto error = TerminateServicesAsyncOperation::End(operation);
    WriteTrace(
        error.ToLogLevel(),
        TraceType_ActivationManager,
        "TerminateServices error  {0}.",
        error);
}

void ProcessActivationManager::CleanupPortAclsForNode(wstring const & nodeId)
{
    this->endpointProvider_->CleanupAllForNode(nodeId);
}

void ProcessActivationManager::OnContainerTerminated(
    wstring const & appServiceId,
    std::wstring const & nodeId,
    DWORD exitCode)
{
    ApplicationServiceSPtr appService;
    auto err = applicationServiceMap_->Remove(nodeId, appServiceId, appService);
    WriteTrace(err.ToLogLevel(),
        TraceType_ActivationManager,
        this->Root.TraceId,
        "Removing service with Id {0} from servicemap returned error {1}. The service is ContainerHost.",
        appServiceId,
        err);

    if (err.IsSuccess())
    {
        bool isRootContainer = false;
        if (appService != nullptr)
        {
            hostingTrace.ContainerTerminated(
                this->Root.TraceId,
                appService->ContainerDescriptionObj.ContainerName,
                appService->ContainerDescriptionObj.ApplicationName,
                appService->ContainerDescriptionObj.ServiceName,
                appService->ContainerDescriptionObj.CodePackageName);

            isRootContainer = appService->IsContainerRoot;

#if defined(PLATFORM_UNIX)
            this->UnmapClearContainerVolumes(appService);
#endif
            appService->SetTerminatedExternally(true);
            appService->AbortAndWaitForTermination();
        }
    }

    this->SendApplicationServiceTerminationNotification(
        nodeId,
        appServiceId,
        exitCode,
        ActivityDescription(ActivityId(), ActivityType::Enum::ServicePackageEvent),
        false);

    // Create marker files for the container log directory with appHostId so that FabricDCA can delete them.
    ContainerHelper::GetContainerHelper().MarkContainerLogFolder(appServiceId);
}

#if defined(PLATFORM_UNIX)
void ProcessActivationManager::UnmapClearContainerVolumes(ApplicationServiceSPtr const & appService)
{
    if (!(appService->IsContainerHost) ||
        (appService->ContainerDescriptionObj.PodDescription.IsolationMode != ServiceModel::ContainerIsolationMode::hyperv))
    {
        return;
    }

    vector<ContainerVolumeDescription> pluginProvidedVolumes;
    for (auto const & volume : appService->ContainerDescriptionObj.ContainerVolumes)
    {
        if (!volume.Driver.empty())
        {
            pluginProvidedVolumes.push_back(volume);
        }
    }
    WriteTrace(
        LogLevel::Info,
        TraceType_ActivationManager,
        Root.TraceId,
        "Application service {0} for parent process {1} has {2} volumes that need to be unmapped.",
        appService->ApplicationServiceId,
        appService->ParentId,
        pluginProvidedVolumes.size());

    auto operation = VolumeMapper::BeginUnmapAllVolumes(
        appService->ContainerDescriptionObj.ContainerName,
        move(pluginProvidedVolumes),
        TimeSpan::MaxValue,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnUnmapAllVolumesCompleted(operation, false);
        },
        this->Root.CreateAsyncOperationRoot());
    this->OnUnmapAllVolumesCompleted(operation, true);
}

void ProcessActivationManager::OnUnmapAllVolumesCompleted(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    // TODO: Should add some retry logic on unmap failure
    VolumeMapper::EndUnmapAllVolumes(operation);
}
#endif

void ProcessActivationManager::OnContainerEngineTerminated(
    DWORD exitCode,
    ULONG continuousFailureCount,
    TimeSpan const& nextStartTime)
{
    //Step1: Send a notification for AppService termination to Fabric.exe
    auto appServices = this->applicationServiceMap_->RemoveAllContainerServices();

    for (auto i = appServices.begin(); i != appServices.end(); i++)
    {
        vector<wstring> appServiceIds;
        vector<wstring> rootContainers;
        for (auto it = i->second.begin(); it != i->second.end(); ++it)
        {
            if (it->second->IsContainerRoot)
            {
                rootContainers.push_back(it->second->ApplicationServiceId);
            }
            else
            {
                appServiceIds.push_back(it->second->ApplicationServiceId);
            }
        }

        this->SendApplicationServiceTerminationNotification(
            ActivityDescription(ActivityId(),
            ActivityType::Enum::ServicePackageEvent), 
            i->first, 
            appServiceIds,
            ProcessActivator::ProcessAbortExitCode,
            false);

        if (!rootContainers.empty())
        {
            this->SendApplicationServiceTerminationNotification(
                ActivityDescription(ActivityId(),
                ActivityType::Enum::ServicePackageEvent),
                i->first,
                rootContainers,
                ProcessActivator::ProcessAbortExitCode,
                true);
        }
    }
    appServices.clear();

    // Step2: Post a notification for DockerProcess termination to Fabric.exe
    // Sending a notification to Fabric for Dockerd is best effort.
    if (HostingConfig::GetConfig().ReportDockerHealth)
    {
        auto componentRoot = this->Root.CreateComponentRoot();

        Threadpool::Post([this, exitCode, continuousFailureCount, nextStartTime, componentRoot]() {
            this->SendDockerProcessTerminationNotification(exitCode, continuousFailureCount, nextStartTime);
        });
    }
}

void ProcessActivationManager::SendDockerProcessTerminationNotification(DWORD exitCode, ULONG continuousFailureCount, TimeSpan const& nextStartTime)
{
    auto operation = AsyncOperation::CreateAndStart<NotifyDockerProcessTerminationAsyncOperation>(
        *this,
        exitCode,
        continuousFailureCount,
        nextStartTime,
        [this](AsyncOperationSPtr const & operation) { this->FinishSendDockerProcessTerminationNotification(operation, false); },
        this->Root.CreateAsyncOperationRoot());
    this->FinishSendDockerProcessTerminationNotification(operation, true);
}

void ProcessActivationManager::FinishSendDockerProcessTerminationNotification(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }
    MessageUPtr reply;
    auto error = NotifyDockerProcessTerminationAsyncOperation::End(operation);
    error.ReadValue();
}

Common::ErrorCode ProcessActivationManager::CopyAndACLCertificateFromDataPackage(
    wstring const & dataPackageName,
    wstring const & dataPackageVersion,
    wstring const & relativePath,
    wstring const & destinationPath,
    vector<wstring> const & machineAccountNamesForACL)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto certificateFolderPath = Path::GetDirectoryName(destinationPath);
    auto certificateFolderName = Path::GetFileName(certificateFolderPath);
    auto serviceManifestName = certificateFolderName.substr(certificateFolderName.find(L"_") + 1);
    auto dataPackageFolderName = serviceManifestName + L"." + dataPackageName + L"." + dataPackageVersion;
    auto certificateSourcePath = Path::Combine(Path::GetDirectoryName(Path::GetDirectoryName(certificateFolderPath)),
                                               Path::Combine(dataPackageFolderName, relativePath));
    if (!File::Exists(certificateSourcePath))
    {
        WriteError(
            TraceType_ActivationManager,
            "File {0} not found",
            certificateSourcePath);
        return ErrorCodeValue::FileNotFound;
    }

    error = File::Copy(certificateSourcePath, destinationPath, true);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_ActivationManager,
            "Failed to copy {0} to {1}. Error {2}",
            certificateSourcePath,
            destinationPath,
            error);
        return error;
    }

    vector<SidSPtr> sidVector;
    for (auto machineAccountNameForACL : machineAccountNamesForACL)
    {
        Common::SidSPtr sid;
        error = BufferedSid::CreateSPtr(machineAccountNameForACL, sid);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_ActivationManager,
                "Failed to convert {0} to SID: error={1}",
                machineAccountNameForACL,
                error);
            return error;
        }

        sidVector.push_back(sid);
    }

    error = SecurityUtility::OverwriteFileAcl(
        sidVector,
        destinationPath,
        GENERIC_READ | GENERIC_WRITE,
        TimeSpan::MaxValue);

    if (!error.IsSuccess())
    {
       WriteError(
            TraceType_ActivationManager,
            "Failed to OverwriteFileAcl for {0} : {1}",
            destinationPath,
            error);
       return error;
    }

    return error;
}

#if defined(PLATFORM_UNIX)
Common::ErrorCode ProcessActivationManager::CopyAndACLPEMFromStore(
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    wstring const & destinationPath,
    vector<wstring> const & machineAccountNamesForACL,
    __out wstring & privKeyPath)
{
    ErrorCode error(ErrorCodeValue::Success);
    X509FindValue::SPtr findValue;
    error = X509FindValue::Create(X509FindType::FindByThumbprint, x509FindValue, findValue);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed X509FindValue::Create Error={0}",
            error);
            return error;
    }

    CertContextUPtr loadedCert;
    error = CryptoUtility::GetCertificate(
    X509Default::StoreLocation(),
    x509StoreName.empty() ? X509Default::StoreName() : x509StoreName,
    findValue,
    loadedCert);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed to GetCertificate {0}. Error={1}.",
            x509FindValue,
            error);
        return error;
    }

    wstring srcPemPath(loadedCert.FilePath().begin(), loadedCert.FilePath().end());
    error = Common::File::Copy(srcPemPath, destinationPath, true);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed to copy {0} to {1}. Error={2}.",
            loadedCert.FilePath(),
            destinationPath,
            error);
        return error;
    }

    vector<SidSPtr> sidVector;
    for (auto machineAccountNameForACL : machineAccountNamesForACL)
    {
        Common::SidSPtr sid;
        error = BufferedSid::CreateSPtr(machineAccountNameForACL, sid);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_ActivationManager,
                "Failed to convert {0} to SID: error={1}",
                machineAccountNameForACL,
                error);
            return error;
        }

        sidVector.push_back(sid);
    }

    error = SecurityUtility::OverwriteFileAcl(
        sidVector,
        destinationPath,
        GENERIC_READ | GENERIC_WRITE,
        TimeSpan::MaxValue);
    if (!error.IsSuccess())
    {
       WriteError(
            TraceType_ActivationManager,
            "Failed to OverwriteFileAcl for {0} : {1}",
            destinationPath,
            error);
       return error;
    }

    PrivKeyContext privKey;
    error = Common::LinuxCryptUtil().ReadPrivateKey(loadedCert, privKey);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Failed to ReadPrivateKey for {0}. Error={1}.",
            x509FindValue,
            error);
        return error;
    }

    wstring tempStr(privKey.FilePath().begin(), privKey.FilePath().end());
    privKeyPath = tempStr;
    return error;
}
#endif

void ProcessActivationManager::ScheduleTimerForStats(TimeSpan const& timeSpan)
{
    if (timeSpan != TimeSpan::MaxValue)
    {
        statisticsIntervalTimerSPtr_ = Timer::Create("Hosting.PostStatistics", [this](TimerSPtr const & timer) { this->PostStatistics(timer); }, false);
        statisticsIntervalTimerSPtr_->Change(timeSpan);
    }
}

void ProcessActivationManager::PostStatistics(TimerSPtr const & timer)
{
    if (timer) { timer->Cancel(); }

    if (this->State.Value != FabricComponentState::Opened)
    {
        return;
    }

    size_t containersCount = 0;
    size_t applicationServicesCount = 0;

    applicationServiceMap_->GetCount(containersCount, applicationServicesCount);

    hostingTrace.ActiveExecutablesAndContainersStats(containersCount, applicationServicesCount);

    this->ScheduleTimerForStats(HostingConfig::GetConfig().StatisticsInterval);
}

// Test Methods
bool ProcessActivationManager::Test_VerifyLimitsEnforced(std::vector<double> & cpuLimitCP, std::vector<uint64> & memoryLimitCP) const
{
    auto & serviceMap = const_cast<ApplicationServiceMap &>(*(applicationServiceMap_.get()));
    auto applicationProcessServices = serviceMap.GetAllServices(false);
    auto applicationContainerServices = serviceMap.GetAllServices(true);

    auto verifyFunc = [&](map<wstring, map<wstring, ApplicationServiceSPtr>> const & applicationServices)
    {
        for (auto itFirst = applicationServices.begin(); itFirst != applicationServices.end(); ++itFirst)
        {
            for (auto itSecond = itFirst->second.begin(); itSecond != itFirst->second.end(); ++itSecond)
            {
                auto & applicationService = itSecond->second;
                if (!applicationService->Test_VerifyLimitsEnforced(cpuLimitCP, memoryLimitCP))
                {
                    return false;
                }
            }
        }
        return true;
    };

    if (!verifyFunc(applicationProcessServices)) return false;
    if (!verifyFunc(applicationContainerServices)) return false;

    return true;
}

bool ProcessActivationManager::Test_VerifyPods(__inout std::vector<int> & containerGroupsExpected) const
{
    auto & serviceMap = const_cast<ApplicationServiceMap &>(*(applicationServiceMap_.get()));
    auto applicationContainerServices = serviceMap.GetAllServices(true);
    map<wstring, int> containerGroupsMapping;
    vector<int> containerGroupsActual;
    int containerRoots = 0;

    for (auto itFirst = applicationContainerServices.begin(); itFirst != applicationContainerServices.end(); ++itFirst)
    {
        for (auto itSecond = itFirst->second.begin(); itSecond != itFirst->second.end(); ++itSecond)
        {
            auto & applicationService = itSecond->second;
            auto & containerDescription = applicationService->ContainerDescriptionObj;
            if (containerDescription.IsContainerRoot)
            {
                ++containerRoots;
            }
            if (!containerDescription.GroupContainerName.empty())
            {
                containerGroupsMapping[containerDescription.GroupContainerName]++;
                if (!applicationService->Test_VerifyContainerGroupSettings())
                {
                    return false;
                }
            }
        }
    }

    for (auto containerGroup : containerGroupsMapping)
    {
        containerGroupsActual.push_back(containerGroup.second);
    }

    std::sort(containerGroupsExpected.begin(), containerGroupsExpected.end());
    std::sort(containerGroupsActual.begin(), containerGroupsActual.end());

    //verify that the number of container group roots is the same as expected
    if (containerGroupsExpected.size() != containerRoots)
    {
        WriteError(
            TraceType_ActivationManager,
            Root.TraceId,
            "Number of container roots does not match. Expected count {0}, found {1}",
            containerGroupsExpected.size(),
            containerRoots);
        return false;
    }
    //verify that the distribution of containers is the same as expected
    if (containerGroupsActual != containerGroupsExpected)
    {
        WriteError(
            TraceType_ActivationManager,
            Root.TraceId,
            "Container groups do not match. Expected size {0}, found {1}",
            containerGroupsExpected.size(),
            containerGroupsActual.size());
        return false;
    }

    return true;
}
