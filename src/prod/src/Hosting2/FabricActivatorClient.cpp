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
using namespace Reliability;

StringLiteral const TraceType_ActivatorClient("FabricActivatorClient");

class FabricActivatorClient::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation(FabricActivatorClient & owner,
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
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "Opening FabricActivatorClient: {0}",
            timeoutHelper_.GetRemainingTime());

        //cache the current process Id;
        owner_.processId_ = ::GetCurrentProcessId();
        // Step 1: Open IPC Client
        OpenIPC(thisSPtr);
    }

private:
    void OpenIPC(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.ipcClient_)
        {
            SecuritySettings ipcClientSecuritySettings;
            auto error = SecuritySettings::CreateNegotiateClient(SecurityConfig::GetConfig().FabricHostSpn, ipcClientSecuritySettings);
            owner_.Client.SecuritySettings = ipcClientSecuritySettings;

            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivatorClient,
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "IPC Client Open: ErrorCode={0}",
                error);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
            RegisterClientWithActivator(thisSPtr);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

    void RegisterClientWithActivator(AsyncOperationSPtr thisSPtr)
    {
        MessageUPtr request = CreateRegisterClientRequest();
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteInfo(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "Register FabricActivatorClient timeout: {0}",
            timeout);

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation) { OnRegistrationCompleted(operation, false); },
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
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(RegisterFabricActivatorClient): ErrorCode={0}",
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        RegisterFabricActivatorClientReply replyBody;
        if (!reply->GetBody<RegisterFabricActivatorClientReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<RegisterFabricActivatorClient> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        if (replyBody.Error.IsError(ErrorCodeValue::AlreadyExists) &&
            !timeoutHelper_.IsExpired)
        {
            WriteInfo(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "Retrying register fabricActivatorClient: timeout {0}",
                timeoutHelper_.GetRemainingTime());
            RegisterClientWithActivator(operation->Parent);
        }
        else
        {
            error = error = replyBody.Error;
            WriteTrace(
                error.ToLogLevel(),
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "Register fabricActivatorClient completed with error {0}: timeout {1}",
                replyBody.Error,
                timeoutHelper_.GetRemainingTime());
            TryComplete(operation->Parent, replyBody.Error);
        }

    }

    MessageUPtr CreateRegisterClientRequest()
    {
        RegisterFabricActivatorClientRequest registerRequest(owner_.processId_, owner_.nodeId_, owner_.nodeInstanceId_);

        MessageUPtr request = make_unique<Message>(registerRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::RegisterFabricActivatorClient));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "RegisterFabricActivatorClient: Message={0}, Body={1}", *request, registerRequest);

        return move(request);
    }

private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
};

class FabricActivatorClient::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        __in FabricActivatorClient & owner,
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
        if (owner_.ipcClient_)
        {
            UnregisterFabricActivatorClient(thisSPtr);
        }
        else
        {
            WriteNoise(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "FabricActivatorClient: closed");
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
            [this](AsyncOperationSPtr const & operation) { OnUnregisterCompleted(operation, false); },
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(RegisterFabricActivatorClient): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        RegisterFabricActivatorClientReply replyBody;
        if (!reply->GetBody<RegisterFabricActivatorClientReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<RegisterFabricActivatorClient> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        owner_.UnregisterIpcMessageHandler();

        error = owner_.ipcClient_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "FabricActivatorClient: Failed to close ipcclient. Error {0}",
                error);
        }
        TryComplete(operation->Parent, error);
    }

private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
};

class FabricActivatorClient::ActivateProcessAsyncOperation : public AsyncOperation
{
public:
    ActivateProcessAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & applicationId,
        wstring const & appServiceId,
        ProcessDescriptionUPtr const & processDescription,
        wstring const & secUser,
        bool isContainerHost,
        ContainerDescriptionUPtr && containerDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        applicationId_(applicationId),
        appServiceId_(appServiceId),
        processDescription_(processDescription),
        secUser_(secUser),
        isContainerHost_(isContainerHost),
        containerDescription_(move(containerDescription)),
        timeoutHelper_(timeout),
        processId_()
    {
    }

    static ErrorCode ActivateProcessAsyncOperation::End(AsyncOperationSPtr const & operation, __out DWORD & processId)
    {
        auto thisPtr = AsyncOperation::End<ActivateProcessAsyncOperation>(operation);

        if (thisPtr->Error.IsSuccess())
        {
            processId = thisPtr->processId_;
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {

        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending process activation message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateActivateProcessRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnActivateProcessCompleted(operation, false); },
            thisSPtr);
        OnActivateProcessCompleted(operation, true);
    }

private:

    void OnActivateProcessCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ActivateApplicationProcess): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        ActivateProcessReply replyBody;
        if (!reply->GetBody<ActivateProcessReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ActivateProcessReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }
        if (replyBody.Error.IsSuccess())
        {
            this->processId_ = replyBody.ProcessId;
            TryComplete(operation->Parent, replyBody.Error);
        }
        else
        {
            auto err = replyBody.Error;
            if (!replyBody.ErrorMessage.empty())
            {
                auto msg = replyBody.ErrorMessage;
                err = ErrorCode(err.ReadValue(),
                    move(msg));
            }
            else
            {
                err = ErrorCode(err.ReadValue(),
                    wformatString("{0}{1}", StringResource::Get(IDS_HOSTING_ServiceHostActivationFailed), replyBody.Error.ErrorCodeValueToString()));
            }
            TryComplete(operation->Parent, err);
        }
    }

    MessageUPtr CreateActivateProcessRequest()
    {
        ActivateProcessRequest activateRequest(owner_.processId_,
            owner_.nodeId_,
            applicationId_,
            appServiceId_,
            *processDescription_,
            secUser_,
            timeoutHelper_.GetRemainingTime().Ticks,
            owner_.fabricBinFolder_,
            isContainerHost_,
            *containerDescription_);

        MessageUPtr request = make_unique<Message>(activateRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ActivateProcessRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateActivateProcessRequest: Message={0}, Body={1}", *request, activateRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    ProcessDescriptionUPtr const & processDescription_;
    ContainerDescriptionUPtr containerDescription_;
    wstring const secUser_;
    wstring const applicationId_;
    wstring const appServiceId_;
    DWORD processId_;
    bool isContainerHost_;
};

class FabricActivatorClient::ConfigureContainerGroupAsyncOperation : public AsyncOperation
{
public:
    ConfigureContainerGroupAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & servicePackageId,
        wstring const & partitionId,
        wstring const & servicePackageActivationId,
        ServiceModel::NetworkType::Enum networkType,
        wstring const & openNetworkAssignedIp,
        std::map<std::wstring, std::wstring> const & overlayNetworkResources,
        std::vector<std::wstring> const & dnsServers,
        AppInfo appInfo,
        ServiceModel::ServicePackageResourceGovernanceDescription const & spRg,
#if defined(PLATFORM_UNIX)
        ContainerPodDescription const & podDesc,
#endif
        bool cleanup,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        servicePackageId_(servicePackageId),
        partitionId_(partitionId),
        servicePackageActivationId_(servicePackageActivationId),
        networkType_(networkType),
        openNetworkAssignedIp_(openNetworkAssignedIp),
        overlayNetworkResources_(overlayNetworkResources),
        dnsServers_(dnsServers),
        appName_(appInfo.appName_),
        appId_(appInfo.appId_),
        appfolder_(appInfo.appFolder_),
        cleanup_(cleanup),
        timeoutHelper_(timeout),
        containerName_(),
        spRg_(spRg)
#if defined(PLATFORM_UNIX)
        , podDescription_(podDesc)
#endif
    {
    }

    static ErrorCode ConfigureContainerGroupAsyncOperation::End(AsyncOperationSPtr const & operation, __out wstring & containerName)
    {
        auto thisPtr = AsyncOperation::End<ConfigureContainerGroupAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            containerName = move(thisPtr->containerName_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (cleanup_)
        {
            auto operation = owner_.BeginDeactivateProcess(
                servicePackageId_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnContainerTerminationCompleted(operation, false);
            },
                thisSPtr);
            this->OnContainerTerminationCompleted(operation, true);
        }
        else
        {
            WriteNoise(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "FabricActivatorClient: Sending configure container group to FabricActivator service. {0}",
                timeoutHelper_.GetRemainingTime());

            MessageUPtr request = CreateConfigureContainerGroupRequest();
            auto operation = owner_.Client.BeginRequest(
                move(request),
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { OnConfigureGroupCompleted(operation, false); },
                thisSPtr);
            OnConfigureGroupCompleted(operation, true);
        }
    }

private:

    void OnContainerTerminationCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.EndDeactivateProcess(operation);
        if (!error.IsSuccess())
        {
            owner_.AbortProcess(servicePackageId_);
        }
        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }

    void OnConfigureGroupCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureContainerGroup): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ConfigureContainerGroup> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }
        if (replyBody.Error.IsSuccess())
        {
            containerName_ = replyBody.Data;
            TryComplete(operation->Parent, replyBody.Error);
        }
        else
        {
            auto err = replyBody.Error;
            if (!replyBody.ErrorMessage.empty())
            {
                auto msg = replyBody.ErrorMessage;
                err = ErrorCode(err.ReadValue(),
                    move(msg));
            }

            TryComplete(operation->Parent, err);
        }
    }

    MessageUPtr CreateConfigureContainerGroupRequest()
    {
#if !defined(PLATFORM_UNIX)
        SetupContainerGroupRequest setupRequest(
            servicePackageId_,
            appId_,
            appName_,
            appfolder_,
            owner_.nodeId_,
            partitionId_,
            servicePackageActivationId_,
            networkType_,
            openNetworkAssignedIp_,
            overlayNetworkResources_,
            dnsServers_,
            timeoutHelper_.GetRemainingTime().Ticks,
            spRg_);
#else
        SetupContainerGroupRequest setupRequest(
            servicePackageId_,
            appId_,
            appName_,
            appfolder_,
            owner_.nodeId_,
            partitionId_,
            servicePackageActivationId_,
            networkType_,
            openNetworkAssignedIp_,
            overlayNetworkResources_,
            dnsServers_,
            timeoutHelper_.GetRemainingTime().Ticks,
            podDescription_,
            spRg_);
#endif

        MessageUPtr request = make_unique<Message>(setupRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::SetupContainerGroup));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateActivateProcessRequest: Message={0}, Body={1}", *request, setupRequest);

        return move(request);
    }

private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring servicePackageId_;
    wstring appId_;
    wstring appfolder_;
    wstring appName_;
    wstring partitionId_;
    wstring servicePackageActivationId_;
    ServiceModel::NetworkType::Enum networkType_;
    wstring openNetworkAssignedIp_;
    std::map<std::wstring, std::wstring> overlayNetworkResources_;
    std::vector<std::wstring> dnsServers_;
    ServicePackageResourceGovernanceDescription const & spRg_;
#if defined(PLATFORM_UNIX)
    ContainerPodDescription podDescription_;
#endif
    bool cleanup_;
    wstring containerName_;
};

class FabricActivatorClient::DeactivateProcessAsyncOperation : public AsyncOperation
{
public:
    DeactivateProcessAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & appServiceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        appServiceId_(appServiceId),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode DeactivateProcessAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateProcessAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {

        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending process deactivation message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateRequestMessage();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnDeactivateProcessCompleted(operation, false); },
            thisSPtr);
        OnDeactivateProcessCompleted(operation, true);
    }

private:

    void OnDeactivateProcessCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(DeactivateProcess): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        DeactivateProcessReply replyBody;
        if (!reply->GetBody<DeactivateProcessReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<DeactivateProcessReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateRequestMessage()
    {
        DeactivateProcessRequest requestBody(owner_.nodeId_,
            appServiceId_,
            timeoutHelper_.GetRemainingTime().Ticks);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DeactivateProcessRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateDeactivateProcessRequest: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring const appServiceId_;
};

class FabricActivatorClient::ActivateHostedServiceAsyncOperation : public AsyncOperation
{
public:

    ActivateHostedServiceAsyncOperation(
        __in FabricActivatorClient & owner,
        HostedServiceParameters const & params,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        params_(params)
    {
    }

    static ErrorCode ActivateHostedServiceAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateHostedServiceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending hosted service activation message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateActivateHostedServiceRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnActivateHostedServiceCompleted(operation, false); },
            thisSPtr);

        OnActivateHostedServiceCompleted(operation, true);
    }

    void OnActivateHostedServiceCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr message;
        ErrorCode error;

        error = owner_.Client.EndRequest(operation, message);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ActivateHostedService): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        FabricHostOperationReply  reply;
        if (!message->GetBody(reply))
        {
            error = ErrorCodeValue::InvalidMessage;
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *message,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, reply.Error);
    }

private:

    MessageUPtr CreateActivateHostedServiceRequest()
    {
        ActivateHostedServiceRequest request(params_);

        MessageUPtr message = make_unique<Message>(request);
        message->Headers.Add(Transport::ActorHeader(Actor::HostedServiceActivator));
        message->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ActivateHostedServiceRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateActivateHostedServiceRequest: Message={0}, Body={1}", *message, request);

        return move(message);
    }


    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;

    HostedServiceParameters params_;
};

class FabricActivatorClient::DeactivateHostedServiceAsyncOperation : public AsyncOperation
{
public:

    DeactivateHostedServiceAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & serviceName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        serviceName_(serviceName)
    {
    }

    static ErrorCode DeactivateHostedServiceAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateHostedServiceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending hosted service deactivation message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateDeactivateHostedServiceRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnDeactivateHostedServiceCompleted(operation, false); },
            thisSPtr);

        OnDeactivateHostedServiceCompleted(operation, true);
    }

    void OnDeactivateHostedServiceCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr message;
        ErrorCode error;

        error = owner_.Client.EndRequest(operation, message);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(DeactivateHostedService): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        FabricHostOperationReply reply;
        if (!message->GetBody(reply))
        {
            error = ErrorCodeValue::InvalidMessage;
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *message,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, reply.Error);
    }

private:

    MessageUPtr CreateDeactivateHostedServiceRequest()
    {
        DeactivateHostedServiceRequest request(serviceName_);

        MessageUPtr message = make_unique<Message>(request);
        message->Headers.Add(Transport::ActorHeader(Actor::HostedServiceActivator));
        message->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DeactivateHostedServiceRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateDeactivateHostedServiceRequest: Message={0}, Body={1}", *message, request);

        return move(message);
    }

    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;

    wstring serviceName_;
};

class FabricActivatorClient::TerminateProcessAsyncOperation : public AsyncOperation
{
public:
    TerminateProcessAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & appServiceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(HostingConfig::GetConfig().FabricHostCommunicationTimeout),
        requestTimeout_(HostingConfig::GetConfig().RequestTimeout),
        appServiceId_(appServiceId)
    {
    }

    static ErrorCode TerminateProcessAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<TerminateProcessAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SendTerminateRequest(thisSPtr);
    }

private:
    void SendTerminateRequest(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending process termination message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateRequestMessage();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            requestTimeout_,
            [this](AsyncOperationSPtr const & operation) { OnDeactivateProcessCompleted(operation, false); },
            thisSPtr);
        OnDeactivateProcessCompleted(operation, true);
    }
    void OnDeactivateProcessCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::Timeout))
            {
                if (timeoutHelper_.GetRemainingTime() >= requestTimeout_)
                {
                    WriteInfo(
                        TraceType_ActivatorClient,
                        owner_.Root.TraceId,
                        "Fabric timed out while communicating with FabricHost, retrying with timeout {0}",
                        timeoutHelper_.GetRemainingTime());
                    SendTerminateRequest(operation->Parent);
                    return;
                }
                else
                {
                    WriteError(
                        TraceType_ActivatorClient,
                        owner_.Root.TraceId,
                        "Fabric failed to communicate with FabricHost with : ErrorCode={0}",
                        error);
                    ::ExitProcess((uint)ErrorCodeValue::Timeout);
                }
            }
            else
            {
                WriteWarning(
                    TraceType_ActivatorClient,
                    owner_.Root.TraceId,
                    "End(TerminateApplicationProcess): ErrorCode={0}",
                    error);

                TryComplete(operation->Parent, error);
                return;
            }
        }
        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }

    MessageUPtr CreateRequestMessage()
    {
        DeactivateProcessRequest requestBody(owner_.nodeId_,
            appServiceId_,
            timeoutHelper_.GetRemainingTime().Ticks);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::TerminateProcessRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateDeactivateProcessRequest: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }


private:
    FabricActivatorClient & owner_;
    TimeoutHelper timeoutHelper_;
    TimeSpan requestTimeout_;
    wstring const appServiceId_;
};

class FabricActivatorClient::ConfigureSecurityPrincipalsAsyncOperation : public AsyncOperation
{
public:
    ConfigureSecurityPrincipalsAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & applicationId,
        ULONG applicationPackageCounter,
        PrincipalsDescription const & principalsDescription,
        int const allowedUserCreationFailureCount,
        bool const configureForNode,
        bool const updateExisting,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        applicationId_(applicationId),
        applicationPackageCounter_(applicationPackageCounter),
        principalsDescription_(principalsDescription),
        allowedUserCreationFailureCount_(allowedUserCreationFailureCount),
        configureForNode_(configureForNode),
        updateExisting_(updateExisting),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode ConfigureSecurityPrincipalsAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out PrincipalsProviderContextUPtr & principalsContext)
    {
        auto thisPtr = AsyncOperation::End<ConfigureSecurityPrincipalsAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            principalsContext = move(thisPtr->principalsContext_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending configure security principals message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateConfigureSecurityPrincipalsRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnConfigureSecurityPrincipalsCompleted(operation, false); },
            thisSPtr);
        OnConfigureSecurityPrincipalsCompleted(operation, true);
    }

private:

    void OnConfigureSecurityPrincipalsCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureSecurityPrincipals): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        ConfigureSecurityPrincipalReply replyBody;
        if (!reply->GetBody<ConfigureSecurityPrincipalReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ConfigureSecurityPrincipalReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        if (replyBody.Error.IsSuccess())
        {
            this->principalsContext_ = make_unique<PrincipalsProviderContext>();
            this->principalsContext_->AddSecurityPrincipals(replyBody.PrincipalInformation);
        }
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateConfigureSecurityPrincipalsRequest()
    {
        ConfigureSecurityPrincipalRequest configureRequest(
            owner_.nodeId_,
            applicationId_,
            applicationPackageCounter_,
            principalsDescription_,
            allowedUserCreationFailureCount_,
            configureForNode_,
            updateExisting_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ConfigureSecurityPrincipalRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateConfigureSecurityPrincipalsRequest: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    PrincipalsDescription const & principalsDescription_;
    wstring const applicationId_;
    ULONG applicationPackageCounter_;
    PrincipalsProviderContextUPtr principalsContext_;
    int const allowedUserCreationFailureCount_;
    bool const configureForNode_;
    bool const updateExisting_;
};

class FabricActivatorClient::CleanupSecurityPrincipalsAsyncOperation : public AsyncOperation
{
public:
    CleanupSecurityPrincipalsAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & applicationId,
        bool const cleanupForNode,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        applicationId_(applicationId),
        cleanupForNode_(cleanupForNode),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode CleanupSecurityPrincipalsAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CleanupSecurityPrincipalsAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {

        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending configure security principals message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateCleanupSecurityPrincipalsRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnCleanupSecurityPrincipalsCompleted(operation, false); },
            thisSPtr);
        OnCleanupSecurityPrincipalsCompleted(operation, true);
    }

private:

    void OnCleanupSecurityPrincipalsCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureSecurityPrincipals): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ConfigureSecurityPrincipalReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateCleanupSecurityPrincipalsRequest()
    {
        vector<wstring> appIds;
        appIds.push_back(applicationId_);
        CleanupSecurityPrincipalRequest cleanupRequest(
            owner_.nodeId_,
            appIds,
            cleanupForNode_);

        MessageUPtr request = make_unique<Message>(cleanupRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::CleanupSecurityPrincipalRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateCleanupSecurityPrincipalsRequest: Message={0}, Body={1}", *request, cleanupRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring applicationId_;
    bool const cleanupForNode_;
};

class FabricActivatorClient::CleanupApplicationLocalGroupsAsyncOperation : public AsyncOperation
{
public:
    CleanupApplicationLocalGroupsAsyncOperation(
        __in FabricActivatorClient & owner,
        vector<wstring> const & applicationIds,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        applicationIds_(applicationIds),
        deletedAppIds_(),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode CleanupApplicationLocalGroupsAsyncOperation::End(AsyncOperationSPtr const & operation, __out vector<wstring> & deletedAppIds)
    {
        auto thisPtr = AsyncOperation::End<CleanupApplicationLocalGroupsAsyncOperation>(operation);
        deletedAppIds = move(thisPtr->deletedAppIds_);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {

        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending cleanup application local group message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateCleanupSecurityPrincipalsRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnCleanupSecurityPrincipalsCompleted(operation, false); },
            thisSPtr);
        OnCleanupSecurityPrincipalsCompleted(operation, true);
    }

private:

    void OnCleanupSecurityPrincipalsCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureSecurityPrincipals): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        CleanupApplicationPrincipalsReply replyBody;
        if (!reply->GetBody<CleanupApplicationPrincipalsReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ConfigureSecurityPrincipalReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        deletedAppIds_ = move(replyBody.DeletedAppIds);
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateCleanupSecurityPrincipalsRequest()
    {
        CleanupSecurityPrincipalRequest cleanupRequest(
            owner_.nodeId_,
            applicationIds_,
            false);

        MessageUPtr request = make_unique<Message>(cleanupRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::CleanupApplicationLocalGroup));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateApplicationLocalGroupsRequest: Message={0}, Body={1}", *request, cleanupRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    vector<wstring> applicationIds_;
    vector<wstring> deletedAppIds_;
};

#if defined(PLATFORM_UNIX)
class FabricActivatorClient::DeleteApplicationFoldersAsyncOperation : public AsyncOperation
{
public:
    DeleteApplicationFoldersAsyncOperation(
        __in FabricActivatorClient & owner,
        vector<wstring> const & appFolders,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        appFolders_(appFolders),
        deletedAppFolders_(),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode DeleteApplicationFoldersAsyncOperation::End(AsyncOperationSPtr const & operation, __out vector<wstring> & deletedAppFolders)
    {
        auto thisPtr = AsyncOperation::End<DeleteApplicationFoldersAsyncOperation>(operation);
        deletedAppFolders = move(thisPtr->deletedAppFolders_);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {

        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending delete application folder message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateDeleteFoldersRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnDeleteFoldersCompleted(operation, false); },
            thisSPtr);
        OnDeleteFoldersCompleted(operation, true);
    }

private:

    void OnDeleteFoldersCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(DeleteFolders): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        DeleteApplicationFoldersReply replyBody;
        if (!reply->GetBody<DeleteApplicationFoldersReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<DeleteApplicationFoldersReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        deletedAppFolders_ = move(replyBody.DeletedAppFolders);
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateDeleteFoldersRequest()
    {
        DeleteFolderRequest deleteRequest(
            owner_.nodeId_,
            appFolders_);

        MessageUPtr request = make_unique<Message>(deleteRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DeleteApplicationFolder));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "DeleteApplicationFolderRequest: Message={0}, Body={1}", *request, deleteRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    vector<wstring> appFolders_;
    vector<wstring> deletedAppFolders_;
};

#endif

class FabricActivatorClient::ConfigureCrashDumpsAsyncOperation : public AsyncOperation
{
public:
    ConfigureCrashDumpsAsyncOperation(
        __in FabricActivatorClient & owner,
        bool enable,
        wstring const & servicePackageId,
        vector<wstring> const & exeNames,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        enable_(enable),
        servicePackageId_(servicePackageId),
        exeNames_(exeNames),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode ConfigureCrashDumpsAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConfigureCrashDumpsAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending configure crash dumps message to FabricActivator service. Timeout: {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateConfigureCrashDumpRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnConfigureCrashDumpCompleted(operation, false); },
            thisSPtr);
        OnConfigureCrashDumpCompleted(operation, true);
    }

private:

    void OnConfigureCrashDumpCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureCrashDump): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        ConfigureCrashDumpReply replyBody;
        if (!reply->GetBody<ConfigureCrashDumpReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteError(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ConfigureCrashDumpReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateConfigureCrashDumpRequest()
    {
        ConfigureCrashDumpRequest configureRequest(
            enable_,
            owner_.nodeId_,
            servicePackageId_,
            exeNames_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ConfigureCrashDumpRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "ConfigureCrashDumpRequest: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }

private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    bool enable_;
    wstring const & servicePackageId_;
    vector<wstring> const & exeNames_;
};

class FabricActivatorClient::ConfigureEndpointSecurityAsyncOperation : public AsyncOperation
{
public:
    ConfigureEndpointSecurityAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & principalSid,
        UINT port,
        bool isHttps,
        bool cleanupAcls,
        wstring const & prefix,
        wstring const & servicePackageId,
        bool isSharedPort,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        principalSid_(principalSid),
        port_(port),
        isHttps_(isHttps),
        cleanupAcls_(cleanupAcls),
        prefix_(prefix),
        servicePackageId_(servicePackageId),
        isSharedPort_(isSharedPort),
        timeoutHelper_(HostingConfig::GetConfig().RequestTimeout)
    {
    }

    static ErrorCode ConfigureEndpointSecurityAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConfigureEndpointSecurityAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending configure endpoint security message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateConfigureEndpointSecurityRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnConfigureEndpointSecurityCompleted(operation, false); },
            thisSPtr);
        OnConfigureEndpointSecurityCompleted(operation, true);
    }

private:

    void OnConfigureEndpointSecurityCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureEndpointSecurity): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        auto err = replyBody.Error;
        if (!replyBody.ErrorMessage.empty())
        {
            auto msg = replyBody.ErrorMessage;
            err = ErrorCode(err.ReadValue(),
                move(msg));
        }
        TryComplete(operation->Parent, err);
    }

    MessageUPtr CreateConfigureEndpointSecurityRequest()
    {
        ConfigureEndpointSecurityRequest configureRequest(
            principalSid_,
            port_,
            isHttps_,
            prefix_,
            servicePackageId_,
            owner_.nodeId_,
            isSharedPort_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        if (cleanupAcls_)
        {
            request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::CleanupPortSecurity));
        }
        else
        {
            request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::SetupPortSecurity));
        }

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "ConfigureEndpointSecurityRequest: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring const principalSid_;
    UINT port_;
    bool isHttps_;
    bool cleanupAcls_;
    wstring prefix_;
    wstring servicePackageId_;
    bool isSharedPort_;
};

class FabricActivatorClient::ConfigureEndpointCertificateAndFirewallPolicyAsyncOperation : public AsyncOperation
{
public:
    ConfigureEndpointCertificateAndFirewallPolicyAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & servicePackageId,
        vector<EndpointCertificateBinding> endpointBindings,
        bool cleanup,
        bool cleanupFirewallPolicy,
        std::vector<LONG> const & firewallPorts,
        bool retryOnFailure,
        TimeSpan const & timeSpan,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        bindings_(endpointBindings),
        servicePackageId_(servicePackageId),
        cleanup_(cleanup),
        retryOnFailure_(retryOnFailure),
        retryCount_(0),
        timeoutHelper_(timeSpan),
        timer_(),
        cleanupFirewallPolicy_(cleanupFirewallPolicy),
        firewallPorts_(firewallPorts)
    {
    }

    static ErrorCode ConfigureEndpointCertificateAndFirewallPolicyAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConfigureEndpointCertificateAndFirewallPolicyAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending configure endpointBinding and firewall policy cleanup message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateConfigureEndpointBindingAndFirewallPolicyRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnConfigureEndpointBindingAndFirewallPolicyCompleted(operation, false); },
            thisSPtr);
        OnConfigureEndpointBindingAndFirewallPolicyCompleted(operation, true);
    }

private:

    void OnConfigureEndpointBindingAndFirewallPolicyCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureEndpointBindingAndFirewallPolicy): ErrorCode={0}",
                error);

            CleanupTimer();

            if (retryOnFailure_ &&
                owner_.get_FabricComponentState().Value == FabricComponentState::Opened)
            {
                RescheduleRequest(operation->Parent);
            }
            else
            {
                TryComplete(operation->Parent, error);
            }
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        CleanupTimer();
        auto err = replyBody.Error;
        if (!replyBody.ErrorMessage.empty())
        {
            auto msg = replyBody.ErrorMessage;
            err = ErrorCode(err.ReadValue(),
                move(msg));
        }
        TryComplete(operation->Parent, err);
    }

    MessageUPtr CreateConfigureEndpointBindingAndFirewallPolicyRequest()
    {
        ConfigureEndpointBindingAndFirewallPolicyRequest configureRequest(
            owner_.nodeId_,
            servicePackageId_,
            bindings_,
            cleanup_,
            cleanupFirewallPolicy_,
            firewallPorts_,
            owner_.nodeInstanceId_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ConfigureEndpointCertificatesAndFirewallPolicy));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "ConfigureEndpointCertificatesAndFirewallPolicy: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }

    void RescheduleRequest(AsyncOperationSPtr const & thisSPtr)
    {
        retryCount_++;
        if (retryCount_ > HostingConfig::GetConfig().ActivationMaxFailureCount)
        {
            Assert::CodingError("Fabric unable to communicate with FabricHost after {0} attempts", retryCount_);
        }

        int64 retryTicks = (uint64)(FabricHostConfig::GetConfig().ActivationRetryBackoffInterval.Ticks);
        TimeSpan delay = TimeSpan::FromTicks(retryTicks);
        {
            WriteInfo(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "Scheduling configure endpoint binding and firewall policy cleanup for servicepackageId {0}  after {1} ",
                servicePackageId_,
                delay);

            //reset timeout to original timeout in retry case.
            timeoutHelper_.SetRemainingTime(timeoutHelper_.OriginalTimeout);

            AcquireExclusiveLock lock(timerLock_);
            timer_ = Timer::Create(
                "Hosting.ActivatorClient",
                [this, thisSPtr](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->OnStart(thisSPtr);
            });
            timer_->Change(delay);
        }

        return;
    }

    void CleanupTimer()
    {
        AcquireExclusiveLock lock(timerLock_);
        if (timer_ != nullptr)
        {
            timer_->Cancel();
            timer_ = nullptr;
        }
    }

private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring servicePackageId_;
    vector<EndpointCertificateBinding> bindings_;
    bool cleanup_;
    bool cleanupFirewallPolicy_;
    vector<LONG> firewallPorts_;
    bool retryOnFailure_;
    TimerSPtr timer_;
    int retryCount_;
    ExclusiveLock timerLock_;
};

class FabricActivatorClient::ConfigureContainerCertificateExportAsyncOperation : public AsyncOperation
{
public:
    ConfigureContainerCertificateExportAsyncOperation(
        __in FabricActivatorClient & owner,
        std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & certificateRef,
        std::wstring workDirectoryPath,
        TimeSpan const & timeSpan,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        certificateRef_(certificateRef),
        workDirectoryPath_(workDirectoryPath),
        timeoutHelper_(timeSpan),
        certificatePaths_(),
        certificatePasswordPaths_()
    {
    }

    static ErrorCode ConfigureContainerCertificateExportAsyncOperation::End(AsyncOperationSPtr const & operation, __out std::map<std::wstring, std::wstring> & certificatePaths, __out std::map<std::wstring, std::wstring> & certificatePasswordPaths)
    {
        auto thisPtr = AsyncOperation::End<ConfigureContainerCertificateExportAsyncOperation>(operation);
        certificatePaths = move(thisPtr->certificatePaths_);
        certificatePasswordPaths = move(thisPtr->certificatePasswordPaths_);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending configure containercertificate message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateConfigureContainerCertificateExportRequest();

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            OnConfigureContainerCertificateExportCompleted(operation, false);
        },
            thisSPtr);
        OnConfigureContainerCertificateExportCompleted(operation, true);
    }

private:

    void OnConfigureContainerCertificateExportCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureContainerCertificateExport): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        ConfigureContainerCertificateExportReply replyBody;
        if (!reply->GetBody<ConfigureContainerCertificateExportReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ConfigureContainerCertificateExport> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        certificatePaths_ = move(replyBody.CertificatePaths);
        certificatePasswordPaths_ = move(replyBody.CertificatePasswordPaths);
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateConfigureContainerCertificateExportRequest()
    {
        ConfigureContainerCertificateExportRequest configureRequest(
            certificateRef_,
            workDirectoryPath_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ConfigureContainerCertificateExport));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "ConfigureContainerCertificateExport: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }

private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> certificateRef_;
    std::wstring workDirectoryPath_;
    std::map<std::wstring, std::wstring> certificatePaths_;
    std::map<std::wstring, std::wstring> certificatePasswordPaths_;
};

class FabricActivatorClient::CleanupContainerCertificateExportAsyncOperation : public AsyncOperation
{
public:
    CleanupContainerCertificateExportAsyncOperation(
        __in FabricActivatorClient & owner,
        std::map<std::wstring, std::wstring> certificatePaths,
        std::map<std::wstring, std::wstring> certificatePasswordPaths,
        TimeSpan const & timeSpan,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        certificatePaths_(certificatePaths),
        certificatePasswordPaths_(certificatePasswordPaths),
        timeoutHelper_(timeSpan)
    {
    }

    static ErrorCode CleanupContainerCertificateExportAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CleanupContainerCertificateExportAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending cleanup ContainerCertificateExport message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateCleanupContainerCertificateExportRequest();

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            OnCreateCleanupContainerCertificateExportCompleted(operation, false);
        },
            thisSPtr);
        OnCreateCleanupContainerCertificateExportCompleted(operation, true);
    }

private:

    void OnCreateCleanupContainerCertificateExportCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(CleanupContainerCertificateExport): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationRepl> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateCleanupContainerCertificateExportRequest()
    {
        CleanupContainerCertificateExportRequest configureRequest(
            certificatePaths_,
            certificatePasswordPaths_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::CleanupContainerCertificateExport));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CleanupContainerCertificateExport: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }

private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    std::map<std::wstring, std::wstring> certificatePaths_;
    std::map<std::wstring, std::wstring> certificatePasswordPaths_;
};

class FabricActivatorClient::CreateSymbolicLinkAsyncOperation : public AsyncOperation
{
public:
    CreateSymbolicLinkAsyncOperation(
        __in FabricActivatorClient & owner,
        vector<ArrayPair<wstring, wstring>> const & symbolicLinks,
        DWORD flags,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        symbolicLinks_(symbolicLinks),
        flags_(flags),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode CreateSymbolicLinkAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CreateSymbolicLinkAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending createsymboliclink message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = PrepareCreateSymbolicLinkRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnCreateSymbolicLinkCompleted(operation, false); },
            thisSPtr);
        OnCreateSymbolicLinkCompleted(operation, true);
    }

private:

    void OnCreateSymbolicLinkCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(CreateSymbolicLink): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr PrepareCreateSymbolicLinkRequest()
    {
        CreateSymbolicLinkRequest configureRequest(
            symbolicLinks_,
            flags_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::CreateSymbolicLinkRequest));


        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "CreateSymbolicLinkRequest: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    vector<ArrayPair<wstring, wstring>> symbolicLinks_;
    DWORD flags_;
};

class FabricActivatorClient::ConfigureSharedFolderACLsAsyncOperation : public AsyncOperation
{
public:
    ConfigureSharedFolderACLsAsyncOperation(
        __in FabricActivatorClient & owner,
        vector<wstring> const & sharedPaths,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        sharedPaths_(sharedPaths),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode ConfigureSharedFolderACLsAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConfigureSharedFolderACLsAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending configuresharedfolder message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = PrepareRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnRequestCompleted(operation, false); },
            thisSPtr);
        OnRequestCompleted(operation, true);
    }

private:

    void OnRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(configuresharedfolder): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr PrepareRequest()
    {
        ConfigreSharedFolderACLRequest configureRequest(
            sharedPaths_,
            timeoutHelper_.GetRemainingTime().Ticks);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ConfigureSharedFolderACL));


        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "ConfigureSharedFolderRequest: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    vector<wstring> sharedPaths_;
};

class FabricActivatorClient::ConfigureSBMShareAsyncOperation : public AsyncOperation
{
public:
    ConfigureSBMShareAsyncOperation(
        __in FabricActivatorClient & owner,
        vector<std::wstring> const & sidPrincipals,
        DWORD accessMask,
        wstring const & localFullPath,
        wstring const & shareName,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        sidPrincipals_(sidPrincipals),
        accessMask_(accessMask),
        localFullPath_(localFullPath),
        shareName_(shareName),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode ConfigureSBMShareAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConfigureSBMShareAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending ConfigureSMBShare message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = PrepareRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnRequestCompleted(operation, false); },
            thisSPtr);
        OnRequestCompleted(operation, true);
    }

private:

    void OnRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureSMBShare): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ConfigureSMBShare> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr PrepareRequest()
    {
        ConfigureSMBShareRequest configureRequest(
            sidPrincipals_,
            accessMask_,
            localFullPath_,
            shareName_,
            timeoutHelper_.GetRemainingTime().Ticks);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ConfigureSMBShare));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "ConfigureSMBShare: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;

    vector<std::wstring> sidPrincipals_;
    DWORD accessMask_;
    wstring localFullPath_;
    wstring shareName_;
};

class FabricActivatorClient::AssignIpAddressesAsyncOperation : public AsyncOperation
{
public:
    AssignIpAddressesAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & servicePackageId,
        vector<wstring> const & codePackageNames,
        bool cleanup,
        TimeSpan const & timeSpan,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        servicePackageId_(servicePackageId),
        codePackageNames_(codePackageNames),
        cleanup_(cleanup),
        timeoutHelper_(timeSpan)
    {
    }

    static ErrorCode AssignIpAddressesAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out vector<wstring> & ipAssignments)
    {
        auto thisPtr = AsyncOperation::End<AssignIpAddressesAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            ipAssignments = move(thisPtr->assignedIps_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending assign IP address request FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateAssignIpAddressRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnIpAssignmentRequestCompleted(operation, false); },
            thisSPtr);
        OnIpAssignmentRequestCompleted(operation, true);
    }

private:

    void OnIpAssignmentRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(AssignIpAddressRequest): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        AssignIpAddressesReply replyBody;
        if (!reply->GetBody<AssignIpAddressesReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<AssignIpAddressReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        assignedIps_ = replyBody.AssignedIps;
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateAssignIpAddressRequest()
    {
        AssignIpAddressesRequest configureRequest(
            owner_.nodeId_,
            servicePackageId_,
            codePackageNames_,
            cleanup_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::AssignIpAddresses));


        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "AssignIpAddressesRequest: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring servicePackageId_;
    vector<wstring> codePackageNames_;
    vector<wstring> assignedIps_;
    bool cleanup_;
};

class FabricActivatorClient::ManageOverlayNetworkResourcesAsyncOperation : public AsyncOperation
{
public:
    ManageOverlayNetworkResourcesAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & nodeName,
        wstring const & nodeIpAddress,
        wstring const & servicePackageId,
        map<wstring, vector<wstring>> const & codePackageNetworkNames,
        ManageOverlayNetworkAction::Enum const action,
        TimeSpan const & timeSpan,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        nodeName_(nodeName),
        nodeIpAddress_(nodeIpAddress),
        servicePackageId_(servicePackageId),
        codePackageNetworkNames_(codePackageNetworkNames),
        timeoutHelper_(timeSpan),
        action_(action)
    {
    }

    static ErrorCode ManageOverlayNetworkResourcesAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out std::map<wstring, std::map<std::wstring, std::wstring>> & assignedOverlayNetworkResources)
    {
        auto thisPtr = AsyncOperation::End<ManageOverlayNetworkResourcesAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            assignedOverlayNetworkResources = move(thisPtr->assignedOverlayNetworkResources_);
        }
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        wstring actionStr(ManageOverlayNetworkAction::ToString(action_));
        StringUtility::ToLower(actionStr);

        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending {0} overlay network resource request FabricActivator service. {1}",
            actionStr,
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateManageOverlayNetworkResourceRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnOverlayNetworkResourceManagementRequestCompleted(operation, false); },
            thisSPtr);
        OnOverlayNetworkResourceManagementRequestCompleted(operation, true);
    }

private:

    void OnOverlayNetworkResourceManagementRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ManageOverlayNetworkResourcesRequest): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        ManageOverlayNetworkResourcesReply replyBody;
        if (!reply->GetBody<ManageOverlayNetworkResourcesReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<ManageOverlayNetworkResourcesReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        // re-hydrate original data structure
        for (auto const & nr : replyBody.AssignedNetworkResources)
        {
            wstring codePackageName;
            wstring networkName;
            StringUtility::SplitOnce(nr.first, codePackageName, networkName, L",");

            auto cpIter = assignedOverlayNetworkResources_.find(codePackageName);
            if (cpIter == assignedOverlayNetworkResources_.end())
            {
                assignedOverlayNetworkResources_.insert(make_pair(codePackageName, std::map<std::wstring, std::wstring>()));
                cpIter = assignedOverlayNetworkResources_.find(codePackageName);
            }

            auto networkIter = cpIter->second.find(networkName);
            if (networkIter == cpIter->second.end())
            {
                cpIter->second[networkName] = nr.second;
            }
            else
            {
                networkIter->second = nr.second;
            }
        }

        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateManageOverlayNetworkResourceRequest()
    {
        ManageOverlayNetworkResourcesRequest configureRequest(
            owner_.nodeId_,
            nodeName_,
            nodeIpAddress_,
            servicePackageId_,
            codePackageNetworkNames_,
            action_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ManageOverlayNetworkResources));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "Manage overlay network resources request: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }

private:

    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring nodeName_;
    wstring nodeIpAddress_;
    wstring servicePackageId_;
    map<wstring, vector<wstring>> codePackageNetworkNames_;
    std::map<wstring, std::map<std::wstring, std::wstring>> assignedOverlayNetworkResources_;
    ManageOverlayNetworkAction::Enum action_;
};

class FabricActivatorClient::UpdateOverlayNetworkRoutesAsyncOperation : public AsyncOperation
{
public:
    UpdateOverlayNetworkRoutesAsyncOperation(
        __in FabricActivatorClient & owner,
        wstring const & networkName,
        wstring const & nodeIpAddress,
        wstring const & instanceID,
        int64 const & sequenceNumber,
        bool const & isDelta,
        std::vector<Management::NetworkInventoryManager::NetworkAllocationEntrySPtr> const & networkMappingTable,
        TimeSpan const & timeSpan,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        networkName_(networkName),
        nodeIpAddress_(nodeIpAddress),
        instanceID_(instanceID),
        sequenceNumber_(sequenceNumber),
        isDelta_(isDelta),
        networkMappingTable_(networkMappingTable),
        timeoutHelper_(timeSpan)
    {
    }

    static ErrorCode UpdateOverlayNetworkRoutesAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpdateOverlayNetworkRoutesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending update overlay network routes request FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateUpdateOverlayNetworkRoutesRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnUpdateOverlayNetworkRoutesRequestCompleted(operation, false); },
            thisSPtr);

        OnUpdateOverlayNetworkRoutesRequestCompleted(operation, true);
    }

private:

    void OnUpdateOverlayNetworkRoutesRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(UpdateOverlayNetworkRoutesRequest): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        UpdateOverlayNetworkRoutesReply replyBody;
        if (!reply->GetBody<UpdateOverlayNetworkRoutesReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<UpdateOverlayNetworkRoutesReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateUpdateOverlayNetworkRoutesRequest()
    {
        UpdateOverlayNetworkRoutesRequest configureRequest(
            this->networkName_,
            this->nodeIpAddress_,
            this->instanceID_,
            this->sequenceNumber_,
            this->isDelta_,
            this->networkMappingTable_);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::UpdateOverlayNetworkRoutes));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "Update overlay network routes request: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }

private:

    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring networkName_;
    wstring nodeIpAddress_;
    wstring instanceID_;
    int64 sequenceNumber_;
    bool isDelta_;
    std::vector<Management::NetworkInventoryManager::NetworkAllocationEntrySPtr> networkMappingTable_;
};

class FabricActivatorClient::GetNetworkDeployedPackagesAsyncOperation : public AsyncOperation
{
public:
    GetNetworkDeployedPackagesAsyncOperation(
        __in FabricActivatorClient & owner,
        std::vector<std::wstring> const & servicePackageIds,
        std::wstring const & codePackageName,
        std::wstring const & networkName,
        std::wstring const & nodeId,
        std::map<std::wstring, std::wstring> const & codePackageInstanceAppHostMap,
        TimeSpan const & timeSpan,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        servicePackageIds_(servicePackageIds),
        codePackageName_(codePackageName),
        networkName_(networkName),
        nodeId_(nodeId),
        codePackageInstanceAppHostMap_(codePackageInstanceAppHostMap),
        networkReservedCodePackages_(),
        timeoutHelper_(timeSpan)
    {
    }

    static ErrorCode GetNetworkDeployedPackagesAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & networkReservedCodePackages,
        __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & codePackageInstanceIdentifierContainerInfoMap)
    {
        auto thisPtr = AsyncOperation::End<GetNetworkDeployedPackagesAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            networkReservedCodePackages = move(thisPtr->networkReservedCodePackages_);
            codePackageInstanceIdentifierContainerInfoMap = move(thisPtr->codePackageInstanceIdentifierContainerInfoMap_);
        }
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending get network deployed packages request FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateGetNetworkDeployedPackagesRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnGetNetworkDeployedPackagesRequestCompleted(operation, false); },
            thisSPtr);

        OnGetNetworkDeployedPackagesRequestCompleted(operation, true);
    }

private:

    void OnGetNetworkDeployedPackagesRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(GetNetworkDeployedPackagesRequest): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        GetNetworkDeployedPackagesReply replyBody;
        if (!reply->GetBody<GetNetworkDeployedPackagesReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<GetNetworkDeployedPackagesReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        RehydrateNetworkReservedCodePackages(replyBody);

        RehydrateCodePackageInstanceIdentifierContainerInfoMap(replyBody);

        TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
    }

    void RehydrateNetworkReservedCodePackages(GetNetworkDeployedPackagesReply const & replyBody)
    {
        for (auto const & nr : replyBody.NetworkDeployedPackages)
        {
            wstring networkName;
            wstring servicePackageId;
            StringUtility::SplitOnce(nr.first, networkName, servicePackageId, L",");

            auto networkIter = networkReservedCodePackages_.find(networkName);
            if (networkIter == networkReservedCodePackages_.end())
            {
                networkReservedCodePackages_.insert(make_pair(networkName, std::map<std::wstring, std::vector<std::wstring>>()));
                networkIter = networkReservedCodePackages_.find(networkName);
            }

            auto serviceIter = networkIter->second.find(servicePackageId);
            if (serviceIter == networkIter->second.end())
            {
                networkIter->second[servicePackageId] = nr.second;
            }
            else
            {
                serviceIter->second = nr.second;
            }
        }
    }

    void RehydrateCodePackageInstanceIdentifierContainerInfoMap(GetNetworkDeployedPackagesReply const & replyBody)
    {
        for (auto const & cp : replyBody.CodePackageInstanceIdentifierContainerInfoMap)
        {
            wstring codePackageInstanceIdentifier;
            wstring containerId;
            StringUtility::SplitOnce(cp.first, codePackageInstanceIdentifier, containerId, L",");

            auto cpIter = codePackageInstanceIdentifierContainerInfoMap_.find(codePackageInstanceIdentifier);
            if (cpIter == codePackageInstanceIdentifierContainerInfoMap_.end())
            {
                codePackageInstanceIdentifierContainerInfoMap_.insert(make_pair(codePackageInstanceIdentifier, std::map<std::wstring, std::wstring>()));
                cpIter = codePackageInstanceIdentifierContainerInfoMap_.find(codePackageInstanceIdentifier);
            }

            auto containerIter = cpIter->second.find(containerId);
            if (containerIter == cpIter->second.end())
            {
                cpIter->second[containerId] = cp.second;
            }
            else
            {
                containerIter->second = cp.second;
            }
        }
    }

    MessageUPtr CreateGetNetworkDeployedPackagesRequest()
    {
        GetNetworkDeployedPackagesRequest packagesRequest(
            this->servicePackageIds_,
            this->codePackageName_,
            this->networkName_,
            this->nodeId_,
            this->codePackageInstanceAppHostMap_);

        MessageUPtr request = make_unique<Message>(packagesRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::GetNetworkDeployedCodePackages));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "Get network deployed packages request: Message={0}, Body={1}", *request, packagesRequest);

        return move(request);
    }

private:

    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    vector<wstring> servicePackageIds_;
    wstring codePackageName_;
    wstring networkName_;
    wstring nodeId_;
    std::map<std::wstring, std::wstring> codePackageInstanceAppHostMap_;
    std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> networkReservedCodePackages_;
    std::map<std::wstring, std::map<std::wstring, std::wstring>> codePackageInstanceIdentifierContainerInfoMap_;
};

class FabricActivatorClient::GetDeployedNetworksAsyncOperation : public AsyncOperation
{
public:
    GetDeployedNetworksAsyncOperation(
        __in FabricActivatorClient & owner,
        NetworkType::Enum networkType,
        TimeSpan const & timeSpan,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        networkType_(networkType),
        timeoutHelper_(timeSpan)
    {
    }

    static ErrorCode GetDeployedNetworksAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out std::vector<std::wstring> & networkNames)
    {
        auto thisPtr = AsyncOperation::End<GetDeployedNetworksAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            networkNames = move(thisPtr->networkNames_);
        }
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending get deployed networks request FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateGetDeployedNetworksRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnGetDeployedNetworksRequestCompleted(operation, false); },
            thisSPtr);

        OnGetDeployedNetworksRequestCompleted(operation, true);
    }

private:

    void OnGetDeployedNetworksRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(GetDeployedNetworksRequest): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        GetDeployedNetworksReply replyBody;
        if (!reply->GetBody<GetDeployedNetworksReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<GetDeployedNetworksReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        networkNames_ = replyBody.NetworkNames;

        TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
    }

    MessageUPtr CreateGetDeployedNetworksRequest()
    {
        GetDeployedNetworksRequest networksRequest(networkType_);

        MessageUPtr request = make_unique<Message>(networksRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::GetDeployedNetworks));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "Get deployed networks request: Message={0}, Body={1}", *request, networksRequest);

        return move(request);
    }

private:

    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    NetworkType::Enum networkType_;
    std::vector<std::wstring> networkNames_;
};

class FabricActivatorClient::FabricUpgradeAsyncOperation : public AsyncOperation
{
public:
    FabricUpgradeAsyncOperation(
        __in FabricActivatorClient & owner,
        bool const useFabricInstallerService,
        wstring const & programToRun,
        wstring const & arguments,
        wstring const & downloadedFabricPackageLocation,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        useFabricInstallerService_(useFabricInstallerService),
        programToRun_(programToRun),
        arguments_(arguments),
        downloadedFabricPackageLocation_(downloadedFabricPackageLocation),
        timeoutHelper_(HostingConfig::GetConfig().FabricUpgradeTimeout)
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
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending FabricUpgrade message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateFabricUpgradeRequest();
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnFabricUpgradeCompleted(operation, false); },
            thisSPtr);
        OnFabricUpgradeCompleted(operation, true);
    }

private:

    void OnFabricUpgradeCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(FabricUpgrade): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "FabricUpgrade Reply. GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        TryComplete(operation->Parent, replyBody.Error);
    }

    MessageUPtr CreateFabricUpgradeRequest()
    {
        FabricUpgradeRequest fabricUpgradeRequest(
            useFabricInstallerService_,
            programToRun_,
            arguments_,
            downloadedFabricPackageLocation_);

        MessageUPtr request = make_unique<Message>(fabricUpgradeRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::FabricUpgradeRequest));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "FabricUpgradeRequest: Message={0}, Body={1}", *request, fabricUpgradeRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    bool const useFabricInstallerService_;
    wstring const programToRun_;
    wstring const arguments_;
    wstring const downloadedFabricPackageLocation_;
};

class FabricActivatorClient::ConfigureResourceACLsAsyncOperation : public AsyncOperation
{
public:
    ConfigureResourceACLsAsyncOperation(
        __in FabricActivatorClient & owner,
        vector<wstring> const & sids,
        DWORD accessMask,
        vector<CertificateAccessDescription> const & certificateAccessDescriptions,
        vector<wstring> const & applicationFolders,
        ULONG applicationCounter,
        wstring const & nodeId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        sids_(sids),
        accessMask_(accessMask),
        certificateAccessDescription_(certificateAccessDescriptions),
        applicationFolders_(applicationFolders),
        applicationCounter_(applicationCounter),
        nodeId_(nodeId),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode ConfigureResourceACLsAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConfigureResourceACLsAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending configure resource acls message to FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateConfigureResourceACLsRequest();

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnConfigureResourceACLsCompleted(operation, false); },
            thisSPtr);
        OnConfigureResourceACLsCompleted(operation, true);
    }

private:

    void OnConfigureResourceACLsCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(ConfigureResourceACLs): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        auto err = replyBody.Error;
        if (!replyBody.ErrorMessage.empty())
        {
            auto msg = replyBody.ErrorMessage;
            err = ErrorCode(err.ReadValue(),
                move(msg));
        }
        TryComplete(operation->Parent, err);
    }

    MessageUPtr CreateConfigureResourceACLsRequest()
    {
        ConfigureResourceACLsRequest configureRequest(
            certificateAccessDescription_,
            sids_,
            accessMask_,
            applicationFolders_,
            applicationCounter_,
            nodeId_,
            timeoutHelper_.GetRemainingTime().Ticks);

        auto request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::SetupResourceACLs));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "ConfigureResourceACLsRequest: Message={0}, Body={1}", *request, configureRequest);

        return move(request);
    }


private:
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    vector<wstring>  sids_;
    vector<CertificateAccessDescription> const & certificateAccessDescription_;
    vector<wstring> const & applicationFolders_;
    DWORD accessMask_;
    ULONG applicationCounter_;
    wstring nodeId_;
};

class FabricActivatorClient::DownloadContainerImagesAsyncOperation : public AsyncOperation
{
public:
    DownloadContainerImagesAsyncOperation(
        std::vector<ContainerImageDescription> const & containerImages,
        __in FabricActivatorClient & owner,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        images_(containerImages),
        owner_(owner),
        timer_(),
        retryCount_(0),
        timeoutHelper_(timeout)
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
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending download container images FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateDownloadContainerImageRequest();

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnContainerImageDownloadCompleted(operation, false); },
            thisSPtr);
        OnContainerImageDownloadCompleted(operation, true);
    }

private:

    void OnContainerImageDownloadCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(DownloadContainerImages): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        auto err = replyBody.Error;
        if (!err.IsSuccess() &&
            retryCount_ < HostingConfig::GetConfig().ContainerImageDownloadRetryCount)
        {
            RescheduleRequest(operation->Parent);
        }
        else
        {
            if (!replyBody.ErrorMessage.empty())
            {
                auto msg = replyBody.ErrorMessage;
                err = ErrorCode(err.ReadValue(),
                    move(msg));
            }
            CleanupTimer();
            TryComplete(operation->Parent, err);
        }
    }

    void RescheduleRequest(AsyncOperationSPtr const & thisSPtr)
    {
        retryCount_++;

        int64 retryTicks = HostingConfig::GetConfig().ContainerImageDownloadBackoff.Ticks;
        TimeSpan delay = TimeSpan::FromTicks(retryTicks);
        {
            WriteInfo(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "Scheduling downloadContainerImages request retrycount {0}  after {1} ",
                retryCount_,
                delay);

            //reset timeout to original timeout in retry case.
            timeoutHelper_.SetRemainingTime(timeoutHelper_.OriginalTimeout);

            AcquireExclusiveLock lock(timerLock_);
            timer_ = Timer::Create(
                "Hosting.ActivatorClient",
                [this, thisSPtr](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->OnStart(thisSPtr);
            });
            timer_->Change(delay);
        }

        return;
    }

    void CleanupTimer()
    {
        AcquireExclusiveLock lock(timerLock_);
        if (timer_ != nullptr)
        {
            timer_->Cancel();
            timer_ = nullptr;
        }
    }

    MessageUPtr CreateDownloadContainerImageRequest()
    {
        DownloadContainerImagesRequest downloadRequest(
            images_,
            timeoutHelper_.GetRemainingTime().Ticks);

        auto request = make_unique<Message>(downloadRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DownloadContainerImages));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "DownloadContainerImagesRequest: Message={0}, Body={1}", *request, downloadRequest);

        return move(request);
    }


private:
    vector<ContainerImageDescription> images_;
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    bool retryOnFailure_;
    TimerSPtr timer_;
    int retryCount_;
    ExclusiveLock timerLock_;
};

class FabricActivatorClient::DeleteContainerImagesAsyncOperation : public AsyncOperation
{
public:
    DeleteContainerImagesAsyncOperation(
        std::vector<wstring> const & containerImages,
        __in FabricActivatorClient & owner,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        images_(containerImages),
        owner_(owner),
        timeoutHelper_(timeout)
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
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending delete container images FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateDeleteContainerImageRequest();

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnContainerImageDeleteCompleted(operation, false); },
            thisSPtr);
        OnContainerImageDeleteCompleted(operation, true);
    }

private:

    void OnContainerImageDeleteCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(DeleteContainerImages): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        FabricHostOperationReply replyBody;
        if (!reply->GetBody<FabricHostOperationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<FabricHostOperationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        auto err = replyBody.Error;
        WriteTrace(
            err.ToLogLevel(),
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "DeleteContainerImages returned ErrorCode={0}",
            err);

        TryComplete(operation->Parent, err);
    }


    MessageUPtr CreateDeleteContainerImageRequest()
    {
        DeleteContainerImagesRequest downloadRequest(
            images_,
            timeoutHelper_.GetRemainingTime().Ticks);

        auto request = make_unique<Message>(downloadRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DeleteContainerImages));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "DownloadContainerImagesRequest: Message={0}, Body={1}", *request, downloadRequest);

        return move(request);
    }


private:
    vector<wstring> images_;
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
};

class FabricActivatorClient::GetContainerInfoAsyncOperation : public AsyncOperation
{
public:
    GetContainerInfoAsyncOperation(
        std::wstring const & appServiceId,
        bool isServicePackageActivationModeExclusive,
        std::wstring const & containerInfoType,
        std::wstring const & containerInfoArgs,
        __in FabricActivatorClient & owner,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        appServiceId_(appServiceId),
        isServicePackageActivationModeExclusive_(isServicePackageActivationModeExclusive),
        containerInfoType_(containerInfoType),
        containerInfoArgs_(containerInfoArgs),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode GetContainerInfoAsyncOperation::End(AsyncOperationSPtr const & operation, __out wstring & containerInfo)
    {
        auto thisPtr = AsyncOperation::End<GetContainerInfoAsyncOperation>(operation);
        containerInfo = thisPtr->containerInfo_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "FabricActivatorClient: Sending get container info FabricActivator service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateGetContainerInfoRequest();

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnGetContainerInfoCompleted(operation, false); },
            thisSPtr);
        OnGetContainerInfoCompleted(operation, true);
    }

private:

    void OnGetContainerInfoCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(GetContainerInfo): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // process the reply
        GetContainerInfoReply replyBody;
        if (!reply->GetBody<GetContainerInfoReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<GetContainerInfoReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        containerInfo_ = replyBody.ContainerInfo;

        auto err = replyBody.Error;
        WriteTrace(
            err.ToLogLevel(),
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "GetContainerInfo returned ErrorCode={0}",
            err);

        TryComplete(operation->Parent, err);
    }


    MessageUPtr CreateGetContainerInfoRequest()
    {
        GetContainerInfoRequest getContainerInfoRequest(owner_.nodeId_, appServiceId_, isServicePackageActivationModeExclusive_, containerInfoType_, containerInfoArgs_);

        auto request = make_unique<Message>(getContainerInfoRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::GetContainerInfo));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId, "GetContainerInfoRequest: Message={0}, Body={1}", *request, getContainerInfoRequest);

        return move(request);
    }


private:
    wstring appServiceId_;
    bool isServicePackageActivationModeExclusive_;
    wstring containerInfoType_;
    wstring containerInfoArgs_;
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
    wstring containerInfo_;
};

// GetImages AsyncOperation class implementation
class FabricActivatorClient::GetImagesAsyncOperation : public AsyncOperation
{
public:
    GetImagesAsyncOperation(
        __in FabricActivatorClient & owner,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        images_(),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode GetImagesAsyncOperation::End(AsyncOperationSPtr const & operation, __out std::vector<wstring>& images)
    {
        auto thisPtr = AsyncOperation::End<GetImagesAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            images = thisPtr->images_;
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        // Create request for ProcessActivationManager
        auto request = CreateGetImagesRequest();

        // Send request for images
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnGetImagesCompleted(operation, false); },
            thisSPtr);
        OnGetImagesCompleted(operation, true);
    }

private:
    void OnGetImagesCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
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
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "End(GetImages): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        // Read reply
        GetImagesReply replyBody;
        if (!reply->GetBody<GetImagesReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType_ActivatorClient,
                owner_.Root.TraceId,
                "GetBody<GetImagesReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        images_ = replyBody.Images;
        auto err = replyBody.Error;

        WriteTrace(
            err.ToLogLevel(),
            TraceType_ActivatorClient,
            owner_.Root.TraceId,
            "GetImages returned ErrorCode={0}",
            err);

        TryComplete(operation->Parent, err);
    }

    MessageUPtr CreateGetImagesRequest()
    {
        auto request = make_unique<Message>();
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::GetImages));
        return move(request);
    }

private:
    std::vector<wstring> images_;
    FabricActivatorClient & owner_;
    TimeoutHelper timeoutHelper_;
};


class FabricActivatorClient::ConfigureNodeForDnsServiceAsyncOperation : public AsyncOperation
{
public:
    ConfigureNodeForDnsServiceAsyncOperation(
        __in FabricActivatorClient & owner,
        bool isDnsServiceEnabled,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        isDnsServiceEnabled_(isDnsServiceEnabled),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConfigureNodeForDnsServiceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        bool setAsPreferredDns = DNS::DnsServiceConfig::GetConfig().SetAsPreferredDns;

        ConfigureNodeForDnsServiceRequest configureRequest(isDnsServiceEnabled_, setAsPreferredDns);

        MessageUPtr request = make_unique<Message>(configureRequest);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ConfigureNodeForDnsService));

        WriteNoise(TraceType_ActivatorClient, owner_.Root.TraceId,
            "ConfigureNodeForDnsServiceRequest: Message={0}, Body={1}",
            *request, configureRequest);

        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnConfigureNodeForDnsServiceCompleted(operation, false); },
            thisSPtr);
        this->OnConfigureNodeForDnsServiceCompleted(operation, true);
    }

private:
    void OnConfigureNodeForDnsServiceCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType_ActivatorClient, owner_.Root.TraceId, "ConfigureNodeForDnsServiceRequest End: ErrorCode={0}", error);
        }

        TryComplete(operation->Parent, error);
    }

private:
    bool isDnsServiceEnabled_;
    TimeoutHelper timeoutHelper_;
    FabricActivatorClient & owner_;
};

FabricActivatorClient::FabricActivatorClient(
    ComponentRoot const & root,
    _In_ HostingSubsystem & hosting,
    wstring const & nodeId,
    wstring const & fabricBinFolder,
    uint64 nodeInstanceId)
    : RootedObject(root)
    , hosting_(hosting)
    , fabricBinFolder_(fabricBinFolder)
    , nodeId_(nodeId)
    , ipcClient_()
    , processId_()
    , clientId_()
    , processTerminationEvent_()
    , containerHealthStatusChangeEvent_()
    , containerRootDiedEvent_()
    , nodeInstanceId_(nodeInstanceId)
{
    wstring activatorAddress;

    if (Environment::GetEnvironmentVariable(Constants::FabricActivatorAddressEnvVariable, activatorAddress, NOTHROW()))
    {
        WriteInfo(TraceType_ActivatorClient,
            root.TraceId,
            "FabricHost address retrieved {0}",
            activatorAddress);

        clientId_ = Guid::NewGuid().ToString();

        auto ipcClient = make_unique<IpcClient>(this->Root,
            clientId_,
            activatorAddress,
            false /* disallow use of unreliable transport */,
            L"FabricActivatorClient");
        ipcClient_ = move(ipcClient);
    }
    else
    {
        WriteWarning(
            TraceType_ActivatorClient,
            root.TraceId,
            "IPCServer address not found in env. IPC client not initialized");
    }
}

FabricActivatorClient::~FabricActivatorClient()
{
}

Common::AsyncOperationSPtr FabricActivatorClient::OnBeginOpen(
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

Common::ErrorCode FabricActivatorClient::OnEndOpen(Common::AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr FabricActivatorClient::OnBeginClose(
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

Common::ErrorCode FabricActivatorClient::OnEndClose(Common::AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void FabricActivatorClient::OnAbort()
{
    if (this->ipcClient_)
    {
        this->UnregisterFabricActivatorClient();
        this->UnregisterIpcMessageHandler();
        this->ipcClient_->Abort();
    }
}

MessageUPtr FabricActivatorClient::CreateUnregisterClientRequest()
{
    RegisterFabricActivatorClientRequest registerRequest(processId_, nodeId_, nodeInstanceId_);

    MessageUPtr request = make_unique<Message>(registerRequest);
    request->Headers.Add(Transport::ActorHeader(Actor::FabricActivator));
    request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::UnregisterFabricActivatorClient));

    WriteNoise(TraceType_ActivatorClient, Root.TraceId, "RegisterFabricActivatorClient: Message={0}, Body={1}", *request, registerRequest);

    return move(request);
}
void FabricActivatorClient::UnregisterFabricActivatorClient()
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
                TraceType_ActivatorClient,
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
            TraceType_ActivatorClient,
            Root.TraceId,
            "Unregister fabricactivatorclient request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "Unregister fabricactivatorclient request cleanupWaiter failed on Wait");
    }
}
void FabricActivatorClient::RegisterIpcMessageHandler()
{
    auto root = this->Root.CreateComponentRoot();
    ipcClient_->RegisterMessageHandler(
        Actor::FabricActivatorClient,
        [this, root](MessageUPtr & message, IpcReceiverContextUPtr & context)
    {
        this->IpcMessageHandler(*message, context);
    },
        false/*dispatchOnTransportThread*/);
}

void FabricActivatorClient::UnregisterIpcMessageHandler()
{
    ipcClient_->UnregisterMessageHandler(Actor::FabricActivatorClient);
}

HHandler FabricActivatorClient::AddProcessTerminationHandler(EventHandler const & handler)
{
    return processTerminationEvent_.Add(handler);
}

void FabricActivatorClient::RemoveProcessTerminationHandler(HHandler const & handler)
{
    processTerminationEvent_.Remove(handler);
}

HHandler FabricActivatorClient::AddContainerHealthCheckStatusChangeHandler(EventHandler const & handler)
{
    return containerHealthStatusChangeEvent_.Add(handler);
}

void FabricActivatorClient::RemoveContainerHealthCheckStatusChangeHandler(HHandler const & handler)
{
    containerHealthStatusChangeEvent_.Remove(handler);
}

HHandler FabricActivatorClient::AddRootContainerTerminationHandler(EventHandler const & handler)
{
    return containerRootDiedEvent_.Add(handler);
}

void FabricActivatorClient::RemoveRootContainerTerminationHandler(HHandler const & handler)
{
    containerRootDiedEvent_.Remove(handler);
}

void FabricActivatorClient::IpcMessageHandler(__in Message & message, __in IpcReceiverContextUPtr & context)
{
    WriteNoise(TraceType_ActivatorClient, Root.TraceId, "Received message: actor={0}, action={1}", message.Actor, message.Action);

    if (message.Actor == Actor::FabricActivatorClient)
    {
        ProcessIpcMessage(message, context);
    }
    else
    {
        WriteWarning(TraceType_ActivatorClient, Root.TraceId, "Unexpected message: actor={0}, action={1}", message.Actor, message.Action);
    }
}

void FabricActivatorClient::ProcessIpcMessage(__in Message & message, __in IpcReceiverContextUPtr & context)
{
    if (message.Action == Hosting2::Protocol::Actions::ServiceTerminatedNotificationRequest)
    {
        ProcessApplicationServiceTerminatedRequest(message, context);
    }
    else if (message.Action == Hosting2::Protocol::Actions::ContainerHealthCheckStatusChangeNotificationRequest)
    {
        ProcessContainerHealthCheckStatusChangeRequest(message, context);
    }
    else if (message.Action == Hosting2::Protocol::Actions::DockerProcessTerminatedNotificationRequest)
    {
        ProcessDockerProcessTerminatedRequest(message, context);
    }
    else if (message.Action == Hosting2::Protocol::Actions::GetOverlayNetworkDefinition)
    {
        ProcessGetOverlayNetworkDefinitionRequest(message, context);
        return;
    }
    else if (message.Action == Hosting2::Protocol::Actions::DeleteOverlayNetworkDefinition)
    {
        ProcessDeleteOverlayNetworkDefinitionRequest(message, context);
        return;
    }
    else if (message.Action == Hosting2::Protocol::Actions::PublishNetworkTablesRequest)
    {
        ProcessPublishNetworkTablesRequest(message, context);
        return;
    }
}

AsyncOperationSPtr FabricActivatorClient::BeginActivateProcess(
    wstring const & applicationId,
    wstring const & appServiceId,
    ProcessDescriptionUPtr const & processDescription,
    wstring const & runasUserId,
    bool isContainerHost,
    ContainerDescriptionUPtr && containerDescription,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<ActivateProcessAsyncOperation>(
        *this,
        applicationId,
        appServiceId,
        processDescription,
        runasUserId,
        isContainerHost,
        move(containerDescription),
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndActivateProcess(Common::AsyncOperationSPtr const & operation, __out DWORD & processId)
{
    return ActivateProcessAsyncOperation::End(operation, processId);
}

AsyncOperationSPtr FabricActivatorClient::BeginDeactivateProcess(
    wstring const & appServiceId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<DeactivateProcessAsyncOperation>(
        *this,
        appServiceId,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndDeactivateProcess(Common::AsyncOperationSPtr const & operation)
{
    return DeactivateProcessAsyncOperation::End(operation);
}


AsyncOperationSPtr FabricActivatorClient::BeginActivateHostedService(
    HostedServiceParameters const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");

    return AsyncOperation::CreateAndStart<ActivateHostedServiceAsyncOperation>(*this, params, timeout, callback, parent);
}

ErrorCode FabricActivatorClient::EndActivateHostedService(AsyncOperationSPtr const & operation)
{
    return ActivateHostedServiceAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginDeactivateHostedService(
    wstring const & serviceName,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");

    return AsyncOperation::CreateAndStart<DeactivateHostedServiceAsyncOperation>(*this, serviceName, timeout, callback, parent);
}

ErrorCode FabricActivatorClient::EndDeactivateHostedService(AsyncOperationSPtr const & operation)
{
    return DeactivateHostedServiceAsyncOperation::End(operation);
}

void FabricActivatorClient::ProcessApplicationServiceTerminatedRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    ServiceTerminatedNotification notification;

    if (!message.GetBody<ServiceTerminatedNotification>(notification))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }
    for (auto it = notification.ApplicationServiceIds.begin(); it != notification.ApplicationServiceIds.end(); it++)
    {
        if (notification.IsContainerRoot)
        {
            containerRootDiedEvent_.Fire(ApplicationHostTerminatedEventArgs(notification.ActivityDescription, *it, notification.ExitCode));
        }
        else
        {
            processTerminationEvent_.Fire(ApplicationHostTerminatedEventArgs(notification.ActivityDescription, *it, notification.ExitCode));
        }
    }
    //Just return message to ack reception
    MessageUPtr response = make_unique<Message>(notification);
    context->Reply(move(response));
}

void FabricActivatorClient::ProcessContainerHealthCheckStatusChangeRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    ContainerHealthCheckStatusChangeNotification notification;

    if (!message.GetBody<ContainerHealthCheckStatusChangeNotification>(notification))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);

        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);

        return;
    }

    containerHealthStatusChangeEvent_.Fire(ContainerHealthCheckStatusChangedEventArgs(notification.TakeHealthStatusInfoList()));

    //Just return message to ack reception
    MessageUPtr response = make_unique<Message>(notification); // TODO: We should return empty message.
    context->Reply(move(response));
}

void FabricActivatorClient::ProcessDockerProcessTerminatedRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    //Report Health on Node saying docker crashed with ttl of default 1 minute.
    DockerProcessTerminatedNotification notification;

    if (!message.GetBody<DockerProcessTerminatedNotification>(notification))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);

        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    hosting_.HealthManagerObj->ReportHealthWithTTL(
        L"DockerProcessManager",
        SystemHealthReportCode::Hosting_DockerDaemonUnhealthy,
        wformatString("Docker daemon terminated with exit code:{0}, ContinuousFailureCount: {1}, NextStartTime: {2} seconds",
            notification.ExitCode,
            notification.ContinuousFailureCount,
            notification.NextStartTime.TotalSeconds()),
        SequenceNumber::GetNext(),
        HostingConfig::GetConfig().DockerHealthReportTTL);

    //Just return message to ack reception
    MessageUPtr response = make_unique<Message>();
    context->Reply(move(response));
}

void FabricActivatorClient::ProcessGetOverlayNetworkDefinitionRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    Management::NetworkInventoryManager::NetworkAllocationRequestMessage request;
    if (!message.GetBody<Management::NetworkInventoryManager::NetworkAllocationRequestMessage>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    hosting_.NetworkInventoryAgent->BeginSendAllocationRequestMessage(
        request,
        HostingConfig::GetConfig().RequestTimeout,
        [this, ctx = context.release()](AsyncOperationSPtr const & operation)
    {
        Management::NetworkInventoryManager::NetworkAllocationResponseMessage networkAllocationResponse;
        auto error = hosting_.NetworkInventoryAgent->EndSendAllocationRequestMessage(operation, networkAllocationResponse);

        if (!error.IsSuccess())
        {
            networkAllocationResponse.ErrorCode.ReadValue();
            networkAllocationResponse.ErrorCode = error;
        }

        MessageUPtr response = make_unique<Message>(networkAllocationResponse);
        ctx->Reply(move(response));
    });
}

void FabricActivatorClient::ProcessDeleteOverlayNetworkDefinitionRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    Management::NetworkInventoryManager::NetworkRemoveRequestMessage request;
    if (!message.GetBody<Management::NetworkInventoryManager::NetworkRemoveRequestMessage>(request))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    hosting_.NetworkInventoryAgent->BeginSendDeallocationRequestMessage(
        request,
        HostingConfig::GetConfig().RequestTimeout,
        [this, ctx = context.release()](AsyncOperationSPtr const & operation)
    {
        Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage networkRemoveResponse;
        auto error = hosting_.NetworkInventoryAgent->EndSendDeallocationRequestMessage(operation, networkRemoveResponse);

        if (!error.IsSuccess())
        {
            networkRemoveResponse.ErrorCode.ReadValue();
            networkRemoveResponse.ErrorCode = error;
        }

        MessageUPtr response = make_unique<Message>(networkRemoveResponse);
        ctx->Reply(move(response));
    });
}

void FabricActivatorClient::ProcessPublishNetworkTablesRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    Management::NetworkInventoryManager::PublishNetworkTablesMessageRequest publishNetworkTablesRequest;
    if (!message.GetBody<Management::NetworkInventoryManager::PublishNetworkTablesMessageRequest>(publishNetworkTablesRequest))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    hosting_.NetworkInventoryAgent->BeginSendPublishNetworkTablesRequestMessage(
        publishNetworkTablesRequest,
        HostingConfig::GetConfig().RequestTimeout,
        [this, ctx = context.release()](AsyncOperationSPtr const & operation)
    {
        Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage publishNetworkTablesResponse;
        auto error = hosting_.NetworkInventoryAgent->EndSendPublishNetworkTablesRequestMessage(operation, publishNetworkTablesResponse);

        if (!error.IsSuccess())
        {
            publishNetworkTablesResponse.ErrorCode.ReadValue();
            publishNetworkTablesResponse.ErrorCode = error;
        }

        MessageUPtr response = make_unique<Message>(publishNetworkTablesResponse);
        ctx->Reply(move(response));
    });
}

AsyncOperationSPtr FabricActivatorClient::BeginConfigureSecurityPrincipals(
    wstring const & applicationId,
    ULONG applicationPackageCounter,
    PrincipalsDescription const & principalsDescription,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<ConfigureSecurityPrincipalsAsyncOperation>(
        *this,
        applicationId,
        applicationPackageCounter,
        principalsDescription,
        0 /*allowedUserCreationFailureCount*/,
        false /*configureForNode*/,
        false /*updateExisting*/,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndConfigureSecurityPrincipals(
    Common::AsyncOperationSPtr const & operation,
    __out PrincipalsProviderContextUPtr & principalsContext)
{
    return ConfigureSecurityPrincipalsAsyncOperation::End(operation, principalsContext);
}

AsyncOperationSPtr FabricActivatorClient::BeginConfigureSecurityPrincipalsForNode(
    wstring const & applicationId,
    ULONG applicationPackageCounter,
    PrincipalsDescription const & principalsDescription,
    int allowedUserCreationFailureCount,
    bool updateExisting,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    ASSERT_IFNOT(applicationId == ApplicationIdentifier::FabricSystemAppId->ToString(), "BeginConfigureSecurityPrincipalsForNode is currently only supported for system application.");

    return AsyncOperation::CreateAndStart<ConfigureSecurityPrincipalsAsyncOperation>(
        *this,
        applicationId,
        applicationPackageCounter,
        principalsDescription,
        allowedUserCreationFailureCount,
        true /*configureForNode*/,
        updateExisting,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndConfigureSecurityPrincipalsForNode(
    Common::AsyncOperationSPtr const & operation,
    __out PrincipalsProviderContextUPtr & principalsContext)
{
    return ConfigureSecurityPrincipalsAsyncOperation::End(operation, principalsContext);
}

AsyncOperationSPtr FabricActivatorClient::BeginCleanupSecurityPrincipals(
    wstring const & applicationId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");

    return AsyncOperation::CreateAndStart<CleanupSecurityPrincipalsAsyncOperation>(
        *this,
        applicationId,
        false /*cleanupForNode*/,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndCleanupSecurityPrincipals(
    Common::AsyncOperationSPtr const & operation)
{
    return CleanupSecurityPrincipalsAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginCleanupApplicationSecurityGroup(
    vector<wstring> const & applicationIds,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<CleanupApplicationLocalGroupsAsyncOperation>(
        *this,
        applicationIds,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndCleanupApplicationSecurityGroup(
    Common::AsyncOperationSPtr const & operation,
    __out vector<wstring> & deletedAppIds)
{
    return CleanupApplicationLocalGroupsAsyncOperation::End(operation, deletedAppIds);
}

#if defined(PLATFORM_UNIX)
AsyncOperationSPtr FabricActivatorClient::BeginDeleteApplicationFolder(
    vector<wstring> const & appFolders,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<DeleteApplicationFoldersAsyncOperation>(
        *this,
        appFolders,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndDeleteApplicationFolder(
    Common::AsyncOperationSPtr const & operation,
    __out vector<wstring> & deletedAppFolders)
{
    return DeleteApplicationFoldersAsyncOperation::End(operation, deletedAppFolders);
}
#endif

AsyncOperationSPtr FabricActivatorClient::BeginConfigureSMBShare(
    std::vector<std::wstring> const & sidPrincipals,
    DWORD accessMask,
    std::wstring const & localFullPath,
    std::wstring const & shareName,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<ConfigureSBMShareAsyncOperation>(
        *this,
        sidPrincipals,
        accessMask,
        localFullPath,
        shareName,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndConfigureSMBShare(Common::AsyncOperationSPtr const & operation)
{
    return ConfigureSBMShareAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginCreateSymbolicLink(
    vector<ArrayPair<wstring, wstring>> const & symbolicLinks,
    DWORD flags,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<CreateSymbolicLinkAsyncOperation>(
        *this,
        symbolicLinks,
        flags,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndCreateSymbolicLink(
    Common::AsyncOperationSPtr const & operation)
{
    return CreateSymbolicLinkAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginConfigureSharedFolderPermissions(
    vector<wstring> const & sharedFolders,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<ConfigureSharedFolderACLsAsyncOperation>(
        *this,
        sharedFolders,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndConfigureSharedFolderPermissions(
    Common::AsyncOperationSPtr const & operation)
{
    return ConfigureSharedFolderACLsAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginConfigureCrashDumps(
    bool enable,
    wstring const & servicePackageId,
    vector<wstring> const & exeNames,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ConfigureCrashDumpsAsyncOperation>(
        *this,
        enable,
        servicePackageId,
        exeNames,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndConfigureCrashDumps(
    Common::AsyncOperationSPtr const & operation)
{
    return ConfigureCrashDumpsAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginFabricUpgrade(
    bool const useFabricInstallerService,
    wstring const & programToRun,
    wstring const & arguments,
    wstring const & downloadedFabricPackageLocation,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<FabricUpgradeAsyncOperation>(
        *this,
        useFabricInstallerService,
        programToRun,
        arguments,
        downloadedFabricPackageLocation,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndFabricUpgrade(
    AsyncOperationSPtr const & operation)
{
    return FabricUpgradeAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginConfigureEndpointSecurity(
    wstring const & principalSid,
    UINT port,
    bool isHttps,
    bool cleanupAcls,
    wstring const & prefix,
    wstring const & servicePackageId,
    bool isSharedPort,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<ConfigureEndpointSecurityAsyncOperation>(
        *this,
        principalSid,
        port,
        isHttps,
        cleanupAcls,
        prefix,
        servicePackageId,
        isSharedPort,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndConfigureEndpointSecurity(
    Common::AsyncOperationSPtr const & operation)
{
    return ConfigureEndpointSecurityAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginConfigureEndpointCertificateAndFirewallPolicy(
    wstring const & servicePackageId,
    std::vector<EndpointCertificateBinding> const & endpointCertBindings,
    bool cleanupEndpointCert,
    bool cleanupFirewallPolicy,
    std::vector<LONG> const & firewallPorts,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<ConfigureEndpointCertificateAndFirewallPolicyAsyncOperation>(
        *this,
        servicePackageId,
        endpointCertBindings,
        cleanupEndpointCert,
        cleanupFirewallPolicy,
        firewallPorts,
        false /* to retry on failure */,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::ConfigureEndpointBindingAndFirewallPolicy(
    wstring const & servicePackageId,
    vector<EndpointCertificateBinding> const & certBindings,
    bool cleanupEndpointCert,
    bool cleanupFirewallPolicy,
    std::vector<LONG> const & firewallPorts)
{
    AsyncOperationWaiterSPtr operationWaiter = make_shared<AsyncOperationWaiter>();

    TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;

    AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<ConfigureEndpointCertificateAndFirewallPolicyAsyncOperation>(
        *this,
        servicePackageId,
        certBindings,
        cleanupEndpointCert,
        cleanupFirewallPolicy,
        firewallPorts,
        true  /* to retry on failure */,
        timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->EndConfigureEndpointCertificateAndFirewallPolicy(operation).ReadValue();
    },
        this->Root.CreateAsyncOperationRoot());

    return ErrorCodeValue::Success;
}

ErrorCode FabricActivatorClient::EndConfigureEndpointCertificateAndFirewallPolicy(AsyncOperationSPtr const & operation)
{
    auto error = ConfigureEndpointCertificateAndFirewallPolicyAsyncOperation::End(operation);
    WriteTrace(error.ToLogLevel(),
        TraceType_ActivatorClient,
        Root.TraceId,
        "ConfigureEndpointCertificateAndFirewallPolicy cleanup error {0}",
        error);

    return error;
}

AsyncOperationSPtr FabricActivatorClient::BeginConfigureResourceACLs(
    vector<wstring> const & sids,
    DWORD accessMask,
    vector<CertificateAccessDescription> const & certificateAccessDescriptions,
    vector<wstring> const & applicationFolders,
    ULONG applicationCounter,
    wstring const & nodeId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<ConfigureResourceACLsAsyncOperation>(
        *this,
        sids,
        accessMask,
        certificateAccessDescriptions,
        applicationFolders,
        applicationCounter,
        nodeId,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndConfigureResourceACLs(
    Common::AsyncOperationSPtr const & operation)
{
    return ConfigureResourceACLsAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginSetupContainerGroup(
    std::wstring const & servicePackageId,
    ServiceModel::NetworkType::Enum networkType,
    std::wstring const & openNetworkAssignedIp,
    std::map<std::wstring, std::wstring> const & overlayNetworkResources,
    std::vector<std::wstring> const & dnsServers,
    std::wstring const & appfolder,
    std::wstring const & appId,
    std::wstring const & appName,
    std::wstring const & partitionId,
    std::wstring const & servicePackageActivationId,
    ServiceModel::ServicePackageResourceGovernanceDescription const & spRg,
#if defined(PLATFORM_UNIX)
    ContainerPodDescription const & podDescription,
#endif
    bool isCleanup,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    AppInfo appInfo(appfolder, appId, appName);
    return AsyncOperation::CreateAndStart<ConfigureContainerGroupAsyncOperation>(
        *this,
        servicePackageId,
        partitionId,
        servicePackageActivationId,
        networkType,
        openNetworkAssignedIp,
        overlayNetworkResources,
        dnsServers,
        appInfo,
        spRg,
#if defined(PLATFORM_UNIX)
        podDescription,
#endif
        isCleanup,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndSetupContainerGroup(AsyncOperationSPtr const & operation, wstring & containerName)
{
    return ConfigureContainerGroupAsyncOperation::End(operation, containerName);
}

AsyncOperationSPtr FabricActivatorClient::BeginAssignIpAddresses(
    wstring const & servicePackageInstanceId,
    vector<wstring> const & codePackages,
    bool cleanup,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<AssignIpAddressesAsyncOperation>(
        *this,
        servicePackageInstanceId,
        codePackages,
        cleanup,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndAssignIpAddresses(
    AsyncOperationSPtr const & operation,
    __out vector<wstring> & assignedIps)
{
    return AssignIpAddressesAsyncOperation::End(operation, assignedIps);
}

AsyncOperationSPtr FabricActivatorClient::BeginManageOverlayNetworkResources(
    wstring const & nodeName,
    wstring const & nodeIpAddress,
    wstring const & servicePackageInstanceId,
    map<wstring, vector<wstring>> const & codePackageNetworkNames,
    ManageOverlayNetworkAction::Enum const action,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ManageOverlayNetworkResourcesAsyncOperation>(
        *this,
        nodeName,
        nodeIpAddress,
        servicePackageInstanceId,
        codePackageNetworkNames,
        action,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndManageOverlayNetworkResources(
    AsyncOperationSPtr const & operation,
    __out std::map<wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources)
{
    return ManageOverlayNetworkResourcesAsyncOperation::End(operation, assignedNetworkResources);
}

Common::AsyncOperationSPtr FabricActivatorClient::BeginUpdateRoutes(
    Management::NetworkInventoryManager::PublishNetworkTablesRequestMessage const & networkTables,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateOverlayNetworkRoutesAsyncOperation>(
        *this,
        networkTables.NetworkName,
        this->hosting_.FabricNodeConfigObj.IPAddressOrFQDN,
        networkTables.InstanceID,
        networkTables.SequenceNumber,
        networkTables.IsDelta,
        networkTables.NetworkMappingTable,
        timeout,
        callback,
        parent);
}

Common::ErrorCode FabricActivatorClient::EndUpdateRoutes(Common::AsyncOperationSPtr const & operation)
{
    return UpdateOverlayNetworkRoutesAsyncOperation::End(operation);
}

ErrorCode FabricActivatorClient::ConfigureEndpointSecurity(
    wstring const & principalSid,
    UINT port,
    bool isHttps,
    bool cleanupAcls,
    wstring const & servicePackageId,
    bool isSharedPort)
{
    AsyncOperationWaiterSPtr operationWaiter = make_shared<AsyncOperationWaiter>();

    TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;

    auto operation = AsyncOperation::CreateAndStart<ConfigureEndpointSecurityAsyncOperation>(
        *this,
        principalSid,
        port,
        isHttps,
        cleanupAcls,
        wstring(),
        servicePackageId,
        isSharedPort,
        [operationWaiter](AsyncOperationSPtr const & operation)
    {
        auto error = ConfigureEndpointSecurityAsyncOperation::End(operation);
        operationWaiter->SetError(error);
        operationWaiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    ErrorCode error;
    if (operationWaiter->WaitOne(timeout))
    {
        error = operationWaiter->GetError();
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivatorClient,
            Root.TraceId,
            "ConfigureEndpointSecurity request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "ConfigureEndpointSecurity operationWaiter failed on Wait");
        error = ErrorCode(ErrorCodeValue::Timeout);
    }
    return error;
}

Common::ErrorCode FabricActivatorClient::CleanupAssignedIPs(wstring const & servicePackageId)
{
    AsyncOperationWaiterSPtr operationWaiter = make_shared<AsyncOperationWaiter>();

    TimeSpan timeout = HostingConfig::GetConfig().ActivationTimeout;

    auto operation = AsyncOperation::CreateAndStart<AssignIpAddressesAsyncOperation>(
        *this,
        servicePackageId,
        vector<wstring>(),
        true,
        timeout,
        [operationWaiter](AsyncOperationSPtr const & operation)
    {
        vector<wstring> ips;
        auto error = AssignIpAddressesAsyncOperation::End(operation, ips);
        operationWaiter->SetError(error);
        operationWaiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    ErrorCode error;
    if (operationWaiter->WaitOne(timeout))
    {
        error = operationWaiter->GetError();
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivatorClient,
            Root.TraceId,
            "AssignIpAddressesAsyncOperation request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "AssignIpAddressesAsyncOperation operationWaiter failed on Wait");
        error = ErrorCode(ErrorCodeValue::Timeout);
    }

    return error;
}

Common::ErrorCode FabricActivatorClient::CleanupAssignedOverlayNetworkResources(
    std::map<std::wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    std::wstring const & servicePackageId)
{
    AsyncOperationWaiterSPtr operationWaiter = make_shared<AsyncOperationWaiter>();

    TimeSpan timeout = HostingConfig::GetConfig().ActivationTimeout;

    auto operation = AsyncOperation::CreateAndStart<ManageOverlayNetworkResourcesAsyncOperation>(
        *this,
        nodeName,
        nodeIpAddress,
        servicePackageId,
        codePackageNetworkNames,
        ManageOverlayNetworkAction::Enum::Unassign,
        timeout,
        [operationWaiter](AsyncOperationSPtr const & operation)
    {
        std::map<wstring, std::map<std::wstring, std::wstring>> assignedOverlayNetworkResources;
        auto error = ManageOverlayNetworkResourcesAsyncOperation::End(operation, assignedOverlayNetworkResources);
        operationWaiter->SetError(error);
        operationWaiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    ErrorCode error;
    if (operationWaiter->WaitOne(timeout))
    {
        error = operationWaiter->GetError();
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivatorClient,
            Root.TraceId,
            "ManageOverlayNetworkResourcesAsyncOperation request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "ManageOverlayNetworkResourcesAsyncOperation operationWaiter failed on Wait");
        error = ErrorCode(ErrorCodeValue::Timeout);
    }

    return error;
}

AsyncOperationSPtr FabricActivatorClient::BeginConfigureContainerCertificateExport(
    std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & certificateRef,
    std::wstring workDirectoryPath,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<ConfigureContainerCertificateExportAsyncOperation>(
        *this,
        certificateRef,
        workDirectoryPath,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndConfigureContainerCertificateExport(
    Common::AsyncOperationSPtr const & operation,
    __out std::map<std::wstring, std::wstring> & certificatePaths,
    __out std::map<std::wstring, std::wstring> & certificatePasswordPaths)
{
    return ConfigureContainerCertificateExportAsyncOperation::End(operation, certificatePaths, certificatePasswordPaths);
}

AsyncOperationSPtr FabricActivatorClient::BeginCleanupContainerCertificateExport(
    std::map<std::wstring, std::wstring> const & certificatePaths,
    std::map<std::wstring, std::wstring> const & certificatePasswordPaths,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<CleanupContainerCertificateExportAsyncOperation>(
        *this,
        certificatePaths,
        certificatePasswordPaths,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndCleanupContainerCertificateExport(
    Common::AsyncOperationSPtr const & operation)
{
    return CleanupContainerCertificateExportAsyncOperation::End(operation);
}

void FabricActivatorClient::AbortApplicationEnvironment(wstring const & applicationId)
{
    AsyncOperationWaiterSPtr cleanupWaiter = make_shared<AsyncOperationWaiter>();

    TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;

    auto operation = AsyncOperation::CreateAndStart<CleanupSecurityPrincipalsAsyncOperation>(
        *this,
        applicationId,
        false /*cleanupForNode*/,
        timeout,
        [cleanupWaiter](AsyncOperationSPtr const & operation)
    {
        auto error = CleanupSecurityPrincipalsAsyncOperation::End(operation);
        cleanupWaiter->SetError(error);
        cleanupWaiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    if (cleanupWaiter->WaitOne(timeout))
    {
        auto error = cleanupWaiter->GetError();
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivatorClient,
            Root.TraceId,
            "AbortApplicationEnvironment request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "AbortApplicationEnvironment cleanupWaiter failed on Wait");
    }
}

Common::AsyncOperationSPtr FabricActivatorClient::BeginTerminateProcess(
    std::wstring const & appServiceId,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TerminateProcessAsyncOperation>(
        *this,
        appServiceId,
        callback,
        parent);
}

Common::ErrorCode FabricActivatorClient::EndTerminateProcess(
    Common::AsyncOperationSPtr const & operation)
{
    return TerminateProcessAsyncOperation::End(operation);
}

void FabricActivatorClient::AbortProcess(wstring const & appServiceId)
{
    AsyncOperationWaiterSPtr cleanupWaiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<TerminateProcessAsyncOperation>(
        *this,
        appServiceId,
        [cleanupWaiter](AsyncOperationSPtr const & operation)
    {
        auto error = TerminateProcessAsyncOperation::End(operation);
        cleanupWaiter->SetError(error);
        cleanupWaiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    if (cleanupWaiter->WaitOne(HostingConfig::GetConfig().FabricHostCommunicationTimeout))
    {
        auto error = cleanupWaiter->GetError();
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivatorClient,
            Root.TraceId,
            "TerminateProcess request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType_ActivatorClient,
            Root.TraceId,
            "TerminateProcess cleanupWaiter failed on Wait");
    }
}

AsyncOperationSPtr FabricActivatorClient::BeginDownloadContainerImages(
    vector<ContainerImageDescription> const & containerImages,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
        containerImages,
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndDownloadContainerImages(
    Common::AsyncOperationSPtr const & operation)
{
    return DownloadContainerImagesAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginDeleteContainerImages(
    vector<wstring> const & containerImages,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<DeleteContainerImagesAsyncOperation>(
        containerImages,
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndDeleteContainerImages(
    Common::AsyncOperationSPtr const & operation)
{
    return DeleteContainerImagesAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricActivatorClient::BeginGetContainerInfo(
    wstring const & appServiceId,
    bool isServicePackageActivationModeExclusive,
    wstring const & containerInfoType,
    wstring const & containerInfoArgs,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<GetContainerInfoAsyncOperation>(
        appServiceId,
        isServicePackageActivationModeExclusive,
        containerInfoType,
        containerInfoArgs,
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndGetContainerInfo(
    Common::AsyncOperationSPtr const & operation,
    __out wstring & containerInfo)
{
    return GetContainerInfoAsyncOperation::End(operation, containerInfo);
}

AsyncOperationSPtr FabricActivatorClient::BeginGetImages(
    TimeSpan timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(this->ipcClient_, "IPC client must be initialized for this operation");
    return AsyncOperation::CreateAndStart<GetImagesAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode FabricActivatorClient::EndGetImages(
    Common::AsyncOperationSPtr const & operation,
    __out std::vector<wstring> & images)
{
    return GetImagesAsyncOperation::End(operation, images);
}

bool FabricActivatorClient::IsIpcClientInitialized()
{
    return (bool)ipcClient_;
}

Common::AsyncOperationSPtr FabricActivatorClient::BeginConfigureNodeForDnsService(
    bool isDnsServiceEnabled,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ConfigureNodeForDnsServiceAsyncOperation>(
        *this, isDnsServiceEnabled, timeout, callback, parent);
}

Common::ErrorCode FabricActivatorClient::EndConfigureNodeForDnsService(
    Common::AsyncOperationSPtr const & operation)
{
    return ConfigureNodeForDnsServiceAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr FabricActivatorClient::BeginGetNetworkDeployedPackages(
    std::vector<std::wstring> const & servicePackageIds,
    std::wstring const & codePackageName,
    std::wstring const & networkName,
    std::wstring const & nodeId,
    std::map<std::wstring, std::wstring> const & codePackageInstanceAppHostMap,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetNetworkDeployedPackagesAsyncOperation>(
        *this,
        servicePackageIds,
        codePackageName,
        networkName,
        nodeId,
        codePackageInstanceAppHostMap,
        timeout,
        callback,
        parent);
}

Common::ErrorCode FabricActivatorClient::EndGetNetworkDeployedPackages(
    Common::AsyncOperationSPtr const & operation,
    __out std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & networkReservedCodePackages,
    __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & codePackageInstanceIdentifierContainerInfoMap)
{
    return GetNetworkDeployedPackagesAsyncOperation::End(operation, networkReservedCodePackages, codePackageInstanceIdentifierContainerInfoMap);
}

Common::AsyncOperationSPtr FabricActivatorClient::BeginGetDeployedNetworks(
    ServiceModel::NetworkType::Enum networkType,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetDeployedNetworksAsyncOperation>(
        *this,
        networkType,
        timeout,
        callback,
        parent);
}

Common::ErrorCode FabricActivatorClient::EndGetDeployedNetworks(
    Common::AsyncOperationSPtr const & operation,
    __out std::vector<std::wstring> & networkNames)
{
    return GetDeployedNetworksAsyncOperation::End(operation, networkNames);
}