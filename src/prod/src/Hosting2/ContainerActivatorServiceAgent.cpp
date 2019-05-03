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
using namespace Api;

StringLiteral const TraceType("ContainerActivatorServiceAgent");

class ContainerActivatorServiceAgent::RegisterAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    RegisterAsyncOperation(
        ContainerActivatorServiceAgent & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
    {
    }

    static ErrorCode RegisterAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<RegisterAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        OpenIpcServer(thisSPtr);
    }

private:
    void OpenIpcServer(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ipcServer_->Open();
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                owner_.TraceId,
                "IPC server open failed with {0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "IPC server open succeeded. TransportListedAddress {0}",
            owner_.ipcServer_->TransportListenAddress);

        owner_.RegisterIpcRequestHandler();

        OpenIpcClient(thisSPtr);
    }

    void OpenIpcClient(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ipcClient_->Open();
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                owner_.TraceId,
                "IPC Client Open: ErrorCode={0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "IPC client open succeeded. ServerTransportAddress:{0}",
            owner_.ipcClient_->ServerTransportAddress);

        RegisterWithFabricHost(thisSPtr);
    }

    void RegisterWithFabricHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto request = CreateRegisterClientRequest();

        auto operation = owner_.ipcClient_->BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                OnRegistrationCompleted(operation, false); 
            },
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
        auto error = owner_.ipcClient_->EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                owner_.TraceId,
                "End(RegisterFabricActivatorContainerClient): ErrorCode={0}",
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        RegisterContainerActivatorServiceReply replyBody;
        if (!reply->GetBody<RegisterContainerActivatorServiceReply>(replyBody))
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<RegisterFabricActivatorContainerClientReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (replyBody.Error.IsError(ErrorCodeValue::AlreadyExists) && 
            !timeoutHelper_.IsExpired)
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "Retrying register with FabricActivatorClient: timeout {0}",
                timeoutHelper_.GetRemainingTime());

            RegisterWithFabricHost(operation->Parent);
            return;
        }

        error = replyBody.Error;
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Register FabricActivatorContainerClient completed with error {0}: timeout {1}",
                error,
                timeoutHelper_.GetRemainingTime());

            TryComplete(operation->Parent, error);
            return;
        }

        if (replyBody.IsContainerServicePresent)
        {
            error = owner_.activatorImpl_->StartEventMonitoring(
                replyBody.IsContainerServiceManaged, 
                replyBody.EventSinceTime);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "activatorImpl_->StartEventMonitoring failed: ErrorCode={1}",
                    error);
            }
        }

        TryComplete(operation->Parent, error);
    }

    MessageUPtr CreateRegisterClientRequest()
    {
        RegisterContainerActivatorServiceRequest registerRequest(
            owner_.processId_, 
            owner_.ipcServer_->TransportListenAddress);

        auto request = make_unique<Message>(registerRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::ContainerActivatorServiceClient));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::RegisterContainerActivatorService));

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "RegisterContainerActivatorService: Message={0}, Body={1}",
            *request,
            registerRequest);

        return move(request);
    }

private:
    TimeoutHelper timeoutHelper_;
    ContainerActivatorServiceAgent & owner_;
};

class ContainerActivatorServiceAgent::ActivateContainerAsyncOperation : public AsyncOperation
{
    DENY_COPY(ActivateContainerAsyncOperation)

public:
    ActivateContainerAsyncOperation(
        __in ContainerActivatorServiceAgent & owner,
        __in IpcReceiverContextUPtr & context,
        ContainerActivationArgs const & activationArgs,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , context_(move(context))
        , activationArgs_(move(activationArgs))
        , timeoutHelper_(timeout)
    {
    }

    virtual ~ActivateContainerAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateContainerAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ActivateContainer(thisSPtr);
    }

    void ActivateContainer(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.activatorImpl_->BeginActivateContainer(
            activationArgs_,
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

        auto error = owner_.activatorImpl_->EndActivateContainer(operation, containerId_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "FinishActivateContainer: Error={0}, ErrorMessage={1}.",
            error,
            error.Message);

        SendActivateProcessReplyAndComplete(error, operation->Parent);
    }

    void SendActivateProcessReplyAndComplete(ErrorCode error, AsyncOperationSPtr const & thisSPtr)
    {
        auto replyBody = make_unique<ActivateContainerReply>(error, error.Message, containerId_);
        auto reply = make_unique<Message>(*replyBody);

        context_->Reply(move(reply));

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Sent ActivateContainerReply: for AppHostId={0}, ContainerName={1}, Error={2}, ErrorMessage={3}.",
            activationArgs_.AppHostId,
            activationArgs_.ContainerDescriptionObj.ContainerName,
            error,
            error.Message);

        TryComplete(thisSPtr, error);
    }

private:
    ContainerActivatorServiceAgent & owner_;
    IpcReceiverContextUPtr context_;
    ContainerActivationArgs activationArgs_;
    TimeoutHelper timeoutHelper_;
    wstring containerId_;
};

class ContainerActivatorServiceAgent::DeactivateContainerAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeactivateContainerAsyncOperation)

public:
    DeactivateContainerAsyncOperation(
        __in ContainerActivatorServiceAgent & owner,
        __in IpcReceiverContextUPtr & context,
        ContainerDeactivationArgs const & deactivationArgs,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , context_(move(context))
        , deactivationArgs_(move(deactivationArgs))
        , timeoutHelper_(timeout)
    {
    }

    virtual ~DeactivateContainerAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateContainerAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        DeactivateContainer(thisSPtr);
    }

    void DeactivateContainer(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.activatorImpl_->BeginDeactivateContainer(
            deactivationArgs_,
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

        auto error = owner_.activatorImpl_->EndDeactivateContainer(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "FinishDeactivateContainer: Error={0}, Message={1}.",
            error,
            error.Message);

        SendDeactivateProcessReplyAndComplete(error, operation->Parent);
    }

    void SendDeactivateProcessReplyAndComplete(ErrorCode error, AsyncOperationSPtr const & thisSPtr)
    {
        auto replyBody = make_unique<ContainerOperationReply>(error, error.Message);
        auto reply = make_unique<Message>(*replyBody);

        context_->Reply(move(reply));

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Sent DeactivateContainerReply: for ContainerName={0}, isGraceful={1}, Error={2}, Message={3}.",
            deactivationArgs_.ContainerName,
            deactivationArgs_.IsGracefulDeactivation,
            error,
            error.Message);

        TryComplete(thisSPtr, error);
    }

private:
    ContainerActivatorServiceAgent & owner_;
    IpcReceiverContextUPtr context_;
    ContainerDeactivationArgs deactivationArgs_;
    TimeoutHelper timeoutHelper_;
};

class ContainerActivatorServiceAgent::DownloadContainerImagesAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadContainerImagesAsyncOperation)

public:
    DownloadContainerImagesAsyncOperation(
        __in ContainerActivatorServiceAgent & owner,
        __in IpcReceiverContextUPtr & context,
        vector<ContainerImageDescription> const & images,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , images_(move(images))
        , context_(move(context))
        , owner_(owner)
        , timeoutHelper_(timeout)
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
        DownloadImages(thisSPtr);
    }

    void DownloadImages(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.activatorImpl_->BeginDownloadImages(
            images_,
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

        auto error = owner_.activatorImpl_->EndDownloadImages(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "FinishDownloadImages: Error={0}, Message={1}.",
            error,
            error.Message);

        SendDownloadImagesReplyAndComplete(error, operation->Parent);
    }

    void SendDownloadImagesReplyAndComplete(ErrorCode error, AsyncOperationSPtr const & thisSPtr)
    {
        auto replyBody = make_unique<ContainerOperationReply>(error, error.Message);
        auto reply = make_unique<Message>(*replyBody);

        context_->Reply(move(reply));

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Sent DownloadImagesReply: Error={0}, Message={1}.",
            error,
            error.Message);

        TryComplete(thisSPtr, error);
    }

private:
    ContainerActivatorServiceAgent & owner_;
    IpcReceiverContextUPtr context_;
    TimeoutHelper timeoutHelper_;
    vector<ContainerImageDescription> images_;
};

class ContainerActivatorServiceAgent::DeleteContainerImagesAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeleteContainerImagesAsyncOperation)

public:
    DeleteContainerImagesAsyncOperation(
        __in ContainerActivatorServiceAgent & owner,
        __in IpcReceiverContextUPtr & context,
        vector<wstring> const & images,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , context_(move(context))
        , images_(move(images))
        , timeoutHelper_(timeout)
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
        DeleteImages(thisSPtr);
    }

    void DeleteImages(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.activatorImpl_->BeginDeleteImages(
            images_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDeletingImages(operation, false);
            },
            thisSPtr);

        FinishDeletingImages(operation, true);
    }

    void FinishDeletingImages(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.activatorImpl_->EndDeleteImages(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "FinishDownloadImages: Error={0}, Message={1}.",
            error,
            error.Message);

        SendDeleteImagesReplyAndComplete(error, operation->Parent);
    }

    void SendDeleteImagesReplyAndComplete(ErrorCode error, AsyncOperationSPtr const & thisSPtr)
    {
        auto replyBody = make_unique<ContainerOperationReply>(error, error.Message);
        auto reply = make_unique<Message>(*replyBody);

        context_->Reply(move(reply));

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Sent DeleteImagesReply: Error={0}, Message={1}.",
            error,
            error.Message);

        TryComplete(thisSPtr, error);
    }

private:
    ContainerActivatorServiceAgent & owner_;
    IpcReceiverContextUPtr context_;
    TimeoutHelper timeoutHelper_;
    vector<wstring> images_;
};

class ContainerActivatorServiceAgent::ContainerUpdateRoutesAsyncOperation : public AsyncOperation
{
    DENY_COPY(ContainerUpdateRoutesAsyncOperation)

public:
    ContainerUpdateRoutesAsyncOperation(
        __in ContainerActivatorServiceAgent & owner,
        __in IpcReceiverContextUPtr & context,
        ContainerUpdateRoutesRequest const & updateRoutesRequest,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , updateRoutesRequest_(move(updateRoutesRequest))
        , context_(move(context))
        , owner_(owner)
        , timeoutHelper_(timeout)
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
        auto operation = owner_.activatorImpl_->BeginContainerUpdateRoutes(
            updateRoutesRequest_,
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

        auto error = owner_.activatorImpl_->EndContainerUpdateRoutes(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "FinishContainerUpdateRoutes: Error={0}, Message={1}.",
            error,
            error.Message);

        SendContainerUpdateRoutesReplyAndComplete(error, operation->Parent);
    }

    void SendContainerUpdateRoutesReplyAndComplete(ErrorCode error, AsyncOperationSPtr const & thisSPtr)
    {
        auto replyBody = make_unique<ContainerOperationReply>(error, error.Message);
        auto reply = make_unique<Message>(*replyBody);

        context_->Reply(move(reply));

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Sent ContainerUpdateRoutesReply: Error={0}, Message={1}.",
            error,
            error.Message);

        TryComplete(thisSPtr, error);
    }

private:
    ContainerActivatorServiceAgent & owner_;
    IpcReceiverContextUPtr context_;
    TimeoutHelper timeoutHelper_;
    ContainerUpdateRoutesRequest updateRoutesRequest_;
};

class ContainerActivatorServiceAgent::InvokeContainerApiAsyncOperation : public AsyncOperation
{
    DENY_COPY(InvokeContainerApiAsyncOperation)

public:
    InvokeContainerApiAsyncOperation(
        __in ContainerActivatorServiceAgent & owner,
        __in IpcReceiverContextUPtr & context,
        ContainerApiExecutionArgs const & apiExecArgs,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , context_(move(context))
        , apiExecArgs_(move(apiExecArgs))
        , timeoutHelper_(timeout)
    {
    }

    virtual ~InvokeContainerApiAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<InvokeContainerApiAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        InvokeContainerApi(thisSPtr);
    }

    void InvokeContainerApi(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.activatorImpl_->BeginInvokeContainerApi(
            apiExecArgs_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishInvokeContainerApi(operation, false);
            },
            thisSPtr);

        FinishInvokeContainerApi(operation, true);
    }

    void FinishInvokeContainerApi(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.activatorImpl_->EndInvokeContainerApi(operation, apiExecResonse_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "FinishActivateContainer: Error={0}, Message={1}.",
            error,
            error.Message);

        SendInvokeContainerApiReplyAndComplete(error, operation->Parent);
    }

    void SendInvokeContainerApiReplyAndComplete(ErrorCode error, AsyncOperationSPtr const & thisSPtr)
    {
        auto replyBody = make_unique<InvokeContainerApiReply>(error, error.Message, apiExecResonse_);
        auto reply = make_unique<Message>(*replyBody);

        context_->Reply(move(reply));

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Sent InvokeContainerApiReply: ContainerName={0}, Error={1}, Message={2}.",
            apiExecArgs_.ContainerName,
            error,
            error.Message);

        TryComplete(thisSPtr, error);
    }

private:
    ContainerActivatorServiceAgent & owner_;
    IpcReceiverContextUPtr context_;
    ContainerApiExecutionArgs apiExecArgs_;
    TimeoutHelper timeoutHelper_;
    ContainerApiExecutionResponse apiExecResonse_;
};

class ContainerActivatorServiceAgent::NotifyContainerEventAsyncOperation :
    public AsyncOperation
{
public:
    NotifyContainerEventAsyncOperation(
        ContainerActivatorServiceAgent & owner,
        ContainerEventNotification && notification,
        AsyncCallback const & callback,
        AsyncOperationSPtr parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , notification_(move(notification))
    {
    }

    static ErrorCode NotifyContainerEventAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<NotifyContainerEventAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SendEventNotification(thisSPtr);
    }

    void SendEventNotification(AsyncOperationSPtr const & thisSPtr)
    {
        auto request = CreateEventNotificationRequestMessage();

        auto operation = owner_.ipcClient_->BeginRequest(
            move(request),
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const & operation)
            {
                FinishSendServiceEventMessage(operation, false);
            },
            thisSPtr);

        FinishSendServiceEventMessage(operation, true);
    }

    void FinishSendServiceEventMessage(
        AsyncOperationSPtr operation,
        bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }
        
        MessageUPtr reply;
        auto error = owner_.ipcClient_->EndRequest(operation, reply);
        if (!error.IsSuccess() && 
            error.IsError(ErrorCodeValue::Timeout))
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "Sending notification for container event timed out. Retrying...");

            SendEventNotification(operation->Parent);
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }

    MessageUPtr CreateEventNotificationRequestMessage()
    {
        auto request = make_unique<Message>(notification_);
        request->Headers.Add(Transport::ActorHeader(Actor::ContainerActivatorServiceClient));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ContainerEventNotificationRequest));

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "ContainerEventNotification: Message={0}, Body={1}.",
            *request,
            notification_);

        return move(request);
    }

private:
    ContainerActivatorServiceAgent & owner_;
    ContainerEventNotification notification_;
};

ContainerActivatorServiceAgent::~ContainerActivatorServiceAgent()
{
}

ErrorCode ContainerActivatorServiceAgent::Create(
    __out ContainerActivatorServiceAgentSPtr & result)
{
    auto activationManagerSPtr = make_shared<ContainerActivatorServiceAgent>();

    auto error = activationManagerSPtr->Initialize();
    if (error.IsSuccess())
    {
        result.swap(activationManagerSPtr);
    }

    return error;
}

void ContainerActivatorServiceAgent::Release()
{
    if (ipcServer_ != nullptr)
    {
        this->UnregisterIpcRequestHandler();
        ipcServer_->Close();
    }

    if (ipcClient_ != nullptr)
    {
        ipcClient_->Close();
    }
}

void ContainerActivatorServiceAgent::ProcessContainerEvents(
    ContainerEventNotification && notification)
{
    this->SendContainerEventNotification(move(notification));
}

ErrorCode ContainerActivatorServiceAgent::RegisterContainerActivatorService(
    IContainerActivatorServicePtr const & activatorService)
{
    activatorImpl_ = activatorService;

    auto waiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<RegisterAsyncOperation>(
        *this,
        ContainerActivatorServiceConfig::GetConfig().StartTimeout,
        [waiter, this](AsyncOperationSPtr const& operation)
        {
            UNREFERENCED_PARAMETER(operation);
            waiter->Set();
        },
        this->CreateAsyncOperationRoot());

    waiter->WaitOne();

    auto error = RegisterAsyncOperation::End(operation);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        this->TraceId,
        "ContainerActivatorServiceAgent::RegisterContainerActivatorService() finished with {0}.",
        error);

    return error;
}

ContainerActivatorServiceAgent::ContainerActivatorServiceAgent()
    : ipcClient_()
    , ipcServer_()
    , processId_()
    , activatorImpl_()
{
}

ErrorCode ContainerActivatorServiceAgent::Initialize()
{
    wstring activatorAddress;
    auto res = Environment::GetEnvironmentVariable(Constants::FabricActivatorAddressEnvVariable, activatorAddress, NOTHROW());
    if (!res)
    {
        WriteError(
            TraceType,
            this->TraceId,
            "FabricActivatorAddress not found in environment.");

        return ErrorCode(ErrorCodeValue::InvalidOperation);
    }

    WriteInfo(
        TraceType,
        this->TraceId,
        "FabricHost address retrieved {0}",
        activatorAddress);

    auto ipcServer = make_unique<IpcServer>(
        *this,
        ContainerActivatorServiceConfig::GetConfig().ContainerActivatorServiceAddress,
        ContainerActivatorServiceConfig::GetConfig().ContainerActivatorServiceId,
        false /* disallow use of unreliable transport */,
        L"ContainerActivatorService");

    SecuritySettings ipcServerSecuritySettings;
    auto error = SecuritySettings::CreateNegotiateServer(
        FabricConstants::WindowsFabricAdministratorsGroupName,
        ipcServerSecuritySettings);

    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            "ipcServer->SecuritySettings.CreateNegotiateServer error={0}",
            error);
        return error;
    }

    error = ipcServer->SetSecurity(ipcServerSecuritySettings);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            "ipcServer->SetSecurity error={0}",
            error);
        return error;
    }

    auto ipcClient = make_unique<IpcClient>(
        *this,
        Guid::NewGuid().ToString(),
        activatorAddress,
        false /* disallow use of unreliable transport */,
        L"ContainerActivatorServiceClient");

    SecuritySettings ipcClientSecuritySettings;
    error = SecuritySettings::CreateNegotiateClient(
        SecurityConfig::GetConfig().FabricHostSpn,
        ipcClientSecuritySettings);

    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            this->TraceId,
            "SecuritySettings::CreateNegotiateClient(): ErrorCode={0}.",
            error);

        return error;
    }

    ipcClient->SecuritySettings = ipcClientSecuritySettings;

    ipcServer_ = move(ipcServer);
    ipcClient_ = move(ipcClient);

    this->processId_ = ::GetCurrentProcessId();

    return error;
}

void ContainerActivatorServiceAgent::RegisterIpcRequestHandler()
{
    auto root = this->CreateComponentRoot();

    ipcServer_->RegisterMessageHandler(
        Actor::ContainerActivatorService,
        [this, root](MessageUPtr & message, IpcReceiverContextUPtr & context)
        {
            this->ProcessIpcMessage(*message, context);
        },
        false/*dispatchOnTransportThread*/);
}

void ContainerActivatorServiceAgent::UnregisterIpcRequestHandler()
{
    if (ipcServer_ != nullptr)
    {
        ipcServer_->UnregisterMessageHandler(Actor::ContainerActivatorService);
    }
}

void ContainerActivatorServiceAgent::ProcessIpcMessage(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    auto const & action = message.Action;

    WriteInfo(
        TraceType,
        TraceId,
        "Processing Ipc message with action {0}",
        action);

    if (action == Hosting2::Protocol::Actions::ActivateContainerRequest)
    {
        this->ProcessActivateContainerRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::DeactivateContainerRequest)
    {
        this->ProcessDeactivateContainerRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::DownloadContainerImages)
    {
        this->ProcessDownloadContainerImagesMessage(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ContainerUpdateRoutes)
    {
        this->ProcessContainerUpdateRoutesMessage(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::DeleteContainerImages)
    {
        this->ProcessDeleteContainerImagesMessage(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::InvokeContainerApiRequest)
    {
        this->ProcessInvokeContainerApiMessage(message, context);
    }
    else
    {
        WriteWarning(
            TraceType,
            TraceId,
            "Dropping unsupported message: {0}",
            message);
    }
}

void ContainerActivatorServiceAgent::ProcessActivateContainerRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    ActivateContainerRequest request;

    if (!message.GetBody<ActivateContainerRequest>(request))
    {
        WriteWarning(
            TraceType,
            TraceId,
            "Invalid message received: {0}, dropping",
            message);

        return;
    }

    WriteNoise(
        TraceType,
        TraceId,
        "Processing activate container request for NodeId={0}, ApplicationId={1}, AppHostId={2}, ContainerName={3}.",
        request.ActivationArgs.NodeId,
        request.ActivationArgs.ContainerDescriptionObj.ApplicationId,
        request.ActivationArgs.AppHostId,
        request.ActivationArgs.ContainerDescriptionObj.ContainerName);

    auto operation = AsyncOperation::CreateAndStart<ActivateContainerAsyncOperation>(
        *this,
        context,
        request.ActivationArgs,
        TimeSpan::FromTicks(request.TimeoutInTicks),
        [this](AsyncOperationSPtr const & operation)
        {
            ActivateContainerAsyncOperation::End(operation);
        },
        this->CreateAsyncOperationRoot());
}

void ContainerActivatorServiceAgent::ProcessDeactivateContainerRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    DeactivateContainerRequest request;

    if (!message.GetBody<DeactivateContainerRequest>(request))
    {
        WriteWarning(
            TraceType,
            TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    WriteNoise(
        TraceType,
        TraceId,
        "Processing deactivate container request for ContainerName={0}, IsGraceful={1}.",
        request.DeactivationArgs.ContainerName,
        request.DeactivationArgs.IsGracefulDeactivation);

    auto operation = AsyncOperation::CreateAndStart<DeactivateContainerAsyncOperation>(
        *this,
        context,
        request.DeactivationArgs,
        TimeSpan::FromTicks(request.TimeoutInTicks),
        [this](AsyncOperationSPtr const & operation)
        {
            DeactivateContainerAsyncOperation::End(operation);
        },
        this->CreateAsyncOperationRoot());
}

void ContainerActivatorServiceAgent::ProcessDownloadContainerImagesMessage(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    DownloadContainerImagesRequest request;
    if (!message.GetBody<DownloadContainerImagesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.GetStatus());

        WriteWarning(
            TraceType,
            TraceId,
            "Invalid message received: {0}, dropping",
            message);

        return;
    }

    auto operation = AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
        *this,
        context,
        request.Images,
        TimeSpan::FromTicks(request.TimeoutInTicks),
        [this](AsyncOperationSPtr const & operation)
        {
            DownloadContainerImagesAsyncOperation::End(operation);
        },
        this->CreateAsyncOperationRoot());
}

void ContainerActivatorServiceAgent::ProcessDeleteContainerImagesMessage(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    DeleteContainerImagesRequest request;
    if (!message.GetBody<DeleteContainerImagesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.GetStatus());

        WriteWarning(
            TraceType,
            TraceId,
            "Invalid message received: {0}, dropping",
            message);

        return;
    }

    auto operation = AsyncOperation::CreateAndStart<DeleteContainerImagesAsyncOperation>(
        *this,
        context,
        request.Images,
        TimeSpan::FromTicks(request.TimeoutInTicks),
        [this](AsyncOperationSPtr const & operation)
        {
            DeleteContainerImagesAsyncOperation::End(operation);
        },
        this->CreateAsyncOperationRoot());
}

void ContainerActivatorServiceAgent::ProcessContainerUpdateRoutesMessage(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    ContainerUpdateRoutesRequest request;
    if (!message.GetBody<ContainerUpdateRoutesRequest>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.GetStatus());

        WriteWarning(
            TraceType,
            TraceId,
            "Invalid message received: {0}, dropping",
            message);

        return;
    }

    auto operation = AsyncOperation::CreateAndStart<ContainerUpdateRoutesAsyncOperation>(
        *this,
        context,
        request,
        TimeSpan::FromTicks(request.TimeoutInTicks),
        [this](AsyncOperationSPtr const & operation)
    {
        ContainerUpdateRoutesAsyncOperation::End(operation);
    },
        this->CreateAsyncOperationRoot());
}

void ContainerActivatorServiceAgent::ProcessInvokeContainerApiMessage(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    InvokeContainerApiRequest request;

    if (!message.GetBody<InvokeContainerApiRequest>(request))
    {
        WriteWarning(
            TraceType,
            TraceId,
            "Invalid message received: {0}, dropping",
            message);

        return;
    }

    WriteNoise(
        TraceType,
        TraceId,
        "Processing InvokeContainerApi request for ContainerName={0}.",
        request.ApiExecArgs.ContainerName);

    auto operation = AsyncOperation::CreateAndStart<InvokeContainerApiAsyncOperation>(
        *this,
        context,
        request.ApiExecArgs,
        TimeSpan::FromTicks(request.TimeoutInTicks),
        [this](AsyncOperationSPtr const & operation)
        {
            InvokeContainerApiAsyncOperation::End(operation);
        },
        this->CreateAsyncOperationRoot());
}

void ContainerActivatorServiceAgent::SendContainerEventNotification(
    ContainerEventNotification && notification)
{
    auto operation = AsyncOperation::CreateAndStart<NotifyContainerEventAsyncOperation>(
        *this,
        move(notification),
        [this](AsyncOperationSPtr const & operation)
        {
            this->FinishSendContainerEventNotification(operation);
        },
        this->CreateAsyncOperationRoot());
}

void ContainerActivatorServiceAgent::FinishSendContainerEventNotification(
    AsyncOperationSPtr const & operation)
{
    auto error = NotifyContainerEventAsyncOperation::End(operation);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "FinishSendContainerEventNotification: Error {0}", error);
}






















