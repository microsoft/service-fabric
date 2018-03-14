// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("VersionedServicePackage");

// ********************************************************************************************************************
// VersionedServicePackage::CodePackagesAsyncOperationBase Implementation
//
class VersionedServicePackage::CodePackagesAsyncOperationBase
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CodePackagesAsyncOperationBase)

public:
    CodePackagesAsyncOperationBase(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        codePackages_(codePackages),
        failed_(codePackages),
        completed_(),
        lock_(),
        lastError_(ErrorCodeValue::Success),
        pendingOperationCount_(0)
    {
    }

    virtual ~CodePackagesAsyncOperationBase()
    {
    }

protected:
    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        auto thisPtr = AsyncOperation::End<CodePackagesAsyncOperationBase>(operation);

        {
            AcquireWriteLock lock(thisPtr->lock_);
            failed = move(thisPtr->failed_);
            completed = move(thisPtr->completed_);
        }

        return thisPtr->Error;
    }

protected:
    virtual AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) = 0;

    virtual ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation) = 0;

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        pendingOperationCount_.store(codePackages_.size());

        for (auto iter = codePackages_.begin(); iter != codePackages_.end(); ++iter)
        {
            auto codePackage = (*iter).second;
            auto timeout = timeoutHelper_.GetRemainingTime();

            auto operation = BeginCodePackageOperation(
                codePackage,
                timeout,
                [this, codePackage](AsyncOperationSPtr const & operation)
            {
                this->FinishCodePackageOperation(codePackage, operation, false);
            },
                thisSPtr);
            FinishCodePackageOperation(codePackage, operation, true);
        }

        CheckPendingOperations(thisSPtr, codePackages_.size());
    }

    void FinishCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = this->EndCodePackageOperation(codePackage, operation);
        {
            AcquireWriteLock lock(lock_);

            if (!error.IsSuccess())
            {
                lastError_.Overwrite(error);
            }
            else
            {
                auto iter = failed_.find(codePackage->Description.Name);

                ASSERT_IF(
                    iter == failed_.end(),
                    "CodePackage {0} must be present in the failed map as failed map is initialized with all code packages.",
                    codePackage->Description.Name);

                failed_.erase(iter);
                completed_.insert(make_pair(codePackage->Description.Name, codePackage));
            }
        }

        uint64 pendingOperationCount = --pendingOperationCount_;
        CheckPendingOperations(operation->Parent, pendingOperationCount);
    }

    void CheckPendingOperations(AsyncOperationSPtr const & thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            auto lastError = ErrorCode(ErrorCodeValue::Success);
            {
                AcquireReadLock lock(lock_);
                lastError = lastError_;
                lastError_.ReadValue();
            }

            TryComplete(thisSPtr, lastError);
            return;
        }
    }

protected:
    VersionedServicePackage & owner_;
    RwLock lock_;

private:
    CodePackageMap const codePackages_;
    TimeoutHelper timeoutHelper_;
    CodePackageMap failed_;
    CodePackageMap completed_;
    ErrorCode lastError_;
    atomic_uint64 pendingOperationCount_;
};

// ********************************************************************************************************************
// VersionedServicePackage::ActivateCodePackageAsyncOperation Implementation
//
class VersionedServicePackage::ActivateCodePackagesAsyncOperation
    : public VersionedServicePackage::CodePackagesAsyncOperationBase
{
    DENY_COPY(ActivateCodePackagesAsyncOperation)

public:
    ActivateCodePackagesAsyncOperation(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : CodePackagesAsyncOperationBase(owner, codePackages, timeout, callback, parent)
    {
    }

    virtual ~ActivateCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        return CodePackagesAsyncOperationBase::End(operation, failed, completed);
    }

protected:
    AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ActivateCodePackage): CodePackage={0}:{1} Version={2} Timeout={3}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            timeout);
        return codePackage->BeginActivate(timeout, callback, parent);
    }

    ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation)
    {
        auto error = codePackage->EndActivate(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ActivateCodePackage): CodePackage={0}:{1} Version={2} Error={3}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            error);
        return error;
    }
};

// ********************************************************************************************************************
// VersionedServicePackage::UpdateCodePackagesAsyncOperation Implementation
//
class VersionedServicePackage::UpdateCodePackagesAsyncOperation
    : public VersionedServicePackage::CodePackagesAsyncOperationBase
{
    DENY_COPY(UpdateCodePackagesAsyncOperation)

public:
    UpdateCodePackagesAsyncOperation(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        ServicePackageDescription const & newPackageDescription,
        ServicePackageVersionInstance const & newVersionInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : CodePackagesAsyncOperationBase(owner, codePackages, timeout, callback, parent),
        newPackageDescription_(newPackageDescription),
        newVersionInstance_(newVersionInstance)
    {
    }

    virtual ~UpdateCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        return CodePackagesAsyncOperationBase::End(operation, failed, completed);
    }

protected:
    AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(UpdateCodePackageContext): CodePackage={0}:{1} Version={2}, NewServicePackageVersionInstance={3}, Timeout={4}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            newVersionInstance_,
            timeout);

        // Computing the new RolloutVersion of the CodePackaget to Update.
        // During rollback, it is possible that the RolloutVersion is changed, but the actual content hasn't changed.
        // We do not restart the CodePackage but just update the CodePackate with the new CodePackage and send an update
        // to all Hosts
        RolloutVersion newRolloutVersion = RolloutVersion::Invalid;
        for (auto iter = newPackageDescription_.DigestedCodePackages.begin(); iter != newPackageDescription_.DigestedCodePackages.end(); ++iter)
        {
            if (iter->CodePackage == codePackage->Description.CodePackage)
            {
                newRolloutVersion = iter->RolloutVersionValue;
            }
        }

        if (!StringUtility::AreEqualCaseInsensitive(codePackage->Description.CodePackage.Name, (wstring)Constants::ImplicitTypeHostCodePackageName))
        {
            ASSERT_IF(newRolloutVersion == RolloutVersion::Invalid, "The CodePackage that needs to be updated should be present in the NewPackageDescription. CodePackage={0}", codePackage->Description);
        }

        return codePackage->BeginUpdateContext(newRolloutVersion, newVersionInstance_, timeout, callback, parent);
    }

    ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation)
    {
        auto error = codePackage->EndUpdateContext(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(UpdateCodePackageContext): CodePackage={0}:{1} Version={2}, NewServicePackageVersionInstance={3}, Error={4}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            newVersionInstance_,
            error);
        return error;
    }

private:
    ServicePackageVersionInstance newVersionInstance_;
    ServicePackageDescription newPackageDescription_;
};

// ********************************************************************************************************************
// VersionedServicePackage::DeactivateCodePackagesAsyncOperation Implementation
//
class VersionedServicePackage::DeactivateCodePackagesAsyncOperation
    : public VersionedServicePackage::CodePackagesAsyncOperationBase
{
    DENY_COPY(DeactivateCodePackagesAsyncOperation)

public:
    DeactivateCodePackagesAsyncOperation(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : CodePackagesAsyncOperationBase(owner, codePackages, timeout, callback, parent)
    {
    }

    virtual ~DeactivateCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        return CodePackagesAsyncOperationBase::End(operation, failed, completed);
    }

protected:
    AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(DectivateCodePackage): CodePackage={0}:{1} Version={2} Timeout={3}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            timeout);
        return codePackage->BeginDeactivate(timeout, callback, parent);
    }

    ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation)
    {
        codePackage->EndDeactivate(operation);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(DectivateCodePackage): CodePackage={0}:{1} Version={2}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version);
        return ErrorCode(ErrorCodeValue::Success);
    }
};

// ********************************************************************************************************************
// VersionedServicePackage::AbortCodePackagesAsyncOperation Implementation
//
class VersionedServicePackage::AbortCodePackagesAsyncOperation
    : public VersionedServicePackage::CodePackagesAsyncOperationBase
{
    DENY_COPY(AbortCodePackagesAsyncOperation)

public:
    AbortCodePackagesAsyncOperation(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : CodePackagesAsyncOperationBase(owner, codePackages, timeout, callback, parent)
    {
    }

    virtual ~AbortCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        return CodePackagesAsyncOperationBase::End(operation, failed, completed);
    }

protected:
    AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {

        UNREFERENCED_PARAMETER(timeout);

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(AbortAndWaitForTermination): CodePackage={0}:{1} Version={2}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version);
        codePackage->Abort();
        return codePackage->BeginAbortAndWaitForTermination(callback, parent);
    }

    ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation)
    {
        codePackage->EndAbortAndWaitForTermination(operation);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(AbortAndWaitForTermination): CodePackage={0}:{1} Version={2}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version);
        return ErrorCode(ErrorCodeValue::Success);
    }
};

// ********************************************************************************************************************
// VersionedServicePackage::OpenAsyncOperation Implementation
//
class VersionedServicePackage::OpenAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in VersionedServicePackage & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        packageVersionInstance_(),
        packageDescription_(),
        packageEnvironment_(),
        codePackages_()
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
        if (owner_.GetState() == VersionedServicePackage::Opened)
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
            packageVersionInstance_ = owner_.currentVersionInstance__;
            packageDescription_ = owner_.packageDescription__;
        }

        RegisterToLocalResourceManager(thisSPtr);
    }

private:
    void RegisterToLocalResourceManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ServicePackageObj.HostingHolder.RootedObject.LocalResourceManagerObj->RegisterServicePackage(owner_);
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        RegisterToHealthManager(thisSPtr);
    }

    void RegisterToHealthManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ServicePackageObj.HostingHolder.RootedObject.HealthManagerObj->RegisterSource(
            owner_.ServicePackageObj.Id,
            owner_.ServicePackageObj.ApplicationName,
            owner_.healthPropertyId_);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        SetupEnvironment(thisSPtr);
    }

    void SetupEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(SetupPackageEnvironment): Id={0}, Version={1}, Timeout={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            timeout);
        auto operation = owner_.EnvironmentManagerObj->BeginSetupServicePackageInstanceEnvironment(
            owner_.ServicePackageObj.ApplicationEnvironment,
            owner_.ServicePackageObj.Id,
            owner_.ServicePackageObj.InstanceId,
            packageDescription_,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishSetupEnvironment(operation, false); },
            thisSPtr);
        FinishSetupEnvironment(operation, true);
    }

    void FinishSetupEnvironment(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.EnvironmentManagerObj->EndSetupServicePackageInstanceEnvironment(operation, packageEnvironment_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(SetupPackageEnvironment): Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(operation->Parent, error);
            return;
        }

        // If the ServicePackage being activated is FileStoreService, then setup staging and store shares
        if (owner_.ServicePackageObj.Id.ApplicationId.IsSystem() &&
            owner_.ServicePackageObj.Id.ServicePackageName == L"FileStoreService")
        {
            SetupSMBSharesForFileStoreService(operation->Parent);
        }
        else if (owner_.ServicePackageObj.Id.ApplicationId.IsSystem() &&
            StringUtility::AreEqualCaseInsensitive(owner_.ServicePackageObj.Id.ServicePackageName,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
        {
            ConfigureNodeForDnsService(operation->Parent);
        }
        else
        {
            WriteCurrentPackage(operation->Parent);
        }
    }

    void SetupSMBSharesForFileStoreService(AsyncOperationSPtr const & thisSPtr)
    {
        vector<wstring> sidStrings;
        if (Management::FileStoreService::Utility::HasAccountsConfigured())
        {
            auto groupName = AccountHelper::GetAccountName(
                Management::FileStoreService::Constants::FileStoreServiceUserGroup,
                owner_.ServicePackageObj.Id.ApplicationId.ApplicationNumber);

            SidSPtr groupSid;
            auto error = BufferedSid::CreateSPtr(groupName, groupSid);
            ASSERT_IF(error.IsError(ErrorCodeValue::NotFound), "Expected local group name {0} not found", groupName);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Failed to create Sid for FileStoreService group with {0}. Id={1}, Version={2}",
                    error,
                    owner_.ServicePackageObj.Id,
                    packageVersionInstance_);

                TransitionToFailed(thisSPtr, error);
                return;
            }

            wstring sidStr;
            error = groupSid->ToString(sidStr);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Failed to convert Sid to String for FileStoreService group with {0}. Id={1}, Version={2}",
                    error,
                    owner_.ServicePackageObj.Id,
                    packageVersionInstance_);
                TransitionToFailed(thisSPtr, error);
                return;
            }

            sidStrings.push_back(sidStr);
        }
        else
        {

            // If no account is provided in FileStoreService section,
            // then by default ACL shares to current user of fabric
            sidStrings.push_back(owner_.EnvironmentManagerObj->CurrentUserSid);
        }

        if (Management::FileStoreService::FileStoreServiceConfig::GetConfig().AnonymousAccessEnabled)
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Anonymous access has been enabled for FileStoreService. Id={0}, Version={1}",
                owner_.ServicePackageObj.Id,
                packageVersionInstance_);

            SidSPtr anonymousSid, everyoneSid;
            auto error = BufferedSid::CreateSPtr(WELL_KNOWN_SID_TYPE::WinAnonymousSid, anonymousSid);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }

            error = BufferedSid::CreateSPtr(WELL_KNOWN_SID_TYPE::WinWorldSid, everyoneSid);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }

            wstring anonymousSidStr, everyoneSidStr;
            error = anonymousSid->ToString(anonymousSidStr);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }

            error = everyoneSid->ToString(everyoneSidStr);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }

            sidStrings.push_back(anonymousSidStr);
            sidStrings.push_back(everyoneSidStr);
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ConfigureSMBShare for Store): Id={0}, Version={1}, Timeout={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            timeoutHelper_.GetRemainingTime());

        wstring storeLocalFullPath = Path::Combine(
            owner_.ServicePackageObj.runLayout_.GetApplicationWorkFolder(owner_.ServicePackageObj.Id.ApplicationId.ToString()),
            Management::FileStoreService::Constants::StoreRootDirectoryName);
        wstring storeShareName = wformatString("{0}_{1}", Management::FileStoreService::Constants::StoreShareName, owner_.ServicePackageObj.HostingHolder.Value.NodeName);
        auto operation = owner_.ServicePackageObj.HostingHolder.Value.FabricActivatorClientObj->BeginConfigureSMBShare(
            sidStrings,
            GENERIC_READ | GENERIC_EXECUTE,
            storeLocalFullPath,
            storeShareName,
            timeoutHelper_.GetRemainingTime(),
            [this, sidStrings](AsyncOperationSPtr const & operation) { this->FinishConfigureStoreShare(operation, false, sidStrings); },
            thisSPtr);
        FinishConfigureStoreShare(operation, true, sidStrings);
    }

    void FinishConfigureStoreShare(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously, vector<wstring> const & sidStrings)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        AsyncOperationSPtr thisSPtr = operation->Parent;
        auto error = owner_.ServicePackageObj.HostingHolder.Value.FabricActivatorClientObj->EndConfigureSMBShare(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ConfigureSMBShare for Store): Id={0}, Version={1}, Error: {2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ConfigureSMBShare for Staging): Id={0}, Version={1}, Timeout={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            timeoutHelper_.GetRemainingTime());

        wstring stagingLocalFullPath = Path::Combine(
            owner_.ServicePackageObj.runLayout_.GetApplicationWorkFolder(owner_.ServicePackageObj.Id.ApplicationId.ToString()),
            Management::FileStoreService::Constants::StagingRootDirectoryName);
        wstring stagingShareName = wformatString("{0}_{1}", Management::FileStoreService::Constants::StagingShareName, owner_.ServicePackageObj.HostingHolder.Value.NodeName);
        auto configureOperation = owner_.ServicePackageObj.HostingHolder.Value.FabricActivatorClientObj->BeginConfigureSMBShare(
            sidStrings,
            GENERIC_ALL,
            stagingLocalFullPath,
            stagingShareName,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishConfigureStagingShare(operation, false); },
            thisSPtr);
        FinishConfigureStagingShare(configureOperation, true);
    }

    void FinishConfigureStagingShare(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        AsyncOperationSPtr thisSPtr = operation->Parent;
        auto error = owner_.ServicePackageObj.HostingHolder.Value.FabricActivatorClientObj->EndConfigureSMBShare(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ConfigureSMBShare for Staging): Id={0}, Version={1}, Error: {2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        WriteCurrentPackage(thisSPtr);
    }

    void ConfigureNodeForDnsService(AsyncOperationSPtr const & thisSPtr)
    {
        std::wstring sidString;
        const std::wstring DnsServicePackageName(L"Code");
        SecurityPrincipalInformationSPtr info;
        ErrorCode error = owner_.TryGetPackageSecurityPrincipalInfo(DnsServicePackageName, info);
        if (error.IsSuccess())
        {
            sidString = info->SidString;
        }

        auto operation = owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->BeginSetupDnsEnvAndStartMonitor(
            sidString,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnConfigureNodeForDnsServiceComplete(operation, false /*expectedCompletedSynchronously*/); },
            thisSPtr);

        this->OnConfigureNodeForDnsServiceComplete(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnConfigureNodeForDnsServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->EndSetupDnsEnvAndStartMonitor(operation);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, owner_.TraceId,
                "Failed to configure node for DNS service, error {0}", error);
        }
        else
        {
            WriteInfo(TraceType, owner_.TraceId,
                "Successfully configured node for DNS service");
        }

        // Always continue to next step, regardless of the return value.
        // Error in environment setup should not prevent DnsService from being started.
        // DnsService environment is needed for regular services only, containers can still function properly.
        // DnsService will display warning on the node which is not configured properly.
        WriteCurrentPackage(thisSPtr);
    }

    void WriteCurrentPackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.WriteCurrentPackageFile(packageVersionInstance_.Version);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "WriteCurrentPackageFile: Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        AddServiceTypesEntriesToServiceTypeStateManager(thisSPtr);
    }

    void AddServiceTypesEntriesToServiceTypeStateManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->OnServicePackageActivated(
            owner_.serviceTypeInstanceIds_,
            owner_.ServicePackageObj.ApplicationName,
            owner_.componentId_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "OnServicePackageActivated: Id={0}, Version={1}, ErrorCode={2} ",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        owner_.activationTime_ = DateTime::Now();

        LoadCodePackages(thisSPtr);
    }

    void LoadCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.LoadCodePackages(
            packageVersionInstance_,
            packageDescription_,
            packageEnvironment_,
            codePackages_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "LoadCodePackages: Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        owner_.ActivateCodePackagesAsync(
            codePackages_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
        {
            this->FinishActivateCodePackages(operation, error, completed, failed);
        },
            thisSPtr);
    }

    void FinishActivateCodePackages(AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
    {
        UNREFERENCED_PARAMETER(failed);     // in case of failure we abort all of the codePackages_
        UNREFERENCED_PARAMETER(completed);  // in case of success we move all of the codePackages_ inside the package

        if (!error.IsSuccess())
        {
            TransitionToFailed(operation->Parent, error);
            return;
        }
        else
        {
            {
                AcquireWriteLock lock(owner_.Lock);
                owner_.environmentContext__ = packageEnvironment_;
                owner_.codePackages__ = codePackages_;
            }

            TransitionToOpened(operation->Parent);
            return;
        }
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.ServicePackageObj.HostingHolder.RootedObject.HealthManagerObj->ReportHealth(
            owner_.ServicePackageObj.Id,
            owner_.healthPropertyId_,
            SystemHealthReportCode::Hosting_ServicePackageActivated,
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

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failureError)
    {
        owner_.AbortCodePackagesAsync(
            codePackages_,
            [this, failureError](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
        {
            error.ReadValue();
            AbortPackageEnvironment(operation->Parent, failureError);
        },
            thisSPtr);
    }

    void AbortPackageEnvironment(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        if (packageEnvironment_)
        {
            owner_.EnvironmentManagerObj->AbortServicePackageInstanceEnvironment(
                packageEnvironment_,
                packageDescription_,
                owner_.ServicePackageObj.InstanceId);
        }

        if (owner_.ServicePackageObj.Id.ApplicationId.IsSystem() &&
            StringUtility::AreEqualCaseInsensitive(owner_.ServicePackageObj.Id.ServicePackageName,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
        {
            owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->RemoveDnsFromEnvAndStopMonitorAndWait();
        }

        FinishTransitionToFailed(thisSPtr, error);
    }

    void FinishTransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        owner_.TransitionToFailed().ReadValue();
        TryComplete(thisSPtr, error);
        return;
    }

private:
    VersionedServicePackage & owner_;
    TimeoutHelper timeoutHelper_;
    ServicePackageVersionInstance packageVersionInstance_;
    ServicePackageDescription packageDescription_;
    ServicePackageInstanceEnvironmentContextSPtr packageEnvironment_;
    CodePackageMap codePackages_;
};

// ********************************************************************************************************************
// VersionedServicePackage::SwitchAsyncOperation Implementation
//
class VersionedServicePackage::SwitchAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(SwitchAsyncOperation)

public:
    SwitchAsyncOperation(
        __in VersionedServicePackage & owner,
        ServicePackageVersionInstance const & toVersionInstance,
        ServicePackageDescription const & newPackageDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        currentVersionInstance_(),
        currentPackageDescription_(),
        newVersionInstance_(toVersionInstance),
        newPackageDescription_(newPackageDescription),
        newCodePackages_(),
        codePackagesToActivate_(),
        codePackagesToDeactivate_(),
        codePackagesToUpdate_()
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
        auto error = owner_.TransitionToSwitching();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        //For Testing. This test config cancels the parent UpgradeService Package Operation 
        if (HostingConfig::GetConfig().AbortSwitchServicePackage)
        {
            thisSPtr->Parent->Cancel();
            return;
        }

        {
            AcquireReadLock lock(owner_.Lock);
            currentVersionInstance_ = owner_.currentVersionInstance__;
            currentPackageDescription_ = owner_.packageDescription__;
        }

        // check if version instances are same
        if (currentVersionInstance_ == newVersionInstance_)
        {
            TransitionToOpened(thisSPtr);
            return;
        }

        bool versionUpdateOnly = (currentPackageDescription_.ContentChecksum == newPackageDescription_.ContentChecksum);

        ASSERT_IF(
            currentVersionInstance_.Version.RolloutVersionValue.Major != newVersionInstance_.Version.RolloutVersionValue.Major && !versionUpdateOnly,
            "Cannot switch major versions. ServicePackageId={0}, CurrentVersion={1}, NewVersion={2}, VersionUpdateOnly={3}",
            owner_.ServicePackageObj.Id,
            currentVersionInstance_,
            newVersionInstance_,
            versionUpdateOnly);

        if (versionUpdateOnly)
        {
            {
                AcquireReadLock lock(owner_.Lock);
                codePackagesToUpdate_ = owner_.codePackages__;
                newCodePackages_ = owner_.codePackages__;
            }

            SwitchCurrentPackageFile(thisSPtr);
        }
        else
        {
            // Reset the activation time during switch so that we do not report
            // ServiceTypeRegistration timeout without waiting for sufficient time            
            owner_.activationTime_ = DateTime::Now();

            LoadNewCodePackages(thisSPtr);
        }
    }

    void LoadNewCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.LoadCodePackages(newVersionInstance_, newPackageDescription_, owner_.environmentContext__, newCodePackages_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "LoadCodePackages: Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            newVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        SwitchCodePackages(thisSPtr);
    }

    void SwitchCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        {
            AcquireReadLock lock(owner_.Lock);

            for (auto iterNew = newCodePackages_.begin(); iterNew != newCodePackages_.end(); ++iterNew)
            {
                auto iterCurrent = owner_.codePackages__.find(iterNew->first);
                if (iterCurrent == owner_.codePackages__.end())
                {
                    codePackagesToActivate_.insert(make_pair(iterNew->first, iterNew->second));
                }
                else
                {
                    if (iterNew->second->RolloutVersionValue != iterCurrent->second->RolloutVersionValue)
                    {
                        codePackagesToDeactivate_.insert(make_pair(iterCurrent->first, iterCurrent->second));
                        codePackagesToActivate_.insert(make_pair(iterNew->first, iterNew->second));
                    }
                    else
                    {
                        codePackagesToUpdate_.insert(make_pair(iterCurrent->first, iterCurrent->second));
                    }
                }
            }

            for (auto iterCurrent = owner_.codePackages__.begin(); iterCurrent != owner_.codePackages__.end(); ++iterCurrent)
            {
                auto iterNew = newCodePackages_.find(iterCurrent->first);
                if (iterNew == newCodePackages_.end())
                {
                    codePackagesToDeactivate_.insert(make_pair(iterCurrent->first, iterCurrent->second));
                }
            }
        }

        DeactivateModifiedAndRemovedOldCodePackages(thisSPtr);
    }

    void DeactivateModifiedAndRemovedOldCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        bool useActivationTimeout = true;
        if (HostingConfig::GetConfig().ActivationTimeout > HostingConfig::GetConfig().ApplicationUpgradeTimeout)
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "ApplicationUpgradeTimeout {0} should be more than ActivationTimeout {1}.",
                HostingConfig::GetConfig().ApplicationUpgradeTimeout,
                HostingConfig::GetConfig().ActivationTimeout);

            useActivationTimeout = false;
        }

        owner_.DeactivateCodePackagesAsync(
            codePackagesToDeactivate_,
            useActivationTimeout ? HostingConfig::GetConfig().ActivationTimeout : timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
        {
            this->FinishDeactivateModifiedAndRemovedOldCodePackages(operation, error, completed, failed);
        },
            thisSPtr);
        return;
    }

    void FinishDeactivateModifiedAndRemovedOldCodePackages(AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
    {
        UNREFERENCED_PARAMETER(completed);

        // abort the failed code packages, the completed ones are already inactive
        if (!error.IsSuccess())
        {
            AbortFailedModifiedAndRemovedOldCodePackages(operation->Parent, failed);
        }
        else
        {
            SwitchCurrentPackageFile(operation->Parent);
        }
    }

    void AbortFailedModifiedAndRemovedOldCodePackages(AsyncOperationSPtr const & thisSPtr, CodePackageMap const & failedCodePackages)
    {
        owner_.AbortCodePackagesAsync(
            failedCodePackages,
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
        {
            error.ReadValue();
            this->SwitchCurrentPackageFile(operation->Parent);
        },
            thisSPtr);
        return;
    }

    void SwitchCurrentPackageFile(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.WriteCurrentPackageFile(newVersionInstance_.Version);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "SwitchCurrentPackageFile: Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            newVersionInstance_.Version,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        {
            AcquireWriteLock lock(owner_.Lock);
            owner_.currentVersionInstance__ = newVersionInstance_;
            owner_.packageDescription__ = newPackageDescription_;
        }

        UpdateExistingCodePackages(thisSPtr);
    }

    void UpdateExistingCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.UpdateCodePackagesAsync(
            codePackagesToUpdate_,
            newPackageDescription_,
            newVersionInstance_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
        {
            this->FinishUpdateExistingCodePackages(operation, error, completed, failed);
        },
            thisSPtr);
        return;
    }

    void FinishUpdateExistingCodePackages(AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
    {
        // the completed code packages are updated, updated + activated = new code packages of this service package
        codePackagesToUpdate_ = completed;

        // abort the failed code packages, and add them to activate list
        if (!error.IsSuccess())
        {
            AbortUpdateFailedOldCodePackages(operation->Parent, failed);
        }
        else
        {
            ActivateModifiedAndAddedNewCodePackages(operation->Parent);
        }
    }

    void AbortUpdateFailedOldCodePackages(AsyncOperationSPtr const & thisSPtr, CodePackageMap const & failedCodePackages)
    {
        // the updated failed code packages will be aborted and activated again
        for (auto iter = failedCodePackages.begin(); iter != failedCodePackages.end(); ++iter)
        {
            auto newCodePackage = CodePackageSPtr(new CodePackage(
                owner_.ServicePackageObj.HostingHolder,
                VersionedServicePackageHolder(owner_, owner_.CreateComponentRoot()),
                iter->second->Description,
                owner_.ServicePackageObj.Id,
                owner_.ServicePackageObj.InstanceId,
                newVersionInstance_,
                owner_.ServicePackageObj.ApplicationName,
                iter->second->RolloutVersionValue,
                owner_.environmentContext__,
                iter->second->RunAsId,
                iter->second->SetupRunAs,
                iter->second->IsShared,
                ProcessDebugParameters(),
                iter->second->PortBindings,
                iter->second->GuestServiceTypes));
            codePackagesToActivate_.insert(
                make_pair(
                    iter->second->Description.Name,
                    move(newCodePackage)));
        }

        owner_.AbortCodePackagesAsync(
            failedCodePackages,
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
        {
            error.ReadValue();
            this->ActivateModifiedAndAddedNewCodePackages(operation->Parent);
        },
            thisSPtr);
        return;
    }

    void ActivateModifiedAndAddedNewCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.ActivateCodePackagesAsync(
            codePackagesToActivate_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
        {
            this->FinishActivateModifiedAndAddedNewCodePackages(operation, error, completed, failed);
        },
            thisSPtr);
        return;
    }

    void FinishActivateModifiedAndAddedNewCodePackages(AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
    {
        UNREFERENCED_PARAMETER(completed);
        UNREFERENCED_PARAMETER(failed);

        if (!error.IsSuccess())
        {
            // deactivate all of the packages we tried to update and move to Failed state.
            owner_.AbortCodePackagesAsync(
                codePackagesToActivate_,
                [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
            {
                error.ReadValue();
                this->TransitionToFailed(operation->Parent, error);
            },
                operation->Parent);
        }
        else
        {
            ChangeOwnedCodePackages();
            TransitionToOpened(operation->Parent);
        }
    }

    void ChangeOwnedCodePackages()
    {
        {
            AcquireWriteLock lock(owner_.Lock);
            owner_.codePackages__.clear();

            for (auto iter = codePackagesToUpdate_.begin(); iter != codePackagesToUpdate_.end(); ++iter)
            {
                owner_.codePackages__.insert(make_pair(iter->first, iter->second));
            }

            for (auto iter = codePackagesToActivate_.begin(); iter != codePackagesToActivate_.end(); ++iter)
            {
                owner_.codePackages__.insert(make_pair(iter->first, iter->second));
            }

            ASSERT_IF(
                owner_.codePackages__.size() != newCodePackages_.size(),
                "The size {0} of the final code packages must match size {0} of new code packages",
                owner_.codePackages__.size(),
                newCodePackages_.size());
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
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        CodePackageMap existingCodePackages;
        {
            AcquireWriteLock lock(owner_.Lock);
            existingCodePackages = move(owner_.codePackages__);
            owner_.codePackages__.clear();
        }

        owner_.AbortCodePackagesAsync(
            existingCodePackages,
            [this, failedError](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
        {
            error.ReadValue();
            owner_.TransitionToFailed().ReadValue();
            TryComplete(operation->Parent, failedError);
        },
            thisSPtr);
        return;
    }

private:
    VersionedServicePackage & owner_;
    TimeoutHelper timeoutHelper_;
    ServicePackageVersionInstance currentVersionInstance_;
    ServicePackageDescription currentPackageDescription_;
    ServicePackageVersionInstance const newVersionInstance_;
    ServicePackageDescription newPackageDescription_;
    CodePackageMap newCodePackages_;

    CodePackageMap codePackagesToActivate_;
    CodePackageMap codePackagesToDeactivate_;
    CodePackageMap codePackagesToUpdate_;
};

// ********************************************************************************************************************
// VersionedServicePackage::CloseAsyncOperation Implementation
//
class VersionedServicePackage::CloseAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in VersionedServicePackage & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        packageVersionInstance_(),
        timeoutHelper_(timeout)
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
        if (owner_.GetState() == VersionedServicePackage::Closed)
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

        {
            AcquireReadLock lock(owner_.Lock);
            packageVersionInstance_ = owner_.currentVersionInstance__;
        }

        DeactivateCodePackages(thisSPtr);
    }

private:

    void DeactivateCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        CodePackageMap existingCodePackages;
        {
            AcquireWriteLock lock(owner_.Lock);
            existingCodePackages = move(owner_.codePackages__);
            owner_.codePackages__.clear();
        }

        owner_.DeactivateCodePackagesAsync(
            existingCodePackages,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
        {
            this->FinishDeactivateCodePackages(operation, error, completed, failed);
        },
            thisSPtr);
        return;
    }

    void FinishDeactivateCodePackages(AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
    {
        UNREFERENCED_PARAMETER(completed);

        if (!error.IsSuccess())
        {
            AbortFailedCodePackages(operation->Parent, failed);
        }
        else
        {
            CleanupEnvironment(operation->Parent);
        }
    }

    void AbortFailedCodePackages(AsyncOperationSPtr const & thisSPtr, CodePackageMap const & failedCodePackages)
    {
        owner_.AbortCodePackagesAsync(
            failedCodePackages,
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
        {
            error.ReadValue();
            this->CleanupEnvironment(operation->Parent);
        },
            thisSPtr);
        return;
    }

    void CleanupEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        ServicePackageInstanceEnvironmentContextSPtr packageEnvironment;

        {
            AcquireWriteLock lock(owner_.Lock);
            packageEnvironment = move(owner_.environmentContext__);
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CleanupPackageEnvironment): Id={0}, Version={1}, Timeout={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            timeout);
        auto operation = owner_.EnvironmentManagerObj->BeginCleanupServicePackageInstanceEnvironment(
            packageEnvironment,
            owner_.packageDescription__,
            owner_.ServicePackageObj.InstanceId,
            timeout,
            [this, packageEnvironment](AsyncOperationSPtr const & operation) { this->FinishCleanupEnvironment(packageEnvironment, operation, false); },
            thisSPtr);
        FinishCleanupEnvironment(packageEnvironment, operation, true);
    }

    void FinishCleanupEnvironment(
        ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironment,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.EnvironmentManagerObj->EndCleanupServicePackageInstanceEnvironment(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(CleanupPackageEnvironment): Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            owner_.EnvironmentManagerObj->AbortServicePackageInstanceEnvironment(
                packageEnvironment,
                owner_.packageDescription__,
                owner_.ServicePackageObj.InstanceId);
        }

        if (owner_.ServicePackageObj.Id.ApplicationId.IsSystem() &&
            StringUtility::AreEqualCaseInsensitive(owner_.ServicePackageObj.Id.ServicePackageName,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
        {
            ConfigureNodeForDnsService(operation->Parent);
        }
        else
        {
            RemoveCurrentPackageFile(operation->Parent);
        }
    }

    void ConfigureNodeForDnsService(AsyncOperationSPtr const & thisSPtr)
    {
        std::wstring sidString;
        const std::wstring DnsServicePackageName(L"Code");
        SecurityPrincipalInformationSPtr info;
        ErrorCode error = owner_.TryGetPackageSecurityPrincipalInfo(DnsServicePackageName, info);
        if (error.IsSuccess())
        {
            sidString = info->SidString;
        }

        auto operation = owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->BeginRemoveDnsFromEnvAndStopMonitor(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnConfigureNodeForDnsServiceComplete(operation, false /*expectedCompletedSynchronously*/); },
            thisSPtr);

        this->OnConfigureNodeForDnsServiceComplete(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnConfigureNodeForDnsServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->EndRemoveDnsFromEnvAndStopMonitor(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, owner_.TraceId,
                "Failed to remove DNS service configuration from node, error {0}", error);
        }
        else
        {
            WriteInfo(TraceType, owner_.TraceId,
                "Successfully removed DNS service configuration from node");
        }

        // Always continue to the next step. Cleanup cannot fail if successfully invoked.
        // The node will continue to function properly even if the environment is not cleaned up properly.
        RemoveCurrentPackageFile(thisSPtr);
    }

    void RemoveCurrentPackageFile(AsyncOperationSPtr const & thisSPtr)
    {
        auto currentPackageFilePath = owner_.runLayout_.GetCurrentServicePackageFile(
            owner_.ServicePackageObj.Id.ApplicationId.ToString(),
            owner_.ServicePackageObj.ServicePackageName,
            owner_.ServicePackageObj.Id.PublicActivationId);

        auto error = File::Delete2(currentPackageFilePath, true);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FileNotFound))
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Failed to remove current package file: Id={0}, Version={1}, File={2}. ErrorCode={3}.",
                owner_.ServicePackageObj.Id,
                packageVersionInstance_,
                currentPackageFilePath,
                error);

            TransitionToFailed(thisSPtr);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Successfully removed current package file: Id={0}, Version={1}, File={2}. ErrorCode={3}.",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            currentPackageFilePath,
            error);

        TransitionToClosed(thisSPtr);
    }

    void TransitionToClosed(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToClosed();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr);
            return;
        }
        else
        {
            // In every case we need to unregister service package from LRM to free up resources.
            owner_.ServicePackageObj.HostingHolder.RootedObject.LocalResourceManagerObj->UnregisterServicePackage(owner_);

            // unregister the componenet from Healthmanager
            owner_.ServicePackageObj.HostingHolder.RootedObject.HealthManagerObj->UnregisterSource(owner_.ServicePackageObj.Id, owner_.healthPropertyId_);

            // remove any failures registered for this service package
            owner_.ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->UnregisterFailure(owner_.failureId_);

            // Deletes all the ServiceType entries belonging to this ServicePackage from the ServiceTypeStateManager            
            owner_.ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->OnServicePackageDeactivated(
                owner_.serviceTypeInstanceIds_,
                owner_.componentId_);

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
    VersionedServicePackage & owner_;
    TimeoutHelper timeoutHelper_;
    ServicePackageVersionInstance packageVersionInstance_;
};

// ********************************************************************************************************************
// VersionedServicePackage::TerminateFabricTypeHostAsyncOperation Implementation
//
class VersionedServicePackage::TerminateFabricTypeHostAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(TerminateFabricTypeHostAsyncOperation)

public:
    TerminateFabricTypeHostAsyncOperation(
        __in VersionedServicePackage & owner,
        int64 instanceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timer_(),
        instanceId_(instanceId)
    {
    }

    virtual ~TerminateFabricTypeHostAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out int64 & instanceId)
    {
        auto thisPtr = AsyncOperation::End<TerminateFabricTypeHostAsyncOperation>(operation);
        instanceId = move(thisPtr->instanceId_);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TryTerminateFabricTypeHostCodePackage(thisSPtr);
    }

private:
    void TryTerminateFabricTypeHostCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        bool isOpened = CheckIfOpened();

        if (!isOpened)
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "TerminateFabricTypeHostCodePackage: VersionedServicePackage is closed");
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        CodePackageSPtr fabricTypeHostPackageSPtr;

        {
            AcquireReadLock lock(owner_.Lock);

            auto iter = owner_.codePackages__.find(Constants::ImplicitTypeHostCodePackageName);
            if (iter != owner_.codePackages__.end())
            {
                fabricTypeHostPackageSPtr = iter->second;
            }
        }

        //If the FabricTypeHost code package is not found then we ended up deactivating the code package. So, no need for termination.
        if (!fabricTypeHostPackageSPtr)
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "TerminateFabricTypeHostCodePackage: FabricTypeHost code package not hosted");
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        TimeSpan retryDueTime;
        auto error = fabricTypeHostPackageSPtr->TerminateCodePackageExternally(retryDueTime);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "TerminateFabricTypeHostCodePackage: Fabric unable to terminate FabricTypeHost with error {0}",
                error);

            //AppHostId not found at Fabric, will retry
            if (error.IsError(ErrorCodeValue::NotFound))
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "TerminateFabricTypeHostCodePackage: Retrying FabricTypeHost termination in {0}ms",
                    retryDueTime.TotalMilliseconds());

                timer_ = Timer::Create("VersionedServicePackage.TerminateFabricTypeHost",
                    [this, thisSPtr](TimerSPtr const & timer)
                {
                    timer->Cancel();
                    this->TryTerminateFabricTypeHostCodePackage(thisSPtr);
                },
                    false);
                timer_->Change(retryDueTime);

                return;
            }
        }

        thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    bool CheckIfOpened()
    {
        return (owner_.GetState() == VersionedServicePackage::Opened);
    }

private:
    VersionedServicePackage & owner_;
    TimerSPtr timer_;
    int64 instanceId_;
};

// ********************************************************************************************************************
// VersionedServicePackage::RestartCodePackageInstanceAsyncOperation Implementation
//
class VersionedServicePackage::RestartCodePackageInstanceAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(RestartCodePackageInstanceAsyncOperation)

public:
    RestartCodePackageInstanceAsyncOperation(
        __in VersionedServicePackage & owner,
        wstring const & codePackageName,
        int64 instanceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , codePackageName_(codePackageName)
        , instanceId_(instanceId)
        , timeout_(timeout)
    {
    }

    virtual ~RestartCodePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<RestartCodePackageInstanceAsyncOperation>(operation);
        
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        uint64 currentState;
        auto shouldRestart = this->IsCurrentStateValidForRestart(currentState);

        if (!shouldRestart)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "RestartCodePackageInstanceAsyncOperation: Skipping restart as current VSP state is '{0}'. CodePackageName=[{1}], InstanceId=[{2}].",
                owner_.StateToString(currentState),
                codePackageName_,
                instanceId_);

            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        this->RestartCodePackage(thisSPtr);
    }

private:
    void RestartCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        CodePackageSPtr codePackageSPtr;

        {
            AcquireReadLock lock(owner_.Lock);

            auto iter = owner_.codePackages__.find(codePackageName_);
            if (iter != owner_.codePackages__.end())
            {
                codePackageSPtr = iter->second;
            }
        }

        // If code package is not found then we ended up deactivating the code package. So, no need for termination.
        if (!codePackageSPtr)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "RestartCodePackageInstanceAsyncOperation: CodePackage '{0}' is not present.",
                codePackageName_);
            
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::NotFound);            
            return;
        }

        auto operation = codePackageSPtr->BeginRestartCodePackageInstance(
            instanceId_,
            timeout_,
            [this, codePackageSPtr](AsyncOperationSPtr const & operation) { OnRestartCompleted(operation, false, codePackageSPtr); },
            thisSPtr);

        this->OnRestartCompleted(operation, true, codePackageSPtr);
    }

    void OnRestartCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously, CodePackageSPtr codePackageSPtr)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = codePackageSPtr->EndRestartCodePackageInstance(operation);

        TryComplete(operation->Parent, error);
    }

    bool IsCurrentStateValidForRestart(uint64 & currState) // TODO: Should we include Analyzing state as valid state.
    {
        currState = owner_.GetState();

        return (currState == VersionedServicePackage::Opened);
    }

private:
    VersionedServicePackage & owner_;
    wstring codePackageName_;
    int64 instanceId_;
    TimeSpan timeout_;
};

// ********************************************************************************************************************
// VersionedServicePackage Implementation
//

VersionedServicePackage::VersionedServicePackage(
    ServicePackage2Holder const & servicePackageHolder,
    ServicePackageVersionInstance const & versionInstance,
    ServicePackageDescription const & servicePackageDescription,
    vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
    : ComponentRoot(),
    StateMachine(Created),
    servicePackageHolder_(servicePackageHolder),
    currentVersionInstance__(versionInstance),
    packageDescription__(servicePackageDescription),
    environmentContext__(),
    codePackages__(),
    failureId_(wformatString("VersionedServicePackage:{0}:{1}", servicePackageHolder.RootedObject.Id, servicePackageHolder.RootedObject.InstanceId)),
    componentId_(servicePackageHolder.RootedObject.Id.ToString()),
    healthPropertyId_(*ServiceModel::Constants::HealthActivationProperty),
    activationTime_(DateTime::MaxValue),
    runLayout_(servicePackageHolder.RootedObject.HostingHolder.RootedObject.DeploymentFolder),
    serviceTypeInstanceIds_(serviceTypeInstanceIds)
{
    this->SetTraceId(servicePackageHolder_.Root->TraceId);
    WriteNoise(
        TraceType, TraceId,
        "VersionedServicePackage.constructor: {0} ({1}:{2})",
        static_cast<void*>(this),
        servicePackageHolder_.RootedObject.Id,
        servicePackageHolder_.RootedObject.InstanceId);
}

VersionedServicePackage::~VersionedServicePackage()
{
    WriteNoise(
        TraceType, TraceId,
        "VersionedServicePackage.destructor: {0} ({1}:{2})",
        static_cast<void*>(this),
        servicePackageHolder_.RootedObject.Id,
        servicePackageHolder_.RootedObject.InstanceId);
}

AsyncOperationSPtr VersionedServicePackage::BeginOpen(
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

ErrorCode VersionedServicePackage::EndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr VersionedServicePackage::BeginSwitch(
    ServicePackageVersionInstance const & toVersion,
    ServicePackageDescription toPackageDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SwitchAsyncOperation>(
        *this,
        toVersion,
        toPackageDescription,
        timeout,
        callback,
        parent);
}

ErrorCode VersionedServicePackage::EndSwitch(
    AsyncOperationSPtr const & operation)
{
    return SwitchAsyncOperation::End(operation);
}

AsyncOperationSPtr VersionedServicePackage::BeginClose(
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

void VersionedServicePackage::EndClose(
    AsyncOperationSPtr const & operation)
{
    CloseAsyncOperation::End(operation);
}

void VersionedServicePackage::OnAbort()
{
    CodePackageMap codePackages;
    ServicePackageInstanceEnvironmentContextSPtr packageEnvironment;

    {
        AcquireWriteLock lock(this->Lock);
        codePackages = move(this->codePackages__);
        packageEnvironment = move(this->environmentContext__);
    }

    // abort all code packages
    for (auto iter = codePackages.begin(); iter != codePackages.end(); ++iter)
    {
        iter->second->AbortAndWaitForTermination();
    }
    codePackages.clear();

    if (packageEnvironment)
    {
        this->EnvironmentManagerObj->AbortServicePackageInstanceEnvironment(packageEnvironment, packageDescription__, ServicePackageObj.InstanceId);
    }

    if (this->ServicePackageObj.Id.ApplicationId.IsSystem() &&
        StringUtility::AreEqualCaseInsensitive(ServicePackageObj.Id.ServicePackageName,
            ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
    {
        this->ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->RemoveDnsFromEnvAndStopMonitorAndWait();
    }

    // unregister the componenet from Healthmanager
    this->ServicePackageObj.HostingHolder.RootedObject.HealthManagerObj->UnregisterSource(this->ServicePackageObj.Id, healthPropertyId_);

    // remove any failures registered for this service package
    this->ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->UnregisterFailure(failureId_);

    // Deletes all the ServiceType entries belonging to this ServicePackage from the ServiceTypeStateManager
    this->ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->OnServicePackageDeactivated(serviceTypeInstanceIds_, componentId_);

    // Unregisters service package from LRM.
    ServicePackageObj.HostingHolder.RootedObject.LocalResourceManagerObj->UnregisterServicePackage(*this);
}

ServicePackage2 const & VersionedServicePackage::get_ServicePackage() const
{
    return this->servicePackageHolder_.RootedObject;
}

EnvironmentManagerUPtr const & VersionedServicePackage::get_EnvironmentManager() const
{
    return this->ServicePackageObj.HostingHolder.RootedObject.ApplicationManagerObj->EnvironmentManagerObj;
}

ServicePackageVersionInstance const VersionedServicePackage::get_CurrentVersionInstance() const
{
    {
        AcquireReadLock lock(Lock);
        return this->currentVersionInstance__;
    }
}

ULONG VersionedServicePackage::GetFailureCount() const
{
    ULONG maxFailureCount = 0;
    ULONG failureCount;
    {
        AcquireReadLock lock(this->Lock);
        for (auto iter = codePackages__.begin(); iter != codePackages__.end(); ++iter)
        {
            failureCount = iter->second->GetMaxContinuousFailureCount();
            if (failureCount > maxFailureCount)
            {
                maxFailureCount = failureCount;
            }
        }
    }

    return maxFailureCount;
}

vector<CodePackageSPtr> VersionedServicePackage::GetCodePackages(wstring const & filterCodePackageName)
{
    vector<CodePackageSPtr> codePackages;
    {
        AcquireReadLock lock(this->Lock);
        for (auto itCodePackage = codePackages__.begin(); itCodePackage != codePackages__.end(); ++itCodePackage)
        {
            if (itCodePackage->second->IsImplicitTypeHost && 
                HostingConfig::GetConfig().IncludeGuestServiceTypeHostInCodePackageQuery == false)
            {
                continue;
            }

            if (filterCodePackageName.empty())
            {
                codePackages.push_back(itCodePackage->second);
            }
            else if (StringUtility::AreEqualCaseInsensitive(itCodePackage->second->Description.Name, filterCodePackageName))
            {
                codePackages.push_back(itCodePackage->second);
                break;
            }
        }
    }

    return codePackages;
}

ErrorCode VersionedServicePackage::AnalyzeApplicationUpgrade(
    ServicePackageVersionInstance const & toVersionInstance,
    ServicePackageDescription const & toPackageDescription,
    __out set<wstring, IsLessCaseInsensitiveComparer<wstring>> & affectedRuntimeIds)
{
    auto error = this->TransitionToAnalyzing();
    if (!error.IsSuccess())
    {
        return error;
    }

    ServicePackageVersionInstance currentPackageVersionInstance;
    CodePackageMap currentCodePackages;
    {
        AcquireReadLock lock(this->Lock);
        currentPackageVersionInstance = this->currentVersionInstance__;
        currentCodePackages = this->codePackages__;
    }

    if (currentPackageVersionInstance == toVersionInstance)
    {
        // None of the CodePackages in this ServicePackage will be affected        
        this->TransitionToOpened().ReadValue();
        return ErrorCodeValue::Success;
    }

    //it might happen that RG policy on SP level is changing even though the CP itself is not
    //we can optimize this and do a dynamic update of limits on Linux
    //Bug #9358009
    bool hasRgChange = this->PackageDescription.ResourceGovernanceDescription.HasRgChange(
        toPackageDescription.ResourceGovernanceDescription,
        PackageDescription.DigestedCodePackages,
        toPackageDescription.DigestedCodePackages);

    for (auto currentIter = currentCodePackages.begin(); currentIter != currentCodePackages.end(); ++currentIter)
    {
        bool matchingEntryFound = false;
        for (auto newIter = toPackageDescription.DigestedCodePackages.begin(); newIter != toPackageDescription.DigestedCodePackages.end(); ++newIter)
        {
            if (StringUtility::AreEqualCaseInsensitive(currentIter->second->CodePackageInstanceId.CodePackageName, newIter->CodePackage.Name))
            {
                matchingEntryFound = true;
                if (currentIter->second->RolloutVersionValue != newIter->RolloutVersionValue || hasRgChange)
                {
                    // The CodePackage is modified in the new ServicePackage or RG is changing in some way
                    error = this->ServicePackageObj.HostingHolder.Value.FabricRuntimeManagerObj->GetRuntimeIds(currentIter->second->CodePackageInstanceId, affectedRuntimeIds);
                }

                break;
            }
        }

        if (!matchingEntryFound)
        {
            // The CodePackage is deleted in the new ServicePackage
            error = this->ServicePackageObj.HostingHolder.Value.FabricRuntimeManagerObj->GetRuntimeIds(currentIter->second->CodePackageInstanceId, affectedRuntimeIds);
        }

        if (!error.IsSuccess()) { break; }
    }

    this->TransitionToOpened().ReadValue();
    return error;
}

ErrorCode VersionedServicePackage::WriteCurrentPackageFile(
    ServicePackageVersion const & packageVersion)
{
    wstring currentPackageFilePath = runLayout_.GetCurrentServicePackageFile(
        this->ServicePackageObj.Id.ApplicationId.ToString(),
        this->ServicePackageObj.ServicePackageName,
        this->ServicePackageObj.Id.PublicActivationId);

    wstring packageFilePath = runLayout_.GetServicePackageFile(
        this->ServicePackageObj.Id.ApplicationId.ToString(),
        this->ServicePackageObj.ServicePackageName,
        packageVersion.RolloutVersionValue.ToString());

    return File::Copy(packageFilePath, currentPackageFilePath, true);
}

ErrorCode VersionedServicePackage::LoadCodePackages(
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    ServicePackageDescription const & packageDescription,
    ServicePackageInstanceEnvironmentContextSPtr const & envContext,
    __inout CodePackageMap & codePackages)
{
    for (auto iter = packageDescription.DigestedCodePackages.begin();
        iter != packageDescription.DigestedCodePackages.end();
        ++iter)
    {
        wstring userId;
        if (!iter->RunAsPolicy.CodePackageRef.empty())
        {
            ASSERT_IFNOT(
                StringUtility::AreEqualCaseInsensitive(iter->CodePackage.Name, iter->RunAsPolicy.CodePackageRef),
                "Error in the DigestedCodePackage element. The CodePackageRef '{0}' in RunAsPolicy does not match the CodePackage Name '{1}'.",
                iter->RunAsPolicy.CodePackageRef,
                iter->CodePackage.Name);

            auto error = this->ServicePackageObj.ApplicationEnvironment->TryGetUserId(iter->RunAsPolicy.UserRef, userId);
            if (!error.IsSuccess())
            {
                Assert::CodingError("UserId {0} must be present in the application environment context.", iter->RunAsPolicy.UserRef);
            }
        }
        else
        {
            // If the ServicePackage being activated is ResourceMonitor, then we add a special id so that we run it as part of windows administrator group
            if (ServicePackageObj.Id.ApplicationId.IsSystem() &&
                ServicePackageObj.Id.ServicePackageName == L"RMS")
            {
                userId = *Constants::WindowsFabricAdministratorsGroupAllowedUser;
            }
        }

        wstring setuppointRunas;
        if (!iter->SetupRunAsPolicy.CodePackageRef.empty())
        {
            ASSERT_IFNOT(
                StringUtility::AreEqualCaseInsensitive(iter->CodePackage.Name, iter->SetupRunAsPolicy.CodePackageRef),
                "Error in the DigestedCodePackage element. The CodePackageRef '{0}' in RunAsPolicy does not match the CodePackage Name '{1}'.",
                iter->SetupRunAsPolicy.CodePackageRef,
                iter->CodePackage.Name);

            auto error = this->ServicePackageObj.ApplicationEnvironment->TryGetUserId(iter->SetupRunAsPolicy.UserRef, setuppointRunas);
            if (!error.IsSuccess())
            {
                Assert::CodingError("UserId {0} must be present in the application environment context.", iter->SetupRunAsPolicy.UserRef);
            }
        }
        Common::EnvironmentMap envMap;
        wstring envBlock = iter->DebugParameters.EnvironmentBlock;
        LPVOID penvBlock = &envBlock;

        Environment::FromEnvironmentBlock(penvBlock, envMap);

        wstring codePackagePath = this->ServicePackageObj.runLayout_.GetCodePackageFolder(this->ServicePackageObj.ApplicationEnvironment->ApplicationId.ToString(),
            this->ServicePackageObj.ServicePackageName,
            iter->CodePackage.Name,
            iter->CodePackage.Version,
            false);
        wstring lockFilePath = iter->DebugParameters.LockFile;
        if (!lockFilePath.empty() && !Path::IsPathRooted(lockFilePath))
        {
            lockFilePath = Path::Combine(codePackagePath, lockFilePath);
        }

        wstring debugParamsFile = iter->DebugParameters.DebugParametersFile;
        if (!debugParamsFile.empty() && !Path::IsPathRooted(debugParamsFile))
        {
            debugParamsFile = Path::Combine(codePackagePath, debugParamsFile);
        }

        ProcessDebugParameters debugParameters(iter->DebugParameters.ExePath,
            iter->DebugParameters.Arguments,
            lockFilePath,
            iter->DebugParameters.WorkingFolder,
            debugParamsFile,
            envMap,
            iter->DebugParameters.ContainerEntryPoints,
            iter->DebugParameters.ContainerMountedVolumes,
            iter->DebugParameters.ContainerEnvironmentBlock);

        auto codePackage = CodePackageSPtr(new CodePackage(
            this->ServicePackageObj.HostingHolder,
            VersionedServicePackageHolder(*this, this->CreateComponentRoot()),
            *iter,
            this->ServicePackageObj.Id,
            this->ServicePackageObj.InstanceId,
            servicePackageVersionInstance,
            this->ServicePackageObj.ApplicationName,
            iter->RolloutVersionValue,
            envContext,
            userId,
            setuppointRunas,
            iter->IsShared,
            debugParameters,
            GetCodePackagePortMappings(iter->ContainerPolicies, envContext)));
        codePackage->SetDebugParameters(debugParameters);

        codePackages.insert(
            make_pair(
                codePackage->Description.Name,
                move(codePackage)));
    }

    // check for guest service types
    vector<wstring> guestServiceTypes;
    for (auto iter = packageDescription.DigestedServiceTypes.ServiceTypes.begin();
        iter != packageDescription.DigestedServiceTypes.ServiceTypes.end();
        ++iter)
    {
        if (!iter->IsStateful && iter->UseImplicitHost)
        {
            guestServiceTypes.push_back(iter->ServiceTypeName);
        }
    }

    if (guestServiceTypes.size() > 0)
    {
        CodePackageSPtr typeHostCodePackage;

        auto error = CreateImplicitTypeHostCodePackage(
            packageDescription.ManifestVersion,
            servicePackageVersionInstance,
            guestServiceTypes,
            envContext,
            typeHostCodePackage);
        if (!error.IsSuccess()) { return error; }

        codePackages.insert(
            make_pair(
                typeHostCodePackage->Description.Name,
                move(typeHostCodePackage)));
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode VersionedServicePackage::CreateImplicitTypeHostCodePackage(
    wstring const & codePackageVersion,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    vector<wstring> const & typeNames,
    ServicePackageInstanceEnvironmentContextSPtr const & envContext,
    __out CodePackageSPtr & typeHostCodePackage)
{
    wstring typeHostPath;

    //no need to set this if we do not use FabricTypeHost.exe
    if (!HostingConfig::GetConfig().HostGuestServiceTypeInProc)
    {
        auto error = this->ServicePackageObj.HostingHolder.RootedObject.GetTypeHostPath(typeHostPath);
        if (!error.IsSuccess()) { return error; }
    }

    wstring arguments = L"";
    for (auto iter = typeNames.begin(); iter != typeNames.end(); ++iter)
    {
        arguments = wformatString("{0} -type:{1}", arguments, *iter);
    }

    DigestedCodePackageDescription codePackageDescription;
    codePackageDescription.CodePackage.Name = (wstring)Constants::ImplicitTypeHostCodePackageName;
    codePackageDescription.CodePackage.Version = codePackageVersion;
    codePackageDescription.CodePackage.IsShared = false;
    codePackageDescription.CodePackage.EntryPoint.EntryPointType = EntryPointType::Exe;
    codePackageDescription.CodePackage.EntryPoint.ExeEntryPoint.Program = typeHostPath;
    codePackageDescription.CodePackage.EntryPoint.ExeEntryPoint.Arguments = arguments;
    codePackageDescription.CodePackage.EntryPoint.ExeEntryPoint.WorkingFolder = ServiceModel::ExeHostWorkingFolder::Work;

    RolloutVersion rolloutVersion = RolloutVersion::Invalid;

    typeHostCodePackage = CodePackageSPtr(new CodePackage(
        this->ServicePackageObj.HostingHolder,
        VersionedServicePackageHolder(*this, this->CreateComponentRoot()),
        codePackageDescription,
        this->ServicePackageObj.Id,
        this->ServicePackageObj.InstanceId,
        servicePackageVersionInstance,
        this->ServicePackageObj.ApplicationName,
        rolloutVersion,
        envContext,
        L"",
        L"",
        false,
        ProcessDebugParameters(),
        map<wstring, wstring>(),
        typeNames));

    return ErrorCode(ErrorCodeValue::Success);
}


void VersionedServicePackage::ActivateCodePackagesAsync(
    CodePackageMap const & codePackages,
    TimeSpan const timeout,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(
        TraceType,
        TraceId,
        "Begin(ActivateCodePackages): Id={0}:{1}, CodePackageCount={2}, Timeout={3}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        codePackages.size(),
        timeout);
    auto operation = AsyncOperation::CreateAndStart<ActivateCodePackagesAsyncOperation>(
        *this,
        codePackages,
        timeout,
        [this, asyncCallback](AsyncOperationSPtr const & operation) { this->FinishAtivateCodePackages(operation, false, asyncCallback); },
        parent);
    this->FinishAtivateCodePackages(operation, true, asyncCallback);
}

void VersionedServicePackage::FinishAtivateCodePackages(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    CodePackageMap failed;
    CodePackageMap completed;
    auto error = ActivateCodePackagesAsyncOperation::End(operation, failed, completed);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "End(ActivateCodePackages): Id={0}:{1}, CompletedCount={2}, FailedCount={3}, ErrorCode={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        completed.size(),
        failed.size(),
        error);

    asyncCallback(operation, error, completed, failed);
}

void VersionedServicePackage::UpdateCodePackagesAsync(
    CodePackageMap const & codePackages,
    ServicePackageDescription const & newPackageDescription,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    TimeSpan const timeout,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(
        TraceType,
        TraceId,
        "Begin(UpdateCodePackages): Id={0}:{1}, CodePackageCount={2}, ServicePackageVersionInstance={3}, Timeout={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        codePackages.size(),
        servicePackageVersionInstance,
        timeout);

    auto operation = AsyncOperation::CreateAndStart<UpdateCodePackagesAsyncOperation>(
        *this,
        codePackages,
        newPackageDescription,
        servicePackageVersionInstance,
        timeout,
        [this, asyncCallback](AsyncOperationSPtr const & operation) { this->FinishUpdateCodePackages(operation, false, asyncCallback); },
        parent);
    this->FinishUpdateCodePackages(operation, true, asyncCallback);
}

void VersionedServicePackage::FinishUpdateCodePackages(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    CodePackageMap failed;
    CodePackageMap completed;
    auto error = UpdateCodePackagesAsyncOperation::End(operation, failed, completed);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "End(UpdateCodePackages): Id={0}:{1}, CompletedCount={2}, FailedCount={3}, ErrorCode={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        completed.size(),
        failed.size(),
        error);

    asyncCallback(operation, error, completed, failed);
}

void VersionedServicePackage::DeactivateCodePackagesAsync(
    CodePackageMap const & codePackages,
    TimeSpan const timeout,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(
        TraceType,
        TraceId,
        "Begin(DeactivateCodePackages): Id={0}:{1}, CodePackageCount={2}, Timeout={3}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        codePackages.size(),
        timeout);
    auto operation = AsyncOperation::CreateAndStart<DeactivateCodePackagesAsyncOperation>(
        *this,
        codePackages,
        timeout,
        [this, asyncCallback](AsyncOperationSPtr const & operation) { this->FinishDeactivateCodePackages(operation, false, asyncCallback); },
        parent);
    this->FinishDeactivateCodePackages(operation, true, asyncCallback);
}

void VersionedServicePackage::FinishDeactivateCodePackages(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    CodePackageMap failed;
    CodePackageMap completed;
    auto error = DeactivateCodePackagesAsyncOperation::End(operation, failed, completed);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "End(DectivateCodePackages): Id={0}:{1}, CompletedCount={2}, FailedCount={3}, ErrorCode={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        completed.size(),
        failed.size(),
        error);

    asyncCallback(operation, error, completed, failed);
}

void VersionedServicePackage::AbortCodePackagesAsync(
    CodePackageMap const & codePackages,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
    AsyncOperationSPtr const & parent)
{
    TimeSpan timeout = TimeSpan::MaxValue;
    WriteNoise(
        TraceType,
        TraceId,
        "Begin(AbortCodePackages): Id={0}:{1}, CodePackageCount={2}, Timeout={3}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        codePackages.size(),
        timeout);
    auto operation = AsyncOperation::CreateAndStart<AbortCodePackagesAsyncOperation>(
        *this,
        codePackages,
        timeout,
        [this, asyncCallback](AsyncOperationSPtr const & operation) { this->FinishAbortCodePackages(operation, false, asyncCallback); },
        parent);
    this->FinishAbortCodePackages(operation, true, asyncCallback);
}

void VersionedServicePackage::FinishAbortCodePackages(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    CodePackageMap failed;
    CodePackageMap completed;
    auto error = AbortCodePackagesAsyncOperation::End(operation, failed, completed);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "End(AbortCodePackages): Id={0}:{1}, CompletedCount={2}, FailedCount={3}, ErrorCode={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        completed.size(),
        failed.size(),
        error);

    asyncCallback(operation, error, completed, failed);
}

void VersionedServicePackage::OnServiceTypeRegistrationNotFound(
    uint64 const registrationTableVersion,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    ULONG failureCount = this->GetFailureCount();

    WriteNoise(
        TraceType,
        TraceId,
        "OnServiceTypeRegistrationNotFound: {0}:{1}:{2}, VersionedServiceTypeId={3}, RegistrationTableVersion={4}, FailureCount={5}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        CurrentVersionInstance,
        versionedServiceTypeId,
        registrationTableVersion,
        failureCount);

    if (failureCount > (ULONG)HostingConfig::GetConfig().ServiceTypeDisableFailureThreshold)
    {
        bool disable = false;
        {
            AcquireReadLock lock(this->Lock);
            if (this->GetState_CallerHoldsLock() == VersionedServicePackage::Opened)
            {
                this->ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->RegisterFailure(failureId_);
                disable = true;
            }
        }

        if (disable)
        {
            this->ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->Disable(
                ServiceTypeInstanceIdentifier(versionedServiceTypeId.Id, servicePackageInstanceId.ActivationContext, servicePackageInstanceId.PublicActivationId),
                this->ServicePackageObj.ApplicationName,
                failureId_);
        }
    }

    bool reportRegistrationTimeout = false;
    {
        AcquireReadLock lock(this->Lock);
        if ((DateTime::Now() - this->activationTime_ > HostingConfig::GetConfig().ServiceTypeRegistrationTimeout) &&
            this->GetState_CallerHoldsLock() == VersionedServicePackage::Opened)
        {
            reportRegistrationTimeout = true;
        }
    }

    if (reportRegistrationTimeout)
    {
        this->ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->OnServiceTypeNotRegistrationNotFoundAfterTimeout(
            ServiceTypeInstanceIdentifier(versionedServiceTypeId.Id, servicePackageInstanceId.ActivationContext, servicePackageInstanceId.PublicActivationId),
            registrationTableVersion);
    }
}

map<wstring, wstring> VersionedServicePackage::GetCodePackagePortMappings(
    ContainerPoliciesDescription const & description,
    ServicePackageInstanceEnvironmentContextSPtr const & envContext)
{
    map<wstring, wstring> portBindings;
    for (auto iter = description.PortBindings.begin(); iter != description.PortBindings.end(); iter++)
    {
        auto internalPort = iter->ContainerPort;
        for (auto it = envContext->Endpoints.begin(); it != envContext->Endpoints.end(); it++)
        {
            if (StringUtility::AreEqualCaseInsensitive(iter->EndpointResourceRef, (*it)->Name))
            {
                //If containerport was specified as ), use the endpoint port as exposed port.
                if (internalPort == 0)
                {
                    internalPort = (*it)->Port;
                }
                auto containerProtocolType = ((*it)->Protocol == ProtocolType::Udp) ? ProtocolType::Udp : ProtocolType::Tcp;

                auto containerPort = wformatString("{0}/{1}", StringUtility::ToWString(internalPort), ProtocolType::EnumToString(containerProtocolType));
                portBindings.insert(make_pair(containerPort, StringUtility::ToWString((*it)->Port)));
                break;
            }
        }
    }
    return portBindings;
}

Common::ErrorCode VersionedServicePackage::IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent)
{
    auto codePackages = this->GetCodePackages();
    for (auto iter = codePackages.begin(); iter != codePackages.end(); ++iter)
    {
        isCodePackageLockFilePresent = !(*iter)->DebugParameters.LockFile.empty() && File::Exists((*iter)->DebugParameters.LockFile);
        if (isCodePackageLockFilePresent)
        {
            WriteInfo(
                TraceType,
                TraceId,
                "IsCodePackageLockFilePresent: CodePackage={0} has lock file present at {1}",
                (*iter)->CodePackageInstanceId,
                (*iter)->DebugParameters.LockFile);
            break;
        }
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr VersionedServicePackage::BeginTerminateFabricTypeHostCodePackage(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    int64 instanceId)
{
    return AsyncOperation::CreateAndStart<TerminateFabricTypeHostAsyncOperation>(
        *this,
        instanceId,
        callback,
        parent);
}

ErrorCode VersionedServicePackage::EndTerminateFabricTypeHostCodePackage(
    AsyncOperationSPtr const & operation,
    __out int64 & instanceId)
{
    return TerminateFabricTypeHostAsyncOperation::End(operation, instanceId);
}

AsyncOperationSPtr VersionedServicePackage::BeginRestartCodePackageInstance(
    wstring const & codePackageName,
    int64 instanceId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RestartCodePackageInstanceAsyncOperation>(
        *this,
        codePackageName,
        instanceId,
        timeout,
        callback,
        parent);
}

ErrorCode VersionedServicePackage::EndRestartCodePackageInstance(AsyncOperationSPtr const & operation)
{
    return RestartCodePackageInstanceAsyncOperation::End(operation);
}

void VersionedServicePackage::OnDockerHealthCheckUnhealthy(wstring const & codePackageName, int64 instanceId)
{
    auto operation = this->BeginRestartCodePackageInstance(
        codePackageName,
        instanceId,
        HostingConfig::GetConfig().ContainerTerminationTimeout,
        [this](AsyncOperationSPtr const & operation) { this->EndRestartCodePackageInstance(operation); },
        this->CreateAsyncOperationRoot());
}

ErrorCode VersionedServicePackage::TryGetPackageSecurityPrincipalInfo(
    __in std::wstring const & name,
    __out SecurityPrincipalInformationSPtr & info)
{
    if (this->ServicePackageObj.ApplicationEnvironment->PrincipalsContext != nullptr)
    {
        DigestedCodePackageDescription package;
        auto packages = this->PackageDescription.DigestedCodePackages;
        for (auto iter = packages.begin(); iter != packages.end(); ++iter)
        {
            if (StringUtility::AreEqualCaseInsensitive(iter->Name, name))
            {
                return this->ServicePackageObj.ApplicationEnvironment->PrincipalsContext->TryGetPrincipalInfo(iter->RunAsPolicy.UserRef, info);
            }
        }
    }

    return ErrorCodeValue::NotFound;
}
