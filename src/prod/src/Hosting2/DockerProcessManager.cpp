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
        if (HostingConfig::GetConfig().SkipDockerProcessManagement == true)
        {
            owner_.isDockerServiceManaged_ = false;

            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "Skipping docker process management.");

            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

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

        InitializeDockerProcess(thisSPtr);
    }

private:
    void InitializeDockerProcess(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.IsFabricHostClosing())
        {
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
        if (owner_.IsFabricHostClosing())
        {
            TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }

        auto errorCode = owner_.ShutdownDockerService();
        if (errorCode.IsSuccess())
        {
            if (owner_.IsWinServerOrLinux())
            {
                StartDockerProcess(thisSPtr);
            }
            else
            {
                //
                // For client windows OS (Win10) we do not manage docker daemon.
                // We restart the docker daemon service to ensure cleaning up
                // any previously running container.
                //
                owner_.isDockerServiceManaged_ = false;
                StartDockerNtService(thisSPtr);
            }

            return;
        }
        else if (errorCode.IsError(ErrorCodeValue::OperationsPending))
        {
            Threadpool::Post(
                [this, thisSPtr]() { StopDockerNtService(thisSPtr); },
                TimeSpan::FromSeconds(3));

            return;
        }
        else if (errorCode.IsError(ErrorCodeValue::ServiceNotFound))
        {
            auto error = TransitionToStarted();
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "Failed to transition dockerprocessmanager state {0}",
                    error);
            }
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

    void StartDockerNtService(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.IsFabricHostClosing())
        {
            TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }

        auto error = owner_.StartDockerService();
        if (error.IsError(ErrorCodeValue::OperationsPending))
        {
            Threadpool::Post(
                [this, thisSPtr]() { StartDockerNtService(thisSPtr); },
                TimeSpan::FromSeconds(3));

            return;
        }
        else if (!error.IsSuccess())
        {
            CheckMaxRetryCount();

            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "StartDockerNtService: ErrorCode={0}, FailureRetryCount={1}. Retrying...",
                error,
                failureRetryCount_);

            Threadpool::Post(
                [this, thisSPtr]() { StartDockerNtService(thisSPtr); },
                TimeSpan::FromSeconds(retryDelay_));

            return;
        }

        error = this->TransitionToStarted();
        TryComplete(thisSPtr, error);
        return;
    }

    void StartDockerProcess(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.IsFabricHostClosing())
        {
            TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }

        wstring workingDir;
        auto error = FabricEnvironment::GetFabricDataRoot(workingDir);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        wstring dockerpidFile;
        error = owner_.ConfigureDockerPidFile(workingDir, dockerpidFile);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        EnvironmentMap environmentMap;
        wstring dockerProcessName;

        wstring args;

#if !defined(PLATFORM_UNIX)
        args = wformatString("{0} {1} --pidfile {2}",
            HostingConfig::GetConfig().UseContainerServiceArguments ? HostingConfig::GetConfig().ContainerServiceArguments : L"",
            HostingConfig::GetConfig().EnableContainerServiceDebugMode ? L"--debug" : L"",
            dockerpidFile);

        dockerProcessName = HostingConfig::GetConfig().ContainerProviderProcessName;
        auto dockertempDir = Path::Combine(workingDir, Constants::DockerTempDirName);
        if (Directory::Exists(dockertempDir))
        {
            error = Directory::Delete(
                dockertempDir,
                true /*recursive*/,
                true /*deleteReadOnlyFiles*/);
            //Delete is best effort. Dont want to block process startup for it.
            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                owner_.Root.TraceId,
                "Failed to delete temp docker dir {0}",
                error);
        }

        error = Directory::Create2(dockertempDir);
        if (!error.IsSuccess())
        {
            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                owner_.Root.TraceId,
                "ContainerProvider createdirectory {1}: ErrorCode={0}",
                error,
                dockertempDir);
        }
        else
        {
            //Get current env map and update tmp/temp vars
            auto result = Environment::GetEnvironmentMap(environmentMap);
            if (!result)
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "ContainerProvider failed to get current environment block, not setting temp dir");
            }
            else
            {
                environmentMap[Constants::EnvironmentVariable::DockerTempDir] = dockertempDir;
                environmentMap[Constants::EnvironmentVariable::TempDir] = dockertempDir;
            }
        }
#else
        args = wformatString("{0} --pidfile {1}",
            HostingConfig::GetConfig().UseContainerServiceArguments ? HostingConfig::GetConfig().ContainerServiceArguments : L"",
            dockerpidFile);

        dockerProcessName = wformatString("/usr/bin/{0}", HostingConfig::GetConfig().ContainerProviderProcessName);

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

// File redirection only works on Windows.
#if !defined(PLATFORM_UNIX)

        wstring dockerLogDirectory = workingDir;
        wstring dockerLogFileOutPrefix = L"";
        int containerServiceLogFileRetentionCount = 0;
        int containerServiceLogFileMaxSizeInKb = 0;

        if (HostingConfig::GetConfig().EnableContainerServiceDebugMode)
        {
            wstring logDirectory;
            error = FabricEnvironment::GetFabricLogRoot(logDirectory);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }

            dockerLogDirectory = Path::Combine(logDirectory, Constants::DockerLogDirectory, false /*escapePath*/);
            SetupDockerLoggingDirectory(dockerLogDirectory);

            // The string output from DateTime now has colons in it which is an illegal
            // character for windows file names.  Therefore, replace colons with dots (: --> .)
            wstring dateStringForFileOut = DateTime::Now().ToString(true /*utc*/);
            replace(dateStringForFileOut.begin(), dateStringForFileOut.end(), ':', '.');

            dockerLogFileOutPrefix = wformatString("{0}_{1}",
                HostingConfig::GetConfig().ContainerServiceLogFileNamePrefix,
                dateStringForFileOut);

            containerServiceLogFileRetentionCount = HostingConfig::GetConfig().ContainerServiceLogFileRetentionCount;
            containerServiceLogFileMaxSizeInKb = HostingConfig::GetConfig().ContainerServiceLogFileMaxSizeInKb;
        }

        ProcessDescription procDesc(
            dockerProcessName,
            args,
            workingDir,
            environmentMap,
            workingDir,
            workingDir,
            workingDir,
            dockerLogDirectory,
            false,
            HostingConfig::GetConfig().EnableContainerServiceDebugMode,
            false,
            false,
            dockerLogFileOutPrefix,
            containerServiceLogFileRetentionCount,
            containerServiceLogFileMaxSizeInKb);

#else
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
#endif

        // Step1: CleanupHNSEndpoints. If it fails to cleanup it will not prevent docker process startup.
        owner_.CleanupHNSEndpoints();

        // Step2: Start process
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

// This is only called on windows platforms.
#if !defined(PLATFORM_UNIX)
    void SetupDockerLoggingDirectory(wstring const & dockerLoggingDirectory)
    {
        bool createDockerLogDirectory = true;

        if (Common::Directory::Exists(dockerLoggingDirectory))
        {
            auto fileNameCollection = Common::Directory::GetFiles(dockerLoggingDirectory);

            // We need to delete the Docker logs if the log directory gets too large as we don't want to fill up a hardrive with endless log files
            // For example, if Docker is continously crashing and coming back up and that goes on for several months, this directory could be filled with
            // hundreds of thousands of files.
            if (fileNameCollection.size() > HostingConfig::GetConfig().ContainerServiceLogFileRetentionCount * HostingConfig::GetConfig().ContainerServiceLogFileRetentionFactor)
            {
                wstring tempDockerLogDirectory = wformatString("{0}_{1}", dockerLoggingDirectory, DateTime::Now().Ticks);
                auto error = Common::Directory::Rename(dockerLoggingDirectory, tempDockerLogDirectory, true /*overWrite*/);

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType,
                        owner_.Root.TraceId,
                        "Error '{0}' renaming directory {1} to {2} during docker logging redirection initialization.",
                        error,
                        dockerLoggingDirectory,
                        tempDockerLogDirectory);

                    createDockerLogDirectory = false;
                }
                else
                {
                    auto root = owner_.Root.CreateComponentRoot();

                    Threadpool::Post(
                        [root, this, tempDockerLogDirectory]()
                    {
                        auto error = Common::Directory::Delete_WithRetry(tempDockerLogDirectory, true /*recursive*/, true /*deleteReadOnlyFiles*/);

                        if (!error.IsSuccess())
                        {
                            WriteWarning(
                                TraceType,
                                owner_.Root.TraceId,
                                "Error '{0}' deleting docker logging directory {1}.",
                                error,
                                tempDockerLogDirectory);
                        }
                    });
                }
            }
            else
            {
                createDockerLogDirectory = false;
            }
        }

        if (createDockerLogDirectory)
        {
            auto error = Common::Directory::Create2(dockerLoggingDirectory);

            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType,
                    owner_.Root.TraceId,
                    "Error '{0}' creating directory {1} while setting up docker logging redirection.",
                    error,
                    dockerLoggingDirectory);

                ASSERT_IF(!error.IsSuccess(),
                    "Error '{0}' creating directory {1} while setting up docker logging redirection.",
                    error,
                    dockerLoggingDirectory);
            }
        }
    }
#endif

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

            // Reset so that the file redirection handlers close the file handle
            activationContext_.reset();

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
            if (activationContext_ != nullptr)
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "Docker daemon process has started with Pid={0}.",
                    activationContext_->ProcessId);
            }
            else
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "Docker daemon service has restarted successfully. IsDockerServiceManaged={0}.",
                    owner_.isDockerServiceManaged_);
            }
        }
        else
        {
            WriteError(
                TraceType,
                owner_.Root.TraceId,
                "Docker daemon start operation failed. StateTransitionResult={0}. IsDockerServiceManaged={0}",
                transitionResult,
                owner_.isDockerServiceManaged_);

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
        if (HostingConfig::GetConfig().SkipDockerProcessManagement == true || 
            owner_.isDockerServicePresent_ == false)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        if(owner_.GetState() == owner_.Deactivated)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        auto transitionResult = owner_.TransitionToDeactivating();
        if(transitionResult.IsSuccess())
        {
            if (owner_.IsWinServerOrLinux())
            {
                this->DoDeactivate(thisSPtr);
            }
            else
            {
                //
                // For client windows OS (Win10) we do not manage docker daemon.
                // We restart the docker daemon service to ensure cleaning up
                // any running container.
                //
                this->RestartDockerNtService(thisSPtr);
            }
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
    void RestartDockerNtService(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ShutdownDockerService();
        if (error.IsSuccess())
        {
            // Issue the start and finished deactivation.
            owner_.StartDockerService();
        }
        else if (error.IsError(ErrorCodeValue::OperationsPending))
        {
            Threadpool::Post(
                [this, thisSPtr]() { RestartDockerNtService(thisSPtr); },
                TimeSpan::FromSeconds(5));

            return;
        }

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "RestartDockerNtService: ErrorCode={0}.",
            error);

        TransitionToDeactivated(thisSPtr, error);
    }

    void DoDeactivate(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.CleanupStatTimer();

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

#ifndef PLATFORM_UNIX
class DockerProcessManager::HNSEndpointCleanup :
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
    DENY_COPY(HNSEndpointCleanup)

public:
    typedef int(__stdcall *pFuncHnsCall)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR*);

    HNSEndpointCleanup(
        DockerProcessManager & owner)
        : owner_(owner)
        , pHnsCall_()
    {
        wstring vmcomputeDll = L"vmcompute.dll";
        // %windows%\system32 is searched for the DLL and its dependencies.
        DWORD searchSys32 = LOAD_LIBRARY_SEARCH_SYSTEM32;
        auto hmodule = LoadLibraryEx(vmcomputeDll.c_str(), nullptr, searchSys32);
        if (!hmodule)
        {
            auto err = ::GetLastError();
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to load vmcompute.dll. Error:{0}",
                err);
            return;
        }

        pHnsCall_ = (pFuncHnsCall)GetProcAddress(hmodule, "HNSCall");
        if (!pHnsCall_)
        {
            auto err = ::GetLastError();
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to GetProcAddress of HNSCall in vmcompute.dll. Error:{0}",
                err);
            return;
        }
    }

    void CleanupHNSEndpoints()
    {
        wstring method(L"GET");
        wstring path(L"/endpoints/");
        wstring request(L"");
        LPWSTR p_output = nullptr;

        if (!pHnsCall_)
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetProcAddress for HNSCall is not initialized.");
            return;
        }

        auto status = pHnsCall_(method.c_str(), path.c_str(), request.c_str(), &p_output);
        if (status != 0)
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to invoke HNS call for Method:{0}, Path:{1}, Request:{2}. Status error:{3}",
                method,
                path,
                request,
                status);
            return;
        }

        wstring output(p_output);

        HNSEndpointResponse hnsEndpointResponse;
        auto error = JsonHelper::Deserialize(hnsEndpointResponse, output);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed deserializing HNSResponse:{0} Error:{1}",
                output,
                error);
            return;
        }

        wstring deleteMethod(L"DELETE");
        for (HNSEndpoint const& endpointId : hnsEndpointResponse.hnsEndpoints_)
        {
            wstring endpoint = wformatString("{0}{1}", L"/endpoints/", endpointId.id_);
            auto deleteStatus = pHnsCall_(deleteMethod.c_str(), endpoint.c_str(), request.c_str(), &p_output);

            if (deleteStatus != 0)
            {
                WriteError(
                    TraceType,
                    owner_.Root.TraceId,
                    "Failed deleting HNSEndpoint:{0} delete response:{1}.",
                    endpointId.id_,
                    p_output);
            }
            else
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "HNSEndpoint:{0} delete response:{1}.",
                    endpointId.id_,
                    p_output);
            }
        }
    }

private:
    class HNSEndpoint : public IFabricJsonSerializable
    {
    public:
        HNSEndpoint() {}
        HNSEndpoint(std::wstring const& id)
        {
            id_ = id;
        }
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"ID", id_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        wstring id_;
    };

    class HNSEndpointResponse : public IFabricJsonSerializable
    {
        DENY_COPY(HNSEndpointResponse)

    public:
        HNSEndpointResponse() {}
        HNSEndpointResponse(HNSEndpoint const& hnsEndpoint)
        {
            hnsEndpoints_.push_back(hnsEndpoint);
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Output", hnsEndpoints_)
        END_JSON_SERIALIZABLE_PROPERTIES()

       std::vector<HNSEndpoint> hnsEndpoints_;
    };

private:
    DockerProcessManager & owner_;
    pFuncHnsCall pHnsCall_;
};
#endif

FabricActivationManager const & DockerProcessManager::get_ActivationManager() const
{ 
    return processActivationManager_.ActivationManager; 
}

DockerProcessManager::DockerProcessManager(
    ComponentRoot const & root,
    ContainerEngineTerminationCallback const & callback,
    ProcessActivationManager & processActivationManager)
    : RootedObject(root)
    , StateMachine(Inactive)
    , callback_(callback)
    , processActivationManager_(processActivationManager)
    , activationContext_()
    , lock_()
    , isDockerServicePresent_(true)
    , isDockerServiceManaged_(true)
    , continuousExitCount_(0)
    , resetStatstimer_()
#ifndef PLATFORM_UNIX
    , hnsEndpointCleanup_()
#endif
{
#ifndef PLATFORM_UNIX
    hnsEndpointCleanup_ = make_unique<HNSEndpointCleanup>(*this);
#endif
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
    this->CleanupStatTimer();

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
    if (this->IsFabricHostClosing())
    {
        return;
    }

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

    ULONG continuousExitCount = 0;
    TimeSpan dueTime = this->UpdateExitStatsAndGetDueTime(continuousExitCount);

    callback_(exitCode, continuousExitCount, dueTime);

    // Reset so that the file redirection handlers close the file handle
    activationContext_.reset();

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

    this->ScheduleStart(dueTime);
}

void DockerProcessManager::ScheduleStart(TimeSpan const & dueTime)
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "DockerProcessManager scheduling restart after {0} seconds",
        dueTime.TotalSeconds());

    auto root = this->Root.CreateComponentRoot();
    Threadpool::Post(
        [this, root]() {
        this->BeginInitialize(
            HostingConfig::GetConfig().ActivationTimeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->EndInitialize(operation);
        },
            this->Root.CreateAsyncOperationRoot());
    },
        dueTime);
}

void DockerProcessManager::CleanupStatTimer()
{
    AcquireWriteLock lock(this->Lock);

    if (resetStatstimer_)
    {
        resetStatstimer_->Cancel();
        resetStatstimer_.reset();
    }
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
                else if (status.dwCurrentState == SERVICE_STOP_PENDING)
                {
                    errorCode.Overwrite(ErrorCodeValue::InvalidState);
                }
                else
                {
                    WriteInfo(
                        TraceType,
                        Root.TraceId,
                        "Container provider service {0} is in {1} state, stopping now",
                        serviceName,
                        status.dwCurrentState);

                    errorCode.Overwrite(ErrorCodeValue::OperationsPending);

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
    if (errorCode.IsSuccess() && !serviceStopped)
    {
        errorCode.Overwrite(ErrorCodeValue::InvalidState);
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

ErrorCode DockerProcessManager::StartDockerService()
{
    ErrorCode errorCode(ErrorCodeValue::Success);

#if !defined(PLATFORM_UNIX)
    auto serviceStarted = false;
    SC_HANDLE serviceHandle = NULL;

    auto scmHandle = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scmHandle != NULL)
    {
        auto serviceName = HostingConfig::GetConfig().ContainerProviderServiceName;
        serviceHandle = ::OpenService(scmHandle, serviceName.c_str(), SERVICE_START | SERVICE_QUERY_STATUS);
        if (serviceHandle != NULL)
        {
            SERVICE_STATUS status;
            if (::QueryServiceStatus(serviceHandle, &status))
            {
                if (status.dwCurrentState == SERVICE_RUNNING)
                {
                    serviceStarted = true;
                }
                else if (status.dwCurrentState == SERVICE_START_PENDING)
                {
                    errorCode.Overwrite(ErrorCodeValue::InvalidState);
                }
                else
                {
                    WriteInfo(
                        TraceType,
                        Root.TraceId,
                        "Container provider service {0} is in {1} state, starting now...",
                        serviceName,
                        status.dwCurrentState);

                    errorCode.Overwrite(ErrorCodeValue::OperationsPending);

                    if (!::StartService(serviceHandle, 0, NULL))
                    {
                        errorCode = ErrorCode::FromWin32Error();
                        WriteWarning(
                            TraceType,
                            Root.TraceId,
                            "Failed to issue start control command to Container provider service {0}, error {1}.",
                            serviceName,
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

    if (errorCode.IsSuccess() && !serviceStarted)
    {
        errorCode.Overwrite(ErrorCodeValue::InvalidState);
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

bool DockerProcessManager::IsWinServerOrLinux()
{
#if !defined(PLATFORM_UNIX)
    return IsWindowsServer();
#else
    return true;
#endif
}

bool DockerProcessManager::IsFabricHostClosing()
{
    // If true then FabricHost is closing down
    return (this->ActivationManager.State.Value > FabricComponentState::Opened);
}

TimeSpan DockerProcessManager::UpdateExitStatsAndGetDueTime(_Out_ ULONG & continuousExitCount)
{
    {
        AcquireWriteLock lock(this->Lock);

        continuousExitCount = ++continuousExitCount_;

        if (resetStatstimer_)
        {
            resetStatstimer_->Cancel();
            resetStatstimer_ = nullptr;
        }

        auto root = Root.CreateComponentRoot();
        resetStatstimer_ = Timer::Create(
            "DockerProcessManager.ContinousExitFailureReset", [root, this](TimerSPtr const & timer) { OnContinousExitFailureResetTimeout(timer, continuousExitCount_); }, false);
        resetStatstimer_->Change(HostingConfig::GetConfig().DockerProcessContinuousExitFailureResetInterval);
    }

    int64 retrySeconds = (int64)(HostingConfig::GetConfig().DockerProcessRetryBackoffInterval.TotalSeconds() * continuousExitCount);
    if (retrySeconds > HostingConfig::GetConfig().DockerProcessMaxRetryInterval.TotalSeconds())
    {
        retrySeconds = HostingConfig::GetConfig().DockerProcessMaxRetryInterval.TotalSeconds();
    }

    return TimeSpan::FromSeconds(static_cast<double>(retrySeconds));
}

void DockerProcessManager::OnContinousExitFailureResetTimeout(TimerSPtr const & timer, ULONG exitCount)
{
    UNREFERENCED_PARAMETER(timer);

    {
        AcquireWriteLock lock(this->Lock);

        if (!IsFabricHostClosing())
        {
            if (this->continuousExitCount_ != exitCount)
            {
                // the exit count has changed which indicates that the
                // process has exited again. If you cancel the timer here this will cancel the
                // timer created by UpdateExitStatsAndGetDueTime during a new crash.
                return;
            }

            this->continuousExitCount_ = 0;
        }

        //If FabricHost is closing cancel the time so that we do not leak a refernce to component root.
        if (resetStatstimer_)
        {
            resetStatstimer_->Cancel();
            resetStatstimer_ = nullptr;
        }
    }
}

ErrorCode DockerProcessManager::ConfigureDockerPidFile(wstring const& workingDirectory, wstring & dockerpidFile)
{
    auto dockerPidDirectory = Path::Combine(workingDirectory, Constants::DockerProcessIdFileDirectory);

    if (!Directory::Exists(dockerPidDirectory))
    {
        auto error = Directory::Create2(dockerPidDirectory);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                Root.TraceId,
                "Failed to create docker pid directory {0} Error:{1}",
                dockerPidDirectory,
                error);
            return error;
        }
    }

    //Before creating new dockerpid file, post removal of all the other pid files in the directory.
    auto root = this->Root.CreateComponentRoot();
    auto dockerPidFilesToRemove = Directory::GetFiles(dockerPidDirectory);
    Threadpool::Post([this, dockerPidFilesToRemove, dockerPidDirectory, root] {
        for (auto const& dockerPidFile : dockerPidFilesToRemove)
        {
            auto pidFile = Path::Combine(dockerPidDirectory, dockerPidFile);
            auto error = File::Delete2(pidFile);
            if (!error.IsSuccess())
            {
                WriteInfo(
                    TraceType,
                    Root.TraceId,
                    "Failed to remove docker pid file {0} Error:{1}",
                    pidFile,
                    error);
            }
        }
    });

    dockerpidFile = Path::Combine(dockerPidDirectory, wformatString("{0}_{1}", DateTime::Now().Ticks, Constants::DockerProcessFile), true /*escapePath*/);

    return ErrorCodeValue::Success;
}

void DockerProcessManager::CleanupHNSEndpoints()
{
#ifndef PLATFORM_UNIX
    if(HostingConfig::GetConfig().HNSEndpointsCleanup)
    {
        this->hnsEndpointCleanup_->CleanupHNSEndpoints();
    }
#endif
}