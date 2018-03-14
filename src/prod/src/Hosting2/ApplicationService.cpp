// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Management::ResourceMonitor;
#if !defined(PLATFORM_UNIX)
using namespace HttpCommon;
#endif

StringLiteral const TraceType_ActivationManager("ApplicationService");

class ApplicationService::MeasureResourceUsageAsyncOperation :
    public AsyncOperation,
    Common::TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(MeasureResourceUsageAsyncOperation)

public:
    MeasureResourceUsageAsyncOperation(
        __in ApplicationService & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner)
    {
    }

    virtual ~MeasureResourceUsageAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, ResourceMeasurement & resourceMeasurement)
    {
        auto thisPtr = AsyncOperation::End<MeasureResourceUsageAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            resourceMeasurement = move(thisPtr->resourceMeasurement_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.IsContainerHost)
        {
            auto operation = owner_.ActivationManager.containerActivator_->BeginInvokeContainerApi(
                owner_.ContainerDescriptionObj.ContainerName,
                L"GET",
                L"/containers/{id}/stats?stream=false",
                L"application/json",
                L"",
                HostingConfig::GetConfig().ContainerStatsTimeout,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnContainerApiStatsCompleted(operation, false);
            },
                thisSPtr);
            this->OnContainerApiStatsCompleted(operation, true);
        }
        else
        {
            auto error = MeasureProcessCpuUsage();
            if (error.IsSuccess())
            {
                error = MeasureProcessMemoryUsage();
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        owner_.parentId_,
                        "Application Service with service Id {0} MeasureProcessMemoryUsage error {1}",
                        owner_.appServiceId_,
                        error);
                }
            }
            else
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.parentId_,
                    "Application Service with service Id {0} MeasureProcessCpuUsage error {1}",
                    owner_.appServiceId_,
                    error);
            }
            TryComplete(thisSPtr, error);
            return;
        }
    }

    ErrorCode MeasureProcessCpuUsage()
    {
#if !defined(PLATFORM_UNIX)
        auto activationContext = static_pointer_cast<ProcessActivationContext>(owner_.activationContext_);

        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION info = { 0 };
        bool success = QueryInformationJobObject(
            activationContext->JobHandle->Value,
            JobObjectBasicAccountingInformation,
            &info,
            sizeof(info),
            NULL
        );
        if (!success)
        {
            return ErrorCode::FromWin32Error(::GetLastError());
        }
        resourceMeasurement_.TimeRead = Common::DateTime::Now();
        resourceMeasurement_.TotalCpuTime = info.TotalKernelTime.QuadPart + info.TotalUserTime.QuadPart;
        return ErrorCodeValue::Success;
#else
        // Linux is currently not supported for processes
        return ErrorCodeValue::NotImplemented;
#endif

    }

    ErrorCode MeasureProcessMemoryUsage()
    {
#if !defined(PLATFORM_UNIX)
        auto activationContext = static_pointer_cast<ProcessActivationContext>(owner_.activationContext_);

        PROCESS_MEMORY_COUNTERS memoryCounters = { 0 };
        if (GetProcessMemoryInfo(activationContext->ProcessHandle->Value, &memoryCounters, sizeof(memoryCounters)) != 0)
        {
            resourceMeasurement_.MemoryUsage = memoryCounters.PagefileUsage;
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCode::FromWin32Error(::GetLastError());
        }
#else
        // Linux is currently not supported for processes
        return ErrorCodeValue::NotImplemented;
#endif
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
        auto error = owner_.ActivationManager.containerActivator_->EndInvokeContainerApi(operation, result);

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
                        resourceMeasurement_.MemoryUsage = containerStatsResponse.MemoryStats_.CommitBytes_;
                        resourceMeasurement_.TotalCpuTime = containerStatsResponse.CpuStats_.CpuUsage_.TotalUsage_;
                        resourceMeasurement_.TimeRead = containerStatsResponse.Read_;
                    }
                    else
                    {
                        WriteWarning(
                            TraceType_ActivationManager,
                            owner_.parentId_,
                            "Application Service with service Id {0} ContainerStatsResponse error {1}",
                            owner_.appServiceId_,
                            error);
                    }
                }
                else
                {
                    WriteWarning(
                        TraceType_ActivationManager,
                        owner_.parentId_,
                        "Application Service with service Id {0} ContainerApiResult status {1}",
                        owner_.appServiceId_,
                        containerApiResult.Status());
                    error = ErrorCode(ErrorCodeValue::InvalidOperation);
                }
            }
            else
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.parentId_,
                    "Application Service with service Id {0} ContainerApiResponse error {1}",
                    owner_.appServiceId_,
                    error);
            }
            TryComplete(operation->Parent, error);
            return;
        }

    }

private:
    ApplicationService & owner_;
    ResourceMeasurement resourceMeasurement_;
};

class ApplicationService::ActivateAsyncOperation : 
    public AsyncOperation,
    Common::TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        __in ApplicationService & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        processId_(0)
    {
    }

    virtual ~ActivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, DWORD & processId)
    {
        auto thisPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);
        if(thisPtr->Error.IsSuccess())
        {
            processId = thisPtr->processId_;
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToStarting();
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.parentId_,
                "Application Service with service Id {0} failed to start. ErrorCode={1}",
                owner_.appServiceId_,
                error);

            TryComplete(thisSPtr, error);
        }
        else
        {
            if (owner_.isContainerHost_)
            {
                auto operation = owner_.ActivationManager.ContainerActivatorObj->BeginActivate(
                    owner_.IsSecurityUserLocalSystem(),
                    owner_.ApplicationServiceId,
                    owner_.parentId_,
                    owner_.containerDescription_,
                    owner_.processDescription_,
                    owner_.fabricBinFolder_,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation)
                { this->FinishActivateContainer(operation, false); },
                thisSPtr);
                FinishActivateContainer(operation, true);
            }
            else
            {
                auto operation = owner_.ActivationManager.ProcessActivatorObj->BeginActivate(owner_.securityUser_,
                    owner_.processDescription_,
                    owner_.fabricBinFolder_,
                    GetProcessTerminationHandler(DateTime::Now()),
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation)
                { this->OnProcessActivated(operation); },
                thisSPtr);
                if (operation->CompletedSynchronously)
                {
                    FinishActivateProcess(operation);
                }
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
        auto error = owner_.ActivationManager.ProcessActivatorObj->EndActivate(operation, owner_.activationContext_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivationManager,
            owner_.parentId_,
            "End(Service Activate): ErrorCode={0}, Service Id ={1}, ExeName {2}",
            error,
            owner_.appServiceId_,
            Path::GetFileName(owner_.processDescription_.ExePath));

        ErrorCode transitionResult = this->TransitionState(error);

        if(!transitionResult.IsSuccess())
        {
            error = transitionResult;
        }
        else
        {
            if(error.IsSuccess())
            {
                WriteInfo(
                    TraceType_ActivationManager,
                    owner_.parentId_,
                    "application Service {0} was activated with process id {1}, ExeName {2}",
                    owner_.appServiceId_,
                    processId_,
                    Path::GetFileName(owner_.processDescription_.ExePath));
            }
        }
        TryComplete(operation->Parent, error);   
    }

    void FinishActivateContainer(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.ActivationManager.ContainerActivatorObj->EndActivate(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_ActivationManager,
            owner_.parentId_,
            "End(Container Activate): ErrorCode={0}, ErrorMessage = {1}, Service Id ={2}, ExeName {3}",
            error,
            error.Message,
            owner_.appServiceId_,
            owner_.processDescription_.ExePath);

        ErrorCode transitionResult = this->TransitionState(error);

        if (!transitionResult.IsSuccess())
        {
            error = transitionResult;
        }
        else
        {
            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceType_ActivationManager,
                    owner_.parentId_,
                    "Container Service {0} was activated. ImageName={1}, ContainerName={2}.",
                    owner_.appServiceId_,
                    Path::GetFileName(owner_.processDescription_.ExePath),
                    owner_.containerDescription_.ContainerName);
                
                if (error.IsSuccess())
                {

                    hostingTrace.ContainerActivatedOperational(
                        owner_.TraceId,
                        owner_.ContainerDescriptionObj.ContainerName,
                        ServiceModel::ContainerIsolationMode::EnumToString(owner_.ContainerDescriptionObj.IsolationMode),
                        owner_.ContainerDescriptionObj.ApplicationName,
                        owner_.ContainerDescriptionObj.ServiceName);

                    hostingTrace.ContainerActivated(
                        owner_.TraceId,
                        owner_.ContainerDescriptionObj.ContainerName,
                        ServiceModel::ContainerIsolationMode::EnumToString(owner_.ContainerDescriptionObj.IsolationMode),
                        owner_.ContainerDescriptionObj.ApplicationName,
                        owner_.ContainerDescriptionObj.ServiceName);
                }
               
            }
        }
        TryComplete(operation->Parent, error);
    }

    ErrorCode TransitionState(ErrorCode startedResult)
    {
        ErrorCode transitionResult;
        if(startedResult.IsSuccess())
        {
            if (!owner_.isContainerHost_)
            {
                //try to get processId before transitioning to running
                processId_ = owner_.activationContext_->ProcessId;
            }
            transitionResult = owner_.TransitionToStarted();
        }
        else
        {
            transitionResult = owner_.TransitionToFailed();
        }

        if(!transitionResult.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.parentId_,
                "application Service {0} has failed to transition to the right state while starting. ErrorCode: {1}",
                owner_.appServiceId_,
                transitionResult);
        }

        return transitionResult;
    }

    ProcessWait::WaitCallback GetProcessTerminationHandler(DateTime startTime)
    {
        ProcessActivationManager & controller  = owner_.ActivationManager;
        wstring serviceId  = owner_.ApplicationServiceId;
        wstring parentId = owner_.ParentId;
        wstring exePath = owner_.processDescription_.ExePath;
        DateTime exeStartTime = startTime;
        auto root = owner_.ActivationManager.Root.CreateComponentRoot();

        return ProcessWait::WaitCallback(
            [root, &controller, serviceId, parentId, exePath, exeStartTime](DWORD processId, ErrorCode const & waitResult, DWORD processExitCode)
        {
            controller.OnApplicationServiceTerminated(processId, exeStartTime, waitResult, processExitCode, parentId, serviceId, exePath);
        });
    }

private:
    ApplicationService & owner_;
    TimeoutHelper timeoutHelper_;
    DWORD processId_;
};

class ApplicationService::DeactivateAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    DeactivateAsyncOperation(   
        ApplicationService & applicationService,
        bool isGraceful,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        isGraceful_(isGraceful),
        timeoutHelper_(timeout),        
        owner_(applicationService)
    {
    }

    static ErrorCode DeactivateAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {  
        if(owner_.GetState() == owner_.Stopped)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        ErrorCode transitionResult = owner_.TransitionToStopping();
        if(transitionResult.IsSuccess())
        {                    
            this->DoStop(thisSPtr);
        }
        else
        {            
            WriteWarning(
                TraceType_ActivationManager,
                owner_.ParentId,
                "ApplicationService failed to stop. ErrorCode: {0}. {1}",                
                transitionResult,
                owner_.appServiceId_);

            TransitionToAborted(thisSPtr);
        }
    }

private:
    void DoStop(AsyncOperationSPtr const & thisSPtr)
    {   
        if (owner_.isContainerHost_)
        {
            WriteInfo(
                TraceType_ActivationManager,
                owner_.TraceId,
                "Removing container {0} as part of deactivation",
                owner_.containerDescription_.ContainerName);
            auto activationContext = static_pointer_cast<ProcessActivationContext>(owner_.activationContext_);
            if(activationContext != nullptr &&
               activationContext->ProcessMonitor != nullptr)
            {  
                activationContext->ProcessMonitor->Cancel();
            }
            owner_.activationContext_.reset();
            auto operation = owner_.ActivationManager.ContainerActivatorObj->BeginDeactivate(
                owner_.containerDescription_.ContainerName,
                owner_.containerDescription_.AutoRemove,
                owner_.containerDescription_.IsContainerRoot,
                owner_.processDescription_.CgroupName,
                isGraceful_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ this->FinishStopCompleted(operation, false); },
                thisSPtr);

            FinishStopCompleted(operation, true);
        }
        else
        {
            if (!owner_.activationContext_)
            {
                WriteInfo(
                    TraceType_ActivationManager,
                    owner_.TraceId,
                    "Skipping stopping since service {0} is not activated",
                    owner_.appServiceId_);
                TransitionToStopped(thisSPtr);
            }
            else
            {
                WriteNoise(
                    TraceType_ActivationManager,
                    owner_.TraceId,
                    "Stop service begin: service id ={0}",
                    owner_.appServiceId_);

                auto operation = owner_.ActivationManager.ProcessActivatorObj->BeginDeactivate(
                    owner_.activationContext_,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation){ this->FinishStopCompleted(operation, false); },
                    thisSPtr);

                FinishStopCompleted(operation, true);
            }
        }
    }

    void FinishStopCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {            
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        ErrorCode errorCode = ErrorCodeValue::Success;
        if (owner_.isContainerHost_)
        {
            errorCode = owner_.ActivationManager.ContainerActivatorObj->EndDeactivate(operation);
            if (errorCode.IsSuccess())
            {
           
                hostingTrace.ContainerDeactivatedOperational(
                    owner_.TraceId,
                    owner_.ContainerDescriptionObj.ContainerName,
                    owner_.ContainerDescriptionObj.ApplicationName,
                    owner_.ContainerDescriptionObj.ServiceName);
                    
                hostingTrace.ContainerDeactivated(
                    owner_.TraceId,
                    owner_.ContainerDescriptionObj.ContainerName,
                    owner_.ContainerDescriptionObj.ApplicationName,
                    owner_.ContainerDescriptionObj.ServiceName);
            }
        }
        else
        {
            errorCode = owner_.ActivationManager.ProcessActivatorObj->EndDeactivate(operation);
        }
        WriteTrace(
            errorCode.ToLogLevel(),
            TraceType_ActivationManager,
            owner_.TraceId,
            "End(DeactivateApplicationService): ErrorCode={0}, service Id={1}",
            errorCode,
            owner_.appServiceId_);

        if (errorCode.IsSuccess())
        {  
            TransitionToStopped(operation->Parent);
        }
        else
        {
            TransitionToFailed(operation->Parent);
        }

    }

    void TransitionToStopped(AsyncOperationSPtr const & thisSPtr)
    {
        auto transitionResult = owner_.TransitionToStopped();
        if (transitionResult.IsSuccess())
        {
            WriteInfo(
                TraceType_ActivationManager,
                owner_.TraceId,
                "ApplicationService deactivated appService {0} exeName {1}",
                owner_.appServiceId_,
                Path::GetFileName(owner_.processDescription_.ExePath));
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        else
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.TraceId,
                "Application Service: {0} has failed to transition to Stopped state. ErrorCode: {1}.",
                owner_.appServiceId_,
                transitionResult);

            TransitionToFailed(thisSPtr);
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.TransitionToFailed().ReadValue();
        TransitionToAborted(thisSPtr);
    }

    void TransitionToAborted(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.AbortAndWaitForTerminationAsync(
            [this](AsyncOperationSPtr const & operation)
        {
            this->TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
        },
            thisSPtr);
        return;
    }

private:
    bool isGraceful_;
    TimeoutHelper timeoutHelper_;
    ApplicationService & owner_;   
};

ApplicationService::ApplicationService(
    ProcessActivationManagerHolder const & activatorHolder,
    wstring const & parentId,
    std::wstring const & appServiceId,
    ProcessDescription const & processDescription,
    wstring const & fabricBinFolder,
    SecurityUserSPtr const & runAs,
    BOOL isContainerHost,
    ContainerDescription const & containerDescription)
    : ComponentRoot(),
    StateMachine(Inactive),
    activatorHolder_(activatorHolder),
    parentId_(parentId),
    appServiceId_(appServiceId),
    processDescription_(processDescription),
    securityUser_(runAs),
    isContainerHost_(isContainerHost),
    containerDescription_(containerDescription),
    fabricBinFolder_(fabricBinFolder),
    rwlock_()
{
    this->SetTraceId(parentId);
    WriteNoise(
        TraceType_ActivationManager, 
        TraceId, 
        "ApplicationService.constructor: {0} ({1})", 
        static_cast<void*>(this),
        appServiceId_);
}

ApplicationService::~ApplicationService()
{
    WriteNoise(
        TraceType_ActivationManager, 
        TraceId, 
        "ApplicationService.destructor: {0} ({1})", 
        static_cast<void*>(this),
        appServiceId_);
}

AsyncOperationSPtr ApplicationService::BeginActivate(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);

    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);          
}

ErrorCode ApplicationService::EndActivate(
    AsyncOperationSPtr const & operation,
    DWORD & processId)
{
    return ActivateAsyncOperation::End(operation, processId);
}

AsyncOperationSPtr ApplicationService::BeginDeactivate(
    bool isGraceful,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        isGraceful,
        timeout, 
        callback, 
        parent);
}

ErrorCode ApplicationService::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    return DeactivateAsyncOperation::End(operation);
}

AsyncOperationSPtr ApplicationService::BeginMeasureResourceUsage(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<MeasureResourceUsageAsyncOperation>(
        *this,
        callback,
        parent);
}

ErrorCode ApplicationService::EndMeasureResourceUsage(
    AsyncOperationSPtr const & operation,
    ResourceMeasurement & resourceMeasurement)
{
    return MeasureResourceUsageAsyncOperation::End(operation, resourceMeasurement);
}

void ApplicationService::OnAbort()
{
    if (isContainerHost_)
    {
        ActivationManager.ContainerActivatorObj->TerminateContainer(
            containerDescription_.ContainerName,
            containerDescription_.IsContainerRoot,
            processDescription_.CgroupName);
    }
    else
    {
        IProcessActivationContextSPtr activationContext;
        {
            AcquireWriteLock(this->Lock);
            if (this->activationContext_)
            {
                activationContext = move(this->activationContext_);
            }
        }
        if (activationContext)
        {
            WriteInfo(
                TraceType_ActivationManager,
                TraceId,
                "Aborting ApplicationService {0}, with process id {1}",
                appServiceId_,
                activationContext->ProcessId);
            activatorHolder_.RootedObject.ProcessActivatorObj->Terminate(activationContext, ProcessActivator::ProcessAbortExitCode);
        }
        else
        {
            WriteInfo(
                TraceType_ActivationManager,
                TraceId,
                "Cannot abort ApplicationService {0}, since activationcontext is not present",
                appServiceId_);
        }
    }
}

ProcessActivatorUPtr const & ApplicationService::GetProcessActivator() const
{ 
    return this->activatorHolder_.RootedObject.ProcessActivatorObj; 
}

Common::ErrorCode ApplicationService::GetProcessId(__out DWORD & processId)
{
    ErrorCode error(ErrorCodeValue::InvalidArgument);
    {
        AcquireReadLock lock(rwlock_);
        if(this->activationContext_)
        {
            processId = this->activationContext_->ProcessId;
            error = ErrorCode(ErrorCodeValue::Success);
        }
    }
    return error;
}

bool ApplicationService::IsApplicationHostProcess(DWORD processId)
{
    DWORD procId;
    auto error = GetProcessId(procId);
    if (!error.IsSuccess())
    {
        return false;
    }
    return procId == processId;
}

void ApplicationService::ResumeProcessIfNeeded()
{
    if (this->activationContext_)
    {
        auto err = this->activationContext_->ResumeProcess();
        WriteWarning(
            TraceType_ActivationManager,
            TraceId,
            "Failed to resume process for ApplicationService {0}, error {1}",
            appServiceId_,
            err);
    }
}

bool ApplicationService::IsSecurityUserLocalSystem()
{
    return (
        securityUser_ != nullptr &&
        securityUser_->AccountType == SecurityPrincipalAccountType::LocalSystem);
}

bool ApplicationService::Test_VerifyLimitsEnforced(std::vector<double> & cpuLimitCP, std::vector<uint64> & memoryLimitCP)
{
    AcquireWriteLock lock(rwlock_);
    uint64 cpuLimit = 0;
    uint64 memoryLimit = 0;

    if (isContainerHost_)
    {
        ContainerInspectResponse containerInspect;
        AsyncOperationWaiterSPtr operationWaiter = make_shared<AsyncOperationWaiter>();
        this->activatorHolder_.RootedObject.ContainerActivatorObj->BeginQuery(
            containerDescription_,
            HostingConfig::GetConfig().ActivationTimeout,
            [&](AsyncOperationSPtr const & operation)
        {
            auto error = this->activatorHolder_.RootedObject.ContainerActivatorObj->EndQuery(operation, containerInspect);
            operationWaiter->SetError(error);
            operationWaiter->Set();
        },
            this->activatorHolder_.Root->CreateAsyncOperationRoot()
            );
        ErrorCode error;
        if (operationWaiter->WaitOne(HostingConfig::GetConfig().ActivationTimeout))
        {
            error = operationWaiter->GetError();
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType_ActivationManager,
                    TraceId,
                    "Failed to query docker for {0}, error {1}",
                    appServiceId_,
                    error);
            }
        }
        else
        {
            error = ErrorCode(ErrorCodeValue::Timeout);
            WriteError(
                TraceType_ActivationManager,
                TraceId,
                "Failed to query docker for {0}, error {1}",
                appServiceId_,
                error);

        }

        if (error.IsSuccess())
        {
            cpuLimit = containerInspect.HostConfig.NanoCpus;
            memoryLimit = containerInspect.HostConfig.Memory;

            if (this->ProcessDescriptionObj.ResourceGovernancePolicy.NanoCpus != cpuLimit)
            {
                WriteError(
                    TraceType_ActivationManager,
                    TraceId,
                    "Mismatch of cpu quota for {0}, found {1}, expected {2}",
                    appServiceId_,
                    cpuLimit,
                    this->processDescription_.ResourceGovernancePolicy.NanoCpus);
                return false;
            }

            if (static_cast<uint64>(this->processDescription_.ResourceGovernancePolicy.MemoryInMB) * 1024 * 1024 != memoryLimit)
            {
                WriteError(
                    TraceType_ActivationManager,
                    TraceId,
                    "Mismatch of memory limit for {0}, found {1}, expected {2}",
                    appServiceId_,
                    memoryLimit,
                    static_cast<uint64>(this->processDescription_.ResourceGovernancePolicy.MemoryInMB) * 1024 * 1024);
                return false;
            }

        }
        else
        {
            return false;
        }
    }
    else
    {
        if (this->activationContext_)
        {
#if !defined(PLATFORM_UNIX)
            auto error = GetProcessActivator()->Test_QueryJobObject(*(this->activationContext_), cpuLimit, memoryLimit);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType_ActivationManager,
                    TraceId,
                    "Failed to get job object info for {0}, error {1}",
                    appServiceId_,
                    error);
                return false;
            }
#else
            auto & cgroupName = dynamic_cast<ProcessActivationContext const &>(*(this->activationContext_)).CgroupName;

            if (!cgroupName.empty())
            {
                std::string cgroupNameUtf8;
                StringUtility::Utf16ToUtf8(cgroupName, cgroupNameUtf8);

                auto error = GetProcessActivator()->Test_QueryCgroupInfo(cgroupNameUtf8, cpuLimit, memoryLimit);
                if (error != 0)
                {
                    WriteError(
                        TraceType_ActivationManager,
                        TraceId,
                        "Failed to get cgroup info for {0}, error {1}",
                        appServiceId_,
                        error);
                    return false;
                }
            }
#endif
#if !defined(PLATFORM_UNIX)
            if (this->processDescription_.ResourceGovernancePolicy.CpuShares != cpuLimit)
            {
                WriteError(
                    TraceType_ActivationManager,
                    TraceId,
                    "Mismatch of cpu rate for {0}, found {1}, expected {2}",
                    appServiceId_,
                    cpuLimit,
                    this->processDescription_.ResourceGovernancePolicy.CpuShares);
                return false;
            }
#else
            if (this->processDescription_.ResourceGovernancePolicy.CpuQuota != cpuLimit)
            {
                WriteError(
                    TraceType_ActivationManager,
                    TraceId,
                    "Mismatch of cpu quota for {0}, found {1}, expected {2}",
                    appServiceId_,
                    cpuLimit,
                    this->processDescription_.ResourceGovernancePolicy.CpuQuota);
                return false;
            }
#endif

            if (static_cast<uint64>(this->processDescription_.ResourceGovernancePolicy.MemoryInMB) * 1024 * 1024 != memoryLimit)
            {
                WriteError(
                    TraceType_ActivationManager,
                    TraceId,
                    "Mismatch of memory limit for {0}, found {1}, expected {2}",
                    appServiceId_,
                    memoryLimit,
                    static_cast<uint64>(this->processDescription_.ResourceGovernancePolicy.MemoryInMB) * 1024 * 1024);
                return false;
            }
        }
    }

    if (cpuLimit > 0)
    {
        if (isContainerHost_)
        {
            cpuLimitCP.push_back((double)cpuLimit / Constants::DockerNanoCpuMultiplier);
        }
        else
        {
#if !defined(PLATFORM_UNIX)
            cpuLimitCP.push_back(((double)cpuLimit / Constants::JobObjectCpuCyclesNumber) * Environment::GetNumberOfProcessors());
#else
            cpuLimitCP.push_back((double)cpuLimit / Constants::CgroupsCpuPeriod);
#endif
        }
    }

    if (memoryLimit > 0)
    {
        memoryLimitCP.push_back(memoryLimit);
    }
    WriteNoise(
        TraceType_ActivationManager,
        TraceId,
        "Verified limits enforced for {0} with {1} isContainer {2}",
        appServiceId_,
        processDescription_,
        isContainerHost_);

    return true;
}

bool ApplicationService::Test_VerifyContainerGroupSettings()
{
    ContainerInspectResponse containerInspect;
    AsyncOperationWaiterSPtr operationWaiter = make_shared<AsyncOperationWaiter>();
    this->activatorHolder_.RootedObject.ContainerActivatorObj->BeginQuery(
        containerDescription_,
        HostingConfig::GetConfig().ActivationTimeout,
        [&](AsyncOperationSPtr const & operation)
    {
        auto error = this->activatorHolder_.RootedObject.ContainerActivatorObj->EndQuery(operation, containerInspect);
        operationWaiter->SetError(error);
        operationWaiter->Set();
    },
        this->activatorHolder_.Root->CreateAsyncOperationRoot()
        );
    ErrorCode error;
    if (operationWaiter->WaitOne(HostingConfig::GetConfig().ActivationTimeout))
    {
        error = operationWaiter->GetError();
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_ActivationManager,
                TraceId,
                "Failed to query docker for {0}, error {1}",
                appServiceId_,
                error);
        }
    }
    else
    {
        error = ErrorCode(ErrorCodeValue::Timeout);
        WriteError(
            TraceType_ActivationManager,
            TraceId,
            "Failed to query docker for {0}, error {1}",
            appServiceId_,
            error);
    }

    
    bool success = true;

    if (error.IsSuccess())
    {
#if defined(PLATFORM_UNIX)
        //all of the namespaces should be the same
        //by sharing the namespace we enable the containers to do IPC, have predicatble PIDs etc
        //chech the logic in ContainerConfig.cpp for more details
        auto namespaceId = ContainerHostConfig::GetContainerNamespace(containerDescription_.GroupContainerName);
        WriteInfo(TraceType_ActivationManager,
         TraceId,
         "Container group info IPC:{0}, Userns:{1}, Pid:{2}, UTS:{3}, CgroupParent:{4}",
         containerInspect.HostConfig.IpcMode,
         containerInspect.HostConfig.UsernsMode,
         containerInspect.HostConfig.PidMode,
         containerInspect.HostConfig.UTSMode,
         containerInspect.HostConfig.CgroupName);

        success = success && containerInspect.HostConfig.IpcMode == namespaceId;
        success = success && containerInspect.HostConfig.UsernsMode == namespaceId;
        success = success && containerInspect.HostConfig.PidMode == namespaceId;
        success = success && containerInspect.HostConfig.UTSMode == namespaceId;

        //we put all the containers in a cgroup so that we can govern them together
        success = success && !containerInspect.HostConfig.CgroupName.empty() && containerInspect.HostConfig.CgroupName == processDescription_.CgroupName;
        if (success)
        {
            std::string cgroupNameUtf8;
            StringUtility::Utf16ToUtf8(containerInspect.HostConfig.CgroupName, cgroupNameUtf8);
            uint64 cpuLimit = 0;
            uint64 memoryLimit = 0;

            auto error = GetProcessActivator()->Test_QueryCgroupInfo(cgroupNameUtf8, cpuLimit, memoryLimit);
            if (error != 0)
            {
                WriteError(
                    TraceType_ActivationManager,
                    TraceId,
                    "Failed to get cgroup info for {0}, error {1}",
                    appServiceId_,
                    error);
                success = false;
            }
            else
            {
                WriteInfo(
                    TraceType_ActivationManager,
                    TraceId,
                    "Cgroup info for {0}, cpuActual:{1},memoryActual:{2}, cpuExpected:{3}, memoryExpected:{4}",
                    appServiceId_,
                    cpuLimit,
                    memoryLimit,
                    processDescription_.ServicePackageResourceGovernance.CpuCores * Constants::CgroupsCpuPeriod,
                    processDescription_.ServicePackageResourceGovernance.MemoryInMB * 1024 * 1024);

                success = success && (cpuLimit == (uint64)(processDescription_.ServicePackageResourceGovernance.CpuCores * Constants::CgroupsCpuPeriod));
                success = success && (memoryLimit == (uint64)(processDescription_.ServicePackageResourceGovernance.MemoryInMB) * 1024 * 1024);
            }
        }
#endif
        return success;
    }
    else
    {
        return false;
    }
}
