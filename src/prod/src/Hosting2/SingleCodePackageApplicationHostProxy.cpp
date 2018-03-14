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

        processPath_ = processDescription->ExePath;
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetProcessDescriotion: ErrorCode={0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

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
            }

            std::vector<std::wstring> list;
            error = Common::IpUtility::GetDnsServers(list);
            if (error.IsSuccess() && !list.empty())
            {
                if (dnsServers.empty())
                {
                    dnsServers.insert(dnsServers.end(), list.begin(), list.end());
                }
                else
                {
                    // Avoid duplicates
                    for (int i = 0; i < list.size(); i++)
                    {
                        if (std::find(dnsServers.begin(), dnsServers.end(), list[i]) == dnsServers.end())
                        {
                            dnsServers.push_back(list[i]);
                        }
                    }
                }
            }

            WriteNoise(TraceType, owner_.TraceId,
                "Container DNS setup: assignedIP={0}, SetContainerDnsWhenPortBindingsAreEmpty={1}, IsPortBindingEmpty={2}, HostDnsChain={3}, ContainerDnsChain={4}",
                assignedIP, DNS::DnsServiceConfig::GetConfig().SetContainerDnsWhenPortBindingsAreEmpty, owner_.PortBindings.empty(), list, dnsServers);

            if (!error.IsSuccess())
            {
                WriteWarning(TraceType, owner_.TraceId,
                    "Failed to create DNS chain for the container : ErrorCode={0}",
                    error);

                error = ErrorCodeValue::ContainerFailedToCreateDnsChain;
                TryComplete(thisSPtr, error);
                return;
            }
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
            auto autoRemove = (containerPolicies.ContainersRetentionCount == 0);
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "AutoRemove defaults to {0} containerretentioncount {1} failure count {2}",
                autoRemove,
                containerPolicies.ContainersRetentionCount,
                owner_.codePackageInstance_->ContinuousFailureCount);
            if (autoRemove == false)
            {
                //If retentioncount is negative, we keep retaining all failed containers
                if (containerPolicies.ContainersRetentionCount > 0)
                {
                    if (owner_.codePackageInstance_->ContinuousFailureCount >= (ULONG)containerPolicies.ContainersRetentionCount)
                    {
                        autoRemove = true;
                    }
                }
            }
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
                containerPolicies.LogConfig,
                containerPolicies.Volumes, 
                dnsServers,
                containerPolicies.RepositoryCredentials,
                containerPolicies.HealthConfig,
                securityOptions,
                owner_.codePackageInstance_->EnvContext->GroupContainerName,
                autoRemove,
                containerPolicies.RunInteractive,
                false,
                owner_.codePackageInstance_->Context.CodePackageInstanceId.CodePackageName,
                owner_.ServicePackageInstanceId.PublicActivationId,
                owner_.ServicePackageInstanceId.ActivationContext.ToString());

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
#if !defined(PLATFORM_UNIX)
            error = SetupAppHostMonitor(processId);
#endif
        }

        TryComplete(operation->Parent, error);
        if (owner_.terminatedExternally_.load())
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "ApplicationHost {0} terminated externally during open with exit code {1}, aborting",
                owner_.HostId,
                owner_.exitCode_.load());
            //process terminated while applicationhost was being opened
            owner_.Abort();
        }
    }

    ErrorCode SetupAppHostMonitor(DWORD processId)
    {
        if (!isContainerHost_)
        {
            HandleUPtr appHostProcessHandle;
            DWORD desiredAccess = SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION;

            auto err = ProcessUtility::OpenProcess(
                desiredAccess,
                FALSE,
                processId,
                appHostProcessHandle);
            if (!err.IsSuccess())
            {
                WriteError(
                    TraceType,
                    "ApplicationHostProxy: OpenProcess: ErrorCode={0}, ParentId={1}. Fabric will not monitor AppHost {2}",
                    err,
                    processId,
                    owner_.HostId);
            }
            else
            {

                HostingSubsystem & hosting = owner_.Hosting;
                wstring hostId = owner_.HostId;
                owner_.procHandle_ = move(appHostProcessHandle);

                auto hostMonitor = ProcessWait::CreateAndStart(
                    Handle(owner_.procHandle_->Value, Handle::DUPLICATE()),
                    processId,
                    [&hosting, hostId](pid_t, ErrorCode const &, DWORD exitCode)
                {
                    //LINUXTODO, what happens waitResult indicates wait failed?
                    hosting.ApplicationHostManagerObj->OnApplicationHostTerminated(hostId, exitCode);
                });
                owner_.apphostMonitor_ = move(hostMonitor);
            }
        }
        return ErrorCodeValue::Success;
    }

private:
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

        if (owner_.apphostMonitor_ != nullptr)
        {
            owner_.apphostMonitor_->Cancel();
        }

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
            codePackageInstance->EntryPoint.EntryPointType == EntryPointType::ContainerHost),
        isolationContext,
        codePackageInstance->RunAsId,
        codePackageInstance->CodePackageInstanceId.ServicePackageInstanceId,
        codePackageInstance->EntryPoint.EntryPointType)
    , codePackageActivationId_(hostId)
    , codePackageInstance_(codePackageInstance)
    , codePackageRuntimeInformation_()
    , notificationLock_()
    , terminatedExternally_(false)
    , isCodePackageActive_(false)
    , exitCode_(1)
    , apphostMonitor_()
    , procHandle_()
{
    WriteNoise(
        TraceType,
        TraceId,
        "SingleCodePackageApplicationHostProxy.constructor: HostType={0}, HostId={1}, ServicePackageInstanceId={2}.",
        this->HostType,
        this->HostId,
        this->ServicePackageInstanceId);
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
    this->terminatedExternally_.store(true);

    if (this->State.Value != FabricComponentState::Closing &&
        this->State.Value != FabricComponentState::Opening)
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

    {
        AcquireReadLock readLock(notificationLock_);

        if (codePackageInstance_ != nullptr)
        {
            codePackageInstance_->OnHealthCheckStatusChanged(codePackageActivationId_, healthStatusInfo);
        }
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
    if (apphostMonitor_ != nullptr)
    {
        apphostMonitor_->Cancel();
    }
    if (!terminatedExternally_.load())
    {
        this->exitCode_.store(ProcessActivator::ProcessAbortExitCode);
        Hosting.ApplicationHostManagerObj->FabricActivator->AbortProcess(this->HostId);
    }
    
    this->NotifyTermination();
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
    if (codePackageInstance_->EntryPoint.EntryPointType != EntryPointType::ContainerHost)
    {
        this->GetCurrentProcessEnvironmentVariables(envVars, tempDirectory, this->Hosting.FabricBinFolder);
    }
    //Add folders created for application.
    envVars[wformatString("{0}App_Work", Constants::EnvironmentVariable::FoldersPrefix)] = workDirectory;
    envVars[wformatString("{0}App_Log", Constants::EnvironmentVariable::FoldersPrefix)] = logDirectory;
    envVars[wformatString("{0}App_Temp", Constants::EnvironmentVariable::FoldersPrefix)] = tempDirectory;
    envVars[wformatString("{0}Application", Constants::EnvironmentVariable::FoldersPrefix)] = appDirectory;

    auto error = AddHostContextAndRuntimeConnection(envVars);
    if (!error.IsSuccess()) { return error; }

    codePackageInstance_->Context.ToEnvironmentMap(envVars);
    for (auto it = codePackageInstance_->EnvVariablesDescription.EnvironmentVariables.begin(); it != codePackageInstance_->EnvVariablesDescription.EnvironmentVariables.end(); ++it)
    {
        envVars[it->Name] = it->Value;
    }

    auto serviceName = this->GetServiceName();
    if (!serviceName.empty())
    {
        envVars[Constants::EnvironmentVariable::ServiceName] = serviceName;
    }

    //Add all the ActivationContext, NodeContext environment variables
    for (auto it = codePackageInstance_->EnvContext->Endpoints.begin(); it != codePackageInstance_->EnvContext->Endpoints.end(); it++)
    {
        envVars[wformatString("{0}{1}", Constants::EnvironmentVariable::EndpointPrefix, (*it)->Name)] = StringUtility::ToWString((*it)->Port);
        envVars[wformatString("{0}{1}", Constants::EnvironmentVariable::EndpointIPAddressOrFQDNPrefix, (*it)->Name)] = (*it)->IpAddressOrFqdn;
    }

    envVars[Constants::EnvironmentVariable::NodeIPAddressOrFQDN] = Hosting.FabricNodeConfigObj.IPAddressOrFQDN;

    if (codePackageInstance_->EntryPoint.EntryPointType == EntryPointType::ContainerHost)
    {
        //Get the container image name based on Os build version
        wstring imageName = codePackageInstance_->EntryPoint.ContainerEntryPoint.ImageName;

        error = ContainerHelper::GetContainerHelper().GetContainerImageName(codePackageInstance_->ContainerPolicies.ImageOverrides, imageName);
        if (!error.IsSuccess())
        {
            return error;
        }

        exePath = imageName;
        arguments = codePackageInstance_->EntryPoint.ContainerEntryPoint.Commands;

#if !defined(PLATFORM_UNIX)
        //Windows containers only allow mapping volumes to c: today
        //so we need to fix up path for each directory;
        auto appDirectoryOnContainer = HostingConfig::GetConfig().GetContainerApplicationFolder(applicationIdString);

        envVars[wformatString("{0}App_Work", Constants::EnvironmentVariable::FoldersPrefix)] = Path::Combine(appDirectoryOnContainer, L"work");
        envVars[wformatString("{0}App_Log", Constants::EnvironmentVariable::FoldersPrefix)] = Path::Combine(appDirectoryOnContainer, L"log");
        envVars[wformatString("{0}App_Temp", Constants::EnvironmentVariable::FoldersPrefix)] = Path::Combine(appDirectoryOnContainer, L"temp");
        envVars[wformatString("{0}Application", Constants::EnvironmentVariable::FoldersPrefix)] = appDirectoryOnContainer;

        envVars[wformatString("{0}Application_OnHost", Constants::EnvironmentVariable::FoldersPrefix)] = appDirectory;

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
            envVars[wformatString("{0}_{1}_PFX", certificateFolderName, iter.first)] = Path::Combine(certificateFolderPath, Path::GetFileName(iter.second));
            envVars[wformatString("{0}_{1}_Password", certificateFolderName, iter.first)] = Path::Combine(certificateFolderPath, Path::GetFileName(passwordMap[iter.first]));
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
            envVars[wformatString("{0}_{1}_PEM", certificateFolderName, iter.first)] = Path::Combine(certificateFolderPath, Path::GetFileName(iter.second));
            envVars[wformatString("{0}_{1}_PrivateKey", certificateFolderName, iter.first)] = Path::Combine(certificateFolderPath, Path::GetFileName(passwordMap[iter.first]));
        }
#endif

        wstring configStoreEnvVarName;
        wstring configStoreEnvVarValue;
        error = ComProxyConfigStore::FabricGetConfigStoreEnvironmentVariable(configStoreEnvVarName, configStoreEnvVarValue);
        if (error.IsSuccess())
        {
            if (!configStoreEnvVarName.empty())
            {
                envVars[configStoreEnvVarName] = configStoreEnvVarValue;
            }
        }
    }

    ProcessDebugParameters debugParameters(codePackageInstance_->DebugParameters);
    if (HostingConfig::GetConfig().EnableProcessDebugging)
    {
        if (!codePackageInstance_->DebugParameters.WorkingFolder.empty())
        {
            startInDirectory = codePackageInstance_->DebugParameters.WorkingFolder;
        }

        if (!codePackageInstance_->DebugParameters.DebugParametersFile.empty())
        {
            File debugParametersFile;
            error = debugParametersFile.TryOpen(codePackageInstance_->DebugParameters.DebugParametersFile);
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
        cgroupName);

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
    auto activationId = this->ServicePackageInstanceId.ActivationContext.ToString();
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

void SingleCodePackageApplicationHostProxy::NotifyTermination()
{
    if (this->Hosting.State.Value > FabricComponentState::Opened)
    {
        // no need to notify if the Subsystem is closing down
        return;
    }

    {
        AcquireWriteLock writeLock(notificationLock_);

        auto tempCodePackageInstance = move(codePackageInstance_);

        auto exitCode = (DWORD)exitCode_.load();
        auto terminatedExternally = terminatedExternally_.load();

        tempCodePackageInstance->OnEntryPointTerminated(codePackageActivationId_, exitCode, !terminatedExternally);
    }
}
