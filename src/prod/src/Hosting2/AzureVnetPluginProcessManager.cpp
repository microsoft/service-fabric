// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("AzureVnetPluginProcessManager");

class AzureVnetPluginProcessManager::OpenAsyncOperation
    : public AsyncOperation,
      private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in AzureVnetPluginProcessManager & owner,
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

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto containerNetworkSetup = SetupConfig::GetConfig().ContainerNetworkSetup;
        if (containerNetworkSetup)
        {
            owner_.ShutdownAzureVnetPlugin();
            StartAzureVnetPlugin(thisSPtr);
        }
        else
        {
            // Don't monitor the plugin if set up has not been performed
            // otherwise it will conflict with the deployer steps.
            WriteInfo(
                TraceType,
                "Skipping azure vnet plugin process manager. ContainerNetworkSetup: {0}",
                containerNetworkSetup);

            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

private:
    void StartAzureVnetPlugin(AsyncOperationSPtr const & thisSPtr)
    {
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

        wstring azureVnetPluginProcessName = wformatString("{0}/Fabric/Fabric.Code/{1}", binRoot, HostingConfig::GetConfig().AzureVnetPluginName);
        EnvironmentMap environmentMap;
        wstring pathVar;
        Environment::GetEnvironmentVariable(L"PATH", pathVar, L"/usr/bin:/usr/sbin:/sbin");
        environmentMap[L"PATH"] = pathVar;

        WriteInfo(
            TraceType,
            "Starting azure vnet plugin process name {0}",
            azureVnetPluginProcessName);

        ProcessDescription procDesc(
            azureVnetPluginProcessName,
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
                this->OnAzureVnetPluginProcessStarted(operation, false);
            },
            thisSPtr);

         OnAzureVnetPluginProcessStarted(operation, true);
    }

    void OnAzureVnetPluginProcessStarted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
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
            "Azure vnet plugin process started. ErrorCode={0}",
            error);

        TryComplete(operation->Parent, error);
        return;
    }

    ProcessWait::WaitCallback GetProcessTerminationHandler()
    {
        AzureVnetPluginProcessManager & controller = owner_;
        return ProcessWait::WaitCallback(
            [&controller](DWORD, ErrorCode const & waitResult, DWORD processExitCode)
        {
            controller.OnProcessTerminated(processExitCode, waitResult);
        });
    }

private:
    AzureVnetPluginProcessManager & owner_;
    IProcessActivationContextSPtr activationContext_;
    TimeoutHelper timeoutHelper_;
};

class AzureVnetPluginProcessManager::CloseAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in AzureVnetPluginProcessManager & owner,
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
        WriteInfo(TraceType, "Closing azure vnet plugin process manager.");

        owner_.CleanupTimer();
        owner_.ShutdownAzureVnetPlugin();
    }

private:
    AzureVnetPluginProcessManager & owner_;
    TimeoutHelper timeoutHelper_;
};

AzureVnetPluginProcessManager::AzureVnetPluginProcessManager(
    ComponentRootSPtr const & root,
    ProcessActivationManager & processActivationManager)
    : RootedObject(*root),
    processActivationManager_(processActivationManager),
    lastProcessTerminationTime_(DateTime::Now()),
    retryTimer_(),
    retryTimerLock_(),
    retryOnFailure_(true),
    retryCount_(0)
{
}

AzureVnetPluginProcessManager::~AzureVnetPluginProcessManager()
{
    WriteNoise(TraceType, "AzureVnetPluginProcessManager.destructor");
}

// ********************************************************************************************************************
// AsyncFabricComponent methods
// ********************************************************************************************************************
AsyncOperationSPtr AzureVnetPluginProcessManager::OnBeginOpen(
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

ErrorCode AzureVnetPluginProcessManager::OnEndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr AzureVnetPluginProcessManager::OnBeginClose(
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

ErrorCode AzureVnetPluginProcessManager::OnEndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void AzureVnetPluginProcessManager::OnAbort()
{
    WriteInfo(TraceType, "Aborting azure vnet plugin process manager.");

    CleanupTimer();
    ShutdownAzureVnetPlugin();
}

void AzureVnetPluginProcessManager::OnProcessTerminated(DWORD exitCode, ErrorCode waitResult)
{
    WriteInfo(
        TraceType,
        "Azure vnet plugin process terminated exitCode {0}, wait errorCode {1}",
        exitCode,
        waitResult);

    auto currentTime = DateTime::Now();
    auto processTerminationInterval = currentTime - lastProcessTerminationTime_;
    lastProcessTerminationTime_ = currentTime;

    // if process termination interval has exceeded the limit then reset the retry count.
    // This gives an opportunity to allow failures that occur over a longer period of time, a chance to recover
    if (processTerminationInterval > HostingConfig::GetConfig().AzureVnetPluginActivationExceptionInterval)
    {
        WriteInfo(
            TraceType,
            "Azure vnet plugin process has not terminated in the last {0} minutes. Resetting retry count.",
            HostingConfig::GetConfig().AzureVnetPluginActivationExceptionInterval);

        retryCount_ = 0;
    }

    retryCount_++;
    if (retryCount_ > HostingConfig::GetConfig().AzureVnetPluginActivationMaxFailureCount)
    {
        Assert::CodingError("Unable to start azure vnet plugin process.");
    }

    CleanupTimer();

    int64 delay = HostingConfig::GetConfig().AzureVnetPluginActivationRetryBackoffInterval.Ticks * retryCount_;

    WriteInfo(
        TraceType,
        "Retrying request to start azure vnet plugin. Attempt: {0} Delay(ticks): {1}",
        retryCount_,
        delay);

    AcquireExclusiveLock lock(this->get_StateLock());
    auto state = this->State.get_Value_CallerHoldsLock();
    if (state == FabricComponentState::Opened)
    {
        AcquireExclusiveLock lock(retryTimerLock_);
        retryTimer_ = Timer::Create(
            "Hosting.AzureVnetPluginProcessManager.Restart",
            [this](TimerSPtr const & timer)
        {
            timer->Cancel();
            this->RestartAzureVnetPlugin();
        });
        retryTimer_->Change(TimeSpan::FromTicks(delay));
    }
}

void AzureVnetPluginProcessManager::RestartAzureVnetPlugin()
{
    auto operation = AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        HostingConfig::GetConfig().AzureVnetPluginProcessManagerInitTimeout,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnProcessStartCompleted(operation, false);
        },
        this->Root.CreateAsyncOperationRoot());

    this->OnProcessStartCompleted(operation, true);
}

void AzureVnetPluginProcessManager::OnProcessStartCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = AsyncOperation::End(operation);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "Azure vnet plugin process started after process terminated. ErrorCode={0} Retry count {1}",
        error,
        retryCount_);
}

void AzureVnetPluginProcessManager::CleanupTimer()
{
    AcquireExclusiveLock lock(retryTimerLock_);
    if (retryTimer_ != nullptr)
    {
        retryTimer_->Cancel();
    }
}

void AzureVnetPluginProcessManager::ShutdownAzureVnetPlugin()
{
    WriteInfo(
        TraceType,
        "Shutting down azure vnet plugin process.");

    // Kill azure-vnet-plugin process
    auto shutdownCommand = wformatString("killall {0}", HostingConfig::GetConfig().AzureVnetPluginName);
    auto shutdownCommandUtf8 = StringUtility::Utf16ToUtf8(shutdownCommand);
    system(shutdownCommandUtf8.c_str());

    // Remove the azure vnet sock file if it exists
    auto removeSockFileCommand = wformatString("rm -f {0}", HostingConfig::GetConfig().AzureVnetPluginSockPath);
    auto removeSockFileCommandUtf8 = StringUtility::Utf16ToUtf8(removeSockFileCommand);
    system(removeSockFileCommandUtf8.c_str());
}
