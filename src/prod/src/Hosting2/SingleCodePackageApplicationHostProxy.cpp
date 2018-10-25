// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ImageModel;
using namespace Hosting2;
using namespace Transport;

StringLiteral const TraceType("SingleCodePackageApplicationHostProxy");

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::OpenAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::OpenAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        processPath_(),
        isContainerHost_(owner_.EntryPointType == EntryPointType::Enum::ContainerHost)
    {
    }

    virtual ~OpenAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Opening ApplicationHostProxy: Id={0}, Type={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeoutHelper_.GetRemainingTime());

        ActivateProcess(thisSPtr);
    }

private:
    void ActivateProcess(AsyncOperationSPtr const & thisSPtr)
    {
        ProcessDescriptionUPtr processDescription;
        auto error = owner_.GetProcessDescription(processDescription);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetProcessDescription: ErrorCode={0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        processPath_ = processDescription->ExePath;

        wstring assignedIP;
        error = owner_.GetAssignedIPAddress(assignedIP);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCode::Success();
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetAssignedIP error: ErrorCode={0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        // DNS chain section for containers.
        vector<wstring> dnsServers;
        if (error.IsSuccess() && isContainerHost_ && DNS::DnsServiceConfig::GetConfig().IsEnabled)
        {
            // Multi-IP chain
            if (!assignedIP.empty())
            {
                std::wstring dnsServerIP;
                auto err = Common::IpUtility::GetAdapterAddressOnTheSameNetwork(assignedIP, /*out*/dnsServerIP);
                if (err.IsSuccess())
                {
                    dnsServers.push_back(dnsServerIP);
                }
                else
                {
                    WriteWarning(TraceType, owner_.TraceId,
                        "Failed to get DNS IP address for container with assigned IP={0} : ErrorCode={1}",
                        assignedIP, err);
                }
            }
            // NAT container has to have the special DNS NAT adapter IP address set as the preferred DNS.
            // This is a workaround for the Windows bug which prevents NAT containers to reach the node.
            // Please remove once Windows fixes the NAT bug.
            else if (
                DNS::DnsServiceConfig::GetConfig().SetContainerDnsWhenPortBindingsAreEmpty ||
                !owner_.PortBindings.empty())
            {
#if defined(PLATFORM_UNIX)
                // On Linux, if no port bindings are specified in non-mip scenario, "host" networking mode is used as 
                // default networking mode, unless overriden. Hence, we need to add DNS servers on the host network. 
                // Otherwise, public connectivity will be broken for the container connected to host network. 
                if (owner_.PortBindings.empty() &&
                    StringUtility::AreEqualCaseInsensitive(HostingConfig::GetConfig().DefaultContainerNetwork, L"host"))
                {
                    std::wstring localhost = L"127.0.0.1";
                    dnsServers.push_back(localhost);
                }
                else
                {
#endif
                    std::wstring dnsServerIP;
                    auto err = FabricEnvironment::GetFabricDnsServerIPAddress(dnsServerIP);
                    if (err.IsSuccess())
                    {
                        dnsServers.push_back(dnsServerIP);
                    }
                    else
                    {
                        WriteWarning(TraceType, owner_.TraceId,
                            "Failed to get DNS IP address for the container from the environment : ErrorCode={0}",
                            err);
                    }
#if defined(PLATFORM_UNIX)
                } 
#endif
            }

            WriteNoise(TraceType, owner_.TraceId,
                "Container DNS setup: assignedIP={0}, SetContainerDnsWhenPortBindingsAreEmpty={1}, IsPortBindingEmpty={2}, DefaultContainerNetwork={3}, ContainerDnsChain={4}",
                assignedIP,
                DNS::DnsServiceConfig::GetConfig().SetContainerDnsWhenPortBindingsAreEmpty,
                owner_.PortBindings.empty(),
                HostingConfig::GetConfig().DefaultContainerNetwork,
                dnsServers);
        }

        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ProcessActivate): HostId={0}, HostType={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeout);

        if (error.IsSuccess())
        {
            auto containerPolicies = owner_.ContainerPolicies;

            vector<wstring> securityOptions;
            if (!containerPolicies.SecurityOptions.empty())
            {
                for (auto it = containerPolicies.SecurityOptions.begin(); it != containerPolicies.SecurityOptions.end(); ++it)
                {
                    securityOptions.push_back(it->Value);
                }
            }

            bool autoRemove = false;
            if (containerPolicies.AutoRemove.empty())
            {
                if (containerPolicies.ContainersRetentionCount != 0)
                {
                    WriteInfo(
                        TraceType,
                        owner_.TraceId,
                        "Containerretentioncount {1} failure count {2}",
                        containerPolicies.ContainersRetentionCount,
                        owner_.codePackageInstance_->ContinuousFailureCount);

                    //If retentioncount is negative, we keep retaining all failed containers
                    if (containerPolicies.ContainersRetentionCount > 0)
                    {
                        if (owner_.codePackageInstance_->ContinuousFailureCount >= (ULONG)containerPolicies.ContainersRetentionCount)
                        {
                            autoRemove = true;
                        }
                    }
                }
                else
                {
                    autoRemove = true;
                }
            }

            std::vector<ContainerLabelDescription> newLabels(containerPolicies.Labels.size());
            if (containerPolicies.Labels.size() > 0)
            {
                std::copy(containerPolicies.Labels.begin(), containerPolicies.Labels.end(), newLabels.begin());
            }

            auto logConfig = containerPolicies.LogConfig;

            if (isContainerHost_)
            {
                GetLogConfigAndLogLabel(logConfig, newLabels);
            }

#if defined(PLATFORM_UNIX)
            ContainerPodDescription podDesc(L"",
                                   owner_.codePackageInstance_->EnvContext->ContainerGroupIsolated ?
                                       ContainerIsolationMode::hyperv: ContainerIsolationMode::process,
                                   vector<PodPortMapping>());
#endif

            auto containerDescription = make_unique<ContainerDescription>(
                owner_.ApplicationName,
                owner_.GetServiceName(),
                owner_.ApplicationId.ToString(),
                owner_.GetUniqueContainerName(),
                owner_.codePackageInstance_->EntryPoint.ContainerEntryPoint.EntryPoint,
                containerPolicies.Isolation,
                containerPolicies.Hostname,
                owner_.Hosting.DeploymentFolder,
                owner_.Hosting.NodeWorkFolder,
                assignedIP,
                owner_.PortBindings,
                logConfig,
                containerPolicies.Volumes,
                newLabels,
                dnsServers,
                containerPolicies.RepositoryCredentials,
                containerPolicies.HealthConfig,
                securityOptions,
#if defined(PLATFORM_UNIX)
                podDesc,
#endif
                owner_.removeServiceFabricRuntimeAccess_,
                owner_.codePackageInstance_->EnvContext->GroupContainerName,
                containerPolicies.UseDefaultRepositoryCredentials,
                containerPolicies.UseTokenAuthenticationCredentials,
                autoRemove,
                containerPolicies.RunInteractive,
                false /*isContainerRoot*/,
                owner_.codePackageInstance_->Context.CodePackageInstanceId.CodePackageName,
                owner_.ServicePackageInstanceId.PublicActivationId,
                owner_.ServicePackageInstanceId.ActivationContext.ToString(),
                owner_.codePackageInstance_->CodePackageObj.BindMounts);

            auto operation = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->BeginActivateProcess(
                owner_.ApplicationId.ToString(),
                owner_.HostId,
                processDescription,
                owner_.RunAsId,
                isContainerHost_,
                move(containerDescription),
                timeout,
                [this](AsyncOperationSPtr const & operation) { this->OnProcessActivated(operation); },
                thisSPtr);
            if (operation->CompletedSynchronously)
            {
                FinishActivateProcess(operation);
            }
        }
    }

    void OnProcessActivated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishActivateProcess(operation);
        }
    }

    void FinishActivateProcess(AsyncOperationSPtr const & operation)
    {
        DWORD processId;
        auto error = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->EndActivateProcess(operation, processId);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ProcessActivate): ErrorCode={0}, HostId={1}",
            error,
            owner_.HostId);

        if (error.IsSuccess())
        {
            owner_.codePackageRuntimeInformation_ = make_shared<CodePackageRuntimeInformation>(processPath_, processId);
        }

        owner_.activationFailed_.store(!error.IsSuccess());
        owner_.activationFailedError_ = error;

        this->TryComplete(operation->Parent, error);
    }

private:
    ErrorCode GetLogConfigAndLogLabel(
        _Out_ LogConfigDescription & logConfig,
        _Out_ std::vector<ContainerLabelDescription> & newLabels)
    {
        auto containerPolicies = owner_.ContainerPolicies;
        LogConfigDescription tempLogConfig;

        auto error = GetLogConfigDescription(containerPolicies.LogConfig, tempLogConfig);

        if (error.IsSuccess())
        {
            // If the log driver is the sblogdriver then we want to add a label to the container containing the value.
            logConfig = move(tempLogConfig);
            for(auto& it : logConfig.DriverOpts)
            {
                if (it.Name == Constants::ContainerLogDriverOptionLogBasePathKey)
                {
                    ContainerLabelDescription logRootPathLabel;
                    logRootPathLabel.Name = Constants::ContainerLabels::LogBasePathKeyName;
                    logRootPathLabel.Value = it.Value;
                    newLabels.push_back(logRootPathLabel);
                }
            }
        }
        else
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Failed to parse log driver config. Proceeding without making changes to the logConfig.");
        }
        return ErrorCode::Success();
    }

    ErrorCode GetLogConfigDescription(
        LogConfigDescription const & appLogConfig,
        _Out_ LogConfigDescription & logConfig)
    {
        // logConfig will be populated with appLogConfig if log driver is specified in appLogConfig, otherwise it will be populated from ContainerLogDriverConfig
        if (!appLogConfig.Driver.empty())
        {
            // populate from appLogConfig because it is not empty or the cluster manifest has no default
            logConfig = appLogConfig;
        }
        else if (!Hosting2::ContainerLogDriverConfig::GetConfig().Type.empty())
        {
            // populate from ContainerLogDriverConfig
            logConfig.Driver = Hosting2::ContainerLogDriverConfig::GetConfig().Type;

            ServiceModel::ContainerLogDriverConfigDescription logDriverOpts;

            auto error = JsonHelper::Deserialize(logDriverOpts, Hosting2::ContainerLogDriverConfig::GetConfig().Config);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "GetLogConfigDescription failed to deserialize log driver options from ContainerLogDriver.Config {0}. Returning with error {1}.",
                    Hosting2::ContainerLogDriverConfig::GetConfig().Config,
                    error);

                return error;
            }

            WriteInfo(
                TraceType,
                owner_.TraceId,
                "GetlogConfigDescription successfully deserialized ContainerLogDriver.Config. Will proceed to use log driver configs from this value {0}.",
                Hosting2::ContainerLogDriverConfig::GetConfig().Config);

            for (auto& it: logDriverOpts.Config)
            {
                DriverOptionDescription driverOption;
                driverOption.Name = it.first;   // key
                driverOption.Value = it.second; // value
                driverOption.IsEncrypted = L"false";

                logConfig.DriverOpts.push_back(move(driverOption));
            }
        }
        else
        {
            logConfig = owner_.ContainerPolicies.LogConfig;
        }

        return ErrorCode::Success();
    }

    SingleCodePackageApplicationHostProxy & owner_;
    TimeoutHelper const timeoutHelper_;
    std::wstring processPath_;
    bool isContainerHost_;
};

// ********************************************************************************************************************
// ApplicationHostProxy::CloseAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::CloseAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Closing SingleCodePackageApplicationHostProxy: Id={0}, Type={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeout);

        if (!owner_.terminatedExternally_.load())
        {
            owner_.exitCode_.store(ProcessActivator::ProcessDeactivateExitCode);

            auto operation = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->BeginDeactivateProcess(
                owner_.HostId,
                timeout,
                [this](AsyncOperationSPtr const & operation) { this->OnProcessTerminated(operation); },
                thisSPtr);

            if (operation->CompletedSynchronously)
            {
                FinishTerminateProcess(operation);
            }

        }
        else
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "terminatedExternally_ is true. Host process already closed.");

            owner_.NotifyTermination();

            auto error = ErrorCode(ErrorCodeValue::Success);
            TryComplete(thisSPtr, error);
        }
    }

private:
    void OnProcessTerminated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishTerminateProcess(operation);
        }
    }

    void FinishTerminateProcess(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->EndDeactivateProcess(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "ApplicationHostProxy Closed. ErrorCode={0}",
            error);

        if (error.IsSuccess())
        {
            owner_.NotifyTermination();
        }

        TryComplete(operation->Parent, error);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    TimeoutHelper const timeoutHelper_;
};

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::ApplicationHostCodePackageOperation Implementation
//
class SingleCodePackageApplicationHostProxy::ApplicationHostCodePackageOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ApplicationHostCodePackageOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        ApplicationHostCodePackageOperationRequest const & request,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , request_(request)
    {
    }

    static ErrorCode ApplicationHostCodePackageOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostCodePackageOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.Hosting.State.Value > FabricComponentState::Opened ||
            owner_.State.Value > FabricComponentState::Opened ||
            owner_.ShouldNotify() == false)
        {
            //
            // No need to proceed if Hosting or ApplicationHostProxy
            // is closing or ApplicationHost has terminated.
            //
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        auto operation = owner_.codePackageInstance_->BeginApplicationHostCodePackageOperation(
            request_,
            [this](AsyncOperationSPtr const & operation)
            {
            this->OnApplicationHostCodePackageOperationCompleted(operation, false);
            },
            thisSPtr);

        this->OnApplicationHostCodePackageOperationCompleted(operation, true);
    }

private:

    void OnApplicationHostCodePackageOperationCompleted(
        AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.codePackageInstance_->EndApplicationHostCodePackageOperation(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "OnApplicationHostCodePackageOperationCompleted: ErrorCode={0}",
            error);

        this->TryComplete(operation->Parent, error);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    ApplicationHostCodePackageOperationRequest request_;
};

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::CodePackageEventNotificationAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::SendDependentCodePackageEventAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(SendDependentCodePackageEventAsyncOperation)

public:
    SendDependentCodePackageEventAsyncOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        CodePackageEventDescription const & eventDescription,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , eventDescription_(eventDescription)
        , timeoutHelper_(HostingConfig::GetConfig().RequestTimeout)
    {
    }

    virtual ~SendDependentCodePackageEventAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<SendDependentCodePackageEventAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->SendEvent(thisSPtr);
    }

private:
    void SendEvent(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(SendDependentCodePackageEvent): AppHostContext={0}, CodePackageInstanceId={1}, InstanceId={2}, Event={3}, Timeout={4}",
            owner_.Context,
            owner_.CodePackageInstanceId,
            owner_.codePackageInstance_->InstanceId,
            eventDescription_,
            timeoutHelper_.GetRemainingTime());

        auto request = this->CreateNotificationRequestMessage();

        auto operation = owner_.Hosting.IpcServerObj.BeginRequest(
            move(request),
            owner_.HostId,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            { 
                this->FinishSendEvent(operation, false);
            },
            thisSPtr);
        
        this->FinishSendEvent(operation, true);
    }

    void FinishSendEvent(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Hosting.IpcServerObj.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(SendDependentCodePackageEvent): AppHostContext={0}, CodePackageInstanceId={1}, InstanceId={2}, Event={3}, ErrorCode={4}",
                owner_.Context,
                owner_.CodePackageInstanceId,
                owner_.codePackageInstance_->InstanceId,
                eventDescription_,
                error);

            this->TryComplete(operation->Parent, error);

            if (!error.IsError(ErrorCodeValue::NotFound))
            {
                owner_.Abort();
            }

            return;
        }

        CodePackageEventNotificationReply replyBody;
        if (!reply->GetBody<CodePackageEventNotificationReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);

            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<CodePackageEventNotificationReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);

            this->TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "CodePackageEventNotificationReply: AppHostContext={0}, CodePackageInstanceId={1}, ReplyBody={2}.",
            owner_.Context,
            owner_.CodePackageInstanceId,
            replyBody);

        this->TryComplete(operation->Parent, error);
    }

    MessageUPtr CreateNotificationRequestMessage()
    {
        CodePackageEventNotificationRequest requestBody(eventDescription_);

        auto request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHost));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::CodePackageEventNotification));

        return move(request);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    CodePackageEventDescription eventDescription_;
    TimeoutHelper timeoutHelper_;
};

SingleCodePackageApplicationHostProxy::SingleCodePackageApplicationHostProxy(
    HostingSubsystemHolder const & hostingHolder,
    wstring const & hostId,
    ApplicationHostIsolationContext const & isolationContext,
    CodePackageInstanceSPtr const & codePackageInstance)
    : ApplicationHostProxy(
        hostingHolder,
        ApplicationHostContext(
            hostId,
            ApplicationHostType::Activated_SingleCodePackage,
            codePackageInstance->IsContainerHost,
            codePackageInstance->IsActivator),
        isolationContext,
        codePackageInstance->RunAsId,
        codePackageInstance->CodePackageInstanceId.ServicePackageInstanceId,
        codePackageInstance->EntryPoint.EntryPointType,
        codePackageInstance->CodePackageObj.RemoveServiceFabricRuntimeAccess)
    , codePackageActivationId_(hostId)
    , codePackageInstance_(codePackageInstance)
    , codePackageRuntimeInformation_()
    , shouldNotify_(true)
    , notificationLock_()
    , terminatedExternally_(false)
    , activationFailed_(false)
    , activationFailedError_()
    , isCodePackageActive_(false)
    , exitCode_(1)
    , procHandle_()
{
}

// ********************************************************************************************************************
// SingleCodePackageApplicationHostProxy::GetContainerLogsAsyncOperation Implementation
//
class SingleCodePackageApplicationHostProxy::GetContainerInfoAsyncOperation : public AsyncOperation
{
public:
    GetContainerInfoAsyncOperation(
        __in SingleCodePackageApplicationHostProxy & owner,
        wstring const & containerInfoType,
        wstring const & containerInfoArgs,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        containerInfoType_(containerInfoType),
        containerInfoArgs_(containerInfoArgs),
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
            TraceType,
            owner_.TraceId,
            "SingleCodePackageApplicationHostProxy: Sending get container info ApplicationHostProxy service. {0}",
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->BeginGetContainerInfo(
            owner_.HostId,
            owner_.ServicePackageInstanceId.ActivationContext.IsExclusive,
            containerInfoType_,
            containerInfoArgs_,
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

        auto error = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->EndGetContainerInfo(operation, containerInfo_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(GetContainerInfo): ErrorCode={0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, error);
    }

private:
    SingleCodePackageApplicationHostProxy & owner_;
    wstring containerInfoType_;
    wstring containerInfoArgs_;
    TimeoutHelper const timeoutHelper_;
    wstring containerInfo_;
};

SingleCodePackageApplicationHostProxy::~SingleCodePackageApplicationHostProxy()
{
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::BeginActivateCodePackage(
    CodePackageInstanceSPtr const & codePackage,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(codePackage);
    UNREFERENCED_PARAMETER(timeout);

    if (isCodePackageActive_.load())
    {
        Assert::CodingError(
            "SingleCodePackageApplicationHostProxy::BeginActivateCodePackage should only be called once. CodePackage {0}",
            codePackageInstance_->Context);
    }

    isCodePackageActive_.store(true);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::EndActivateCodePackage(
    Common::AsyncOperationSPtr const & operation,
    __out CodePackageActivationId & activationId,
    __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation)
{
    activationId = codePackageActivationId_;
    codePackageRuntimeInformation = codePackageRuntimeInformation_;

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::BeginDeactivateCodePackage(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(codePackageInstanceId);
    UNREFERENCED_PARAMETER(activationId);
    UNREFERENCED_PARAMETER(timeout);

    isCodePackageActive_.store(false);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::EndDeactivateCodePackage(
    AsyncOperationSPtr const & operation)
{
    UNREFERENCED_PARAMETER(operation);

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::BeginGetContainerInfo(
    wstring const & containerInfoType,
    wstring const & containerInfoArgs,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetContainerInfoAsyncOperation>(
        *this,
        containerInfoType,
        containerInfoArgs,
        timeout,
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::EndGetContainerInfo(
    AsyncOperationSPtr const & operation,
    __out wstring & containerInfo)
{
    return GetContainerInfoAsyncOperation::End(operation, containerInfo);
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::BeginApplicationHostCodePackageOperation(
    ApplicationHostCodePackageOperationRequest const & requestBody,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ApplicationHostCodePackageOperation>(
        *this,
        requestBody,
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostProxy::EndApplicationHostCodePackageOperation(
    AsyncOperationSPtr const & operation)
{
    return ApplicationHostCodePackageOperation::End(operation);
}

void SingleCodePackageApplicationHostProxy::SendDependentCodePackageEvent(
    CodePackageEventDescription const & eventDescription)
{
    AsyncOperation::CreateAndStart<SendDependentCodePackageEventAsyncOperation>(
        *this,
        eventDescription,
        [this](AsyncOperationSPtr const & operation)
        {
            SendDependentCodePackageEventAsyncOperation::End(operation).ReadValue();
        },
        this->CreateAsyncOperationRoot());
}

bool SingleCodePackageApplicationHostProxy::HasHostedCodePackages()
{
    return isCodePackageActive_.load();
}

void SingleCodePackageApplicationHostProxy::OnApplicationHostRegistered()
{
    // NO-OP
}

void SingleCodePackageApplicationHostProxy::OnApplicationHostTerminated(DWORD exitCode)
{
    this->exitCode_.store(exitCode);

    if (this->State.Value < FabricComponentState::Closing)
    {
        this->terminatedExternally_.store(true);
    }

    //
    // Abort if we are not already closing.
    //
    if (this->State.Value != FabricComponentState::Closing)
    {
        this->Abort();
    }

    WriteInfo(
        TraceType,
        TraceId,
        "OnApplicationHostTerminated called for HostId={0}, ServicePackageInstanceId={1} with ExitCode={2}. Current state is {3}.",
        this->HostId,
        this->ServicePackageInstanceId,
        exitCode,
        this->State);
}

void SingleCodePackageApplicationHostProxy::EmitDiagnosticTrace()
{
    //
    // IMPORTANT
    // Trace emitted here is consumed by Fault Analysis Service (FAS). If you are making a change
    // make sure FAS owners are aware of changes to avoid breaking the diagnostics tools.
    //

    if (activationFailed_.load())
    {
        // TODO: Trace here for failed start.
        return;
    }

    auto exitCode = (DWORD)exitCode_.load();
    auto unexpectedTermination = terminatedExternally_.load();

    if (this->Context.IsContainerHost)
    {
        hostingTrace.ContainerExitedOperational(
            Guid::NewGuid(),
            this->codePackageInstance_->Context.ApplicationName,
            this->GetServiceName(),
            this->ServicePackageInstanceId.ServicePackageName,
            this->ServicePackageInstanceId.PublicActivationId,
            this->ServicePackageInstanceId.ActivationContext.IsExclusive,
            this->codePackageInstance_->CodePackageInstanceId.CodePackageName,
            this->EntryPointType,
            this->codePackageInstance_->EntryPoint.ContainerEntryPoint.ImageName,
            this->GetUniqueContainerName(),
            this->HostId,
            exitCode,
            unexpectedTermination,
            this->codePackageInstance_->GetLastActivationTime());
    }
    else
    {
        DWORD processId = 0;
        if (codePackageRuntimeInformation_ != nullptr)
        {
            processId = codePackageRuntimeInformation_->ProcessId;
        }

        hostingTrace.ProcessExitedOperational(
            Guid::NewGuid(),
            this->codePackageInstance_->Context.ApplicationName,
            this->GetServiceName(),
            this->ServicePackageInstanceId.ServicePackageName,
            this->ServicePackageInstanceId.PublicActivationId,
            this->ServicePackageInstanceId.ActivationContext.IsExclusive,
            this->codePackageInstance_->CodePackageInstanceId.CodePackageName,
            this->EntryPointType,
            this->codePackageInstance_->EntryPoint.ExeEntryPoint.Program,
            processId,
            this->HostId,
            exitCode,
            unexpectedTermination,
            this->codePackageInstance_->GetLastActivationTime());
    }
}

void SingleCodePackageApplicationHostProxy::OnContainerHealthCheckStatusChanged(ContainerHealthStatusInfo const & healthStatusInfo)
{
    if (this->Hosting.State.Value > FabricComponentState::Opened)
    {
        // no need to notify if the Subsystem is closing down
        return;
    }

    WriteNoise(
        TraceType,
        TraceId,
        "OnContainerHealthCheckStatusChanged called for HostId={0}, ServicePackageInstanceId={1} with {2}. Current state is {3}.",
        this->HostId,
        this->ServicePackageInstanceId,
        healthStatusInfo,
        this->State);

    if (this->ShouldNotify())
    {
        codePackageInstance_->OnHealthCheckStatusChanged(codePackageActivationId_, healthStatusInfo);
    }
}

void SingleCodePackageApplicationHostProxy::OnCodePackageTerminated(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId)
{
    UNREFERENCED_PARAMETER(codePackageInstanceId);
    UNREFERENCED_PARAMETER(activationId);

    // NO-OP
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::OnBeginOpen(
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

ErrorCode SingleCodePackageApplicationHostProxy::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr SingleCodePackageApplicationHostProxy::OnBeginClose(
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

ErrorCode SingleCodePackageApplicationHostProxy::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void SingleCodePackageApplicationHostProxy::OnAbort()
{
    //
    // Abort can happen in three cases:
    // 1) Open of proxy failed.
    // 2) CodePackage itself is shutting down.
    // 3) UpdateContext to ApplicationHost fails.
    // 4) Proxy received termination notification on AppHostManager notification.
    //
    // If proxy open failed CodePackageInstance open will also fail.
    //

    if (!terminatedExternally_.load())
    {
        this->exitCode_.store(ProcessActivator::ProcessAbortExitCode);
        Hosting.ApplicationHostManagerObj->FabricActivator->AbortProcess(this->HostId);
    }

    // We will notify only when CodePackage has activated meaning the process has started.
    // If the process failed to start then open will fail, it will cause CodePackageInstance Start to fail
    // which will trigger reschedule of CodePackageInstance.
    if (!activationFailed_.load())
    {
        this->NotifyTermination();
    }

    this->EmitDiagnosticTrace();
}

ErrorCode SingleCodePackageApplicationHostProxy::GetProcessDescription(
    __out ProcessDescriptionUPtr & processDescription)
{
    ASSERT_IF(
        codePackageInstance_->EntryPoint.EntryPointType == EntryPointType::DllHost,
        "SingleCodePackageApplicationHostProxy can only handle Executable entry points.");

    bool isExternalExecutable = codePackageInstance_->EntryPoint.ExeEntryPoint.IsExternalExecutable;

    wstring exePath;
    wstring arguments;

    exePath = codePackageInstance_->EntryPoint.ExeEntryPoint.Program;
    StringUtility::Trim<wstring>(exePath, L"'");
    StringUtility::Trim<wstring>(exePath, L"\"");
    arguments = codePackageInstance_->EntryPoint.ExeEntryPoint.Arguments;


    RunLayoutSpecification runLayout(Hosting.DeploymentFolder);
    auto applicationIdString = codePackageInstance_->Context.CodePackageInstanceId.ServicePackageInstanceId.ApplicationId.ToString();

    wstring appDirectory = runLayout.GetApplicationFolder(applicationIdString);
    wstring logDirectory = runLayout.GetApplicationLogFolder(applicationIdString);
    wstring tempDirectory = runLayout.GetApplicationTempFolder(applicationIdString);
    wstring workDirectory = runLayout.GetApplicationWorkFolder(applicationIdString);
    wstring codePackageDirectory = runLayout.GetCodePackageFolder(
        applicationIdString,
        codePackageInstance_->Context.CodePackageInstanceId.ServicePackageInstanceId.ServicePackageName,
        codePackageInstance_->Context.CodePackageInstanceId.CodePackageName,
        codePackageInstance_->Version,
        codePackageInstance_->IsPackageShared);

#if defined(PLATFORM_UNIX)
    if (isExternalExecutable)
    {
        exePath = Path::Combine(HostingConfig::GetConfig().LinuxExternalExecutablePath, exePath);
    }
    else if (!Path::IsPathRooted(exePath))
    {
        exePath = Path::Combine(codePackageDirectory, exePath);
    }
#else
    if (!Path::IsPathRooted(exePath) && !isExternalExecutable)
    {
        exePath = Path::Combine(codePackageDirectory, exePath);
    }
#endif

    // TODO: Check with Rajeet.
    // This will name is not unique per activation of service package in exlusive mode.
    wstring redirectedFileNamePrefix = wformatString(
        "{0}_{1}_{2}",
        codePackageInstance_->Context.CodePackageInstanceId.CodePackageName,
        codePackageInstance_->Context.CodePackageInstanceId.ServicePackageInstanceId.ServicePackageName,
        codePackageInstance_->IsSetupEntryPoint ? L"S" : L"M");


    wstring startInDirectory;
    switch (codePackageInstance_->EntryPoint.ExeEntryPoint.WorkingFolder)
    {
    case ExeHostWorkingFolder::Work:
        startInDirectory = workDirectory;
        break;
    case ExeHostWorkingFolder::CodePackage:
        startInDirectory = codePackageDirectory;
        break;
    case ExeHostWorkingFolder::CodeBase:
        startInDirectory = Path::GetDirectoryName(exePath);
        break;
    default:
        Assert::CodingError("Invalid WorkingFolder value {0}", static_cast<int>(codePackageInstance_->EntryPoint.ExeEntryPoint.WorkingFolder));
    }

    EnvironmentMap envVars;
    if (!this->removeServiceFabricRuntimeAccess_) // This if check is just an optimization for not doing this work if runtime access is not allowed for this host.
    {
        if (codePackageInstance_->EntryPoint.EntryPointType != EntryPointType::ContainerHost)
        {
            this->GetCurrentProcessEnvironmentVariables(envVars, tempDirectory, this->Hosting.FabricBinFolder);
        }
        //Add folders created for application.
        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Work", Constants::EnvironmentVariable::FoldersPrefix), workDirectory);
        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Log", Constants::EnvironmentVariable::FoldersPrefix), logDirectory);
        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Temp", Constants::EnvironmentVariable::FoldersPrefix), tempDirectory);
        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}Application", Constants::EnvironmentVariable::FoldersPrefix), appDirectory);

        auto error = AddHostContextAndRuntimeConnection(envVars);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                TraceId,
                "AddHostContextAndRuntimeConnection: ErrorCode={0}",
                error);
            return error;
        }

        codePackageInstance_->Context.ToEnvironmentMap(envVars);

        //Add all the ActivationContext, NodeContext environment variables
        for (auto it = codePackageInstance_->EnvContext->Endpoints.begin(); it != codePackageInstance_->EnvContext->Endpoints.end(); it++)
        {
            AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}{1}", Constants::EnvironmentVariable::EndpointPrefix, (*it)->Name), StringUtility::ToWString((*it)->Port));
            AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}{1}", Constants::EnvironmentVariable::EndpointIPAddressOrFQDNPrefix, (*it)->Name), (*it)->IpAddressOrFqdn);
        }

        AddSFRuntimeEnvironmentVariable(envVars, Constants::EnvironmentVariable::NodeIPAddressOrFQDN, Hosting.FabricNodeConfigObj.IPAddressOrFQDN);

        // Add partition id even when no runtime is being shared when host is SFBlockstore service.
        // if (StringUtility::AreEqualCaseInsensitive(codePackageInstance_->Name, Constants::BlockStoreServiceCodePackageName))
        {
            // ActivationContext is only available for Exclusive Hosting mode.
            if (ServicePackageInstanceId.ActivationContext.IsExclusive)
            {
                AddSFRuntimeEnvironmentVariable(envVars, Constants::EnvironmentVariable::PartitionId, ServicePackageInstanceId.ActivationContext.ToString());
            }
        }

        for (auto const & kvPair : codePackageInstance_->CodePackageObj.OnDemandActivationEnvironment)
        {
            this->AddSFRuntimeEnvironmentVariable(envVars, kvPair.first, kvPair.second);
        }
    }

    map<wstring, wstring> encryptedSettings;
    for (auto const& it : codePackageInstance_->EnvVariablesDescription.EnvironmentVariables)
    {
        wstring value = L"";
        if (StringUtility::AreEqualCaseInsensitive(it.Type, Constants::Encrypted))
        {
            //Decrypt while passing it to docker or while creating process
            encryptedSettings.insert(
                make_pair(
                    it.Name,
                    it.Value));
            continue;
        }
        else if (StringUtility::AreEqualCaseInsensitive(it.Type, Constants::SecretsStoreRef))
        {
            //Todo: 11905876: Integrate querying SecretStoreService in Hosting
            continue;
        }
        else
        {
            //Defaults to PlainText
            AddEnvironmentVariable(envVars, it.Name, it.Value);
        }
    }

    //Add the env vars for ConfigPackagePolicies
    for (auto const& it : this->codePackageInstance_->CodePackageObj.EnvironmentVariableForMounts)
    {
        AddEnvironmentVariable(envVars, it.first, it.second);
    }


    auto serviceName = this->GetServiceName();
    if (!serviceName.empty())
    {
        AddSFRuntimeEnvironmentVariable(envVars, Constants::EnvironmentVariable::ServiceName, serviceName);
    }

    if (codePackageInstance_->EntryPoint.EntryPointType == EntryPointType::ContainerHost)
    {
        //Get the container image name based on Os build version
        wstring imageName = codePackageInstance_->EntryPoint.ContainerEntryPoint.ImageName;

        auto error = ContainerHelper::GetContainerHelper().GetContainerImageName(codePackageInstance_->ContainerPolicies.ImageOverrides, imageName);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                TraceId,
                "GetContainerImageName({0}, {1}): ErrorCode={2}",
                codePackageInstance_->ContainerPolicies.ImageOverrides,
                imageName,
                error);
            return error;
        }

        exePath = imageName;
        arguments = codePackageInstance_->EntryPoint.ContainerEntryPoint.Commands;

#if !defined(PLATFORM_UNIX)
        //Windows containers only allow mapping volumes to c: today
        //so we need to fix up path for each directory;
        auto appDirectoryOnContainer = HostingConfig::GetConfig().GetContainerApplicationFolder(applicationIdString);

        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Work", Constants::EnvironmentVariable::FoldersPrefix), Path::Combine(appDirectoryOnContainer, L"work"));
        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Log", Constants::EnvironmentVariable::FoldersPrefix), Path::Combine(appDirectoryOnContainer, L"log"));
        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}App_Temp", Constants::EnvironmentVariable::FoldersPrefix), Path::Combine(appDirectoryOnContainer, L"temp"));
        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}Application", Constants::EnvironmentVariable::FoldersPrefix), appDirectoryOnContainer);
        AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}Application_OnHost", Constants::EnvironmentVariable::FoldersPrefix), appDirectory);

#endif

        if (!this->removeServiceFabricRuntimeAccess_)
        {
#if !defined(PLATFORM_UNIX)

            wstring certificateFolderPath;
            wstring certificateFolderName;
            auto passwordMap = codePackageInstance_->EnvContext->CertificatePasswordPaths;
            for (auto const & iter : codePackageInstance_->EnvContext->CertificatePaths)
            {
                if (certificateFolderPath.empty())
                {
                    certificateFolderName = Path::GetFileName(Path::GetDirectoryName(iter.second));
                    certificateFolderPath = Path::Combine(Path::Combine(appDirectoryOnContainer, L"work"), certificateFolderName);
                }
                AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}_{1}_PFX", certificateFolderName, iter.first), Path::Combine(certificateFolderPath, Path::GetFileName(iter.second)));
                AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}_{1}_Password", certificateFolderName, iter.first), Path::Combine(certificateFolderPath, Path::GetFileName(passwordMap[iter.first])));
            }
#else
            wstring certificateFolderPath;
            wstring certificateFolderName;
            auto passwordMap = codePackageInstance_->EnvContext->CertificatePasswordPaths;
            for (auto const & iter : codePackageInstance_->EnvContext->CertificatePaths)
            {
                if (certificateFolderPath.empty())
                {
                    certificateFolderName = Path::GetFileName(Path::GetDirectoryName(iter.second));
                    certificateFolderPath = Path::Combine(workDirectory, certificateFolderName);
                }
                AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}_{1}_PEM", certificateFolderName, iter.first), Path::Combine(certificateFolderPath, Path::GetFileName(iter.second)));
                AddSFRuntimeEnvironmentVariable(envVars, wformatString("{0}_{1}_PrivateKey", certificateFolderName, iter.first), Path::Combine(certificateFolderPath, Path::GetFileName(passwordMap[iter.first])));
            }
#endif

            auto const & configStoreDesc = FabricGlobals::Get().GetConfigStore();
            if (!configStoreDesc.StoreEnvironmentVariableName.empty())
            {
                AddSFRuntimeEnvironmentVariable(envVars, configStoreDesc.StoreEnvironmentVariableName, configStoreDesc.StoreEnvironmentVariableValue);
            }
        }
    }

    ProcessDebugParameters debugParameters(this->codePackageInstance_->DebugParameters);
    if (HostingConfig::GetConfig().EnableProcessDebugging)
    {
        if (!codePackageInstance_->DebugParameters.WorkingFolder.empty())
        {
            startInDirectory = codePackageInstance_->DebugParameters.WorkingFolder;
        }

        if (!codePackageInstance_->DebugParameters.DebugParametersFile.empty())
        {
            File debugParametersFile;
            auto error = debugParametersFile.TryOpen(codePackageInstance_->DebugParameters.DebugParametersFile);
            if (error.IsSuccess())
            {
                int fileSize = static_cast<int>(debugParametersFile.size());
                ByteBufferUPtr json = make_unique<ByteBuffer>();
                json->resize(fileSize);
                DWORD bytesRead;
                error = debugParametersFile.TryRead2(reinterpret_cast<void*>((*json).data()), fileSize, bytesRead);
                debugParametersFile.Close();
                json->resize(bytesRead);
                if (error.IsSuccess())
                {
                    ProcessDebugParametersJson p;
                    error = JsonHelper::Deserialize(p, json);
                    if (error.IsSuccess())
                    {
                        debugParameters.set_Arguments(p.Arguments);
                        debugParameters.set_ExePath(p.ExePath);
                    }
                }
            }
        }
    }

    ServiceModel::ResourceGovernancePolicyDescription resourceGovernancePolicy(codePackageInstance_->ResourceGovernancePolicy);
    ServiceModel::ServicePackageResourceGovernanceDescription spResourceGovernanceDescription(codePackageInstance_->CodePackageObj.VersionedServicePackageObj.PackageDescription.ResourceGovernanceDescription);

    bool isImplicitCodePackage = codePackageInstance_->CodePackageObj.IsImplicitTypeHost;
    // Depending on the platform (Windows/Linux) and CP type (container/process) we need to adjust affinity and CPU shares.
    // On Linux, we use cpu quota and period and we also need to pass the cgroupName to use for this code package
    // We should not be adjusting limits for fabric type host nor for setup entry point

    wstring cgroupName = L"";
    bool isContainerGroup = codePackageInstance_->EnvContext->SetupContainerGroup;

    if (!isImplicitCodePackage && !codePackageInstance_->IsSetupEntryPoint)
    {
        Hosting.LocalResourceManagerObj->AdjustCPUPoliciesForCodePackage(codePackageInstance_->CodePackageObj, resourceGovernancePolicy, Context.IsContainerHost, isContainerGroup);
#if defined(PLATFORM_UNIX)
        //do not pass cgroup name in case this is a container which is not member of a container group or the container group does not have service package rg limits
        if ((resourceGovernancePolicy.ShouldSetupCgroup() || spResourceGovernanceDescription.IsGoverned) && !HostingConfig::GetConfig().LocalResourceManagerTestMode && (!Context.IsContainerHost || (isContainerGroup && spResourceGovernanceDescription.IsGoverned)))
        {
            //important to add / because of way docker handles cgroup parent name
            cgroupName = L"/" + Constants::CgroupPrefix + L"/" + codePackageInstance_->CodePackageInstanceId.ServicePackageInstanceId.ToString();
            //if this is a container group just pass the parent here else append code package name
            if (!Context.IsContainerHost)
            {
                cgroupName = cgroupName + L"/" + codePackageInstance_->CodePackageInstanceId.CodePackageName;
            }
        }
#endif

    }
    //For blockstore service set workdir same as exe location
    if (StringUtility::AreEqualCaseInsensitive(codePackageInstance_->Name, Constants::BlockStoreServiceCodePackageName))
    {
        startInDirectory = Path::GetDirectoryName(exePath);
    }
    processDescription = make_unique<ProcessDescription>(
        exePath,
        arguments,
        startInDirectory,
        envVars,
        appDirectory,
        tempDirectory,
        workDirectory,
        logDirectory,
        HostingConfig::GetConfig().EnableActivateNoWindow,
        codePackageInstance_->EntryPoint.ExeEntryPoint.ConsoleRedirectionEnabled,
        false,
        false,
        redirectedFileNamePrefix,
        codePackageInstance_->EntryPoint.ExeEntryPoint.ConsoleRedirectionFileRetentionCount,
        codePackageInstance_->EntryPoint.ExeEntryPoint.ConsoleRedirectionFileMaxSizeInKb,
        debugParameters,
        resourceGovernancePolicy,
        spResourceGovernanceDescription,
        cgroupName,
        false,
        encryptedSettings);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SingleCodePackageApplicationHostProxy::GetAssignedIPAddress(__out wstring & assignedIPaddress)
{
    return this->codePackageInstance_->EnvContext->GetIpAddressAssignedToCodePackage(
        this->codePackageInstance_->Name,
        assignedIPaddress);
}

wstring SingleCodePackageApplicationHostProxy::GetUniqueContainerName()
{
    auto name = wformatString("SF-{0}-{1}",
        StringUtility::ToWString(this->ApplicationId.ApplicationNumber),
        this->HostId);
    auto activationId = this->ServicePackageInstanceId.PublicActivationId;
    if (!activationId.empty())
    {
        name = wformatString("{0}_{1}", name, activationId);
    }
    StringUtility::ToLower(name);
    return name;
}

wstring SingleCodePackageApplicationHostProxy::GetContainerGroupId()
{
    return this->ServicePackageInstanceId.ActivationContext.ToString();
}

wstring SingleCodePackageApplicationHostProxy::GetServiceName()
{
    wstring serviceName;
    if (this->ServicePackageInstanceId.ActivationContext.IsExclusive)
    {
        auto res = this->Hosting.TryGetExclusiveServicePackageServiceName(
            this->ServicePackageInstanceId.ServicePackageId,
            this->ServicePackageInstanceId.ActivationContext,
            serviceName);

        ASSERT_IF(res == false, "ServiceName must be present for exlusive service package activation.");
    }
    return serviceName;
}

bool SingleCodePackageApplicationHostProxy::ShouldNotify()
{
    bool shouldNotify;

    {
        AcquireWriteLock readLock(notificationLock_);
        shouldNotify = shouldNotify_;
    }

    return shouldNotify;
}

void SingleCodePackageApplicationHostProxy::NotifyTermination()
{
    if (this->Hosting.State.Value > FabricComponentState::Opened)
    {
        // no need to notify if the Subsystem is closing down
        return;
    }

    {
        AcquireWriteLock writeLock(notificationLock_);
        if (!shouldNotify_)
        {
            return;
        }
        
        shouldNotify_ = false;
    }

    auto exitCode = (DWORD)exitCode_.load();

    codePackageInstance_->OnEntryPointTerminated(
        codePackageActivationId_, 
        exitCode, 
        false /* Ignore reporting */);
}
