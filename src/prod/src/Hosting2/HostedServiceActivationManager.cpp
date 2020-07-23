// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Transport;
using namespace Management::ImageModel;

StringLiteral const TraceType_ActivationManager("HostedServiceActivationManager");

class HostedServiceActivationManager::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation( HostedServiceActivationManager & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        lastError_(ErrorCodeValue::Success),
        hostedServices()
    {
    }

    static ErrorCode OpenAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SidUPtr currentSid;
        auto error = BufferedSid::GetCurrentUserSid(currentSid);
        if(!error.IsSuccess())
        {
            WriteError(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to get current user sid ErrorCode={0}. Exiting..",
                error);
            TryComplete(thisSPtr,error);
            return;
        }

        owner_.isSystem_ = currentSid->IsLocalSystem();

        WriteInfo(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "HostedServiceActivationManager service account IsSystem {0}",
            owner_.isSystem_);

        error = owner_.endpointProvider_->Open();
        if(!error.IsSuccess())
        {
            WriteError(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Open endpointprovider failed with ErrorCode={0}. Exiting..",
                error);

            TryComplete(thisSPtr,error);
            return;
        }

        owner_.RegisterIpcRequestHandler();

        InitializeAllServices(thisSPtr);

        owner_.EnableChangeMonitoring();
    }
private:
    void InitializeAllServices(AsyncOperationSPtr const & thisSPtr)
    {
        wstring fileConfigEnvVariable = FabricEnvironment::FileConfigStoreEnvironmentVariable;
        wstring packageConfigEnvVariable = FabricEnvironment::PackageConfigStoreEnvironmentVariable;

        if(!::SetEnvironmentVariableW(fileConfigEnvVariable.c_str(), NULL))
        {
            Trace.WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to clear out Env variable {0}, proceeding with service activation",
                fileConfigEnvVariable);
        }
        if(!::SetEnvironmentVariableW(packageConfigEnvVariable.c_str(), NULL))
        {
            Trace.WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to clear out Env variable {0}, proceeding with service activation",
                fileConfigEnvVariable);
        }
        if(!::SetEnvironmentVariableW(
            Constants::FabricActivatorAddressEnvVariable.c_str(),
            owner_.fabricHost_.IpcServerObj.TransportListenAddress.c_str()))
        {
            Trace.WriteWarning(TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to setEnv variable {0} to communicate FabricHost IPC address",
                Constants::FabricActivatorAddressEnvVariable);
            TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
            return;
        }

        if (HostingConfig::GetConfig().DisableContainers)
        {
            Trace.WriteInfo(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Ignoring FabricCAS.exe activation since DisableContainers is set to {0}",
                HostingConfig::GetConfig().DisableContainers);

            StartHostedServices(thisSPtr);
        }
        else
        {
            StartContainerActivatorService(thisSPtr);
        }
    }

    void StartContainerActivatorService(AsyncOperationSPtr const & thisSPtr)
    {
        HostedServiceSPtr containerManagerSerivce;
        auto error = owner_.CreateContainerActivatorService(containerManagerSerivce);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        owner_.servicesMap_->Add(containerManagerSerivce);

        auto operation = containerManagerSerivce->BeginActivate(
            FabricHostConfig::GetConfig().StopTimeout,
            [this, &containerManagerSerivce](AsyncOperationSPtr const & operation)
            {
                OnContainerActivatorServiceStarted(operation, containerManagerSerivce, false);
            },
            thisSPtr);

        OnContainerActivatorServiceStarted(operation, containerManagerSerivce, true);
    }

    void OnContainerActivatorServiceStarted(
        AsyncOperationSPtr operation,
        HostedServiceSPtr const & containerManagerSerivce,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = containerManagerSerivce->EndActivate(operation);
        if (!error.IsSuccess())
        {
            Trace.WriteError(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Failed to start ContainerActivatorService. Error={0}.",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        Trace.WriteInfo(
            TraceType_ActivationManager,
            owner_.Root.TraceId,
            "ContainerActivatorService started successfully.");

        StartHostedServices(operation->Parent);
    }

    void StartHostedServices(AsyncOperationSPtr const & thisSPtr)
    { 

        if (FabricHostConfig::GetConfig().Test_IgnoreHostedServices)
        {
            Trace.WriteInfo(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "Ignoring HostedServices activation since IgnoreHostedServices is set to {0}",
                FabricHostConfig::GetConfig().Test_IgnoreHostedServices);
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        StringCollection sections;
        FabricHostConfig & config = FabricHostConfig::GetConfig();
        config.GetSections(sections, Constants::HostedServiceSectionName);

        for (auto iter = sections.begin(); iter != sections.end(); ++iter)
        {
            StringMap entries;
            config.GetKeyValues(*iter, entries);
            HostedServiceSPtr service = owner_.CreateHostedService(*iter, entries);
            if (service)
            {
                hostedServices.push_back(service);
            }
        }
        if (hostedServices.size() > 0)
        {
            ActivateHostedServices(thisSPtr);
        }
        else
        {
            CheckPendingOperations(thisSPtr, 0);
        }
    }

    void ActivateHostedServices(AsyncOperationSPtr const & thisSPtr)
    {
        pendingOperationCount_.store(hostedServices.size());
        for(auto iter = hostedServices.begin(); iter != hostedServices.end(); ++iter)
        {
            HostedServiceSPtr hostedService = *iter;
            owner_.servicesMap_->Add(hostedService);

            auto operation = hostedService->BeginActivate(
                FabricHostConfig::GetConfig().StopTimeout,
                [this, &hostedService](AsyncOperationSPtr const & operation)
            { 
                this->FinishActivate(operation, hostedService, false); 
            },
                thisSPtr);
            FinishActivate(operation, hostedService, true);
        }
    }

    void FinishActivate(AsyncOperationSPtr operation, 
        HostedServiceSPtr const & hostedService, 
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        ErrorCode error = hostedService->EndActivate(operation);
        if(!error.IsSuccess())
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
            lastError_.ReadValue();
            TryComplete(thisSPtr, lastError_);
        }
    }

private:
    vector<HostedServiceSPtr> hostedServices;
    TimeoutHelper timeoutHelper_;
    HostedServiceActivationManager & owner_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
};

class HostedServiceActivationManager::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        __in HostedServiceActivationManager & owner,
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
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
#if !defined(PLATFORM_UNIX)
        //
        // ContainerActivatorService (FabricCAS.exe) should be deactivated last
        // as FabricNode (fabric.exe) depends on it for container management.
        //
        auto error = owner_.servicesMap_->Get(
            Constants::FabricContainerActivatorServiceName,
            containerActivatorService_);
        if (error.IsSuccess())
        {
            owner_.servicesMap_->Remove(Constants::FabricContainerActivatorServiceName);
        }

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::SystemServiceNotFound))
        {
            lastError_.Overwrite(error);
        }
#endif
        DeactivateHostedServices(thisSPtr);
        FabricHostConfig::GetConfig().UnregisterForUpdates(&owner_);
    }

private:

    void DeactivateHostedServices(AsyncOperationSPtr const & thisSPtr)
    {
        vector<HostedServiceSPtr> hostedServices = owner_.servicesMap_->Close(); 
        Trace.WriteInfo(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Shutting down Hosted Services on FabricHost close. HostedServices count {0}",
            hostedServices.size());
        if(hostedServices.size() > 0)
        {
            pendingOperationCount_.store(hostedServices.size());
            for(auto iter = hostedServices.begin(); iter != hostedServices.end(); iter++)
            {
                HostedServiceSPtr hostedService = *iter;
                auto operation = hostedService->BeginDeactivate(
                    true,
                    FabricHostConfig::GetConfig().StopTimeout,
                    [this, hostedService](AsyncOperationSPtr const & operation)
                { 
                    this->FinishDeactivate(operation, hostedService, false); 
                },
                    thisSPtr);
                FinishDeactivate(operation, hostedService, true);
            }
        }
        else
        {
            CheckPendingOperations(thisSPtr, 0);
        }
    }

    void FinishDeactivate(AsyncOperationSPtr operation, 
        HostedServiceSPtr hostedService, 
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        ErrorCode error = hostedService->EndDeactivate(operation);
        if(!error.IsSuccess())
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
            owner_.UnregisterIpcRequestHandler();

            auto error = owner_.endpointProvider_->Close();
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Failed to close endpointprovider. Error {0}",
                    error);
                lastError_.Overwrite(error);
            }

            DeactivateContainerActivatorService(thisSPtr);
        }
    }

    void DeactivateContainerActivatorService(AsyncOperationSPtr const & thisSPtr)
    {
        if (containerActivatorService_ == nullptr)
        {
            TryComplete(thisSPtr, lastError_);
            return;
        }

        auto operation = containerActivatorService_->BeginDeactivate(
            true,
            FabricHostConfig::GetConfig().StopTimeout,
            [this](AsyncOperationSPtr const & operation)
            {
                OnContainerActivatorServiceDeactivated(operation, false);
            },
            thisSPtr);

        OnContainerActivatorServiceDeactivated(operation, true);
    }

    void OnContainerActivatorServiceDeactivated(
        AsyncOperationSPtr operation, 
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = containerActivatorService_->EndDeactivate(operation);
        if (!error.IsSuccess())
        {
            Trace.WriteError(
                TraceType_ActivationManager,
                owner_.Root.TraceId,
                "ContainerActivator Service close failed with error:{0}.",
                error);
            lastError_.Overwrite(error);
        }

        lastError_.ReadValue();
        TryComplete(operation->Parent, lastError_);
    }

private:
    TimeoutHelper timeoutHelper_;
    HostedServiceActivationManager & owner_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
    HostedServiceSPtr containerActivatorService_;
};

class HostedServiceActivationManager::StartHostedServiceAsyncOperation : public AsyncOperation
{
public:

    StartHostedServiceAsyncOperation(
        __in HostedServiceActivationManager& owner,
        __in HostedServiceSPtr const & hostedService,
        __in_opt IpcReceiverContextUPtr context,
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        hostedService_(hostedService),
        context_(move(context))
    {
    }

    static ErrorCode StartHostedServiceAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<StartHostedServiceAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        Trace.WriteNoise(TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Starting hosted service {0}",
            hostedService_->ServiceName);

        auto operation = hostedService_->BeginActivate(
            FabricHostConfig::GetConfig().StopTimeout,
            [this, thisSPtr](AsyncOperationSPtr const & operation)
            { 
                this->OnStartCompleted(operation, thisSPtr); 
            },
            thisSPtr);
    }

private:

    void OnStartCompleted(
        AsyncOperationSPtr const & operation,
        AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode e = hostedService_->EndActivate(operation);
        Trace.WriteTrace(e.ToLogLevel(),
            TraceType_ActivationManager,
            "Started Hosted service {0} returned {1}",
            hostedService_->ServiceName,
            e);

        if (! e.IsSuccess())
        {
            if(hostedService_->GetState() == HostedService::Aborted ||
                hostedService_->GetState() == HostedService::Failed)
            {
                Trace.WriteNoise(TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Hosted service {0} has been aborted or failed",
                    hostedService_->ServiceName);
            }
            else
            {
                Assert::CodingError("Invalid state for Hosted Service {0}", hostedService_->GetState());
            }
        }

        if (context_)
        {
            SendReply(e, context_);
        }
        TryComplete(thisSPtr, e);
    }

    HostedServiceActivationManager& owner_;
    HostedServiceSPtr hostedService_;
    IpcReceiverContextUPtr context_;
};

class HostedServiceActivationManager::UpdateHostedServiceAsyncOperation : public AsyncOperation
{
public:

    UpdateHostedServiceAsyncOperation(
        __in HostedServiceActivationManager& owner,
        __in HostedServiceSPtr const & hostedService,
        __in HostedServiceParameters const & params,
        __in IpcReceiverContextUPtr context,
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        hostedService_(hostedService),
        section_(),
        params_(params),
        context_(move(context)),
        retryCount_(0)
    {
    }

    UpdateHostedServiceAsyncOperation(
        __in HostedServiceActivationManager& owner,
        __in HostedServiceSPtr const & hostedService,
        __in wstring const & section,
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        hostedService_(hostedService),
        section_(section),
        params_(),
        context_(nullptr),
        retryCount_(0)
    {
    }

    static ErrorCode UpdateHostedServiceAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpdateHostedServiceAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        StartServiceUpdate(thisSPtr);
    }

private:

    void StartServiceUpdate(AsyncOperationSPtr const & thisSPtr)
    {
        Trace.WriteNoise(
            TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Updating hosted service {0}",
            hostedService_->ServiceName);

        if (! section_.empty())
        {
            auto operation = hostedService_->BeginUpdate(
                section_,
                FabricHostConfig::GetConfig().StopTimeout,
                [this, thisSPtr](AsyncOperationSPtr const & operation)
                { 
                    this->OnUpdateCompleted(operation, thisSPtr);  
                },
                thisSPtr);
        }
        else
        {
            auto operation = hostedService_->BeginUpdate(
                params_,
                FabricHostConfig::GetConfig().StopTimeout,
                [this, thisSPtr](AsyncOperationSPtr const & operation)
                { 
                    this->OnUpdateCompleted(operation, thisSPtr);  
                },
                thisSPtr);
        }
    }

    void OnUpdateCompleted(
        AsyncOperationSPtr const & operation,
        AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode e = hostedService_->EndUpdate(operation);

        Trace.WriteTrace(e.ToLogLevel(),
            TraceType_ActivationManager,
            "Update Hosted service {0} returned {1}, retryCount {2}",
            hostedService_->ServiceName,
            e,
            retryCount_);
        if(!e.IsSuccess() && hostedService_->CanScheduleUpdate())
        {
            //try rescheduling
            retryCount_++;
            if(retryCount_ < FabricHostConfig::GetConfig().ActivationMaxFailureCount)
            {
                int64 retryTicks = (uint64)(FabricHostConfig::GetConfig().ActivationRetryBackoffInterval.Ticks * retryCount_);
                if(retryTicks > FabricHostConfig::GetConfig().ActivationMaxRetryInterval.Ticks)
                {
                    retryTicks = FabricHostConfig::GetConfig().ActivationMaxRetryInterval.Ticks;
                }
                TimeSpan delay = TimeSpan::FromTicks(retryTicks);
                {
                    WriteInfo(
                        TraceType_ActivationManager,
                        owner_.Root.TraceId,
                        "Scheduling update for service {0}  to run after {1} ",
                        hostedService_->ServiceName,
                        delay);
                    AcquireExclusiveLock lock(hostedService_->timerLock_);
                    hostedService_->updateTimer_ = Timer::Create(
                        "Hosting.ServiceUpdate",
                        [this, thisSPtr](TimerSPtr const & timer)
                        {
                            timer->Cancel();
                            this->StartServiceUpdate(thisSPtr);
                        });
                    hostedService_->updateTimer_->Change(delay);
                }

                return;
            }
            else
            {
                WriteError(
                    TraceType_ActivationManager,
                    owner_.Root.TraceId,
                    "Updating hosted service {0} failed more than max retry count {1}. Exiting",
                    hostedService_->ServiceName,
                    retryCount_);
                ::ExitProcess(1);
            }
        }

        if (context_)
        {
            SendReply(e, context_);
        }
        TryComplete(thisSPtr, e);
    }

    HostedServiceActivationManager& owner_;
    HostedServiceSPtr hostedService_;
    wstring section_;
    HostedServiceParameters params_;
    IpcReceiverContextUPtr context_;
    int retryCount_;
};

class HostedServiceActivationManager::StopHostedServiceAsyncOperation : public AsyncOperation
{
public:

    StopHostedServiceAsyncOperation(
        __in HostedServiceActivationManager& owner,
        __in HostedServiceSPtr const & hostedService,
        __in bool stopGracefully,
        __in_opt IpcReceiverContextUPtr context,
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        hostedService_(hostedService),
        stopGracefully_(stopGracefully),
        context_(move(context))
    {
    }

    static ErrorCode StopHostedServiceAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<StopHostedServiceAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        Trace.WriteNoise(
            TraceType_ActivationManager,
            owner_.Root.TraceId,
            "Stopping hosted service {0}",
            hostedService_->ServiceName);

        auto operation = hostedService_->BeginDeactivate(
            stopGracefully_,
            FabricHostConfig::GetConfig().StopTimeout,
            [this, thisSPtr](AsyncOperationSPtr const & operation)
            {
                this->OnStopCompleted(operation);
            },
            thisSPtr);
    }

private:

    void OnStopCompleted(AsyncOperationSPtr const & operation)
    {
        ErrorCode e = hostedService_->EndDeactivate(operation);
        Trace.WriteTrace(e.ToLogLevel(),
            TraceType_ActivationManager,
            "Stop Hosted service {0} returned {1}",
            hostedService_->ServiceName,
            e);

        if(!e.IsSuccess())
        {
            hostedService_->AbortAndWaitForTermination();
        }

        if (context_)
        {
            SendReply(e, context_);
        }
        TryComplete(operation->Parent, e);
    }

    HostedServiceActivationManager& owner_;
    HostedServiceSPtr hostedService_;
    bool stopGracefully_;
    IpcReceiverContextUPtr context_;
};

HostedServiceActivationManager::HostedServiceActivationManager(
    Common::ComponentRoot const & root,
    FabricActivationManager const & fabricHost,
    bool hostedServiceActivateHidden,
    bool skipFabricSetup) : RootedObject(root)
    , hostedServiceActivateHidden_(hostedServiceActivateHidden)
    , fabricHost_(fabricHost)
    , endpointProvider_()
    , servicesMap_()
    , skipFabricSetup_(skipFabricSetup)
    , isSystem_(false)
{
    auto map = make_shared<HostedServiceMap>();
    servicesMap_ = move(map);

    auto endpointProvider = make_unique<HttpEndpointSecurityProvider>(root);
    endpointProvider_ = move(endpointProvider);
}

HostedServiceActivationManager::~HostedServiceActivationManager()
{
}

Common::AsyncOperationSPtr HostedServiceActivationManager::OnBeginOpen(
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

Common::ErrorCode HostedServiceActivationManager::OnEndOpen(Common::AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr HostedServiceActivationManager::OnBeginClose(
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);  
}

Common::ErrorCode HostedServiceActivationManager::OnEndClose(Common::AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void HostedServiceActivationManager::OnAbort()
{
    Trace.WriteInfo(TraceType_ActivationManager,
        Root.TraceId,
        "Aborting HostedServiceActivationManager");

    FabricHostConfig::GetConfig().UnregisterForUpdates(this);
    this->endpointProvider_->Abort();
    vector<HostedServiceSPtr> hostedServices = this->servicesMap_->Close();
    for(auto iter = hostedServices.begin(); iter != hostedServices.end(); iter++)
    {
        (*iter)->AbortAndWaitForTermination();
    }

    UnregisterIpcRequestHandler();
}

ErrorCode HostedServiceActivationManager::ValidateRunasParams(wstring AccountName, SecurityPrincipalAccountType::Enum AccountType, wstring Password)
{
    // Validate AccountType.
    SecurityPrincipalAccountType::Validate(AccountType);

    if(! AccountName.empty())
    {
        if(AccountType == SecurityPrincipalAccountType::DomainUser)
        {
            if(Password.empty())
            {
                Trace.WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Invalid Runas Account {0} of type specified {1}. Valid password is needed",
                    AccountName,
                    AccountType);
                return ErrorCode(ErrorCodeValue::InvalidCredentials);
            }
        }
    }
    else if(! Password.empty())
    {
        Trace.WriteWarning(TraceType_ActivationManager,
            Root.TraceId,
            "Invalid Runas Account {0} of type specified {1}. Password specification not required",
            AccountName,
            AccountType);
        return ErrorCode(ErrorCodeValue::InvalidCredentials);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void HostedServiceActivationManager::SendReply(
    Common::ErrorCode error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    FabricHostOperationReply replyBody(error);
    MessageUPtr reply = make_unique<Message>(replyBody);
    context->Reply(move(reply));
}

ErrorCode HostedServiceActivationManager::CreateSecurityUser(
    wstring const & applicationId,
    wstring const & serviceNodeName,
    bool runasSpecified,
    SecurityPrincipalAccountType::Enum runasAccountType,
    wstring const & runasAccountName,
    wstring const & runasPassword,
    bool runasPasswordEncrypted,
    __out SecurityUserSPtr & secUser)
{
    secUser = nullptr;
    if (!runasSpecified)
    {
        runasAccountType = SecurityPrincipalAccountType::NetworkService;
    }

    if (runasSpecified || this->isSystem_)
    {
        ErrorCode error;

        error = ValidateRunasParams(runasAccountName, runasAccountType, runasPassword);
        if (! error.IsSuccess())
        {
            return error;
        }

        // If runas account type is LocalSystem, we need FabricHost to run as system as well.
        if(runasAccountType == SecurityPrincipalAccountType::Enum::LocalSystem)
        {
            if(!this->isSystem_)
            {
                Trace.WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "HostedServices cannot be started as {0} since current process is not System",
                    runasAccountType);
                return ErrorCode(ErrorCodeValue::InvalidCredentialType);
            }
            else
            {
                return ErrorCode(ErrorCodeValue::Success);
            }
        }

        // HostedService like Fabric.exe needs to be added to FabricAdmins group.
        vector<wstring> runasGroupMembership;
        runasGroupMembership.push_back(FabricConstants::WindowsFabricAdministratorsGroupName);


        // Fabric.exe invokes Crypto APIs (e.g. CertCreateSelfSignCertificate).
        // For these Crypto APIs to work correctly, profile of the user Fabric.exe is
        // impersonating should be loaded. For example, when using user key containers,
        // CertCreateSelfSignCertificate will fail with 0x80070002 if the impersonated
        // user's profile is not loaded.
        bool loadProfile = IsFabricService(applicationId, serviceNodeName);

        secUser = SecurityUser::CreateSecurityUser(
            applicationId,
            runasAccountName,
            runasAccountName,
            runasPassword,
            runasPasswordEncrypted,
            loadProfile,
            false,
            NTLMAuthenticationPolicyDescription(),
            runasAccountType,
            runasGroupMembership);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode HostedServiceActivationManager::CreateSecurityUser(HostedServiceParameters const & params, __out SecurityUserSPtr & secUser)
{
    return CreateSecurityUser(
        params.ServiceName,
        params.ServiceNodeName,
        params.RunasSpecified,
        params.RunasAccountType,
        params.RunasAccountName,
        params.RunasPassword,
        params.RunasPasswordEncrypted,
        secUser);
}

ErrorCode HostedServiceActivationManager::CreateSecurityUser(wstring const & section, StringMap const & entries, __out SecurityUserSPtr & secUser)
{
    wstring runasAccountName;
    SecurityPrincipalAccountType::Enum runasAccountType = SecurityPrincipalAccountType::NetworkService;
    wstring runasPassword;
    bool runasPasswordEncrypted = false;
    bool runasSpecified = false;

    auto nameIter  = entries.find(Constants::RunasAccountName);
    if(nameIter != entries.end())
    {
        runasSpecified = true;
        runasAccountName = nameIter->second;

        auto passIter = entries.find(Constants::RunasPassword);
        if(passIter != entries.end())
        {
            runasPassword = passIter->second;
            runasPasswordEncrypted =  FabricHostConfig::GetConfig().IsConfigSettingEncrypted(section, passIter->first);
        }
    }

    auto typeIter = entries.find(Constants::RunasAccountType);
    if(typeIter != entries.end())
    {
        runasSpecified = true;
        ErrorCode error = SecurityPrincipalAccountType::FromString(typeIter->second, runasAccountType);
        if(!error.IsSuccess())
        {
            Trace.WriteWarning(TraceType_ActivationManager,
                Root.TraceId,
                "Invalid Runas AccountType {0} specified",
                typeIter->second);
            return ErrorCode(ErrorCodeValue::InvalidCredentials);
        }
    }

    wstring serviceNodeName;
    auto nodeIter = entries.find(Constants::HostedServiceParamServiceNodeName);
    if(nodeIter != entries.end())
    {
        serviceNodeName = nodeIter->second;
    }

    return CreateSecurityUser(
        section,
        serviceNodeName,
        runasSpecified,
        runasAccountType,
        runasAccountName,
        runasPassword,
        runasPasswordEncrypted,
        secUser);
}

void HostedServiceActivationManager::GetHostedServiceCertificateThumbprint(StringMap const & entries, __out wstring & thumbprint)
{
    auto iter = entries.find(Constants::HostedServiceSSLCertFindValue);
    if (iter == entries.end())
    {
        return;
    }
    thumbprint = iter->second;
}

ErrorCode HostedServiceActivationManager::CreateContainerActivatorService(
    __out HostedServiceSPtr & containerActivatorSerivce)
{
    wstring fabricCodePath;
    auto error = FabricEnvironment::GetFabricCodePath(fabricCodePath);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "CreateContainerActivatorService(): Failed get FabricCodePath. Error={0}.",
            error);

        return error;
    }

    wstring serviceNodeName(L"NodeNameDontExist");

    auto exePath = Path::Combine(fabricCodePath, Constants::FabricContainerActivatorServiceExeName);
    if (!File::Exists(exePath))
    {
        Trace.WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "FabricContainerActivatorService does not exist. FilePath={0}",
            exePath);

       ASSERT_IF(true, "FabricContainerActivatorService does not exist. FilePath={0}", exePath);
    }

    auto workingDir = Path::GetDirectoryName(exePath);

    HostedService::Create(
        HostedServiceActivationManagerHolder(*this, this->Root.CreateComponentRoot()),
        Constants::FabricContainerActivatorServiceName,
        exePath,
        L"",
        workingDir,
        L"",
        true,
        serviceNodeName,
        0,
        L"",
        SecurityUserSPtr(),
        L"",
        L"",
        X509FindType::FindByThumbprint,
        ServiceModel::ResourceGovernancePolicyDescription(),
        containerActivatorSerivce);

    return ErrorCode::Success();
}

HostedServiceSPtr HostedServiceActivationManager::CreateHostedService(wstring const & section, StringMap const & entries)
{
    HostedServiceSPtr hostedService=nullptr;
    wstring serviceName = section;
    wstring exeName;
    wstring args;
    wstring workingDir;
    wstring environment;
    wstring endpointProtocol;
    UINT port = 0;
    wstring sslCertFindValue;
    wstring sslCertStoreLocation;
    wstring serviceNodeName;
    X509FindType::Enum sslCertFindType = X509FindType::FindByThumbprint;
    ServiceModel::ResourceGovernancePolicyDescription rgPolicyDescription = ServiceModel::ResourceGovernancePolicyDescription();

    bool disabled = false;
    bool ctrlCSpecified = false;

    for(auto it = entries.begin(); it != entries.end(); it++)
    {    
        if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceParamExeName))
        {
            exeName = it->second;
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceParamArgs))
        {
            args = it->second;
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::WorkingDirectory))
        {
            workingDir = it->second;
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceParamCtrlCSenderPath))
        {
            ctrlCSpecified = true;
        }
        else if (StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceParamDisabled))
        {
            if(Config::TryParse<bool>(disabled, it->second) &&
                disabled)
            {
                break;
            }
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceEnvironmentParam))
        {
            environment = it->second;
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceProtocolParam))
        {
            endpointProtocol = it->second;
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServicePortParam))
        {
            if(!Config::TryParse<UINT>(port, it->second))
            {
                Trace.WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to parse port {0}, marking service {1} as disabled",
                    it->second,
                    section);
                disabled = true;
            }
        }
        else if (StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceCpusetCpus))
        {
            rgPolicyDescription.CpusetCpus = it->second;
        }
        else if (StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceCpuShares))
        {
            if (!Config::TryParse<UINT>(rgPolicyDescription.CpuShares, it->second))
            {
                Trace.WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to parse process CpuShares limit, marking service {1} as disabled",
                    it->second,
                    section);
                disabled = true;
            };
        }
        else if (StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceMemoryInMB))
        {
            if (!Config::TryParse<UINT>(rgPolicyDescription.MemoryInMB, it->second))
            {
                Trace.WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to parse process MemoryInMB limit, marking service {1} as disabled",
                    it->second,
                    section);
                disabled = true;
            };
        }
        else if (StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceMemorySwapInMB))
        {
            if (!Config::TryParse<UINT>(rgPolicyDescription.MemorySwapInMB, it->second))
            {
                Trace.WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to parse process MemorySwapInMB limit, marking service {1} as disabled",
                    it->second,
                    section);
                disabled = true;
            };
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceSSLCertStoreName))
        {
            sslCertStoreLocation = it->second;
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceSSLCertFindType))
        {
            auto error = Common::X509FindType::Parse(it->second, sslCertFindType);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to parse SSL certificate find type - {0}",
                    it->second);
            }
        }
        else if(StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceSSLCertFindValue))
        {
            sslCertFindValue = it->second;
        }
        else if (StringUtility::AreEqualCaseInsensitive(it->first, Constants::HostedServiceParamServiceNodeName))
        {
            serviceNodeName = it->second;
        }
    }
    if(!disabled && !exeName.empty())
    {
        if (!File::Exists(exeName))
        {
            Trace.WriteWarning(TraceType_ActivationManager,
                Root.TraceId,
                "Failed to confirm file exists {0} for Hosted Service - {1}",
                exeName,
                serviceName);
            ::ExitProcess(0);
        }
        if(workingDir.empty())
        {
            workingDir = Path::GetDirectoryName(exeName);
        }

        SecurityUserSPtr secPrincipal;
        auto error = CreateSecurityUser(section, entries, secPrincipal);

        if(error.IsSuccess())
        {
            HostedService::Create(HostedServiceActivationManagerHolder(*this, 
                this->Root.CreateComponentRoot()), 
                serviceName,
                exeName, 
                args,
                workingDir,
                environment,
                ctrlCSpecified,
                serviceNodeName,
                port,
                endpointProtocol,
                secPrincipal,
                sslCertFindValue,
                sslCertStoreLocation,
                sslCertFindType,
                rgPolicyDescription,
                hostedService);  
        }
    }
    else
    {
        if(disabled)
        {
            Trace.WriteNoise(TraceType_ActivationManager,
                Root.TraceId,
                "Hosted service {0} is disabled, skip adding it to service map",
                serviceName);
        }
        else
        {
            Trace.WriteWarning(TraceType_ActivationManager,
                Root.TraceId,
                "Hosted service {0} specified with no exeName, skip adding it to service map",
                section);
        }
    }
    return hostedService;
}

HostedServiceSPtr HostedServiceActivationManager::CreateHostedService(HostedServiceParameters const & params)
{
    HostedServiceSPtr hostedService = nullptr;
    SecurityUserSPtr secUser = nullptr;
    
    if (! params.ExeName.empty())
    {
        ErrorCode error = CreateSecurityUser(params, secUser);

        if (error.IsSuccess())
        {
            // Create resource governance policy
            ServiceModel::ResourceGovernancePolicyDescription rgPolicyDescription = ServiceModel::ResourceGovernancePolicyDescription();
            rgPolicyDescription.CpusetCpus = params.CpusetCpus;
            rgPolicyDescription.CpuShares = params.CpuShares;
            rgPolicyDescription.MemoryInMB = params.MemoryInMB;
            rgPolicyDescription.MemorySwapInMB = params.MemorySwapInMB;

            HostedService::Create(
                HostedServiceActivationManagerHolder(*this, this->Root.CreateComponentRoot()),
                params.ServiceName,
                params.ExeName,
                params.Arguments,
                params.WorkingDirectory,
                params.Environment,
                params.CtrlCSpecified,
                params.ServiceNodeName,
                params.Port,
                params.EndpointProtocol,
                secUser,
                params.SslCertificateFindValue,
                params.SslCertificateStoreLocation,
                params.SslCertificateFindType,
                rgPolicyDescription,
                hostedService
                );
        }
        else
        {
            Trace.WriteWarning(TraceType_ActivationManager,
                Root.TraceId,
                "Hosted service {0} specified with no exeName, skip adding it to service map",
                params.ServiceName);
        }
    }
    
    return hostedService;
}

void HostedServiceActivationManager::EnableChangeMonitoring()
{
    Trace.WriteInfo(TraceType_ActivationManager,
        Root.TraceId,
        "Enabling Change monitoring for section {0}",
        Constants::HostedServiceSectionName);
    FabricHostConfig::GetConfig().RegisterForUpdates(Constants::HostedServiceSectionName, this);
}

void HostedServiceActivationManager::OnConfigChange(wstring const & section, wstring const & key)
{
    Trace.WriteNoise(
        TraceType_ActivationManager,
        Root.TraceId,
        "Config file change notification received for section {0} key {1}",
        section,
        key);

    if (FabricHostConfig::GetConfig().Test_IgnoreHostedServices)
    {
        Trace.WriteNoise(
            TraceType_ActivationManager,
            Root.TraceId,
            "Config file change notification received for section {0} key {1} is ignored as Test_IgnoreHostedServices is set.",
            section,
            key);
        return;
    }

    HostedServiceSPtr hostedService = nullptr;
    SecurityUserSPtr secUser;
    StringMap entries;
    bool isDisabled = false;
    bool removeService = false;
    bool shouldUpdate = false;
    HostedServiceSPtr newService = nullptr;;
    ErrorCode error;
    wstring thumbprint;
    wstring arguments;

    FabricHostConfig::GetConfig().GetKeyValues(section, entries);

    if (! entries.empty())
    {
        //Check if service is disabled now
        auto it = entries.find(Constants::HostedServiceParamDisabled);
        if (it != entries.end())
        {
            Config::TryParse<bool>(isDisabled, it->second);
        }

        error = CreateSecurityUser(section, entries, secUser);
        if (! error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Invalid credentials specified for  HostedService {0}. Error {1}",
                section,
                error);
            return;
        }

        GetHostedServiceCertificateThumbprint(entries, thumbprint);

        it = entries.find(Constants::HostedServiceParamArgs);
        if (it != entries.end())
        {
            arguments = it->second;
        }

    }
    {
        AcquireWriteLock lock(lock_);
        error = servicesMap_->Get(section, hostedService);
        if (error.IsError(ErrorCodeValue::ObjectClosed))
        {
            Trace.WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Addition of new service {0} failed due to {1}",
                section,
                error);
            return;
        }
        if (hostedService &&
            (entries.empty() ||
            isDisabled))
        {
            ErrorCode err = servicesMap_->Remove(hostedService->ServiceName);
            if (!err.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to remove HostedService {0} from map, due to error {1}",
                    hostedService->ServiceName,
                    err);
                return;
            }
            removeService = true;
        }
        else if (! hostedService)
        {
            if (! entries.empty())
            {
                //New service added to settings. Start and add it to cache.
                Trace.WriteInfo(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "New section added to config file for hosted service {0}",
                    section);
                newService = CreateHostedService(section, entries);
                if (newService)
                {
                    error = servicesMap_->Add(newService);
                    if (!error.IsSuccess())
                    {
                        Trace.WriteWarning(
                            TraceType_ActivationManager,
                            Root.TraceId,
                            "Addition of new service {0} failed due to {1}",
                            section,
                            error);
                        return;
                    }
                }
                else
                {
                    Trace.WriteInfo(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Skipping activation of service {0} as CreateHostedService returned null (due to service being disabled or bad params)",
                        section);
                }
            }
            else
            {
                Trace.WriteNoise(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Ignoring notification on section {0} and key {1} since entries are empty",
                    section,
                    key);
            }
        }
        else
        {
            auto iter = entries.find(Constants::HostedServiceParamExeName);
            if (iter != entries.end())
            {
                if (!StringUtility::AreEqualCaseInsensitive(iter->second, hostedService->GetProcessDescription().ExePath))
                {
                    Trace.WriteInfo(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "ExePath has changed from {0} to {1} for HostedService {2}, terminating FabricHost to pickup updated changes",
                        hostedService->GetProcessDescription().ExePath, iter->second, hostedService->ServiceName);
                    ::ExitProcess(0);
                }
            }

            shouldUpdate = hostedService->IsUpdateNeeded(secUser, thumbprint, arguments) || hostedService->IsRGPolicyUpdateNeeded(entries);
        }
    }

    if (removeService)
    {
        Trace.WriteInfo(
            TraceType_ActivationManager,
            Root.TraceId,
            "Hosted service {0} config changed. Stopping service with gracefulstop = {1}",
            hostedService->ServiceName,
            !isDisabled);

        StopHostedService(hostedService, !isDisabled);
    }
    else if (newService)
    {
        StartHostedService(newService);
    }
    else if (shouldUpdate)
    {
        UpdateHostedService(hostedService, section);
    }

    return;
}

void HostedServiceActivationManager::OnHostedServiceTerminated(
    ErrorCode const & waitResult,
    DWORD exitCode,
    std::wstring const & serviceName,
    std::wstring const & serviceId)
{
    UNREFERENCED_PARAMETER(waitResult);

    LONG state = this->State.Value;
    if (state > FabricComponentState::Opened)
    {
        WriteNoise(
            TraceType_ActivationManager,
            Root.TraceId,
            "Ignoring termination of HostedService: ServiceName={0} as current state is {1}",
            serviceName,
            state);
        return;
    }

    HostedServiceSPtr hostedService = nullptr;
    auto error = this->servicesMap_->Get(serviceName, hostedService);
    WriteInfo(
        TraceType_ActivationManager,
        Root.TraceId,
        "Retrieving Hosted Service from map: ErrorCode={0}, ServiceName={1}",
        error,
        serviceName);

    if (error.IsSuccess())
    {
        this->NotifyHostedServiceTermination(serviceName);

        vector<wstring> childServiceNames;
        hostedService->ClearChildServices(childServiceNames);
        if (! childServiceNames.empty())
        {
            HostedServiceSPtr childHostedService;

            for (auto it = childServiceNames.begin(); it != childServiceNames.end(); it++)
            {

                auto childError = this->servicesMap_->Get(*it, childHostedService);
                if (childError.IsSuccess())
                {
                    childError = this->servicesMap_->Remove(*it);
                    if (childError.IsSuccess())
                    {
                        childHostedService->TerminateServiceInstance();

                        WriteInfo(
                            TraceType_ActivationManager,
                            Root.TraceId,
                            "Terminating child hosted service {0} of service {1}",
                            *it,
                            serviceName);
                    }
                    else
                    {
                        WriteWarning(
                            TraceType_ActivationManager,
                            Root.TraceId,
                            "Failed to remove child hosted service {0} of service {1} from hosted service map due to {2}",
                            *it,
                            serviceName,
                            childError);
                    }
                }
                else
                {
                    WriteInfo(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Failed to retreive child hosted service {0} of service {1} due to {2}",
                        *it,
                        serviceName,
                        childError);
                }
            }
        }
    }

    std::wstring exeName;
    if (error.IsSuccess())
    {
        const std::wstring & exePath = hostedService->GetProcessDescription().ExePath;
        exeName = Path::GetFileName(exePath);
    }

    if(exitCode == 0 || 
       exitCode == ProcessActivator::ProcessDeactivateExitCode || 
       exitCode == ERROR_CONTROL_C_EXIT ||
       exitCode == STATUS_CONTROL_C_EXIT)
    {
        hostingTrace.HostedServiceTerminated(serviceName, exitCode);
    }
    else
    {
        if (hostedService != nullptr)
        {
            Federation::NodeId serviceNodeID;
            ErrorCode generateNodeIdResult = Federation::NodeIdGenerator::GenerateFromString(hostedService->ServiceNodeName, serviceNodeID);
            if (generateNodeIdResult.IsSuccess())
            {
                hostingTrace.HostedServiceUnexpectedTermination(serviceNodeID.ToString(), hostedService->ServiceNodeName, exitCode, exeName);
            }
            else
            {
                WriteWarning(TraceType_ActivationManager,
                    Root.TraceId,
                    "Error generating NodeId from Node name {0} with error code {1}",
                    hostedService->ServiceNodeName,
                    generateNodeIdResult.ErrorCodeValueToString());
            }
        }
        else
        {
            WriteWarning(TraceType_ActivationManager,
                Root.TraceId,
                "Hosted service terminated with ServiceName {0} and ErrorCode {1}",
                serviceName,
                exitCode);
        }
    }

    // We only proceed further if we could find the hosted service from
    // the map. Otherwise, we're done here.
    if (!error.IsSuccess()) { return; }

    if(!StringUtility::AreEqualCaseInsensitive(hostedService->ServiceId, serviceId))
    {
        WriteWarning(TraceType_ActivationManager,
            Root.TraceId,
            "Hosted service {0} Id does not match expected {1}",
            serviceId,
            hostedService->ServiceId);
        return;
    }
    bool canReschedule = hostedService->RescheduleServiceActivation(exitCode);
    if(!canReschedule)
    {
        hostingTrace.HostedServiceActivationLimitExceeded(exeName, serviceName);
        ExitProcess(0);
    }
}

void HostedServiceActivationManager::NotifyHostedServiceTermination(wstring const & hostedServiceName)
{
    if (StringUtility::AreEqualCaseInsensitive(
        hostedServiceName, 
        Constants::FabricContainerActivatorServiceName))
    {
        fabricHost_.ProcessActivationManagerObj->OnContainerActivatorServiceTerminated();
    }
}

void HostedServiceActivationManager::RegisterIpcRequestHandler()
{
    auto root = this->Root.CreateComponentRoot();
    this->fabricHost_.IpcServerObj.RegisterMessageHandler(
        Actor::HostedServiceActivator,
        [this, root](MessageUPtr & message, IpcReceiverContextUPtr & context) { this->ProcessIpcMessage(*message, context); },
        false/*dispatchOnTransportThread*/);
}

void HostedServiceActivationManager::UnregisterIpcRequestHandler()
{
    this->fabricHost_.IpcServerObj.UnregisterMessageHandler(Actor::HostedServiceActivator);
}

void HostedServiceActivationManager::ProcessIpcMessage(
    __in Message & message, 
    __in IpcReceiverContextUPtr & context)
{
    wstring const & action = message.Action;
    WriteNoise(
        TraceType_ActivationManager,
        Root.TraceId,
        "Processing Ipc message with action {0}",
        action);

    if (action == Hosting2::Protocol::Actions::ActivateHostedServiceRequest)
    {
        this->ProcessActivateHostedServiceRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::DeactivateHostedServiceRequest)
    {
        this->ProcessDeactivateHostedServiceRequest(message, context);
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

void HostedServiceActivationManager::ProcessActivateHostedServiceRequest(
    __in Message & message, 
    __in IpcReceiverContextUPtr & context)
{
    ActivateHostedServiceRequest request;
    ErrorCode error;

    if (! message.GetBody(request))
    {
        error = ErrorCodeValue::InvalidMessage;
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);

        SendReply(error, context);
        return;
    }

    HostedServiceSPtr hostedService;
    HostedServiceSPtr newService;
    SecurityUserSPtr secUser;
    bool shouldUpdate = false;
    HostedServiceParameters const & params = request.Parameters;

    {
        AcquireWriteLock lock(lock_);

        if (! params.ParentServiceName.empty())
        {
            HostedServiceSPtr parentHostedService;
            error = servicesMap_->Get(params.ParentServiceName, parentHostedService);

            if (! error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to retrieve parent service {0} of service {1} due to {2}",
                    params.ParentServiceName,
                    params.ServiceName,
                    error);

                SendReply(error, context);
                return;
            }

            error = parentHostedService->AddChildService(params.ServiceName);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to retrieve parent service {0} of service {1} due to {2}",
                    params.ParentServiceName,
                    params.ServiceName,
                    error);

                SendReply(error, context);
                return;
            }
        }

        error = servicesMap_->Get(params.ServiceName, hostedService);
        if (error.IsError(ErrorCodeValue::ObjectClosed))
        {
            Trace.WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Addition of new service {0} failed due to {1}",
                params.ServiceName,
                error);

            SendReply(error, context);
            return;
        }

        if (! hostedService)
        {
            Trace.WriteInfo(
                TraceType_ActivationManager,
                Root.TraceId,
                "Activate params is for a new hosted service {0}",
                params.ServiceName);

            newService = CreateHostedService(params);
            if (newService)
            {
                error = servicesMap_->Add(newService);
                if (! error.IsSuccess())
                {
                    Trace.WriteWarning(
                        TraceType_ActivationManager,
                        Root.TraceId,
                        "Addition of new service {0} failed due to {1}",
                        params.ServiceName,
                        error);

                    SendReply(error, context);
                    return;
                }
            }
            else
            {
                Trace.WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Skipping activation of service {0} as CreateHostedService returned null (due to exename missing or bad secuser settings).  Exename: {1}",
                    params.ServiceName,
                    params.ExeName);

                SendReply(ErrorCodeValue::InvalidArgument, context);
                return;
            }
        }
        else
        {
            error = CreateSecurityUser(params, secUser);
            if (! error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Invalid credentials specified for HostedService {0}. Error {1}",
                    params.ServiceName,
                    error);

                SendReply(error, context);
                return;
            }

            shouldUpdate = hostedService->IsUpdateNeeded(secUser, params.SslCertificateFindValue, params.Arguments) || hostedService->IsRGPolicyUpdateNeeded(params);
        }
    }
    
    if (newService)
    {
        StartHostedService(newService, move(context));
    }
    else if (shouldUpdate)
    {
        UpdateHostedService(hostedService, params, move(context));
    }

    return;
}

void HostedServiceActivationManager::ProcessDeactivateHostedServiceRequest(
    __in Message & message, 
    __in IpcReceiverContextUPtr & context)
{
    DeactivateHostedServiceRequest request;
    ErrorCode error;

    if (!message.GetBody(request))
    {
        error = ErrorCodeValue::InvalidMessage;
        WriteWarning(
            TraceType_ActivationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);

        SendReply(error, context);
        return;
    }

    HostedServiceSPtr hostedService = nullptr;
    {
        AcquireWriteLock lock(lock_);

        error = servicesMap_->Get(request.ServiceName, hostedService);
        if (! error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceType_ActivationManager,
                Root.TraceId,
                "Failed to retrieve HostedService {0} from map, due to error {1}",
                request.ServiceName,
                error);
            
            SendReply(error, context);
            return;
        }

        if (hostedService)
        {
            error = servicesMap_->Remove(hostedService->ServiceName);
            if (! error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceType_ActivationManager,
                    Root.TraceId,
                    "Failed to remove HostedService {0} from map, due to error {1}",
                    hostedService->ServiceName,
                    error);
                
                SendReply(error, context);
                return;
            }
        }
        else
        {
            Trace.WriteNoise(
                TraceType_ActivationManager,
                Root.TraceId,
                "Removal of HostedService {0} unnecessary: not found in map.",
                request.ServiceName);

            SendReply(ErrorCodeValue::Success, context);
            return;
        }
    }

    //  If the service was successfully removed from the map, stop it
    StopHostedService(hostedService, request.Graceful, move(context));
}

void HostedServiceActivationManager::StartHostedService(
    __in HostedServiceSPtr const & hostedService,
    __in_opt IpcReceiverContextUPtr context)
{
    auto op = AsyncOperation::CreateAndStart<StartHostedServiceAsyncOperation>(
        *this,
        hostedService,
        move(context),
        [this](AsyncOperationSPtr const & operation)
        { 
            this->FinishStartHostedService(operation, false); 
        },
        Root.CreateAsyncOperationRoot());

    FinishStartHostedService(op, true);
}

void HostedServiceActivationManager::FinishStartHostedService(AsyncOperationSPtr op, bool expectedCompletedSynchronously)
{
    if (op->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StartHostedServiceAsyncOperation::End(op);

    WriteTrace(
        error.ToLogLevel(),
        TraceType_ActivationManager,
        "End(StartHostedService): ErrorCode={0}",
        error
        );
}

void HostedServiceActivationManager::UpdateHostedService(
    __in HostedServiceSPtr const & hostedService,
    __in HostedServiceParameters const & params,
    __in IpcReceiverContextUPtr context)
{
    auto op =  AsyncOperation::CreateAndStart<UpdateHostedServiceAsyncOperation>(
        *this,
        hostedService,
        params,
        move(context),
        [this](AsyncOperationSPtr const & operation)
        {
            this->FinishUpdateHostedService(operation, false);
        },
        Root.CreateAsyncOperationRoot());

    FinishUpdateHostedService(op, true);
}

void HostedServiceActivationManager::UpdateHostedService(
    __in HostedServiceSPtr const & hostedService,
    __in wstring const & section)
{
    auto op =  AsyncOperation::CreateAndStart<UpdateHostedServiceAsyncOperation>(
        *this,
        hostedService,
        section,
        [this](AsyncOperationSPtr const & operation)
        {
            this->FinishUpdateHostedService(operation, false);
        },
        Root.CreateAsyncOperationRoot());

    FinishUpdateHostedService(op, true);
}

void HostedServiceActivationManager::FinishUpdateHostedService(AsyncOperationSPtr op, bool expectedCompletedSynchronously)
{
    if (op->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = UpdateHostedServiceAsyncOperation::End(op);

    WriteTrace(
        error.ToLogLevel(),
        TraceType_ActivationManager,
        "End(UpdateHostedService): ErrorCode={0}",
        error
        );
}

void HostedServiceActivationManager::StopHostedService(
    __in HostedServiceSPtr const & hostedService,
    __in bool stopGracefully,
    __in_opt IpcReceiverContextUPtr context)
{
    auto op =  AsyncOperation::CreateAndStart<StopHostedServiceAsyncOperation>(
        *this,
        hostedService,
        stopGracefully,
        move(context),
        [this](AsyncOperationSPtr const & operation)
        { 
            this->FinishStopHostedService(operation, false); 
        },
        Root.CreateAsyncOperationRoot());

    FinishStopHostedService(op, true);
}

void HostedServiceActivationManager::FinishStopHostedService(AsyncOperationSPtr op, bool expectedCompletedSynchronously)
{
    if (op->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StopHostedServiceAsyncOperation::End(op);

    WriteTrace(
        error.ToLogLevel(),
        TraceType_ActivationManager,
        "End(StopHostedService): ErrorCode={0}",
        error
        );
}

bool HostedServiceActivationManager::IsFabricService(const std::wstring & hostedServiceName, const std::wstring & serviceNodeName)
{
    const std::wstring hostedFabricServiceName = Constants::HostedServiceSectionName + serviceNodeName + L"_" + Constants::FabricServiceName;
    return StringUtility::AreEqualCaseInsensitive(hostedServiceName, hostedFabricServiceName);
}
