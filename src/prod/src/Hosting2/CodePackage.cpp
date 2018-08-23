// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("CodePackage");

class CodePackage::UpdateContextAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UpdateContextAsyncOperation)

public:
    UpdateContextAsyncOperation(
        __in CodePackage & owner,
        RolloutVersion const & updatedCodePackageRolloutVersion,
        ServicePackageVersionInstance const & updatedServicePackageVersionInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        updatedCodePackageRolloutVersion_(updatedCodePackageRolloutVersion),
        timeoutHelper_(timeout)
    {
        existingContext_ = owner_.context_;

        updatedContext_ = CodePackageContext(
            owner_.context_.CodePackageInstanceId,
            owner_.context_.CodePackageInstanceSeqNum, 
            owner_.context_.ServicePackageInstanceSeqNum,
            updatedServicePackageVersionInstance,
            owner_.context_.ApplicationName);
    }

    virtual ~UpdateContextAsyncOperation() 
    { 
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpdateContextAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode transitionResult = owner_.TransitionToUpdating();
        if(transitionResult.IsSuccess())
        {
            this->DoUpdate(thisSPtr);
        }
        else
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "CodePackage failed to update. ErrorCode={0}. {1}:{2}",
                transitionResult,
                owner_.id_,
                owner_.instanceId_);

            TryComplete(thisSPtr, transitionResult);
        }
    }

private:
    void DoUpdate(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IFNOT(owner_.mainEntryPointCodePackageInstance_, "MainEntryPointCodePackageInstance must not be null.");

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(UpdateContext): {0}:{1}.",
            owner_.id_,
            owner_.instanceId_);

        auto error = owner_.WriteConfigPackagePolicies(true);

        if (!error.IsSuccess())
        {
            auto transitionResult = owner_.TransitionToFailed();
            if (!transitionResult.IsSuccess())
            {
                error = transitionResult;
            }
            TryComplete(thisSPtr, error);
            return;
        }

        auto operation = owner_.mainEntryPointCodePackageInstance_->BeginUpdateContext(
            updatedContext_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishUpdateCompleted(operation, false); },
            thisSPtr);

        this->FinishUpdateCompleted(operation, true);
    }

    void FinishUpdateCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        ASSERT_IFNOT(owner_.mainEntryPointCodePackageInstance_, "MainEntryPointCodePackageInstance must not be null.");

        ErrorCode result = owner_.mainEntryPointCodePackageInstance_->EndUpdateContext(operation);

        WriteTrace(
            result.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            owner_.TraceId,
            "End(UpdateContext): ErrorCode={0}. {1}:{2}.",
            result,
            owner_.id_,
            owner_.instanceId_);

        ErrorCode transitionResult;
        if(result.IsSuccess())
        {
            auto error = owner_.Hosting.FabricRuntimeManagerObj->UpdateRegistrationsFromCodePackage(existingContext_, updatedContext_);

            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                owner_.TraceId,
                "UpdateRegistrationsFromCodePackage: ExistingContext={0}, UpdatedContext={1}, ErrorCode={2}. {3}:{4}",
                existingContext_,
                updatedContext_,
                error,
                owner_.id_,
                owner_.instanceId_);

            owner_.context_ = updatedContext_;
            owner_.rollOutVersion_ = updatedCodePackageRolloutVersion_;

            transitionResult = owner_.TransitionToEntryPoint_Initialized();
        }
        else
        {
            transitionResult = owner_.TransitionToFailed();
        }

        if(!transitionResult.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "UpdateContext {0} failed to transition to the right state becuase of {1}. CurrentState: {2}. {3}:{4}",
                updatedContext_,
                transitionResult,
                owner_.StateToString(owner_.GetState()),
                owner_.id_,
                owner_.instanceId_);

            result = transitionResult;
        }

        TryComplete(operation->Parent, result);
    }

private:
    CodePackage & owner_;    
    TimeoutHelper timeoutHelper_;
    CodePackageContext existingContext_;
    CodePackageContext updatedContext_;
    RolloutVersion updatedCodePackageRolloutVersion_;
};

class CodePackage::RestartCodePackageInstanceAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    RestartCodePackageInstanceAsyncOperation(
        CodePackage & codePackage,
        int64 const instanceIdOfCodePackageInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        timeoutHelper_(timeout),
        owner_(codePackage),
        instanceIdOfCodePackageInstance_(instanceIdOfCodePackageInstance)
    {
    }

    static ErrorCode RestartCodePackageInstanceAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<RestartCodePackageInstanceAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {   
        uint64 previousState;
        auto error = owner_.TransitionToEntryPoint_Restarting(previousState);
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CodePackageInstanceSPtr entryPointSPtr;
        error = owner_.GetEntryPoint(instanceIdOfCodePackageInstance_, entryPointSPtr);
        if(!error.IsSuccess())
        {
            this->RevertToPreviousState(previousState);
            TryComplete(thisSPtr, error);
            return;
        }

        // Transition to EntryPoint_Started state to allow transition to Aborted state if needed.
        // EntryPoint stop is a long running operation and an Abort transition should not be blocked till it completes.
        error = owner_.TransitionToEntryPoint_Restarted();
        if(!error.IsSuccess())
        {
            // CodePackage is Aborted
            TryComplete(thisSPtr, error);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(StopCodePackageInstance): {0}:{1}.",
            owner_.id_,
            owner_.instanceId_);

        auto operation = entryPointSPtr->BeginStop(
            timeoutHelper_.GetRemainingTime(),
            [this, entryPointSPtr](AsyncOperationSPtr const & operation) { this->FinishStopEntryPoint(operation, false, entryPointSPtr); },
            thisSPtr);

        this->FinishStopEntryPoint(operation, true, entryPointSPtr);
    }

private:   
    void FinishStopEntryPoint(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously, CodePackageInstanceSPtr entryPointSPtr)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        entryPointSPtr->EndStop(operation);

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(StopCodePackageInstance): {0}:{1}",
            owner_.id_,
            owner_.instanceId_);
        
        TryComplete(operation->Parent, ErrorCode::Success());
    }

    void RevertToPreviousState(uint64 const previousState)
    {
        // If there is not matching CodePackage to restart, we will revert to the previous state
        // We should handle every state from which we can transition into "EntryPoint_Restarting"
        // If transtion to previous state fails the CodePackage should have been aborted
        ErrorCode error;
        if(previousState == Setup_Initialized)
        {
            error = owner_.TransitionToSetup_Initialized();
        }
        else if(previousState == EntryPoint_Initialized)
        {
            error = owner_.TransitionToEntryPoint_Initialized();
        }
        else
        {
            Assert::CodingError("PreviousState={0} is not expected.", previousState);
        }

        if(!error.IsSuccess())
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "CodePackage failed to revert to previous state of '{0}' from EntryPoint_Restarting. Error:{1}",
                previousState,
                error);
        }
    }

private:
    int64 const instanceIdOfCodePackageInstance_;
    TimeoutHelper timeoutHelper_;
    CodePackage & owner_;
    FileChangeMonitor2SPtr lockFileMonitor_;
};

class CodePackage::GetContainerInfoAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    GetContainerInfoAsyncOperation(
        CodePackage & codePackage,
        int64 const instanceIdOfCodePackageInstance,
        wstring const & containerInfoType,
        wstring const & containerInfoArgs,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        timeoutHelper_(timeout),
        owner_(codePackage),
        instanceIdOfCodePackageInstance_(instanceIdOfCodePackageInstance),
        containerInfoType_(containerInfoType),
        containerInfoArgs_(containerInfoArgs)
    {
    }

    static ErrorCode GetContainerInfoAsyncOperation::End(
        AsyncOperationSPtr const & operation, __out wstring & containerInfo)
    {
        auto thisPtr = AsyncOperation::End<GetContainerInfoAsyncOperation>(operation);
        containerInfo = thisPtr->containerInfo_;
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        CodePackageInstanceSPtr entryPointSPtr;
        auto error = owner_.GetEntryPoint(instanceIdOfCodePackageInstance_, entryPointSPtr);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        if (entryPointSPtr->EntryPoint.EntryPointType != EntryPointType::ContainerHost)
        {
            TryComplete(thisSPtr, error);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(GetContainerLogs): {0}:{1}.",
            owner_.id_,
            owner_.instanceId_);

        auto operation = entryPointSPtr->BeginGetContainerInfo(
            containerInfoType_,
            containerInfoArgs_,
            timeoutHelper_.GetRemainingTime(),
            [this, entryPointSPtr](AsyncOperationSPtr const & operation) { this->FinishGetContainerInfo(operation, false, entryPointSPtr); },
            thisSPtr);
        this->FinishGetContainerInfo(operation, true, entryPointSPtr);
    }

private:
    void FinishGetContainerInfo(
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynhronously, 
        CodePackageInstanceSPtr entryPointSPtr)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        entryPointSPtr->EndGetContainerInfo(operation, containerInfo_);

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(GetContainerLogs): {0}:{1}",
            owner_.id_,
            owner_.instanceId_);

        TryComplete(operation->Parent, ErrorCode::Success());
    }

private:
    int64 const instanceIdOfCodePackageInstance_;
    wstring const & containerInfoType_;
    wstring const & containerInfoArgs_;
    TimeoutHelper timeoutHelper_;
    CodePackage & owner_;
    wstring containerInfo_;
};

class CodePackage::DeactivateAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    DeactivateAsyncOperation(
        CodePackage & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        timeoutHelper_(timeout),
        owner_(codePackage)
    {
    }

    static void DeactivateAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {       
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
                owner_.TraceId,
                "CodePackage failed to deactivate. ErrorCode: {0}. {1}:{2}",
                transitionResult,
                owner_.id_,
                owner_.instanceId_);

            TransitionToAborted(thisSPtr);
        }
    }

private:
    void DoDeactivate(AsyncOperationSPtr const & thisSPtr)
    {    
        ASSERT_IFNOT(owner_.mainEntryPointCodePackageInstance_, "MainEntryPointCodePackageInstance must not be null.");
        
        owner_.CleanupTimer();

        {
            AcquireWriteLock lock(owner_.Lock);

            this->mainCodePackageInstance_ = move(owner_.mainEntryPointCodePackageInstance_);
            if(owner_.setupEntryPointCodePackageInstance_)
            {
                owner_.setupEntryPointCodePackageInstance_.reset();
            }
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(DeactivateMainCodePackageInstance): {0}:{1}.",
            owner_.id_,
            owner_.instanceId_);

        auto operation = mainCodePackageInstance_->BeginStop(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishStopMainEntryPoint(operation, false); },
            thisSPtr);

        FinishStopMainEntryPoint(operation, true);
    }

    void FinishStopMainEntryPoint(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        mainCodePackageInstance_->EndStop(operation);

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(DeactivateMainCodePackageInstance): {0}:{1}",
            owner_.id_,
            owner_.instanceId_);

        TransitionToDeactivated(operation->Parent);
    }

    void TransitionToDeactivated(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.setupEntryPointStats_.Close();
        owner_.mainEntryPointStats_.Close();

        auto error = owner_.TransitionToDeactivated();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr);
        }
        else
        {
            owner_.NotifyMainEntryPointDeactivation(mainCodePackageInstance_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
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
    TimeoutHelper timeoutHelper_;
    CodePackage & owner_;
    CodePackageInstanceSPtr mainCodePackageInstance_;
};

class CodePackage::ApplicationHostCodePackageOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ApplicationHostCodePackageOperation(
        __in CodePackage & owner,
        ApplicationHostCodePackageOperationRequest const & request,
        int64 requestorInstanceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , request_(request)
        , requestorInstanceId_(requestorInstanceId)
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
        uint64 currentState;
        CodePackageInstanceSPtr mainEntryPoint;

        owner_.GetMainEntryPointAndCurrentState(mainEntryPoint, currentState);
        
        if (currentState == CodePackage::Aborted ||
            currentState >= CodePackage::Deactivating)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackageOperation: Ignoring {0} as current state is '{1}'.",
                request_,
                owner_.StateToString(currentState));

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        if (mainEntryPoint == nullptr)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackageOperation: Ignoring {0} as main entry point is null. Current state is '{1}'.",
                request_,
                owner_.StateToString(currentState));

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        if (mainEntryPoint->InstanceId != requestorInstanceId_)
        {
            // Stale request.

            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackageOperation: Ignoring {0} as RequestorInstanceId={1} mismatched with CurrentInstanceId={2}.",
                request_,
                requestorInstanceId_,
                mainEntryPoint->InstanceId);

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InstanceIdMismatch));
            return;
        }

        auto operation = owner_.VersionedServicePackageObj.BeginApplicationHostCodePackageOperation(
            request_,
            requestorInstanceId_,
            [this](AsyncOperationSPtr const & operation)
            {
                OnApplicationHostCodePackageOperationCompleted(operation, false);
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

        auto error = owner_.VersionedServicePackageObj.EndApplicationHostCodePackageOperation(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "OnApplicationHostCodePackageOperationCompleted: ErrorCode={0}",
            error);

        this->TryComplete(operation->Parent, error);
    }

private:
    CodePackage & owner_;
    ApplicationHostCodePackageOperationRequest request_;
    int64 requestorInstanceId_;
};

CodePackage::RunStats::RunStats(
    ComponentRoot const & root, 
    CodePackage & codePackage, 
    bool isSetupEntryPoint)
    : RootedObject(root),
    stats_(),
    statsLock_(),
    codePackage_(codePackage),
    activationHealthPropertyId_(move(wformatString("CodePackageActivation:{0}:{1}:{2}", codePackage.id_.CodePackageName, isSetupEntryPoint ? L"SetupEntryPoint" : L"EntryPoint", codePackage.instanceId_))),
    healthCheckStatusHealthPropertyId_(move(wformatString("DockerHealthCheckStatus:{0}:{1}:{2}", codePackage.id_.CodePackageName, isSetupEntryPoint ? L"SetupEntryPoint" : L"EntryPoint", codePackage.instanceId_))),
    lastReportedActivationHealth_(FABRIC_HEALTH_STATE_INVALID),
    lastReportedHealthCheckStatusHealth_(FABRIC_HEALTH_STATE_INVALID),
    lastReportedHealthCheckStatusTimeInSec_(0),
    isClosed_(true)
{
}

CodePackageEntryPointStatistics CodePackage::RunStats::get_Stats()
{
    {
        AcquireReadLock lock(statsLock_);
        return stats_;
    }    
}

ContainerHealthConfigDescription const & CodePackage::RunStats::get_HealthConfig() const
{
    return codePackage_.Description.ContainerPolicies.HealthConfig;
}

void CodePackage::RunStats::UpdateDockerHealthCheckStatusStats(
    ContainerHealthStatusInfo const & healthStatusInfo,
    int64 instanceId)
{
    auto shouldReportHealth = false;
    auto shouldRestart = false;

    {
        AcquireWriteLock lock(statsLock_);

        if (isClosed_)
        {
            return;
        }

        if (lastReportedHealthCheckStatusTimeInSec_ > healthStatusInfo.TimeStampInSeconds)
        {
            WriteNoise(
                TraceType,
                Root.TraceId,
                "RunStats::UpdateDockerHealthCheckStatusStats(): Received stale health report: LastReportedTime={0}, StaleReport={1}. IsHealthy={2}.",
                lastReportedHealthCheckStatusTimeInSec_,
                healthStatusInfo,
                healthStatusInfo.IsHealthy);
            
            // Stale health event.
            return;
        }

        WriteNoise(
            TraceType,
            Root.TraceId,
            "RunStats::UpdateDockerHealthCheckStatusStats(): Received new health report: LastReportedTime={0}, NewReport={1}. IsHealthy={2}.",
            lastReportedHealthCheckStatusTimeInSec_,
            healthStatusInfo,
            healthStatusInfo.IsHealthy);

        auto newHealthState = healthStatusInfo.IsHealthy ? FABRIC_HEALTH_STATE_OK : FABRIC_HEALTH_STATE_WARNING;

        shouldReportHealth = (lastReportedHealthCheckStatusHealth_ != newHealthState);
        shouldRestart = (!(healthStatusInfo.IsHealthy) && HealthConfig.RestartContainerOnUnhealthyDockerHealthStatus);

        lastReportedHealthCheckStatusHealth_ = newHealthState;
        lastReportedHealthCheckStatusTimeInSec_ = healthStatusInfo.TimeStampInSeconds;        
    }

    if (shouldReportHealth)
    {
        auto reportCode = healthStatusInfo.IsHealthy ? 
            SystemHealthReportCode::Hosting_DockerHealthCheckStatusHealthy : 
            SystemHealthReportCode::Hosting_DockerHealthCheckStatusUnhealthy;

        wstring healthDescription(
            move(wformatString(
                "Docker HEALTHCHECK reported health_status={0}. ContainerName={1}, TimeStamp={2}.",
                healthStatusInfo.GetDockerHealthStatusString(),
                healthStatusInfo.ContainerName,
                healthStatusInfo.GetTimeStampAsUtc())));

        if (shouldRestart)
        {
            healthDescription = wformatString("{0}{1}{2}", healthDescription, "Restarting the container as per ", HealthConfig);
        }

        codePackage_.Hosting.HealthManagerObj->ReportHealth(
            codePackage_.CodePackageInstanceId.ServicePackageInstanceId,
            healthCheckStatusHealthPropertyId_,
            reportCode,
            healthDescription,
            SequenceNumber::GetNext());
    }

    if (shouldRestart)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "RunStats::UpdateDockerHealthCheckStatusStats(): Restarting the container. CodePackageInstanceId={0}, HealthReport={1}",
            codePackage_.CodePackageInstanceId,
            healthStatusInfo);

        codePackage_.VersionedServicePackageObj.OnDockerHealthCheckUnhealthy(codePackage_.Description.Name, instanceId);
    }
}

void CodePackage::RunStats::UpdateProcessActivationStats(ErrorCode const & error)
{    
    bool shouldReportHealth = false;
    SystemHealthReportCode::Enum reportCode;
    FABRIC_SEQUENCE_NUMBER sequenceNumber = 0;

    {
        AcquireWriteLock lock(statsLock_);

        if(isClosed_)
        {
            return;
        }

        this->stats_.LastActivationTime = DateTime::Now();
        this->stats_.ActivationCount++;

        if (error.IsSuccess())
        {
            this->stats_.LastSuccessfulActivationTime = this->stats_.LastActivationTime;
            this->stats_.ContinuousActivationFailureCount = 0;
        }
        else
        {
            this->stats_.ActivationFailureCount++;
            this->stats_.ContinuousActivationFailureCount++;
        }

        this->GetHealth_CallerHoldsLock(shouldReportHealth, reportCode, sequenceNumber);
    }

    if(shouldReportHealth)
   {   
        codePackage_.Hosting.HealthManagerObj->ReportHealth(
            codePackage_.CodePackageInstanceId.ServicePackageInstanceId,
            activationHealthPropertyId_,
            reportCode,
            error.Message,
            sequenceNumber);
    }    
}

void CodePackage::RunStats::UpdateProcessExitStats(DWORD exitCode, bool ignoreReporting)
{
    bool shouldReportHealth = false;
    SystemHealthReportCode::Enum reportCode = SystemHealthReportCode::Hosting_CodePackageActivated;
    FABRIC_SEQUENCE_NUMBER sequenceNumber = 0;

    {
        AcquireWriteLock lock(statsLock_);

        if(isClosed_)
        {
            return;
        }

        if(timer_)
        {
            timer_->Cancel();
            timer_ = nullptr;
        }

        this->stats_.LastExitCode = exitCode;
        this->stats_.LastExitTime = DateTime::Now();
        this->stats_.ExitCount++;

        if (exitCode == 0 || exitCode == ProcessActivator::ProcessDeactivateExitCode)
        {
            this->stats_.LastSuccessfulExitTime = this->stats_.LastExitTime;
            this->stats_.ContinuousExitFailureCount = 0;
        }
        else
        {
            this->stats_.ContinuousExitFailureCount++;
            this->stats_.ExitFailureCount++;

            auto root = Root.CreateComponentRoot();
            timer_ = Timer::Create(
                "Hosting.ContinousExitFailureReset", [root, this] (TimerSPtr const & timer) { OnContinousExitFailureResetTimeout(timer, this->stats_.ExitCount); }, false);      

            timer_->Change(HostingConfig::GetConfig().CodePackageContinuousExitFailureResetInterval);
        }
        if (!ignoreReporting)
        {
            this->GetHealth_CallerHoldsLock(shouldReportHealth, reportCode, sequenceNumber);
        }
    }

    if(shouldReportHealth)
    {
        codePackage_.Hosting.HealthManagerObj->ReportHealth(
            codePackage_.CodePackageInstanceId.ServicePackageInstanceId,
            activationHealthPropertyId_,
            reportCode,
            wformatString("{0}{1}", StringResource::Get(IDS_HOSTING_ServiceHostTerminated), exitCode),
            sequenceNumber);
    }
}

void CodePackage::RunStats::OnContinousExitFailureResetTimeout(TimerSPtr const & timer, ULONG exitCount)
{
    UNREFERENCED_PARAMETER(timer);

    bool shouldReportHealth = false;
    SystemHealthReportCode::Enum reportCode;
    FABRIC_SEQUENCE_NUMBER sequenceNumber = 0;

    {
        AcquireWriteLock lock(statsLock_);

        if(isClosed_)
        {
            return;
        }

        if(this->stats_.ExitCount != exitCount)
        {
            // The exit count has changed which indicates that the
            // process has exited again
            return;
        }

        if(timer_)
        {
            timer_->Cancel();
            timer_ = nullptr;
        }

        this->stats_.ContinuousExitFailureCount = 0;

        this->GetHealth_CallerHoldsLock(shouldReportHealth, reportCode, sequenceNumber);
    }

    if(shouldReportHealth)
    {                
        codePackage_.Hosting.HealthManagerObj->ReportHealth(
            codePackage_.CodePackageInstanceId.ServicePackageInstanceId,
            activationHealthPropertyId_,
            reportCode,
            L"" /*extraDescription*/,
            sequenceNumber);
    }
}

void CodePackage::RunStats::GetHealth_CallerHoldsLock(
    __out bool & shouldReportHealth, 
    __out SystemHealthReportCode::Enum & reportCode, 
    __out FABRIC_SEQUENCE_NUMBER & sequenceNumber)
{
    shouldReportHealth = false;
    FABRIC_HEALTH_STATE healthState;
    sequenceNumber = -1;
    ULONG maxContinousFailureCount = GetMaxContinuousFailureCount_CallerHoldsLock();

    if(maxContinousFailureCount < HostingConfig::GetConfig().CodePackageHealthWarningThreshold)
    {
        healthState = FABRIC_HEALTH_STATE_OK;
        reportCode = SystemHealthReportCode::Hosting_CodePackageActivated;
    }
    else if(maxContinousFailureCount >= HostingConfig::GetConfig().CodePackageHealthWarningThreshold && maxContinousFailureCount < HostingConfig::GetConfig().CodePackageHealthErrorThreshold)
    {
        healthState = FABRIC_HEALTH_STATE_WARNING;
        reportCode = SystemHealthReportCode::Hosting_CodePackageActivationWarning;
    }
    else
    {
        healthState = FABRIC_HEALTH_STATE_ERROR;
        reportCode = SystemHealthReportCode::Hosting_CodePackageActivationError;
    }

    if(lastReportedActivationHealth_ != healthState)
    {
        shouldReportHealth = true;
        lastReportedActivationHealth_ = healthState;
        sequenceNumber = SequenceNumber::GetNext();
    }    
}

ULONG CodePackage::RunStats::GetMaxContinuousFailureCount()
{
    {
        AcquireReadLock lock(statsLock_);
        return GetMaxContinuousFailureCount_CallerHoldsLock();
    }
}

ULONG CodePackage::RunStats::GetMaxContinuousFailureCount_CallerHoldsLock()
{
    if (this->stats_.ContinuousExitFailureCount > this->stats_.ContinuousActivationFailureCount)
    {
        return this->stats_.ContinuousExitFailureCount;
    }
    else
    {
        return this->stats_.ContinuousActivationFailureCount;
    } 
}

ErrorCode CodePackage::RunStats::Open()
{ 
    {
        AcquireWriteLock lock(statsLock_);
        isClosed_ = false;
    }

    auto error = codePackage_.Hosting.HealthManagerObj->RegisterSource(
        codePackage_.CodePackageInstanceId.ServicePackageInstanceId,
        codePackage_.Context.ApplicationName,
        activationHealthPropertyId_);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "RunStats::Open(): Failed to register health property id '{0}'. CodePackageInstanceId={1}, Error={2}.",
            activationHealthPropertyId_,
            codePackage_.CodePackageInstanceId,
            error);

        return error;
    }

    error = codePackage_.Hosting.HealthManagerObj->RegisterSource(
        codePackage_.CodePackageInstanceId.ServicePackageInstanceId,
        codePackage_.Context.ApplicationName,
        healthCheckStatusHealthPropertyId_);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "RunStats::Open(): Failed to register health property id '{0}'. CodePackageInstanceId={1}, Error={2}.",
            healthCheckStatusHealthPropertyId_,
            codePackage_.CodePackageInstanceId,
            error);
    }

    return error;
}

void CodePackage::RunStats::Close()
{    
    {
        AcquireWriteLock lock(statsLock_);

        isClosed_ = true;

        if(timer_)
        {
            timer_->Cancel();
            timer_ = nullptr;
        }
    }

    codePackage_.Hosting.HealthManagerObj->UnregisterSource(
        codePackage_.CodePackageInstanceId.ServicePackageInstanceId,
        activationHealthPropertyId_);

    codePackage_.Hosting.HealthManagerObj->UnregisterSource(
        codePackage_.CodePackageInstanceId.ServicePackageInstanceId,
        healthCheckStatusHealthPropertyId_);
}

void CodePackage::RunStats::WriteTo(TextWriter& w, FormatOptions const & options) const
{
    this->stats_.WriteTo(w, options);
}

CodePackage::CodePackage(
    HostingSubsystemHolder const & hostingHolder,
    VersionedServicePackageHolder const & versionedServicePackageHolder,
    DigestedCodePackageDescription const & description,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    int64 const servicePackageInstanceSeqNum,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    wstring const & applicationName,
    RolloutVersion rolloutVersion,
    ServicePackageInstanceEnvironmentContextSPtr const & envContext,
    int64 activatorCodePackageInstanceId,
    Common::EnvironmentMap && onDemandActivationEnvironment,
    wstring const & runAsId,
    wstring const & setupRunas,
    bool isShared,
    ProcessDebugParameters const & debugParameters,
    map<wstring,wstring> const & portBindings,
    bool removeServiceFabricRuntimeAccess,
    vector<GuestServiceTypeInfo> const & guestServiceTypes)
    : ComponentRoot(),
    StateMachine(Inactive),
    hostingHolder_(hostingHolder),
    versionedServicePackageHolder_(versionedServicePackageHolder),
    context_(),
    id_(CodePackageInstanceIdentifier(servicePackageInstanceId, description.Name)),
    instanceId_(hostingHolder.Value.GetNextSequenceNumber()),
    description_(description),
    envContext_(envContext),
    activatorCodePackageInstanceId_(activatorCodePackageInstanceId),
    onDemandActivationEnvironment_(move(onDemandActivationEnvironment)),
    runAsId_(runAsId),
    setupRunas_(setupRunas),
    rollOutVersion_(rolloutVersion),
    setupEntryPointStats_(*this, *this, true),
    mainEntryPointStats_(*this, *this, false),
    setupEntryPointCodePackageInstance_(),
    mainEntryPointCodePackageInstance_(),
    mainEntryPointRunInterval_(TimeSpan::Zero),
    isShared_(isShared),
    debugParameters_(debugParameters),
    portBindings_(portBindings),
    retryTimer_(),
    isImplicitTypeHost_(description.Name == Constants::ImplicitTypeHostCodePackageName),
    guestServiceTypes_(guestServiceTypes),
    timerLock_(),
    lockFileMonitor_(),
    randomGenerator_((int)(instanceId_ % 100000)),
    removeServiceFabricRuntimeAccess_(removeServiceFabricRuntimeAccess),
    conifgPackagePoliciesBindMounts_(),
    envVariblesMountPoints_()
{
    ASSERT_IF(
        isImplicitTypeHost_ == true && guestServiceTypes_.size() == 0,
        "CodePackage hosting guest service type must be supplied on or more guest service types. CodePackageInstanceId={0}.",
        id_);
    
    this->SetTraceId(hostingHolder_.Root->TraceId);
    WriteNoise(
        TraceType, 
        TraceId, 
        "CodePackage.constructor: {0} ({1}:{2})", 
        static_cast<void*>(this),
        id_,
        instanceId_);

    context_ = CodePackageContext(
        id_, 
        instanceId_, 
        servicePackageInstanceSeqNum,
        servicePackageVersionInstance,
        applicationName);

    if(description_.CodePackage.EntryPoint.EntryPointType == EntryPointType::Exe)
    {
        mainEntryPointRunInterval_ = TimeSpan::FromSeconds(description_.CodePackage.EntryPoint.ExeEntryPoint.PeriodicIntervalInSeconds);
    }
}

CodePackage::~CodePackage()
{
    WriteNoise(
        TraceType, 
        TraceId, 
        "CodePackage.destructor: {0} ({1}:{2})", 
        static_cast<void*>(this),
        id_,
        instanceId_);
}

void CodePackage::SetDebugParameters(ProcessDebugParameters const & debugParameters)
{
    this->debugParameters_ = debugParameters;
}

DeployedCodePackageQueryResult CodePackage::GetDeployedCodePackageQueryResult()
{       
    CodePackageInstanceSPtr mainCodePackageInstance, setupCodePackageInstance;
    DeploymentStatus::Enum status;
    {
        AcquireReadLock lock(this->Lock);
        if (this->mainEntryPointCodePackageInstance_)
        {
            mainCodePackageInstance = mainEntryPointCodePackageInstance_;
        }

        if (this->setupEntryPointCodePackageInstance_)
        {
            setupCodePackageInstance = setupEntryPointCodePackageInstance_;
        }        

        uint64 currentState = this->GetState_CallerHoldsLock();
        if(currentState & (Inactive|Setup_Initializing|Setup_Initialized|EntryPoint_Initializing|EntryPoint_Initialized|EntryPoint_Restarting|EntryPoint_Restarted))
        {
            status = DeploymentStatus::Active;
        }
        else if(currentState == Updating)
        {
            status = DeploymentStatus::Upgrading;
        }
        else if(currentState & (Deactivating|Deactivated|Failed|Aborted))
        {
            status = DeploymentStatus::Deactivating;
        }
        else
        {
            Assert::CodingError("CodePackage: State={0} should be mapped to FABRIC_DEPLOYMENT_STATUS", currentState);
        }
    }

    CodePackageEntryPoint setupEntryPoint, mainEntryPoint;
    if (mainCodePackageInstance)
    {
        mainEntryPoint = mainCodePackageInstance->GetCodePackageEntryPoint();
    }
    else
    {
        mainEntryPoint = CodePackageEntryPoint(
            L"",
            -1,
            this->RunAsId,
            DateTime::Zero,
            EntryPointStatus::Stopped,
            mainEntryPointStats_.Stats,
            0);
    }

    if (setupCodePackageInstance)
    {
        setupEntryPoint = setupCodePackageInstance->GetCodePackageEntryPoint();
    }
    else if(description_.CodePackage.HasSetupEntryPoint)
    {
        setupEntryPoint = CodePackageEntryPoint(
            L"",
            -1,
            this->SetupRunAs,
            DateTime::Zero,
            EntryPointStatus::Stopped,
            setupEntryPointStats_.Stats,
            0);
    }

    HostType::Enum hostType;
    HostIsolationMode::Enum hostIsolationMode;
    this->GetHostTypeAndIsolationMode(hostType, hostIsolationMode);

    wstring serviceName;

    if (this->ServicePackageInstanceId.ActivationContext.IsExclusive)
    {
        auto res = this->Hosting.TryGetExclusiveServicePackageServiceName(
            this->ServicePackageInstanceId.ServicePackageId,
            this->ServicePackageInstanceId.ActivationContext,
            serviceName);

        ASSERT_IF(res == false, "ServiceName must be present for exlusive service package activation.");
    }

    if (this->CodePackageInstanceId.ServicePackageInstanceId.ApplicationId.IsSystem())
    {
        return DeployedCodePackageQueryResult(
            this->description_.Name, 
            this->Hosting.FabricNodeConfigObj.NodeVersion.Version.CodeVersion.ToString(), 
            this->id_.ServicePackageInstanceId.ServicePackageName,
            this->id_.ServicePackageInstanceId.PublicActivationId,
            hostType,
            hostIsolationMode,
            status,
            (ULONG)mainEntryPointRunInterval_.TotalSeconds(),
            this->description_.CodePackage.HasSetupEntryPoint,
            move(mainEntryPoint),
            move(setupEntryPoint),
            serviceName);
    }
    else
    {
        return DeployedCodePackageQueryResult(
            this->description_.CodePackage.Name, 
            this->description_.CodePackage.Version, 
            this->id_.ServicePackageInstanceId.ServicePackageName,
            this->id_.ServicePackageInstanceId.PublicActivationId,
            hostType,
            hostIsolationMode,
            status,
            (ULONG)mainEntryPointRunInterval_.TotalSeconds(),
            this->description_.CodePackage.HasSetupEntryPoint,
            move(mainEntryPoint),
            move(setupEntryPoint),
            serviceName);
    }
}

void CodePackage::GetHostTypeAndIsolationMode(HostType::Enum & hostType, HostIsolationMode::Enum & hostIsolationMode)
{
    hostType = HostType::FromEntryPointType(description_.CodePackage.EntryPoint.EntryPointType);

    hostIsolationMode = HostIsolationMode::None;

    if (hostType == HostType::ContainerHost)
    {
        hostIsolationMode = HostIsolationMode::FromContainerIsolationMode(description_.ContainerPolicies.Isolation);
    }
}

TimeSpan CodePackage::GetDueTime(CodePackageEntryPointStatistics stats, TimeSpan runInterval)
{   
    DateTime nowTime = DateTime::Now();
    TimeSpan dueTime = TimeSpan::Zero;

    if(stats.ActivationCount == 0)
    {
        return TimeSpan::Zero;
    }

    LONG failureCounts = stats.ContinuousExitFailureCount + stats.ContinuousActivationFailureCount;

    WriteNoise(
        TraceType,
        TraceId,
        "Calculating due time with FailureCount={0} and RunInterval={1}. {2}:{3}",
        failureCounts,
        runInterval,
        id_,
        instanceId_);

    if (failureCounts == 0)
    {
        if(runInterval != TimeSpan::Zero)
        {            
            // run periodic task at the multiple of next period from the last execution time
            DateTime nextTime = stats.LastActivationTime + runInterval;
            while(nextTime < nowTime)
            {
                nextTime = nextTime + runInterval;
            };
            dueTime = TimeSpan::FromTicks(nextTime.Ticks - nowTime.Ticks);
        }
    }
    else
    {   //
        // If there is a failure, even periodic entry point is run similar to continuous.
        // To calculate exponential back-off interval we pick 1.5 as the base so that the 
        // delay is not very large for the first 4-5 attempts.
        //
        // Add randomization within range of 5 sec for the corner case scenario where if a
        // node has multiple bad behaving code packages then all code package do not get
        // scheduled for start together.
        //
        auto exponentiation = (int64)pow(HostingConfig::GetConfig().ActivationRetryBackoffExponentiationBase, failureCounts);
        auto backoffinterval = HostingConfig::GetConfig().ActivationRetryBackoffInterval.TotalSeconds();
        auto retrySeconds = backoffinterval * exponentiation;
        if (retrySeconds > HostingConfig::GetConfig().ActivationMaxRetryInterval.TotalSeconds())
        {
            retrySeconds = HostingConfig::GetConfig().ActivationMaxRetryInterval.TotalSeconds();
        }

        retrySeconds += randomGenerator_.Next(5);

        dueTime = TimeSpan::FromSeconds(static_cast<double>(retrySeconds));
    }

    return dueTime;
}

ULONG CodePackage::GetMaxContinuousFailureCount()
{
    auto maxSetupEntryPointFailure = this->setupEntryPointStats_.GetMaxContinuousFailureCount();
    auto maxMainEntryPointFailure = this->mainEntryPointStats_.GetMaxContinuousFailureCount();
    return  max(maxSetupEntryPointFailure, maxMainEntryPointFailure);
}

AsyncOperationSPtr CodePackage::BeginActivate(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);
    
    WriteNoise(
        TraceType,
        TraceId,
        "Activating CodePackage {0},{1}",
        description_.CodePackage.Name,
        description_.CodePackage.Version);

    auto error = this->TransitionToStats_Initializing();

    if(error.IsSuccess())
    {
        error = this->mainEntryPointStats_.Open();
        if(error.IsSuccess())
        {
            error = this->setupEntryPointStats_.Open();
        }
    }    

    if(error.IsSuccess())
    {
        error = WriteConfigPackagePolicies();
        if (error.IsSuccess())
        {
            error = this->InitializeSetupEntryPoint();
        }
    }
    else
    {
        this->TransitionToFailed();
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        error,
        callback,
        parent);
}

ErrorCode CodePackage::EndActivate(
    AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr CodePackage::BeginUpdateContext(
    RolloutVersion const & newCodePackageRolloutVersion,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateContextAsyncOperation>(
        *this,
        newCodePackageRolloutVersion,
        servicePackageVersionInstance,
        timeout,
        callback, 
        parent);
}

ErrorCode CodePackage::EndUpdateContext(AsyncOperationSPtr const & operation)
{
    return UpdateContextAsyncOperation::End(operation);
}

AsyncOperationSPtr CodePackage::BeginRestartCodePackageInstance(
    int64 instanceIdOfCodePackageInstance,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RestartCodePackageInstanceAsyncOperation>(
        *this,
        instanceIdOfCodePackageInstance,
        timeout,
        callback, 
        parent);
}

ErrorCode CodePackage::EndRestartCodePackageInstance(AsyncOperationSPtr const & operation)
{
    return RestartCodePackageInstanceAsyncOperation::End(operation);
}

AsyncOperationSPtr CodePackage::BeginGetContainerInfo(
    int64 instanceIdOfCodePackageInstance,
    wstring const & containerInfoTypeArgument,
    wstring const & containerInfoArgsArgument,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetContainerInfoAsyncOperation>(
        *this,
        instanceIdOfCodePackageInstance,
        containerInfoTypeArgument,
        containerInfoArgsArgument,
        timeout,
        callback,
        parent);
}

ErrorCode CodePackage::EndGetContainerInfo(AsyncOperationSPtr const & operation, __out wstring & containerInfo)
{
    return GetContainerInfoAsyncOperation::End(operation, containerInfo);
}

AsyncOperationSPtr CodePackage::BeginDeactivate(
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

void CodePackage::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    DeactivateAsyncOperation::End(operation);
}

AsyncOperationSPtr CodePackage::BeginApplicationHostCodePackageOperation(
    ApplicationHostCodePackageOperationRequest const & request,
    int64 requestorInstanceId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ApplicationHostCodePackageOperation>(
        *this,
        request,
        requestorInstanceId,
        callback,
        parent);
}

ErrorCode CodePackage::EndApplicationHostCodePackageOperation(
    AsyncOperationSPtr const & operation)
{
    return ApplicationHostCodePackageOperation::End(operation);
}

void CodePackage::OnAbort()
{
    CleanupTimer();
    CodePackageInstanceSPtr setupCodePackageInstance, mainCodePackageInstance;

    {
        AcquireWriteLock lock(this->Lock);
        setupCodePackageInstance = move(setupEntryPointCodePackageInstance_);
        mainCodePackageInstance = move(mainEntryPointCodePackageInstance_);
    }

    if(setupCodePackageInstance)
    {
        setupCodePackageInstance->AbortAndWaitForTermination();
    }

    if(mainCodePackageInstance)
    {
        mainCodePackageInstance->AbortAndWaitForTermination();
        this->NotifyMainEntryPointDeactivation(mainCodePackageInstance);
    }

    this->setupEntryPointStats_.Close();
    this->mainEntryPointStats_.Close();
}

ErrorCode CodePackage::InitializeSetupEntryPoint()
{
    ErrorCode transitionResult = this->TransitionToSetup_Initializing();
    if (!transitionResult.IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "CodePackage cannot transition to Setup_Initializing state. ErrorCode={0}. Current State={1}. {2}:{3}",
            transitionResult,
            StateToString(this->GetState()),
            id_,
            instanceId_);

        return transitionResult;
    }

    {
        AcquireWriteLock lock(this->Lock);
        setupEntryPointCodePackageInstance_.reset();
        mainEntryPointCodePackageInstance_.reset();
    }

    if(description_.CodePackage.HasSetupEntryPoint)
    {
        EntryPointDescription setupEntryPoint;
        setupEntryPoint.EntryPointType = EntryPointType::Exe;
        setupEntryPoint.ExeEntryPoint = description_.CodePackage.SetupEntryPoint;

        auto setupEntryPointCodePackageInstance = make_shared<CodePackageInstance>(
            CodePackageHolder(*this, this->CreateComponentRoot()),
            setupEntryPoint,
            [this](ErrorCode error, int64 instanceId) { this->OnSetupEntryPointStarted(error, instanceId); },
            [this](DWORD exitCode, int64 instanceId) { this->OnSetupEntryPointTerminated(exitCode, instanceId); },
            false /* isShared */,
            true /* isSetupEntryPoint */,
            false /* IsActivator */,
            TimeSpan::Zero,
            setupEntryPointStats_,
            setupRunas_);

        {
            AcquireWriteLock lock(this->Lock);
            setupEntryPointCodePackageInstance_ = move(setupEntryPointCodePackageInstance);
        }

        setupEntryPointCodePackageInstance_->BeginStart(
            [this](AsyncOperationSPtr const & operation) { this->FinishSetupEntryPointStartCompleted(operation); },
            this->CreateAsyncOperationRoot());
    }
    else
    {
        WriteInfo(
            TraceType,
            TraceId,
            "CodePackage has no SetupEntryPoint. Skipping SetupEntryPoint and starting the MainEntryPoint. {0}:{1}",
            id_,
            instanceId_);
    }

    transitionResult = this->TransitionToSetup_Initialized();
    if(!transitionResult.IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "CodePackage cannot transition to Setup_Initialized state. ErrorCode={0}. Current State={1}. {2}:{3}",         
            transitionResult,
            StateToString(this->GetState()),
            id_,
            instanceId_);

        return transitionResult;
    }

    if(!description_.CodePackage.HasSetupEntryPoint)
    {
        transitionResult = this->InitializeMainEntryPoint();
    }

    return transitionResult;
}

void CodePackage::FinishSetupEntryPointStartCompleted(AsyncOperationSPtr const & operation)
{
    ErrorCode result = setupEntryPointCodePackageInstance_->EndStart(operation);
    ASSERT_IFNOT(
        result.IsSuccess(),
        "The CodePackageInstance of setup entrypoint has to be initialized successfully.");
}

ErrorCode CodePackage::InitializeMainEntryPoint()
{
    ErrorCode transitionResult = this->TransitionToEntryPoint_Initializing();
    if(!transitionResult.IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "CodePackage cannot transition to EntryPoint_Initializing state. ErrorCode={0}. Current State={1}. {2}:{3}",
            transitionResult,
            StateToString(this->GetState()),
            id_,
            instanceId_);
        return transitionResult;
    }

    {
        AcquireWriteLock lock(this->Lock);
        setupEntryPointCodePackageInstance_.reset();
    }

    auto mainEntryPointCodePackageInstance = make_shared<CodePackageInstance>(
        CodePackageHolder(*this, this->CreateComponentRoot()),
        description_.CodePackage.EntryPoint,
        [this](ErrorCode error, int64 instanceId) { this->OnMainEntryPointStarted(error, instanceId); },
        [this] (DWORD exitCode, int64 instanceId) { this->OnMainEntryPointTerminated(exitCode, instanceId); },
        description_.CodePackage.IsShared,
        false /* isSetupEntryPoint */,
        this->IsActivator,
        mainEntryPointRunInterval_,
        this->mainEntryPointStats_,
        this->RunAsId); 

    {
        AcquireWriteLock lock(this->Lock);
        mainEntryPointCodePackageInstance_ = move(mainEntryPointCodePackageInstance);

        if (this->IsActivator)
        {
            VersionedServicePackageObj.OnActivatorCodePackageInitialized(
                mainEntryPointCodePackageInstance_->InstanceId);
        }
    }

    mainEntryPointCodePackageInstance_->BeginStart(
        [this](AsyncOperationSPtr const & operation) { this->FinishMainEntryPointStartCompleted(operation); },
        this->CreateAsyncOperationRoot());
    transitionResult = this->TransitionToEntryPoint_Initialized();
    if(!transitionResult.IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "CodePackage cannot transition to EntryPoint_Initialized state. ErrorCode={0}. Current State={1}. {2}:{3}",
            transitionResult,
            StateToString(this->GetState()),
            id_,
            instanceId_);
    }

    return transitionResult;
}

void CodePackage::FinishMainEntryPointStartCompleted(AsyncOperationSPtr const & operation)
{
    ErrorCode result = mainEntryPointCodePackageInstance_->EndStart(operation);
    ASSERT_IFNOT(
        result.IsSuccess(),
        "The CodePackageInstance of entrypoint has to be initialized successfully.");
}

void CodePackage::CleanupTimer()
{
    AcquireExclusiveLock grabLock(timerLock_);
    if (this->retryTimer_)
    {
        this->retryTimer_->Cancel();
        this->retryTimer_.reset();
    }
}

void CodePackage::RestartTerminatedCodePackageInstance(int64 instanceId)
{
    if (HostingConfig::GetConfig().EnableProcessDebugging && !debugParameters_.LockFile.empty())
    {
        if (File::Exists(debugParameters_.LockFile))
        {
            WriteInfo(
                TraceType,
                TraceId,
                "InitializeSetupEntryPoint: Wait for deleting debug lock file. {0}:{1}",
                id_,
                instanceId_);

            lockFileMonitor_ = FileChangeMonitor2::Create(debugParameters_.LockFile, TRUE);
            lockFileMonitor_->RegisterFailedEventHandler(
                [this, instanceId](ErrorCode const &)
                {
                    this->lockFileMonitor_->Close();
                    this->RestartTerminatedCodePackageInstance(instanceId);
                });

            lockFileMonitor_->Open(
                [this, instanceId](FileChangeMonitor2SPtr const & monitor)
                {
                    monitor->Close();
                    this->RestartTerminatedCodePackageInstance(instanceId);
                });

            return; // ignore error code. Failure will raise FailedEvent.
        }
    }
    
    CodePackageInstanceSPtr instance = nullptr;
    {
        AcquireWriteLock lock(Lock);
        instance = this->mainEntryPointCodePackageInstance_;
        if (instance == nullptr)
        {
            instance = this->setupEntryPointCodePackageInstance_;
        }
    }

    if (instance != nullptr &&
        instanceId != instance->InstanceId)
    {
        WriteInfo(
            TraceType,
            TraceId,
            "Cannot restart CodePackage instance because instanceId {0} is different from current instanceId {1}",
            instanceId,
            instance->InstanceId);
        return;
    }

    auto error = this->InitializeSetupEntryPoint();
    if (!error.IsSuccess())
    {        
        auto thisSPtr = this->CreateAsyncOperationRoot();
        TimeSpan dueTime = HostingConfig::GetConfig().ActivationRetryBackoffInterval;
        WriteWarning(
            TraceType,
            TraceId,
            "Could not restart termininated entry point because of error {0}, retry scheduled after  {1} ",
            error,
            dueTime);
        
        {
            AcquireExclusiveLock grabLock(timerLock_);

            //Dont retry if closing, aborting or failed
            if(this->GetState() & (Aborted | Deactivated | Deactivating | Failed))
            {
                return;
            }

            this->retryTimer_ = Timer::Create(
                "Hosting.RestartTerminatedCodePackage",
                [this, instanceId](Common::TimerSPtr const & timer)
                {
                    timer->Cancel();
                    this->RestartTerminatedCodePackageInstance(instanceId);
                },
                false);
            retryTimer_->Change(dueTime);
        }        
    }
}

void CodePackage::OnSetupEntryPointStarted(ErrorCode error, int64 instanceId)
{
    UNREFERENCED_PARAMETER(error);
    UNREFERENCED_PARAMETER(instanceId);

    // NO-OP
}

void CodePackage::OnSetupEntryPointTerminated(DWORD exitCode, int64 instanceId)
{
    WriteTrace(
        ((exitCode == 0) ? LogLevel::Info : LogLevel::Warning),
        TraceType,
        TraceId,
        "SetupEntryPoint:{0} terminated with exitcode {1}. {2}:{3}",
        description_.CodePackage.SetupEntryPoint,
        exitCode,
        id_,
        instanceId_);

    ErrorCode errorCode = ErrorCode::FromWin32Error(exitCode);
    if(errorCode.IsSuccess())
    {
        ErrorCode transitionResult = this->InitializeMainEntryPoint();
    }
    else
    {
        RestartCodePackage(instanceId);
    }
}

void CodePackage::OnMainEntryPointStarted(ErrorCode error, int64 instanceId)
{
    if (!this->VersionedServicePackageObj.IsOnDemandActivationEnabled || this->IsActivator)
    {
        //
        // No need to notify if on-demand activation is
        // not enabled or if this CP is an activator CP.
        //
        return;
    }

    uint64 currentState;
    CodePackageInstanceSPtr mainEntryPoint;
    this->GetMainEntryPointAndCurrentState(mainEntryPoint, currentState);

    if (mainEntryPoint == nullptr || mainEntryPoint->InstanceId != instanceId)
    {
        WriteNoise(
            TraceType,
            TraceId,
            "OnMainEntryPointStarted: Skipping notification. CodePackageInstanceId={0}.",
            id_);
        return;
    }

    map<wstring, wstring> properties;
    properties.insert(make_pair(CodePackageEventProperties::Version, this->Version));

    auto eventType = CodePackageEventType::Started;
    if (!error.IsSuccess())
    {
        eventType = CodePackageEventType::StartFailed;
        properties.insert(make_pair(CodePackageEventProperties::ErrorMessage, error.TakeMessage()));

        auto failureCount = this->VersionedServicePackageObj.GetFailureCount();
        properties.insert(make_pair(CodePackageEventProperties::FailureCount, wformatString("{0}", failureCount)));
    }

    CodePackageEventDescription eventDesc(
        this->CodePackageInstanceId.CodePackageName,
        mainEntryPoint->IsSetupEntryPoint,
        mainEntryPoint->IsContainerHost,
        eventType,
        DateTime::Now(),
        this->Hosting.GetNextSequenceNumber(),
        properties);

    this->VersionedServicePackageObj.ProcessDependentCodePackageEvent(
        move(eventDesc),
        this->ActivatorCodePackageInstanceId);
}

void CodePackage::SendDependentCodePackageEvent(
    CodePackageEventDescription const & eventDesc,
    int64 targetActivatorCodePackageInstanceId)
{
    uint64 currentState;
    CodePackageInstanceSPtr mainEntryPoint;
    this->GetMainEntryPointAndCurrentState(mainEntryPoint, currentState);

    if (currentState == CodePackage::Aborted ||
        currentState >= CodePackage::Deactivating)
    {
        WriteNoise(
            TraceType,
            TraceId,
            "SendDependentCodePackageEvent: Ignoring Event={0} for TargetInstanceId={1} as CurrentSate={2}. CodePackageInstanceId={3}",
            eventDesc,
            targetActivatorCodePackageInstanceId,
            this->StateToString(currentState),
            this->CodePackageInstanceId);
        return;
    }

    if (mainEntryPoint == nullptr)
    {
        WriteNoise(
            TraceType,
            TraceId,
            "SendDependentCodePackageEvent: Ignoring Event={0} for TargetInstanceId={1} as MainEntryPoint is null. CodePackageInstanceId={2}",
            eventDesc,
            targetActivatorCodePackageInstanceId,
            this->CodePackageInstanceId);
        return;
    }

    if (mainEntryPoint->InstanceId != targetActivatorCodePackageInstanceId)
    {
        WriteNoise(
            TraceType,
            TraceId,
            "SendDependentCodePackageEvent: Ignoring Event={0} as InstanceId mismatched (Current={1}, Target={2}). CodePackageInstanceId={3}",
            eventDesc,
            mainEntryPoint->InstanceId,
            targetActivatorCodePackageInstanceId,
            this->CodePackageInstanceId);
        return;
    }

    mainEntryPoint->SendDependentCodePackageEvent(eventDesc);
}

void CodePackage::OnMainEntryPointTerminated(DWORD exitCode, int64 instanceId)
{
    WriteTrace(
        ((exitCode == 0 || exitCode == ProcessActivator::ProcessDeactivateExitCode || exitCode == STATUS_CONTROL_C_EXIT) ? LogLevel::Info : LogLevel::Warning),
        TraceType,
        TraceId,
        "MainEntryPoint:{0} terminated with exitcode {1}. {2}:{3}",
        description_.CodePackage.EntryPoint,
        exitCode,
        id_,
        instanceId_);

    if (this->VersionedServicePackageObj.IsOnDemandActivationEnabled)
    {
        this->NotifyMainEntryPointTermination(exitCode, instanceId);
    }
    else
    {
        this->RestartCodePackage(instanceId);
    }
}

void CodePackage::NotifyMainEntryPointDeactivation(CodePackageInstanceSPtr mainEntryPoint)
{
    if (this->IsActivator)
    {
        return;
    }

    map<wstring, wstring> properties;
    properties.insert(make_pair(CodePackageEventProperties::Version, this->Version));

    CodePackageEventDescription eventDesc(
        this->CodePackageInstanceId.CodePackageName,
        mainEntryPoint->IsSetupEntryPoint,
        mainEntryPoint->IsContainerHost,
        CodePackageEventType::Stopped,
        DateTime::Now(),
        this->Hosting.GetNextSequenceNumber(),
        properties);

    this->VersionedServicePackageObj.ProcessDependentCodePackageEvent(
        move(eventDesc),
        this->ActivatorCodePackageInstanceId);
}

void CodePackage::NotifyMainEntryPointTermination(DWORD exitCode, int64 instanceId)
{
    uint64 currentState;
    CodePackageInstanceSPtr mainEntryPoint;
    this->GetMainEntryPointAndCurrentState(mainEntryPoint, currentState);

    if (mainEntryPoint == nullptr || mainEntryPoint->InstanceId != instanceId)
    {
        WriteNoise(
            TraceType,
            TraceId,
            "NotifyMainEntryPointTermination: Skipping. CodePackageInstanceId={0}.",
            id_);
        return;
    }

    if (this->IsActivator)
    {
        auto operation = this->VersionedServicePackageObj.BeginProcessActivatorCodePackageTerminated(
            mainEntryPoint->InstanceId,
            [this, instanceId](AsyncOperationSPtr const & operation)
            { 
                this->FinishNotifyMainEntryPointTermination(operation, false, instanceId);
            },
            this->CreateAsyncOperationRoot());

        this->FinishNotifyMainEntryPointTermination(operation, true, instanceId);
    }
    else
    {
        map<wstring, wstring> properties;
        properties.insert(make_pair(CodePackageEventProperties::Version, this->Version));
        properties.insert(make_pair(CodePackageEventProperties::ExitCode, wformatString("{0}", exitCode)));

        auto failureCount = this->VersionedServicePackageObj.GetFailureCount();
        properties.insert(make_pair(CodePackageEventProperties::FailureCount, wformatString("{0}", failureCount)));

        CodePackageEventDescription eventDesc(
            this->CodePackageInstanceId.CodePackageName,
            mainEntryPoint->IsSetupEntryPoint,
            mainEntryPoint->IsContainerHost,
            CodePackageEventType::Terminated,
            DateTime::Now(),
            this->Hosting.GetNextSequenceNumber(),
            properties);

        this->VersionedServicePackageObj.ProcessDependentCodePackageEvent(
            move(eventDesc),
            this->ActivatorCodePackageInstanceId);

        this->RestartTerminatedCodePackageInstance(instanceId);
    }
}

void CodePackage::FinishNotifyMainEntryPointTermination(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynhronously,
    int64 instanceId)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    this->VersionedServicePackageObj.EndProcessActivatorCodePackageTerminated(operation);
    this->RestartTerminatedCodePackageInstance(instanceId);
}

void CodePackage::RestartCodePackage(int64 instanceId)
{
    if (this->IsImplicitTypeHost)
    {
        RestartTerminatedCodePackageInstance(instanceId);
        return;
    }

    auto operation = this->VersionedServicePackageObj.BeginTerminateFabricTypeHostCodePackage(
        [this](AsyncOperationSPtr const & operation) { this->FinishTerminateFabricTypeHostCodePackage(operation, false); },
        this->CreateAsyncOperationRoot(),
        instanceId);

    this->FinishTerminateFabricTypeHostCodePackage(operation, true);
}

void CodePackage::FinishTerminateFabricTypeHostCodePackage(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynhronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    int64 instanceId = 0;

    this->VersionedServicePackageObj.EndTerminateFabricTypeHostCodePackage(operation, instanceId).ReadValue();

    this->RestartTerminatedCodePackageInstance(instanceId);
}

ErrorCode CodePackage::TerminateCodePackageExternally(__out TimeSpan & retryDueTime)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::Success);
    retryDueTime = TimeSpan::Zero;

    CodePackageInstanceSPtr setupCodePackageInstance, mainCodePackageInstance;

    {
        AcquireReadLock lock(this->Lock);
        setupCodePackageInstance = setupEntryPointCodePackageInstance_;
        mainCodePackageInstance = mainEntryPointCodePackageInstance_;
    }


    if (setupCodePackageInstance)
    {
        error = setupCodePackageInstance->TerminateExternally();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            TraceId,
            "Terminate SetupEntryPoint for CodePackage {0} completed with error {1}",
            this->Description.Name,
            error);

        if (!error.IsSuccess())
        {
            retryDueTime = this->GetDueTime(setupEntryPointStats_.get_Stats(), TimeSpan::Zero);
        }
    }

    if (error.IsSuccess() && mainCodePackageInstance)
    {
        error = mainCodePackageInstance->TerminateExternally();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            TraceId,
            "Terminate MainEntryPoint for CodePackage {0} completed with error {1}",
            this->Description.Name,
            error);

        if (!error.IsSuccess())
        {
            retryDueTime = this->GetDueTime(mainEntryPointStats_.get_Stats(), mainEntryPointRunInterval_);
        }
    }

    return error;
}

void CodePackage::GetMainEntryPointAndCurrentState(
    _Out_ CodePackageInstanceSPtr & mainEntryPoint,
    _Out_ uint64 & currentState)
{
    AcquireReadLock lock(this->Lock);

    mainEntryPoint = mainEntryPointCodePackageInstance_;
    currentState = this->GetState_CallerHoldsLock();
}

ErrorCode CodePackage::GetEntryPoint(
    int64 const instanceIdOfCodePackageInstance, 
    __out CodePackageInstanceSPtr & entryPoint)
{
    AcquireReadLock lock(this->Lock);

    if (this->mainEntryPointCodePackageInstance_)
    {
        entryPoint = this->mainEntryPointCodePackageInstance_;
    }
    else if (this->setupEntryPointCodePackageInstance_)
    {
        entryPoint = this->setupEntryPointCodePackageInstance_;
    }

    if (!entryPoint)
    {
        return ErrorCodeValue::CodePackageNotFound;
    }

    if (instanceIdOfCodePackageInstance != 0 &&
        instanceIdOfCodePackageInstance != entryPoint->InstanceId)
    {
        return ErrorCodeValue::InstanceIdMismatch;
    }

    return ErrorCodeValue::Success;
}

ErrorCode CodePackage::WriteConfigPackagePolicies(bool update)
{
    if (description_.ConfigPackagePolicy.ConfigPackages.empty())
    {
        return ErrorCodeValue::Success;
    }

    //Create Directory inside settiings and mount paths for the config packages
    map<wstring, vector<wstring>> mtPoints;
    wstring settingsDirectory = this->Hosting.RunLayout.GetApplicationSettingsFolder(this->id_.ServicePackageInstanceId.ApplicationId.ToString());

    for (auto configPackagePolicy : description_.ConfigPackagePolicy.ConfigPackages)
    {
        wstring mountPoint = configPackagePolicy.MountPoint;
        if (mountPoint.empty())
        {
            if (this->description_.CodePackage.EntryPoint.EntryPointType != EntryPointType::Enum::ContainerHost)
            {
                mountPoint = HostingConfig::GetConfig().MountPointForSettings;
            }
            else
            {
                mountPoint = HostingConfig::GetConfig().ContainerMountPointForSettings;
            }
        }

        wstring configPackageVerison;
        for (auto digestedConfigPackage : this->VersionedServicePackageObj.PackageDescription.DigestedConfigPackages)
        {
            if (StringUtility::AreEqualCaseInsensitive(digestedConfigPackage.Name, configPackagePolicy.Name))
            {
                configPackageVerison = digestedConfigPackage.ConfigPackage.Version;
            }
        }

        wstring configPackagNameVersion = wformatString("{0}{1}", configPackagePolicy.Name, configPackageVerison);

        wstring relativeSectionPath = Path::Combine(this->id_.ServicePackageInstanceId.ServicePackageName, configPackagNameVersion);
        relativeSectionPath = Path::Combine(relativeSectionPath, configPackagePolicy.SectionName);

        auto it = mtPoints.find(mountPoint);
        if (it != mtPoints.end())
        {
            it->second.push_back(relativeSectionPath);
        }
        else
        {
            vector<wstring> configSections;
            configSections.push_back(relativeSectionPath);
            mtPoints[mountPoint] = move(configSections);
        }

        auto iter = conifgPackagePoliciesBindMounts_.find(mountPoint);

        if (update)
        {
            if (iter == conifgPackagePoliciesBindMounts_.end())
            {
                Assert::CodingError("Mount point {0} shoud be present since Update {1}", mountPoint, update);
            }
        }
        else
        {
            auto guid = Guid::NewGuid().ToString();
            wstring pathInHost;
            if (iter == conifgPackagePoliciesBindMounts_.end())
            {
                bool isContainerHost = (this->description_.CodePackage.EntryPoint.EntryPointType == EntryPointType::Enum::ContainerHost);
                if (!isContainerHost)
                {
                    pathInHost = this->Hosting.RunLayout.GetApplicationWorkFolder(this->id_.ServicePackageInstanceId.ApplicationId.ToString());
                    pathInHost = Path::Combine(pathInHost, mountPoint);
                    pathInHost = Path::Combine(pathInHost, this->id_.ServicePackageInstanceId.ServicePackageName);
                }
                else
                {
                    pathInHost = Path::Combine(settingsDirectory, this->id_.ServicePackageInstanceId.ServicePackageName);
                }

                pathInHost = Path::Combine(pathInHost, this->Description.Name);
                if (isContainerHost)
                {
                    pathInHost = Path::Combine(pathInHost, guid);
                }
                //Add to codepackage name the mount points inside containers
                //Map will be : <MountPointInsideConatiner, HostDirectory>>
                // for containers its: settings\SPName\CodePackageName\Guid
                // for non container hosts its: work\mountPoint\SPName\CodePackageName
                conifgPackagePoliciesBindMounts_[mountPoint] = pathInHost;
            }
            else
            {
                pathInHost = iter->second;
            }

            if (!configPackagePolicy.EnvironmentVariableName.empty())
            {
                auto envIter = envVariblesMountPoints_.find(configPackagePolicy.EnvironmentVariableName);
                if (envIter == envVariblesMountPoints_.end())
                {
                    envVariblesMountPoints_[configPackagePolicy.EnvironmentVariableName] = pathInHost;
                }
            }
        }
    }

   //Copy the settings for each of the conifgPolicy to a single directory to be mounted
    for (auto const& mountPointDirectory : conifgPackagePoliciesBindMounts_)
    {
        wstring copyTo = mountPointDirectory.second;

        if (update)
        {
            //Remove is best effort. Remove the directory and copy updated contents.
            auto error = Directory::Delete(copyTo, true /*recursive*/);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    TraceId,
                    "Remove directory {0} during update failed {1}",
                    error,
                    copyTo);
            }
        }

        auto it = mtPoints.find(mountPointDirectory.first);
        if (it == mtPoints.end())
        {
            Assert::CodingError("Mount Point {0} should be specified ", mountPointDirectory.first);
        }

        for (auto section : it->second)
        {
            wstring copyfrom = Path::Combine(settingsDirectory, section);

            if (!Directory::Exists(copyfrom))
            {
                WriteError(
                    TraceType,
                    TraceId,
                    "Host directory {0} to mount doesn't exist",
                    copyfrom);
                return ErrorCodeValue::NotFound;
            }

            //Directory copy will create the copyTo directory if it doesn't exist.
            auto error = Directory::Copy(copyfrom, copyTo, true /*overWrite*/);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType,
                    TraceId,
                    "Failed to Copy Src Directory {0} to Destination Directory {1} Error:{2}",
                    copyfrom,
                    copyTo,
                    error);
                return error;
            }
            else
            {
                WriteInfo(
                    TraceType,
                    TraceId,
                    "Copy Src Directory:{0} Destination Directory:{1}",
                    copyfrom,
                    copyTo);
            }
        }
    }

    return ErrorCodeValue::Success;
}