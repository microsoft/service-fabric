// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ImageModel;
using namespace ServiceModel;

StringLiteral const TraceType("Activator");

// ********************************************************************************************************************
// Activator.ActivateAsyncOperation Implementation
//
// base class for retry-able application or service package activations
class Activator::ActivateAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivateAsyncOperation)

public:
    enum Kind
    {
        ActivateApplicationAsyncOperation,
        ActivateServicePackageInstanceAsyncOperation
    };

public:
    ActivateAsyncOperation(
        Kind kind,
        __in Activator & owner,
        wstring const & applicationName,
        wstring const & activationId,
        int maxFailureCount,
        bool onlyIfUsed,
        bool ensureLatestVersion,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        kind_(kind),
        owner_(owner),
        applicationName_(applicationName),
        activationId_(activationId),
        activationInstanceId_(owner.activationInstanceId_++),
        retryTimer_(),
        lock_(),
        status_(),
        maxFailureCount_(maxFailureCount),
        onlyIfUsed_(onlyIfUsed),
        ensureLatestVersion_(ensureLatestVersion),
        healthReported_(false)
    {
    }

    virtual ~ActivateAsyncOperation()
    {
    }

protected:
    // activate the entity, once the activation is completed, call FinishActivateEntity to report the result
    virtual void OnActivateEntity(AsyncOperationSPtr const & thisSPtr) = 0;

    // check if the entity being activated is in use or not
    virtual bool IsEntityUsed() = 0;


    //
    // health reporting
    //

    // register the source id for reporting health
    virtual ErrorCode OnRegisterComponent() = 0;

    // unregister the source id
    virtual void OnUnregisterComponent() = 0;

    // report health on the entity
    virtual void OnReportHealth(SystemHealthReportCode::Enum reportCode, wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber) = 0;

private:
    static bool ShouldReplace(AsyncOperationSPtr const & existingItem, AsyncOperationSPtr const & withThisItem)
    {
        auto current = AsyncOperation::Get<ActivateAsyncOperation>(withThisItem);
        if (current->ensureLatestVersion_)
        {
            if (existingItem == nullptr)
            {
                return true;
            }
            else
            {
                auto existing = AsyncOperation::Get<ActivateAsyncOperation>(existingItem);
                return !existing->ensureLatestVersion_;
            }
        }
        else
        {
            return false;
        }
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        // try to add this operation in the pending activations map
        auto error = owner_.pendingActivations_->Start(
            activationId_,
            activationInstanceId_,
            thisSPtr,
            [thisSPtr](AsyncOperationSPtr const & existing, uint64 existingInstanceId, OperationStatus const & existingStatus) -> bool
              {
                  UNREFERENCED_PARAMETER(existingStatus);
                  UNREFERENCED_PARAMETER(existingInstanceId);

                  return ShouldReplace(existing, thisSPtr);
              },
            status_);
        if(!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::AlreadyExists))
            {
                // if other thread won, return activation in progress error
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::HostingActivationInProgress));
            }
            else
            {
                // error in getting the pending activation, usually this means that we are closing down
                TryComplete(thisSPtr, error);
            }

            return;
        }

        {
            AcquireWriteLock lock(lock_);
            if(status_.State == OperationState::Completed) { return; }

            error = this->RegisterComponent();
        }

        if(!error.IsSuccess())
        {
            if (TryStartComplete())
            {
                CompletePendingActivation(error);
                FinishComplete(thisSPtr, error);
            }

            return;
        }

        // this is the primary activation operation for this id, start the activation
        ScheduleActivationIfNeeded(thisSPtr, false /*isRetry*/);
    }

    virtual void OnCancel()
    {
        TimerSPtr timer;
        {
            AcquireWriteLock lock(lock_);
            timer = retryTimer_;
            retryTimer_.reset();
        }

        if (timer)
        {
            timer->Cancel();
        }

        CompletePendingActivation(ErrorCode(ErrorCodeValue::OperationCanceled));

        AsyncOperation::OnCancel();
    }

    void FinishActivateEntity(AsyncOperationSPtr const & thisSPtr, ErrorCode error, bool shouldRetry = true)
    {
        ULONG retryCount = 0;
        {
            AcquireReadLock lock(lock_);
            retryCount = status_.FailureCount;
        }

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "Activate: {0}, ErrorCode={1}, RetryCount={2}",
            activationId_,
            error,
            retryCount);

        if (error.IsSuccess())
        {
            // complete the activation
            if (TryStartComplete())
            {
                CompletePendingActivation(error);
                FinishComplete(thisSPtr, error);
            }
        }
        else
        {
            // retry if possible
            if (shouldRetry && (retryCount + 1 < maxFailureCount_))
            {
                UpdatePendingActivation(error);
                ScheduleActivationIfNeeded(thisSPtr, true /*isRetry*/);
            }
            else
            {
                if (TryStartComplete())
                {
                    CompletePendingActivation(error);
                    FinishComplete(thisSPtr, error);
                }
            }
        }
    }

private:
    ErrorCode RegisterComponent()
    {
        // V1 upgrade message will not have applicationName. Health will not be
        // be reported then
        if(applicationName_.empty())
        {
            return ErrorCodeValue::Success;
        }

        return this->OnRegisterComponent();
    }

    void UnregisterComponent()
    {
        if(applicationName_.empty())
        {
            return;
        }

        this->OnUnregisterComponent();
    }

    void ReportHealth(SystemHealthReportCode::Enum reportCode, wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        if(applicationName_.empty())
        {
            return;
        }

        this->OnReportHealth(reportCode, description, sequenceNumber);
    }

    void ScheduleActivationIfNeeded(AsyncOperationSPtr const & thisSPtr, bool isRetry)
    {
        if (this->onlyIfUsed_ && !this->IsEntityUsed())
        {
            if (TryStartComplete())
            {
                CompletePendingActivation(ErrorCodeValue::HostingActivationEntityNotInUse);
                FinishComplete(thisSPtr, ErrorCodeValue::HostingActivationEntityNotInUse);
            }

            return;
        }
        else
        {
            ScheduleActivation(thisSPtr, isRetry);
        }
    }

    void ScheduleActivation(AsyncOperationSPtr const & thisSPtr, bool isRetry)
    {
        if(isRetry)
        {
            TimerSPtr timer = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer) { this->OnScheduledCallback(timer, thisSPtr); }, false);
            TimeSpan dueTime;
            {
                AcquireWriteLock lock(lock_);
                if(status_.State == OperationState::Completed) 
                {
                    timer->Cancel();
                    return;
                }

                if(retryTimer_) { retryTimer_->Cancel(); }
                retryTimer_ = timer;

                dueTime = GetRetryDueTime(
                    status_.FailureCount,
                    HostingConfig::GetConfig().ActivationRetryBackoffInterval,
                    HostingConfig::GetConfig().ActivationMaxRetryInterval);
            }

            timer->Change(dueTime);
        }
        else
        {
            this->OnScheduledCallback(nullptr /*null timer object*/, thisSPtr);
        }
    }

    void OnScheduledCallback(TimerSPtr const & timer, AsyncOperationSPtr const & thisSPtr)
    {
        if (timer) { timer->Cancel(); }

        {
            AcquireReadLock lock(lock_);
            if(status_.State == OperationState::Completed) { return; }
        }

        this->OnActivateEntity(thisSPtr);
    }

    void CompletePendingActivation(ErrorCode const error)
    {
        {
            AcquireWriteLock lock(lock_);

            status_.LastError = error;
            status_.State = OperationState::Completed;

            if(status_.FailureCount > 0)
            {
                owner_.applicationManager_.hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->UnregisterFailure(activationId_);
            }

            this->UnregisterComponent();

            owner_.pendingActivations_->Complete(activationId_, activationInstanceId_, status_).ReadValue();
        }

        owner_.pendingActivations_->Remove(activationId_, activationInstanceId_);
    }

    void UpdatePendingActivation(ErrorCode const error)
    {
        bool shouldReport = false;
        FABRIC_SEQUENCE_NUMBER healthSequence = 0;
        wstring healthDescription = L"";
        {
            AcquireWriteLock lock(lock_);
            if(status_.State == OperationState::Completed) { return; }

            if(this->IsInternalError(error))
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "UpdatePendingActivation: Error '{0}' is an internal error and does not count as a real failure.",
                    error);
                return;
            }

            status_.LastError = error;
            status_.FailureCount++;

            if (status_.FailureCount == 1)
            {
                owner_.applicationManager_.hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->RegisterFailure(activationId_);
            }

            if(!healthReported_)
            {
                shouldReport = true;
                if (error.Message.empty())
                {
                    healthDescription = error.ErrorCodeValueToString();
                }
                else
                {
                    healthDescription = error.Message;
                }
                healthReported_ = true;
                healthSequence = SequenceNumber::GetNext();
            }

            owner_.pendingActivations_->UpdateStatus(activationId_, activationInstanceId_, status_).ReadValue();
        }

        if(shouldReport)
        {
            this->ReportHealth(
                SystemHealthReportCode::Hosting_ActivationFailed,
                healthDescription,
                healthSequence);
        }
    }

    static bool IsInternalError(ErrorCode const & error)
    {
        // To determine if error health report should be sent for the given error
        switch(error.ReadValue())
        {
            // If state machine lock cannot be acquired, activation fails with InvalidState
        case ErrorCodeValue::InvalidState:
        case ErrorCodeValue::ObjectClosed:
            return true;

        default:
            return false;
        }
    }

    static wstring ToWString(vector<wstring> const & paths)
    {
        wstring retval(L"");
        for(auto iter = paths.begin(); iter != paths.end(); ++iter)
        {
            retval.append(*iter);
            retval.append(L",");
        }

        return retval;
    }

    static TimeSpan GetRetryDueTime(LONG retryCount, TimeSpan const retryBackoffInterval, TimeSpan const maxRetryInterval)
    {
        int64 retryTicks = max(
            retryCount * retryBackoffInterval.Ticks,
            retryBackoffInterval.Ticks);

        if (retryTicks > maxRetryInterval.Ticks)
        {
            retryTicks = maxRetryInterval.Ticks;
        }

        return TimeSpan::FromTicks(retryTicks);
    }

public:
    const Kind kind_;

protected:
    Activator & owner_;
    wstring const activationId_;
    uint64 const activationInstanceId_;
    OperationStatus status_;
    wstring const applicationName_;
    bool ensureLatestVersion_;

private:
    ULONG const maxFailureCount_;
    bool onlyIfUsed_;
    bool healthReported_;
    RwLock lock_;
    TimerSPtr retryTimer_;
};

// ********************************************************************************************************************
// Activator.ActivateApplicationAsyncOperation Implementation
//

class Activator::ActivateApplicationAsyncOperation
    : public Activator::ActivateAsyncOperation
{
    DENY_COPY(ActivateApplicationAsyncOperation)

public:
    ActivateApplicationAsyncOperation(
        __in Activator & owner,
        ApplicationIdentifier const & applicationId,
        ApplicationVersion const & applicationVersion,
        wstring const & applicationName,
        ULONG maxFailureCount,
        bool onlyIfUsed,
        bool ensureLatestVersion,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ActivateAsyncOperation(
            Kind::ActivateApplicationAsyncOperation,
            owner,
            applicationName,
            Activator::GetOperationId(applicationId, applicationVersion),
            maxFailureCount,
            onlyIfUsed,
            ensureLatestVersion,
            callback,
            parent),
        applicationId_(applicationId),
        applicationVersion_(applicationVersion),
        application_(),
        healthPropertyId_(wformatString("Activation:{0}", applicationVersion))
    {
    }

    virtual ~ActivateApplicationAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out OperationStatus & activationStatus,
        __out Application2SPtr & application)
    {
        auto thisPtr = AsyncOperation::End<ActivateApplicationAsyncOperation>(operation);
        activationStatus = thisPtr->status_;
        if (thisPtr->Error.IsSuccess())
        {
            application = thisPtr->application_;
        }
        else
        {
            application = Application2SPtr();
        }
        return thisPtr->Error;
    }

    void GetApplicationInfo(__out wstring & applicationName, __out ApplicationIdentifier & applicationId)
    {
        applicationName = applicationName_;
        applicationId = applicationId_;
    }

protected:
    virtual bool IsEntityUsed()
    {
        return owner_.applicationManager_.DeactivatorObj->IsUsed(applicationId_);
    }

    virtual void OnActivateEntity(AsyncOperationSPtr const & thisSPtr)
    {
        application_.reset();
        auto error = owner_.applicationManager_.applicationMap_->Get(applicationId_, application_);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::ApplicationManagerApplicationNotFound))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetApplication: ApplicationId={0}, ErrorCode={1}",
                applicationId_,
                error);
            FinishActivateEntity(thisSPtr, error, false);
            return;
        }

        if (error.IsError(ErrorCodeValue::ApplicationManagerApplicationNotFound))
        {
            application_ = make_shared<Application2>(
                HostingSubsystemHolder(
                owner_.applicationManager_.hosting_,
                owner_.applicationManager_.hosting_.Root.CreateComponentRoot()),
                applicationId_,
                applicationName_);
            error = owner_.applicationManager_.applicationMap_->Add(application_);
            if (!error.IsSuccess())
            {
                this->FinishActivateEntity(thisSPtr, error, true);
                return;
            }
        }

        auto operation = application_->BeginActivate(
            applicationVersion_,
            ensureLatestVersion_,
            HostingConfig::GetConfig().ActivationTimeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishActivateApplication(operation, false); },
            thisSPtr);
        this->FinishActivateApplication(operation, true);
    }

    void FinishActivateApplication(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = application_->EndActivate(operation);

        if (error.IsError(ErrorCodeValue::HostingApplicationVersionMismatch))
        {
            bool hasReplica;
            owner_.applicationManager_.ScheduleForDeactivationIfNotUsed(application_->Id, hasReplica);
            this->FinishActivateEntity(operation->Parent, error, false);
            return;
        }

        bool shouldAbortApplication = false;
        if (error.IsSuccess())
        {
            error = owner_.applicationManager_.EnsureApplicationEntry(application_->Id);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "Application: Error when calling EnsureApplicationEntry. Id={0}:{1}. Error: {2} ",
                    application_->Id,
                    application_->InstanceId,
                    error);
                shouldAbortApplication = true;
            }
        }
        else
        {
            auto state = application_->GetState();
            shouldAbortApplication = (state == Application2::Failed);
        }


        if (shouldAbortApplication)
        {
            application_->AbortAndWaitForTerminationAsync(
                [this, error](AsyncOperationSPtr const & abortOperation)
            {
                Application2SPtr removed;
                owner_.applicationManager_.applicationMap_->Remove(application_->Id, application_->InstanceId, removed).ReadValue();

                bool shouldRetry = IsRetryableError(error);

                this->FinishActivateEntity(abortOperation->Parent, error, shouldRetry);
                return;
            },
                operation->Parent);
            return;
        }

        bool shouldRetry = IsRetryableError(error);
        this->FinishActivateEntity(operation->Parent, error, shouldRetry);
    }

    virtual ErrorCode OnRegisterComponent()
    {
        return owner_.applicationManager_.hosting_.HealthManagerObj->RegisterSource(applicationId_, applicationName_, healthPropertyId_);
    }

    virtual void OnUnregisterComponent()
    {
        owner_.applicationManager_.hosting_.HealthManagerObj->UnregisterSource(applicationId_, healthPropertyId_);
    }

    virtual void OnReportHealth(SystemHealthReportCode::Enum reportCode, std::wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        owner_.applicationManager_.hosting_.HealthManagerObj->ReportHealth(
            applicationId_,
            healthPropertyId_,
            reportCode,
            description,
            sequenceNumber);
    }

private:
    bool IsRetryableError(ErrorCode const & error)
    {
        switch(error.ReadValue())
        {
            // If the ApplicationPackage file is NotFound, then we would need to
            // download it. We should stop retrying activation.
        case ErrorCodeValue::FileNotFound:
            return false;

        default:
            return true;
        }
    }

private:
    ApplicationIdentifier const applicationId_;
    ApplicationVersion const applicationVersion_;
    Application2SPtr application_;
    wstring const healthPropertyId_;
};


// ********************************************************************************************************************
// Activator::ActivateServicePackageInstanceAsyncOperation Implementation
//

class Activator::ActivateServicePackageInstanceAsyncOperation
    : public Activator::ActivateAsyncOperation
{
    DENY_COPY(ActivateServicePackageInstanceAsyncOperation)

public:
    ActivateServicePackageInstanceAsyncOperation(
        __in Activator & owner,
        Application2SPtr const & application,
        ServicePackageInstanceIdentifier const & servicePackageInstanceId,
        ServicePackageVersionInstance const & servicePackageVersionInstance,
        shared_ptr<ServicePackageDescription> const & servicePackageDescriptionPtr,
        ULONG maxFailureCount,
        bool onlyIfUsed,
        bool ensureLatestVersion,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ActivateAsyncOperation(
            Kind::ActivateServicePackageInstanceAsyncOperation,
            owner,
            application->AppName,
            Activator::GetOperationId(servicePackageInstanceId, servicePackageVersionInstance),
            maxFailureCount,
            onlyIfUsed,
            ensureLatestVersion,
            callback,
            parent),
        application_(application),
        servicePackageInstanceId_(servicePackageInstanceId),
        servicePackageVersionInstance_(servicePackageVersionInstance),
        servicePackageDescriptionPtr__(servicePackageDescriptionPtr),
        descriptionLock_(),
        healthPropertyId_(wformatString("Activation:{0}", servicePackageVersionInstance))
    {
    }

    virtual ~ActivateServicePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out OperationStatus & activationStatus)
    {
        auto thisPtr = AsyncOperation::End<ActivateServicePackageInstanceAsyncOperation>(operation);
        activationStatus = thisPtr->status_;
        return thisPtr->Error;
    }

    void GetServicePackageInstanceInfo(
        __out wstring & applicationName,
        __out wstring & serviceManifestName,
        __out wstring servicePackagePublicActivationId,
        __out wstring & serviceManifestVersion)
    {
        applicationName = applicationName_;
        serviceManifestName = servicePackageInstanceId_.ServicePackageName;
        servicePackagePublicActivationId = servicePackageInstanceId_.PublicActivationId;

        {
            AcquireReadLock lock(descriptionLock_);
            if (servicePackageDescriptionPtr__)
            {
                serviceManifestVersion =  servicePackageDescriptionPtr__->ManifestVersion;
            }
        }
    }

protected:
    virtual bool IsEntityUsed()
    {
        return owner_.applicationManager_.DeactivatorObj->IsUsed(servicePackageInstanceId_);
    }

    virtual void OnActivateEntity(AsyncOperationSPtr const & thisSPtr)
    {
        shared_ptr<ServicePackageDescription> servicePackageDescriptionPtr = nullptr;
        {
            AcquireReadLock lock(descriptionLock_);
            servicePackageDescriptionPtr = servicePackageDescriptionPtr__;
        }

        if (servicePackageDescriptionPtr == nullptr)
        {
            servicePackageDescriptionPtr = make_shared<ServicePackageDescription>();
            auto error = owner_.applicationManager_.LoadServicePackageDescription(
                servicePackageInstanceId_.ApplicationId,
                servicePackageInstanceId_.ServicePackageName,
                servicePackageVersionInstance_.Version.RolloutVersionValue,
                *servicePackageDescriptionPtr);

            if (!error.IsSuccess())
            {
                // If the ServicePackage file is NotFound, then we would need to
                // download it. We should stop retrying activation.
                bool shouldRetry = error.IsError(ErrorCodeValue::FileNotFound);
                this->FinishActivateEntity(thisSPtr, error, shouldRetry);
                return;
            }

            {
                AcquireWriteLock lock(descriptionLock_);
                servicePackageDescriptionPtr__ = servicePackageDescriptionPtr;
            }
        }

        vector<ServicePackageInstanceOperation> packageActivations;
        packageActivations.push_back(
            ServicePackageInstanceOperation(
            servicePackageInstanceId_.ServicePackageName,
            servicePackageInstanceId_.ActivationContext,
            servicePackageInstanceId_.PublicActivationId,
            servicePackageVersionInstance_.Version.RolloutVersionValue,
            servicePackageVersionInstance_.InstanceId,
            *servicePackageDescriptionPtr));

        // In case there are too many files in FSS and anonymous access is enabled, FSS activation could take
        // a long time. Using a separate timeout for FSS activation
        bool isFSSPackage = servicePackageInstanceId_.ServicePackageId.ApplicationId.IsSystem()
            && servicePackageInstanceId_.ServicePackageId.ServicePackageName == L"FileStoreService";

        auto activationTimeout = isFSSPackage ? HostingConfig::GetConfig().FSSActivationTimeout : HostingConfig::GetConfig().ActivationTimeout;

        auto operation = application_->BeginActivateServicePackageInstances(
            move(packageActivations),
            ensureLatestVersion_,
            activationTimeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishActivateServicePackage(operation, false); },
            thisSPtr);

        this->FinishActivateServicePackage(operation, true);
    }

    void FinishActivateServicePackage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        OperationStatusMapSPtr activationStatusMap;
        auto error = application_->EndActivateServicePackages(
            operation,
            activationStatusMap);
        if (!error.IsSuccess())
        {
            auto state = application_->GetState();
            if(state == Application2::Failed)
            {
                application_->AbortAndWaitForTerminationAsync(
                    [this, error](AsyncOperationSPtr const & abortOperation)
                {
                    Application2SPtr removed;
                    owner_.applicationManager_.applicationMap_->Remove(application_->Id, application_->InstanceId, removed).ReadValue();
                    this->FinishActivateEntity(abortOperation->Parent, error, false);
                    return;
                },
                    operation->Parent);
                return;
            }
            else if(state == Application2::Deactivated || state == Application2::Aborted)
            {
                // The Application is in terminal state and is no longer valid, hence
                // we cannot retry
                this->FinishActivateEntity(operation->Parent, error, false);
                return;
            }
        }

        bool shouldRetry = IsRetryableError(error);
        this->FinishActivateEntity(operation->Parent, error, shouldRetry);
    }

    virtual ErrorCode OnRegisterComponent()
    {
        return owner_.applicationManager_.hosting_.HealthManagerObj->RegisterSource(servicePackageInstanceId_, applicationName_, healthPropertyId_);
    }

    virtual void OnUnregisterComponent()
    {
        owner_.applicationManager_.hosting_.HealthManagerObj->UnregisterSource(servicePackageInstanceId_, healthPropertyId_);
    }

    virtual void OnReportHealth(SystemHealthReportCode::Enum reportCode, std::wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        owner_.applicationManager_.hosting_.HealthManagerObj->ReportHealth(
            servicePackageInstanceId_,
            healthPropertyId_,
            reportCode,
            description,
            sequenceNumber);
    }

private:
    bool IsRetryableError(ErrorCode const & error)
    {
        switch(error.ReadValue())
        {
        case ErrorCodeValue::HostingServicePackageVersionMismatch:
            return false;

        default:
            return true;
        }
    }

private:
    shared_ptr<ServicePackageDescription> servicePackageDescriptionPtr__;
    RwLock descriptionLock_;
    ServicePackageInstanceIdentifier const servicePackageInstanceId_;
    ServicePackageVersionInstance const servicePackageVersionInstance_;
    Application2SPtr const application_;
    wstring const healthPropertyId_;
};


// ********************************************************************************************************************
// Activator::EnsureActivationAsyncOperation Implementation
//

class Activator::EnsureActivationAsyncOperation
    : public AsyncOperation
{
    DENY_COPY(EnsureActivationAsyncOperation)

public:
    EnsureActivationAsyncOperation(
        __in Activator & owner,
        ApplicationUpgradeSpecification const & appUpgradeSpec,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(
            callback,
            parent),
        owner_(owner),
        appUpgradeSpecification_(appUpgradeSpec),
        application_(),
        pendingActivationCount_(0),
        activationLock_(),
        lastActivationError_()
    {
    }

    virtual ~EnsureActivationAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<EnsureActivationAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        EnsureApplicationActivation(thisSPtr);
    }

private:
    void EnsureApplicationActivation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(ActivateApplication): Ensure Application activation after Upgrade. ApplicationId={0}, AppVersion={1}",
            appUpgradeSpecification_.ApplicationId,
            appUpgradeSpecification_.AppVersion);

            auto operation = owner_.BeginActivateApplication(
                appUpgradeSpecification_.ApplicationId,
                appUpgradeSpecification_.AppVersion,
                appUpgradeSpecification_.AppName,
                MAXULONG, /* max retry count, give up when not used or higher or same version is already running */
                true,     /* only if used */
                true,     /* ensure latest version */
                [this](AsyncOperationSPtr const & operation)
                {
                    this->FinishActivateApplication(operation, false);
                },
                thisSPtr);
            FinishActivateApplication(operation, true);
    }

    void FinishActivateApplication(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        OperationStatus activationStatus;
        auto error = owner_.EndActivateApplication(operation, activationStatus, application_);

        WriteTrace(
            (error.IsSuccess() || error.IsError(ErrorCodeValue::HostingActivationEntityNotInUse)) ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            owner_.Root.TraceId,
            "End(ActivateApplication): Ensure Application activation after Upgrade. Error={0}, ApplicationId={1}, AppVersion={2}",
            error,
            appUpgradeSpecification_.ApplicationId,
            appUpgradeSpecification_.AppVersion);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        // ensure activation of the service packages passed in the upgrade call
        EnsureServicePackagesActivation(operation->Parent);
    }

    void EnsureServicePackagesActivation(AsyncOperationSPtr const & thisSPtr)
    {
        // Find all instances of ServicePackageInctance(s) from deactivator.
        size_t svcPkgInstanceCount = 0;
        vector<pair<ServicePackageVersionInstance, vector<ServicePackageInstanceIdentifier>>> packageInstancesInfo;

        for (auto const & packageUpgrade : appUpgradeSpecification_.PackageUpgrades)
        {
            ServicePackageIdentifier servicePackageId(appUpgradeSpecification_.ApplicationId, packageUpgrade.ServicePackageName);

            ServicePackageVersionInstance svcPkgVerInstance(
                ServicePackageVersion(appUpgradeSpecification_.AppVersion, packageUpgrade.RolloutVersionValue),
                appUpgradeSpecification_.InstanceId);

            vector<ServicePackageInstanceIdentifier> servicePackageInstances;
            owner_.applicationManager_.DeactivatorObj->GetServicePackageInstanceEntries(servicePackageId, servicePackageInstances);

            if (servicePackageInstances.empty())
            {
                WriteNoise(
                    TraceType,
                    owner_.Root.TraceId,
                    "Begin(ActivateServicePackage): EnsureServicePackagesActivation(): No instances found in deactivator. ServicePackageId={0}, ServicePackageVersionInstance={1}",
                    servicePackageId,
                    svcPkgVerInstance);

                continue;
            }

            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "Begin(ActivateServicePackage): EnsureServicePackagesActivation(): ServicePackageId={0}, ServicePackageVersionInstance={1}, ServicePackageInstances={2}.",
                servicePackageId,
                svcPkgVerInstance,
                servicePackageInstances.size());

            svcPkgInstanceCount += servicePackageInstances.size();
            packageInstancesInfo.push_back(make_pair(move(svcPkgVerInstance), move(servicePackageInstances)));
        }

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Begin(ActivateServicePackage): EnsureServicePackagesActivation(). Total ServicePackageInstances found in deactivator={0}.",
            svcPkgInstanceCount);

        pendingActivationCount_.store(svcPkgInstanceCount);

        // Activate the ServiePackageInstance(s)
        for (auto const & pkgInstanceInfo : packageInstancesInfo)
        {
            for (auto const & pkgInstance : pkgInstanceInfo.second)
            {
                ActivateServicePackageInstance(thisSPtr, pkgInstanceInfo.first, pkgInstance);
            }
        }

        CheckPendingActivations(thisSPtr, svcPkgInstanceCount);
    }

    void ActivateServicePackageInstance(
        AsyncOperationSPtr const & thisSPtr,
        ServicePackageVersionInstance const & servicePackageVersionInstance,
        ServicePackageInstanceIdentifier const & servicePackageInstanceId)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(ActivateServicePackageInstance): Ensure ServicePackageInstance activation after Upgrade. ServicePackageInstanceId={0}, ServicePackageVersionInstance={1}",
            servicePackageInstanceId,
            servicePackageVersionInstance);

        auto operation = owner_.BeginActivateServicePackageInstance(
            application_,
            servicePackageInstanceId,
            servicePackageVersionInstance,
            nullptr,
            ULONG_MAX,  /* give up only if latest version is used or package is not needed */
            true,       /* onlyIfUsed */
            true,       /* ensureLatestVersion */
            [servicePackageInstanceId, servicePackageVersionInstance, this](AsyncOperationSPtr const & operation)
            {
                this->FinishActivateServicePackageInstance(servicePackageInstanceId, servicePackageVersionInstance, operation, false);
            },
            thisSPtr);

            FinishActivateServicePackageInstance(servicePackageInstanceId, servicePackageVersionInstance, operation, true);
    }

    void FinishActivateServicePackageInstance(
        ServicePackageInstanceIdentifier const & servicePackageInstanceId,
        ServicePackageVersionInstance const & servicePackageVersionInstance,
        AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        OperationStatus status;
        auto error = owner_.EndActivateServicePackageInstance(
            operation,
            status);

        WriteTrace(
            (error.IsSuccess() || error.IsError(ErrorCodeValue::HostingActivationEntityNotInUse)) ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            owner_.Root.TraceId,
            "End(ActivateServicePackageInstance): Ensure ServicePackageIntance activation after Upgrade. Error={0}, servicePackageInstanceId={1}, ServicePackageVersionInstance={2}, OperationStatus={3}",
            error,
            servicePackageInstanceId,
            servicePackageVersionInstance,
            status);

        if (!error.IsSuccess())
        {
            {
                AcquireWriteLock lock(activationLock_);
                lastActivationError_.Overwrite(error);
            }
        }

        uint64 pendingActivationCount = --pendingActivationCount_;
        CheckPendingActivations(operation->Parent, pendingActivationCount);
    }

    void CheckPendingActivations(AsyncOperationSPtr const & thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            CompleteServicePackagesActivation(thisSPtr, lastActivationError_);
        }
    }

    void CompleteServicePackagesActivation(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        TryComplete(thisSPtr, error);
        return;
    }

private:
    Activator & owner_;
    ApplicationUpgradeSpecification appUpgradeSpecification_;
    Application2SPtr application_;
    atomic_uint64 pendingActivationCount_;
    ErrorCode lastActivationError_;
    RwLock activationLock_;
};


// ********************************************************************************************************************
// Activator Implementation
//

Activator::Activator(
    ComponentRoot const & root,
    __in ApplicationManager & applicationManager)
    : RootedObject(root),
    FabricComponent(),
    applicationManager_(applicationManager),
    pendingActivations_(),
    activationInstanceId_(0)
{
    auto pendingActivations = make_unique<PendingOperationMap>();
    pendingActivations_ = move(pendingActivations);
}

Activator::~Activator()
{
}

AsyncOperationSPtr Activator::BeginActivateApplication(
    ApplicationIdentifier const & applicationId,
    ApplicationVersion const & applicationVersion,
    wstring const & applicationName,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->BeginActivateApplication(
        applicationId,
        applicationVersion,
        applicationName,
        HostingConfig::GetConfig().ActivationMaxFailureCount,
        false,
        false,
        callback,
        parent);
}

AsyncOperationSPtr Activator::BeginActivateApplication(
    ApplicationIdentifier const & applicationId,
    ApplicationVersion const & applicationVersion,
    wstring const & applicationName,
    ULONG maxFailureCount,
    bool onlyIfUsed,
    bool ensureLatestVersion,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateApplicationAsyncOperation>(
        *this,
        applicationId,
        applicationVersion,
        applicationName,
        maxFailureCount,
        onlyIfUsed,
        ensureLatestVersion,
        callback,
        parent);
}

ErrorCode Activator::EndActivateApplication(
    AsyncOperationSPtr const & operation,
    __out OperationStatus & activationStatus,
    __out Application2SPtr & application)
{
    return ActivateApplicationAsyncOperation::End(operation, activationStatus, application);
}

void Activator::EnsureActivationAsync(
    ApplicationUpgradeSpecification const & appUpgradeSpec)
{
    auto operation = AsyncOperation::CreateAndStart<EnsureActivationAsyncOperation>(
        *this,
        appUpgradeSpec,
        [this,appUpgradeSpec](AsyncOperationSPtr const & operation)
        {
            auto error = EnsureActivationAsyncOperation::End(operation);
            error.ReadValue();
            WriteTrace(
                (error.IsSuccess() || error.IsError(ErrorCodeValue::HostingActivationEntityNotInUse)) ? LogLevel::Info : LogLevel::Warning,
                TraceType,
                this->Root.TraceId,
                "EnsureActivationAsyncOperation: AppUpgradeSpec = {0}, ErrorCode={1}",
                appUpgradeSpec,
                error);
        },
        this->Root.CreateAsyncOperationRoot());
}

AsyncOperationSPtr Activator::BeginActivateServicePackageInstance(
    Application2SPtr const & application,
    ServicePackageInstanceIdentifier const & servicePackageId,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    shared_ptr<ServicePackageDescription> const & packageDescriptionPtr,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->BeginActivateServicePackageInstance(
        application,
        servicePackageId,
        servicePackageVersionInstance,
        packageDescriptionPtr,
        HostingConfig::GetConfig().ActivationMaxFailureCount,
        false,
        false,
        callback,
        parent);

}

AsyncOperationSPtr Activator::BeginActivateServicePackageInstance(
    Application2SPtr const & application,
    ServicePackageInstanceIdentifier const & servicePackageId,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    shared_ptr<ServicePackageDescription> const & packageDescriptionPtr,
    ULONG maxFailureCount,
    bool onlyIfUsed,
    bool ensureLatestVersion,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateServicePackageInstanceAsyncOperation>(
        *this,
        application,
        servicePackageId,
        servicePackageVersionInstance,
        packageDescriptionPtr,
        maxFailureCount,
        onlyIfUsed,
        ensureLatestVersion,
        callback,
        parent);
}

ErrorCode Activator::EndActivateServicePackageInstance(
    AsyncOperationSPtr const & operation,
    __out OperationStatus & activationStatus)
{
    return ActivateServicePackageInstanceAsyncOperation::End(operation, activationStatus);
}

ErrorCode Activator::OnOpen()
{
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode Activator::OnClose()
{
    this->ClosePendingActivations();
    return ErrorCode(ErrorCodeValue::Success);
}

void Activator::OnAbort()
{
    this->ClosePendingActivations();
}

void Activator::ClosePendingActivations()
{
    auto removed = pendingActivations_->Close();
    for(auto iter = removed.begin(); iter != removed.end(); ++iter)
    {
        (*iter)->Cancel();
    }
}

wstring Activator::GetOperationId(ApplicationIdentifier const & applicationId, ApplicationVersion const & applicationVersion)
{
    return wformatString("Activate:{0}:{1}", applicationId, applicationVersion);
}

wstring Activator::GetOperationId(ServicePackageInstanceIdentifier const & servicePackageInstanceId, ServicePackageVersionInstance const & servicePackageVersionInstance)
{
    return wformatString("Activate:{0}:{1}", servicePackageInstanceId, servicePackageVersionInstance);
}

ErrorCode Activator::GetApplicationsQueryResult(std::wstring const & filterApplicationName, __out vector<DeployedApplicationQueryResult> & deployedApplications, std::wstring const & continuationToken)
{
    vector<AsyncOperationSPtr> pendingAsyncOperations;
    auto error = pendingActivations_->GetPendingAsyncOperations(pendingAsyncOperations);
    if(!error.IsSuccess())
    {
        return error;
    }

    for(auto iter = pendingAsyncOperations.begin(); iter != pendingAsyncOperations.end(); ++iter)
    {
        auto activateAsyncOperation = AsyncOperation::Get<ActivateAsyncOperation>(*iter);
        if (activateAsyncOperation->kind_ == ActivateAsyncOperation::Kind::ActivateApplicationAsyncOperation)
        {
            wstring applicationName;
            ApplicationIdentifier applicationId;
            auto activateApplicationAsyncOperation = AsyncOperation::Get<ActivateApplicationAsyncOperation>(*iter);
            activateApplicationAsyncOperation->GetApplicationInfo(applicationName, applicationId);

            auto queryResult = DeployedApplicationQueryResult(
                applicationName,
                applicationId.ApplicationTypeName,
                DeploymentStatus::Activating);

            // If the application name doesn't respect the continuation token, then skip this item
            if (!continuationToken.empty() && (continuationToken >= applicationName))
            {
                continue;
            }

            if (filterApplicationName.empty())
            {
                deployedApplications.push_back(move(queryResult));
            }
            else if (filterApplicationName == applicationName)
            {
                deployedApplications.push_back(move(queryResult));
                return ErrorCodeValue::Success;
            }
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode Activator::GetServiceManifestQueryResult(std::wstring const & filterApplicationName, std::wstring const & filterServiceManifestName, vector<DeployedServiceManifestQueryResult> & deployedServiceManifests)
{
    ASSERT_IF(filterApplicationName.empty(), "filterApplicationName should not be empty");

    vector<AsyncOperationSPtr> pendingAsyncOperations;
    auto error = pendingActivations_->GetPendingAsyncOperations(pendingAsyncOperations);
    if(!error.IsSuccess())
    {
        return error;
    }

    for(auto iter = pendingAsyncOperations.begin(); iter != pendingAsyncOperations.end(); ++iter)
    {
        auto activateAsyncOperation = AsyncOperation::Get<ActivateAsyncOperation>(*iter);
        if (activateAsyncOperation->kind_ == ActivateAsyncOperation::Kind::ActivateServicePackageInstanceAsyncOperation)
        {
            wstring applicationName, serviceManifestName, serviceManifestVersion, servicePackagePublicActivationId;

            auto activateServicePackageInstanceAsyncOperation = AsyncOperation::Get<ActivateServicePackageInstanceAsyncOperation>(*iter);
            activateServicePackageInstanceAsyncOperation->GetServicePackageInstanceInfo(
                applicationName,
                serviceManifestName,
                servicePackagePublicActivationId,
                serviceManifestVersion);

            if (filterApplicationName != applicationName) { continue; }

            auto queryResult = DeployedServiceManifestQueryResult(
                serviceManifestName,
                servicePackagePublicActivationId,
                serviceManifestVersion,
                DeploymentStatus::Activating);

            if (filterServiceManifestName.empty())
            {
                deployedServiceManifests.push_back(move(queryResult));
            }
            else if (StringUtility::AreEqualCaseInsensitive(filterServiceManifestName, serviceManifestName))
            {
                deployedServiceManifests.push_back(move(queryResult));
                return ErrorCodeValue::Success;
            }
        }
    }

    return ErrorCodeValue::Success;
}
