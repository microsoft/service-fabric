// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceType_ActivationManager("HostedServiceInstance");

// ********************************************************************************************************************
// HostedServiceInstance::ActivateAsyncOperation Implementation
//
class HostedServiceInstance::ActivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        __in HostedServiceInstance & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~ActivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);
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
                owner_.TraceId,
                "Hosted Service with exe {0} failed to start. ErrorCode={1}",
                owner_.processDescription_.ExePath,
                error);

            TryComplete(thisSPtr, error);
        }
        else
        {
            if(owner_.securityUser_)
            {
                error = owner_.securityUser_->ConfigureAccount();
                if(!error.IsSuccess())
                {
                    WriteWarning(                
                        TraceType_ActivationManager,
                        owner_.TraceId,
                        "HostedService {0} failed to configure security user. ErrorCode={1}. ",           
                        owner_.serviceName_,
                        error);
                    auto transitionError  = this->TransitionState(error);
                    if(!transitionError.IsSuccess())
                    {
                        error.Overwrite(transitionError);
                    }
                    TryComplete(thisSPtr, error);
                    return;
                }
            }
            error = owner_.ConfigurePortSecurity(false);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.TraceId,
                    "Failed to configure port ACLs for service {0}. ErrorCode {1}",
                    owner_.serviceName_,
                    error); 
                auto transitionError  = this->TransitionState(error);
                if(!transitionError.IsSuccess())
                {
                    error.Overwrite(transitionError);
                }
                TryComplete(thisSPtr, error);
                return;
            }
            auto operation = owner_.ActivationManager.ProcessActivatorObj->BeginActivate(owner_.securityUser_,
                owner_.processDescription_,
                L"",
                GetProcessTerminationHandler(),
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
            owner_.TraceId,
            "End(Service Activate): ErrorCode={0}, Service Exe ={1}",
            error,
            owner_.processDescription_.ExePath);

        ErrorCode transitionResult = this->TransitionState(error);

        if(!transitionResult.IsSuccess())
        {
            error = transitionResult;
        }
        TryComplete(operation->Parent, error);   
    }

    ErrorCode TransitionState(ErrorCode startedResult)
    {
        ErrorCode transitionResult;
        if(startedResult.IsSuccess())
        {
            //Get processId before transitioning to started.
            DWORD processId = owner_.activationContext_->ProcessId;
            transitionResult = owner_.TransitionToStarted();

            hostingTrace.HostedServiceActivated(owner_.ServiceName, processId);
        }
        else
        {
            transitionResult = owner_.TransitionToScheduling();                        
            hostingTrace.HostedServiceActivationFailed(owner_.ServiceName, startedResult);
        }

        if(!transitionResult.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.TraceId,
                "Hosted Service {0} has failed to transition to the right state while starting. ErrorCode: {1}",
                owner_.serviceName_,
                transitionResult);
        }

        return transitionResult;
    }

    ProcessWait::WaitCallback GetProcessTerminationHandler()
    {
        auto root = owner_.ActivationManager.Root.CreateComponentRoot();
        wstring serviceName = owner_.ServiceName;
        wstring serviceId = owner_.hostedServiceHolder_.RootedObject.ServiceId;

        HostedServiceActivationManager & controller  = owner_.ActivationManager;
        return ProcessWait::WaitCallback(
            [root, &controller, serviceName, serviceId](pid_t, ErrorCode const & waitResult, DWORD exitCode)
        {
            controller.OnHostedServiceTerminated(waitResult, exitCode, serviceName, serviceId);
        });
    }
private:
    HostedServiceInstance & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// HostedServiceInstance::DeactivateAsyncOperation Implementation
//
class HostedServiceInstance::DeactivateAsyncOperation	: public AsyncOperation
{
public:
    DeactivateAsyncOperation(
        HostedServiceInstance & owner,
        bool gracefulStop,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        gracefulStop_(gracefulStop),
        timeoutHelper_(timeout),
        processId_(0)
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
        if(owner_.GetState() == owner_.Stopped)
        {            
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        ErrorCode transitionResult = owner_.TransitionToStopping();
        if(transitionResult.IsSuccess())
        {
            this->DoStopProcess(thisSPtr);
        }
        else
        {
            // Error in transition to Stopping state
            WriteWarning(
                TraceType_ActivationManager,
                owner_.TraceId,
                "Hosted service: {0} failed to stop. ErrorCode={1}",
                owner_.serviceName_,                
                transitionResult);

            TransitionToAborted(thisSPtr);
        }
    }

private:
    void DoStopProcess(AsyncOperationSPtr const & thisSPtr)
    {
        if(!owner_.activationContext_)
        {
            WriteInfo(
                TraceType_ActivationManager,
                owner_.TraceId,
                "Skipping deactivation since hosted service {0} is not activated",
                owner_.serviceName_);
            TransitionToStopped(thisSPtr);
        }
        else
        {
            processId_ = owner_.activationContext_->ProcessId;

            this->owner_.CleanupTimer();
            if(gracefulStop_)
            {
                WriteNoise(
                    TraceType_ActivationManager,
                    owner_.TraceId,
                    "Deactivate hosted service begin: service name ={0}",
                    owner_.serviceName_);

                auto operation = owner_.ActivationManager.ProcessActivatorObj->BeginDeactivate(
                    owner_.activationContext_,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation){ this->FinishStopCompleted(operation, false); },
                    thisSPtr);       

                FinishStopCompleted(operation, true);
            }
            else
            {
                WriteNoise(
                    TraceType_ActivationManager,
                    owner_.TraceId,
                    "Disabling hosted service: service name ={0}",
                    owner_.serviceName_);
                owner_.DisableServiceInstance();
                TransitionToStopped(thisSPtr);
            }
        }
    }

    void FinishStopCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {            
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto errorCode = owner_.ActivationManager.ProcessActivatorObj->EndDeactivate(operation);

        WriteTrace(
            errorCode.ToLogLevel(),
            TraceType_ActivationManager,
            owner_.TraceId,
            "End(DeactivateHostedService): ErrorCode={0}, service name={1}",
            errorCode,
            owner_.serviceName_);

        if (errorCode.IsSuccess())
        {  
            TransitionToStopped(operation->Parent);
        }
        else
        {
            hostingTrace.HostedServiceDeactivationFailed(owner_.ServiceName, processId_, errorCode);
            TransitionToFailed(operation->Parent);
        }

    }

    void TransitionToStopped(AsyncOperationSPtr const & thisSPtr)
    {
        hostingTrace.HostedServiceDeactivated(owner_.ServiceName, processId_);
        auto transitionResult = owner_.TransitionToStopped();
        if (transitionResult.IsSuccess())
        {
            auto error = owner_.ConfigurePortSecurity(true);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    owner_.TraceId,
                    "Hosted Service: {0} has failed to cleanup port security. ErrorCode: {1}.Ignore and continue",
                    owner_.serviceName_,
                    error);
            }
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        else
        {
            WriteWarning(
                TraceType_ActivationManager,
                owner_.TraceId,
                "Hosted Service: {0} has failed to transition to Stopped state. ErrorCode: {1}.",
                owner_.serviceName_,
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
    TimeoutHelper timeoutHelper_;
    HostedServiceInstance & owner_;
    bool gracefulStop_;
    DWORD processId_;
};

HostedServiceInstance::HostedServiceInstance(
    HostedServiceHolder const & serviceHolder,
    wstring const & serviceName,
    TimeSpan runInterval,
    HostedService::RunStats & stats,
    RwLock & statsLock,
    SecurityUserSPtr const & runAs)
    : StateMachine(Created)
    , hostedServiceHolder_(serviceHolder)
    , serviceName_(serviceName)
    , processDescription_(hostedServiceHolder_.RootedObject.GetProcessDescription())
    , activationContext_()
    , securityUser_(runAs)
    , stats_(stats)
    , runInterval_(runInterval)
    , statsLock_(statsLock)
    , runTimer_()
{
}

HostedServiceInstance::~HostedServiceInstance()
{
}

HostedService const & HostedServiceInstance::get_HostedService() const
{ 
    return this->hostedServiceHolder_.RootedObject; 
}

AsyncOperationSPtr HostedServiceInstance::BeginStart(    
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    ErrorCode transitionResult = this->TransitionToScheduling();
    if(transitionResult.IsSuccess())
    {
        this->ScheduleStart();
    }
    else
    {
        WriteWarning(
            TraceType_ActivationManager,
            TraceId,
            "Hosted service: {0} failed to transition to Scheduling state. CurrentState={1}, ErrorCode={2}.",
            serviceName_,
            StateToString(this->GetState()),
            transitionResult);
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        transitionResult,
        callback,
        parent);
}

ErrorCode HostedServiceInstance::EndStart(AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr HostedServiceInstance::BeginStop(
    bool gracefulStop,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        gracefulStop,
        timeout,
        callback,
        parent);
}

void HostedServiceInstance::EndStop(AsyncOperationSPtr const & operation)
{
    DeactivateAsyncOperation::End(operation);
}

void HostedServiceInstance::ScheduleStart()
{
    this->CleanupTimer();

    TimeSpan dueTime = this->hostedServiceHolder_.RootedObject.GetDueTime(this->GetRunStats(), this->runInterval_);

    auto thisSPtr = this->CreateAsyncOperationRoot();
    runTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](Common::TimerSPtr const &) { this->OnScheduledStart(thisSPtr); }, true);	

    ErrorCode transitionResult = this->TransitionToScheduled();

    if(!transitionResult.IsSuccess())
    {
        this->CleanupTimer();

        WriteWarning(
            TraceType_ActivationManager,
            TraceId,
            "Hosted service: {0} failed to transition to Scheduled state. CurrentState={1}, ErrorCode={2}",
            serviceName_,
            StateToString(this->GetState()),
            transitionResult);
    }
    else
    {
        WriteInfo(
            TraceType_ActivationManager,
            TraceId,
            "Hosted service: {0} is scheduled to run at {1}.",
            serviceName_,            
            dueTime);

        runTimer_->Change(dueTime);
    }    
}

void HostedServiceInstance::OnScheduledStart(AsyncOperationSPtr const & parent)
{    
    this->CleanupTimer();

    auto operation = AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        FabricHostConfig::GetConfig().StopTimeout,
        [this](AsyncOperationSPtr const & operation){ this->FinishStartCompleted(operation, false); },
        parent);    

    FinishStartCompleted(operation, true);
}

void HostedServiceInstance::FinishStartCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    ErrorCode errorCode = ActivateAsyncOperation::End(operation);

    {
        AcquireWriteLock lock(this->statsLock_);
        this->stats_.UpdateProcessActivationStats(errorCode.IsSuccess());
    }

    if(!errorCode.IsSuccess())
    {
        if(errorCode.IsError(ErrorCodeValue::InvalidState) || errorCode.IsError(ErrorCodeValue::ObjectClosed))
        {
            WriteWarning(
                TraceType_ActivationManager,
                TraceId,
                "Hosted Service: {0} is not running and cannot be restarted because of its state. CurrentState={1}. ErrorCode={2}.",
                serviceName_,
                this->StateToString(this->GetState()),                
                errorCode);
            this->TransitionToFailed().ReadValue();
        }
        else
        {
            ScheduleStart();
        }         
    }  
}

bool HostedServiceInstance::UpdateRunStats(DWORD exitCode)
{
    if(this->activationContext_)
    {
        WriteInfo(
            TraceType_ActivationManager,
            TraceId,
            "Hosted Service: {0} with processId {1} terminated with exitcode {2}.",
            serviceName_,
            this->activationContext_->ProcessId,
            exitCode); 
    }
    this->stats_.UpdateProcessExitStats(exitCode);
    auto maxFailureCount = FabricHostConfig::GetConfig().ActivationMaxFailureCount;
    if(this->stats_.GetMaxContinuousFailureCount() >= (ULONG)maxFailureCount)
    {
        WriteWarning(
            TraceType_ActivationManager,
            TraceId,
            "Hosted Service: {0} has exceeded its max failure retry limit of {1}.",
            serviceName_,
            maxFailureCount); 
        return false;
    }
    return true;
}

HostedService::RunStats HostedServiceInstance::GetRunStats()
{
    {
        AcquireReadLock lock(statsLock_);
        return stats_;
    }
}

void HostedServiceInstance::CleanupTimer()
{
    AcquireWriteLock lock(this->Lock);

    if (runTimer_)
    {
        runTimer_->Cancel();
        runTimer_.reset();
    }
}

void HostedServiceInstance::OnAbort()
{
    this->CleanupTimer();

    if(activationContext_)
    {
        this->ActivationManager.ProcessActivatorObj->Terminate(activationContext_, ProcessActivator::ProcessAbortExitCode);
    }
    auto error = ConfigurePortSecurity(true);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            TraceId,
            "Hosted Service: {0} has failed to cleanup port security. ErrorCode: {1}.Ignore and continue",
            serviceName_,
            error);
    }
}

void HostedServiceInstance::DisableServiceInstance()
{
    if (activationContext_)
    {
        this->ActivationManager.ProcessActivatorObj->Terminate(activationContext_, ProcessActivator::ProcessAbortExitCode);
        activationContext_.reset();
    }
    auto error = ConfigurePortSecurity(true);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ActivationManager,
            TraceId,
            "Hosted Service: {0} has failed to cleanup port security. ErrorCode: {1}.Ignore and continue",
            serviceName_,
            error);
    }
}

void HostedServiceInstance::TerminateServiceInstance()
{
   AbortAndWaitForTermination();
}

Common::ErrorCode HostedServiceInstance::ConfigurePortSecurity(bool cleanup)
{
    ErrorCode error;
    bool isHttps = false;
    if (HostedServiceObj.protocol_ == ProtocolType::Enum::Https)
    {
        isHttps = true;
    }

    if(this->securityUser_ &&
        this->securityUser_->SidObj &&
        this->HostedServiceObj.needPortAcls_)
    {
        wstring sidString;
        error = securityUser_->SidObj->ToString(sidString);
        if(error.IsSuccess())
        {
            if(cleanup)
            {
                error = ActivationManager.EndpointProvider->CleanupPortAcls(
                    sidString,
                    HostedServiceObj.port_,
                    isHttps);
            }
            else
            {
                WriteInfo(
                    TraceType_ActivationManager,
                    TraceId,
                    "HostedService {0} configure port ACLs. Port {1}, Protocol {2}",
                    serviceName_,
                    this->HostedServiceObj.port_,
                    this->HostedServiceObj.protocol_);

                error = ActivationManager.EndpointProvider->ConfigurePortAcls(
                    sidString,
                    HostedServiceObj.port_,
                    isHttps);
            }
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_ActivationManager,
                    TraceId,
                    "HostedService failed to configure port ACLs with cleanup {0} for service {1}. ErrorCode: {2}",
                    cleanup,
                    serviceName_,
                    error);
            }
        }
    }

    if (isHttps)
    {
        if (cleanup)
        {
            error = ActivationManager.EndpointProvider->CleanupPortCertificate(
                HostedServiceObj.port_,
                HostedServiceObj.sslCertFindValue_,
                HostedServiceObj.sslCertStoreLocation_,
                HostedServiceObj.sslCertFindType_);
        }
        else
        {
            error = ActivationManager.EndpointProvider->ConfigurePortCertificate(
                HostedServiceObj.port_,
                HostedServiceObj.sslCertFindValue_,
                HostedServiceObj.sslCertStoreLocation_,
                HostedServiceObj.sslCertFindType_);
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_ActivationManager,
                TraceId,
                "HostedService failed to configure port Security with cleanup {0} for service {1}. ErrorCode: {2}",
                cleanup,
                serviceName_,
                error);
        }
    }

    return error;
}
