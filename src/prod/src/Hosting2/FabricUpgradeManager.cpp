// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;

StringLiteral const Trace_FabricUpgradeManager("FabricUpgradeManager");

class FabricUpgradeManager::DownloadForFabricUpgradeAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DownloadForFabricUpgradeAsyncOperation)

public:
    DownloadForFabricUpgradeAsyncOperation(
        FabricUpgradeManager & owner,
        FabricVersion const & fabricVersion,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        fabricVersion_(fabricVersion)
    {
    }

    virtual ~DownloadForFabricUpgradeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadForFabricUpgradeAsyncOperation>(operation);        
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {        
        FabricVersionInstance currentFabricVersionInstance = owner_.hosting_.FabricNodeConfigObj.NodeVersion;        
        if(currentFabricVersionInstance.Version == fabricVersion_)
        {
            WriteInfo(
                Trace_FabricUpgradeManager,
                owner_.Root.TraceId,
                "DownloadForFabricUpgradeAsyncOperation: FabricVersion:{0} is already running. Skipping Download.",
                fabricVersion_);
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        WriteInfo(
            Trace_FabricUpgradeManager,
            owner_.Root.TraceId,
            "Begin(DownloadForFabricUpgrade): {0}",
            fabricVersion_);

        auto operation = owner_.fabricUpgradeImpl_->BeginDownload(
            fabricVersion_,
            [this](AsyncOperationSPtr const & operation){ this->FinishDownload(operation, false); },
            thisSPtr);
        FinishDownload(operation, true);
    }

    void FinishDownload(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fabricUpgradeImpl_->EndDownload(operation);

        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            Trace_FabricUpgradeManager,
            owner_.Root.TraceId,
            "End(DownloadForFabricUpgrade): {0}. Error: {1}.",
            fabricVersion_,
            error);        

        TryComplete(operation->Parent, error);
    }    

private:
    FabricUpgradeManager & owner_;
    FabricVersion const fabricVersion_;
};

class FabricUpgradeManager::ValidateFabricUpgradeAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ValidateFabricUpgradeAsyncOperation)

public:
    ValidateFabricUpgradeAsyncOperation(
        FabricUpgradeManager & owner,
        FabricUpgradeSpecification const & fabricUpgradeSpec,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        fabricUpgradeSpec_(fabricUpgradeSpec),
        operationGuid_(Guid::NewGuid().ToString()),
        shouldRestartReplica_(false),
        healthPropertyId_(wformatString("FabricUpgradeValidation:{0}", fabricUpgradeSpec.Version))
    {
    }

    virtual ~ValidateFabricUpgradeAsyncOperation()
    {
    }

    static ErrorCode End(__out bool & shouldRestartReplica, AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ValidateFabricUpgradeAsyncOperation>(operation);
        shouldRestartReplica = thisPtr->shouldRestartReplica_;
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        {
            // TODO: Bug# 1136788 - Implement TransitionCallerHolds_Lock method in StateMachine.h
            AcquireExclusiveLock lock(owner_.lock_);

            error = owner_.TransitionToValidating();
            if(!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }

            OperationStatus status;
            error = owner_.pendingOperations_->Start(operationGuid_, 0, thisSPtr, status);
        }

        if(!error.IsSuccess())
        {
            CompleteValidate(thisSPtr, error);
            return;
        }

        FabricVersionInstance currentFabricVersionInstance = owner_.hosting_.FabricNodeConfigObj.NodeVersion;
        FabricVersionInstance targetFabricVersionInstance(
            FabricVersion(fabricUpgradeSpec_.Version.CodeVersion, fabricUpgradeSpec_.Version.ConfigVersion),
            fabricUpgradeSpec_.InstanceId);
        if(currentFabricVersionInstance == targetFabricVersionInstance)
        {
            WriteInfo(
                Trace_FabricUpgradeManager,
                owner_.Root.TraceId,
                "ValidateFabricUpgradeAsyncOperation: FabricVersionInstance:{0} is already running.",
                targetFabricVersionInstance);
            CompleteValidate(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        WriteInfo(
            Trace_FabricUpgradeManager,
            owner_.Root.TraceId,
            "Begin(ValidateAndAnalyzeFabricUpgradeImpl): {0}",
            fabricUpgradeSpec_,
            error);

        auto operation = owner_.fabricUpgradeImpl_->BeginValidateAndAnalyze(
            currentFabricVersionInstance,
            fabricUpgradeSpec_,
            [this](AsyncOperationSPtr const & operation){ this->FinishValidateFabricUpgrade(operation, false); },
            thisSPtr);

        FinishValidateFabricUpgrade(operation, true);
    }

    void FinishValidateFabricUpgrade(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fabricUpgradeImpl_->EndValidateAndAnalyze(shouldRestartReplica_, operation);

        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            Trace_FabricUpgradeManager,
            owner_.Root.TraceId,
            "End(ValidateAndAnalyzeFabricUpgradeImpl): {0}. ShouldRestartReplica: {1}. Error: {2}.",
            fabricUpgradeSpec_,
            shouldRestartReplica_,
            error);

        if(!error.IsSuccess())
        {
            this->ReportHealth(error);
        }

        CompleteValidate(operation->Parent, error);
    }

    void ReportHealth(ErrorCode const & error)
    {
        owner_.hosting_.HealthManagerObj->ReportHealthWithTTL(
            healthPropertyId_,
            SystemHealthReportCode::Hosting_FabricUpgradeValidationFailed,
            error.Message,
            SequenceNumber::GetNext(),
            Reliability::FailoverConfig::GetConfig().FabricUpgradeValidateRetryInterval + Client::ClientConfig::GetConfig().HealthReportSendInterval);
    }

    void CompleteValidate(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        owner_.pendingOperations_->Remove(operationGuid_, 0);
        {
            AcquireExclusiveLock lock(owner_.lock_);
            if(owner_.pendingOperations_->IsEmpty())
            {
                owner_.TransitionToIdle();
            }
        }

        TryComplete(thisSPtr, error);
    }

private:
    FabricUpgradeManager & owner_;
    FabricUpgradeSpecification const fabricUpgradeSpec_;
    wstring const healthPropertyId_;
    bool shouldRestartReplica_;

    wstring operationGuid_;
};

class FabricUpgradeManager::FabricUpgradeAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(FabricUpgradeAsyncOperation)

public:
    FabricUpgradeAsyncOperation(
        FabricUpgradeManager & owner,
        FabricUpgradeSpecification const & fabricUpgradeSpec,
        bool const shouldRestart,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        fabricUpgradeSpec_(fabricUpgradeSpec),
        shouldRestart_(shouldRestart),
        configNotificationHandler_(),
        operationGuid_(Guid::NewGuid().ToString()),
        healthPropertyId_(wformatString("FabricUpgrade:{0}", fabricUpgradeSpec.Version))
    {
    }

    virtual ~FabricUpgradeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<FabricUpgradeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToUpgrading();
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        ASSERT_IF(!owner_.pendingOperations_->IsEmpty(), "There should be no pending operations when Upgrading");

        OperationStatus status;
        error = owner_.pendingOperations_->Start(
            operationGuid_,
            0,
            thisSPtr,
            status);
        if(!error.IsSuccess())
        {
            CompleteUpgrade(thisSPtr, error);
            return;
        }

        FabricVersionInstance currentFabricVersionInstance = owner_.hosting_.FabricNodeConfigObj.NodeVersion;
        FabricVersionInstance targetFabricVersionInstance(
            FabricVersion(fabricUpgradeSpec_.Version.CodeVersion, fabricUpgradeSpec_.Version.ConfigVersion),
            fabricUpgradeSpec_.InstanceId);
        if(currentFabricVersionInstance == targetFabricVersionInstance)
        {
            WriteInfo(
                Trace_FabricUpgradeManager,
                owner_.Root.TraceId,
                "FabricUpgradeAsyncOperation: FabricVersionInstance:{0} is already running.",
                targetFabricVersionInstance);
            CompleteUpgrade(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        WriteInfo(
            Trace_FabricUpgradeManager,
            owner_.Root.TraceId,
            "Begin(FabricUpgradeImpl): CurrentVersion={0}, TargetVersion={1}.",
            currentFabricVersionInstance,
            targetFabricVersionInstance);

        HHandler handle = this->owner_.hosting_.FabricNodeConfigObj.NodeVersionEntry.AddHandler(
            [this, thisSPtr] (EventArgs const&) { this->NodeVersionSettingsUpdateHandler(thisSPtr); }) ;
        configNotificationHandler_ = make_unique<ResourceHolder<HHandler>>(
            move(handle),
            [this](ResourceHolder<HHandler> * thisPtr)
        {
            this->owner_.hosting_.FabricNodeConfigObj.NodeVersionEntry.RemoveHandler(thisPtr->Value);
        });

        auto operation = owner_.fabricUpgradeImpl_->BeginUpgrade(
            currentFabricVersionInstance,
            fabricUpgradeSpec_,
            [this](AsyncOperationSPtr const & operation){ this->FinishFabricUpgrade(operation, false); },
            thisSPtr);
        FinishFabricUpgrade(operation, true);
    }

    void FinishFabricUpgrade(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fabricUpgradeImpl_->EndUpgrade(operation);

        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            Trace_FabricUpgradeManager,
            owner_.Root.TraceId,
            "End(FabricUpgradeImpl): {0}. Error: {1}.",
            fabricUpgradeSpec_,
            error);

        if(!error.IsSuccess())
        {
            this->ReportHealth(error);
            CompleteUpgrade(operation->Parent, error);
        }
    }

    void NodeVersionSettingsUpdateHandler(AsyncOperationSPtr const & thisSPtr)
    {
        auto upgradeAsyncOperation = static_cast<FabricUpgradeAsyncOperation*>(thisSPtr.get());
        FabricVersionInstance updatedFabricVersionInstance = upgradeAsyncOperation->owner_.hosting_.FabricNodeConfigObj.NodeVersion;

        ErrorCode error(ErrorCodeValue::Success);
        if(updatedFabricVersionInstance.Version != fabricUpgradeSpec_.Version || updatedFabricVersionInstance.InstanceId != fabricUpgradeSpec_.InstanceId)
        {
            error = ErrorCodeValue::FabricNotUpgrading;
        }

        auto fabricVersionInstance = FabricVersionInstance(fabricUpgradeSpec_.Version, fabricUpgradeSpec_.InstanceId);
        if(error.IsSuccess())
        {
            hostingTrace.NodeVersionUpgraded(owner_.Root.TraceId, owner_.hosting_.NodeId, fabricVersionInstance, updatedFabricVersionInstance);
        }
        else
        {
            hostingTrace.NodeVersionUpgradeFailed(owner_.Root.TraceId, owner_.hosting_.NodeId, fabricVersionInstance, updatedFabricVersionInstance, error);
        }

        if(error.IsSuccess() && shouldRestart_)
        {
            WriteInfo(
                Trace_FabricUpgradeManager,
                owner_.Root.TraceId,
                "Upgrade {0} completed successfully. Exiting the fabric process since ShouldRestart flag is set to true.",
                fabricUpgradeSpec_);

            ::ExitProcess(ProcessActivator::ProcessAbortExitCode);
        }

        CompleteUpgrade(thisSPtr, error);
    }

    void ReportHealth(ErrorCode const & error)
    {
        owner_.hosting_.HealthManagerObj->ReportHealthWithTTL(
            healthPropertyId_,
            SystemHealthReportCode::Hosting_FabricUpgradeFailed,
            error.Message,
            SequenceNumber::GetNext(),
            Reliability::FailoverConfig::GetConfig().FabricUpgradeUpgradeRetryInterval + Client::ClientConfig::GetConfig().HealthReportSendInterval);
    }

    void CompleteUpgrade(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        owner_.pendingOperations_->Remove(operationGuid_, 0);

        owner_.TransitionToIdle();

        TryComplete(thisSPtr, error);
    }

    virtual void OnCompleted()
    {
        if(configNotificationHandler_)
        {
            configNotificationHandler_.reset();
        }

        AsyncOperation::OnCompleted();
    }

private:
    FabricUpgradeManager & owner_;
    FabricUpgradeSpecification const fabricUpgradeSpec_;
    wstring const healthPropertyId_;
    bool const shouldRestart_;

    unique_ptr<ResourceHolder<HHandler>> configNotificationHandler_;
    wstring operationGuid_;
};

FabricUpgradeManager::FabricUpgradeManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    StateMachine(Inactive),
    hosting_(hosting),
    fabricUpgradeImpl_(),
    pendingOperations_(),
    lock_()
{
    pendingOperations_ = make_unique<PendingOperationMap>();
}

FabricUpgradeManager::~FabricUpgradeManager()
{
}

ErrorCode FabricUpgradeManager::Open()
{
    auto error = this->TransitionToStarting();
    if(error.IsSuccess())
    {
        if(this->testFabricUpgradeImpl_)
        {
            WriteNoise(
                Trace_FabricUpgradeManager,
                Root.TraceId,
                "Test FabricUpgradeImpl found.");
            fabricUpgradeImpl_ = move(testFabricUpgradeImpl_);
        }
        else
        {
            fabricUpgradeImpl_ = make_shared<FabricUpgradeImpl>(this->Root, this->hosting_);
        }

        error = this->TransitionToIdle();
    }

    return error;
}

ErrorCode FabricUpgradeManager::Close()
{
    ErrorCode error = this->TransitionToClosing();
    if(error.IsSuccess())
    {
        this->OnAbort();
        error = this->TransitionToClosed();
    }

    return error;
}

void FabricUpgradeManager::OnAbort()
{
    vector<AsyncOperationSPtr> pendingAsyncOperations = this->pendingOperations_->Close();
    for(auto iter = pendingAsyncOperations.cbegin(); iter != pendingAsyncOperations.cend(); ++iter)
    {
        (*iter)->Cancel();
    }

    WriteInfo(
        Trace_FabricUpgradeManager,
        Root.TraceId,
        "Close/Abort called on FabricUpgradeManager. Cancelled {0} pending operations",
        pendingAsyncOperations.size());
}

AsyncOperationSPtr FabricUpgradeManager::BeginDownloadFabric(
    FabricVersion const & fabricVersion,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadForFabricUpgradeAsyncOperation>(
        *this,
        fabricVersion,
        callback,
        parent);
}

ErrorCode FabricUpgradeManager::EndDownloadFabric(
    AsyncOperationSPtr const & operation)
{
    return DownloadForFabricUpgradeAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricUpgradeManager::BeginValidateFabricUpgrade(
    FabricUpgradeSpecification const & fabricUpgradeSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ValidateFabricUpgradeAsyncOperation>(
        *this,
        fabricUpgradeSpec,
        callback,
        parent);
}

ErrorCode FabricUpgradeManager::EndValidateFabricUpgrade(
    __out bool & shouldRestartReplica,
    AsyncOperationSPtr const & operation)
{
    return ValidateFabricUpgradeAsyncOperation::End(
        shouldRestartReplica,
        operation);
}

AsyncOperationSPtr FabricUpgradeManager::BeginFabricUpgrade(
    FabricUpgradeSpecification const & fabricUpgradeSpec,
    bool const shouldRestart,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<FabricUpgradeAsyncOperation>(
        *this,
        fabricUpgradeSpec,
        shouldRestart,
        callback,
        parent);
}

ErrorCode FabricUpgradeManager::EndFabricUpgrade(
    AsyncOperationSPtr const & operation)
{
    return FabricUpgradeAsyncOperation::End(operation);
}

bool FabricUpgradeManager::Test_SetFabricUpgradeImpl(
    IFabricUpgradeImplSPtr const & testFabricUpgradeImpl)
{
    AcquireReadLock lock(this->Lock);
    if(this->GetState_CallerHoldsLock() == Inactive)
    {
        WriteInfo(
            Trace_FabricUpgradeManager,
            Root.TraceId,
            "setting testFabricUpgradeImpl");
        testFabricUpgradeImpl_ = testFabricUpgradeImpl;
        return true;
    }

    WriteInfo(
        Trace_FabricUpgradeManager,
        Root.TraceId,
        "not setting testFabricUpgradeImpl");

    return false;

}
