// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("DockerProcessManager");

class DockerProcessManager::InitializeAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(InitializeAsyncOperation)

public:
    InitializeAsyncOperation(
        __in DockerProcessManager & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
        , failureRetryCount_(0)
        , maxRetryCount_(HostingConfig::GetConfig().DockerProcessManagerMaxRetryCount)
        , retryDelay_(HostingConfig::GetConfig().DockerProcessManagerRetryIntervalInSec)
    {
    }

    virtual ~InitializeAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation, 
        IProcessActivationContextSPtr & activationContext)
    {
        auto thisPtr = AsyncOperation::End<InitializeAsyncOperation>(operation);
        if (thisPtr->activationContext_ != nullptr)
        {
            activationContext = move(thisPtr->activationContext_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        //
        // We need to always check for service presence to start/skip
        // listening to docker events.
        //
        auto error = owner_.CheckDockerServicePresence();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "CheckDockerServicePresence() failed with Error={0}.",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        if (HostingConfig::GetConfig().SkipDockerProcessManagement == true || IsWinServerOrLinux() == false)
        {
            owner_.isDockerServiceManaged_ = false;

            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "Skipping docker process management.",
                error);

            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

        InitializeDockerProcess(thisSPtr);
    }

private:
    void InitializeDockerProcess(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.ActivationManager.State.Value > FabricComponentState::Opened)
        {
            //
            // FabricHost is closing down
            //
            TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }

        //
        // If docker is already running make this No Op
        //
        if (owner_.GetState() == owner_.Initialized)
        {
            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

        auto transitionResult = owner_.TransitionToInitializing();
        if (transitionResult.IsSuccess())
        {
            this->StopDockerNtService(thisSPtr);
            return;
        }
        
        if (owner_.GetState() == owner_.Initialized)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        CheckMaxRetryCount();

        WriteWarning(
            TraceType,
            owner_.Root.TraceId,
            "InitializeDockerProcess: Failed to transition to initializing state. ErrorCode={0}. RetryCount={1}.",
            transitionResult,
            failureRetryCount_);

        Threadpool::Post(
            [this, thisSPtr]() { InitializeDockerProcess(thisSPtr); },
            TimeSpan::FromSeconds(retryDelay_));
    }

    void StopDockerNtService(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.ActivationManager.State.Value > FabricComponentState::Opened)
        {
            //
            // FabricHost is closing down
            //
            TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }

        auto errorCode = owner_.ShutdownDockerService();
        if (errorCode.IsSuccess())
        {
            StartDockerProcess(thisSPtr);
            return;
        }
        else if (errorCode.IsError(ErrorCodeValue::ServiceNotFound))
        {
            TryComplete(thisSPtr, errorCode);
            return;
        }

        CheckMaxRetryCount();

        WriteWarning(
            TraceType,
            owner_.Root.TraceId,
            "StopDockerNtService: ErrorCode={0}, FailureRetryCount={1}. Retrying...",
            errorCode,
            failureRetryCount_);

        Threadpool::Post(
            [this, thisSPtr]() { StopDockerNtService(thisSPtr); },
            TimeSpan::FromSeconds(retryDelay_));
    }

    void StartDockerProcess(AsyncOperationSPtr const & thisSPtr)
    {
        wstring workingDir;
        auto error = FabricEnvironment::GetFabricDataRoot(workingDir);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        auto dockerpidFile = Path::Combine(workingDir, L"sfdocker.pid");
        if (File::Exists(dockerpidFile))
        {
            File::Delete(dockerpidFile, false);
        }

        EnvironmentMap environmentMap;
        wstring args;
        wstring dockerProcessName;

#if !defined(PLATFORM_UNIX)
        auto addressArg = HostingConfig::GetConfig().ContainerServiceAddressArgument;
        if (addressArg.empty())
        {
            addressArg = wformatString(
                "-H localhost:{0} -H npipe://", 
                HostingConfig::GetConfig().ContainerHostPort);
        }

        args = wformatString("{0} --pidfile {1}", addressArg, dockerpidFile);
        dockerProcessName = HostingConfig::GetConfig().ContainerProviderProcessName;
#else
        dockerProcessName = wformatString("/usr/bin/{0}",HostingConfig::GetConfig().ContainerProviderProcessName);
        args = wformatString(
            "-H localhost:{0} -H unix:///var/run/docker.sock --pidfile {1}", 
            HostingConfig::GetConfig().ContainerHostPort,
            dockerpidFile);

        wstring pathVar;
        Environment::GetEnvironmentVariable(L"PATH", pathVar, L"/usr/bin:/usr/sbin:/sbin");
        environmentMap[L"PATH"] = pathVar;
#endif

        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Starting docker process: ProcessName={0}, Args={1}.",
            dockerProcessName,
            args);

        ProcessDescription procDesc(
            dockerProcessName,
            args,
            workingDir,
            environmentMap,
            workingDir,
            workingDir,
            workingDir,
            workingDir,
            false,
            false,
            false,
            false);

        auto operation = owner_.processActivationManager_.ProcessActivatorObj->BeginActivate(
            nullptr,
            procDesc,
            workingDir,
            GetProcessTerminationHandler(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishStartDockerProcess(operation, false);
            },
            thisSPtr);

        FinishStartDockerProcess(operation, true);
    }

    void FinishStartDockerProcess(
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.processActivationManager_.ProcessActivatorObj->EndActivate(
            operation,
            activationContext_);

        if (!error.IsSuccess())
        {
            CheckMaxRetryCount();

            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishStartDockerProcess: ErrorCode={0}, FailureRetryCount={1}. Retrying...",
                error,
                failureRetryCount_);

            auto thisSPtr = operation->Parent;

            Threadpool::Post(
                [this, thisSPtr]() { StartDockerProcess(thisSPtr); },
                TimeSpan::FromSeconds(retryDelay_));

            return;
        }

        error = this->TransitionToStarted();
        TryComplete(operation->Parent, error);
        return;
    }

    ProcessWait::WaitCallback GetProcessTerminationHandler()
    {
        DockerProcessManager & controller = owner_;
        auto root = owner_.Root.CreateComponentRoot();

        return ProcessWait::WaitCallback(
            [root, &controller](DWORD, ErrorCode const & waitResult, DWORD processExitCode)
            {
                controller.OnProcessTerminated(processExitCode, waitResult);
            });
    }

    ErrorCode TransitionToStarted()
    {
        auto transitionResult = owner_.TransitionToInitialized();
        if (transitionResult.IsSuccess())
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "Docker process has started. {0}",
                activationContext_->ProcessId);
        }
        else
        {
            WriteError(
                TraceType,
                owner_.Root.TraceId,
                "Docker process failed to start. transitionResult={0}",
                transitionResult);

            owner_.TransitionToFailed();
        }

        return transitionResult;
    }

    void CheckMaxRetryCount()
    {
        if (++failureRetryCount_ > maxRetryCount_)
        {
            StringLiteral msgFormat("DockerProcessManager failed to start docker process. RetryCount={0}. Asserting for self-repair.");
            
            WriteError(
                TraceType,
                owner_.Root.TraceId,
                msgFormat,
                failureRetryCount_);
            
            Assert::CodingError(msgFormat, failureRetryCount_);
        }
    }

    bool IsWinServerOrLinux()
    {
#if !defined(PLATFORM_UNIX)
        return IsWindowsServer();
#else
        return true;
#endif
    }

private:
    SC_HANDLE scmHandle_;
    SC_HANDLE serviceHandle_;
    IProcessActivationContextSPtr activationContext_;
    DockerProcessManager & owner_;
    TimeoutHelper timeoutHelper_;
    int failureRetryCount_;
    int maxRetryCount_;
    int retryDelay_;
};

class DockerProcessManager::DeactivateAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    DeactivateAsyncOperation(        
        DockerProcessManager & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        timeoutHelper_(timeout),        
        owner_(codePackage)        
    {
    }

    static ErrorCode DeactivateAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {       
        if (HostingConfig::GetConfig().SkipDockerProcessManagement == true)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        if(owner_.GetState() == owner_.Deactivated)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        ErrorCode transitionResult = owner_.TransitionToDeactivating();
        if(transitionResult.IsSuccess())
        {                    
            this->DoDeactivate(thisSPtr);
        }
        else
        {            
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "DockerProcessManager failed to deactivate. ErrorCode: {0}",                
                transitionResult);

            TransitionToAborted(thisSPtr, transitionResult);
        }
    }

private:
    void DoDeactivate(AsyncOperationSPtr const & thisSPtr)
    {    

        activationContext_ = move(owner_.activationContext_);
        if (activationContext_ == nullptr)
        {
            TransitionToDeactivated(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {

            auto operation = owner_.processActivationManager_.ProcessActivatorObj->BeginDeactivate(
                activationContext_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->FinishDeactivation(operation, false); },
                thisSPtr);

            FinishDeactivation(operation, true);
        }
    }

    void FinishDeactivation(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {                
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }        

        auto error = owner_.processActivationManager_.ProcessActivatorObj->EndDeactivate(operation);

        TransitionToDeactivated(operation->Parent, error);
    }

    void TransitionToDeactivated(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        if (error.IsSuccess())
        {
            auto err = owner_.TransitionToDeactivated();
            if (!err.IsSuccess())
            {
                TransitionToFailed(thisSPtr);
                TryComplete(thisSPtr, err);
                return;
            }
            else
            {
                TryComplete(thisSPtr, ErrorCodeValue::Success);
                return;
            }
        }
        else
        {
            TransitionToAborted(thisSPtr, error);
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.TransitionToFailed().ReadValue();
        TransitionToAborted(thisSPtr, ErrorCodeValue::InvalidState);
    }

    void TransitionToAborted(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        if (activationContext_ != nullptr)
        {
            owner_.processActivationManager_.ProcessActivatorObj->Terminate(activationContext_, ProcessActivator::ProcessAbortExitCode);
        }
        TryComplete(thisSPtr, error);
    }

private:
    IProcessActivationContextSPtr activationContext_;
    TimeoutHelper timeoutHelper_;
    DockerProcessManager & owner_;    
};

FabricActivationManager const & DockerProcessManager::get_ActivationManager() const
{ 
    return processActivationManager_.ActivationManager; 
}

DockerProcessManager::DockerProcessManager(
    ComponentRoot const & root,
    ContainerEngineTerminationCallback const & callback,
    ProcessActivationManager & processActivationManager)
    : RootedObject(root),
    StateMachine(Inactive),
    callback_(callback),
    processActivationManager_(processActivationManager),
    activationContext_(),
    lock_(),
    isDockerServicePresent_(true),
    isDockerServiceManaged_(true)
{
}

DockerProcessManager::~DockerProcessManager()
{
    WriteNoise(
        TraceType, 
        Root.TraceId, 
        "DockerProcessManager.destructor");
}

AsyncOperationSPtr DockerProcessManager::BeginInitialize(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<InitializeAsyncOperation>(
        *this,
        timeout,
        callback, 
        parent);
}

ErrorCode DockerProcessManager::EndInitialize(AsyncOperationSPtr const & operation)
{
    return InitializeAsyncOperation::End(operation, activationContext_);
}

AsyncOperationSPtr DockerProcessManager::BeginDeactivate(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this, 
        timeout, 
        callback, 
        parent);
}

ErrorCode DockerProcessManager::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    return DeactivateAsyncOperation::End(operation);
}

void DockerProcessManager::OnAbort()
{
    IProcessActivationContextSPtr activationContext;
    {
        AcquireExclusiveLock lock(lock_);
        activationContext = move(activationContext_);
    }
    if (activationContext != nullptr)
    {
        processActivationManager_.ProcessActivatorObj->Terminate(activationContext, ProcessActivator::ProcessAbortExitCode);
    }
}

void DockerProcessManager::OnProcessTerminated(DWORD exitCode, ErrorCode waitResult)
{
    WriteWarning(
        TraceType,
        Root.TraceId,
        "DockerProcessManager terminated with ExitCode={0}, Wait ErrorCode={1}",
        exitCode, 
        waitResult);

    auto err = TransitionToTerminating();
    if (!err.IsSuccess())
    {
        uint64 currentState = this->GetState();
        if (currentState != Deactivating &&
            currentState != Deactivated &&
            currentState != Terminated)
        {
            Assert::CodingError("Invalid state docker process manager {0}", currentState);
        }
    }

    callback_();

    err = TransitionToTerminated();
    if (!err.IsSuccess())
    {
        WriteError(
            TraceType,
            Root.TraceId,
            "DockerProcessManager failed to transition to terminated state. Error={0}, CurrentState={1}.", 
            err,
            GetState());

        return;
    }

    this->BeginInitialize(
        HostingConfig::GetConfig().ActivationTimeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->EndInitialize(operation);
        },
        this->Root.CreateAsyncOperationRoot());
}

ErrorCode DockerProcessManager::ShutdownDockerService()
{
    ErrorCode errorCode(ErrorCodeValue::Success);
#if !defined(PLATFORM_UNIX)
    bool serviceStopped = false;

    auto scmHandle = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    SC_HANDLE serviceHandle = NULL;
    if (scmHandle != NULL)
    {
        auto serviceName = HostingConfig::GetConfig().ContainerProviderServiceName;
        serviceHandle = ::OpenService(scmHandle, serviceName.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (serviceHandle != NULL)
        {
            SERVICE_STATUS status;
            if (::QueryServiceStatus(serviceHandle, &status))
            {
                if (status.dwCurrentState == SERVICE_STOPPED)
                {
                    serviceStopped = true;
                }
                else
                {
                    WriteInfo(
                        TraceType,
                        Root.TraceId,
                        "Container provider service {0} is in {1} state, stopping now",
                        serviceName,
                        status.dwCurrentState);
                    if (!::ControlService(serviceHandle, SERVICE_CONTROL_STOP, &status))
                    {
                        errorCode = ErrorCode::FromWin32Error();
                        WriteWarning(
                            TraceType,
                            Root.TraceId,
                            "Failed to issue stop control command to Container provider service {0}, error {1}",
                            serviceName,
                            errorCode);
                    }
                    else if (::QueryServiceStatus(serviceHandle, &status) &&
                        status.dwCurrentState == SERVICE_STOPPED)
                    {
                        serviceStopped = true;
                    }
                }
            }
            else
            {
                errorCode = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceType,
                    Root.TraceId,
                    "Failed to query service state, error {0}",
                    errorCode);
            }
        }
        else
        {
            errorCode = ErrorCode::FromWin32Error();
            WriteInfo(
                TraceType,
                Root.TraceId,
                "Failed to open service error {0}",
                errorCode);

            if (errorCode.IsWin32Error(ERROR_SERVICE_DOES_NOT_EXIST))
            {
                errorCode = ErrorCode(
                    ErrorCodeValue::ServiceNotFound,
                    StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));
            }
        }
    }
    else
    {
        errorCode = ErrorCode::FromWin32Error();
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Failed to open servicecontrolmanager, error {0}",
            errorCode);
    }

    if (serviceHandle != NULL)
    {
        if (!CloseServiceHandle(serviceHandle))
        {
            errorCode = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType,
                Root.TraceId,
                "Failed to close container service handle,  error {0}",
                errorCode);
        }
    }
    if (scmHandle != NULL)
    {
        if (!CloseServiceHandle(scmHandle))
        {
            errorCode = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType,
                Root.TraceId,
                "Failed to close SCM handle,  error {0}",
                errorCode);
        }
    }
    if (errorCode.IsSuccess())
    {
        if (serviceStopped != true)
        {
            errorCode.Overwrite(ErrorCodeValue::InvalidState);
        }
    }
#else
    auto dockerExeName = wformatString("/usr/bin/{0}", HostingConfig::GetConfig().ContainerProviderProcessName);
    if (!File::Exists(dockerExeName))
    {
        errorCode = ErrorCode(
            ErrorCodeValue::ServiceNotFound, 
            StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));
    }
    else
    {
        //If docker service is already running stop it since otherwise daemon may not start
        system(" [ -f /var/run/docker.pid ] && service docker stop");
    }
#endif
    return errorCode;
}

ErrorCode DockerProcessManager::CheckDockerServicePresence()
{
    ErrorCode errorCode(ErrorCodeValue::Success);

#if !defined(PLATFORM_UNIX)
    auto scmHandle = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    SC_HANDLE serviceHandle = NULL;
    if (scmHandle != NULL)
    {
        auto serviceName = HostingConfig::GetConfig().ContainerProviderServiceName;
        serviceHandle = ::OpenService(scmHandle, serviceName.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (serviceHandle == NULL)
        {
            errorCode = ErrorCode::FromWin32Error();

            if (errorCode.IsWin32Error(ERROR_SERVICE_DOES_NOT_EXIST))
            {
                isDockerServicePresent_ = false;
                isDockerServiceManaged_ = false;

                errorCode = ErrorCode::Success();
            }
            else
            {
                WriteWarning(
                    TraceType,
                    Root.TraceId,
                    "Failed to open socker service handle,  error {0}",
                    errorCode);
            }
        }
    }
    else
    {
        errorCode = ErrorCode::FromWin32Error();
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Failed to open ServiceControlManager, error {0}",
            errorCode);
    }

    if (serviceHandle != NULL)
    {
        if (!CloseServiceHandle(serviceHandle))
        {
            errorCode = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType,
                Root.TraceId,
                "Failed to close container service handle,  error {0}",
                errorCode);
        }
    }
    if (scmHandle != NULL)
    {
        if (!CloseServiceHandle(scmHandle))
        {
            errorCode = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType,
                Root.TraceId,
                "Failed to close SCM handle,  error {0}",
                errorCode);
        }
    }
#else
    auto dockerExeName = wformatString("/usr/bin/{0}", HostingConfig::GetConfig().ContainerProviderProcessName);
    if (!File::Exists(dockerExeName))
    {
        isDockerServicePresent_ = false;
        isDockerServiceManaged_ = false;
    }
#endif

    return errorCode;
}

