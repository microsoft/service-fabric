// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationManager :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ApplicationManager)

    public:
        ApplicationManager(Common::ComponentRoot const & root, __in HostingSubsystem & hosting);
        virtual ~ApplicationManager();

        Common::AsyncOperationSPtr BeginDownloadAndActivate(
            uint64 const sequenceNumber, // type registration sequence number
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & servicePackagePublicActivationId,
            std::wstring const & applicationName,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDownloadAndActivate(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDownloadApplication(
            ApplicationDownloadSpecification const & appDownloadSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDownloadApplication(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatusMapSPtr & appDownloadStatus);

        Common::AsyncOperationSPtr BeginUpgradeApplication(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndUpgradeApplication(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode AnalyzeApplicationUpgrade(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            __out CaseInsensitiveStringSet & affectedRuntimeIds);

        void ScheduleForDeactivationIfNotUsed(ServicePackageInstanceIdentifier const & servicePackageInstanceId, bool & hasReplica);
        void ScheduleForDeactivationIfNotUsed(ServiceModel::ApplicationIdentifier const & applicationId, bool & hasReplica);

        Common::ErrorCode EnsureServicePackageInstanceEntry(ServicePackageInstanceIdentifier const & servicePackageInstanceId);
        Common::ErrorCode EnsureApplicationEntry(ServiceModel::ApplicationIdentifier const & applicationId);

        Common::ErrorCode IncrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId);

        void DecrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId);

        Common::ErrorCode Contains(ServiceModel::ApplicationIdentifier const & appId, bool & contains);

        // Note: The continuation token is expected to be the application name
        Common::ErrorCode GetApplications(__out std::vector<Application2SPtr> & applications, std::wstring const & filterApplicationName = L"", std::wstring const & continuationToken = L"");

        bool IsCodePackageLockFilePresent(ServicePackageInstanceIdentifier const & servicePackageInstanceId) const;

        bool ShouldReportHealth(ServicePackageInstanceIdentifier const & ServicePackageInstanceId) const;

        void OnServiceTypesUnregistered(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds);

        __declspec(property(get=get_EnvironmentManagerObj)) EnvironmentManagerUPtr const & EnvironmentManagerObj;
        EnvironmentManagerUPtr const & get_EnvironmentManagerObj() { return this->environmentManager_; }

        __declspec(property(get=get_ActivatorObj)) ActivatorUPtr const & ActivatorObj;
        ActivatorUPtr const & get_ActivatorObj() { return this->activator_; }

        __declspec(property(get=get_DeactivatorObj)) DeactivatorUPtr const & DeactivatorObj;
        DeactivatorUPtr const & get_DeactivatorObj() { return this->deactivator_; }

    protected:
        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & asyncOperation);

        void OnAbort();

    private:

        void DeactivateServicePackageInstance(ServicePackageInstanceIdentifier servicePackageInstanceId);
        void FinishDeactivateServicePackageInstance(
            Application2SPtr const & application,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynhronously);

        void DeactivateApplication(ServiceModel::ApplicationIdentifier applicationId);
        void FinishDeactivateApplication(
            Application2SPtr const & application,
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynhronously);

        Common::ErrorCode ApplicationManager::LoadServicePackageDescription(
            ServiceModel::ApplicationIdentifier const & appId,
            std::wstring const & servicePackageName,
            ServiceModel::RolloutVersion const & packageRolloutVersion,
            __out ServiceModel::ServicePackageDescription & packageDescription);

        void OnRootContainerTerminated(Common::EventArgs const & eventArgs);

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class DownloadAndActivateAsyncOperation;
        class UpgradeApplicationAsyncOperation;
        friend class Activator;
        friend class Deactivator;

    private:
        HostingSubsystem & hosting_;
        ApplicationMapUPtr applicationMap_;
        EnvironmentManagerUPtr environmentManager_;
        std::unique_ptr<Common::ResourceHolder<Common::HHandler>> terminationNotificationHandler_;
        ActivatorUPtr activator_;
        DeactivatorUPtr deactivator_;
    };
}
