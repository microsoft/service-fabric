// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class VersionedApplication :
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(VersionedApplication)

        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        STATEMACHINE_STATES_10(Created, Opening, Opened, Upgrading, Modifying_PackageUpgrade, Modifying_PackageActivationDeactivation, Analyzing, Closing, Closed, Failed);
        STATEMACHINE_REF_COUNTED_STATES(Modifying_PackageActivationDeactivation);
        STATEMACHINE_TRANSITION(Opening, Created);
        STATEMACHINE_TRANSITION(Opened, Opening|Upgrading|Modifying_PackageUpgrade|Modifying_PackageActivationDeactivation|Analyzing);
        STATEMACHINE_TRANSITION(Upgrading, Opened);
        STATEMACHINE_TRANSITION(Modifying_PackageUpgrade, Opened);
        STATEMACHINE_TRANSITION(Modifying_PackageActivationDeactivation, Opened|Modifying_PackageActivationDeactivation);
        STATEMACHINE_TRANSITION(Analyzing, Opened);
        STATEMACHINE_TRANSITION(Failed, Opening|Closing|Upgrading);
        STATEMACHINE_TRANSITION(Closing, Opened);
        STATEMACHINE_TRANSITION(Closed, Closing);
        STATEMACHINE_ABORTED_TRANSITION(Created|Opened|Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Closed);

    public:
        VersionedApplication(
            Application2Holder const & applicationHolder,
            ServiceModel::ApplicationVersion const & version,
            ServiceModel::ApplicationPackageDescription const & appPackageDescription);
        virtual ~VersionedApplication();

        // Opens the service application with the version specified in the ctor
        // if Open is attempted and it is successful, the application transitions to Opened state
        // if Open is attempted and it is unsuccessful, the application transitions to Failed state
        Common::AsyncOperationSPtr BeginOpen(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginSwitch(
            ServiceModel::ApplicationVersion const & newVersion,
            ServiceModel::ApplicationPackageDescription newPackageDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndSwitch(
            Common::AsyncOperationSPtr const & operation);

        // Perform various operations on the service packages of this application
        // if Application is not in the correct state to process the request, error is returned
        // if Application is in correct state and request is processed:
        //      the operations are performed and the application remains in the opened state (regardless of the outcome of the operations)
        //      if after performing the operations transition to Opened fails, the application goes to failed state and all packages are aborted
        //  the overall async operation is successful only when all package operations are successful and application transitions back to opened state
        //
        // activates specified versions of the service packages. 
        // if the service package is already active and ensureLatestVersion is false, version mismatch error is returned
        // if the service package is already active and ensureLatestVersion is true, 
        //   and if the desired version is lower than running version, it is allowed and no error is returned;
        //       if the desired version is higher than running version, the lower version is aborted and version mismatch error is returned
        //  
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

        // Closes the opened application. 
        // if Close is attempted and it is successful, the application transitions to Closed state
        // if Close is attempted and it is not successful, the application is Aborted
        Common::AsyncOperationSPtr BeginClose(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        void EndClose(
            Common::AsyncOperationSPtr const & operation);

        __declspec(property(get=get_Id)) ServiceModel::ApplicationIdentifier const & Id;
        ServiceModel::ApplicationIdentifier const & get_Id() const;

        __declspec(property(get=get_Version)) ServiceModel::ApplicationVersion const & Version;
        ServiceModel::ApplicationVersion const & get_Version() const;

        Common::ErrorCode AnalyzeApplicationUpgrade(            
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            __out CaseInsensitiveStringSet & affectedRuntimeIds);

        void OnServiceTypeRegistrationNotFound(
            uint64 const registrationTableVersion,
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId);
        
        //Common::ErrorCode HasServicePackage(std::wstring const & servicePackageName, __out bool & exists);

        Common::ErrorCode GetServicePackageInstance(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            __out ServicePackage2SPtr & servicePackageInstances) const;

        Common::ErrorCode GetServicePackageInstance(
            std::wstring const & servicePackageName, 
            std::wstring const & servicePackagePublicActivationId,
            __out ServicePackage2SPtr & servicePackageInstance) const;

        Common::ErrorCode GetServicePackageInstance(
            std::wstring const & servicePackageName,
            ServiceModel::ServicePackageVersion const & version,
            __out ServicePackage2SPtr & servicePackageInstance) const;

        Common::ErrorCode GetAllServicePackageInstances(__out std::vector<ServicePackage2SPtr> & servicePackageInstances) const;        
        Common::ErrorCode GetInstancesOfServicePackage(std::wstring const & servicePackageName, __out std::vector<ServicePackage2SPtr> & servicePackageInstances) const;
        void NotifyDcaAboutServicePackages();

        Common::ErrorCode IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent) const;

        void OnServiceTypesUnregistered(
            ServicePackageInstanceIdentifier const servicePackageInstanceId,
            std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds);

    private:
        __declspec(property(get=get_Application)) Application2 const & ApplicationObj;
        Application2 const & get_Application() const;

        __declspec(property(get=get_EnvironmentManagerObj)) EnvironmentManagerUPtr const & EnvironmentManagerObj;
        EnvironmentManagerUPtr const & get_EnvironmentManagerObj() const;

        static std::wstring GetStatusMapKey(ServicePackage2SPtr const & servicePackageInstance);

    protected:
        virtual void OnAbort();

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;

        class SwitchAsyncOperation;

        class ServicePackageInstancesAsyncOperationBase;

        class ActivateServicePackageInstancesAsyncOperation;
        class ActivateServicePackageInstanceAsyncOperation;

        class UpgradeServicePackagesAsyncOperation;
        class UpgradeServicePackageInstanceAsyncOperation;

        class DeactivateServicePackageInstancesAsyncOperation;
        class DeactivateServicePackageInstanceAsyncOperation;

    private:
        Application2Holder const applicationHolder_;
        std::wstring const healthPropertyId_;

        // WARNING: Access to the following variables should be restricted to only 
        // protected states of the state machine or access should be done under state lock.
        // Currently Opening, Closing, Switching and OnAbort are protected state, more than 
        // one thread cannot execute code in parallel in these states.
        ServiceModel::ApplicationVersion version__;
        ServiceModel::ApplicationPackageDescription packageDescription__;
        ApplicationEnvironmentContextSPtr environmentContext__;
        ServicePackage2MapUPtr packageMap__;
    };
}
