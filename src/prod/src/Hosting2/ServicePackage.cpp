// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ServicePackage");

const int ServicePackage2::ServicePackageActivatedEventVersion;
const int ServicePackage2::ServicePackageDeactivatedEventVersion;
const int ServicePackage2::ServicePackageUpgradedEventVersion;

// ********************************************************************************************************************
// ServicePackage2::ActivateAsyncOperation Implementation
//
class ServicePackage2::ActivateAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        __in ServicePackage2 & owner,
        ServicePackageVersionInstance const & packageVersionInstance,
        bool ensureLatestVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        packageVersionInstance_(packageVersionInstance),
        ensureLatestVersion_(ensureLatestVersion),
        timeoutHelper_(timeout),
        packageDescription_(),
        versionedServicePackage_()
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
        ServicePackageVersionInstance currentVersionInstance;
        bool isActive = false;
        int versionComparision = 0;
        bool failCurrentVersion = false;

        {
            AcquireReadLock lock(owner_.Lock);
            if (owner_.GetState_CallerHoldsLock() == ServicePackage2::Active)
            {
                isActive = true;
                currentVersionInstance = owner_.currentVersionInstance__;
            }

            packageDescription_ = owner_.packageDescription__;
        }

        versionComparision = currentVersionInstance.compare(packageVersionInstance_);
        if (isActive)
        {
            if (versionComparision == 0)
            {
                // the service package is already active in teh right version
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                return;
            }
            else
            {
                WriteInfo(
                    TraceType,
                    owner_.TraceId,
                    "ServicePackageVersionMismatch: Id={0}:{1}, CurrentVersion={2}, RequestedVersion={3}, EnsureLatestVersion={4}",
                    owner_.Id,
                    owner_.InstanceId,
                    owner_.currentVersionInstance__,
                    packageVersionInstance_,
                    ensureLatestVersion_);

                if (!ensureLatestVersion_)
                {
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::HostingServicePackageVersionMismatch));
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
                        // abort the current version after transitining to activating
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

        ASSERT_IF(
            packageDescription_.RolloutVersionValue != packageVersionInstance_.Version.RolloutVersionValue,
            "The RolloutVersion in PackageDescription does not match the RolloutVersion in the PackageVersionInstance. PackageDescription:{0}, PackageVersionInstance:{1}",
            packageDescription_.RolloutVersionValue,
            packageVersionInstance_.Version.RolloutVersionValue);

        if (failCurrentVersion)
        {
            TransitionToFailed(thisSPtr, ErrorCode(ErrorCodeValue::HostingEntityAborted));
            return;
        }

        vector<ServiceTypeInstanceIdentifier> serviceTypeInstanceIds;
        error = owner_.GetServiceTypes(packageDescription_.ManifestVersion, serviceTypeInstanceIds);
        if(!error.IsSuccess())
        {            
            TransitionToFailed(thisSPtr, error);
            return;
        }

        versionedServicePackage_ = make_shared<VersionedServicePackage>(
            ServicePackage2Holder(owner_, owner_.CreateComponentRoot()), 
            packageVersionInstance_,
            packageDescription_,
            serviceTypeInstanceIds);

        OpenVersionedServicePackage(thisSPtr);
    }

private:
    void OpenVersionedServicePackage(AsyncOperationSPtr const & thisSPtr)
    {        
        auto timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(OpenVersionedServicePackage): Id={0}:{1}, Version={2}, Timeout={3}",
            owner_.Id,
            owner_.InstanceId,
            packageVersionInstance_,
            timeout);
        auto operation = versionedServicePackage_->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishOpenVersionedServicePackage(operation, false); },
            thisSPtr);
        FinishOpenVersionedServicePackage(operation, true);
    }

    void FinishOpenVersionedServicePackage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = versionedServicePackage_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(OpenVersionedServicePackage): Id={0}:{1}, Version={2}, ErrorCode={3}",
            owner_.Id,
            owner_.InstanceId,
            packageVersionInstance_,
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
            owner_.versionedServicePackage__ = versionedServicePackage_;
            owner_.currentVersionInstance__ = packageVersionInstance_;
        }        

        auto error = owner_.TransitionToActive();        
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }
        else
        {
            hostingTrace.ServicePackageActivated(owner_.TraceId, owner_.Id.ToString(), packageVersionInstance_.ToString(), owner_.InstanceId);
            hostingTrace.ServicePackageActivatedNotifyDca(
                ServicePackageActivatedEventVersion, 
                owner_.HostingHolder.RootedObject.NodeName,
                owner_.runLayout_.Root, 
                owner_.Id.ApplicationId.ToString(), 
                packageVersionInstance_.Version.ApplicationVersionValue.ToString(),
                owner_.Id.ServicePackageName,
                packageVersionInstance_.Version.RolloutVersionValue.ToString());
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        if (versionedServicePackage_)
        {
            versionedServicePackage_->AbortAndWaitForTerminationAsync(
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
    ServicePackage2 & owner_;
    ServicePackageVersionInstance packageVersionInstance_;
    VersionedServicePackageSPtr versionedServicePackage_;
    ServicePackageDescription packageDescription_;
    TimeoutHelper timeoutHelper_;
    bool ensureLatestVersion_;
};

// ********************************************************************************************************************
// ServicePackage2::UpgradeAsyncOperation Implementation
//
class ServicePackage2::UpgradeAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UpgradeAsyncOperation)

public:
    UpgradeAsyncOperation(
        __in ServicePackage2 & owner,
        ServicePackageDescription const & newPackageDescription,
        ServicePackageVersionInstance newPackageVersionInstance,
        UpgradeType::Enum const upgradeType,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        fromVersionInstance_(),
        toVersionInstance_(newPackageVersionInstance),
        fromPackageDescription_(),
        toPackageDescription_(newPackageDescription),
        fromVersionedServicePackage_(),        		
        toVersionedServicePackage_(),        
        upgradeType_(upgradeType),
        timeoutHelper_(timeout)
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
        ASSERT_IF(upgradeType_ == UpgradeType::Invalid, "UpgradeType cannot be invalid");

        auto error = owner_.TransitionToUpgrading();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        {
            AcquireReadLock lock(owner_.Lock);
            fromVersionInstance_ = owner_.currentVersionInstance__;
            fromVersionedServicePackage_ = owner_.versionedServicePackage__;
            fromPackageDescription_ = owner_.packageDescription__;
        }

        if (fromVersionInstance_ == toVersionInstance_)
        {
            // the service package is already running in the required version
            TransitionToActive(thisSPtr);
            return;
        }

        DoUpgrade(thisSPtr);
    }

private:
    void DoUpgrade(AsyncOperationSPtr const & thisSPtr)
    {                
        bool versionUpdateOnly = (fromPackageDescription_.ContentChecksum == toPackageDescription_.ContentChecksum);

        bool hasRgChange = fromPackageDescription_.ResourceGovernanceDescription.HasRgChange(
            toPackageDescription_.ResourceGovernanceDescription,
            fromPackageDescription_.DigestedCodePackages,
            toPackageDescription_.DigestedCodePackages);

        if(upgradeType_ != UpgradeType::Rolling_ForceRestart &&
           (versionUpdateOnly || fromVersionInstance_.Version.RolloutVersionValue.Major == toVersionInstance_.Version.RolloutVersionValue.Major) &&
            !hasRgChange)
        {        
            // perform switch on the existing versioned service package
            DoSwitch(thisSPtr);
        }
        else
        {
            // close existing versioned service package and open new versioned service package
            DoCloseOpen(thisSPtr);            
        }
    }    

    void DoSwitch(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(SwitchServicePackage): Id={0}:{1}, FromVersion={2}, ToVersion={3}, Timeout={4}",
            owner_.Id,
            owner_.InstanceId,
            fromVersionInstance_,
            toVersionInstance_,
            timeout);

        auto operation = fromVersionedServicePackage_->BeginSwitch(
            toVersionInstance_,
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

        auto error = fromVersionedServicePackage_->EndSwitch(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            owner_.TraceId,
            "End(SwitchServicePackage): Id={0}:{1}, FromVersion={2}, ToVersion={3}, ErrorCode={4}",
            owner_.Id,
            owner_.InstanceId,
            fromVersionInstance_,
            toVersionInstance_,
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

    void DoCloseOpen(AsyncOperationSPtr const & thisSPtr)
    {
        CloseOldVersionedServicePackage(thisSPtr);
    }

    void CloseOldVersionedServicePackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(CloseOldVersionedServicePackage): Id={0}:{1}, Version={2}, Timeout={3}",
            owner_.Id,
            owner_.InstanceId,
            fromVersionInstance_,
            timeout);
        auto operation = fromVersionedServicePackage_->BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishCloseVersionedServicePackage(operation, false); },
            thisSPtr);
        FinishCloseVersionedServicePackage(operation, true);
    }

    void FinishCloseVersionedServicePackage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        fromVersionedServicePackage_->EndClose(operation);
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "End(CloseOldVersionedServicePackage): Id={0}:{1}, Version={2}",
            owner_.Id,
            owner_.InstanceId,
            fromVersionInstance_);

        //If UpgradeService Package has Aborted, then do not open
        if (operation->Parent->Error.IsError(ErrorCodeValue::OperationCanceled))
        {
            TransitionToFailed(operation->Parent, operation->Parent->Error);
            return;
        }

        OpenNewVersionedServicePackage(operation->Parent);
    }

    void OpenNewVersionedServicePackage(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ServiceTypeInstanceIdentifier> serviceTypeIds;
        auto error = owner_.GetServiceTypes(toPackageDescription_.ManifestVersion, serviceTypeIds);
        if(!error.IsSuccess())
        {            
            TransitionToFailed(thisSPtr, error);
            return;
        }

        toVersionedServicePackage_ = make_shared<VersionedServicePackage>(
            ServicePackage2Holder(owner_, owner_.CreateComponentRoot()),
            toVersionInstance_,
            toPackageDescription_,
            serviceTypeIds);        

        auto timeout = timeoutHelper_.GetRemainingTime();
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(OpenNewVersionedServicePackage): Id={0}:{1}, Version={2}, Timeout={3}",
            owner_.Id,
            owner_.InstanceId,
            toVersionInstance_,
            timeout);
        auto operation = toVersionedServicePackage_->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishOpen(operation, false); },
            thisSPtr);
        FinishOpen(operation, true);
    }

    void FinishOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = toVersionedServicePackage_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(OpenNewVersionedServicePackage): Id={0}, Version={1}, ErrorCode={2}",
            owner_.Id,
            toVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            toVersionedServicePackage_->AbortAndWaitForTerminationAsync(
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
            // If we transition directly to Active state, toVersionedServicePackage_
            // and toPackageDescription_ will not be set. Checking that they are 
            // initialized before assigning
            if (toVersionedServicePackage_)
            {
                owner_.versionedServicePackage__ = toVersionedServicePackage_;
            }
            
            if(!toPackageDescription_.ManifestName.empty())
            {
                owner_.packageDescription__ = toPackageDescription_;
            }

            owner_.currentVersionInstance__ = toVersionInstance_;
            
        }

        auto error = owner_.TransitionToActive();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
        }
        else
        {
            hostingTrace.ServicePackageUpgradedNotifyDca(
                ServicePackageUpgradedEventVersion, 
                owner_.HostingHolder.RootedObject.NodeName,
                owner_.runLayout_.Root, 
                owner_.Id.ApplicationId.ToString(), 
                toVersionInstance_.Version.ApplicationVersionValue.ToString(),
                owner_.Id.ServicePackageName,
                toVersionInstance_.Version.RolloutVersionValue.ToString());
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        VersionedServicePackageSPtr versionedServicePackage;
        {
            AcquireWriteLock lock(owner_.Lock);
            versionedServicePackage = move(owner_.versionedServicePackage__);
        }

        if (versionedServicePackage)
        {
            versionedServicePackage->AbortAndWaitForTerminationAsync(
                [this, failedError](AsyncOperationSPtr const & operation)
            {
                this->owner_.TransitionToFailed().ReadValue();
                this->TryComplete(operation->Parent, failedError);
            },
                thisSPtr);
        }
        else
        {
            owner_.TransitionToFailed().ReadValue();
            TryComplete(thisSPtr, failedError);
        }
    }

private:
    ServicePackage2 & owner_;
    ServicePackageVersionInstance fromVersionInstance_;
    ServicePackageVersionInstance toVersionInstance_;
    VersionedServicePackageSPtr fromVersionedServicePackage_;
    VersionedServicePackageSPtr toVersionedServicePackage_;
    ServicePackageDescription fromPackageDescription_;
    ServicePackageDescription toPackageDescription_;
    UpgradeType::Enum upgradeType_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// ServicePackage2::DeactivateAsyncOperation Implementation
//
class ServicePackage2::DeactivateAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DeactivateAsyncOperation)

public:
    DeactivateAsyncOperation(
        __in ServicePackage2 & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        versionedServicePackage_(),
        packageVersionInstance_()
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
        if (owner_.GetState() == ServicePackage2::Deactivated)
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
            AcquireWriteLock lock(owner_.Lock);
            versionedServicePackage_ = move(owner_.versionedServicePackage__);
            packageVersionInstance_ = owner_.currentVersionInstance__;
        }

        if (versionedServicePackage_)
        {
            CloseVersionedServicePackage(thisSPtr);
        }
        else
        {
            TransitionToDeactivated(thisSPtr);
        }
    }

private:
    void CloseVersionedServicePackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CloseVersionedServicePackage): Id={0}:{1}, Version={2}, Timeout={3}",
            owner_.Id,
            owner_.InstanceId,
            packageVersionInstance_,
            timeout);
        auto operation = versionedServicePackage_->BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishCloseVersionedServicePackage(operation, false); },
            thisSPtr);
        FinishCloseVersionedServicePackage(operation, true);
    }

    void FinishCloseVersionedServicePackage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        versionedServicePackage_->EndClose(operation);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(CloseVersionedServicePackage): Id={0}:{1}, Version={2}",
            owner_.Id,
            owner_.InstanceId,
            packageVersionInstance_);
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
            // remove registration from this service package now instead of waiting for the processing of 
            // code package termination, this will ensure that invalid registrations are not used anymore.
            owner_.HostingHolder.RootedObject.FabricRuntimeManagerObj->OnServicePackageClosed(owner_.Id, owner_.InstanceId);

            hostingTrace.ServicePackageDeactivated(owner_.TraceId, owner_.Id.ToString(), packageVersionInstance_.ToString(), owner_.InstanceId);
            hostingTrace.ServicePackageDeactivatedNotifyDca(
                ServicePackageDeactivatedEventVersion, 
                owner_.HostingHolder.RootedObject.NodeName,
                owner_.Id.ApplicationId.ToString(), 
                owner_.Id.ServicePackageName);

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
    ServicePackage2 & owner_;
    TimeoutHelper timeoutHelper_;
    ServicePackageVersionInstance packageVersionInstance_;
    VersionedServicePackageSPtr versionedServicePackage_;
};

// ********************************************************************************************************************
// ServicePackage2 Implementation
//
ServicePackage2::ServicePackage2(
    HostingSubsystemHolder const & hostingHolder,
    ServicePackageDescription const & packageDescription,
    ServicePackageInstanceContext && packageInstanceContext)
    : ComponentRoot(),
    StateMachine(Inactive),
    hostingHolder_(hostingHolder),
    context_(move(packageInstanceContext)),
    instanceId_(hostingHolder.RootedObject.GetNextSequenceNumber()),
    currentVersionInstance__(ServicePackageVersionInstance::Invalid),
    versionedServicePackage__(),
    packageDescription__(packageDescription),
    runLayout_(hostingHolder.RootedObject.DeploymentFolder)
{
    this->SetTraceId(hostingHolder_.Root->TraceId);
    WriteNoise(
        TraceType, TraceId, 
        "ServicePackage.constructor: {0} ({1}:{2})", 
        static_cast<void*>(this),
        Id,
        InstanceId);
}

ServicePackage2::~ServicePackage2()
{
    WriteNoise(
        TraceType, TraceId, 
        "ServicePackage.destructor: {0} ({1}:{2})", 
        static_cast<void*>(this),
        Id,
        InstanceId);
}

// activates the specified version of the service package
AsyncOperationSPtr ServicePackage2::BeginActivate(
    ServicePackageVersionInstance const & packageVersionInstance,
    bool ensureLatestVersion,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        packageVersionInstance,
        ensureLatestVersion,
        timeout,
        callback,
        parent);
}

ErrorCode ServicePackage2::EndActivate(
    AsyncOperationSPtr const & operation)
{
    return ActivateAsyncOperation::End(operation);
}

// upgrades the service package to the specified version
AsyncOperationSPtr ServicePackage2::BeginUpgrade(
    ServicePackageDescription const & packageDescription,
    ServicePackageVersionInstance const & newPackageVersionInstance,
    UpgradeType::Enum const upgradeType,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpgradeAsyncOperation>(
        *this,
        packageDescription,
        newPackageVersionInstance,
        upgradeType,
        timeout,
        callback,
        parent);
}

ErrorCode ServicePackage2::EndUpgrade(
    AsyncOperationSPtr const & operation)
{
    return UpgradeAsyncOperation::End(operation);
}

// deactivates if active
AsyncOperationSPtr ServicePackage2::BeginDeactivate(
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

void ServicePackage2::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    DeactivateAsyncOperation::End(operation);
}

void ServicePackage2::OnAbort()
{
    VersionedServicePackageSPtr versionedServicePackage;
    ServicePackageVersionInstance packageVersionInstance;
    {
        AcquireWriteLock lock(Lock);
        versionedServicePackage = move(versionedServicePackage__);
        packageVersionInstance = this->currentVersionInstance__;
    }

    if (versionedServicePackage)
    {
        versionedServicePackage->AbortAndWaitForTermination();
    }    

    // remove registration from this service package now instead of waiting for the processing of 
    // code package termination, this will ensure that invalid registrations are not used anymore.
    HostingHolder.RootedObject.FabricRuntimeManagerObj->OnServicePackageClosed(Id, InstanceId);

    // TODO: This should be traced as ServicePackgeAborted to differentiate from normal deactivation.
    hostingTrace.ServicePackageDeactivated(TraceId, Id.ToString(), packageVersionInstance.ToString(), InstanceId);
    hostingTrace.ServicePackageDeactivatedNotifyDca(
        ServicePackageDeactivatedEventVersion, 
        hostingHolder_.RootedObject.NodeName,
        context_.ServicePackageInstanceId.ApplicationId.ToString(),
        context_.ServicePackageInstanceId.ServicePackageName);
}

wstring const & ServicePackage2::get_ServicePackageName() const
{ 
    return this->context_.ServicePackageName;
}

wstring const & ServicePackage2::get_ApplicationName() const
{ 
    return this->context_.ApplicationName;
}

ServicePackageInstanceIdentifier const & ServicePackage2::get_Id() const
{ 
    return this->context_.ServicePackageInstanceId;
}

int64 const ServicePackage2::get_InstanceId() const
{ 
    return this->instanceId_; 
}

ApplicationEnvironmentContextSPtr const & ServicePackage2::get_ApplicationEnvironment() const
{
    return this->context_.ApplicationEnvironment;
}

HostingSubsystemHolder const & ServicePackage2::get_HostingHolder() const
{
    return this->hostingHolder_;
}

DeployedServiceManifestQueryResult ServicePackage2::GetDeployedServiceManifestQueryResult() const
{
    wstring serviceManifestName, serviceManifestVersion;
    DeploymentStatus::Enum status;
    {
        AcquireReadLock lock(Lock);        
        serviceManifestName = packageDescription__.ManifestName;
        serviceManifestVersion = packageDescription__.ManifestVersion;

        uint64 currentState = this->GetState_CallerHoldsLock();
        if(currentState & (Inactive|Activating))
        {
            status = DeploymentStatus::Activating;
        }
        else if(currentState & (Active|Analyzing))
        {
            status = DeploymentStatus::Active;
        }
        else if(currentState == Upgrading)
        {
            status = DeploymentStatus::Upgrading;
        }
        else if(currentState & (Deactivating|Deactivated|Aborted|Failed))
        {
            status = DeploymentStatus::Deactivating;
        }
        else
        {
            Assert::CodingError("ServicePackage2: State={0} should be mapped to DeploymentStatus", currentState);
        }
    }

    if (this->Id.ApplicationId.IsSystem())
    {
        auto fabricVersion = this->HostingHolder.RootedObject.FabricNodeConfigObj.NodeVersion.Version;
        return DeployedServiceManifestQueryResult(
            serviceManifestName, 
            context_.ServicePackageInstanceId.PublicActivationId,
            fabricVersion.CodeVersion.ToString(), 
            status);
    }
    else
    {
        return DeployedServiceManifestQueryResult(
            serviceManifestName,
            context_.ServicePackageInstanceId.PublicActivationId,
            serviceManifestVersion, 
            status);
    }
}

VersionedServicePackageSPtr ServicePackage2::GetVersionedServicePackage() const
{
    {
        AcquireReadLock lock(Lock);
        return versionedServicePackage__;
    }
}

void ServicePackage2::OnServiceTypesUnregistered(
    vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
{
    auto versionedServicePackage = this->GetVersionedServicePackage();
    if (versionedServicePackage != nullptr)
    {
        versionedServicePackage->OnServiceTypesUnregistered(serviceTypeInstanceIds);
    }
}

ULONG ServicePackage2::GetFailureCount() const
{
    VersionedServicePackageSPtr versionedServicePackage;
    {
        AcquireReadLock lock(this->Lock);
        versionedServicePackage = this->versionedServicePackage__;
    }

    if (versionedServicePackage)
    {
        return versionedServicePackage->GetFailureCount();
    }
    else
    {
        return 0;
    }
}

ErrorCode ServicePackage2::GetSharedPackageFolders(
    wstring const & applicationTypeName,
    vector<wstring> & sharedPackages)
{
    for(auto iter = this->packageDescription__.DigestedCodePackages.begin(); iter != this->packageDescription__.DigestedCodePackages.end(); ++iter)
    {
        if(iter->IsShared)
        {
            sharedPackages.push_back(this->hostingHolder_.RootedObject.SharedLayout.GetCodePackageFolder(
                applicationTypeName,
                packageDescription__.ManifestName,
                iter->CodePackage.Name,
                iter->CodePackage.Version));
        }
    }

    for(auto iter = this->packageDescription__.DigestedConfigPackages.begin(); iter != this->packageDescription__.DigestedConfigPackages.end(); ++iter)
    {
        if(iter->IsShared)
        {
            sharedPackages.push_back(this->hostingHolder_.RootedObject.SharedLayout.GetConfigPackageFolder(
                applicationTypeName,
                packageDescription__.ManifestName,
                iter->ConfigPackage.Name,
                iter->ConfigPackage.Version));
        }
    }

    for(auto iter = this->packageDescription__.DigestedDataPackages.begin(); iter != this->packageDescription__.DigestedDataPackages.end(); ++iter)
    {
        if(iter->IsShared)
        {
            sharedPackages.push_back(this->hostingHolder_.RootedObject.SharedLayout.GetConfigPackageFolder(
                applicationTypeName,
                packageDescription__.ManifestName,
                iter->DataPackage.Name,
                iter->DataPackage.Version));
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServicePackage2::AnalyzeApplicationUpgrade(
    ServicePackageVersionInstance const & newPackageVersionInstance,
    UpgradeType::Enum const upgradeType,
    __out set<wstring, IsLessCaseInsensitiveComparer<wstring>> & affectedRuntimeIds)
{
    auto error = this->TransitionToAnalyzing();
    if(!error.IsSuccess())
    {
        return error;
    }

    ServicePackageDescription newPackageDescription;
    error = this->LoadServicePackageDescription(newPackageVersionInstance.Version, newPackageDescription);
    if(!error.IsSuccess())
    {
        this->TransitionToActive().ReadValue();
        return error;
    }
 
    ServicePackageVersionInstance currentPackageVersionInstance;  
    ServicePackageDescription currentPackageDescription;    
    VersionedServicePackageSPtr currentVersionedServicePackage;    
    {
        AcquireReadLock lock(this->Lock);       
        currentPackageVersionInstance = this->currentVersionInstance__;
        currentPackageDescription = this->packageDescription__;
        currentVersionedServicePackage = this->versionedServicePackage__;
    }
    
    if(currentPackageVersionInstance == newPackageVersionInstance || 
        ((currentPackageVersionInstance.Version.RolloutVersionValue == newPackageVersionInstance.Version.RolloutVersionValue || currentPackageDescription.ContentChecksum == newPackageDescription.ContentChecksum) && 
        upgradeType != UpgradeType::Rolling_ForceRestart))
    {        
        // None of the CodePackages in this ServicePackage will be affected        
        this->TransitionToActive().ReadValue();
        return ErrorCodeValue::Success;
    } 

    if(currentPackageVersionInstance.Version.RolloutVersionValue.Major != newPackageVersionInstance.Version.RolloutVersionValue.Major ||
        upgradeType == UpgradeType::Rolling_ForceRestart)
    {
        // All the CodePackages in this ServicePackage will be affected
        error = this->HostingHolder.Value.FabricRuntimeManagerObj->GetRuntimeIds(this->Id, affectedRuntimeIds);        
    }
    else
    {
        // Some of the CodePackages in this ServicePackage will be affected
        error = currentVersionedServicePackage->AnalyzeApplicationUpgrade(newPackageVersionInstance, newPackageDescription, affectedRuntimeIds);    
    }

    this->TransitionToActive().ReadValue();
    return error;
}

void ServicePackage2::OnServiceTypeRegistrationNotFound(
    uint64 const registrationTableVersion,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{    
    auto versionedServicePackage = this->GetVersionedServicePackage();
    if(versionedServicePackage)
    {
        versionedServicePackage->OnServiceTypeRegistrationNotFound(registrationTableVersion, versionedServiceTypeId, servicePackageInstanceId);
    }
}

ServicePackageVersionInstance const & ServicePackage2::get_CurrentVersionInstance() const
{
    AcquireReadLock lock(this->Lock);
    return this->currentVersionInstance__;
}

ErrorCode ServicePackage2::LoadServicePackageDescription(
    ServicePackageVersion const & packageVersion,
    __out ServicePackageDescription & packageDescription)
{
    wstring packageFilePath = runLayout_.GetServicePackageFile(
        this->Id.ApplicationId.ToString(), 
        this->ServicePackageName, 
        packageVersion.RolloutVersionValue.ToString());

    auto error = Parser::ParseServicePackage(
        packageFilePath,
        packageDescription);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "LoadServicePackageDescription: Id={0}, Version={1}, ErrorCode={2}",
        Id,
        packageVersion,
        error);

    return error;
}

ErrorCode ServicePackage2::GetServiceTypes(
    std::wstring const & manifestVersion,
    __out std::vector<ServiceTypeInstanceIdentifier> & serviceTypeInstanceIds)
{
    // The ServiceTypeGroup information is NOT present in ServicePackage,
    // hence getting the ServiceTypeNames that are allowed to register from
    // ServiceManifest
    wstring packageFilePath = runLayout_.GetServiceManifestFile(
        this->Id.ApplicationId.ToString(),
        this->ServicePackageName, 
        manifestVersion);

    ServiceManifestDescription manifestDescription;
    auto error = Parser::ParseServiceManifest(
        packageFilePath,
        manifestDescription);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "ParseServiceManifest: Id={0}, ManifestVersion={1}, ErrorCode={2}",
        Id,
        manifestVersion,
        error);

    if(!error.IsSuccess())
    {
        return error;
    }

    for(auto iter = manifestDescription.ServiceTypeNames.begin(); iter != manifestDescription.ServiceTypeNames.end(); ++iter)
    {
        serviceTypeInstanceIds.push_back(ServiceTypeInstanceIdentifier(this->Id, iter->ServiceTypeName));
    }

    for(auto iter = manifestDescription.ServiceGroupTypes.begin(); iter != manifestDescription.ServiceGroupTypes.end(); ++iter)
    {
        serviceTypeInstanceIds.push_back(ServiceTypeInstanceIdentifier(this->Id, iter->Description.ServiceTypeName));
        if(iter->UseImplicitFactory)
        {
            // If ServiceGroupType has UseImplicitFactory set, when all MemberTypes are registered,
            // fwp will register the ServiceGroupType
            for(auto memberIter = iter->Members.begin(); memberIter != iter->Members.end(); ++memberIter)
            {
                serviceTypeInstanceIds.push_back(ServiceTypeInstanceIdentifier(this->Id, memberIter->ServiceTypeName));
            }
        }
    }

    return ErrorCodeValue::Success;

}

void ServicePackage2::NotifyDcaAboutServicePackages()
{
    wstring applicationVersion;
    wstring rolloutVersion;
    bool notify = false;
    {
        AcquireReadLock lock(Lock);
        uint64 state = GetState_CallerHoldsLock();
        if ((Active == state) || (Analyzing == state))
        {
            applicationVersion = currentVersionInstance__.Version.ApplicationVersionValue.ToString();
            rolloutVersion = currentVersionInstance__.Version.RolloutVersionValue.ToString();
            notify = true;
        }
    }

    if (notify)
    {
        hostingTrace.ServicePackageActivatedNotifyDca(
            ServicePackageActivatedEventVersion, 
            hostingHolder_.RootedObject.NodeName,
            runLayout_.Root, 
            this->Id.ApplicationId.ToString(), 
            applicationVersion,
            this->Id.ServicePackageName,
            rolloutVersion);
    }
}

Common::ErrorCode ServicePackage2::IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent) const
{
    auto versionedServicePackage = this->GetVersionedServicePackage();
    if(versionedServicePackage)
    {
        return versionedServicePackage->IsCodePackageLockFilePresent(isCodePackageLockFilePresent);
    }

    return ErrorCodeValue::NotFound;
}
