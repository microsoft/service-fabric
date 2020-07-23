// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class VersionedServicePackage :
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(VersionedServicePackage)

        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        STATEMACHINE_STATES_09(Created, Opening, Opened, Switching, Analyzing, Modifying, Closing, Closed, Failed);
        STATEMACHINE_REF_COUNTED_STATES(Modifying);
        STATEMACHINE_TRANSITION(Opening, Created);
        STATEMACHINE_TRANSITION(Opened, Opening|Switching|Analyzing|Modifying);
        STATEMACHINE_TRANSITION(Modifying, Opened|Modifying);
        STATEMACHINE_TRANSITION(Switching, Opened);
        STATEMACHINE_TRANSITION(Analyzing, Opened);
        STATEMACHINE_TRANSITION(Failed, Opening|Switching|Closing);
        STATEMACHINE_TRANSITION(Closing, Opened);
        STATEMACHINE_TRANSITION(Closed, Closing);
        STATEMACHINE_ABORTED_TRANSITION(Created|Opened|Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Closed);

    public:
        VersionedServicePackage(
            ServicePackage2Holder const & servicePackageHolder,
            ServiceModel::ServicePackageVersionInstance const & versionInstance,
            ServiceModel::ServicePackageDescription const & packageDescription,
            std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds);
        virtual ~VersionedServicePackage();

        // Opens the service package with the version specified in the ctor
        // if Open is attempted and it is successful, the package transitions to Opened state
        // if Open is attempted and it is unsuccessful, the package transitions to Failed state
        Common::AsyncOperationSPtr BeginOpen(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const & operation);

        // Switches to the specified version from the currently running version
        // if Switch is attempted and it is successful, the package transitions to Opened state
        // if Switch is attempted and it is unsuccessful, the package transitions to Failed state
        Common::AsyncOperationSPtr BeginSwitch(
            ServiceModel::ServicePackageVersionInstance const & toVersionInstance,
            ServiceModel::ServicePackageDescription toPackageDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndSwitch(
            Common::AsyncOperationSPtr const & operation);

        // Closes the opened package. 
        // if Close is attempted and it is successful, the package transitions to Closed state
        // if Close is attempted and it is not successful, the package is Aborted
        Common::AsyncOperationSPtr BeginClose(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        void EndClose(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode AnalyzeApplicationUpgrade(
            ServiceModel::ServicePackageVersionInstance const & toVersionInstance,
            ServiceModel::ServicePackageDescription const & toPackageDescription,
            __out CaseInsensitiveStringSet & affectedRuntimeIds);
        
        ULONG GetFailureCount() const;

        std::vector<CodePackageSPtr> GetCodePackages(std::wstring const & filterCodePackageName = L"");

        void OnServiceTypeRegistrationNotFound(
            uint64 const registrationTableVersion,
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId);

        Common::ErrorCode IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent);

        Common::AsyncOperationSPtr BeginRestartCodePackageInstance(
            std::wstring const & codePackageName,
            int64 instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndRestartCodePackageInstance(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginApplicationHostCodePackageOperation(
            ApplicationHostCodePackageOperationRequest const & request,
            int64 requestorInstanceId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndApplicationHostCodePackageOperation(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginProcessActivatorCodePackageTerminated(
            int64 activatorCodePackageInstanceId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        void EndProcessActivatorCodePackageTerminated(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginTerminateFabricTypeHostCodePackage(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent,
            int64 instanceId);

        Common::ErrorCode EndTerminateFabricTypeHostCodePackage(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & instanceId);

        void OnServiceTypesUnregistered(
            std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds);

        void OnDockerHealthCheckUnhealthy(std::wstring const & codePackageName, int64 instanceId);

        void OnActivatorCodePackageInitialized(int64 activatorCodePackageInstanceId);

        void ProcessDependentCodePackageEvent(
            CodePackageEventDescription && eventDesc,
            int64 targetActivatorCodePackageInstanceId);

    public:
        __declspec(property(get=get_CurrentVersionInstance)) ServiceModel::ServicePackageVersionInstance const CurrentVersionInstance;
        ServiceModel::ServicePackageVersionInstance const get_CurrentVersionInstance() const;

        __declspec(property(get = get_ServicePackage)) ServicePackage2 const & ServicePackageObj;
        ServicePackage2 const & get_ServicePackage() const;

        __declspec(property(get = get_IsOnDemandActivationEnabled)) bool const IsOnDemandActivationEnabled;
        bool const get_IsOnDemandActivationEnabled() const { return isOnDemandActivationEnabled_; }

        __declspec(property(get = get_PackageDescription)) ServiceModel::ServicePackageDescription const& PackageDescription;
        ServiceModel::ServicePackageDescription const& get_PackageDescription() const { return packageDescription__; }

        __declspec(property(get=get_EnvironmentManager)) EnvironmentManagerUPtr const & EnvironmentManagerObj;
        EnvironmentManagerUPtr const & get_EnvironmentManager() const;

        __declspec(property(get = get_FabricRuntimeManager)) FabricRuntimeManagerUPtr const & FabricRuntimeManagerObj;
        inline FabricRuntimeManagerUPtr const & get_FabricRuntimeManager() const;

        __declspec(property(get = get_HealthManagerObj)) HostingHealthManagerUPtr const & HealthManagerObj;
        HostingHealthManagerUPtr const & get_HealthManagerObj() const;

        __declspec(property(get = get_LocalResourceManager)) LocalResourceManagerUPtr const & LocalResourceManagerObj;
        inline LocalResourceManagerUPtr const & get_LocalResourceManager() const;

        __declspec (property(get = get_ServiceTypeStateManager)) ServiceTypeStateManagerUPtr const & ServiceTypeStateManagerObj;
        ServiceTypeStateManagerUPtr const & get_ServiceTypeStateManager() const;

        __declspec (property(get = get_Hosting)) HostingSubsystem const & Hosting;
        HostingSubsystem const & get_Hosting() const;

    protected:
        virtual void OnAbort();

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class SwitchAsyncOperation;

        class CodePackagesAsyncOperationBase;
        class ActivateCodePackagesAsyncOperation;
        class UpdateCodePackagesAsyncOperation;
        class DeactivateCodePackagesAsyncOperation;
        class AbortCodePackagesAsyncOperation;
        class TerminateFabricTypeHostAsyncOperation;
        class RestartCodePackageInstanceAsyncOperation;
        class ActivatePrimaryCodePackagesAsyncOperation;
        class DeactivateSecondaryCodePackagesAsyncOperation;
        class ActivateSecondaryCodePackagesAsyncOperation;
        class ApplicationHostCodePackagesAsyncOperation;
        class ApplicationHostActivateCodePackageAsyncOperation;
        class ApplicationHostDeactivateCodePackageAsyncOperation;
        class ApplicationHostAbortCodePackageAsyncOperation;
        class ActivatorCodePackageTerminatedAsyncOperation;

        class ServiceTypeRegistrationTimeoutTracker;
        typedef std::shared_ptr<ServiceTypeRegistrationTimeoutTracker> ServiceTypeRegistrationTimeoutTrackerSPtr;

        class ActivatorCodePackageOperationTracker;
        typedef std::shared_ptr<ActivatorCodePackageOperationTracker> ActivatorCodePackageOperationTrackerSPtr;

        typedef std::function<void(
            Common::AsyncOperationSPtr const & operation, 
            Common::ErrorCode const error, 
            CodePackageMap const & completed, 
            CodePackageMap const & failed)> CodePackagesAsyncOperationCompletedCallback;

        typedef std::map<
            std::wstring,
            ServiceModel::DigestedCodePackageDescription,
            Common::IsLessCaseInsensitiveComparer<std::wstring>> DigestedCodePackageDescriptionMap;

    private:
        void InitializeOnDemandActivationInfo();

        void GetCurrentVersionInstanceAndPackageDescription(
            _Out_ ServiceModel::ServicePackageVersionInstance & versionInstance,
            _Out_ ServiceModel::ServicePackageDescription & packageDescription);

        Common::ErrorCode WriteCurrentPackageFile(
            ServiceModel::ServicePackageVersion const & packageVersion);
        
        Common::ErrorCode LoadCodePackages(
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            ServiceModel::ServicePackageDescription const & packageDescription,
            ServicePackageInstanceEnvironmentContextSPtr const & envContext,
            bool onlyActivatorCodePackage,
            _Out_ CodePackageMap & codePackages);

        void GetUserId(
            std::wstring const & codePackageName,
            ServiceModel::RunAsPolicyDescription const & runAsPolicyDesc,
            bool isSetupRunAsPolicy,
            _Out_ std::wstring & userId);

        void GetProcessDebugParams(
            ServiceModel::DigestedCodePackageDescription const & dcpDesc,
            _Out_ ProcessDebugParameters & debugParams);

        std::wstring GetImplicitTypeHostName();

        Common::ErrorCode GetActivatorCodePackageInstanceId(
            std::wstring const & codePackageName,
            _Out_ int64 & instanceId);

        int64 GetActivatorCodePackageInstanceId();
        Common::ErrorCode GetActivatorCodePackage(CodePackageSPtr & activatorCodePackage);

        Common::ErrorCode CreateImplicitTypeHostCodePackage(
            std::wstring const & codePackageVersion,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            std::vector<GuestServiceTypeInfo> const & serviceTypes,
            ServicePackageInstanceEnvironmentContextSPtr const & envContext,
            bool useSFReplicatedStore,
            __out CodePackageSPtr & typeHostCodePackage);

        void ActivateCodePackagesAsync(
            CodePackageMap const & codePackages,
            Common::TimeSpan const timeout,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
            Common::AsyncOperationSPtr const & parent);

        void FinishActivateCodePackages(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback);

        void UpdateCodePackagesAsync(
            CodePackageMap const & codePackages,
            ServiceModel::ServicePackageDescription const & newPackageDescription,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            Common::TimeSpan const timeout,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
            Common::AsyncOperationSPtr const & parent);

        void FinishUpdateCodePackages(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback);

        void DeactivateCodePackagesAsync(
            CodePackageMap const & codePackages,
            Common::TimeSpan const timeout,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
            Common::AsyncOperationSPtr const & parent);

        void FinishDeactivateCodePackages(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback);

        void AbortCodePackagesAsync(
            CodePackageMap const & codePackages,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
            Common::AsyncOperationSPtr const & parent);

        void FinishAbortCodePackages(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback);

        std::map<std::wstring, std::wstring> GetCodePackagePortMappings(
            ServiceModel::ContainerPoliciesDescription const &,
            ServicePackageInstanceEnvironmentContextSPtr const &);

        ErrorCode TryGetPackageSecurityPrincipalInfo(
            __in std::wstring const & name,
            __out SecurityPrincipalInformationSPtr & info);

        Common::ErrorCode RegisterEventWaitOperation(AsyncOperationSPtr const & operation, bool isActivateOperation = true);
        void UnregisterEventWaitOperation(bool isActivateOperation = true);

    private:
        ServicePackage2Holder const servicePackageHolder_;
        Management::ImageModel::RunLayoutSpecification const runLayout_;
        std::wstring const failureId_;
        std::wstring const healthPropertyId_;
        std::wstring const componentId_;
        std::vector<ServiceTypeInstanceIdentifier> const serviceTypeInstanceIds_;
        ServiceTypeRegistrationTimeoutTrackerSPtr registrationTimeoutTracker_;

        ServiceModel::ServicePackageVersionInstance currentVersionInstance__;
        ServiceModel::ServicePackageDescription packageDescription__;
        ServicePackageInstanceEnvironmentContextSPtr environmentContext__;
        bool codePackagesActivationRequested_;
        std::unique_ptr<Common::AsyncAutoResetEvent> activateEventHandler_;
        std::unique_ptr<Common::AsyncAutoResetEvent> deactivateEventHandler_;
        Common::AsyncOperationSPtr eventWaitAsyncOperation_;
        Common::AsyncOperationSPtr deactivateAsyncOperation_;
        Common::ExclusiveLock lock_;
        CodePackageMap activeCodePackages_;

        //
        // Indicates if this SP allows on-demand activation of CPs.
        //
        bool isOnDemandActivationEnabled_;
        wstring activatorCodePackageName_;
        ServiceModel::RolloutVersion activatorCodePackageRolloutVersion_;
        bool isGuestApplication_;

        //
        // This is InstanceId of current instance of activator CP.
        // When termination notification for activator CP is received
        // it is reset to 0 and when new instance of activator CP comes
        // up, it is set to new InstanceId of activator CP.
        // 
        // This is also reset when activator CP is changed (e.g
        // during switch operation if update context to it fails and
        // activator CP is aborted as a result).
        //
        int64 activatorCodePackageInstanceId_;

        //
        // Tracks the inflight operations requested by activator CP.
        //
        ActivatorCodePackageOperationTrackerSPtr operationTracker_;
    };
}
