// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("VersionedApplication");

// ********************************************************************************************************************
// VersionedApplication::OpenAsyncOperation Implementation
//
class VersionedApplication::OpenAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in VersionedApplication & owner,        
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),        
        timeoutHelper_(timeout),
        description_(),
        environmentContext_(),
        certificateAccessDescriptions_()
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
        if (owner_.GetState() == VersionedApplication::Opened)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        auto error = owner_.TransitionToOpening();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        {
            AcquireReadLock lock(owner_.Lock);
            description_ = owner_.packageDescription__;
        }

        RegisterToHealthManager(thisSPtr);
    }

private:
    void RegisterToHealthManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ApplicationObj.HostingHolder.RootedObject.HealthManagerObj->RegisterSource(
            owner_.ApplicationObj.Id, 
            owner_.ApplicationObj.AppName, 
            owner_.healthPropertyId_);

        if(!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        SetupApplicationEnvironment(thisSPtr);
    }

    void SetupApplicationEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        // Setup the environment manager for this application
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(SetupApplicationEnvironment): Id={0}, Version={1}, Timeout={2}",
            owner_.Id,
            owner_.Version,
            timeout);

        auto operation = owner_.EnvironmentManagerObj->BeginSetupApplicationEnvironment(
            owner_.Id,
            description_,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishSetupApplicationEnvironment(operation, false); },
            thisSPtr);
        FinishSetupApplicationEnvironment(operation, true);
    }

    void FinishSetupApplicationEnvironment(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EnvironmentManagerObj->EndSetupApplicationEnvironment(operation, environmentContext_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(SetupApplicationEnvironment): Id={0}, ErrorCode={1}",
            owner_.Id,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(operation->Parent, error);
        }
        else
        {
            SetupFolderAndCertificateAcls(operation->Parent);        
        }
    }

    void SetupFolderAndCertificateAcls(AsyncOperationSPtr const & thisSPtr)
    {
        vector<wstring> sidStrings;
        bool localAppGroup = false;
        if ((environmentContext_->PrincipalsContext != nullptr) &&
            (!environmentContext_->PrincipalsContext->PrincipalsInformation.empty()))
        {
            for(auto iter = environmentContext_->PrincipalsContext->PrincipalsInformation.begin(); iter != environmentContext_->PrincipalsContext->PrincipalsInformation.end(); ++iter)
            {
                if (!(*iter)->IsLocalUser)
                {
#if !defined(PLATFORM_UNIX)
                    sidStrings.push_back((*iter)->SidString);
#endif
                }
                else
                {
                    localAppGroup = true;
                }
            }
        }
#if defined(PLATFORM_UNIX)
        else {
            SidSPtr appSid;
            auto error = BufferedSid::CreateSPtr(Constants::AppRunAsAccount, appSid);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }
            wstring sidStr;
            error = appSid->ToString(sidStr);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }
            sidStrings.push_back(sidStr);
        }
#endif

        //In addition to runas accounts also ACL folders for Fabric account if fabric is not running as SYSTEM
        if (!owner_.EnvironmentManagerObj->IsSystem)
        {
#if defined(PLATFORM_UNIX)
            if (sidStrings.empty())
#endif
            sidStrings.push_back(owner_.EnvironmentManagerObj->CurrentUserSid);
        }
        if (localAppGroup)
        {
            wstring groupName = ApplicationPrincipals::GetApplicationLocalGroupName(
                owner_.ApplicationObj.HostingHolder.RootedObject.NodeId,
                owner_.Id.ApplicationNumber);
            SidSPtr groupSid;
            auto error = BufferedSid::CreateSPtr(groupName, groupSid);
            ASSERT_IF(error.IsError(ErrorCodeValue::NotFound), "Expected local group name {0} not found", groupName);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }
            wstring sidStr;
            error = groupSid->ToString(sidStr);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }
            sidStrings.push_back(sidStr);
        }
        if(!sidStrings.empty())
        {
            SetupResourceAcls(
                GENERIC_ALL,
                sidStrings,
                thisSPtr);
        }
        else
        {
            TransitionToOpened(thisSPtr);
        }
    }

    void SetupResourceAcls(
        DWORD const accessMask,
        vector<wstring> const & sids,
        AsyncOperationSPtr const & thisSPtr)
    {        
        vector<wstring> applicationFolders;
        map<wstring, map<DWORD, vector<wstring>>> resourceSidMapping;

        wstring applicationFolder = owner_.ApplicationObj.HostingHolder.RootedObject.RunLayout.GetApplicationFolder(owner_.Id.ToString());
        applicationFolders.push_back(applicationFolder);

        GetApplicationRootDirectories(applicationFolders);

        for(auto iter = description_.DigestedEnvironment.Policies.SecurityAccessPolicies.begin(); iter != description_.DigestedEnvironment.Policies.SecurityAccessPolicies.end(); ++iter)
        {
            DWORD access = FILE_GENERIC_READ;
            if(GrantAccessType::Enum::Full == (*iter).Rights)
            {
                access = FILE_ALL_ACCESS;
            }
            SecurityPrincipalInformationSPtr securityUser;
            auto error = environmentContext_->PrincipalsContext->TryGetPrincipalInfo((*iter).PrincipalRef, securityUser);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }
            auto it = resourceSidMapping.find((*iter).ResourceRef);
            if(it == resourceSidMapping.end())
            {
                resourceSidMapping.insert(make_pair((*iter).ResourceRef, map<DWORD, vector<wstring>>()));
                it = resourceSidMapping.find((*iter).ResourceRef);
            }
            auto maskIt = it->second.find(access);
            if(maskIt == it->second.end())
            {
                it->second.insert(make_pair(access, vector<wstring>()));
                maskIt = it->second.find(access);
            }
            maskIt->second.push_back(securityUser->SidString);
        }

        for(auto iter = description_.DigestedCertificates.SecretsCertificate.begin(); iter != description_.DigestedCertificates.SecretsCertificate.end(); ++iter)
        {
            if(!iter->X509FindValue.empty())
            {
                auto it = resourceSidMapping.find((*iter).Name);
                if(it != resourceSidMapping.end())
                {
                    for(auto itMask = (*it).second.begin(); itMask != (*it).second.end(); ++itMask)
                    {
                        CertificateAccessDescription certAccess(*iter, (*itMask).second, (*itMask).first);
                        certificateAccessDescriptions_.push_back(move(certAccess));
                    }
                }
                else
                {
                    CertificateAccessDescription certAccess(*iter, sids, FILE_ALL_ACCESS);
                    certificateAccessDescriptions_.push_back(move(certAccess));
                }
            }
        }

        auto operation = owner_.ApplicationObj.HostingHolder.RootedObject.FabricActivatorClientObj->BeginConfigureResourceACLs(
            sids,
            accessMask,
            certificateAccessDescriptions_,
            applicationFolders,
            owner_.Id.ApplicationNumber,
            owner_.ApplicationObj.HostingHolder.RootedObject.NodeId,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnResourcesConfigured(operation, false); },
            thisSPtr);
        OnResourcesConfigured(operation, true);
    }

    // Get all the app instance directories in the JBOD directory with symbolic links from the app instance work folder
    // ex: FooJBODDir:\directoryName\NodeId\%AppInstance%_App%App_Type%
    void GetApplicationRootDirectories(__inout std::vector<wstring> & applicationFolders)
    {
        wstring appInstanceId = owner_.Id.ToString();

        //Skip in case it is FabricApp as we donot setup the root folder for Fabric Application
        if (StringUtility::AreEqualCaseInsensitive(appInstanceId, ApplicationIdentifier::FabricSystemAppId->ToString()))
        {
            return;
        }

        Common::FabricNodeConfig::KeyStringValueCollectionMap const& jbodDrives = owner_.ApplicationObj.HostingHolder.RootedObject.FabricNodeConfigObj.LogicalApplicationDirectories;

        wstring nodeId = owner_.ApplicationObj.HostingHolder.RootedObject.NodeId;
        wstring rootDeployedfolder;
        for (auto iter = jbodDrives.begin(); iter != jbodDrives.end(); ++iter)
        {
            rootDeployedfolder = Path::Combine(iter->second, nodeId);
            rootDeployedfolder = Path::Combine(rootDeployedfolder, appInstanceId);
            applicationFolders.push_back(move(rootDeployedfolder));
        }
    }

    void OnResourcesConfigured(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.ApplicationObj.HostingHolder.RootedObject.FabricActivatorClientObj->EndConfigureResourceACLs(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "ConfigureCertificateACLs: error={0}",
            error);
        if(!error.IsSuccess())
        {
            if (!error.Message.empty())
            {
                TransitionToFailed(operation->Parent, error);
            }
            else
            {
                TransitionToFailed(operation->Parent, ErrorCode(error.ReadValue(),
                    wformatString("{0}{1}", StringResource::Get(IDS_HOSTING_ResourceACL_Failed), error.ErrorCodeValueToString())));
            }
        }
        else
        {
            TransitionToOpened(operation->Parent);
        }
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        {
            AcquireWriteLock lock(owner_.Lock);
            owner_.environmentContext__ = environmentContext_;
        }

        owner_.ApplicationObj.HostingHolder.RootedObject.HealthManagerObj->ReportHealth(
            owner_.ApplicationObj.Id,
            owner_.healthPropertyId_,
            SystemHealthReportCode::Hosting_ApplicationActivated,
            L"" /*extraDescription*/,
            SequenceNumber::GetNext());

        auto error = owner_.TransitionToOpened();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        if (environmentContext_)
        {
            owner_.EnvironmentManagerObj->AbortApplicationEnvironment(environmentContext_);
        }

        owner_.TransitionToFailed().ReadValue();
        TryComplete(thisSPtr, error);
    }

private:
    VersionedApplication & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationPackageDescription description_;
    ApplicationEnvironmentContextSPtr environmentContext_;
    vector<CertificateAccessDescription> certificateAccessDescriptions_;
};

// ********************************************************************************************************************
// VersionedApplication::SwitchAsyncOperation Implementation
//
class VersionedApplication::SwitchAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(SwitchAsyncOperation)

public:
    SwitchAsyncOperation(
        __in VersionedApplication & owner,
        ApplicationVersion const & newVersion,
        ApplicationPackageDescription const & newPackageDescription,        
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        currentVersion_(),
        currentPackageDescription_(),
        newVersion_(newVersion),
        newPackageDescription_(newPackageDescription)
    {
    }

    virtual ~SwitchAsyncOperation() 
    { 
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<SwitchAsyncOperation>(operation);
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

        //For Testing. This test config cancels the parent UpgradeApplication Package Operation 
        if (HostingConfig::GetConfig().AbortSwitchApplicationPackage)
        {
            thisSPtr->Parent->Cancel();
            return;
        }

        {
            AcquireReadLock lock(owner_.Lock);
            currentVersion_ = owner_.version__;
            currentPackageDescription_ = owner_.packageDescription__;
        }

        // check if version instances are same
        if (currentVersion_ == newVersion_)
        {
            TransitionToOpened(thisSPtr);
            return;
        } 

        if(currentPackageDescription_.ContentChecksum == newPackageDescription_.ContentChecksum)
        {
            DoUpdateVersion(thisSPtr);
        }
        else
        {
            Assert::CodingError("Currently switch should only be called when version needs to be updated");
        }
    }

    void DoUpdateVersion(AsyncOperationSPtr const & thisSPtr)
    {
        {
            // Update the version just on the VersionedApplication
            // The version on the ServicePackages are modified when the
            // ServicePackages are updated
            AcquireWriteLock lock(owner_.Lock);
            owner_.version__ = newVersion_;
            owner_.packageDescription__ = newPackageDescription_;
        }

        TransitionToOpened(thisSPtr);
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToOpened();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        vector<ServicePackage2SPtr> servicePackages = owner_.packageMap__->Close();
        if (servicePackages.size() == 0)
        {
            owner_.TransitionToFailed().ReadValue();
            TryComplete(thisSPtr, failedError);
            return;
        }
        else
        {
            atomic_uint64 pendingOperationCount(servicePackages.size());
            for(auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
            {
                auto servicePackage = *iter;
                servicePackage->AbortAndWaitForTerminationAsync(
                    [this, &pendingOperationCount, failedError](AsyncOperationSPtr const & operation)
                {
                    --pendingOperationCount;
                    if (pendingOperationCount.load() == 0)
                    {
                        owner_.TransitionToFailed().ReadValue();
                        TryComplete(operation->Parent, failedError);
                        return;
                    }
                },
                    thisSPtr);
            }
        }
    }

private:
    VersionedApplication & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationVersion currentVersion_;
    ApplicationVersion const newVersion_;
    ApplicationPackageDescription newPackageDescription_;    
    ApplicationPackageDescription currentPackageDescription_;        
};

// ********************************************************************************************************************
// VersionedApplication::ServicePackageInstancesAsyncOperationBase Implementation
//
class VersionedApplication::ServicePackageInstancesAsyncOperationBase
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ServicePackageInstancesAsyncOperationBase)

public:
    ServicePackageInstancesAsyncOperationBase(
        __in VersionedApplication & owner,
        vector<ServicePackageInstanceOperation> && packageInstanceOperations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        packageInstanceOperations_(move(packageInstanceOperations)),
        statusMap_(),
        pendingOperationCount_(0),
        lastError_(),
        lock_()
    {
        statusMap_ = make_shared<OperationStatusMap>();
        for (auto const & item : packageInstanceOperations_)
        {
            statusMap_->Initialize(item.OperationId);
        }
    }

    virtual ~ServicePackageInstancesAsyncOperationBase()
    {
    }

protected:
    virtual AsyncOperationSPtr BeginOperation(
        ServicePackageInstanceOperation const & packageInstanceOperation,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) = 0;

    virtual ErrorCode EndOperation(
        ServicePackageInstanceOperation const & packageInstanceOperation,
        AsyncOperationSPtr const & asyncOperation) = 0;

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToModifying_PackageActivationDeactivation();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        PerformServicePackageOperations(thisSPtr);
    }

private:
    void PerformServicePackageOperations(AsyncOperationSPtr const & thisSPtr)
    {
        pendingOperationCount_.store(packageInstanceOperations_.size());

        for (auto const & packageInstanceOperation : packageInstanceOperations_)
        {
            PerformServicePackageOperation(thisSPtr, packageInstanceOperation);
        }

        CheckPendingOperations(thisSPtr, packageInstanceOperations_.size());
    }

    void PerformServicePackageOperation(AsyncOperationSPtr const & thisSPtr, ServicePackageInstanceOperation const & packageInstanceOperation)
    {
        statusMap_->SetState(packageInstanceOperation.OperationId, OperationState::InProgress);

        auto timeout = timeoutHelper_.GetRemainingTime();
        auto asyncOperation = this->BeginOperation(
            packageInstanceOperation,
            timeout,
            [this, packageInstanceOperation](AsyncOperationSPtr const & asyncOperation)
            {
                this->FinishServicePackageInstanceOperation(packageInstanceOperation, asyncOperation, false);
            },
            thisSPtr);
        FinishServicePackageInstanceOperation(packageInstanceOperation, asyncOperation, true);
    }

    void FinishServicePackageInstanceOperation(
        ServicePackageInstanceOperation const & packageInstanceOperation,
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = this->EndOperation(packageInstanceOperation, asyncOperation);
        if (!error.IsSuccess())
        {
            {
                AcquireWriteLock lock(lock_);
                lastError_.Overwrite(error);
            }
        }

        statusMap_->Set(packageInstanceOperation.OperationId, error, OperationState::Completed);
        uint64 pendingOperationCount = --pendingOperationCount_;
        CheckPendingOperations(asyncOperation->Parent, pendingOperationCount);
    }

    void CheckPendingOperations(AsyncOperationSPtr const & thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            TransitionToOpened(thisSPtr);
        }
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToOpened();
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::OperationsPending))
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        ErrorCode lastError;
        {
            AcquireReadLock lock(lock_);
            lastError = lastError_;
        }

        TryComplete(thisSPtr, lastError);
        return;
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        vector<ServicePackage2SPtr> servicePackages = owner_.packageMap__->Close();
        if (servicePackages.size() == 0)
        {
            owner_.TransitionToFailed().ReadValue();
            TryComplete(thisSPtr, failedError);
            return;
        }
        else
        {
            pendingOperationCount_.store(servicePackages.size());
            for (auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
            {
                auto servicePackage = *iter;
                servicePackage->AbortAndWaitForTerminationAsync(
                    [this, failedError](AsyncOperationSPtr const & operation)
                {
                    --pendingOperationCount_;
                    if (pendingOperationCount_.load() == 0)
                    {
                        owner_.TransitionToFailed().ReadValue();
                        TryComplete(operation->Parent, failedError);
                        return;
                    }
                },
                    thisSPtr);
            }
        }
    }

protected:
    VersionedApplication & owner_;
    TimeoutHelper timeoutHelper_;
    OperationStatusMapSPtr statusMap_;
    ApplicationEnvironmentContextSPtr applicationEnvironment_;

private:
    vector<ServicePackageInstanceOperation> const packageInstanceOperations_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
    RwLock lock_;
};

// ********************************************************************************************************************
// VersionedApplication::ActivateServicePackageInstanceAsyncOperation Implementation
//
class VersionedApplication::ActivateServicePackageInstanceAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivateServicePackageInstanceAsyncOperation)

public:
    ActivateServicePackageInstanceAsyncOperation(
        __in VersionedApplication & owner,
        ServicePackageInstanceOperation const & packageInstanceOperation,
        bool ensureLatestVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeout_(timeout),
        packageInstanceOperation_(packageInstanceOperation),
        servicePackageInstance_(),
        servicePackageVersionInstance_(ServicePackageVersion(owner.Version, packageInstanceOperation.PackageRolloutVersion), packageInstanceOperation.VersionInstanceId),
        ensureLatestVersion_(ensureLatestVersion)
    {
    }

    virtual ~ActivateServicePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateServicePackageInstanceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = TryGetOrCreateServicePackageInstance();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        ConfigureSharedPackagePermissions(thisSPtr);
    }

private:
    void ConfigureSharedPackagePermissions(AsyncOperationSPtr const & thisSPtr)
    {
        vector<wstring> sharedFolders;
        auto error = servicePackageInstance_->GetSharedPackageFolders(owner_.packageDescription__.ApplicationTypeName, sharedFolders);
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
        if(!sharedFolders.empty())
        {
            auto operation = owner_.ApplicationObj.HostingHolder.RootedObject.FabricActivatorClientObj->BeginConfigureSharedFolderPermissions(
                sharedFolders,
                timeout_,
                [this](AsyncOperationSPtr const & operation) { this->OnResourcesConfigured(operation, false); },
                thisSPtr);
            OnResourcesConfigured(operation, true);
        }
        else
        {
            ActivateServicePackageInstance(thisSPtr);
        }
    }

    void OnResourcesConfigured(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.ApplicationObj.HostingHolder.RootedObject.FabricActivatorClientObj->EndConfigureSharedFolderPermissions(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "ConfigureSharedFolderACL: error={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, ErrorCode(error.ReadValue()));
            return;
        }
        else
        {
            ActivateServicePackageInstance(operation->Parent);
        }
    }

    void ActivateServicePackageInstance(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ActivateServicePackageInstance): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}, ServicePackageVersionInstance={4}, EnsureLatestVersion={5}, Timeout={6}",
            owner_.Id,
            owner_.Version,
            packageInstanceOperation_.ServicePackageName,
            packageInstanceOperation_.ActivationContext,
            servicePackageVersionInstance_,
            ensureLatestVersion_,
            timeout_);


        auto operation = servicePackageInstance_->BeginActivate(
            servicePackageVersionInstance_,
            ensureLatestVersion_,
            timeout_,
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

        auto error = servicePackageInstance_->EndActivate(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ActivateServicePackageInstance): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}, ServicePackageVersionInstance={4}, Error={5}",
            owner_.Id,
            owner_.Version,
            packageInstanceOperation_.ServicePackageName,
            packageInstanceOperation_.ActivationContext,
            servicePackageVersionInstance_,
            error);

        if (error.IsError(ErrorCodeValue::HostingServicePackageVersionMismatch))
        {
            bool hasReplica;
            owner_.ApplicationObj.HostingHolder.RootedObject.ApplicationManagerObj->ScheduleForDeactivationIfNotUsed(servicePackageInstance_->Id, hasReplica);
            TryComplete(operation->Parent, error);
            return;
        }

        bool shouldAbortServicePackage = false;
        if(error.IsSuccess())
        {
            error = owner_.ApplicationObj.HostingHolder.RootedObject.ApplicationManagerObj->EnsureServicePackageInstanceEntry(servicePackageInstance_->Id);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "ActivateServicePackageInstance: Error when calling EnsureServicePackageInstanceEntry. AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}, ServicePackageVersionInstance={4}, Error={5}.",
                    owner_.Id,
                    owner_.Version,
                    packageInstanceOperation_.ServicePackageName,
                    packageInstanceOperation_.ActivationContext,
                    servicePackageVersionInstance_,
                    error);
                shouldAbortServicePackage = true;
            }
        }
        else
        {
            auto state = servicePackageInstance_->GetState();
            shouldAbortServicePackage = (state == ServicePackage2::Failed);
        }

        if (shouldAbortServicePackage)
        {
            servicePackageInstance_->AbortAndWaitForTerminationAsync(
                [this, error](AsyncOperationSPtr const & abortOperation)
            {
                ServicePackage2SPtr removed;
                owner_.packageMap__->Remove(
                    servicePackageInstance_->ServicePackageName, 
                    servicePackageInstance_->Id.ActivationContext,
                    servicePackageInstance_->InstanceId,
                    removed).ReadValue();
                
                TryComplete(abortOperation->Parent, error);
                return;
            },
                operation->Parent);
            return;
        }
     
        TryComplete(operation->Parent, error);
    }

    ErrorCode TryGetOrCreateServicePackageInstance()
    {
        auto error = owner_.packageMap__->Find(packageInstanceOperation_.ServicePackageName, packageInstanceOperation_.ActivationContext, servicePackageInstance_);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            ServicePackageInstanceContext svcPkgIntanceCtx(
                packageInstanceOperation_.ServicePackageName,
                packageInstanceOperation_.ActivationContext,
                packageInstanceOperation_.PublicActivationId,
                owner_.packageDescription__.ApplicationName,
                owner_.Id,
                owner_.environmentContext__);

            servicePackageInstance_ = make_shared<ServicePackage2>(
                owner_.ApplicationObj.HostingHolder, 
                packageInstanceOperation_.PackageDescription,
                move(svcPkgIntanceCtx));

            return owner_.packageMap__->Add(servicePackageInstance_);
        }
        else
        {
            return error;
        }
    }
private:
    VersionedApplication & owner_;
    TimeSpan const timeout_;
    ServicePackageInstanceOperation const packageInstanceOperation_;
    ServicePackageVersionInstance servicePackageVersionInstance_;
    ServicePackage2SPtr servicePackageInstance_;
    bool ensureLatestVersion_;
};

// ********************************************************************************************************************
// VersionedApplication::UpgradeServicePackageAsyncOperation Implementation
//
class VersionedApplication::UpgradeServicePackageInstanceAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UpgradeServicePackageInstanceAsyncOperation)

public:
    UpgradeServicePackageInstanceAsyncOperation(
        __in VersionedApplication & owner,
        ServicePackage2SPtr const & servicePackageInstance,
        ServicePackageVersionInstance const & servicePackageVersionInstance,
        ServicePackageDescription const & packageDescription,
        UpgradeType::Enum upgradeType,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeout_(timeout),
        servicePackageInstance_(servicePackageInstance),
        servicePackageVersionInstance_(servicePackageVersionInstance),
        packageDescription_(packageDescription),
        upgradeType_(upgradeType)
    {
    }

    virtual ~UpgradeServicePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpgradeServicePackageInstanceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(UpgradeServicePackageInstance): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}, ServicePackageVersionInstance={4}, Timeout={5}.",
            owner_.Id,
            owner_.Version,
            servicePackageInstance_->ServicePackageName,
            servicePackageInstance_->Id.ActivationContext,
            servicePackageVersionInstance_,
            timeout_);

        auto operation = servicePackageInstance_->BeginUpgrade(
            packageDescription_,
            servicePackageVersionInstance_,
            upgradeType_,
            timeout_,
            [this](AsyncOperationSPtr const & operation){ this->FinishUpgradeServicePackageInstance(operation, false); },
            thisSPtr);

        this->FinishUpgradeServicePackageInstance(operation, true);
    }

private:
    void FinishUpgradeServicePackageInstance(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = servicePackageInstance_->EndUpgrade(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(UpgradeServicePackageInstance): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}, ServicePackageVersionInstance={4}, Error={5}.",
            owner_.Id,
            owner_.Version,
            servicePackageInstance_->ServicePackageName,
            servicePackageInstance_->Id.ActivationContext,
            servicePackageVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            if (servicePackageInstance_->GetState() == ServicePackage2::Failed)
            {
                // if the upgrade was not a success and the service package failed, abort it
                // if the service package is required RA will activate it after the upgrade through find
                servicePackageInstance_->AbortAndWaitForTerminationAsync(
                    [this, error](AsyncOperationSPtr const & abortOperation)
                    {
                        ServicePackage2SPtr removed;
                        owner_.packageMap__->Remove(
                            servicePackageInstance_->ServicePackageName,
                            servicePackageInstance_->Id.ActivationContext,
                            servicePackageInstance_->InstanceId,
                            removed).ReadValue();

                        TryComplete(abortOperation->Parent, error);
                        return;
                    },
                    operation->Parent);
            }
            else
            {
                TryComplete(operation->Parent, error);
                return;
            }
        }
        else
        {
            TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

private:
    VersionedApplication & owner_;
    TimeSpan timeout_;
    ServicePackage2SPtr servicePackageInstance_;
    ServicePackageVersionInstance servicePackageVersionInstance_;
    ServicePackageDescription packageDescription_;
    UpgradeType::Enum upgradeType_;
};

// ********************************************************************************************************************
// VersionedApplication::DeactivateServicePackageInstanceAsyncOperation Implementation
//
class VersionedApplication::DeactivateServicePackageInstanceAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DeactivateServicePackageInstanceAsyncOperation)

public:
    DeactivateServicePackageInstanceAsyncOperation(
        __in VersionedApplication & owner,
        ServicePackageInstanceOperation const & packageInstanceOperation,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeout_(timeout),
        packageInstanceOperation_(packageInstanceOperation),
        servicePackageInstance_()
    {
    }

    virtual ~DeactivateServicePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateServicePackageInstanceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.packageMap__->Find(packageInstanceOperation_.ServicePackageName, packageInstanceOperation_.ActivationContext, servicePackageInstance_);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            // package is not present and hence it is not active
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(DeactivateServicePackageInstance): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}, Timeout={4}",
            owner_.Id,
            owner_.Version,
            packageInstanceOperation_.ServicePackageName,
            packageInstanceOperation_.ActivationContext,
            timeout_);
        auto operation = servicePackageInstance_->BeginDeactivate(
            timeout_,
            [this](AsyncOperationSPtr const & operation){ this->FinishDeactivateServicePackage(operation, false); },
            thisSPtr);
        this->FinishDeactivateServicePackage(operation, true);
    }

private:
    void FinishDeactivateServicePackage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        servicePackageInstance_->EndDeactivate(operation);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(DeactivateServicePackageInstance): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}.",
            owner_.Id,
            owner_.Version,
            packageInstanceOperation_.ServicePackageName,
            packageInstanceOperation_.ActivationContext);

        ServicePackage2SPtr removed;
        owner_.packageMap__->Remove(
            servicePackageInstance_->ServicePackageName,
            servicePackageInstance_->Id.ActivationContext,
            servicePackageInstance_->InstanceId,
            removed).ReadValue();

        TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
        return;
    }

private:
    VersionedApplication & owner_;
    TimeSpan const timeout_;
    ServicePackageInstanceOperation const packageInstanceOperation_;
    ServicePackage2SPtr servicePackageInstance_;
};

// ********************************************************************************************************************
// VersionedApplication::ActivateServicePackageInstancesAsyncOperation Implementation
//
class VersionedApplication::ActivateServicePackageInstancesAsyncOperation
    : public VersionedApplication::ServicePackageInstancesAsyncOperationBase
{
    DENY_COPY(ActivateServicePackageInstancesAsyncOperation)

public:
    ActivateServicePackageInstancesAsyncOperation(
        __in VersionedApplication & owner,
        vector<ServicePackageInstanceOperation> && packageInstanceOperations,
        bool ensureLatestVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ServicePackageInstancesAsyncOperationBase(
        owner, 
        move(packageInstanceOperations),
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
        auto thisPtr = AsyncOperation::End<ActivateServicePackageInstancesAsyncOperation>(operation);
        statusMap = thisPtr->statusMap_;
        return thisPtr->Error;
    }

protected:
    virtual AsyncOperationSPtr BeginOperation(
        ServicePackageInstanceOperation const & packageInstanceOperation,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ActivateServicePackageInstanceAsyncOperation>(
            owner_,
            packageInstanceOperation,
            ensureLatestVersion_,
            timeout,
            callback,
            parent);
    }

    virtual ErrorCode EndOperation(
        ServicePackageInstanceOperation const & packageInstanceOperation,
        AsyncOperationSPtr const & asyncOperation)
    {
        UNREFERENCED_PARAMETER(packageInstanceOperation);
        return ActivateServicePackageInstanceAsyncOperation::End(asyncOperation);
    }

private:
    bool ensureLatestVersion_;
};

// ********************************************************************************************************************
// VersionedApplication::UpgradeServicePackagesAsyncOperation Implementation
//
class VersionedApplication::UpgradeServicePackagesAsyncOperation
    : public AsyncOperation
    , protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UpgradeServicePackagesAsyncOperation)

public:
    UpgradeServicePackagesAsyncOperation(
        __in VersionedApplication & owner,
        vector<ServicePackageOperation> && packageOperations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
        , packageOperations_(move(packageOperations))
        , servicePackageInstancesInfo_()
        , servicePackageInstanceCount_(0)
        , statusMap_()
        , pendingOperationCount_(0)
        , lastError_()
        , lock_()
    {
        statusMap_ = make_shared<OperationStatusMap>();
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out OperationStatusMapSPtr & statusMap)
    {
        auto thisPtr = AsyncOperation::End<UpgradeServicePackagesAsyncOperation>(operation);
        statusMap = thisPtr->statusMap_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        auto error = owner_.TransitionToModifying_PackageUpgrade();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        error = InitializeServicePackageInstancesToActivate();
        if (!error.IsSuccess())
        {
            TransitionToOpened(thisSPtr);
            return;
        }

        for (auto const & svcPkgInstanceInfo : servicePackageInstancesInfo_)
        {
            for (auto const & svcPkgInstance : svcPkgInstanceInfo.second)
            {
                statusMap_->Initialize(GetStatusMapKey(svcPkgInstance));

                ServicePackageVersionInstance svcPkgVerInstance(
                    ServicePackageVersion(owner_.Version, svcPkgInstanceInfo.first.PackageRolloutVersion),
                    svcPkgInstanceInfo.first.VersionInstanceId);

                UpgradeServicePackageInstance(svcPkgInstance, svcPkgVerInstance, svcPkgInstanceInfo.first, thisSPtr);
            }
        }
            
        CheckPendingOperations(thisSPtr, servicePackageInstanceCount_);
    }

private:
    ErrorCode InitializeServicePackageInstancesToActivate()
    {
        //
        // Get all ServicePackageIntance(s) that need to be activated from packageInstanceMap.
        //
        for (auto const & packageOp : packageOperations_)
        {
            vector<ServicePackage2SPtr> svcPkgInstances;
            auto error = owner_.packageMap__->GetInstancesOfServicePackage(packageOp.PackageName, svcPkgInstances);
            if (!error.IsSuccess())
            {
                {
                    AcquireWriteLock lock(lock_);
                    lastError_.Overwrite(error);
                }

                return error;
            }

            if (svcPkgInstances.empty())
            {
                // No ServicePackageInstance was found, after the upgrade ApplicationManager will activate the necessary packages
                WriteNoise(
                    TraceType,
                    owner_.TraceId,
                    "Begin(UpgradeServicePackage): No ServicePackageInstance was found, it will be activated by RA after Upgrade if needed through Find. AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageVersionInstance={3}:{4}",
                    owner_.Id,
                    owner_.Version,
                    packageOp.PackageName,
                    packageOp.PackageRolloutVersion,
                    packageOp.VersionInstanceId);

                continue;
            }

            servicePackageInstancesInfo_.push_back(make_pair(packageOp, svcPkgInstances));

            servicePackageInstanceCount_ += svcPkgInstances.size();

            WriteNoise(
                TraceType,
                owner_.TraceId,
                "Begin(UpgradeServicePackage): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageVersionInstance={3}:{4}, ServicePackageInstaces={5}.",
                owner_.Id,
                owner_.Version,
                packageOp.PackageName,
                packageOp.PackageRolloutVersion,
                packageOp.VersionInstanceId,
                svcPkgInstances.size());
        }

        pendingOperationCount_.store(servicePackageInstanceCount_);

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "UpgradeServicePackagesAsyncOperation: ServicePackageInstances Count: {0}",
            servicePackageInstanceCount_);

        return ErrorCode(ErrorCodeValue::Success);
    }

    void UpgradeServicePackageInstance(
        ServicePackage2SPtr const & servicePackageInstance,
        ServicePackageVersionInstance const & servicePackageVersionInstance,
        ServicePackageOperation const & packageOperation,
        AsyncOperationSPtr const & thisSPtr)
    {
        statusMap_->SetState(GetStatusMapKey(servicePackageInstance), OperationState::InProgress);

        auto operation = AsyncOperation::CreateAndStart<UpgradeServicePackageInstanceAsyncOperation>(
            owner_,
            servicePackageInstance,
            servicePackageVersionInstance,
            packageOperation.PackageDescription,
            packageOperation.UpgradeType,
            timeoutHelper_.GetRemainingTime(),
            [this, servicePackageInstance](AsyncOperationSPtr const & asyncOperation)
            {
                this->FinishUpgradeServicePackageInstance(servicePackageInstance, asyncOperation, false);
            },
            thisSPtr);

        this->FinishUpgradeServicePackageInstance(servicePackageInstance, operation, true);
    }

    void FinishUpgradeServicePackageInstance(
        ServicePackage2SPtr const & servicePackageInstance,
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = UpgradeServicePackageInstanceAsyncOperation::End(asyncOperation);
        if (!error.IsSuccess())
        {
            {
                AcquireWriteLock lock(lock_);
                lastError_.Overwrite(error);
            }
        }

        statusMap_->Set(GetStatusMapKey(servicePackageInstance), error, OperationState::Completed);
        
        uint64 pendingOperationCount = --pendingOperationCount_;
        CheckPendingOperations(asyncOperation->Parent, pendingOperationCount);
    }

    void CheckPendingOperations(AsyncOperationSPtr const & thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            TransitionToOpened(thisSPtr);
        }
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToOpened();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        ErrorCode lastError;
        {
            AcquireReadLock lock(lock_);
            lastError = lastError_;
        }

        TryComplete(thisSPtr, lastError);
        return;
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        vector<ServicePackage2SPtr> servicePackageInstances = owner_.packageMap__->Close();
        if (servicePackageInstances.size() == 0)
        {
            owner_.TransitionToFailed().ReadValue();
            TryComplete(thisSPtr, failedError);
            return;
        }
        else
        {
            pendingOperationCount_.store(servicePackageInstances.size());
            
            for (auto const & svcPkgInstance : servicePackageInstances)
            {
                svcPkgInstance->AbortAndWaitForTerminationAsync(
                    [this, failedError](AsyncOperationSPtr const & operation)
                    {
                        if (--pendingOperationCount_ == 0)
                        {
                            owner_.TransitionToFailed().ReadValue();
                            TryComplete(operation->Parent, failedError);
                            return;
                        }
                    },
                    thisSPtr);
            }
        }
    }

private:
    VersionedApplication & owner_;
    TimeoutHelper timeoutHelper_;
    OperationStatusMapSPtr statusMap_;
    vector<pair<ServicePackageOperation, vector<ServicePackage2SPtr>>> servicePackageInstancesInfo_;
    size_t servicePackageInstanceCount_;
    vector<ServicePackageOperation> packageOperations_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
    RwLock lock_;
};

// ********************************************************************************************************************
// VersionedApplication::DeactivateServicePackageInstancesAsyncOperation Implementation
//
class VersionedApplication::DeactivateServicePackageInstancesAsyncOperation
    : public VersionedApplication::ServicePackageInstancesAsyncOperationBase
{
    DENY_COPY(DeactivateServicePackageInstancesAsyncOperation)

public:
    DeactivateServicePackageInstancesAsyncOperation(
        __in VersionedApplication & owner,
        vector<ServicePackageInstanceOperation> && packageInstanceOperations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ServicePackageInstancesAsyncOperationBase(
        owner, 
        move(packageInstanceOperations),
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
        auto thisPtr = AsyncOperation::End<DeactivateServicePackageInstancesAsyncOperation>(operation);
        statusMap = thisPtr->statusMap_;
        return thisPtr->Error;
    }

protected:
    virtual AsyncOperationSPtr BeginOperation(
        ServicePackageInstanceOperation const & packageInstanceOperation,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<DeactivateServicePackageInstanceAsyncOperation>(
            owner_,
            packageInstanceOperation,
            timeout,
            callback,
            parent);
    }

    virtual ErrorCode EndOperation(
        ServicePackageInstanceOperation const & packageInstanceOperation,
        AsyncOperationSPtr const & asyncOperation)
    {
        UNREFERENCED_PARAMETER(packageInstanceOperation);
        return DeactivateServicePackageInstanceAsyncOperation::End(asyncOperation);
    }
};

// ********************************************************************************************************************
// VersionedApplication::CloseAsyncOperation Implementation
//
class VersionedApplication::CloseAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in VersionedApplication & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        packageCount_(0),
        environmentContext_()
    {
    }

    virtual ~CloseAsyncOperation() 
    { 
    }

    static void End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.GetState() == VersionedApplication::Closed)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        auto error = owner_.TransitionToClosing();
        if (!error.IsSuccess())
        {
            TransitionToAborted(thisSPtr);
            return;
        }

        DeactivateServicePackages(thisSPtr);
    }

private:
    void DeactivateServicePackages(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ServicePackage2SPtr> servicePackages = owner_.packageMap__->Close();
        packageCount_.store(servicePackages.size());

        for(auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
        {
            auto servicePackage = (*iter);
            auto timeout = timeoutHelper_.GetRemainingTime();
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "Begin(DeactivateServicePackageInstance): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}, Timeout={4}",
                owner_.Id,
                owner_.Version,
                servicePackage->ServicePackageName,
                servicePackage->Id.ActivationContext,
                timeout);

            auto operation = servicePackage->BeginDeactivate(
                timeout,
                [this, servicePackage](AsyncOperationSPtr const & operation) { this->FinishDeactivateServicePackage(servicePackage, operation, false); },
                thisSPtr);
            FinishDeactivateServicePackage(servicePackage, operation, true);
        }

        CheckPendingDeactivations(thisSPtr, servicePackages.size());
    }

    void FinishDeactivateServicePackage(
        ServicePackage2SPtr const & servicePackage, 
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        servicePackage->EndDeactivate(operation);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(DeactivateServicePackageInstance): AppId={0}, AppVersion={1} ServicePackageName={2}, ServicePackageActivationContext={3}",
            owner_.Id,
            owner_.Version,
            servicePackage->ServicePackageName,
            servicePackage->Id.ActivationContext);

        uint64 currentPackageCount = --packageCount_;
        CheckPendingDeactivations(operation->Parent, currentPackageCount);
    }

    void CheckPendingDeactivations(AsyncOperationSPtr const & thisSPtr, uint64 currentPackageCount)
    {
        if (currentPackageCount == 0)
        {
            CleanupApplicationEnvironment(thisSPtr);
        }
    }

    void CleanupApplicationEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        {
            AcquireWriteLock lock(owner_.Lock);
            environmentContext_ = move(owner_.environmentContext__);
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CleanupApplicationEnvironment): Id={0}, Version={1}, Timeout={2}",
            owner_.Id,
            owner_.Version,
            timeout);
        auto operation = owner_.EnvironmentManagerObj->BeginCleanupApplicationEnvironment(
            environmentContext_,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishCleanupApplicationEnvironment(operation, false); },
            thisSPtr);
        FinishCleanupApplicationEnvironment(operation, true);
    }

    void FinishCleanupApplicationEnvironment(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EnvironmentManagerObj->EndCleanupApplicationEnvironment(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(CleanupApplicationEnvironment): Id={0}, Version={1}, ErrorCode={2}",
            owner_.Id,
            owner_.Version,
            error);
        if (!error.IsSuccess())
        {
            owner_.EnvironmentManagerObj->AbortApplicationEnvironment(environmentContext_);
            TransitionToFailed(operation->Parent);
        }
        else
        {
            TransitionToClosed(operation->Parent);
        }
    }

    void TransitionToClosed(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToClosed();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr);
        }
        else
        {
            owner_.ApplicationObj.HostingHolder.RootedObject.HealthManagerObj->UnregisterSource(owner_.ApplicationObj.Id, owner_.healthPropertyId_);

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
    }

private:
    VersionedApplication & owner_;
    TimeoutHelper timeoutHelper_;
    atomic_uint64 packageCount_;
    ApplicationEnvironmentContextSPtr environmentContext_;
};

// ********************************************************************************************************************
// VersionedApplication Implementation
//
VersionedApplication::VersionedApplication(
    Application2Holder const & applicationHolder,
    ApplicationVersion const & version,
    ApplicationPackageDescription const & appPackageDescription)
    : ComponentRoot(),
    StateMachine(Created),
    applicationHolder_(applicationHolder),
    healthPropertyId_(*ServiceModel::Constants::HealthActivationProperty),
    version__(version),
    packageDescription__(appPackageDescription),
    packageMap__(),
    environmentContext__()
{
    this->SetTraceId(applicationHolder_.Root->TraceId);
    WriteNoise(
        TraceType, TraceId, 
        "VersionedApplication.constructor: {0} ({1}:{2})", 
        static_cast<void*>(this),
        applicationHolder_.RootedObject.Id,
        applicationHolder_.RootedObject.InstanceId);

    auto packageMap = make_unique<ServicePackage2Map>();
    packageMap__ = move(packageMap);
}

VersionedApplication::~VersionedApplication()
{
    WriteNoise(
        TraceType, TraceId, 
        "VersionedApplication.destructor: {0} ({1}:{2})", 
        static_cast<void*>(this),
        applicationHolder_.RootedObject.Id,
        applicationHolder_.RootedObject.InstanceId);
}

AsyncOperationSPtr VersionedApplication::BeginOpen(    
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode VersionedApplication::EndOpen(AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr VersionedApplication::BeginSwitch(
    ServiceModel::ApplicationVersion const & newVersion,
    ServiceModel::ApplicationPackageDescription newPackageDescription,    
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SwitchAsyncOperation>(
        *this,
        newVersion,
        newPackageDescription,        
        timeout,
        callback,
        parent);
}

ErrorCode VersionedApplication::EndSwitch(AsyncOperationSPtr const & operation)
{
    return SwitchAsyncOperation::End(operation);
}

AsyncOperationSPtr VersionedApplication::BeginActivateServicePackageInstances(
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

ErrorCode VersionedApplication::EndActivateServicePackages(
    AsyncOperationSPtr const & operation,
    __out OperationStatusMapSPtr & statusMap)
{
    return ActivateServicePackageInstancesAsyncOperation::End(operation, statusMap);
}

AsyncOperationSPtr VersionedApplication::BeginDeactivateServicePackageInstances(
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

ErrorCode VersionedApplication::EndDeactivateServicePackages(
    AsyncOperationSPtr const & operation,
    __out OperationStatusMapSPtr & statusMap)
{

    return DeactivateServicePackageInstancesAsyncOperation::End(operation, statusMap);
}

AsyncOperationSPtr VersionedApplication::BeginUpgradeServicePackages(
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

ErrorCode VersionedApplication::EndUpgradeServicePackages(
    AsyncOperationSPtr const & operation,
    __out OperationStatusMapSPtr & statusMap)
{
    return UpgradeServicePackagesAsyncOperation::End(operation, statusMap);
}

AsyncOperationSPtr VersionedApplication::BeginClose(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

void VersionedApplication::EndClose(AsyncOperationSPtr const & operation)
{
    CloseAsyncOperation::End(operation);
}

void VersionedApplication::OnAbort()
{
    vector<ServicePackage2SPtr> removed = this->packageMap__->Close();
    for(auto iter = removed.begin(); iter != removed.end(); ++iter)
    {
        (*iter)->AbortAndWaitForTermination();
    }

    if (environmentContext__)
    {
        EnvironmentManagerObj->AbortApplicationEnvironment(environmentContext__);
    }

    this->ApplicationObj.HostingHolder.RootedObject.HealthManagerObj->UnregisterSource(ApplicationObj.Id, healthPropertyId_);
}

Application2 const & VersionedApplication::get_Application() const 
{ 
    return this->applicationHolder_.RootedObject; 
}

ApplicationIdentifier const & VersionedApplication::get_Id() const 
{ 
    return this->ApplicationObj.Id; 
}

ApplicationVersion const & VersionedApplication::get_Version() const
{
    {
        AcquireReadLock lock(this->Lock);
        return this->version__;
    }
}

EnvironmentManagerUPtr const & VersionedApplication::get_EnvironmentManagerObj() const
{
    return this->ApplicationObj.HostingHolder.RootedObject.ApplicationManagerObj->EnvironmentManagerObj; 
}

ErrorCode VersionedApplication::AnalyzeApplicationUpgrade(
    ApplicationUpgradeSpecification const & appUpgradeSpec,
    __out set<wstring, IsLessCaseInsensitiveComparer<wstring>> & affectedRuntimeIds)
{
    auto error = this->TransitionToAnalyzing();
    if(!error.IsSuccess())
    {
        return error;
    }

    ApplicationPackageDescription newPackageDescription;
    error = this->applicationHolder_.Value.LoadApplicationPackageDescription(this->Id, appUpgradeSpec.AppVersion, newPackageDescription);
    if(!error.IsSuccess())
    {
        this->TransitionToOpened().ReadValue();
        return error;
    }

    ApplicationVersion currentAppVersion;
    ApplicationPackageDescription currentPackageDescription;    
    {
        AcquireReadLock lock(this->Lock);
        currentAppVersion = this->version__;
        currentPackageDescription = this->packageDescription__;
    }

    if(currentAppVersion == appUpgradeSpec.AppVersion || 
        currentPackageDescription.ContentChecksum == newPackageDescription.ContentChecksum)
    {
        // If Application is not going to restart, only some ServicePackages will be affected
        for(auto const & pkgUpgradeSpec : appUpgradeSpec.PackageUpgrades)
        {
            vector<ServicePackage2SPtr> servicePackageInstances;
            error = this->packageMap__->GetInstancesOfServicePackage(pkgUpgradeSpec.ServicePackageName, servicePackageInstances);
            if (!error.IsSuccess())
            { 
                break; 
            }

            ServicePackageVersionInstance servicePackageVersionInstance(
                ServicePackageVersion(appUpgradeSpec.AppVersion, pkgUpgradeSpec.RolloutVersionValue),
                appUpgradeSpec.InstanceId);

            for (auto const & pkgInstance : servicePackageInstances)
            {
                error = pkgInstance->AnalyzeApplicationUpgrade(servicePackageVersionInstance, appUpgradeSpec.UpgradeType, affectedRuntimeIds);
                if (!error.IsSuccess())
                {
                    break;
                }
            }
        }
    }
    else
    {
        // If Application is going to restart, then all current ServicePackages will be affected

        vector<ServicePackage2SPtr> servicePackageInstances;
        error = this->packageMap__->GetAllServicePackageInstances(servicePackageInstances);
        
        if (error.IsSuccess())
        {
            for (auto const & pkgInstance : servicePackageInstances)
            {
                error = this->applicationHolder_.Value.HostingHolder.Value.FabricRuntimeManagerObj->GetRuntimeIds(pkgInstance->Id, affectedRuntimeIds);
                if (!error.IsSuccess()) { break; }
            }
        }
    }

    this->TransitionToOpened().ReadValue();
    return error;
}

void VersionedApplication::OnServiceTypesUnregistered(
    ServicePackageInstanceIdentifier const servicePackageInstanceId,
    vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
{
    ServicePackage2SPtr servicePackage;
    auto error = this->packageMap__->Find(servicePackageInstanceId, servicePackage);

    if (error.IsSuccess())
    {
        servicePackage->OnServiceTypesUnregistered(serviceTypeInstanceIds);
    }
}

ErrorCode VersionedApplication::GetServicePackageInstance(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    __out ServicePackage2SPtr & servicePackageInstance) const
{
    return this->packageMap__->Find(servicePackageInstanceId, servicePackageInstance);
}

ErrorCode VersionedApplication::GetServicePackageInstance(
    wstring const & servicePackageName,
    wstring const & servicePackagePublicActivationId,
    __out ServicePackage2SPtr & servicePackageInstance) const
{
    return this->packageMap__->Find(servicePackageName, servicePackagePublicActivationId, servicePackageInstance);
}

ErrorCode VersionedApplication::GetAllServicePackageInstances(__out vector<ServicePackage2SPtr> & servicePackageInstance) const
{
    return this->packageMap__->GetAllServicePackageInstances(servicePackageInstance);
}

Common::ErrorCode VersionedApplication::GetInstancesOfServicePackage(
    wstring const & servicePackageName, 
    __out vector<ServicePackage2SPtr> & servicePackageInstances) const
{
    ASSERT_IF(servicePackageName.empty(), "VersionedApplication::GetInstancesOfServicePackage(): servicePackageName is empty.")
    
    return this->packageMap__->GetInstancesOfServicePackage(servicePackageName, servicePackageInstances);
}

Common::ErrorCode VersionedApplication::GetServicePackageInstance(
    wstring const & servicePackageName,
    ServicePackageVersion const & version,
    __out ServicePackage2SPtr & servicePackageInstance) const
{
    return this->packageMap__->Find(servicePackageName, version, servicePackageInstance);
}

void VersionedApplication::OnServiceTypeRegistrationNotFound(
    uint64 const registrationTableVersion,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    ServicePackage2SPtr servicePackage;
    auto error = this->packageMap__->Find(servicePackageInstanceId, servicePackage);

    if ((error.IsSuccess()) && 
        (servicePackage->CurrentVersionInstance.Version.RolloutVersionValue == versionedServiceTypeId.servicePackageVersionInstance.Version.RolloutVersionValue))
    {
        servicePackage->OnServiceTypeRegistrationNotFound(registrationTableVersion, versionedServiceTypeId, servicePackageInstanceId);
    }
}

void VersionedApplication::NotifyDcaAboutServicePackages()
{
    vector<ServicePackage2SPtr> servicePackages;
    auto error = GetAllServicePackageInstances(servicePackages);
    if (!error.IsSuccess())
    {
        return;
    }

    for (auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
    {
        (*iter)->NotifyDcaAboutServicePackages();
    }
}

Common::ErrorCode VersionedApplication::IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent) const
{
    vector<ServicePackage2SPtr> servicePackages;
    auto error = GetAllServicePackageInstances(servicePackages);
    if(!error.IsSuccess())
    {
        return error;
    }

    ErrorCode lastError(ErrorCodeValue::Success);
    for(auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
    {
        lastError = (*iter)->IsCodePackageLockFilePresent(isCodePackageLockFilePresent);
        if(!lastError.IsSuccess())
        {
            WriteWarning(
                TraceType, 
                TraceId,
                "IsCodePackageLockFilePresent for {0} failed with {1}",
                (*iter)->Id,
                lastError);
        }
        if(isCodePackageLockFilePresent)
        {
            return ErrorCodeValue::Success;
        }
    }

    return lastError;
}

wstring VersionedApplication::GetStatusMapKey(ServicePackage2SPtr const & servicePackageInstance)
{
    return wformatString("{0}_{1}", servicePackageInstance->ServicePackageName, servicePackageInstance->Id.ActivationContext);
}
