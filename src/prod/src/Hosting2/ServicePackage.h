// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServicePackage2 : 
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ServicePackage2)

        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        STATEMACHINE_STATES_08(Inactive, Activating, Active, Deactivating, Deactivated, Upgrading, Analyzing, Failed);
        STATEMACHINE_TRANSITION(Activating, Inactive);
        STATEMACHINE_TRANSITION(Active, Activating|Upgrading|Analyzing);
        STATEMACHINE_TRANSITION(Upgrading, Active);
        STATEMACHINE_TRANSITION(Analyzing, Active);
        STATEMACHINE_TRANSITION(Deactivating, Active);
        STATEMACHINE_TRANSITION(Deactivated, Deactivating);
        STATEMACHINE_TRANSITION(Failed, Activating|Upgrading|Deactivating);
        STATEMACHINE_ABORTED_TRANSITION(Inactive|Active|Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Deactivated);

    public:
        ServicePackage2(
            HostingSubsystemHolder const & hostingHolder,
            ServiceModel::ServicePackageDescription const & packageDescription,
            ServicePackageInstanceContext && packageInstanceContext);
        virtual ~ServicePackage2();

        // activates the specified version of the service package
        Common::AsyncOperationSPtr BeginActivate(
            ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
            bool ensureLatestVersion,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndActivate(
            Common::AsyncOperationSPtr const & operation);

        // upgrades the service package to the specified version
        Common::AsyncOperationSPtr BeginUpgrade(
            ServiceModel::ServicePackageDescription const & packageDescription,
            ServiceModel::ServicePackageVersionInstance const & newPackageVersionInstance,
            ServiceModel::UpgradeType::Enum const upgradeType,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpgrade(
            Common::AsyncOperationSPtr const & operation);

        // deactivates the service package
        Common::AsyncOperationSPtr BeginDeactivate(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        void EndDeactivate(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode AnalyzeApplicationUpgrade(
            ServiceModel::ServicePackageVersionInstance const & newPackageVersionInstance,
            ServiceModel::UpgradeType::Enum const upgradeType,
            __out CaseInsensitiveStringSet & affectedRuntimeIds);

        ULONG GetFailureCount() const;

        Common::ErrorCode GetSharedPackageFolders(
            std::wstring const & applicationTypeName,
            std::vector<std::wstring> & sharedPackages);

        void OnServiceTypeRegistrationNotFound(
            uint64 const registrationTableVersion,
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId);

        void OnServiceTypesUnregistered(std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds);

        void NotifyDcaAboutServicePackages();

        Common::ErrorCode IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent) const;

    public:
        __declspec(property(get = get_ServicePackageName)) std::wstring const & ServicePackageName;
        std::wstring const & get_ServicePackageName() const;

        __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() const;

        __declspec(property(get = get_Id)) ServicePackageInstanceIdentifier const & Id;
        ServicePackageInstanceIdentifier const & get_Id() const;

        __declspec(property(get=get_InstanceId)) int64 const InstanceId;
        int64 const get_InstanceId() const;

        __declspec(property(get=get_CurrentVersionInstance)) ServiceModel::ServicePackageVersionInstance const & CurrentVersionInstance;
        ServiceModel::ServicePackageVersionInstance const & get_CurrentVersionInstance() const;

        __declspec(property(get = get_Description)) ServiceModel::ServicePackageDescription const & Description;
        ServiceModel::ServicePackageDescription const & get_Description() const { return packageDescription__; }

        ServiceModel::DeployedServiceManifestQueryResult GetDeployedServiceManifestQueryResult() const;

        VersionedServicePackageSPtr GetVersionedServicePackage() const;

    private:
        __declspec(property(get=get_ApplicationEnvironment)) ApplicationEnvironmentContextSPtr const & ApplicationEnvironment;
        ApplicationEnvironmentContextSPtr const & get_ApplicationEnvironment() const;        

        __declspec(property(get=get_HostingHolder)) HostingSubsystemHolder const & HostingHolder;
        HostingSubsystemHolder const & get_HostingHolder() const;
        
        Common::ErrorCode LoadServicePackageDescription(
            ServiceModel::ServicePackageVersion const & packageVersion,
            __out ServiceModel::ServicePackageDescription & packageDescription);   

        Common::ErrorCode GetServiceTypes(
            std::wstring const & manifestVersion,
            __out std::vector<ServiceTypeInstanceIdentifier> & serviceTypeIds);

    protected:
        void OnAbort();

    private:
        class ActivateAsyncOperation;
        class DeactivateAsyncOperation;
        class UpgradeAsyncOperation;
        friend class VersionedServicePackage;

    private:
        HostingSubsystemHolder const hostingHolder_;
        ServicePackageInstanceContext const context_;
        int64 const instanceId_;

        VersionedServicePackageSPtr versionedServicePackage__;
        ServiceModel::ServicePackageVersionInstance currentVersionInstance__;
        ServiceModel::ServicePackageDescription packageDescription__;

        Management::ImageModel::RunLayoutSpecification const runLayout_;

        static const int ServicePackageActivatedEventVersion = 0;
        static const int ServicePackageDeactivatedEventVersion = 0;
        static const int ServicePackageUpgradedEventVersion = 0;
    };
}
