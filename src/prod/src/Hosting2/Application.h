// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class Application2 : 
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(Application2)

        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        STATEMACHINE_STATES_10(Inactive, Activating, Active, Modifying_PackageUpgrade, Modifying_PackageActivationDeactivation, Analyzing, Upgrading, Deactivating, Deactivated, Failed);
        STATEMACHINE_REF_COUNTED_STATES(Modifying_PackageActivationDeactivation);
        STATEMACHINE_TRANSITION(Activating, Inactive);
        STATEMACHINE_TRANSITION(Active, Activating|Modifying_PackageUpgrade|Modifying_PackageActivationDeactivation|Upgrading|Analyzing);
        STATEMACHINE_TRANSITION(Modifying_PackageUpgrade, Active);
        STATEMACHINE_TRANSITION(Modifying_PackageActivationDeactivation, Active|Modifying_PackageActivationDeactivation);
        STATEMACHINE_TRANSITION(Upgrading, Active);
        STATEMACHINE_TRANSITION(Analyzing, Active);
        STATEMACHINE_TRANSITION(Deactivating, Active);
        STATEMACHINE_TRANSITION(Deactivated, Deactivating);
        STATEMACHINE_TRANSITION(Failed, Activating|Upgrading|Deactivating);
        STATEMACHINE_ABORTED_TRANSITION(Inactive|Active|Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Deactivated);

    public:
        Application2(
            HostingSubsystemHolder const & hostingHolder,
            ServiceModel::ApplicationIdentifier const & applicationId,
            std::wstring const & applicationName);
        virtual ~Application2();

        __declspec(property(get=get_Id)) ServiceModel::ApplicationIdentifier const & Id;
        ServiceModel::ApplicationIdentifier const & get_Id() const;

        __declspec(property(get=get_AppName)) std::wstring const & AppName;
        std::wstring const & get_AppName() const;

        __declspec(property(get=get_CurrentVersion)) ServiceModel::ApplicationVersion const CurrentVersion;
        ServiceModel::ApplicationVersion const get_CurrentVersion() const;

        bool IsActiveInVersion(ServiceModel::ApplicationVersion const & thisVersion) const;

        __declspec(property(get=get_InstanceId)) int64 const InstanceId;
        int64 const get_InstanceId() const;

        ServiceModel::DeployedApplicationQueryResult GetDeployedApplicationQueryResult();

        VersionedApplicationSPtr GetVersionedApplication() const;

        Common::ErrorCode IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent) const;

        void OnServiceTypesUnregistered(
            ServicePackageInstanceIdentifier const servicePackageInstanceId,
            std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds);

    public:
        //
        // activates specified versions of the service packages. 
        // if the service package is already active and ensureLatestVersion is false, version mismatch error is returned
        // if the service package is already active and ensureLatestVersion is true, 
        //   and if the desired version is lower than running version, it is allowed and no error is returned;
        //       if the desired version is higher than running version, the lower version is aborted and version mismatch error is returned
        //  
        Common::AsyncOperationSPtr BeginActivate(
            ServiceModel::ApplicationVersion const & applicationVersion,
            bool ensureLatestVersion,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndActivate(
            Common::AsyncOperationSPtr const & operation);

        // upgrades the application to the specified version
        // when an application package is upgraded all of the service packages are deactivated
        // service packages get activated again on-demand
        Common::AsyncOperationSPtr BeginUpgrade(
            ServiceModel::ApplicationVersion const & applicationVersion,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpgrade(
            Common::AsyncOperationSPtr const & operation);

        // deactivates the application and all of the associated service packages
        Common::AsyncOperationSPtr BeginDeactivate(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        void EndDeactivate(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginActivateServicePackageInstances(
            std::vector<ServicePackageInstanceOperation> && packageActivations,
            bool ensureLatestVersion,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndActivateServicePackages(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatusMapSPtr & statusMap);

        Common::AsyncOperationSPtr BeginDeactivateServicePackageInstances(
            std::vector<ServicePackageInstanceOperation> && packageDeactivations,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDeactivateServicePackages(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatusMapSPtr & statusMap);

        Common::AsyncOperationSPtr BeginUpgradeServicePackages(
            std::vector<ServicePackageOperation> && packageUpgrades,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndUpgradeServicePackages(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatusMapSPtr & statusMap);

        Common::ErrorCode AnalyzeApplicationUpgrade(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            __out CaseInsensitiveStringSet & affectedRuntimeIds);

        void OnServiceTypeRegistrationNotFound(
            uint64 const registrationTableVersion,
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId);

        void NotifyDcaAboutServicePackages();

    protected:
        void OnAbort();

    private:
        __declspec(property(get=get_HostingHolder)) HostingSubsystemHolder const & HostingHolder;
        HostingSubsystemHolder const & get_HostingHolder() const;

        Common::ErrorCode LoadApplicationPackageDescription(
            ServiceModel::ApplicationIdentifier const & appId,
            ServiceModel::ApplicationVersion const & appVersion,
            __out ServiceModel::ApplicationPackageDescription & appPackageDescription);

        uint64 GetActiveStatesMask() const;

    private:
        class ActivateAsyncOperation;
        class DeactivateAsyncOperation;
        class UpgradeAsyncOperation;

        template<typename OperationType>
        class ServicePackagesAsyncOperationBase;

        class ActivateServicePackageInstancesAsyncOperation;
        class DeactivateServicePackageInstancesAsyncOperation;

        class UpgradeServicePackagesAsyncOperation;

        friend class VersionedApplication;

    private:
        HostingSubsystemHolder const hostingHolder_;

        ServiceModel::ApplicationIdentifier const id_;
        std::wstring const appName_;
        int64 const instanceId_;
        ServiceModel::ApplicationVersion version__;
        ServiceModel::ApplicationPackageDescription packageDescription__;
        VersionedApplicationSPtr versionedApplication__;
        Management::ImageModel::RunLayoutSpecification const runLayout_;
    };
}
