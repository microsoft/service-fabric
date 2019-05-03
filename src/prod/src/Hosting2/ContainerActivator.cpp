// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define __null 0
#if defined(PLATFORM_UNIX)
#undef _WIN64
#undef WIN32_LEAN_AND_MEAN
#include <grpc++/grpc++.h>
#include "cri-api.grpc.pb.h"
#define _WIN64 1
#define WIN32_LEAN_AND_MEAN 1
#endif

#include "stdafx.h"

#if defined(PLATFORM_UNIX)
#include "ClearContainerHelper.h"
#include "ContainerRuntimeHandler.h"
#endif

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const TraceType("ContainerActivator");

class ContainerActivator::SendIpcMessageAsyncOperation : public AsyncOperation
{
    DENY_COPY(SendIpcMessageAsyncOperation)

public:
    SendIpcMessageAsyncOperation(
        ContainerActivator & owner,
        MessageUPtr && message,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , message_(move(message))
        , timeoutHelper_(timeout)
    {
    }

    virtual ~SendIpcMessageAsyncOperation()
    {
    }

    static ErrorCode SendIpcMessageAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out MessageUPtr & reply)
    {
        auto thisPtr = AsyncOperation::End<SendIpcMessageAsyncOperation>(operation);

        if (thisPtr->Error.IsSuccess())
        {
            reply = move(thisPtr->reply_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(
                ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }

        SendIpcMessage(thisSPtr);
    }

    void SendIpcMessage(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.TryGetIpcClient(ipcClient_))
        {
            auto operation = owner_.ipcClient_->BeginRequest(
                move(message_),
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
                {
                    FinishSendIpcMessage(operation, false);
                },
                thisSPtr);

            FinishSendIpcMessage(operation, true);

            return;
        }

        if (timeoutHelper_.IsExpired)
        {
            WriteError(
                TraceType,
                owner_.Root.TraceId,
                "SendIpcMessage failed. ContainerActivatorService is not resgistered.");

            ErrorCode detailError(
                ErrorCodeValue::Timeout,
                move(wstring(L"SendIpcMessage failed. ContainerActivatorService is not resgistered.")));

            TryComplete(thisSPtr, detailError);
            return;
        }

        WriteWarning(
            TraceType,
            owner_.Root.TraceId,
            "SendIpcMessage: ContainerActivatorService is not resgistered. Retrying...");

        Threadpool::Post(
            [this, thisSPtr]() { SendIpcMessage(thisSPtr); },
            TimeSpan::FromSeconds(HostingConfig::GetConfig().ContainerActivatorMessageRetryDelayInSec));
    }

    void FinishSendIpcMessage(
        AsyncOperationSPtr operation,
        bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }

        auto error = owner_.ipcClient_->EndRequest(operation, reply_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "FinishSendIpcMessage: ErrorCode={0}",
            error);

        TryComplete(operation->Parent, error);
    }

private:
    ContainerActivator & owner_;
    MessageUPtr message_;
    TimeoutHelper timeoutHelper_;
    IpcClientSPtr ipcClient_;
    MessageUPtr reply_;
};

class ContainerActivator::InvokeContainerApiOperation : public AsyncOperation
{
public:
    InvokeContainerApiOperation(
        ContainerActivator & owner,
        ContainerApiExecutionArgs && apiExecArgs,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , apiExecArgs_(move(apiExecArgs))
        , timeoutHelper_(timeout)
        , initialEnqueueSucceeded_(false)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out ContainerApiExecutionResponse & response)
    {
        auto thisPtr = AsyncOperation::End<InvokeContainerApiOperation>(operation);

        if (thisPtr->Error.IsSuccess())
        {
            response = move(thisPtr->response_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }

        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](IContainerActivator &)
            {
                this->initialEnqueueSucceeded_ = true;
                this->InvokeContainerApi(thisSPtr);
            },
            [thisSPtr, this](IContainerActivator &)
            {
                initialEnqueueSucceeded_ = true;

                WriteWarning(
                    TraceType,
                    "Operation timed out before InvokeContainerApiOperation job queue item was processed. {0}.",
                    apiExecArgs_);

                this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Timeout));
            });

        if (!this->owner_.JobQueueObject.Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType,
                "Could not enqueue InvokeContainerApiOperation to JobQueue. {0}.",
                apiExecArgs_);

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
        }
    }

    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.JobQueueObject.CompleteAsyncJob();
        }
    }

private:
    void InvokeContainerApi(AsyncOperationSPtr const & thisSPtr)
    {
        auto request = CreateInvokeContainerApiRequestMessage();

        auto operation = owner_.BeginSendMessage(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishInvokeContainerApi(operation, false);
            },
            thisSPtr);

        FinishInvokeContainerApi(operation, true);
    }

    void FinishInvokeContainerApi(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.EndSendMessage(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishInvokeContainerApi: owner_.EndSendMessage() : Error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        InvokeContainerApiReply replyBody;
        if (!reply->GetBody<InvokeContainerApiReply>(replyBody))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishInvokeContainerApi: GetBody<InvokeContainerApiReply> : Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;
        if (error.IsSuccess())
        {
            response_ = replyBody.TakeApiExecResponse();
        }

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "FinishInvokeContainerApi - replyBody.Error={0}, replyBody.ErrorMessage={1}.",
            error,
            replyBody.ErrorMessage);

        TryComplete(
            operation->Parent,
            ErrorCode(error.ReadValue(), replyBody.TakeErrorMessage()));
    }

    MessageUPtr CreateInvokeContainerApiRequestMessage()
    {
        InvokeContainerApiRequest requestBody(
            move(apiExecArgs_),
            timeoutHelper_.GetRemainingTime().Ticks);

        auto request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ContainerActivatorService));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::InvokeContainerApiRequest));

        return move(request);
    }

private:
    ContainerActivator & owner_;
    ContainerApiExecutionArgs apiExecArgs_;
    TimeoutHelper timeoutHelper_;
    ContainerApiExecutionResponse response_;
    bool initialEnqueueSucceeded_;
};

class ContainerActivator::OpenAsyncOperation : public AsyncOperation
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        ContainerActivator & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~OpenAsyncOperation()
    {
    }

    static ErrorCode OpenAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        //Setup job queue for docker opertions.
        auto jobQueue = make_unique<DefaultTimedJobQueue<IContainerActivator>>(
            wformatString("{0}+{1}", TraceType, L"RequestQueue"),
            owner_,
            false /*forceEnqueue*/,
            HostingConfig::GetConfig().MaxContainerOperations);
        jobQueue->SetAsyncJobs(true);
        owner_.requestJobQueue_ = move(jobQueue);

        owner_.RegisterIpcRequestHandler();

        OpenIpAddressProvider(thisSPtr);
    }

private:
    void OpenIpAddressProvider(AsyncOperationSPtr const & thisSPtr)
    {
        auto ipoperation = owner_.ipAddressProvider_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & ipoperation)
            {
                this->OnIpAddressProviderOpened(ipoperation, false);
            },
            thisSPtr);

        OnIpAddressProviderOpened(ipoperation, true);
    }

    void OnIpAddressProviderOpened(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.ipAddressProvider_->EndOpen(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(IPProviderOpen): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        StartContainerService(operation->Parent);
    }

    void StartContainerService(AsyncOperationSPtr const & thisSPtr)
    {
        if (HostingConfig::GetConfig().DisableContainers)
        {
            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

        auto operation = owner_.dockerProcessManager_->BeginInitialize(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation)
            {
                this->OnContainerServiceStarted(operation, false);
            },
            thisSPtr);

        OnContainerServiceStarted(operation, true);
    }

    void OnContainerServiceStarted(AsyncOperationSPtr const & operation, bool expectedCompletedSyncronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSyncronously)
        {
            return;
        }

        auto error = owner_.dockerProcessManager_->EndInitialize(operation);

        if (error.IsError(ErrorCodeValue::ServiceNotFound))
        {
            // Don't fail open if docker service is not present.
            // Container activation will generate health report for it.
            TryComplete(operation->Parent, ErrorCodeValue::Success);
            return;
        }

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OnContainerServiceStarted: ErrorCode={0}",
            error);

        if (error.IsSuccess())
        {
            OpenOverlayNetworkManager(operation->Parent);
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }

    void OpenOverlayNetworkManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.overlayNetworkManager_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnOverlayNetworkManagerOpened(operation, false);
        },
            thisSPtr);

        OnOverlayNetworkManagerOpened(operation, true);
    }

    void OnOverlayNetworkManagerOpened(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.overlayNetworkManager_->EndOpen(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(OverlayNetworkManagerOpen): ErrorCode={0}",
            error);

        if (error.IsSuccess())
        {
            OpenNetworkPluginProcessManager(operation->Parent);
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }

    void OpenNetworkPluginProcessManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.networkPluginProcessManager_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnNetworkPluginProcessManagerOpened(operation, false);
        },
            thisSPtr);

        OnNetworkPluginProcessManagerOpened(operation, true);
    }

    void OnNetworkPluginProcessManagerOpened(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.networkPluginProcessManager_->EndOpen(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(NetworkPluginProcessManagerOpen): ErrorCode={0}",
            error);
        
        TryComplete(operation->Parent, error);
    }

private:
    ContainerActivator & owner_;
    TimeoutHelper timeoutHelper_;
};

class ContainerActivator::CloseAsyncOperation : public AsyncOperation
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        ContainerActivator & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        lastError_(ErrorCodeValue::Success)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static ErrorCode CloseAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            CloseIpAddressProvider(thisSPtr);
        }
        else
        {
            StopContainerService(thisSPtr);
        }
    }

private:
    void StopContainerService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.dockerProcessManager_->BeginDeactivate(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnContainerServiceStopped(operation, false);
            },
            thisSPtr);

        OnContainerServiceStopped(operation, true);
    }

    void OnContainerServiceStopped(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.dockerProcessManager_->EndDeactivate(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(DockerProcessManagerDeactivate): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        CloseIpAddressProvider(operation->Parent);
    }

    void CloseIpAddressProvider(AsyncOperationSPtr const & thisSPtr)
    {
        auto ipoperation = owner_.ipAddressProvider_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & ipoperation)
            {
                this->OnIpAddressProviderClosed(ipoperation, false);
            },
            thisSPtr);

        OnIpAddressProviderClosed(ipoperation, true);
    }

    void OnIpAddressProviderClosed(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.ipAddressProvider_->EndClose(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(IPProviderClose): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        CloseOverlayManager(operation->Parent);
    }

    void CloseOverlayManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto overlayOperation = owner_.overlayNetworkManager_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & overlayOperation)
        {
            this->OnOverlayManagerClosed(overlayOperation, false);
        },
            thisSPtr);

        OnOverlayManagerClosed(overlayOperation, true);
    }

    void OnOverlayManagerClosed(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.overlayNetworkManager_->EndClose(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(OverlayManagerClose): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        CloseNetworkPluginProcessManager(operation->Parent);
    }

    void CloseNetworkPluginProcessManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto pluginOperation = owner_.networkPluginProcessManager_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & pluginOperation)
        {
            this->OnNetworkPluginProcessManagerClosed(pluginOperation, false);
        },
            thisSPtr);

        OnNetworkPluginProcessManagerClosed(pluginOperation, true);
    }

    void OnNetworkPluginProcessManagerClosed(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.networkPluginProcessManager_->EndClose(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(NetworkPluginProcessManagerClose): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        if (HostingConfig::GetConfig().LocalNatIpProviderEnabled)
        {
            CloseNatIpAddressProvider(operation->Parent);
        }
        else
        {
            TryComplete(operation->Parent, lastError_);
        }
    }

    void CloseNatIpAddressProvider(AsyncOperationSPtr const & thisSPtr)
    {
        auto natIpoperation = owner_.natIpAddressProvider_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & natIpoperation)
        {
            this->OnNatIpAddressProviderClosed(natIpoperation, false);
        },
            thisSPtr);

        OnNatIpAddressProviderClosed(natIpoperation, true);
    }

    void OnNatIpAddressProviderClosed(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.natIpAddressProvider_->EndClose(operation);

        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "End(NatIPProviderClose): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        if (owner_.requestJobQueue_)
        {
            owner_.requestJobQueue_->Close();
        }

        owner_.UregisterIpcRequestHandler();

        TryComplete(operation->Parent, lastError_);
    }

private:
    ContainerActivator & owner_;
    ErrorCode lastError_;
    TimeoutHelper timeoutHelper_;
};

class ContainerActivator::ActivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        __in ContainerActivator & owner,
        bool isUserLocalSystem,
        wstring const & appHostId,
        wstring const & nodeId,
        ContainerDescription const & containerDescription,
        ProcessDescription const & processDescription,
        wstring const & fabricbinPath,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , appHostId_(appHostId)
        , nodeId_(nodeId)
        , containerDescription_(containerDescription)
        , timeoutHelper_(timeout)
        , processDescription_(processDescription)
        , fabricBinFolder_(fabricbinPath)
        , isUserLocalSystem_(isUserLocalSystem)
        , initialEnqueueSucceeded_(false)
#if defined(PLATFORM_UNIX)
        , isIsolated_(false)
#endif
        , containerId_()
    {
    }

    virtual ~ActivateAsyncOperation()
    {
    }

    static ErrorCode ActivateAsyncOperation::End(AsyncOperationSPtr const & operation, std::wstring & containerId)
    {
        auto thisPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);

#if defined(PLATFORM_UNIX)
        //do a cleanup for cgroups
        if (!thisPtr->Error.IsSuccess())
        {
            if (thisPtr->containerDescription_.IsContainerRoot &&
                !thisPtr->processDescription_.CgroupName.empty())
            {
                string cgroupName;
                StringUtility::Utf16ToUtf8(thisPtr->processDescription_.CgroupName, cgroupName);
                auto error = ProcessActivator::RemoveCgroup(cgroupName, true);
                WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                    TraceType,
                    thisPtr->owner_.Root.TraceId,
                    "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                    thisPtr->processDescription_.CgroupName, error, cgroup_get_last_errno());
            }
        }
#endif

        if (thisPtr->Error.IsSuccess())
        {
            containerId = move(thisPtr->containerId_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(
                ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }

        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](IContainerActivator &)
            {
                this->initialEnqueueSucceeded_ = true;
                this->ActivateContainer(thisSPtr);
            },
            [thisSPtr, this](IContainerActivator &)
            {
                initialEnqueueSucceeded_ = true;

                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "Operation timed out before job queue item was processed.");

                this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
            });

        if (!this->owner_.requestJobQueue_->Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType,
                "Could not enqueue operation to JobQueue.");

            this->TryComplete(thisSPtr, ErrorCodeValue::FileStoreServiceNotReady);
        }
    }

    void ActivateContainer(AsyncOperationSPtr const & thisSPtr)
    {
#if defined(PLATFORM_UNIX)
        auto error = SetupCgroup();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
#endif

        owner_.eventTracker_->TrackContainer(
            containerDescription_.ContainerName,
            appHostId_,
            nodeId_,
            containerDescription_.HealthConfig.IncludeDockerHealthStatusInSystemHealthReport);

        auto request = CreateActivateContainerRequestMessage();

        auto operation = owner_.BeginSendMessage(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishActivateContainer(operation, false);
            },
            thisSPtr);

        FinishActivateContainer(operation, true);
    }

    void FinishActivateContainer(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.EndSendMessage(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishActivateContainer: owner_.EndSendMessage() : Error={0}.",
                error);

            activationError_.Overwrite(error);
            this->CleanupOnError(operation->Parent);
            return;
        }

        ActivateContainerReply replyBody;
        if (!reply->GetBody<ActivateContainerReply>(replyBody))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishActivateContainer: GetBody<ActivateContainerReply> : Message={0}, ErrorCode={1}",
                *reply,
                error);

            activationError_.Overwrite(error);
            this->CleanupOnError(operation->Parent);
            return;
        }

        error = replyBody.Error;
        if (!error.IsSuccess())
        {
            activationError_ = ErrorCode(replyBody.Error.ReadValue(), replyBody.TakeErrorMessage());

            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishActivateContainer - ActivateContainerReply(Error={0}, ErrorMessage={1}).",
                activationError_,
                activationError_.Message);

            this->CleanupOnError(operation->Parent);
            return;
        }
        else
        {
            this->containerId_ = replyBody.ContainerId;
        }

        TryComplete(operation->Parent, error);
    }

    void CleanupOnError(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.eventTracker_->UntrackContainer(containerDescription_.ContainerName);

        //
        // In cases of timeout, the container might have been successfully started
        // by FabricCAS. Try to terminate the container to ensure no container leak.
        //
        this->TerminateContainer(thisSPtr);
    }

    void TerminateContainer(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.BeginDeactivate(
            containerDescription_,
            containerDescription_.AutoRemove,
            containerDescription_.IsContainerRoot,
#if defined(PLATFORM_UNIX)
            false,
#endif
        processDescription_.CgroupName,
            false,
            HostingConfig::GetConfig().ContainerTerminationTimeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishTerminateContainer(operation, false);
            },
            thisSPtr);

        FinishTerminateContainer(operation, true);
    }

    void FinishTerminateContainer(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EndDeactivate(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "FinishTerminateContainer: owner_.EndDeactivate() : Error={0}.",
            error);

        TryComplete(operation->Parent, activationError_);
    }

    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.requestJobQueue_->CompleteAsyncJob();
        }
    }

    MessageUPtr CreateActivateContainerRequestMessage()
    {
#if !defined(PLATFORM_UNIX) && !defined(DotNetCoreClrIOT)
        if (IsWindowsServer() == false)
        {
            //
            // Windows 10 only supports hyperv mode.
            //
            containerDescription_.SetIsolationMode(ContainerIsolationMode::hyperv);
        }
#endif

        // Retrieve container gateway ip addresses
        std::vector<std::wstring> gatewayIpAddresses;
        owner_.RetrieveGatewayIpAddresses(containerDescription_, gatewayIpAddresses);

        ContainerActivationArgs activationArgs(
            isUserLocalSystem_,
            appHostId_,
            nodeId_,
            containerDescription_,
            processDescription_,
            fabricBinFolder_,
            gatewayIpAddresses);

        //
        // Keep the timeout for remote operation little smaller
        // so that it times out before local operation. This will
        // ensure no container leaks even in rare corner cases.
        //
        auto deltaTime = TimeSpan::FromSeconds(5);
        auto effectiveTime = timeoutHelper_.GetRemainingTime();
        if (effectiveTime > deltaTime)
        {
            effectiveTime = (effectiveTime - deltaTime);
        }

        ActivateContainerRequest requestBody(
            move(activationArgs),
            effectiveTime.Ticks);

        auto request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ContainerActivatorService));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ActivateContainerRequest));

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "CreateActivateContainerRequestMessage: Message={0}, Body={1}",
            *request,
            requestBody);

        return move(request);
    }

    ErrorCode SetupCgroup()
    {
#if defined(PLATFORM_UNIX)

        if (containerDescription_.IsContainerRoot &&
            !processDescription_.CgroupName.empty())
        {
            string cgroupName;
            StringUtility::Utf16ToUtf8(processDescription_.CgroupName, cgroupName);

            //specify that this is only for SP governance setup - cgroupName is for the SP
            auto error = ProcessActivator::CreateCgroup(
                cgroupName,
                &processDescription_.ResourceGovernancePolicy,
                &processDescription_.ServicePackageResourceGovernance,
                nullptr,
                true);
            if (error != 0)
            {
                WriteWarning(
                    TraceType,
                    "Cgroup setup for container root failed for {0} with error {1}, description of error if available {2}",
                    processDescription_.CgroupName, error, cgroup_get_last_errno());

                return ErrorCode(
                    ErrorCodeValue::InvalidOperation,
                    wformatString(
                        "Cgroup setup for container root failed for {0} with error {1}, description of error if available {2}",
                        processDescription_.CgroupName,
                        error,
                        cgroup_get_last_errno()));
            }
            else
            {
                WriteInfo(
                    TraceType,
                    "Cgroup setup for container root success for {0}",
                    processDescription_.CgroupName);
            }
        }
#endif
        return ErrorCode::Success();
    }

protected:
    ContainerActivator & owner_;
    ContainerDescription containerDescription_;
    ProcessDescription const processDescription_;
    wstring appHostId_;
    wstring nodeId_;
    wstring containerId_;
    TimeoutHelper timeoutHelper_;
    wstring fabricBinFolder_;
    bool isUserLocalSystem_;
    bool initialEnqueueSucceeded_;
    ErrorCode activationError_;
#if defined(PLATFORM_UNIX)
    bool isIsolated_;
#endif

#if defined(PLATFORM_UNIX)
public:
    __declspec(property(get=get_IsIsolated)) bool const & IsIsolated;
    bool const & get_IsIsolated() const
    {
        return isIsolated_;
    }
#endif
};

class ContainerActivator::DeactivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeactivateAsyncOperation)

public:
    DeactivateAsyncOperation(
        __in ContainerActivator & owner,
        wstring const & containerName,
        bool configuredForAutoremove,
        bool isContainerRoot,
        wstring cgroupName,
        bool isGracefulDeactivation,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , containerName_(containerName)
        , configuredforAutoRemove_(configuredForAutoremove)
        , isContainerRoot_(isContainerRoot)
        , cgroupName_(cgroupName)
        , timeoutHelper_(timeout)
        , isGracefulDeactivation_(isGracefulDeactivation)
        , initialEnqueueSucceeded_(false)
        , queueRetryCount_(0)
#if defined(PLATFORM_UNIX)
        , isIsolated_(false)
#endif
    {
    }

    virtual ~DeactivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }

        auto timeout = timeoutHelper_.GetRemainingTime();
        if (timeout.Ticks == 0)
        {
            timeout = HostingConfig::GetConfig().ContainerDeactivationQueueTimeount;
        }

        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeout,
            [thisSPtr, this](IContainerActivator &)
            {
                this->initialEnqueueSucceeded_ = true;
                this->DeactivateContainer(thisSPtr);
            },
            [thisSPtr, this](IContainerActivator &)
            {
                WriteWarning(
                    TraceType,
                    "Operation timed out before job queue item was processed. Current retry count {0}, container {1}",
                    this->queueRetryCount_,
                    this->containerName_);

                //forcefully kill container on timeout
                this->isGracefulDeactivation_ = false;
                this->timeoutHelper_.SetRemainingTime(HostingConfig().GetConfig().ContainerTerminationTimeout);
                this->initialEnqueueSucceeded_ = true;
                this->DeactivateContainer(thisSPtr);
            });

        if (!this->owner_.requestJobQueue_->Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType,
                "Could not enqueue operation to JobQueue.");

            this->TryComplete(thisSPtr, ErrorCodeValue::FileStoreServiceNotReady);
        }
    }

    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.requestJobQueue_->CompleteAsyncJob();
        }
    }

private:

    void DeactivateContainer(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.eventTracker_->UntrackContainer(containerName_);

        auto request = CreateDeactivateContainerRequestMessage();

        auto operation = owner_.BeginSendMessage(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDeactivateContainer(operation, false);
            },
            thisSPtr);

        FinishDeactivateContainer(operation, true);
    }

    void FinishDeactivateContainer(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.EndSendMessage(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishDeactivateContainer completed with error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        ContainerOperationReply replyBody;
        if (!reply->GetBody<ContainerOperationReply>(replyBody))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetBody<ContainerOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishDeactivateContainer - replyBody.Error={0}, replyBody.ErrorMessage={1}.",
                error,
                replyBody.ErrorMessage);
        }

#if defined(PLATFORM_UNIX)
        //if we are finishing container removal - remove the cgroup as well
        if (error.IsSuccess())
        {
            RemoveCgroup();
        }
#endif

        if (error.IsSuccess())
        {
            // Create marker files for the container log directory with containerName so that FabricDCA can delete them.
            // Perform this action only on success since if deactivation fails, OnAbort for ApplicationService will also consume this code.
            // If the OnAbort fails as well, then you leaked a container and hence you don't want to remove the container logs in that case.
            ContainerHelper::GetContainerHelper().MarkContainerLogFolder(containerName_);
        }

        TryComplete(
            operation->Parent,
            ErrorCode(error.ReadValue(), replyBody.TakeErrorMessage()));
    }

    MessageUPtr CreateDeactivateContainerRequestMessage()
    {
        ContainerDeactivationArgs deactivationArgs(
            containerName_,
            configuredforAutoRemove_,
            isContainerRoot_,
            cgroupName_,
            isGracefulDeactivation_);

        DeactivateContainerRequest requestBody(
            move(deactivationArgs),
            timeoutHelper_.GetRemainingTime().Ticks);

        auto request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ContainerActivatorService));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DeactivateContainerRequest));

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "CreateDeactivateContainerRequestMessage: Message={0}, Body={1}",
            *request,
            requestBody);

        return move(request);
    }

    void RemoveCgroup()
    {
#if defined(PLATFORM_UNIX)
        if (isContainerRoot_ && !cgroupName_.empty())
        {
            string cgroupName;
            StringUtility::Utf16ToUtf8(cgroupName_, cgroupName);
            auto error = ProcessActivator::RemoveCgroup(cgroupName, true);
            WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                TraceType,
                owner_.Root.TraceId,
                "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                cgroupName_, error, cgroup_get_last_errno());
        }
#endif
    }

protected:
    ContainerActivator & owner_;
    TimeoutHelper timeoutHelper_;
    uint64 queueRetryCount_;
    wstring containerName_;
    bool isGracefulDeactivation_;
    bool configuredforAutoRemove_;
    bool initialEnqueueSucceeded_;
    bool isContainerRoot_;
    wstring cgroupName_;
#if defined(PLATFORM_UNIX)
    bool isIsolated_;
#endif

#if defined(PLATFORM_UNIX)
public:
    __declspec(property(get=get_IsIsolated)) bool const & IsIsolated;
    bool const & get_IsIsolated() const
    {
        return isIsolated_;
    }
#endif
};

class ContainerActivator::DownloadContainerImagesAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadContainerImagesAsyncOperation)

public:
    DownloadContainerImagesAsyncOperation(
        __in ContainerActivator & owner,
        vector<ContainerImageDescription> const & images,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        images_(images),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~DownloadContainerImagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadContainerImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }

        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](IContainerActivator &)
            {
                this->initialEnqueueSucceeded_ = true;
                this->DownloadImages(thisSPtr);
            },
            [thisSPtr, this](IContainerActivator &)
            {
                initialEnqueueSucceeded_ = true;

                WriteWarning(
                    TraceType,
                    "Operation timed out before download container image queue item was processed.");

                ErrorCode error(
                    ErrorCodeValue::Timeout,
                    move(wstring(L"Operation timed out before download container image queue item was processed.")));

                this->TryComplete(thisSPtr, error);
            });

        if (!this->owner_.JobQueueObject.Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType,
                "Could not enqueue container image download operation to JobQueue.");

            ErrorCode error(
                ErrorCodeValue::InvalidOperation,
                move(wstring(L"Could not enqueue container image download operation to JobQueue.")));

            this->TryComplete(thisSPtr, error);
        }
    }

    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.JobQueueObject.CompleteAsyncJob();
        }
    }

private:
    void DownloadImages(AsyncOperationSPtr const & thisSPtr)
    {
        auto request = CreateDownloadContainerRequestMessage();

        auto operation = owner_.BeginSendMessage(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDownloadImages(operation, false);
            },
            thisSPtr);

        FinishDownloadImages(operation, true);
    }

    void FinishDownloadImages(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.EndSendMessage(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishDownloadImages: Error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        ContainerOperationReply replyBody;
        if (!reply->GetBody<ContainerOperationReply>(replyBody))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishDownloadImages: GetBody<ContainerOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishActivateContainer: replyBody.Error={0}, replyBody.ErrorMessage={1}.",
                error,
                replyBody.ErrorMessage);
        }

        TryComplete(
            operation->Parent,
            ErrorCode(error.ReadValue(), replyBody.TakeErrorMessage()));
    }

    MessageUPtr CreateDownloadContainerRequestMessage()
    {
        DownloadContainerImagesRequest requestBody(
            images_,
            timeoutHelper_.GetRemainingTime().Ticks);

        auto request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ContainerActivatorService));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DownloadContainerImages));

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "DownloadContainerImagesRequestMessage: Message={0}, Body={1}",
            *request,
            requestBody);

        return move(request);
    }

private:
    vector<ContainerImageDescription> images_;
    ContainerActivator & owner_;
    TimeoutHelper timeoutHelper_;
    bool initialEnqueueSucceeded_;
};

class ContainerActivator::DeleteContainerImagesAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeleteContainerImagesAsyncOperation)

public:
    DeleteContainerImagesAsyncOperation(
        __in ContainerActivator & owner,
        vector<wstring> const & images,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , images_(images)
        , owner_(owner)
        , timeoutHelper_(timeout)
        , initialEnqueueSucceeded_(false)
    {
    }

    virtual ~DeleteContainerImagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeleteContainerImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }

        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](IContainerActivator &)
            {
                this->initialEnqueueSucceeded_ = true;
                this->DeleteImages(thisSPtr);
            },
            [thisSPtr, this](IContainerActivator &)
            {
                initialEnqueueSucceeded_ = true;

                WriteWarning(
                    TraceType,
                    "Operation timed out before delete container image queue item was processed.");

                this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Timeout));
            });

        if (!this->owner_.JobQueueObject.Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType,
                "Could not enqueue container image download operation to JobQueue.");

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
        }
    }

    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.JobQueueObject.CompleteAsyncJob();
        }
    }

private:
    void DeleteImages(AsyncOperationSPtr const & thisSPtr)
    {
        auto request = CreateDeleteContainerRequestMessage();

        auto operation = owner_.BeginSendMessage(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDeleteImages(operation, false);
            },
            thisSPtr);

        FinishDeleteImages(operation, true);
    }

    void FinishDeleteImages(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.EndSendMessage(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishDeleteImages: Error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        ContainerOperationReply replyBody;
        if (!reply->GetBody<ContainerOperationReply>(replyBody))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishDeleteImages: GetBody<ContainerOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishDeleteImages: replyBody.Error={0}, replyBody.ErrorMessage={1}.",
                error,
                replyBody.ErrorMessage);
        }

        TryComplete(
            operation->Parent,
            ErrorCode(error.ReadValue(), replyBody.TakeErrorMessage()));
    }

    MessageUPtr CreateDeleteContainerRequestMessage()
    {
        DeleteContainerImagesRequest requestBody(
            images_,
            timeoutHelper_.GetRemainingTime().Ticks);

        auto request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ContainerActivatorService));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DeleteContainerImages));

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "DeleteContainerImagesRequestMessage: Message={0}, Body={1}",
            *request,
            requestBody);

        return move(request);
    }

private:
    vector<wstring> images_;
    ContainerActivator & owner_;
    TimeoutHelper timeoutHelper_;
    bool initialEnqueueSucceeded_;
};

class ContainerActivator::GetContainerLogsAsyncOperation : public AsyncOperation
{
    DENY_COPY(GetContainerLogsAsyncOperation)

public:
    GetContainerLogsAsyncOperation(
        __in ContainerActivator & owner,
        Hosting2::ContainerDescription const & containerDescription,
        bool isServicePackageActivationModeExclusive,
        wstring const & tailArg,
        int previous,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        containerDescription_(containerDescription),
        isServicePackageActivationModeExclusive_(isServicePackageActivationModeExclusive),
        containerName_(containerDescription.ContainerName),
        tailArg_(tailArg),
        previous_(previous),
        timeoutHelper_(timeout)
#if defined(PLATFORM_UNIX)
        , isIsolated_(false)
#endif
    {
        if (tailArg_.empty())
        {
            tailArg_ = Constants::DefaultContainerLogsTail;
        }
    }

    virtual ~GetContainerLogsAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & containerLogs)
    {
        auto thisPtr = AsyncOperation::End<GetContainerLogsAsyncOperation>(operation);
        containerLogs = thisPtr->containerLogs_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (previous_ > 0)
        {
            GetPreviousContainerLogs(thisSPtr);
            if(!thisSPtr->Error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "GetContainerLogsAsyncOperation::GetPreviousContainerName: Failed with error={0}.",
                    thisSPtr->Error);

                TryComplete(thisSPtr, thisSPtr->Error);
                return;
            }
        }
        else
        {
            uriPath_ = wformatString(Constants::ContainersLogsUriPath, containerName_, tailArg_);
            GetLogs(thisSPtr);
        }
    }

    void GetPreviousContainerLogs(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();

        auto statusArg = L"{\"status\": [\"exited\",\"dead\"],";
        auto beforeArg = wformatString("\"before\": [\"{0}\"],", containerName_);
        wstring labelArg;
        if (isServicePackageActivationModeExclusive_)
        {
            labelArg = wformatString("\"label\": [\"{0}={1}_{2}_{3}\"]}", Constants::ContainerLabels::SrppQueryFilterLabelKeyName, containerDescription_.ServiceName, containerDescription_.CodePackageName, containerDescription_.PartitionId);
        }
        else
        {
            labelArg = wformatString("\"label\": [\"{0}={1}_{2}_{3}\"]}", Constants::ContainerLabels::MrppQueryFilterLabelKeyName, containerDescription_.ServiceName, containerDescription_.CodePackageName, containerDescription_.ApplicationId);
        }
        auto filterArg = wformatString("{0}{1}{2}", statusArg, beforeArg, labelArg);
        auto uriPath = wformatString("containers/json?all=1&limit={0}&filters={1}", previous_, filterArg);

        ContainerApiExecutionArgs apiExecArgs(
            containerName_,
            *Constants::HttpGetVerb,
            uriPath,
            L"application/json",
            L"");

        auto operation = AsyncOperation::CreateAndStart<InvokeContainerApiOperation>(
            owner_,
            move(apiExecArgs),
            timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->FinishInvokeContainerApiForGetPreviousContainerLogs(operation, false);
        },
            thisSPtr);

        this->FinishInvokeContainerApiForGetPreviousContainerLogs(operation, true);
    }

    void FinishInvokeContainerApiForGetPreviousContainerLogs(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ContainerApiExecutionResponse apiExecResp;
        auto error = InvokeContainerApiOperation::End(operation, apiExecResp);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetContainerLogsAsyncOperation::FinishInvokeContainerApiForGetPreviousContainerLogs: InvokeContainerApiOperation failed with error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (apiExecResp.StatusCode != 200)
        {
            TryComplete(operation->Parent, ErrorCodeValue::InvalidOperation);
            return;
        }

        string response(apiExecResp.ResponseBody.begin(), apiExecResp.ResponseBody.end());

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Docker ps returned {0}",
            response);

        vector<GetContainerResponse> getContianerResponseList;
        auto byteBuffer = make_unique<ByteBuffer>(move(apiExecResp.ResponseBody));
        error = JsonHelper::Deserialize(getContianerResponseList, byteBuffer);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetContainerLogsAsyncOperation::FinishInvokeContainerApiForGetPreviousContainerLogs: Deserialize GetContainerResponse failed with error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (getContianerResponseList.size() == previous_)
        {
            containerName_ = *getContianerResponseList.at(previous_ - 1).Names.begin();
            StringUtility::TrimLeading<wstring>(containerName_, L"/");
            uriPath_ = wformatString(Constants::ContainersLogsUriPath, containerName_, tailArg_);
            GetLogs(operation->Parent);
        }
        else
        {
            wstring errMsg;
            if (isServicePackageActivationModeExclusive_)
            {
                errMsg = wformatString("GetContainerLogsAsyncOperation::FinishInvokeContainerApiForGetPreviousContainerLogs: Failed to find previous {0} container for {1}={2}_{3}_{4}.",
                    previous_, Constants::ContainerLabels::SrppQueryFilterLabelKeyName, containerDescription_.ServiceName, containerDescription_.CodePackageName, containerDescription_.PartitionId);
            }
            else
            {
                errMsg = wformatString("GetContainerLogsAsyncOperation::FinishInvokeContainerApiForGetPreviousContainerLogs: Failed to find previous {0} container for {1}={2}_{3}_{4}.",
                    previous_, Constants::ContainerLabels::MrppQueryFilterLabelKeyName, containerDescription_.ServiceName, containerDescription_.CodePackageName, containerDescription_.ApplicationId);
            }

            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "{0}",
                errMsg);

            error = ErrorCode(ErrorCodeValue::NotFound, move(errMsg));
            TryComplete(operation->Parent, error);
        }
    }

    void GetLogs(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();

        ContainerApiExecutionArgs apiExecArgs(
            containerName_,
            *Constants::HttpGetVerb,
            uriPath_,
            L"",
            L"");

        auto operation = AsyncOperation::CreateAndStart<InvokeContainerApiOperation>(
            owner_,
            move(apiExecArgs),
            timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishInvokeContainerApi(operation, false);
            },
            thisSPtr);

        this->FinishInvokeContainerApi(operation, true);
    }

    void FinishInvokeContainerApi(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ContainerApiExecutionResponse apiExecResp;
        auto error = InvokeContainerApiOperation::End(operation, apiExecResp);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetContainerLogsAsyncOperation::FinishInvokeContainerApi: InvokeContainerApiOperation failed with error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (apiExecResp.StatusCode == 200)
        {
            if (containerDescription_.RunInteractive)
            {
                string bodyAsString((char*)(apiExecResp.ResponseBody.data()), apiExecResp.ResponseBody.size());
                StringUtility::Utf8ToUtf16(bodyAsString, containerLogs_);
            }
            else
            {
                DockerUtilities::GetDecodedDockerStream(apiExecResp.ResponseBody, containerLogs_);
            }

            TryComplete(operation->Parent, ErrorCodeValue::Success);
            return;
        }
        else
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetContainerLogsAsyncOperation return StatusCode={0}.",
                apiExecResp.StatusCode);

            auto errMsg = wformatString("GetContainerLogsAsyncOperation return StatusCode={0}.", apiExecResp.StatusCode);
            error = ErrorCode(ErrorCodeValue::InvalidOperation, move(errMsg));
        }

        TryComplete(operation->Parent, error);
    }

protected:
    ContainerActivator & owner_;
    Hosting2::ContainerDescription containerDescription_;
    bool isServicePackageActivationModeExclusive_;
    wstring containerName_;
    wstring tailArg_;
    int previous_;
    wstring uriPath_;
    TimeoutHelper timeoutHelper_;
    wstring containerLogs_;
#if defined(PLATFORM_UNIX)
    bool isIsolated_;
#endif

#if defined(PLATFORM_UNIX)
public:
    __declspec(property(get=get_IsIsolated)) bool const & IsIsolated;
    bool const & get_IsIsolated() const
    {
        return isIsolated_;
    }
#endif
};


class ContainerActivator::GetContainerStatsAsyncOperation : public AsyncOperation
{
    DENY_COPY(GetContainerStatsAsyncOperation)

public:
    GetContainerStatsAsyncOperation(
        __in ContainerActivator & owner,
        Hosting2::ContainerDescription const & containerDescription,
        Hosting2::ProcessDescription const & processDescription,
        std::wstring const & parentId,
        std::wstring const & appServiceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , containerDescription_(containerDescription)
        , processDescription_(processDescription)
        , parentId_(parentId)
        , appServiceId_(appServiceId)
        , timeoutHelper_(timeout)
    {
    }

    virtual ~GetContainerStatsAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out uint64 & memoryUsage,
        __out uint64 & totalCpuTime,
        __out Common::DateTime & timeRead)
    {
        auto thisPtr = AsyncOperation::End<GetContainerStatsAsyncOperation>(operation);
        memoryUsage = thisPtr->memoryUsage_;
        totalCpuTime = thisPtr->totalCpuTime_;
        timeRead = thisPtr->timeRead_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
#if defined(PLATFORM_UNIX)
        // if container is Clear/Kata container
        if (ContainerRuntimeHandler::ContainerExists(containerDescription_.ContainerName))
        {
            // Read clear/kata container stats
            // Need to read cgroup as crio stats api doesn't work
            string containerGroupId;
            string containerGroupNameUtf8;
            StringUtility::Utf16ToUtf8(containerDescription_.GroupContainerName, containerGroupNameUtf8);

            // Get podID from container group name
            if (!ContainerRuntimeHandler::GetPodIdFromName(containerGroupNameUtf8, containerGroupId))
            {
                WriteWarning(
                    TraceType,
                    parentId_,
                    "Application Service with service Id {0} - Unable to get podID from container group name {1}",
                    appServiceId_,
                    containerDescription_.GroupContainerName);
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                return;
            }

            wstring containerCgroup = processDescription_.CgroupName.empty() ?
                Constants::CrioCgroupName + StringUtility::ToWString(containerGroupId) :
                processDescription_.CgroupName + Constants::CrioCgroupName + StringUtility::ToWString(containerGroupId);

            uint64 cpuUsage;
            uint64 memoryUsage;
            string cgroupNameUtf8;
            StringUtility::Utf16ToUtf8(containerCgroup, cgroupNameUtf8);
            auto error = ProcessActivator::GetCgroupUsage(cgroupNameUtf8, cpuUsage, memoryUsage);

            if (error != 0)
            {
                WriteError(
                    TraceType,
                    parentId_,
                    "Failed to get cgroup info for {0}, error {1}. CgroupName {2}",
                    appServiceId_,
                    error,
                    cgroupNameUtf8);

                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidOperation));
                return;
            }

            WriteInfo(
                TraceType,
                parentId_,
                "Container stats: TotalCpuTime {0} - memUsage {1}", cpuUsage, memoryUsage);

            memoryUsage_ = memoryUsage;
            totalCpuTime_ = cpuUsage;
            timeRead_ = Common::DateTime::Now();

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

#endif

        // Api for getting stats for docker containers
        auto timeout = timeoutHelper_.GetRemainingTime();
        auto operation = owner_.BeginInvokeContainerApi(
            containerDescription_,
            L"GET",
            L"/containers/{id}/stats?stream=false",
            L"application/json",
            L"",
            timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnContainerApiStatsCompleted(operation, false);
        },
            thisSPtr);
        this->OnContainerApiStatsCompleted(operation, true);
    }

    void OnContainerApiStatsCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        wstring result;
        auto error = owner_.EndInvokeContainerApi(operation, result);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        else
        {
            ContainerApiResponse containerApiResponse;
            error = JsonHelper::Deserialize(containerApiResponse, result);

            if (error.IsSuccess())
            {
                ContainerApiResult const & containerApiResult = containerApiResponse.Result();

                if (containerApiResult.Status() == 200)
                {
                    ContainerStatsResponse containerStatsResponse;
                    error = JsonHelper::Deserialize(containerStatsResponse, containerApiResult.Body());
                    if (error.IsSuccess())
                    {
                        memoryUsage_ = containerStatsResponse.MemoryStats_.MemoryUsage_;
                        totalCpuTime_ = containerStatsResponse.CpuStats_.CpuUsage_.TotalUsage_;
                        timeRead_ = containerStatsResponse.Read_;
                    }
                    else
                    {
                        WriteWarning(
                            TraceType,
                            parentId_,
                            "Application Service with service Id {0} ContainerStatsResponse error {1}",
                            appServiceId_,
                            error);
                    }
                }
                else
                {
                    WriteWarning(
                        TraceType,
                        parentId_,
                        "Application Service with service Id {0} ContainerApiResult status {1}",
                        appServiceId_,
                        containerApiResult.Status());
                    error = ErrorCode(ErrorCodeValue::InvalidOperation);
                }
            }
            else
            {
                WriteWarning(
                    TraceType,
                    parentId_,
                    "Application Service with service Id {0} ContainerApiResponse error {1}",
                    appServiceId_,
                    error);
            }

            TryComplete(operation->Parent, error);
            return;
        }
    }

protected:
    // in params
    ContainerActivator & owner_;
    Hosting2::ContainerDescription containerDescription_;
    Hosting2::ProcessDescription processDescription_;
    wstring containerName_;
    std::wstring const parentId_;
    std::wstring appServiceId_;
    TimeoutHelper timeoutHelper_;

    // out params
    uint64 memoryUsage_;
    uint64 totalCpuTime_;
    Common::DateTime timeRead_;

};

class ContainerActivator::GetContainerInfoAsyncOperation : public AsyncOperation
{
    DENY_COPY(GetContainerInfoAsyncOperation)

public:
    GetContainerInfoAsyncOperation(
        __in ContainerActivator & owner,
        wstring const & containerName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , containerName_(containerName)
        , timeoutHelper_(timeout)
        , containerInspect_()
    {
    }

    virtual ~GetContainerInfoAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out ContainerInspectResponse & containerInspect)
    {
        auto thisPtr = AsyncOperation::End<GetContainerInfoAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            containerInspect = move(thisPtr->containerInspect_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->QueryContainer(thisSPtr);
    }

    void QueryContainer(AsyncOperationSPtr const & thisSPtr)
    {
        auto uriPath = wformatString("containers/{0}/json", containerName_);

        auto timeout = timeoutHelper_.GetRemainingTime();

        ContainerApiExecutionArgs apiExecArgs(
            containerName_,
            *Constants::HttpGetVerb,
            uriPath,
            L"application/json",
            L"");

        auto operation = AsyncOperation::CreateAndStart<InvokeContainerApiOperation>(
            owner_,
            move(apiExecArgs),
            timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishInvokeContainerApi(operation, false);
            },
            thisSPtr);

        this->FinishInvokeContainerApi(operation, true);
    }

    void FinishInvokeContainerApi(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ContainerApiExecutionResponse apiExecResp;
        auto error = InvokeContainerApiOperation::End(operation, apiExecResp);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetContainerInfoAsyncOperation::FinishInvokeContainerApi: InvokeContainerApiOperation failed with error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (apiExecResp.StatusCode != 200)
        {
            TryComplete(operation->Parent, ErrorCodeValue::InvalidOperation);
            return;
        }

        string response(apiExecResp.ResponseBody.begin(), apiExecResp.ResponseBody.end());

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Docker inspect returned {0}",
            response);

        auto byteBuffer = make_unique<ByteBuffer>(move(apiExecResp.ResponseBody));
        error = JsonHelper::Deserialize(containerInspect_, byteBuffer);

        TryComplete(operation->Parent, error);
        return;
    }

private:
    ContainerActivator & owner_;
    wstring containerName_;
    ContainerInspectResponse containerInspect_;
    TimeoutHelper timeoutHelper_;
};

class ContainerActivator::ContainerApiOperation : public AsyncOperation
{
public:
    ContainerApiOperation(
        ContainerActivator & owner,
        ContainerDescription const & containerDescription,
        wstring const & httpVerb,
        wstring const & uriPath,
        wstring const & contentType,
        wstring const & requestBody,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , containerDescription_(containerDescription)
        , httpVerb_(httpVerb.empty() ? *Constants::HttpGetVerb : httpVerb)
        , uriPath_(uriPath)
        , requestContentType_(contentType.empty() ? *Constants::HttpContentTypeJson : contentType)
        , requestBody_(requestBody)
        , timeoutHelper_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & response)
    {
        auto thisPtr = AsyncOperation::End<ContainerApiOperation>(operation);
        response = thisPtr->response_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        StringUtility::Replace<wstring>(uriPath_, L"{id}", containerDescription_.ContainerName);

        auto timeout = timeoutHelper_.GetRemainingTime();

        ContainerApiExecutionArgs apiExecArgs(
            containerDescription_.ContainerName,
            httpVerb_,
            uriPath_,
            requestContentType_,
            requestBody_);

        auto operation = AsyncOperation::CreateAndStart<InvokeContainerApiOperation>(
            owner_,
            move(apiExecArgs),
            timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishInvokeContainerApi(operation, false);
            },
            thisSPtr);

        this->FinishInvokeContainerApi(operation, true);
    }

    void FinishInvokeContainerApi(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ContainerApiExecutionResponse apiExecResp;
        auto error = InvokeContainerApiOperation::End(operation, apiExecResp);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "ContainersApiOperation::FinishInvokeContainerApi: InvokeContainerApiOperation failed with error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        wstring responseBody;
        if (containerDescription_.RunInteractive || (apiExecResp.ContentType == *Constants::HttpContentTypeJson))
        {
            string bodyAsString((char*)(apiExecResp.ResponseBody.data()), apiExecResp.ResponseBody.size());
            StringUtility::Utf8ToUtf16(bodyAsString, responseBody);
        }
        else
        {
            //
            // TODO: The raw buffer may not always be multiplexed.
            // we should rather not interpret it and return to user the byte buffer itself.
            //
            DockerUtilities::GetDecodedDockerStream(apiExecResp.ResponseBody, responseBody);
        }

        ContainerApiResponse responseObj(
            apiExecResp.StatusCode,
            move(apiExecResp.ContentType),
            move(apiExecResp.ContentEncoding),
            move(responseBody));

        error = JsonHelper::Serialize(responseObj, response_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "ContainersApiOperation::OnGetHttpResponseCompleted: json serialization failed on response: {0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "ContainersApiOperation::OnGetHttpResponseCompleted: uriPath={0}, response={1}",
            uriPath_,
            response_);

        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }

private:
    ContainerActivator & owner_;
    ContainerDescription const containerDescription_;
    wstring const httpVerb_;
    wstring uriPath_;
    wstring const requestContentType_;
    wstring const requestBody_;
    TimeoutHelper timeoutHelper_;
    wstring response_;
};

#if defined(PLATFORM_UNIX)
// ContainerActivator::ActivateClearContainersAsyncOperation Implementation
class ContainerActivator::ActivateClearContainersAsyncOperation : public ActivateAsyncOperation
{
DENY_COPY(ActivateClearContainersAsyncOperation)

public:
    ActivateClearContainersAsyncOperation(
            __in ContainerActivator & owner,
            bool isSecUserLocalSystem,
            std::wstring const & appHostId,
            std::wstring const & nodeId,
            ContainerDescription const & containerDescription,
            ProcessDescription const & processDescription,
            wstring const & fabricbinPath,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : ActivateAsyncOperation(owner, isSecUserLocalSystem, appHostId, nodeId,
                                     containerDescription, processDescription, fabricbinPath,
                                     timeout, callback, parent)
            , config_()
            , localVolumes_()
            , pullRetries_(5)
    {
        isIsolated_ = true;
    }

    virtual ~ActivateClearContainersAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateClearContainersAsyncOperation>(operation);
        if (!thisPtr->Error.IsSuccess())
        {
            //do a cleanup for cgroups
            if (thisPtr->containerDescription_.IsContainerRoot && !thisPtr->processDescription_.CgroupName.empty())
            {
                string cgroupName;
                StringUtility::Utf16ToUtf8(thisPtr->processDescription_.CgroupName, cgroupName);
                auto error = ProcessActivator::RemoveCgroup(cgroupName, true);
                WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                           TraceType,
                           thisPtr->owner_.Root.TraceId,
                           "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                           thisPtr->processDescription_.CgroupName, error, cgroup_get_last_errno());
            }
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
                timeoutHelper_.GetRemainingTime(),
                [thisSPtr, this](IContainerActivator &)
                {
                    this->initialEnqueueSucceeded_ = true;
                    if (containerDescription_.IsContainerRoot)
                    {
                        SetupCgroup(thisSPtr);
                    }
                    else
                    {
                        MapVolumes(thisSPtr);
                    }
                },
                [thisSPtr, this](IContainerActivator &)
                {
                    initialEnqueueSucceeded_ = true;
                    WriteWarning(
                            TraceType,
                            "Operation timed out before job queue item was processed.");

                    this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
                });

        if (!this->owner_.JobQueueObject.Enqueue(move(jobItem)))
        {
            WriteWarning(
                    TraceType,
                    "Could not enqueue operation to JobQueue.");
            this->TryComplete(thisSPtr, ErrorCodeValue::FileStoreServiceNotReady);
        }
    }

    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.JobQueueObject.CompleteAsyncJob();
        }
    }

    void SetupCgroup(AsyncOperationSPtr const & thisSPtr)
    {
        if (!processDescription_.CgroupName.empty())
        {
            string cgroupName;
            StringUtility::Utf16ToUtf8(processDescription_.CgroupName, cgroupName);
            //specify that this is only for SP governance setup - cgroupName is for the SP
            auto error = ProcessActivator::CreateCgroup(cgroupName, &processDescription_.ResourceGovernancePolicy,
                                                        &processDescription_.ServicePackageResourceGovernance, nullptr, true);
            if (error != 0)
            {
                WriteWarning(
                        TraceType,
                        "Cgroup setup for container root failed for {0} with error {1}, description of error if available {2}",
                        processDescription_.CgroupName, error, cgroup_get_last_errno());
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                return;
            }
            else
            {
                WriteInfo(
                        TraceType,
                        "Cgroup setup for container root success for {0}",
                        processDescription_.CgroupName);
            }
        }
        RunPodSandbox(thisSPtr);
    }

    void RunPodSandbox(AsyncOperationSPtr const & thisSPtr)
    {
        // state of dns service
        bool isdnsservicenabled = DNS::DnsServiceConfig::GetConfig().IsEnabled;

        // dns search options
        std::wstring dnsSearchOptions = ContainerConfig::GetDnsSearchOptions(containerDescription_.ApplicationName);

        // port mapping, <hostip, protocol, container port, host port>
        std::vector<std::tuple<std::wstring,int,int,int>> portmappings;
        for (auto pm : containerDescription_.PodDescription.PortMappings)
        {
            portmappings.push_back(make_tuple(pm.HostIp, (int)pm.Proto, (int)pm.ContainerPort, (int)pm.HostPort));
        }

        std::wstring logDirectory = ClearContainerHelper::GetSandboxLogDirectory(containerDescription_);

        WriteInfo(TraceType, owner_.Root.TraceId,
                  "RunPodSandbox with ContainerPodDescription for AppHostId {0} NodeId {1}: {2}. LogDirectory set to {3}",
                  appHostId_, nodeId_, containerDescription_.PodDescription, logDirectory);

        auto operation = ContainerRuntimeHandler::BeginRunPodSandbox(
                appHostId_,
                nodeId_,
                containerDescription_.ContainerName,
                processDescription_.CgroupName,
                containerDescription_.PodDescription.HostName,
                logDirectory,
                containerDescription_.NetworkConfig.NetworkType,
                isdnsservicenabled,
                dnsSearchOptions,
                containerDescription_.DnsServers,
                portmappings,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnRunPodSandboxCompleted(operationSPtr, false);
                },
                thisSPtr);
        OnRunPodSandboxCompleted(operation, true);
    }

    void OnRunPodSandboxCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = ContainerRuntimeHandler::EndRunPodSandbox(operation);
        TryComplete(operation->Parent, error);
    }

    void CreateContainer(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType, owner_.Root.TraceId,
            "CreateContainer for AppHostId {0} NodeId {1}: containerName {2}",
            appHostId_,
            nodeId_,
            containerDescription_.ContainerName);

        vector<wstring> arguments;
        vector<pair<wstring, wstring>> envs;
        vector<pair<wstring, wstring>> labels;

        // Environment
        for (auto env : config_.Env)
        {
            auto pos = env.find_first_of('=');
            envs.push_back(make_pair(env.substr(0, pos), env.substr(pos + 1)));
        }

        // Mounts, <source, dest, isReadonly>
        vector<tuple<wstring, wstring, bool>> mounts;

        // Binds, including containerDescription_.ContainerVolumes
        for (wstring bind : config_.HostConfig.Binds)
        {
            vector<wstring> tokens;
            StringUtility::Split(bind, tokens, wstring(L":"));
            if ((tokens.size() != 2) && (tokens.size() != 3))
            {
                WriteWarning(TraceType, "Container Bind {0} isn't correctly formatted", bind);
                continue;
            }
            mounts.push_back(make_tuple(tokens[0], tokens[1], (tokens.size() > 2) && (tokens[2] == L"ro")));
        }

        // create container log path, crio does not create it for us.
        std::wstring sandboxLogDirectory = ClearContainerHelper::GetSandboxLogDirectory(containerDescription_);
        if (Directory::Exists(sandboxLogDirectory))
        {
            // The logDirectory of the sandbox should have been created by CRIO when the sandbox was brought up.
            // CRIO does not create the directories needed in the relative path for the container.
            std::wstring fullLogPath = Path::Combine(sandboxLogDirectory, config_.LogPath);
            WriteInfo(
                TraceType,
                "ContainerActivator::CreateContainer - Created full log path for container: {0}",
                fullLogPath
            );
            Directory::Create(Path::GetDirectoryName(fullLogPath));
        }
        else
        {
            WriteInfo(
                TraceType,
                "ContainerActivator::CreateContainer - Vm sandbox log directory {0} does not exist. The container creation will likely fail.",
                sandboxLogDirectory);
        }

        // labels
        for (auto it=config_.Labels.begin(); it!=config_.Labels.end(); ++it)
        {
            labels.push_back(pair<std::wstring, std::wstring>(it->first, it->second));
        }

        auto operation = ContainerRuntimeHandler::BeginCreateContainer(
                appHostId_,
                nodeId_,
                containerDescription_.GroupContainerName,
                containerDescription_.ContainerName,
                processDescription_.ExePath,
                config_.Cmd,
                arguments,
                envs,
                mounts,
                labels,
                L"" ,
                config_.LogPath,
                true,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnCreateContainerCompleted(operationSPtr, false);
                },
                thisSPtr);
        OnCreateContainerCompleted(operation, true);
    }

    void OnCreateContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = ContainerRuntimeHandler::EndCreateContainer(operation);
        if (error.IsSuccess())
        {
            this->StartContainer(operation->Parent, containerDescription_.ContainerName);
        }
        else if (error.ReadValue() == ErrorCodeValue::NotFound)
        {
            DownloadContainerImage(operation->Parent);
            return;
        }
        else
        {
            WriteInfo(
                TraceType, owner_.Root.TraceId,
                "Unsuccessful CreateContainer for AppHostId {0} NodeId {1}: containerName {2}. Error: {3}",
                appHostId_,
                nodeId_,
                containerDescription_.ContainerName,
                error);
            err_.Overwrite(error);
            this->UnmapVolumes(operation->Parent);
        }
    }

    void DownloadContainerImage(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
                TraceType, owner_.Root.TraceId,
                "PullImage for AppHostId {0} NodeId {1}: Image Name {2}",
                appHostId_,
                nodeId_,
                processDescription_.ExePath);

        auto repositoryCredentialsPassword = containerDescription_.RepositoryCredentials.Password;
        //If the RepositoryCredential is encrypted then decrypt the value before trying to pull the image
        if (StringUtility::AreEqualCaseInsensitive(containerDescription_.RepositoryCredentials.Type, Constants::Encrypted) || containerDescription_.RepositoryCredentials.IsPasswordEncrypted)
        {
            wstring value;
            auto error = ContainerConfigHelper::DecryptValue(containerDescription_.RepositoryCredentials.Password, value);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
            repositoryCredentialsPassword = value;
        }

        auto operation = ContainerRuntimeHandler::BeginPullImage(
                processDescription_.ExePath,
                containerDescription_.RepositoryCredentials.AccountName,
                repositoryCredentialsPassword,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnDownloadContainerImageCompleted(operationSPtr, false);
                },
                thisSPtr);
        OnDownloadContainerImageCompleted(operation, true);
    }

    void OnDownloadContainerImageCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = ContainerRuntimeHandler::EndPullImage(operation);
        if (error.IsSuccess())
        {
            this->CreateContainer(operation->Parent);
        }
        else if (error.ReadValue() == ErrorCodeValue::AlreadyExists && --pullRetries_)
        {
            this->DownloadContainerImage(operation->Parent);
        }
        else
        {
            WriteInfo(
                    TraceType, owner_.Root.TraceId,
                    "Unsuccessful PullImage for AppHostId {0} NodeId {1}: Image Name {2}. Error: {3}",
                    appHostId_,
                    nodeId_,
                    processDescription_.ExePath,
                    error);
            err_.Overwrite(error);
            this->UnmapVolumes(operation->Parent);
        }
    }

    void StartContainer(AsyncOperationSPtr const & thisSPtr, wstring containerName)
    {
        WriteInfo(
                TraceType, owner_.Root.TraceId,
                "StartContainer for AppHostId {0} NodeId {1}: containerName {2}",
                appHostId_,
                nodeId_,
                containerDescription_.ContainerName);

        auto operation = ContainerRuntimeHandler::BeginStartContainer(
                containerName,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnStartContainerCompleted(operationSPtr, false);
                },
                thisSPtr);
        OnStartContainerCompleted(operation, true);
    }

    void OnStartContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = ContainerRuntimeHandler::EndStartContainer(operation);
        if (error.IsSuccess())
        {
            this->TryComplete(operation->Parent, error);
        }
        else
        {
            WriteInfo(
                    TraceType, owner_.Root.TraceId,
                    "Unsuccessful StartContainer for AppHostId {0} NodeId {1}: containerName {2}. Error: {3}",
                    appHostId_,
                    nodeId_,
                    containerDescription_.ContainerName,
                    error);
            err_.Overwrite(error);
            RemoveContainer(operation->Parent);
        }
    }

    void RemoveContainer(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
                TraceType, owner_.Root.TraceId,
                "RemoveContainer for AppHostId {0} NodeId {1}: containerName {2}",
                appHostId_,
                nodeId_,
                containerDescription_.ContainerName);

        auto operation = ContainerRuntimeHandler::BeginRemoveContainer(
                containerDescription_.ContainerName,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnRemoveContainerCompleted(operationSPtr, false);
                },
                thisSPtr);

        OnRemoveContainerCompleted(operation, true);
    }

    void OnRemoveContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = ContainerRuntimeHandler::EndRemoveContainer(operation);
        if (!error.IsSuccess())
        {
            WriteInfo(
                    TraceType, owner_.Root.TraceId,
                    "Unsuccessful RemoveContainer for AppHostId {0} NodeId {1}: containerName {2}, Error: {3}",
                    appHostId_,
                    nodeId_,
                    containerDescription_.ContainerName,
                    error);
        }
        else
        {
            ContainerHelper::GetContainerHelper().MarkContainerLogFolder(containerDescription_.ContainerName);
        }

        this->UnmapVolumes(operation->Parent);
    }

    void MapVolumes(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ContainerVolumeDescription> pluginProvidedVolumes;
        for (auto const & volume : containerDescription_.ContainerVolumes)
        {
            if (volume.Driver.empty())
            {
                localVolumes_.push_back(volume);
            }
            else
            {
                pluginProvidedVolumes.push_back(volume);
            }
        }

        WriteInfo(
            TraceType, owner_.Root.TraceId,
            "Mapping {0} volumes for container {1}. AppHostId {2} NodeId {3}",
            pluginProvidedVolumes.size(),
            containerDescription_.ContainerName,
            appHostId_,
            nodeId_);

        auto operation = VolumeMapper::BeginMapAllVolumes(
            containerDescription_.ContainerName,
            move(pluginProvidedVolumes),
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnMapAllVolumesCompleted(operation, false);
            },
            thisSPtr);
        this->OnMapAllVolumesCompleted(operation, true);
    }

private:
    void OnMapAllVolumesCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        vector<ContainerVolumeDescription> mappedVolumes;
        ErrorCode error = VolumeMapper::EndMapAllVolumes(operation, mappedVolumes);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType, owner_.Root.TraceId,
                "Failed to map all volumes for container {0}. AppHostId {1} NodeId {2}. Error: {3}.",
                containerDescription_.ContainerName,
                appHostId_,
                nodeId_,
                error);
            this->TryComplete(operation->Parent, move(error));
            return;
        }

        WriteInfo(
            TraceType, owner_.Root.TraceId,
            "Finished mapping volumes for container {0}. AppHostId {1} NodeId {2}",
            containerDescription_.ContainerName,
            appHostId_,
            nodeId_);

        error = ContainerConfig::CreateContainerConfig(
            localVolumes_,
            mappedVolumes,
            processDescription_,
            containerDescription_,
            L"",
            config_);
        if (!error.IsSuccess())
        {
            this->TryComplete(operation->Parent, move(error));
            return;
        }

        this->CreateContainer(operation->Parent);
    }

    void UnmapVolumes(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ContainerVolumeDescription> pluginProvidedVolumes;
        for (auto const & volume : containerDescription_.ContainerVolumes)
        {
            if (!volume.Driver.empty())
            {
                pluginProvidedVolumes.push_back(volume);
            }
        }
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Unmapping {0} volumes for {1} (after activation failure). AppHostId {2} NodeId {3}",
            pluginProvidedVolumes.size(),
            containerDescription_.ContainerName,
            appHostId_,
            nodeId_);

        auto operation = VolumeMapper::BeginUnmapAllVolumes(
            containerDescription_.ContainerName,
            move(pluginProvidedVolumes),
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUnmapVolumesCompleted(operation, false);
            },
            thisSPtr);
        this->OnUnmapVolumesCompleted(operation, true);
    }

    void OnUnmapVolumesCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = VolumeMapper::EndUnmapAllVolumes(operation);
        if (!error.IsSuccess())
        {
            // TODO: Add some retry logic on unmap failures
            WriteWarning(
                TraceType, owner_.Root.TraceId,
                "Failed to unmap all volumes for {0}. Error: {1} (after activation failure). AppHostId {2} NodeId {3}",
                containerDescription_.ContainerName,
                error,
                appHostId_,
                nodeId_);
        }
        else
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "Finished unmapping all volumes for {0} (after activation failure). AppHostId {1} NodeId {2}",
                containerDescription_.ContainerName,
                appHostId_,
                nodeId_);
        }

        TryComplete(operation->Parent, err_);
    }

private:
    ErrorCode err_;
    ContainerConfig config_;
    std::vector<ContainerVolumeDescription> localVolumes_;
    int pullRetries_;
};

// ********************************************************************************************************************
// ContainerActivator::DeactivateClearContainersAsyncOperation Implementation
//
class ContainerActivator::DeactivateClearContainersAsyncOperation : public DeactivateAsyncOperation
{
DENY_COPY(DeactivateClearContainersAsyncOperation)

public:
    DeactivateClearContainersAsyncOperation(
            __in ContainerActivator & owner,
            ContainerDescription const & containerDescription,
            bool configuredForAutoremove,
            bool isContainerRoot,
            wstring const & cgroupName,
            bool isGracefulDeactivation,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : DeactivateAsyncOperation(owner, containerDescription.ContainerName, configuredForAutoremove, isContainerRoot,
                                       cgroupName, isGracefulDeactivation, timeout, callback, parent)
            , containerDescription_(containerDescription)
    {
        isIsolated_ = true;
    }

    virtual ~DeactivateClearContainersAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateClearContainersAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        if (timeout.Ticks == 0)
        {
            timeout = HostingConfig::GetConfig().ContainerDeactivationQueueTimeount;
            WriteInfo(
                    TraceType,
                    "Deactivation timeout {0}. Current retry count {1}",
                    timeout,
                    queueRetryCount_);
        }

        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
                timeout,
                [thisSPtr, this](IContainerActivator &)
                {
                    this->initialEnqueueSucceeded_ = true;
                    if (isContainerRoot_)
                    {
                        this->StopPodSandbox(thisSPtr);
                    }
                    else
                    {
                        this->StopContainer(thisSPtr);
                    }
                },
                [thisSPtr, this](IContainerActivator &)
                {
                    WriteWarning(
                            TraceType,
                            "Operation timed out before job queue item was processed. Current retry count {0}, container {1}",
                            this->queueRetryCount_,
                            this->containerName_);
                    //forcefully kill container on timeout
                    this->timeoutHelper_.SetRemainingTime(HostingConfig().GetConfig().ContainerDeactivationQueueTimeount);
                    this->initialEnqueueSucceeded_ = true;
                    if (isContainerRoot_)
                    {
                        this->StopPodSandbox(thisSPtr);
                    }
                    else
                    {
                        this->StopContainer(thisSPtr);
                    }
                });

        if (!this->owner_.JobQueueObject.Enqueue(move(jobItem)))
        {
            WriteWarning(
                    TraceType,
                    "Could not enqueue operation to JobQueue.");
            this->TryComplete(thisSPtr, ErrorCodeValue::FileStoreServiceNotReady);
        }
    }

    void StopPodSandbox(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceType, owner_.Root.TraceId, "StopPodSandbox for PodName {0}", containerName_);

        auto operation = ContainerRuntimeHandler::BeginStopPodSandbox(
                containerName_,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnStopPodSandboxCompleted(operationSPtr, false);
                },
                thisSPtr);

        OnStopPodSandboxCompleted(operation, true);
    }

    void OnStopPodSandboxCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = ContainerRuntimeHandler::EndStopPodSandbox(operation);
        if (!error.IsSuccess())
        {
            WriteInfo(
                    TraceType, owner_.Root.TraceId,
                    "Unsuccessful StopPodSandbox for {0}",
                    containerName_);
        }
        this->RemovePodSandbox(operation->Parent);
    }

    void RemovePodSandbox(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = ContainerRuntimeHandler::BeginRemovePodSandbox(
                containerName_,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnRemovePodSandboxCompleted(operationSPtr, false);
                },
                thisSPtr);

        OnRemovePodSandboxCompleted(operation, true);
    }

    void OnRemovePodSandboxCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = ContainerRuntimeHandler::EndRemovePodSandbox(operation);
        if (!error.IsSuccess())
        {
            WriteInfo(
                    TraceType, owner_.Root.TraceId,
                    "Unsuccessful RemovePodSandbox for {0}",
                    containerName_);
        }
        this->UnmapAllVolumes(operation->Parent);
    }

    void UnmapAllVolumes(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ContainerVolumeDescription> pluginProvidedVolumes;
        for (auto const & volume : containerDescription_.ContainerVolumes)
        {
            if (!volume.Driver.empty())
            {
                pluginProvidedVolumes.push_back(volume);
            }
        }
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Unmapping {0} volumes for {1}.",
            pluginProvidedVolumes.size(),
            containerName_);

        auto operation = VolumeMapper::BeginUnmapAllVolumes(
            containerName_,
            move(pluginProvidedVolumes),
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUnmapAllVolumesCompleted(operation, false);
            },
            thisSPtr);
        this->OnUnmapAllVolumesCompleted(operation, true);
    }

    void OnUnmapAllVolumesCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = VolumeMapper::EndUnmapAllVolumes(operation);
        if (!error.IsSuccess())
        {
            // TODO: Add some retry logic on unmap failures
            WriteWarning(
                TraceType, owner_.Root.TraceId,
                "Failed to unmap all volumes for {0}. Error: {1}",
                containerName_,
                error);
        }
        else
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "Finished unmapping all volumes for {0}.",
                containerName_);
        }
        CleanupCgroup(operation->Parent);
    }

    void CleanupCgroup(AsyncOperationSPtr const & thisSPtr)
    {
        if (isContainerRoot_ && !cgroupName_.empty())
        {
            string cgroupName;
            StringUtility::Utf16ToUtf8(cgroupName_, cgroupName);
            auto error = ProcessActivator::RemoveCgroup(cgroupName, true);
            WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                       TraceType,
                       owner_.Root.TraceId,
                       "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                       cgroupName_, error, cgroup_get_last_errno());
        }

        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    void StopContainer(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
                TraceType, owner_.Root.TraceId,
                "StopContainer for {0}",
                containerName_);

        auto operation = ContainerRuntimeHandler::BeginStopContainer(
                containerName_,
                0,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnStopContainerCompleted(operationSPtr, false);
                },
                thisSPtr);

        OnStopContainerCompleted(operation, true);
    }

    void OnStopContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = ContainerRuntimeHandler::EndStopContainer(operation);
        if (!error.IsSuccess())
        {
            WriteInfo(
                    TraceType, owner_.Root.TraceId,
                    "Unsuccessful StopContainer for {0}",
                    containerName_);
        }
        this->RemoveContainer(operation->Parent);
    }

    void RemoveContainer(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
                TraceType, owner_.Root.TraceId,
                "RemoveContainer for {0}",
                containerName_);
        auto operation = ContainerRuntimeHandler::BeginRemoveContainer(
                containerName_,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnRemoveContainerCompleted(operationSPtr, false);
                },
                thisSPtr);

        OnRemoveContainerCompleted(operation, true);
    }

    void OnRemoveContainerCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = ContainerRuntimeHandler::EndRemoveContainer(operation);
        if (!error.IsSuccess())
        {
            WriteInfo(
                    TraceType, owner_.Root.TraceId,
                    "Unsuccessful RemoveContainer for {0}",
                    containerName_);
        }
        else
        {
            ContainerHelper::GetContainerHelper().MarkContainerLogFolder(containerName_);
        }

        this->StopPodSandbox(operation->Parent);
    }

private:
    ContainerDescription containerDescription_;
};

class ContainerActivator::GetContainerLogsClearContainersAsyncOperation : public GetContainerLogsAsyncOperation
{
DENY_COPY(GetContainerLogsClearContainersAsyncOperation)

public:
    GetContainerLogsClearContainersAsyncOperation(
            __in ContainerActivator & owner,
            Hosting2::ContainerDescription const & containerDescription,
            bool isServicePackageActivationModeExclusive,
            wstring const & tailArg,
            int previous,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : GetContainerLogsAsyncOperation(owner, containerDescription, isServicePackageActivationModeExclusive,
                                             tailArg, previous, timeout, callback, parent)
    {
        isIsolated_ = true;
    }

    virtual ~GetContainerLogsClearContainersAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & containerLogs)
    {
        auto thisPtr = AsyncOperation::End<GetContainerLogsClearContainersAsyncOperation>(operation);
        containerLogs = thisPtr->containerLogs_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (previous_ > 0)
        {
            GetPreviousLogs(thisSPtr);
        }
        else
        {
            GetLogs(thisSPtr);
        }
    }

    // Get current logs
    void GetLogs(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = ContainerRuntimeHandler::BeginGetContainerLog(
                containerName_,
                tailArg_,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnGetContainerLogCompleted(operationSPtr, false);
                },
                thisSPtr);
        OnGetContainerLogCompleted(operation, true);
    }

    void OnGetContainerLogCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = ContainerRuntimeHandler::EndGetContainerLog(operation, containerLogs_);
        TryComplete(operation->Parent, error);
    }

    // Get previous logs
    void GetPreviousLogs(AsyncOperationSPtr const & thisSPtr)
    {
        // get list of containers
        auto labelSelectorFilter = std::map<std::wstring, std::wstring>();
        labelSelectorFilter.insert(
            make_pair(Constants::ContainerLabels::DigestedApplicationNameLabelKeyName,
                      ClearContainerHelper::GetDigestedApplicationName(containerDescription_)));

        labelSelectorFilter.insert(
            make_pair(Constants::ContainerLabels::CodePackageNameLabelKeyName,
                      containerDescription_.CodePackageName));

        labelSelectorFilter.insert(
            make_pair(Constants::ContainerLabels::PartitionIdLabelKeyName,
                      containerDescription_.PartitionId));

        auto operation = ContainerRuntimeHandler::BeginListContainers(
                labelSelectorFilter,
                L"ContainerActivator::GetPreviousLogs",
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnGetContainerListCompleted(operationSPtr, false);
                },
                thisSPtr);

        OnGetContainerListCompleted(operation, true);
    }

    void OnGetContainerListCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        const auto & thisSPtr = operation->Parent;

        // filter list of containers and get the second most recent one
        vector<runtime::Container> containers;
        ErrorCode error = ContainerRuntimeHandler::EndListContainers(operation, containers);

        // sort containers newest (first) to oldest (last)
        std::sort(containers.begin(), containers.end(),
            [](const runtime::Container & a, const runtime::Container & b)
            {
                return a.created_at() > b.created_at();
            });

        if (containers.size() < 2)
        {
            WriteInfo(
                TraceType, owner_.Root.TraceId,
                "ContainerActivator:GetContainerLogsClearContainersAsyncOperation : Get previous container logs requested but no previous container exists.");

            ErrorCode failureError(
                ErrorCodeValue::Timeout,
                move(wstring(L"There is no previous container logs for requested container.")));
            TryComplete(thisSPtr, failureError);

            return;
        }

        // get the logs for that container
        // search up log path and use get container log by path to get the logs

        // ClearContainerHelper::GetContainerFullLogPath accepts a container description
        // so we'll create a mock container description using the values from the previous container
        // returned by the list container call.
        const auto& previousContainer = containers[1];
        auto previousContainerLabels = previousContainer.labels();
        std::wstring previousContainerAppName = StringUtility::Utf8ToUtf16(previousContainerLabels[StringUtility::Utf16ToUtf8(Constants::ContainerLabels::ApplicationNameLabelKeyName)]);
        std::wstring previousContainerPartitionId = StringUtility::Utf8ToUtf16(previousContainerLabels[StringUtility::Utf16ToUtf8(Constants::ContainerLabels::PartitionIdLabelKeyName)]);
        std::wstring previousContainerServicePackageActivationId = StringUtility::Utf8ToUtf16(previousContainerLabels[StringUtility::Utf16ToUtf8(Constants::ContainerLabels::ServicePackageActivationIdLabelKeyName)]);
        std::wstring previousContainerCodePackageName = StringUtility::Utf8ToUtf16(previousContainerLabels[StringUtility::Utf16ToUtf8(Constants::ContainerLabels::CodePackageNameLabelKeyName)]);

        uint64 codePackageInstanceId = std::strtoull(
            previousContainerLabels[StringUtility::Utf16ToUtf8(Constants::ContainerLabels::CodePackageInstanceLabelKeyName)].c_str(),
            NULL,
            10);

        auto fullLogPath = ClearContainerHelper::GetContainerFullLogPath(
                                previousContainerAppName,
                                previousContainerPartitionId,
                                previousContainerServicePackageActivationId,
                                previousContainerCodePackageName,
                                codePackageInstanceId);

        auto nextOperation = ContainerRuntimeHandler::BeginGetContainerLogByPath(
                fullLogPath,
                tailArg_,
                owner_.Root.TraceId,
                TimeSpan::MaxValue,
                [&](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    OnGetPreviousContainerLogCompleted(operationSPtr, false);
                },
                thisSPtr);
        OnGetPreviousContainerLogCompleted(nextOperation, true);
    }

    void OnGetPreviousContainerLogCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        // return the logs
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = ContainerRuntimeHandler::EndGetContainerLogByPath(operation, containerLogs_);

        TryComplete(operation->Parent, error);
    }
};
#endif

class ContainerActivator::ContainerUpdateRoutesAsyncOperation : public AsyncOperation
{
    DENY_COPY(ContainerUpdateRoutesAsyncOperation)

public:
    ContainerUpdateRoutesAsyncOperation(
        __in ContainerActivator & owner,
        wstring const & containerId,
        ContainerDescription const & containerDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        containerId_(containerId),
        containerDescription_(containerDescription),
        owner_(owner),
        timeoutHelper_(timeout),
        initialEnqueueSucceeded_(false)
    {
    }

    virtual ~ContainerUpdateRoutesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ContainerUpdateRoutesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }

        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](IContainerActivator &)
        {
            this->initialEnqueueSucceeded_ = true;
            this->ContainerUpdateRoutes(thisSPtr);
        },
            [thisSPtr, this](IContainerActivator &)
        {
            initialEnqueueSucceeded_ = true;

            WriteWarning(
                TraceType,
                "Operation timed out before container update routes queue item was processed.");

            ErrorCode error(
                ErrorCodeValue::Timeout,
                move(wstring(L"Operation timed out before container update routes queue item was processed.")));

            this->TryComplete(thisSPtr, error);
        });

        if (!this->owner_.JobQueueObject.Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType,
                "Could not enqueue container update routes operation to JobQueue.");

            ErrorCode error(
                ErrorCodeValue::InvalidOperation,
                move(wstring(L"Could not enqueue container update routes operation to JobQueue.")));

            this->TryComplete(thisSPtr, error);
        }
    }

    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.JobQueueObject.CompleteAsyncJob();
        }
    }

private:
    void ContainerUpdateRoutes(AsyncOperationSPtr const & thisSPtr)
    {
        auto request = CreateContainerUpdateRoutesRequestMessage();

        auto operation = owner_.BeginSendMessage(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->FinishContainerUpdateRoutes(operation, false);
        },
            thisSPtr);

        FinishContainerUpdateRoutes(operation, true);
    }

    void FinishContainerUpdateRoutes(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.EndSendMessage(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishContainerUpdateRoutes: Error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        ContainerOperationReply replyBody;
        if (!reply->GetBody<ContainerOperationReply>(replyBody))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishContainerUpdateRoutes: GetBody<ContainerOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishContainerUpdateRoutes: replyBody.Error={0}, replyBody.ErrorMessage={1}.",
                error,
                replyBody.ErrorMessage);
        }

        TryComplete(
            operation->Parent,
            ErrorCode(error.ReadValue(), replyBody.TakeErrorMessage()));
    }

    MessageUPtr CreateContainerUpdateRoutesRequestMessage()
    {
        // Retrieve container gateway ip addresses
        std::vector<std::wstring> gatewayIpAddresses;
        owner_.RetrieveGatewayIpAddresses(containerDescription_, gatewayIpAddresses);

        ContainerUpdateRoutesRequest requestBody(
            containerId_,
            containerDescription_.ContainerName,
            containerDescription_.ApplicationId,
            containerDescription_.ApplicationName,
            containerDescription_.NetworkConfig.NetworkType,
            gatewayIpAddresses,
            containerDescription_.AutoRemove,
            containerDescription_.IsContainerRoot,
            containerDescription_.GroupContainerName,
            timeoutHelper_.GetRemainingTime().Ticks);

        auto request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ContainerActivatorService));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ContainerUpdateRoutes));

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "ContainerUpdateRoutesRequestMessage: Message={0}, Body={1}",
            *request,
            requestBody);

        return move(request);
    }

private:
    wstring containerId_;
    ContainerDescription containerDescription_;
    ContainerActivator & owner_;
    TimeoutHelper timeoutHelper_;
    bool initialEnqueueSucceeded_;
};

ContainerActivator::ContainerActivator(
    ComponentRoot const & root,
    ProcessActivationManager & processActivationManager,
    ContainerTerminationCallback const & terminationCallback,
    ContainerEngineTerminationCallback const & engineTerminationCallback,
    ContainerHealthCheckStatusCallback const & healthCheckStatusCallback)
    : IContainerActivator(root)
    , processActivationManager_(processActivationManager)
    , containerServiceProcessId_(0)
    , eventTracker_()
    , terminationCallback_(terminationCallback)
    , engineTerminationCallback_(engineTerminationCallback)
    , healthCheckStatusCallback_(healthCheckStatusCallback)
#if defined(PLATFORM_UNIX)
    , containerStatusCallback_(0)
    , podSandboxStatusCallback_(0)
#endif

{
#if !defined(PLATFORM_UNIX)
    EventRegisterMicrosoft_Windows_KTL();
#endif

    auto dockerManager = make_unique<DockerProcessManager>(root, engineTerminationCallback, processActivationManager);
    auto ipProvider = make_unique<IPAddressProvider>(make_shared<ComponentRoot>(root), processActivationManager);
    auto natIpProvider = make_unique<NatIPAddressProvider>(make_shared<ComponentRoot>(root), *this);
    auto overlayNetworkManager = make_unique<OverlayNetworkManager>(make_shared<ComponentRoot>(root), processActivationManager);
    auto containerNetworkOperations = make_unique<ContainerNetworkOperations>(make_shared<ComponentRoot>(root));
    auto eventTracker = make_unique<ContainerEventTracker>(*this);

    dockerProcessManager_ = move(dockerManager);
    ipAddressProvider_ = move(ipProvider);
    natIpAddressProvider_ = move(natIpProvider);
    overlayNetworkManager_ = move(overlayNetworkManager);
    containerNetworkOperations_ = move(containerNetworkOperations);
    eventTracker_ = move(eventTracker);

    networkPluginProcessRestartedCallback_ = [this]()
    {
        this->overlayNetworkManager_->RequestPublishNetworkTables();
    };

#if defined(PLATFORM_UNIX)
    containerStatusCallback_ = ContainerRuntimeHandler::RegisterContainerStateUpdateCallback(
            [](string const & apphostid, string const & nodeid, string const & name,
               ContainerRuntimeHandler::ContainerState from, ContainerRuntimeHandler::ContainerState to, void *ptr)
            {
                if (to == ContainerRuntimeHandler::ContainerState::Exited || to == ContainerRuntimeHandler::ContainerState::Unknown)
                {
                    wstring apphostidW = Common::StringUtility::Utf8ToUtf16(apphostid);
                    wstring nodeidW = Common::StringUtility::Utf8ToUtf16(nodeid);
                    ((ContainerActivator *)ptr)->processActivationManager_.OnContainerTerminated(apphostidW, nodeidW, -1);
                }
            }, this);

    podSandboxStatusCallback_ = ContainerRuntimeHandler::RegisterPodSandboxStateUpdateCallback(
            [](string const & apphostid, string const & nodeid, string const & name,
               ContainerRuntimeHandler::PodSandboxState from, ContainerRuntimeHandler::PodSandboxState to, void *ptr)
            {
                if (to == ContainerRuntimeHandler::PodSandboxState::NotReady)
                {
                    wstring apphostidW = Common::StringUtility::Utf8ToUtf16(apphostid);
                    wstring nodeidW = Common::StringUtility::Utf8ToUtf16(nodeid);
                    ((ContainerActivator *)ptr)->processActivationManager_.OnContainerTerminated(apphostidW, nodeidW, -1);
                }
            }, this);

    auto networkPluginProcessManager = make_unique<NetworkPluginProcessManager>(
        root,
        processActivationManager,
        networkPluginProcessRestartedCallback_,
        HostingConfig::GetConfig().AzureVnetPluginName,
        L"",
        true, // plugin setup 
        HostingConfig::GetConfig().AzureVnetPluginProcessManagerInitTimeout,
        HostingConfig::GetConfig().AzureVnetPluginActivationExceptionInterval,
        HostingConfig::GetConfig().AzureVnetPluginActivationRetryBackoffInterval,
        HostingConfig::GetConfig().AzureVnetPluginActivationMaxFailureCount);

    this->networkPluginProcessManager_ = move(networkPluginProcessManager);
#else
    auto networkPluginProcessManager = make_unique<NetworkPluginProcessManager>(
        root,
        processActivationManager,
        networkPluginProcessRestartedCallback_,
        HostingConfig::GetConfig().IsolatedNetworkPluginName,
        L"",
        true, // plugin setup
        HostingConfig::GetConfig().IsolatedNetworkPluginProcessManagerInitTimeout,
        HostingConfig::GetConfig().IsolatedNetworkPluginActivationExceptionInterval,
        HostingConfig::GetConfig().IsolatedNetworkPluginActivationRetryBackoffInterval,
        HostingConfig::GetConfig().IsolatedNetworkPluginActivationMaxFailureCount);

    this->networkPluginProcessManager_ = move(networkPluginProcessManager);
#endif
}

#if defined(PLATFORM_UNIX)
ContainerActivator::~ContainerActivator()
{
    if (containerStatusCallback_)
    {
        ContainerRuntimeHandler::UnregisterContainerStateUpdateCallback(containerStatusCallback_);
    }
    if (podSandboxStatusCallback_)
    {
        ContainerRuntimeHandler::UnregisterPodSandboxStateUpdateCallback(podSandboxStatusCallback_);
    }
}
#endif

AsyncOperationSPtr ContainerActivator::BeginActivate(
    bool isSecUserLocalSystem,
    wstring const & appHostId,
    wstring const & nodeId,
    ContainerDescription const & containerDescription,
    ProcessDescription const & processDescription,
    wstring const & fabricbinPath,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
#if defined(PLATFORM_UNIX)
    if (containerDescription.PodDescription.IsolationMode == ContainerIsolationMode::hyperv)
    {
        return AsyncOperation::CreateAndStart<ActivateClearContainersAsyncOperation>(
                *this,
                isSecUserLocalSystem,
                appHostId,
                nodeId,
                containerDescription,
                processDescription,
                fabricbinPath,
                timeout,
                callback,
                parent);
    }
#endif

    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        isSecUserLocalSystem,
        appHostId,
        nodeId,
        containerDescription,
        processDescription,
        fabricbinPath,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndActivate(
    AsyncOperationSPtr const & operation,
    __out std::wstring & containerId)
{
#if defined(PLATFORM_UNIX)
    auto op = std::dynamic_pointer_cast<ActivateAsyncOperation>(operation);
    if (op->IsIsolated)
    {
        return ActivateClearContainersAsyncOperation::End(operation);
    }
#endif
    return ActivateAsyncOperation::End(operation, containerId);
}

AsyncOperationSPtr ContainerActivator::BeginQuery(
    ContainerDescription const & containerDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetContainerInfoAsyncOperation>(
        *this,
        containerDescription.ContainerName,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndQuery(
    AsyncOperationSPtr const & operation,
    __out ContainerInspectResponse & containerInspect)
{
    return GetContainerInfoAsyncOperation::End(operation, containerInspect);
}

AsyncOperationSPtr ContainerActivator::BeginDeactivate(
    ContainerDescription const & containerDescription,
    bool configuredForAutoRemove,
    bool isContainerRoot,
#if defined(PLATFORM_UNIX)
    bool isContainerIsolated,
#endif
    wstring const & cgroupName,
    bool isGracefulDeactivation,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
#if defined(PLATFORM_UNIX)
    if (isContainerIsolated)
    {
        return AsyncOperation::CreateAndStart<DeactivateClearContainersAsyncOperation>(
                *this,
                containerDescription,
                configuredForAutoRemove,
                isContainerRoot,
                cgroupName,
                isGracefulDeactivation,
                timeout,
                callback,
                parent);
    }
#endif
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        containerDescription.ContainerName,
        configuredForAutoRemove,
        isContainerRoot,
        cgroupName,
        isGracefulDeactivation,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
#if defined(PLATFORM_UNIX)
    auto op = std::dynamic_pointer_cast<DeactivateAsyncOperation>(operation);
    if (op->IsIsolated)
    {
        return DeactivateClearContainersAsyncOperation::End(operation);
    }
#endif
    return DeactivateAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerActivator::BeginGetLogs(
    Hosting2::ContainerDescription const & containerDescription,
    bool isServicePackageActivationModeExclusive,
    wstring const & tailArg,
    int previous,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
#if defined(PLATFORM_UNIX)
    if (ContainerRuntimeHandler::ContainerExists(containerDescription.ContainerName))
    {
        return AsyncOperation::CreateAndStart<GetContainerLogsClearContainersAsyncOperation>(
                *this,
                containerDescription,
                isServicePackageActivationModeExclusive,
                tailArg,
                previous,
                timeout,
                callback,
                parent);
    }
#endif
    return AsyncOperation::CreateAndStart<GetContainerLogsAsyncOperation>(
        *this,
        containerDescription,
        isServicePackageActivationModeExclusive,
        tailArg,
        previous,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndGetLogs(
    AsyncOperationSPtr const & operation,
    __out wstring & containerLogs)
{
#if defined(PLATFORM_UNIX)
    auto op = std::dynamic_pointer_cast<GetContainerLogsAsyncOperation>(operation);
    if (op->IsIsolated)
    {
        return GetContainerLogsClearContainersAsyncOperation::End(operation, containerLogs);
    }
#endif
    return GetContainerLogsAsyncOperation::End(operation, containerLogs);
}

AsyncOperationSPtr ContainerActivator::BeginDownloadImages(
    vector<ContainerImageDescription> const & images,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
        *this,
        images,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndDownloadImages(
    AsyncOperationSPtr const & operation)
{
    return DownloadContainerImagesAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerActivator::BeginDeleteImages(
    vector<wstring> const & images,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeleteContainerImagesAsyncOperation>(
        *this,
        images,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndDeleteImages(
    AsyncOperationSPtr const & operation)
{
    return DeleteContainerImagesAsyncOperation::End(operation);
}


AsyncOperationSPtr ContainerActivator::BeginGetStats(
    Hosting2::ContainerDescription const & containerDescription,
    Hosting2::ProcessDescription const & processDescription,
    std::wstring const & parentId,
    std::wstring const & appServiceId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetContainerStatsAsyncOperation>(
        *this,
        containerDescription,
        processDescription,
        parentId,
        appServiceId,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndGetStats(
    AsyncOperationSPtr const & operation,
    __out uint64 & memoryUsage,
    __out uint64 & totalCpuTime,
    __out Common::DateTime & timeRead)
{
    return GetContainerStatsAsyncOperation::End(operation, memoryUsage, totalCpuTime, timeRead);
}

AsyncOperationSPtr ContainerActivator::BeginInvokeContainerApi(
    ContainerDescription const & containerDescription,
    wstring const & httpVerb,
    wstring const & uriPath,
    wstring const & contentType,
    wstring const & requestBody,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerApiOperation>(
        *this,
        containerDescription,
        httpVerb,
        uriPath,
        contentType,
        requestBody,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndInvokeContainerApi(
    AsyncOperationSPtr const & operation,
    __out wstring & result)
{
    return ContainerApiOperation::End(operation, result);
}

void ContainerActivator::TerminateContainer(
    ContainerDescription const & containerDescription,
    bool isContainerRoot,
    wstring const & cgroupName)
{
    AsyncOperationWaiterSPtr cleanupWaiter = make_shared<AsyncOperationWaiter>();

#if defined(PLATFORM_UNIX)
    if ( (!isContainerRoot && ContainerRuntimeHandler::ContainerExists(containerDescription.ContainerName))
         || (isContainerRoot && ContainerRuntimeHandler::PodExists(containerDescription.ContainerName)) )
    {
        auto operation = AsyncOperation::CreateAndStart<DeactivateClearContainersAsyncOperation>(
                *this,
                containerDescription,
                true,
                isContainerRoot,
                cgroupName,
                false,
                HostingConfig::GetConfig().ActivationTimeout,
                [cleanupWaiter](AsyncOperationSPtr const & operation)
                {
                    auto error = DeactivateAsyncOperation::End(operation);
                    cleanupWaiter->SetError(error);
                    cleanupWaiter->Set();
                },
                this->Root.CreateAsyncOperationRoot());
    }
    else
    {
#endif

        auto operation = AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
                *this,
                containerDescription.ContainerName,
                true,
                isContainerRoot,
                cgroupName,
                false,
                HostingConfig::GetConfig().ActivationTimeout,
                [cleanupWaiter](AsyncOperationSPtr const &operation) {
                    auto error = DeactivateAsyncOperation::End(operation);
                    cleanupWaiter->SetError(error);
                    cleanupWaiter->Set();
                },
                this->Root.CreateAsyncOperationRoot());

#if defined(PLATFORM_UNIX)
    }
#endif

    if (cleanupWaiter->WaitOne(HostingConfig::GetConfig().ActivationTimeout))
    {
        auto error = cleanupWaiter->GetError();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "TerminateContainer request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType,
            "TerminateContainer cleanupWaiter failed on Wait");
    }
}

ErrorCode ContainerActivator::AllocateIPAddresses(
    wstring const & nodeId,
    wstring const & servicePackageId,
    vector<wstring> const & codePackageNames,
    vector<wstring> & assignedIps)
{
    if (HostingConfig::GetConfig().LocalNatIpProviderEnabled)
    {
        return this->natIpAddressProvider_->AcquireIPAddresses(nodeId, servicePackageId, codePackageNames, assignedIps);
    }
    else
    {
        return this->ipAddressProvider_->AcquireIPAddresses(nodeId, servicePackageId, codePackageNames, assignedIps);
    }
}

void ContainerActivator::ReleaseIPAddresses(
    wstring const & nodeId,
    wstring const & servicePackageId)
{
    if (HostingConfig::GetConfig().LocalNatIpProviderEnabled)
    {
        this->natIpAddressProvider_->ReleaseIpAddresses(nodeId, servicePackageId);
    }
    else
    {
        this->ipAddressProvider_->ReleaseIpAddresses(nodeId, servicePackageId);
    }
}

void ContainerActivator::CleanupAssignedIpsToNode(
    wstring const & nodeId)
{
    if (HostingConfig::GetConfig().LocalNatIpProviderEnabled)
    {
        this->natIpAddressProvider_->ReleaseAllIpsForNode(nodeId);
    }
    else
    {
        this->ipAddressProvider_->ReleaseAllIpsForNode(nodeId);
    }
}

Common::AsyncOperationSPtr ContainerActivator::BeginAssignOverlayNetworkResources(
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & servicePackageId,
    std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return containerNetworkOperations_->BeginAssignOverlayNetworkResources(
        *this,
        nodeId,
        nodeName,
        nodeIpAddress,
        servicePackageId,
        codePackageNetworkNames,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndAssignOverlayNetworkResources(
    Common::AsyncOperationSPtr const & operation,
    __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources)
{
    return containerNetworkOperations_->EndAssignOverlayNetworkResources(operation, assignedNetworkResources);
}

Common::AsyncOperationSPtr ContainerActivator::BeginReleaseOverlayNetworkResources(
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & servicePackageId,
    std::vector<std::wstring> const & networkNames,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return containerNetworkOperations_->BeginReleaseOverlayNetworkResources(
        *this,
        nodeId,
        nodeName,
        nodeIpAddress,
        servicePackageId,
        networkNames,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndReleaseOverlayNetworkResources(
    Common::AsyncOperationSPtr const & operation)
{
    return containerNetworkOperations_->EndReleaseOverlayNetworkResources(operation);
}

Common::AsyncOperationSPtr ContainerActivator::BeginCleanupAssignedOverlayNetworkResourcesToNode(
    std::wstring const & nodeId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return containerNetworkOperations_->BeginCleanupAssignedOverlayNetworkResourcesToNode(
        *this,
        nodeId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndCleanupAssignedOverlayNetworkResourcesToNode(
    Common::AsyncOperationSPtr const & operation)
{
    return containerNetworkOperations_->EndCleanupAssignedOverlayNetworkResourcesToNode(operation);
}

Common::AsyncOperationSPtr ContainerActivator::BeginAttachContainerToNetwork(
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & containerId,
    std::map<std::wstring, std::wstring> const & overlayNetworkResources,
    Common::StringCollection const & dnsServerList,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    wstring dnsServersString;
    for (auto const & dns : dnsServerList)
    {
        if (dnsServersString.empty())
        {
            dnsServersString = dns;
        }
        else
        {
            dnsServersString = wformatString("{0},{1}", dnsServersString, dns);
        }
    }

    std::vector<OverlayNetworkContainerParametersSPtr> paramsList;
    for (auto const & onr : overlayNetworkResources)
    {
        auto networkDefinition = make_shared<OverlayNetworkDefinition>(
            onr.first, // network name
            L"", // network id
            Common::ContainerEnvironment::ContainerOverlayNetworkTypeName, // network type
            L"", // subnet
            L"", // gateway
            0, // mask
            0, // vxlan id
            L"", // start mac pool address
            L"", // end mac pool address
            nodeIpAddress,
            nodeId,
            nodeName,
            std::map<std::wstring, std::wstring>(), // ip and mac address map to be added
            std::map<std::wstring, std::wstring>()); // ip and mac address map to be removed

        wstring ipAddress;
        wstring macAddress;
        StringUtility::SplitOnce(onr.second, ipAddress, macAddress, L",");

        auto params = make_shared<OverlayNetworkContainerParameters>(
            networkDefinition,
            ipAddress,
            macAddress,
            containerId,
            dnsServersString);

        paramsList.push_back(params);
    }

    return containerNetworkOperations_->BeginAttachContainerToNetwork(
        *this,
        paramsList,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndAttachContainerToNetwork(
    Common::AsyncOperationSPtr const & operation)
{
    return containerNetworkOperations_->EndAttachContainerToNetwork(operation);
}

Common::AsyncOperationSPtr ContainerActivator::BeginDetachContainerFromNetwork(
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & containerId,
    std::vector<std::wstring> const & networkNames,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return containerNetworkOperations_->BeginDetachContainerFromNetwork(
        *this,
        nodeId,
        nodeName,
        nodeIpAddress,
        containerId,
        networkNames,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndDetachContainerFromNetwork(
    Common::AsyncOperationSPtr const & operation)
{
    return containerNetworkOperations_->EndDetachContainerFromNetwork(operation);
}

#if defined(PLATFORM_UNIX)
Common::AsyncOperationSPtr ContainerActivator::BeginSaveContainerNetworkParamsForLinuxIsolation(
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & containerId,
    std::wstring const & openNetworkAssignedIp,
    std::map<std::wstring, std::wstring> const & overlayNetworkResources,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    std::vector<OverlayNetworkContainerParametersSPtr> paramsList;

    if (!openNetworkAssignedIp.empty())
    {
        auto networkDefinition = make_shared<OverlayNetworkDefinition>(
            Common::ContainerEnvironment::ContainerNetworkName,
            L"", // network id
            Common::ContainerEnvironment::ContainerUnderlayNetworkTypeName,
            this->ipAddressProvider_->Subnet,
            this->ipAddressProvider_->GatewayIpAddress,
            0, // mask
            0, // vxlan id
            L"", // start mac pool address
            L"", // end mac pool address
            nodeIpAddress,
            nodeId,
            nodeName,
            std::map<std::wstring, std::wstring>(), // ip and mac address map to be added
            std::map<std::wstring, std::wstring>()); // ip and mac address map to be deleted

        auto params = make_shared<OverlayNetworkContainerParameters>(
            networkDefinition,
            openNetworkAssignedIp,
            L"", // mac address
            containerId,
            L""); // dns server list

        paramsList.push_back(params);
    }

    for (auto const & onr : overlayNetworkResources)
    {
        auto networkDefinition = make_shared<OverlayNetworkDefinition>(
            onr.first,
            L"", // network id
            Common::ContainerEnvironment::ContainerOverlayNetworkTypeName, // network type
            L"", // subnet
            L"", // gateway
            0, // mask
            0, // vxlan id
            L"", // start mac pool address
            L"", // end mac pool address
            nodeIpAddress,
            nodeId,
            nodeName,
            std::map<std::wstring, std::wstring>(), // ip mac address map to be added
            std::map<std::wstring, std::wstring>()); // ip mac address map to be removed

        wstring ipAddress;
        wstring macAddress;
        StringUtility::SplitOnce(onr.second, ipAddress, macAddress, L",");

        auto params = make_shared<OverlayNetworkContainerParameters>(
            networkDefinition,
            ipAddress,
            macAddress,
            containerId,
            L""); // dns server list

        paramsList.push_back(params);
    }

    // If both open and isolated network resources are empty, then NAT config is saved.
    if (openNetworkAssignedIp.empty() && overlayNetworkResources.empty())
    {
        auto networkDefinition = make_shared<OverlayNetworkDefinition>(
            Common::ContainerEnvironment::ContainerNatNetworkTypeName, // network name
            L"", // network id
            Common::ContainerEnvironment::ContainerNatNetworkTypeName, // network type
            L"", // subnet
            L"", // gateway
            0, // mask
            0, // vxlan id
            L"", // start mac pool address
            L"", // end mac pool address
            nodeIpAddress,
            nodeId,
            nodeName,
            std::map<std::wstring, std::wstring>(), // ip mac address map to be added
            std::map<std::wstring, std::wstring>()); // ip mac address map to be removed

        auto params = make_shared<OverlayNetworkContainerParameters>(
            networkDefinition,
            L"", // ip address
            L"", // mac address
            containerId,
            L""); // dns server list

        paramsList.push_back(params);
    }

    return containerNetworkOperations_->BeginSaveContainerNetworkParamsForLinuxIsolation(
        *this,
        paramsList,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndSaveContainerNetworkParamsForLinuxIsolation(
    Common::AsyncOperationSPtr const & operation)
{
    return containerNetworkOperations_->EndSaveContainerNetworkParamsForLinuxIsolation(operation);
}

Common::AsyncOperationSPtr ContainerActivator::BeginClearContainerNetworkParamsForLinuxIsolation(
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & containerId,
    std::vector<std::wstring> const & networkNames,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return containerNetworkOperations_->BeginClearContainerNetworkParamsForLinuxIsolation(
        *this,
        nodeId,
        nodeName,
        nodeIpAddress,
        containerId,
        networkNames,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndClearContainerNetworkParamsForLinuxIsolation(
    Common::AsyncOperationSPtr const & operation)
{
    return containerNetworkOperations_->EndClearContainerNetworkParamsForLinuxIsolation(operation);
}
#endif

Common::AsyncOperationSPtr ContainerActivator::BeginUpdateRoutes(
    OverlayNetworkRoutingInformationSPtr const & routingInfo,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return containerNetworkOperations_->BeginUpdateRoutes(
        *this,
        routingInfo,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndUpdateRoutes(
    Common::AsyncOperationSPtr const & operation)
{
    return containerNetworkOperations_->EndUpdateRoutes(operation);
}

Common::AsyncOperationSPtr ContainerActivator::BeginContainerUpdateRoutes(
    std::wstring const & containerId,
    ContainerDescription const & containerDescription,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerUpdateRoutesAsyncOperation>(
        *this,
        containerId,
        containerDescription,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ContainerActivator::EndContainerUpdateRoutes(
    Common::AsyncOperationSPtr const & operation)
{
    return ContainerUpdateRoutesAsyncOperation::End(operation);
}

void ContainerActivator::GetDeployedNetworkCodePackages(
    std::vector<std::wstring> const & servicePackageIds,
    std::wstring const & codePackageName,
    std::wstring const & networkName,
    __out std::map<std::wstring, std::vector<std::wstring>> & networkReservedCodePackages)
{
    // filter by network name, if provided
    std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> reservedCodePackages;
    if (networkName.empty())
    {
        this->IPAddressProviderObj.GetNetworkReservedCodePackages(reservedCodePackages);
        this->OverlayNetworkManagerObj.GetNetworkReservedCodePackages(reservedCodePackages);
    }
    else if (StringUtility::CompareCaseInsensitive(networkName, NetworkType::OpenStr) == 0)
    {
        this->IPAddressProviderObj.GetNetworkReservedCodePackages(reservedCodePackages);
    }
    else if (StringUtility::CompareCaseInsensitive(networkName, NetworkType::OpenStr) != 0 &&
        StringUtility::CompareCaseInsensitive(networkName, NetworkType::OtherStr) != 0)
    {
        this->OverlayNetworkManagerObj.GetNetworkReservedCodePackages(reservedCodePackages);
    }

    // filter by service package ids and code package name
    for (auto const & network : reservedCodePackages)
    {
        for (auto const & servicePackageIdsMap : network.second)
        {
            vector<std::wstring> codePackagesToBeFiltered;
            if (servicePackageIds.size() == 0)
            {
                codePackagesToBeFiltered = servicePackageIdsMap.second;
            }
            else if (std::find(servicePackageIds.begin(), servicePackageIds.end(), servicePackageIdsMap.first) != servicePackageIds.end() )
            {
                codePackagesToBeFiltered = servicePackageIdsMap.second;
            }

            vector<std::wstring> codePackages;
            for (auto const & cp : codePackagesToBeFiltered)
            {
                if (codePackageName.empty())
                {
                    codePackages.push_back(cp);
                }
                else if (StringUtility::CompareCaseInsensitive(cp, codePackageName) == 0)
                {
                    codePackages.push_back(cp);
                }
            }

            if (codePackages.size() > 0)
            {
                auto key = wformatString("{0},{1}", network.first, servicePackageIdsMap.first);
                networkReservedCodePackages.insert(make_pair(key, std::vector<wstring>()));
                auto networkIter = networkReservedCodePackages.find(key);
                networkIter->second = codePackages;
            }
        }
    }
}

void ContainerActivator::RetrieveGatewayIpAddresses(
    ContainerDescription const & containerDescription,
    __out std::vector<std::wstring> & gatewayIpAddresses)
{
    if ((containerDescription.NetworkConfig.NetworkType & NetworkType::Enum::Open) == NetworkType::Enum::Open && 
        !containerDescription.NetworkConfig.OpenNetworkAssignedIp.empty())
    {
        gatewayIpAddresses.push_back(ipAddressProvider_->GatewayIpAddress);
    }
    else if ((containerDescription.NetworkConfig.NetworkType & NetworkType::Enum::Isolated) == NetworkType::Enum::Isolated && 
        !containerDescription.NetworkConfig.OverlayNetworkResources.empty())
    {
        std::vector<std::wstring> networkNames;
        for (auto const & onr : containerDescription.NetworkConfig.OverlayNetworkResources)
        {
            networkNames.push_back(onr.first);
        }

        overlayNetworkManager_->RetrieveGatewayIpAddress(networkNames, gatewayIpAddresses);
    }
}

void ContainerActivator::GetDeployedNetworkNames(NetworkType::Enum networkType, std::vector<std::wstring> & networkNames)
{
    if ((networkType & NetworkType::Enum::Open) == NetworkType::Enum::Open && this->IPAddressProviderObj.Initialized)
    {
        networkNames.push_back(Common::ContainerEnvironment::ContainerNetworkName);
    }

    if ((networkType & NetworkType::Enum::Isolated) == NetworkType::Enum::Isolated)
    {
        this->OverlayNetworkManagerObj.GetNetworkNames(networkNames);
    }
}

void ContainerActivator::RegisterIpcRequestHandler()
{
    auto root = this->Root.CreateComponentRoot();
    this->ActivationManager.IpcServerObj.RegisterMessageHandler(
        Actor::ContainerActivatorServiceClient,
        [this, root](MessageUPtr & message, IpcReceiverContextUPtr & context)
        {
            this->ProcessIpcMessage(*message, context);
        },
        false/*dispatchOnTransportThread*/);
}

void ContainerActivator::UregisterIpcRequestHandler()
{
    this->ActivationManager.IpcServerObj.UnregisterMessageHandler(Actor::ContainerActivatorServiceClient);
}

void ContainerActivator::ProcessIpcMessage(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    auto const & action = message.Action;

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing Ipc message with action {0}",
        action);

    if (action == Hosting2::Protocol::Actions::RegisterContainerActivatorService)
    {
        this->ProcessRegisterContainerActivatorServiceRequest(message, context);
    }
    else if (message.Action == Hosting2::Protocol::Actions::ContainerEventNotificationRequest)
    {
        this->ProcessContainerEventNotification(message, context);
    }
    else
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Dropping unsupported message: {0}",
            message);
    }
}

void ContainerActivator::ProcessRegisterContainerActivatorServiceRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    RegisterContainerActivatorServiceRequest request;

    if (!message.GetBody<RegisterContainerActivatorServiceRequest>(request))
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);

        return;
    }

    WriteInfo(
        TraceType,
        Root.TraceId,
        "Register ContainerActivatorService with Pid={0} and ListenAddress={1}.",
        request.ProcessId,
        request.ListenAddress);

    auto error = this->RegisterAndMonitorContainerActivatorService(
        request.ProcessId,
        request.ListenAddress);

    SendRegisterContainerActivatorServiceReply(error, context);
}

void ContainerActivator::ProcessContainerEventNotification(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    ContainerEventNotification notification;

    if (!message.GetBody<ContainerEventNotification>(notification))
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    eventTracker_->ProcessContainerEvents(move(notification));

    //Just return message to ack reception
    auto response = make_unique<Message>(notification);
    context->Reply(move(response));
}

ErrorCode ContainerActivator::RegisterAndMonitorContainerActivatorService(
    DWORD processId,
    wstring const & listenAddress)
{
    {
        AcquireWriteLock lock(containerServiceRegistrationLock_);

        if (ipcClient_ != nullptr)
        {
            WriteError(
                TraceType,
                Root.TraceId,
                "FabricCAS with Pid={0} and ListenAddress={1} is already registered. Current requestor Pid is {2}",
                containerServiceProcessId_,
                ipcClient_->ServerTransportAddress,
                processId);

            return ErrorCode(ErrorCodeValue::AlreadyExists);
        }

        ipcClient_ = make_shared<IpcClient>(
            this->Root,
            Guid::NewGuid().ToString(),
            listenAddress,
            false /* disallow use of unreliable transport */,
            L"ContainerActivator");

        SecuritySettings ipcClientSecuritySettings;
        auto error = SecuritySettings::CreateNegotiateClient(
            SecurityConfig::GetConfig().FabricHostSpn,
            ipcClientSecuritySettings);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                this->Root.TraceId,
                "SecuritySettings.CreateNegotiate: ErrorCode={0}",
                error);

            ipcClient_.reset();
            return error;
        }

        ipcClient_->SecuritySettings = ipcClientSecuritySettings;

        error = ipcClient_->Open();

        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                this->Root.TraceId,
                "IPC Client Open: ErrorCode={0}",
                error);

            ipcClient_.reset();

            return error;
        }

        containerServiceProcessId_ = processId;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ContainerActivator::UnregisterContainerActivatorService()
{
    {
        AcquireWriteLock lock(containerServiceRegistrationLock_);

        if (ipcClient_ != nullptr)
        {
            ipcClient_->Abort();
            ipcClient_.reset();

            containerServiceProcessId_ = 0;
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ContainerActivator::RegisterNatIpAddressProvider()
{
    AsyncOperationWaiterSPtr registrationWaiter = make_shared<AsyncOperationWaiter>();
    TimeSpan timeout = TimeSpan::FromSeconds(60);

    for (int i = 0; i < NatIpRetryCount; i++)
    {
        auto operation = natIpAddressProvider_->BeginOpen(
            timeout,
            [this, registrationWaiter](AsyncOperationSPtr const & operation)
        {
            auto error = natIpAddressProvider_->EndOpen(operation);
            registrationWaiter->SetError(error);
            registrationWaiter->Set();
        },
            this->Root.CreateAsyncOperationRoot());

        if (registrationWaiter->WaitOne(timeout))
        {
            auto result = registrationWaiter->GetError();

            WriteTrace(
                result.ToLogLevel(),
                TraceType,
                "Register NatIPAddressProvider request returned error {0}.",
                result);

            if (result.IsSuccess())
            {
                return result;
            }
        }
        else
        {
            WriteWarning(
                TraceType,
                "Register NatIPAddressProvider request timed out.");
        }
        ::Sleep(NatIpRetryIntervalInMilliseconds);
    }

    WriteError(
            TraceType,
            "Unable to register NatIpAddressProvider after {0} retries. Completing with OperationFailed.", NatIpRetryCount);
    return ErrorCodeValue::OperationFailed;
}

ErrorCode ContainerActivator::UnregisterNatIpAddressProvider()
{
    ErrorCode error;
    AsyncOperationWaiterSPtr unregisterWaiter = make_shared<AsyncOperationWaiter>();
    TimeSpan timeout = TimeSpan::FromSeconds(20);

    WriteInfo(TraceType, "UnregisterNatIpAddressProvider started.");
    auto operation = natIpAddressProvider_->BeginClose(
        timeout,
        [this, unregisterWaiter](AsyncOperationSPtr const & operation)
    {
        auto error = natIpAddressProvider_->EndClose(operation);
        unregisterWaiter->SetError(error);
        unregisterWaiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    if (unregisterWaiter->WaitOne(timeout))
    {
        error = unregisterWaiter->GetError();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "UnregisterNatIPAddressProvider request returned error {0}.",
            error);
    }
    else
    {
        WriteWarning(
            TraceType,
            "UnregisterNatIPAddressProvider request timed out.}");
        error = ErrorCode(ErrorCodeValue::Timeout);
    }

    return error;
}

void ContainerActivator::AbortNatIpAddressProvider()
{
    WriteInfo(TraceType, "AbortNatIpAddressProvider");
    if (natIpAddressProvider_)
    { 
        natIpAddressProvider_->Abort();
    }
}

void ContainerActivator::SendRegisterContainerActivatorServiceReply(
    ErrorCode const & error,
    __in IpcReceiverContextUPtr & context)
{
    auto replyBody = make_unique<RegisterContainerActivatorServiceReply>(
        error,
        dockerProcessManager_->IsDockerServicePresent,
        dockerProcessManager_->IsDockerServiceManaged,
        eventTracker_->GetEventSinceTime());

    auto reply = make_unique<Message>(*replyBody);

    WriteNoise(
        TraceType,
        this->Root.TraceId,
        "Sending RegisterContainerActivatorServiceReply: Error ={0}",
        error);

    context->Reply(move(reply));
}

void ContainerActivator::OnContainerActivatorServiceTerminated()
{
    auto state = this->State.Value;
    if (state > FabricComponentState::Opened)
    {
        WriteNoise(
            TraceType,
            Root.TraceId,
            "Ignoring termination of FabricCAS as current state is {2}",
            state);

        return;
    }

    this->UnregisterContainerActivatorService();
}

bool ContainerActivator::TryGetIpcClient(__out IpcClientSPtr & ipcClient)
{
    {
        AcquireWriteLock lock(containerServiceRegistrationLock_);
        if (ipcClient_ != nullptr)
        {
            ipcClient = ipcClient_;
            return true;
        }
    }

    return false;
}

AsyncOperationSPtr ContainerActivator::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::OnEndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerActivator::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::OnEndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void ContainerActivator::OnAbort()
{
    if (ipAddressProvider_)
    {
        ipAddressProvider_->Abort();
    }

    if (natIpAddressProvider_)
    {
        natIpAddressProvider_->Abort();
    }

    if (overlayNetworkManager_)
    {
        overlayNetworkManager_->Abort();
    }

    if (networkPluginProcessManager_)
    {
        networkPluginProcessManager_->Abort();
    }

    if (dockerProcessManager_)
    {
        dockerProcessManager_->Abort();
    }

    if (requestJobQueue_)
    {
        requestJobQueue_->Close();
    }
}

AsyncOperationSPtr ContainerActivator::BeginSendMessage(
    MessageUPtr && message,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SendIpcMessageAsyncOperation>(
        *this,
        move(message),
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivator::EndSendMessage(
    AsyncOperationSPtr const & operation,
    __out MessageUPtr & reply)
{
    return SendIpcMessageAsyncOperation::End(operation, reply);
}

FabricActivationManager const & ContainerActivator::get_ActivationManager() const
{
    return processActivationManager_.ActivationManager;
}
