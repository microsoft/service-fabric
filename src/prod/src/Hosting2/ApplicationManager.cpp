// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Management;

StringLiteral const TraceType("ApplicationManager");

// ********************************************************************************************************************
// ApplicationManager::OpenAsyncOperation Implementation
//

class ApplicationManager::OpenAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        ApplicationManager & owner,
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

    static ErrorCode OpenAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.applicationMap_->Open();
        OpenEnvironmentManager(thisSPtr);
    }

private:
    void OpenEnvironmentManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "Begin(OpenEnvironmentManager)");
        auto operation = owner_.environmentManager_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->FinishOpenEnvironmentManager(operation, false); },
            thisSPtr);
        FinishOpenEnvironmentManager(operation, true);
    }

    void FinishOpenEnvironmentManager(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.environmentManager_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(OpenEnvironmentManager): error={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        OpenActivator(operation->Parent);
    }

    void OpenActivator(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.activator_->Open();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenActivator: error={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        OpenDeactivator(thisSPtr);
    }

    void OpenDeactivator(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.deactivator_->Open();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "OpenDeactivator: error={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        ApplicationManager & appManager = owner_;
        IFabricActivatorClientSPtr activatorClient = this->owner_.hosting_.FabricActivatorClientObj;

        //Register handler to handle process termination events.
        auto terminationHandler = owner_.hosting_.FabricActivatorClientObj->AddRootContainerTerminationHandler(
            [&appManager](EventArgs const& eventArgs)
        {
            appManager.OnRootContainerTerminated(eventArgs);
        });

        owner_.terminationNotificationHandler_ = make_unique<ResourceHolder<HHandler>>(
            move(terminationHandler),
            [activatorClient](ResourceHolder<HHandler> * thisPtr)
        {
            activatorClient->RemoveRootContainerTerminationHandler(thisPtr->Value);
        });
        TryComplete(thisSPtr, error);
    }

private:
    ApplicationManager & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// ApplicationManager::CloseAsyncOperation Implementation
//

class ApplicationManager::CloseAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        ApplicationManager & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        appCount_(0),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static ErrorCode CloseAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        CloseDeactivator(thisSPtr);
    }

private:
    void CloseDeactivator(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.deactivator_->Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseDeactivator: error={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        DeactivateApplications(thisSPtr);
    }

    void DeactivateApplications(AsyncOperationSPtr const & thisSPtr)
    {
        vector<Application2SPtr> applications = owner_.applicationMap_->Close();
        appCount_.store(applications.size());

        for(auto iter = applications.begin(); iter != applications.end(); ++iter)
        {
            auto application = (*iter);
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "Begin(DeactivateApplication): Id={0}",
                application->Id);

            auto operation = application->BeginDeactivate(
                timeoutHelper_.GetRemainingTime(),
                [this, application](AsyncOperationSPtr const & operation){ this->FinishDeactivateApplication(application, operation, false); },
                thisSPtr);
            this->FinishDeactivateApplication(application, operation, true);
        }

        CheckPendingDeactivations(thisSPtr, applications.size());
    }

    void FinishDeactivateApplication(
        Application2SPtr const & application,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        application->EndDeactivate(operation);
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "End(DeactivateApplication): Id={0}.",
            application->Id);

        uint64 currentAppCount = --appCount_;
        CheckPendingDeactivations(operation->Parent, currentAppCount);
    }

    void CheckPendingDeactivations(AsyncOperationSPtr const & thisSPtr, uint64 currentAppCount)
    {
        if (currentAppCount == 0)
        {
            CloseActivator(thisSPtr);
        }
    }

    void CloseActivator(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.activator_->Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CloseActivator: error={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CloseEnvironmentManager(thisSPtr);
    }

    void CloseEnvironmentManager(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "Begin(CloseEnvironmentManager)");
        auto operation = owner_.environmentManager_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnCloseEnvironmentManagerCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishCloseEnvironmentManager(operation);
        }
    }

    void OnCloseEnvironmentManagerCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCloseEnvironmentManager(operation);
        }
    }

    void FinishCloseEnvironmentManager(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.environmentManager_->EndClose(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(CloseEnvironmentManager): ErrorCode={0}",
            error);

        TryComplete(operation->Parent, error);
    }

private:
    ApplicationManager & owner_;
    atomic_uint64 appCount_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// ApplicationManager::DownloadAndActivateAsyncOperation Implementation
//
class ApplicationManager::DownloadAndActivateAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DownloadAndActivateAsyncOperation)

public:
    DownloadAndActivateAsyncOperation(
        ApplicationManager & owner,
        uint64 const sequenceNumber,
        VersionedServiceTypeIdentifier const & versionedServiceTypeId,
        ServicePackageActivationContext const & activationContext,
        wstring const & servicePackagePublicActivationId,
        wstring const & applicationName,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        sequenceNumber_(sequenceNumber),
        versionedServiceTypeId_(versionedServiceTypeId),
        servicePackageInstanceId_(versionedServiceTypeId.Id.ServicePackageId, activationContext, servicePackagePublicActivationId),
        serviceTypeInstanceId_(versionedServiceTypeId.Id, activationContext, servicePackagePublicActivationId),
        applicationName_(applicationName),
        application_()
    {
    }

    virtual ~DownloadAndActivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadAndActivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        DownloadAndActivateApplication(thisSPtr);
    }

private:
    void DownloadAndActivateApplication(AsyncOperationSPtr const & thisSPtr)
    {
         // check if the application object is already present in the application map
        Application2SPtr application;
        auto error = owner_.applicationMap_->Get(versionedServiceTypeId_.Id.ApplicationId, application);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::ApplicationManagerApplicationNotFound))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "ApplicationMap.Contains: ApplicationId={0}, ErrorCode={1}",
                versionedServiceTypeId_.Id.ApplicationId,
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        if ((application != nullptr) && (application->IsActiveInVersion(versionedServiceTypeId_.servicePackageVersionInstance.Version.ApplicationVersionValue)))
        {
            // if application of the same version is already running, start service package download and activation
            application_ = application;
            DownloadAndActivateServicePackage(thisSPtr);
        }
        else
        {
            DownloadApplication(thisSPtr);
        }
    }

    void DownloadApplication(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.hosting_.DownloadManagerObj->BeginDownloadApplicationPackage(
            versionedServiceTypeId_.Id.ApplicationId,
            versionedServiceTypeId_.servicePackageVersionInstance.Version.ApplicationVersionValue,
            serviceTypeInstanceId_.ActivationContext,
            applicationName_,
            [this](AsyncOperationSPtr const & operation){ this->FinishDownloadApplication(operation, false); },
            thisSPtr);
        this->FinishDownloadApplication(operation, true);
    }

    void FinishDownloadApplication(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        OperationStatus downloadStatus;
        ApplicationPackageDescription appPackageDescription;
        auto error = owner_.hosting_.DownloadManagerObj->EndDownloadApplicationPackage(operation, downloadStatus, appPackageDescription);
        if (!error.IsSuccess())
        {
            if (downloadStatus.FailureCount > (ULONG)HostingConfig::GetConfig().ServiceTypeDisableFailureThreshold)
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "Disabling ServiceType for Application: {0}, ServiceTypeInstanceId: {1}, DownloadStatusId: {2}",
                    applicationName_,
                    serviceTypeInstanceId_,
                    downloadStatus.Id);

                owner_.hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->Disable(
                    serviceTypeInstanceId_,
                    applicationName_,
                    downloadStatus.Id);
            }

            TryComplete(operation->Parent, error);
            return;
        }

        ActivateApplication(operation->Parent);
    }

    void ActivateApplication(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.activator_->BeginActivateApplication(
            versionedServiceTypeId_.Id.ApplicationId,
            versionedServiceTypeId_.servicePackageVersionInstance.Version.ApplicationVersionValue,
            applicationName_,
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

        OperationStatus activationStatus;
        auto error = owner_.activator_->EndActivateApplication(operation, activationStatus, application_);
        if (!error.IsSuccess())
        {
            if ((!error.IsError(ErrorCodeValue::HostingApplicationVersionMismatch)) &&
                (activationStatus.FailureCount > (ULONG)HostingConfig::GetConfig().ServiceTypeDisableFailureThreshold))
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "Disabling ServiceType for Application: {0}, ServiceTypeInstanceId: {1}, ActivationStatusId: {2}",
                    applicationName_,
                    serviceTypeInstanceId_,
                    activationStatus.Id);

                owner_.hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->Disable(
                    serviceTypeInstanceId_,
                    applicationName_,
                    activationStatus.Id);
            }

            TryComplete(operation->Parent, error);
            return;
        }

        DownloadAndActivateServicePackage(operation->Parent);
    }

    void DownloadAndActivateServicePackage(AsyncOperationSPtr const & thisSPtr)
    {
        bool shouldSkipPackageDownload = false;
        auto versionedApplication = application_->GetVersionedApplication();
        if (versionedApplication)
        {
            ServicePackage2SPtr servicePackage;
            auto error = versionedApplication->GetServicePackageInstance(
                servicePackageInstanceId_.ServicePackageName,
                versionedServiceTypeId_.servicePackageVersionInstance.Version,
                servicePackage);
            if(!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
            {
                TryComplete(thisSPtr, error);
                return;
            }

            if (servicePackage != nullptr)
            {
                shouldSkipPackageDownload = true;
            }
        }

        if (shouldSkipPackageDownload)
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "Skipping service package download for {0}",
                servicePackageInstanceId_);

            ActivateServicePackageInstance(thisSPtr, nullptr);
        }
        else
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "Downloading service package download for {0}",
                servicePackageInstanceId_);

            DownloadServicePackage(thisSPtr);
        }
    }

    void DownloadServicePackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.hosting_.DownloadManagerObj->BeginDownloadServicePackage(
            versionedServiceTypeId_.Id.ServicePackageId,
            versionedServiceTypeId_.servicePackageVersionInstance.Version,
            servicePackageInstanceId_.ActivationContext,
            applicationName_,
            [this](AsyncOperationSPtr const & operation){ this->FinishDownloadServicePackage(operation, false); },
            thisSPtr);
        this->FinishDownloadServicePackage(operation, true);
    }

    void FinishDownloadServicePackage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        OperationStatus downloadStatus;
        shared_ptr<ServicePackageDescription> servicePackageDescriptionPtr = make_shared<ServicePackageDescription>();
        auto error = owner_.hosting_.DownloadManagerObj->EndDownloadServicePackage(
            operation,
            downloadStatus,
            *servicePackageDescriptionPtr);
        if (!error.IsSuccess())
        {
            if (downloadStatus.FailureCount > (ULONG)HostingConfig::GetConfig().ServiceTypeDisableFailureThreshold)
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "Disabling ServiceType for Application: {0}, ServiceTypeInstanceId: {1}, DownloadStatusId: {2}",
                    applicationName_,
                    serviceTypeInstanceId_,
                    downloadStatus.Id);

                owner_.hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->Disable(
                    serviceTypeInstanceId_,
                    applicationName_,
                    downloadStatus.Id);
            }

            TryComplete(operation->Parent, error);
            return;
        }

        ActivateServicePackageInstance(operation->Parent, servicePackageDescriptionPtr);
    }

    void ActivateServicePackageInstance(AsyncOperationSPtr const & thisSPtr, shared_ptr<ServicePackageDescription> const & servicePackageDescriptionPtr)
    {
        auto operation = owner_.activator_->BeginActivateServicePackageInstance(
            application_,
            servicePackageInstanceId_,
            versionedServiceTypeId_.servicePackageVersionInstance,
            servicePackageDescriptionPtr,
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

        OperationStatus activationStatus;
        auto error = owner_.activator_->EndActivateServicePackageInstance(
            operation,
            activationStatus);
        if (!error.IsSuccess())
        {
            if ((!error.IsError(ErrorCodeValue::HostingServicePackageVersionMismatch)) &&
                (activationStatus.FailureCount > (ULONG)HostingConfig::GetConfig().ServiceTypeDisableFailureThreshold))
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "Disabling ServiceType for Application: {0}, ServiceTypeInstanceId: {1}, ActivationStatusId: {2}",
                    applicationName_,
                    serviceTypeInstanceId_,
                    activationStatus.Id);

                owner_.hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->Disable(
                    serviceTypeInstanceId_,
                    applicationName_,
                    activationStatus.Id);
            }

            TryComplete(operation->Parent, error);
            return;
        }
        else
        {
            application_->OnServiceTypeRegistrationNotFound(
                sequenceNumber_,
                versionedServiceTypeId_,
                servicePackageInstanceId_);

            TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
        }
    }

private:
    ApplicationManager & owner_;
    uint64 const sequenceNumber_;
    VersionedServiceTypeIdentifier const versionedServiceTypeId_;
    ServicePackageInstanceIdentifier const servicePackageInstanceId_;
    ServiceTypeInstanceIdentifier const serviceTypeInstanceId_;
    wstring const applicationName_;
    Application2SPtr application_;
};

// ********************************************************************************************************************
// ApplicationManager::UpgradeApplicationAsyncOperation Implementation
//
class ApplicationManager::UpgradeApplicationAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UpgradeApplicationAsyncOperation)

public:
    UpgradeApplicationAsyncOperation(
        ApplicationManager & owner,
        ApplicationUpgradeSpecification const & upgradeSpecification,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        upgradeSpecification_(upgradeSpecification),
        timeoutHelper_(timeout),
        application_(),
        servicePackageDescriptionMap_(),
        pendingActivationCount_(0),
        lastActivationError_(ErrorCodeValue::Success),
        activationLock_()
    {
    }

    virtual ~UpgradeApplicationAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpgradeApplicationAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            // ensure that the application and service packages are activated in the right version
            // as we complete the upgrade successfully when older application and services packages
            // are not active anymore. This means that if there are existing replicas on the node
            // we need to ensure the activation of the new version.

            thisPtr->owner_.ActivatorObj->EnsureActivationAsync(thisPtr->upgradeSpecification_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.applicationMap_->Get(upgradeSpecification_.ApplicationId, application_);
        if(error.IsSuccess())
        {
            UpgradeApplication(thisSPtr);
            return;
        }
        else if (error.IsError(ErrorCodeValue::ApplicationManagerApplicationNotFound))
        {
            // complete the upgrade, the application and services packages will be activated by RA on find
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }
        else
        {
            // failed to perform find operation on the map
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "GetApplication: ApplicationId={0}, ErrorCode={1}",
                upgradeSpecification_.ApplicationId,
                error);
            TryComplete(thisSPtr, error);
            return;
        }
    }

private:
    void UpgradeApplication(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(UpgradeApplication): Timeout={0}, ApplicationId={1}, CurrentVersion={2}, NewVersion={3}",
            timeout,
            upgradeSpecification_.ApplicationId,
            application_->CurrentVersion,
            upgradeSpecification_.AppVersion);

        // Since all application Upgrades changes major version, an upgrade
        // will cause a restart. Hence we are not passing the UpgradeType flag.
        // Once we support Switch in application Upgrade, we would need to pass
        // this flag.
        // TODO: Pass the UpgradeType flag when implementing Switch
        auto operation = application_->BeginUpgrade(
            upgradeSpecification_.AppVersion,
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->FinishUpgradeApplication(operation, false); },
            thisSPtr);
        FinishUpgradeApplication(operation, true);
    }

    void FinishUpgradeApplication(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = application_->EndUpgrade(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            owner_.Root.TraceId,
            "End(UpgradeApplication): ErrorCode={0}, ApplicationId={1}",
            error,
            upgradeSpecification_.ApplicationId);

        if(!error.IsSuccess())
        {
            CompleteUpgrade(operation->Parent, error);
            return;
        }

        // application has been upgraded, upgrade all services packages
        UpgradePackages(operation->Parent);
    }

    void UpgradePackages(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Begin(UpgradeServicePackages): Timeout={0}, ApplicationId={1}",
            timeout,
            upgradeSpecification_.ApplicationId);

        vector<ServicePackageOperation> packageUpgrades;
        for (auto iter = upgradeSpecification_.PackageUpgrades.begin();  iter != upgradeSpecification_.PackageUpgrades.end(); ++iter)
        {
            ServicePackageDescription packageDescription;
            auto error = owner_.LoadServicePackageDescription(
                upgradeSpecification_.ApplicationId,
                iter->ServicePackageName,
                iter->RolloutVersionValue,
                packageDescription);
            if(!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }

            servicePackageDescriptionMap_.insert(make_pair(packageDescription.ManifestName, packageDescription));

            packageUpgrades.push_back(
                ServicePackageOperation(
                iter->ServicePackageName,
                iter->RolloutVersionValue,
                upgradeSpecification_.InstanceId,
                packageDescription,
                upgradeSpecification_.UpgradeType));
        }

        auto operation = application_->BeginUpgradeServicePackages(
            move(packageUpgrades),
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishUpgradeServicePackages(operation, false); },
            thisSPtr);
        FinishUpgradeServicePackages(operation, true);
    }

    void FinishUpgradeServicePackages(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        OperationStatusMapSPtr statusMap;
        auto error = application_->EndUpgradeServicePackages(operation, statusMap);
        if (error.IsSuccess())
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "End(UpgradeServicePackages): ErrorCode={0}, ApplicationId={1}, Status={2}",
                error,
                upgradeSpecification_.ApplicationId,
                *statusMap);
        }
        else
        {
            if (statusMap != nullptr)
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "End(UpgradeServicePackages): ErrorCode={0}, ApplicationId={1}, Status={2}",
                    error,
                    upgradeSpecification_.ApplicationId,
                    *statusMap);
            }
            else
            {
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "End(UpgradeServicePackages): ErrorCode={0}, ApplicationId={1}",
                    error,
                    upgradeSpecification_.ApplicationId);
            }
        }

        CompleteUpgrade(operation->Parent, error);
    }

    void CompleteUpgrade(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        if(!error.IsSuccess() && application_->GetState() == Application2::Failed)
        {
            application_->AbortAndWaitForTerminationAsync(
                [this, error](AsyncOperationSPtr const & abortOperation)
            {
                // abort the failed application, remove it and complete upgrade with error
                // RA will retry the Upgrade and once finished it will start the necessary
                // package by performing find

                Application2SPtr removed;
                auto removeError = owner_.applicationMap_->Remove(application_->Id, application_->InstanceId, removed);
                WriteTrace(
                    removeError.ToLogLevel(),
                    TraceType,
                    owner_.Root.TraceId,
                    "RemoveApplication: ErrorCode={0}, ApplicationId={1}",
                    removeError,
                    application_->Id);

                TryComplete(abortOperation->Parent, error);
            },
                thisSPtr);

            return;
        }

        TryComplete(thisSPtr, error);
    }

private:
    ApplicationManager & owner_;
    ApplicationUpgradeSpecification const upgradeSpecification_;
    TimeoutHelper timeoutHelper_;
    Application2SPtr application_;
    map<wstring, ServicePackageDescription> servicePackageDescriptionMap_;

    // Used for ServicePackage activation
    atomic_uint64 pendingActivationCount_;
    ErrorCode lastActivationError_;
    RwLock activationLock_;
};

// ********************************************************************************************************************
// ApplicationManager Implementation
//
ApplicationManager::ApplicationManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    hosting_(hosting),
    applicationMap_(),
    environmentManager_(),
    activator_(),
    deactivator_()
{
    auto applicationMap = make_unique<ApplicationMap>(root);
    auto environmentManager = make_unique<EnvironmentManager>(root, hosting_);
    auto activator = make_unique<Activator>(root, *this);
    auto deactivator = make_unique<Deactivator>(root, *this);

    activator_ = move(activator);
    deactivator_ = move(deactivator);
    applicationMap_ = move(applicationMap);
    environmentManager_ = move(environmentManager);
}

ApplicationManager::~ApplicationManager()
{
    WriteNoise(TraceType, Root.TraceId, "ApplicationManager.destructor");
}

AsyncOperationSPtr ApplicationManager::OnBeginOpen(
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

ErrorCode ApplicationManager::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ApplicationManager::OnBeginClose(
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

ErrorCode ApplicationManager::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void ApplicationManager::OnAbort()
{
    deactivator_->Abort();

    vector<Application2SPtr> applications = applicationMap_->Close();
    for(auto iter = applications.begin(); iter != applications.end(); ++iter)
    {
        (*iter)->AbortAndWaitForTermination();
    }

    activator_->Abort();
    environmentManager_->Abort();
}

void ApplicationManager::OnRootContainerTerminated(
    EventArgs const & eventArgs)
{
    ApplicationHostTerminatedEventArgs args = dynamic_cast<ApplicationHostTerminatedEventArgs const &>(eventArgs);
    ServicePackageInstanceIdentifier spId;
    //The hostId for root container is same as servicepackageinstanceId.
    auto error = ServicePackageInstanceIdentifier::FromString(args.HostId, spId);
    if (error.IsSuccess())
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Root container terminated unexpectedly, deactivating servicepackage Id={0}",
            spId);
        DeactivateServicePackageInstance(spId);
    }
    else
    {
        WriteWarning(
            TraceType,
            Root.TraceId, 
            "Failed to convert {0} to ServicePackageInstanceId error {1}",
            args.HostId,
            error);
    }
}

AsyncOperationSPtr ApplicationManager::BeginDownloadAndActivate(
    uint64 const sequenceNumber,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageActivationContext const & activationContext,
    wstring const & servicePackagePublicActivationId,
    wstring const & applicationName,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadAndActivateAsyncOperation>(
        *this,
        sequenceNumber,
        versionedServiceTypeId,
        activationContext,
        servicePackagePublicActivationId,
        applicationName,
        callback,
        parent);
}

ErrorCode ApplicationManager::EndDownloadAndActivate(
    AsyncOperationSPtr const & operation)
{
    return DownloadAndActivateAsyncOperation::End(operation);
}

AsyncOperationSPtr ApplicationManager::BeginDownloadApplication(
    ApplicationDownloadSpecification const & appDownloadSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return hosting_.DownloadManagerObj->BeginDownloadApplication(
        appDownloadSpec,
        callback,
        parent);
}

ErrorCode ApplicationManager::EndDownloadApplication(
    AsyncOperationSPtr const & operation,
    __out OperationStatusMapSPtr & appDownloadStatus)
{
    return hosting_.DownloadManagerObj->EndDownloadApplication(
        operation,
        appDownloadStatus);
}


AsyncOperationSPtr ApplicationManager::BeginUpgradeApplication(
    ApplicationUpgradeSpecification const & appUpgradeSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpgradeApplicationAsyncOperation>(
        *this,
        appUpgradeSpec,
        HostingConfig::GetConfig().ApplicationUpgradeTimeout,
        callback,
        parent);
}

ErrorCode ApplicationManager::EndUpgradeApplication(AsyncOperationSPtr const & operation)
{
    return UpgradeApplicationAsyncOperation::End(operation);
}

void ApplicationManager::ScheduleForDeactivationIfNotUsed(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    bool & hasReplica)
{
    bool isDeactivationScheduled = false;

    ErrorCode error = this->deactivator_->ScheduleDeactivationIfNotUsed(servicePackageInstanceId, isDeactivationScheduled);
    if(error.IsSuccess())
    {
        hasReplica = !isDeactivationScheduled;
    }
    else
    {
        hasReplica = false;
    }
}

void ApplicationManager::ScheduleForDeactivationIfNotUsed(ApplicationIdentifier const & applicationId, bool & hasReplica)
{
    bool isDeactivationScheduled = false;

    ErrorCode error = this->deactivator_->ScheduleDeactivationIfNotUsed(applicationId, isDeactivationScheduled);
    if(error.IsSuccess())
    {
        hasReplica = !isDeactivationScheduled;
    }
    else
    {
        hasReplica = false;
    }
}

void ApplicationManager::DeactivateServicePackageInstance(ServicePackageInstanceIdentifier servicePackageInstanceId)
{
    Application2SPtr application;
    auto error = this->applicationMap_->Get(servicePackageInstanceId.ApplicationId, application);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "DeactivateIfNotUsed: GetApplication failed for servicePackageInstanceId={0} with error {1}.",
            servicePackageInstanceId,
            error);

        if(error.IsError(ErrorCodeValue::ApplicationManagerApplicationNotFound))
        {
            deactivator_->OnDeactivationSuccess(servicePackageInstanceId);
        }

        return;
    }

    // deactivate the service package
    vector<ServicePackageInstanceOperation> packageDeactivations;
    packageDeactivations.push_back(
        ServicePackageInstanceOperation(
            servicePackageInstanceId.ServicePackageName,
            servicePackageInstanceId.ActivationContext,
            servicePackageInstanceId.PublicActivationId,
            RolloutVersion::Zero,
            0,
            ServicePackageDescription()));

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Begin(DeactivateServicePackageIntances): servicePackageInstanceId={0}.",
        servicePackageInstanceId,
        error);

    auto operation = application->BeginDeactivateServicePackageInstances(
        move(packageDeactivations),
        HostingConfig::GetConfig().ActivationTimeout,
        [this, application, servicePackageInstanceId](AsyncOperationSPtr const & operation)
    {
        this->FinishDeactivateServicePackageInstance(application, servicePackageInstanceId, operation, false);
    },
        Root.CreateAsyncOperationRoot());
    FinishDeactivateServicePackageInstance(application, servicePackageInstanceId, operation, true);
}

bool ApplicationManager::IsCodePackageLockFilePresent(ServicePackageInstanceIdentifier const & servicePackageInstanceId) const
{
    Application2SPtr application;
    auto error = this->applicationMap_->Get(servicePackageInstanceId.ApplicationId, application);
    if(!error.IsSuccess())
    {
        return false;
    }

    bool isCodePackageLockFilePresent = false;
    error = application->IsCodePackageLockFilePresent(isCodePackageLockFilePresent);
    if(!error.IsSuccess())
    {
        return false;
    }

    return isCodePackageLockFilePresent;
}

void ApplicationManager::FinishDeactivateServicePackageInstance(
    Application2SPtr const & application,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynhronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    OperationStatusMapSPtr statusMap;
    auto error = application->EndDeactivateServicePackages(operation, statusMap);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "End(DeactivateServicePackageInstance): ServicePackageInstance={0}, Error={1}.",
        servicePackageInstanceId,
        error);

    if(error.IsSuccess())
    {
        this->deactivator_->OnDeactivationSuccess(servicePackageInstanceId);
    }
    else
    {
        this->deactivator_->OnDeactivationFailed(servicePackageInstanceId);
    }
}

void ApplicationManager::DeactivateApplication(ApplicationIdentifier applicationId)
{
    Application2SPtr application;
    auto error = this->applicationMap_->Get(applicationId, application);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "DeactivateApplication: GetApplication failed for Id={0} with error {1}.",
            applicationId,
            error);

        if(error.IsError(ErrorCodeValue::ApplicationManagerApplicationNotFound))
        {
            deactivator_->OnDeactivationSuccess(applicationId);
        }

        return;
    }

    auto appDeactivateOperation = application->BeginDeactivate(
        HostingConfig::GetConfig().ActivationTimeout,
        [this, application](AsyncOperationSPtr const & operation)
    {
        this->FinishDeactivateApplication(application, operation, false);
    },
        Root.CreateAsyncOperationRoot());
    FinishDeactivateApplication(application, appDeactivateOperation, true);

}

void ApplicationManager::FinishDeactivateApplication(
    Application2SPtr const & application,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynhronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    application->EndDeactivate(operation);
    WriteNoise(
        TraceType,
        Root.TraceId,
        "FinishDeactivateApplication: Deactivation completed for {0}:{1}.",
        application->Id,
        application->InstanceId);

    this->deactivator_->OnDeactivationSuccess(application->Id);

    Application2SPtr removed;
    this->applicationMap_->Remove(application->Id, application->InstanceId, removed).ReadValue();
}

ErrorCode ApplicationManager::AnalyzeApplicationUpgrade(
    ApplicationUpgradeSpecification const & appUpgradeSpec,
    __out set<std::wstring, IsLessCaseInsensitiveComparer<wstring>> & affectedRuntimeIds)
{
    Application2SPtr application;
    auto error = this->applicationMap_->Get(appUpgradeSpec.ApplicationId, application);

    if(error.IsError(ErrorCodeValue::ApplicationManagerApplicationNotFound))
    {
        return ErrorCodeValue::Success;
    }

    if(!error.IsSuccess())
    {
        return error;
    }

    return application->AnalyzeApplicationUpgrade(appUpgradeSpec, affectedRuntimeIds);
}

ErrorCode ApplicationManager::EnsureServicePackageInstanceEntry(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    return this->deactivator_->EnsureServicePackageInstanceEntry(servicePackageInstanceId);
}

ErrorCode ApplicationManager::EnsureApplicationEntry(ApplicationIdentifier const & applicationId)
{
    return this->deactivator_->EnsureApplicationEntry(applicationId);
}

ErrorCode ApplicationManager::IncrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    auto error = this->deactivator_->IncrementUsageCount(servicePackageInstanceId);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "ApplicationManager::IncrementUsageCount: ServicePackageInstanId={0}, Error={1}.",
        servicePackageInstanceId,
        error);

    return error;
}

void ApplicationManager::DecrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    auto error = this->deactivator_->DecrementUsageCount(servicePackageInstanceId);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "ApplicationManager::DecrementUsageCount: ServicePackageInstanceId={0}, Error={1}.",
        servicePackageInstanceId,
        error);
}

ErrorCode ApplicationManager::LoadServicePackageDescription(
    ApplicationIdentifier const & appId,
    wstring const & servicePackageName,
    RolloutVersion const & packageRolloutVersion,
    __out ServicePackageDescription & packageDescription)
{
    wstring packageFilePath = hosting_.RunLayout.GetServicePackageFile(
        appId.ToString(),
        servicePackageName,
        packageRolloutVersion.ToString());

    if(!File::Exists(packageFilePath))
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "LoadServicePackageDescription: The ServicePackage file is not found at '{0}'.",
            packageFilePath);
        return ErrorCodeValue::FileNotFound;
    }

    auto error = Parser::ParseServicePackage(
        packageFilePath,
        packageDescription);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "LoadServicePackageDescription: Id={0}, Version={1}, ErrorCode={2}",
        ServicePackageIdentifier(appId, servicePackageName),
        packageRolloutVersion,
        error);

    return error;
}

ErrorCode ApplicationManager::GetApplications(__out std::vector<Application2SPtr> & applications, wstring const & filterApplicationName, wstring const & continuationToken)
{
    return this->applicationMap_->GetApplications(applications, filterApplicationName, continuationToken);
}

ErrorCode ApplicationManager::Contains(ServiceModel::ApplicationIdentifier const & appId, bool & contains)
{
    return this->applicationMap_->Contains(appId, contains);
}

void ApplicationManager::OnServiceTypesUnregistered(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
{
    if (serviceTypeInstanceIds.size() == 0)
    {
        return;
    }

    Application2SPtr application;
    auto error = this->applicationMap_->Get(servicePackageInstanceId.ApplicationId, application);
    if (error.IsSuccess())
    {
        application->OnServiceTypesUnregistered(servicePackageInstanceId, serviceTypeInstanceIds);
    }
}

bool ApplicationManager::ShouldReportHealth(ServicePackageInstanceIdentifier const & servicePackageInstanceId) const
{
    // Donot report health if the Application/ServicePackage is in any of the terminal states or going under deactivation.

    Application2SPtr application;
    auto error = this->applicationMap_->Get(servicePackageInstanceId.ApplicationId, application);
    if (!error.IsSuccess())
    {
        return false;
    }

    uint64 currentState = application->GetState();
    if ((currentState & (Application2::Deactivating | Application2::Deactivated | Application2::Aborted | Application2::Failed)) != 0)
    {
        return false;
    }

    VersionedApplicationSPtr versionApplication = application->GetVersionedApplication();
    if (!versionApplication)
    {
        return false;
    }

    ServicePackage2SPtr servicePackage;
    error = versionApplication->GetServicePackageInstance(servicePackageInstanceId, servicePackage);
    if (!error.IsSuccess())
    {
        return false;
    }

    currentState = servicePackage->GetState();
    if ((currentState & (ServicePackage2::Deactivating | ServicePackage2::Deactivated | ServicePackage2::Aborted | ServicePackage2::Failed)) != 0)
    {
        return false;
    }

    return true;
}
