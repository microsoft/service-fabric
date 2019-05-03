// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("NetworkPluginProcessManager");

class NetworkPluginProcessManager::OpenAsyncOperation
    : public AsyncOperation,
      private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in NetworkPluginProcessManager & owner,
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

    static ErrorCode End(AsyncOperationSPtr const & operation, IProcessActivationContextSPtr & activationContext)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        if (thisPtr->activationContext_ != nullptr)
        {
            activationContext = move(thisPtr->activationContext_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto pluginSetup = owner_.pluginSetup_;
        if (pluginSetup)
        {
            owner_.ShutdownPlugin();
            StartPlugin(thisSPtr);
        }
        else
        {
            // Don't monitor the plugin if set up has not been performed
            // otherwise it will conflict with the deployer steps.
            WriteInfo(
                TraceType,
                "Skipping network plugin process manager. Plugin setup: {0}",
                pluginSetup);

            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

private:
    void StartPlugin(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.IsFabricHostClosing())
        {
            TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }

        ErrorCode errorCode(ErrorCodeValue::Success);

        wstring workingDir;
        auto dataRootError = FabricEnvironment::GetFabricDataRoot(workingDir);
        if (!dataRootError.IsSuccess())
        {
            TryComplete(thisSPtr, dataRootError);
            return;
        }

        wstring binRoot;
        auto binRootError = FabricEnvironment::GetFabricBinRoot(binRoot);
        if (!binRootError.IsSuccess())
        {
            TryComplete(thisSPtr, binRootError);
            return;
        }

        wstring networkPluginProcessName = L"";
        EnvironmentMap environmentMap;
        
#if !defined(PLATFORM_UNIX)
        networkPluginProcessName = wformatString("{0}\\Fabric\\Fabric.Code\\{1}", binRoot, owner_.pluginName_);
#else
        networkPluginProcessName = wformatString("{0}/Fabric/Fabric.Code/{1}", binRoot, owner_.pluginName_);
        wstring pathVar;
        Environment::GetEnvironmentVariable(L"PATH", pathVar, L"/usr/bin:/usr/sbin:/sbin");
        environmentMap[L"PATH"] = pathVar;
#endif

        WriteInfo(
            TraceType,
            "Starting network plugin process name {0}",
            networkPluginProcessName);

        ProcessDescription procDesc(
            networkPluginProcessName,
            L"",
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
                this->OnPluginProcessStarted(operation, false);
            },
            thisSPtr);

        OnPluginProcessStarted(operation, true);
    }

    void OnPluginProcessStarted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.processActivationManager_.ProcessActivatorObj->EndActivate(
            operation,
            activationContext_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "Network plugin process started. ErrorCode={0}",
            error);

        TryComplete(operation->Parent, error);
    }

    ProcessWait::WaitCallback GetProcessTerminationHandler()
    {
        NetworkPluginProcessManager & controller = owner_;
        auto root = owner_.Root.CreateComponentRoot();
        return ProcessWait::WaitCallback(
            [root, &controller](DWORD, ErrorCode const & waitResult, DWORD processExitCode)
        {
            controller.OnProcessTerminated(processExitCode, waitResult);
        });
    }

private:
    NetworkPluginProcessManager & owner_;
    IProcessActivationContextSPtr activationContext_;
    TimeoutHelper timeoutHelper_;
};

class NetworkPluginProcessManager::CloseAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in NetworkPluginProcessManager & owner,
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
        WriteInfo(TraceType, "Closing network plugin process manager.");

        if (this->owner_.pluginSetup_)
        {
            owner_.CleanupTimer();
            owner_.ShutdownPlugin();
        }

        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

private:
    NetworkPluginProcessManager & owner_;
    TimeoutHelper timeoutHelper_;
};

NetworkPluginProcessManager::NetworkPluginProcessManager(
    ComponentRoot const & root,
    ProcessActivationManager & processActivationManager,
    NetworkPluginProcessRestartedCallback const & callback,
    std::wstring const & pluginName,
    std::wstring const & pluginSockPath,
    bool pluginSetup,
    Common::TimeSpan initTimeout,
    Common::TimeSpan activationExceptionInterval,
    Common::TimeSpan activationRetryBackoffInterval,
    int activationMaxFailureCount)
    : RootedObject(root),
    processActivationManager_(processActivationManager),
    networkPluginProcessRestartedCallback_(callback),
    lastProcessTerminationTime_(DateTime::Now()),
    retryTimer_(),
    retryTimerLock_(),
    retryOnFailure_(true),
    retryCount_(0),
    pluginName_(pluginName),
    pluginSockPath_(pluginSockPath),
    pluginSetup_(pluginSetup),
    initTimeout_(initTimeout),
    activationExceptionInterval_(activationExceptionInterval),
    activationRetryBackoffInterval_(activationRetryBackoffInterval),
    activationMaxFailureCount_(activationMaxFailureCount)
{
}

NetworkPluginProcessManager::~NetworkPluginProcessManager()
{
    WriteNoise(TraceType, "NetworkPluginProcessManager.destructor");
}

// ********************************************************************************************************************
// AsyncFabricComponent methods
// ********************************************************************************************************************
AsyncOperationSPtr NetworkPluginProcessManager::OnBeginOpen(
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

ErrorCode NetworkPluginProcessManager::OnEndOpen(AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation, activationContext_);
}

AsyncOperationSPtr NetworkPluginProcessManager::OnBeginClose(
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

ErrorCode NetworkPluginProcessManager::OnEndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void NetworkPluginProcessManager::OnAbort()
{
    WriteInfo(TraceType, "Aborting network plugin process manager.");
    if (pluginSetup_)
    {
        CleanupTimer();
        ShutdownPlugin();
    }
}

void NetworkPluginProcessManager::OnProcessTerminated(DWORD exitCode, ErrorCode waitResult)
{
    if (this->IsFabricHostClosing())
    {
        return;
    }

    WriteInfo(
        TraceType,
        "Network plugin process terminated exitCode {0}, wait errorCode {1}",
        exitCode,
        waitResult);

    auto currentTime = DateTime::Now();
    auto processTerminationInterval = currentTime - lastProcessTerminationTime_;
    lastProcessTerminationTime_ = currentTime;

    // if process termination interval has exceeded the limit then reset the retry count.
    // This gives an opportunity to allow failures that occur over a longer period of time, a chance to recover
    if (processTerminationInterval > activationExceptionInterval_)
    {
        WriteInfo(
            TraceType,
            "Network plugin process has not terminated in the last {0} minutes. Resetting retry count.",
            activationExceptionInterval_);

        retryCount_ = 0;
    }

    retryCount_++;
    if (retryCount_ > activationMaxFailureCount_)
    {
        StringLiteral msgFormat("NetworkPluginProcessManager failed to start plugin process. RetryCount={0}.");

        WriteError(
            TraceType,
            msgFormat,
            retryCount_);

        Assert::CodingError(msgFormat, retryCount_);
    }

    CleanupTimer();

    int64 delay = activationRetryBackoffInterval_.Ticks * retryCount_;

    WriteInfo(
        TraceType,
        "Retrying request to start network plugin. Attempt: {0} Delay(ticks): {1}",
        retryCount_,
        delay);

    AcquireExclusiveLock stateLock(this->get_StateLock());
    auto state = this->State.get_Value_CallerHoldsLock();
    if (state == FabricComponentState::Opened)
    {
        AcquireExclusiveLock timerLock(retryTimerLock_);
        ComponentRootWPtr weakRoot = this->Root.CreateComponentRoot();
        retryTimer_ = Timer::Create(
            "Hosting.NetworkPluginProcessManager.Restart",
            [this, weakRoot](TimerSPtr const & timer)
        {
            timer->Cancel();
            auto root = weakRoot.lock();
            if (root)
            {
                this->RestartPlugin();
            }
        });
        retryTimer_->Change(TimeSpan::FromTicks(delay));
    }
}

void NetworkPluginProcessManager::RestartPlugin()
{
    auto operation = AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        initTimeout_,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnProcessStartCompleted(operation, false);
        },
        this->Root.CreateAsyncOperationRoot());

    this->OnProcessStartCompleted(operation, true);
}

void NetworkPluginProcessManager::OnProcessStartCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = OpenAsyncOperation::End(operation, activationContext_);

    if (error.IsSuccess())
    {
        this->networkPluginProcessRestartedCallback_();
    }

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "Network plugin process started after process terminated. ErrorCode={0} Retry count {1}",
        error,
        retryCount_);
}

void NetworkPluginProcessManager::CleanupTimer()
{
    AcquireExclusiveLock lock(retryTimerLock_);
    if (retryTimer_ != nullptr)
    {
        retryTimer_->Cancel();
    }
}

void NetworkPluginProcessManager::ShutdownPlugin()
{
    WriteInfo(
        TraceType,
        "Shutting down network plugin process.");

    ASSERT_IF(!pluginSetup_, "Network plugin cannot be shutdown because the plugin wasn't setup.");

#if !defined(PLATFORM_UNIX)
    // Kill network plugin process
    auto shutdownCommand = wformatString("taskkill /F /IM {0}", pluginName_);
    auto shutdownCommandUtf8 = StringUtility::Utf16ToUtf8(shutdownCommand);
    std::system(shutdownCommandUtf8.c_str());
#else
    // Kill network plugin process
    auto shutdownCommand = wformatString("killall {0}", pluginName_);
    auto shutdownCommandUtf8 = StringUtility::Utf16ToUtf8(shutdownCommand);
    system(shutdownCommandUtf8.c_str());

    // Remove the sock file if it exists
    auto removeSockFileCommand = wformatString("rm -f {0}", pluginSockPath_);
    auto removeSockFileCommandUtf8 = StringUtility::Utf16ToUtf8(removeSockFileCommand);
    system(removeSockFileCommandUtf8.c_str());
#endif
}

bool NetworkPluginProcessManager::IsFabricHostClosing()
{
    return (this->State.Value > FabricComponentState::Opened);
}