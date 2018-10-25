// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("Application");

// ********************************************************************************************************************
// VersionedApplication::ActivateAsyncOperation Implementation
//
class Application2::ActivateAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        __in Application2 & owner,
        ApplicationVersion const & version,
        bool ensureLatestVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        version_(version),
        packageDescription_(),
        versionedApplication_(),
        ensureLatestVersion_(ensureLatestVersion)
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
        ApplicationVersion currentVersion;
        bool isActive = false;
        int versionComparision = 0;
        bool failCurrentVersion = false;

        {
            AcquireReadLock lock(owner_.Lock);
            if ((owner_.GetState_CallerHoldsLock() & owner_.GetActiveStatesMask()) != 0)
            {
                isActive = true;
                currentVersion = owner_.version__;
            }
        }

        versionComparision = currentVersion.compare(version_);
        if (isActive)
        {
            if (versionComparision == 0)
            {
                // application is already active in the right version
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                return;
            }
            else
            {
                 WriteInfo(
                    TraceType,
                    owner_.TraceId,
                    "ApplicationVersionMismatch: Id={0}, CurrentVersion={1}, RequestedVersion={2}, EnsureLatestVersion={3}",
                    owner_.Id,
                    currentVersion,
                    version_,
                    ensureLatestVersion_);

                // else different versions, check for ensureLatestVersion flag
                if (!ensureLatestVersion_)
                {
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::HostingApplicationVersionMismatch));
                    return;
                }
                else
                {
                    // abort lower version, allow higher version to continue
                    if (versionComparision > 0)
                    {
                        // currently running version is higher than existing version
                        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                        return;
                    }
                    else
                    {
                        WriteInfo(
                            TraceType,
                            owner_.TraceId,
                            "Failing the current version as it is lower than requested and the EnsureLatestVersion is set.");
                        // currently running version is lower than existing version
                        // abort the current version after transitioning to activating
                        failCurrentVersion = true;
                    }
                }
            }
        }

        auto error = owner_.TransitionToActivating();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        if (failCurrentVersion)
        {
            TransitionToFailed(thisSPtr, ErrorCode(ErrorCodeValue::HostingEntityAborted));
            return;
        }

        error = owner_.LoadApplicationPackageDescription(owner_.Id, version_, packageDescription_);
        if(!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        versionedApplication_ = make_shared<VersionedApplication>(
            Application2Holder(owner_, owner_.CreateComponentRoot()),
            version_,
            packageDescription_);
        OpenVersionedApplication(thisSPtr);
    }

private:
    void OpenVersionedApplication(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        // Setup the environment manager for this application
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(OpenVersionedApplication): Id={0}, Version={1}, Timeout={2}",
            owner_.Id,
            version_,
            timeout);

        auto operation = versionedApplication_->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishVersionedApplicationOpen(operation, false); },
            thisSPtr);
        FinishVersionedApplicationOpen(operation, true);
    }

    void FinishVersionedApplicationOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = versionedApplication_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(OpenVersionedApplication): Id={0}, Version={1}, ErrorCode={2}",
            owner_.Id,
            version_,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(operation->Parent, error);
        }
        else
        {
            TransitionToActive(operation->Parent);
        }
    }

    void TransitionToActive(AsyncOperationSPtr const & thisSPtr)
    {
        {
            AcquireWriteLock lock(owner_.Lock);
            owner_.version__ = version_;
            owner_.versionedApplication__ = versionedApplication_;
            owner_.packageDescription__ = packageDescription_;
        }

        auto error = owner_.TransitionToActive();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }
        else
        {
            hostingTrace.ApplicationActivated(owner_.TraceId, owner_.Id.ToString(), version_.ToString(), owner_.InstanceId);
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        if (versionedApplication_)
        {
            versionedApplication_->AbortAndWaitForTerminationAsync(
                [this,failedError](AsyncOperationSPtr const & operation)
            {
                this->owner_.TransitionToFailed().ReadValue();
                this->TryComplete(operation->Parent, failedError);
            },
                thisSPtr);
        }
        else
        {
            this->owner_.TransitionToFailed().ReadValue();
            this->TryComplete(thisSPtr, failedError);
        }
    }

private:
    Application2 & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationVersion const version_;
    ApplicationPackageDescription packageDescription_;
    VersionedApplicationSPtr versionedApplication_;
    bool ensureLatestVersion_;
};

// ********************************************************************************************************************
// VersionedApplication::UpgradeAsyncOperation Implementation
//
class Application2::UpgradeAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UpgradeAsyncOperation)

public:
    UpgradeAsyncOperation(
        __in Application2 & owner,
        ApplicationVersion const & toVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        toVersion_(toVersion),
        fromVersion_(),
        toPackageDescription_(),
        fromPackageDescription_(),
        toVersionedApplication_(),
        fromVersionedApplication_()
    {
    }

    virtual ~UpgradeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpgradeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToUpgrading();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        {
            AcquireReadLock lock(owner_.Lock);
            fromVersion_ = owner_.version__;
            fromPackageDescription_ = owner_.packageDescription__;
            fromVersionedApplication_ = owner_.versionedApplication__;
        }

        if (fromVersion_ == toVersion_)
        {
            TransitionToActive(thisSPtr);
            return;
        }

        error = owner_.LoadApplicationPackageDescription(owner_.Id, toVersion_, toPackageDescription_);
        if(!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        DoUpgrade(thisSPtr);
    }

private:
    void DoUpgrade(AsyncOperationSPtr const & thisSPtr)
    {
        // Currently all Application upgrades cause the Major version of the RolloutVersion to change. Hence calling
        // switch only if the version needs to be updated
        if(fromPackageDescription_.ContentChecksum == toPackageDescription_.ContentChecksum)
        {
            DoSwitch(thisSPtr);
        }
        else
        {
            DoCloseOpen(thisSPtr);
        }
    }

    void DoSwitch(AsyncOperationSPtr const &thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(ApplicationSwitch): Id={0}:{1}, FromVersion={2}, ToVersion={3}, Timeout={4}",
            owner_.Id,
            owner_.InstanceId,
            fromVersion_,
            toVersion_,
            timeout);

        auto operation = fromVersionedApplication_->BeginSwitch(
            toVersion_,
            toPackageDescription_,
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishSwitch(operation, false); },
            thisSPtr);
        FinishSwitch(operation, true);
    }

    void FinishSwitch(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = fromVersionedApplication_->EndSwitch(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            owner_.TraceId,
            "End(ApplicationSwitch): Id={0}:{1}, FromVersion={2}, ToVersion={3}, ErrorCode={4}",
            owner_.Id,
            owner_.InstanceId,
            fromVersion_,
            toVersion_,
            error);

        if (!error.IsSuccess())
        {
            // cannot switch so do Open/Close
            DoCloseOpen(operation->Parent);
        }
        else
        {
            TransitionToActive(operation->Parent);
        }
    }

    void DoCloseOpen(AsyncOperationSPtr const &thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(CloseOldVersionedApplication): Id={0}:{1}, Version={2}, Timeout={3}",
            owner_.Id,
            owner_.InstanceId,
            fromVersion_,
            timeout);
        auto operation = fromVersionedApplication_->BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishCloseOldVersionedApplication(operation, false); },
            thisSPtr);
        FinishCloseOldVersionedApplication(operation, true);
    }

    void FinishCloseOldVersionedApplication(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        fromVersionedApplication_->EndClose(operation);
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "End(CloseOldVersionedApplication): Id={0}:{1}, Version={2}",
            owner_.Id,
            owner_.InstanceId,
            fromVersion_);

        //If UpgradeApplication has Aborted, then do not open
        if (operation->Parent->Error.IsError(ErrorCodeValue::OperationCanceled))
        {
            TransitionToFailed(operation->Parent, operation->Parent->Error);
            return;
        }

        OpenNewVersionedApplication(operation->Parent);
    }

    void OpenNewVersionedApplication(AsyncOperationSPtr const & thisSPtr)
    {
        toVersionedApplication_ = make_shared<VersionedApplication>(
            Application2Holder(owner_, owner_.CreateComponentRoot()),
            toVersion_,
            toPackageDescription_);

        auto timeout = timeoutHelper_.GetRemainingTime();
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(OpenNewVersionedApplication): Id={0}:{1}, Version={2}, Timeout={3}",
            owner_.Id,
            owner_.InstanceId,
            toVersion_,
            timeout);
        auto operation = toVersionedApplication_->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishOpenNewVersionedApplication(operation, false); },
            thisSPtr);
        FinishOpenNewVersionedApplication(operation, true);
    }

    void FinishOpenNewVersionedApplication(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = toVersionedApplication_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(OpenNewVersionedApplication): Id={0}:{1}, Version={2}, ErrorCode={3}",
            owner_.Id,
            owner_.InstanceId,
            toVersion_,
            error);
        if (!error.IsSuccess())
        {
            toVersionedApplication_->AbortAndWaitForTerminationAsync(
                [this, error](AsyncOperationSPtr const & abortOperation)
            {
                this->TransitionToFailed(abortOperation->Parent, error);
            },
                operation->Parent);
        }
        else
        {
            TransitionToActive(operation->Parent);
        }
    }

    void TransitionToActive(AsyncOperationSPtr const & thisSPtr)
    {
        {
            AcquireWriteLock lock(owner_.Lock);
            // If we transition directly to Active state, toVersionedApplication_
            // and toPackageDescrition_ will not be set. Checking that they are
            // initialized before assigning
            if (toVersionedApplication_)
            {
                owner_.versionedApplication__ = toVersionedApplication_;
            }

            if(!toPackageDescription_.ApplicationTypeName.empty())
            {
                owner_.packageDescription__ = toPackageDescription_;
            }

            owner_.version__ = toVersion_;
        }

        auto error = owner_.TransitionToActive();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        VersionedApplicationSPtr versionedApplication;
        {
            AcquireWriteLock lock(owner_.Lock);
            versionedApplication = move(owner_.versionedApplication__);
        }

        if (versionedApplication)
        {
            versionedApplication->AbortAndWaitForTerminationAsync(
                [this, failedError](AsyncOperationSPtr const & operation)
            {
                this->owner_.TransitionToFailed().ReadValue();
                this->TryComplete(operation->Parent, failedError);
            },
                thisSPtr);
        }
        else
        {
            this->owner_.TransitionToFailed().ReadValue();
            this->TryComplete(thisSPtr, failedError);
        }
    }

private:
    Application2 & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationVersion const toVersion_;
    ApplicationVersion fromVersion_;
    ApplicationPackageDescription toPackageDescription_;
    ApplicationPackageDescription fromPackageDescription_;
    VersionedApplicationSPtr fromVersionedApplication_;
    VersionedApplicationSPtr toVersionedApplication_;
};

// ********************************************************************************************************************
// Application2::DeactivateAsyncOperation Implementation
//
class Application2::DeactivateAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DeactivateAsyncOperation)

public:
    DeactivateAsyncOperation(
        __in Application2 & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        versionedApplication_(),
        version_()
    {
    }

    virtual ~DeactivateAsyncOperation()
    {
    }

    static void End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.GetState() == Application2::Deactivated)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        auto error = owner_.TransitionToDeactivating();
        if (!error.IsSuccess())
        {
            TransitionToAborted(thisSPtr);
            return;
        }

        {
            AcquireReadLock lock(owner_.Lock);
            versionedApplication_ = move(owner_.versionedApplication__);
            version_ = owner_.version__;
        }

        if (versionedApplication_)
        {
            CloseVersionedApplication(thisSPtr);
        }
        else
        {
            TransitionToDeactivated(thisSPtr);
        }
    }

private:
    void CloseVersionedApplication(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CloseVersionedApplication): Id={0}:{1}, Version={2}, Timeout={3}",
            owner_.Id,
            owner_.InstanceId,
            version_,
            timeout);
        auto operation = versionedApplication_->BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishCloseVersionedApplication(operation, false); },
            thisSPtr);
        FinishCloseVersionedApplication(operation, true);
    }

    void FinishCloseVersionedApplication(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        versionedApplication_->EndClose(operation);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(CloseVersionedApplication): Id={0}:{1}, Version={2}",
            owner_.id_,
            owner_.InstanceId,
            version_);
        TransitionToDeactivated(operation->Parent);
    }

    void TransitionToDeactivated(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToDeactivated();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr);
            return;
        }
        else
        {
            hostingTrace.ApplicationDeactivated(owner_.TraceId, owner_.Id.ToString(), version_.ToString(), owner_.InstanceId);
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
    Application2 & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationVersion version_;
    VersionedApplicationSPtr versionedApplication_;
};

// ********************************************************************************************************************
// Application2::ServicePackagesAsyncOperationBase Implementation
//
template<typename OperationType>
class Application2::ServicePackagesAsyncOperationBase
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ServicePackagesAsyncOperationBase)

public:
    ServicePackagesAsyncOperationBase(
        __in Application2 & owner,
        vector<OperationType> && packageInstanceOperations,
        bool isUpgradeOperation,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeout_(timeout),
        packageOperations_(move(packageInstanceOperations)),
        isUpgradeOperation_(isUpgradeOperation),
        statusMap_(),
        versionedApplication_()
    {
    }

    virtual ~ServicePackagesAsyncOperationBase()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out OperationStatusMapSPtr & statusMap)
    {
        auto thisPtr = AsyncOperation::End<ServicePackagesAsyncOperationBase>(operation);
        statusMap = thisPtr->statusMap_;
        return thisPtr->Error;
    }

protected:
    virtual AsyncOperationSPtr BeginServicePackagesOperation(
        vector<OperationType> && packageOperations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) = 0;

    virtual ErrorCode EndServicePackagesOperation(
        AsyncOperationSPtr const & operation,
        __out OperationStatusMapSPtr & statusMap) = 0;


    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = isUpgradeOperation_ ?
            owner_.TransitionToModifying_PackageUpgrade() :
            owner_.TransitionToModifying_PackageActivationDeactivation();

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        {
            AcquireReadLock lock(owner_.Lock);
            versionedApplication_ = owner_.versionedApplication__;
        }

        ASSERT_IF(
            versionedApplication_ == nullptr,
            "VersionedApplication object of Application {0}:{1} must be non-null in modifying state.",
            owner_.Id,
            owner_.InstanceId);

        PerformServicePackageOperations(thisSPtr);
    }

private:
    void PerformServicePackageOperations(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->BeginServicePackagesOperation(
            move(packageOperations_),
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishServicePackagesOperation(operation, false); },
            thisSPtr);
        FinishServicePackagesOperation(operation, true);
    }

    void FinishServicePackagesOperation(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = this->EndServicePackagesOperation(operation, statusMap_);
        if (!error.IsSuccess() && versionedApplication_->GetState() == VersionedApplication::Failed)
        {
            TransitionToFailed(operation->Parent, error);
            return;
        }
        else
        {
            TransitionToOpened(operation->Parent, error);
            return;
        }
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr, ErrorCode const servicePackagesOperationError)
    {
        auto transitionError = owner_.TransitionToActive();

        if ((isUpgradeOperation_ && !transitionError.IsSuccess()) ||
            (!isUpgradeOperation_ && !transitionError.IsSuccess() && !transitionError.IsError(ErrorCodeValue::OperationsPending)))
        {
            servicePackagesOperationError.ReadValue();
            TransitionToFailed(thisSPtr, transitionError);
            return;
        }
        else
        {
            TryComplete(thisSPtr, servicePackagesOperationError);
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        VersionedApplicationSPtr versionedApplication;
        {
            AcquireWriteLock lock(owner_.Lock);
            versionedApplication = move(owner_.versionedApplication__);
        }

        if (versionedApplication)
        {
            versionedApplication->AbortAndWaitForTerminationAsync(
                [this, failedError](AsyncOperationSPtr const & operation)
            {
                this->owner_.TransitionToFailed().ReadValue();
                this->TryComplete(operation->Parent, failedError);
            },
                thisSPtr);
        }
        else
        {
            this->owner_.TransitionToFailed().ReadValue();
            this->TryComplete(thisSPtr, failedError);
        }
    }

protected:
    Application2 & owner_;
    VersionedApplicationSPtr versionedApplication_;

private:
    TimeSpan const timeout_;
    vector<OperationType> packageOperations_;
    bool isUpgradeOperation_;
    OperationStatusMapSPtr statusMap_;
};

// ********************************************************************************************************************
// Application2::ActivateServicePackageInstancesAsyncOperation Implementation
//
class Application2::ActivateServicePackageInstancesAsyncOperation
    : public Application2::ServicePackagesAsyncOperationBase<ServicePackageInstanceOperation>
{
    DENY_COPY(ActivateServicePackageInstancesAsyncOperation)

public:
    ActivateServicePackageInstancesAsyncOperation(
        __in Application2 & owner,
        vector<ServicePackageInstanceOperation> && packageActivations,
        bool ensureLatestVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ServicePackagesAsyncOperationBase(
        owner,
        move(packageActivations),
        false,
        timeout,
        callback,
        parent),
        ensureLatestVersion_(ensureLatestVersion)
    {
    }

    virtual ~ActivateServicePackageInstancesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out OperationStatusMapSPtr & statusMap)
    {
        return ServicePackagesAsyncOperationBase::End(operation, statusMap);
    }

protected:
    virtual AsyncOperationSPtr BeginServicePackagesOperation(
        vector<ServicePackageInstanceOperation> && packageOperations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return versionedApplication_->BeginActivateServicePackageInstances(
            move(packageOperations),
            ensureLatestVersion_,
            timeout,
            callback,
            parent);
    }

    virtual ErrorCode EndServicePackagesOperation(
        AsyncOperationSPtr const & operation,
        __out OperationStatusMapSPtr & statusMap)
    {
        return versionedApplication_->EndActivateServicePackages(operation, statusMap);
    }

private:
    bool ensureLatestVersion_;
};

// ********************************************************************************************************************
// Application2::UpdgradeServicePackagesAsyncOperation Implementation
//
class Application2::UpgradeServicePackagesAsyncOperation
    : public Application2::ServicePackagesAsyncOperationBase<ServicePackageOperation>
{
    DENY_COPY(UpgradeServicePackagesAsyncOperation)

public:
    UpgradeServicePackagesAsyncOperation(
        __in Application2 & owner,
        vector<ServicePackageOperation> && packageUpgrades,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ServicePackagesAsyncOperationBase(
        owner,
        move(packageUpgrades),
        true,
        timeout,
        callback,
        parent)
    {
    }

    virtual ~UpgradeServicePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out OperationStatusMapSPtr & statusMap)
    {
        return ServicePackagesAsyncOperationBase::End(operation, statusMap);
    }

protected:
    virtual AsyncOperationSPtr BeginServicePackagesOperation(
        vector<ServicePackageOperation> && packageOperations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return versionedApplication_->BeginUpgradeServicePackages(move(packageOperations), timeout, callback, parent);
    }

    virtual ErrorCode EndServicePackagesOperation(
        AsyncOperationSPtr const & operation,
        __out OperationStatusMapSPtr & statusMap)
    {
        return versionedApplication_->EndUpgradeServicePackages(operation, statusMap);
    }
};

// ********************************************************************************************************************
// Application2::DeactivateServicePackageInstancesAsyncOperation Implementation
//
class Application2::DeactivateServicePackageInstancesAsyncOperation
    : public Application2::ServicePackagesAsyncOperationBase<ServicePackageInstanceOperation>
{
    DENY_COPY(DeactivateServicePackageInstancesAsyncOperation)

public:
    DeactivateServicePackageInstancesAsyncOperation(
        __in Application2 & owner,
        vector<ServicePackageInstanceOperation> && packageDeactivations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ServicePackagesAsyncOperationBase(
        owner,
        move(packageDeactivations),
        false,
        timeout,
        callback,
        parent)
    {
    }

    virtual ~DeactivateServicePackageInstancesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out OperationStatusMapSPtr & statusMap)
    {
        return ServicePackagesAsyncOperationBase::End(operation, statusMap);
    }

protected:
    virtual AsyncOperationSPtr BeginServicePackagesOperation(
        vector<ServicePackageInstanceOperation> && packageOperations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return versionedApplication_->BeginDeactivateServicePackageInstances(move(packageOperations), timeout, callback, parent);
    }

    virtual ErrorCode EndServicePackagesOperation(
        AsyncOperationSPtr const & operation,
        __out OperationStatusMapSPtr & statusMap)
    {
        return versionedApplication_->EndDeactivateServicePackages(operation, statusMap);
    }
};

// ********************************************************************************************************************
// Application2 Implementation
//
Application2::Application2(
    HostingSubsystemHolder const & hostingHolder,
    ApplicationIdentifier const & applicationId,
    wstring const & applicationName)
    : StateMachine(Inactive),
    hostingHolder_(hostingHolder),
    id_(applicationId),
    appName_(applicationName),
    version__(ApplicationVersion::Zero),
    packageDescription__(),
    versionedApplication__(),
    instanceId_(DateTime::Now().Ticks),
    runLayout_(hostingHolder.RootedObject.DeploymentFolder)
{
    this->SetTraceId(hostingHolder.Root->TraceId);
    WriteNoise(TraceType, TraceId, "Application2.constructor: {0} ({1}:{2})", static_cast<void*>(this), id_, instanceId_);
}

Application2::~Application2()
{
    WriteNoise(TraceType, TraceId, "Application2.destructor: {0} ({1}:{2})", static_cast<void*>(this), id_, instanceId_);
}

AsyncOperationSPtr Application2::BeginActivate(
    ApplicationVersion const & applicationVersion,
    bool ensureLatestVersion,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        applicationVersion,
        ensureLatestVersion,
        timeout,
        callback,
        parent);

}

ErrorCode Application2::EndActivate(
    AsyncOperationSPtr const & operation)
{
    return ActivateAsyncOperation::End(operation);
}

AsyncOperationSPtr Application2::BeginUpgrade(
    ApplicationVersion const & applicationVersion,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpgradeAsyncOperation>(
        *this,
        applicationVersion,
        timeout,
        callback,
        parent);
}

ErrorCode Application2::EndUpgrade(
    AsyncOperationSPtr const & operation)
{
    return UpgradeAsyncOperation::End(operation);
}

AsyncOperationSPtr Application2::BeginActivateServicePackageInstances(
    vector<ServicePackageInstanceOperation> && packageActivations,
    bool ensureLatestVersion,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateServicePackageInstancesAsyncOperation>(
        *this,
        move(packageActivations),
        ensureLatestVersion,
        timeout,
        callback,
        parent);
}

ErrorCode Application2::EndActivateServicePackages(
    AsyncOperationSPtr const & operation,
    __out OperationStatusMapSPtr & statusMap)
{
    return ActivateServicePackageInstancesAsyncOperation::End(operation, statusMap);
}

AsyncOperationSPtr Application2::BeginUpgradeServicePackages(
    vector<ServicePackageOperation> && packageUpgrades,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpgradeServicePackagesAsyncOperation>(
        *this,
        move(packageUpgrades),
        timeout,
        callback,
        parent);
}

ErrorCode Application2::EndUpgradeServicePackages(
    AsyncOperationSPtr const & operation,
    __out OperationStatusMapSPtr & statusMap)
{
    return UpgradeServicePackagesAsyncOperation::End(operation, statusMap);
}

AsyncOperationSPtr Application2::BeginDeactivateServicePackageInstances(
    vector<ServicePackageInstanceOperation> && packageDeactivations,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeactivateServicePackageInstancesAsyncOperation>(
        *this,
        move(packageDeactivations),
        timeout,
        callback,
        parent);
}

ErrorCode Application2::EndDeactivateServicePackages(
    AsyncOperationSPtr const & operation,
    __out OperationStatusMapSPtr & statusMap)
{
    return DeactivateServicePackageInstancesAsyncOperation::End(operation, statusMap);
}

AsyncOperationSPtr Application2::BeginDeactivate(
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

void Application2::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    DeactivateAsyncOperation::End(operation);
}

void Application2::OnAbort()
{
    ApplicationVersion version;
    VersionedApplicationSPtr versionedApplication = nullptr;
    {
        AcquireWriteLock lock(this->Lock);
        versionedApplication = move(this->versionedApplication__);
        version = this->version__;
    }

    if (versionedApplication)
    {
        versionedApplication->AbortAndWaitForTermination();
    }

    hostingTrace.ApplicationDeactivated(TraceId, Id.ToString(), version.ToString(), InstanceId);
}

ApplicationIdentifier const & Application2::get_Id() const
{
    return this->id_;
}

int64 const Application2::get_InstanceId() const
{
    return this->instanceId_;
}

wstring const & Application2::get_AppName() const
{
    return this->appName_;
}

ApplicationVersion const Application2::get_CurrentVersion() const
{
    {
        AcquireReadLock lock(this->Lock);
        return version__;
    }
}

bool Application2::IsActiveInVersion(ServiceModel::ApplicationVersion const & thisVersion) const
{
    {
        AcquireReadLock lock(this->Lock);

        auto currentState = this->GetState_CallerHoldsLock();

        return (((currentState & this->GetActiveStatesMask()) != 0) && (thisVersion.compare(this->version__) == 0));
    }
}

uint64 Application2::GetActiveStatesMask() const
{
    // Application is considered active when it is in one of following states.
    return (Active | Modifying_PackageUpgrade | Modifying_PackageActivationDeactivation | Analyzing | Upgrading);
}

VersionedApplicationSPtr Application2::GetVersionedApplication() const
{
    {
        AcquireReadLock lock(this->Lock);
        return versionedApplication__;
    }
}

HostingSubsystemHolder const & Application2::get_HostingHolder() const
{
    return this->hostingHolder_;
}

ErrorCode Application2::AnalyzeApplicationUpgrade(
    ApplicationUpgradeSpecification const & appUpgradeSpec,
    __out set<wstring, IsLessCaseInsensitiveComparer<wstring>> & affectedRuntimeIds)
{
    auto error = this->TransitionToAnalyzing();
    if(!error.IsSuccess())
    {
        return error;
    }

    VersionedApplicationSPtr currentVersionedApplication;
    {
        AcquireReadLock lock(this->Lock);
        currentVersionedApplication = this->versionedApplication__;
    }

    error = currentVersionedApplication->AnalyzeApplicationUpgrade(
        appUpgradeSpec,
        affectedRuntimeIds);

    this->TransitionToActive().ReadValue();
    return error;
}

void Application2::OnServiceTypesUnregistered(
    ServicePackageInstanceIdentifier const servicePackageInstanceId,
    vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
{
    auto versionedApp = this->GetVersionedApplication();
    if (versionedApp != nullptr)
    {
        versionedApp->OnServiceTypesUnregistered(servicePackageInstanceId, serviceTypeInstanceIds);
    }
}

void Application2::OnServiceTypeRegistrationNotFound(
    uint64 const registrationTableVersion,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    ASSERT_IFNOT(
        versionedServiceTypeId.Id.ApplicationId == this->Id,
        "Application:OnServiceTypeRegistrationNotFound: Id {0} does not match the ApplicationId of the specified versionedServiceTypeId {1}.",
        this->Id,
        versionedServiceTypeId);

    VersionedApplicationSPtr versionedApplication;
    {
        AcquireReadLock lock(this->Lock);
        versionedApplication = this->versionedApplication__;
    }

    if ((versionedApplication) &&
        (versionedApplication->Version == versionedServiceTypeId.servicePackageVersionInstance.Version.ApplicationVersionValue))
    {
        versionedApplication->OnServiceTypeRegistrationNotFound(
            registrationTableVersion,
            versionedServiceTypeId,
            servicePackageInstanceId);
    }
}

ErrorCode Application2::LoadApplicationPackageDescription(
    ApplicationIdentifier const &,
    ApplicationVersion const & appVersion,
    __out ApplicationPackageDescription & appPackageDescription)
{
    wstring packageFilePath = runLayout_.GetApplicationPackageFile(
        this->Id.ToString(),
        appVersion.ToString());

    if(!File::Exists(packageFilePath))
    {
        WriteWarning(
            TraceType,
            TraceId,
            "LoadApplicationDescription: The ApplicationPackage file is not found at {0}",
            packageFilePath);
        return ErrorCodeValue::FileNotFound;
    }

    ErrorCode error = Parser::ParseApplicationPackage(packageFilePath, appPackageDescription);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "LoadApplicationDescription: ErrorCode={0}, Id={1}, Version={2}",
        error,
        Id,
        appVersion);

    return error;
}

void Application2::NotifyDcaAboutServicePackages()
{
    VersionedApplicationSPtr versionedApplication = GetVersionedApplication();
    if (versionedApplication)
    {
        versionedApplication->NotifyDcaAboutServicePackages();
    }
}

DeployedApplicationQueryResult Application2::GetDeployedApplicationQueryResult()
{
    DeploymentStatus::Enum status;

    uint64 currentState = this->GetState();
    if(currentState & (Inactive|Activating))
    {
        status = DeploymentStatus::Activating;
    }
    else if(currentState & (Active|Modifying_PackageUpgrade|Modifying_PackageActivationDeactivation|Analyzing))
    {
        status = DeploymentStatus::Active;
    }
    else if(currentState == Upgrading)
    {
        status = DeploymentStatus::Upgrading;
    }
    else if(currentState & (Deactivating|Deactivated|Failed|Aborted))
    {
        status = DeploymentStatus::Deactivating;
    }
    else
    {
        Assert::CodingError("Application2: State={0} should be mapped to FABRIC_DEPLOYMENT_STATUS", currentState);
    }

    auto applicationIdStr = this->Id.ToString();

    return DeployedApplicationQueryResult(
        this->AppName,
        this->Id.ApplicationTypeName,
        status,
        runLayout_.GetApplicationWorkFolder(applicationIdStr),
        runLayout_.GetApplicationLogFolder(applicationIdStr),
        runLayout_.GetApplicationTempFolder(applicationIdStr));
}

Common::ErrorCode Application2::IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent) const
{
    VersionedApplicationSPtr versionedApplication = GetVersionedApplication();
    if(versionedApplication)
    {
        versionedApplication->IsCodePackageLockFilePresent(isCodePackageLockFilePresent);
        return ErrorCodeValue::Success;
    }

    WriteWarning(
        TraceType,
        TraceId,
        "IsCodePackageLockFilePresent: VersionedApplication not found.");

    return ErrorCodeValue::NotFound;
}
