// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CodePackageInstance;
    // 
    // This type represents a CodePackage that is part of a ServiceManifest of a deployment unit.
    //
    class CodePackage :
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        DENY_COPY(CodePackage)

        STATEMACHINE_STATES_12(Inactive, Stats_Initializing, Setup_Initializing, Setup_Initialized, EntryPoint_Initializing, EntryPoint_Initialized, Updating, EntryPoint_Restarting, EntryPoint_Restarted, Deactivating, Deactivated, Failed);
        STATEMACHINE_TRANSITION(Stats_Initializing, Inactive);
        STATEMACHINE_TRANSITION(Setup_Initializing, Stats_Initializing | Setup_Initialized | EntryPoint_Initialized | EntryPoint_Restarted);
        STATEMACHINE_TRANSITION(Setup_Initialized, Setup_Initializing | EntryPoint_Restarting);
        STATEMACHINE_TRANSITION(EntryPoint_Initializing, Setup_Initialized);
        STATEMACHINE_TRANSITION(EntryPoint_Initialized, EntryPoint_Initializing | Updating | EntryPoint_Restarting);
        STATEMACHINE_TRANSITION(Updating, EntryPoint_Initialized);
        STATEMACHINE_TRANSITION(EntryPoint_Restarting, Setup_Initialized|EntryPoint_Initialized);
        STATEMACHINE_TRANSITION(EntryPoint_Restarted, EntryPoint_Restarting);
        STATEMACHINE_TRANSITION(Deactivating, EntryPoint_Initialized);
        STATEMACHINE_TRANSITION(Deactivated, Deactivating);
        STATEMACHINE_TRANSITION(Failed, Stats_Initializing|Updating|Deactivating|EntryPoint_Restarting);
        STATEMACHINE_ABORTED_TRANSITION(Setup_Initialized | EntryPoint_Initialized | EntryPoint_Restarted| Deactivated | Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Deactivated);

    public:
        struct RunStats : public Common::RootedObject
        {
            RunStats();

            RunStats(
                Common::ComponentRoot const & root,
                CodePackage & codePackage,
                bool isSetupEntryPoint);

            __declspec(property(get=get_Stats)) ServiceModel::CodePackageEntryPointStatistics Stats;
            ServiceModel::CodePackageEntryPointStatistics get_Stats();

            __declspec(property(get = get_HealthConfig)) ServiceModel::ContainerHealthConfigDescription const & HealthConfig;
            ServiceModel::ContainerHealthConfigDescription const & get_HealthConfig() const;

            ServiceModel::CodePackageEntryPointStatistics stats;
            void UpdateProcessActivationStats(Common::ErrorCode const & error);
            void UpdateProcessExitStats(DWORD exitCode, bool ignoreReporting);
            ULONG GetMaxContinuousFailureCount();
            void UpdateDockerHealthCheckStatusStats(ContainerHealthStatusInfo const & healthStatusInfo, int64 instanceId);
            
            Common::ErrorCode Open();
            void Close();

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

        private:
            void OnContinousExitFailureResetTimeout(Common::TimerSPtr const & timer, ULONG exitCount);

            ULONG GetMaxContinuousFailureCount_CallerHoldsLock();
            void GetHealth_CallerHoldsLock(
                __out bool & shouldRaiseEvent, 
                __out Common::SystemHealthReportCode::Enum & reportCode, 
                __out FABRIC_SEQUENCE_NUMBER & sequenceNumber);

        private:
            CodePackage & codePackage_;

            std::wstring const activationHealthPropertyId_;
            ServiceModel::CodePackageEntryPointStatistics stats_;
            Common::TimerSPtr timer_;
            FABRIC_HEALTH_STATE lastReportedActivationHealth_;
            bool isClosed_;
            Common::RwLock statsLock_;

            // PropertyId used to report health for docker health check events.
            std::wstring const healthCheckStatusHealthPropertyId_;
            
            // Last reported health state when docker health check event for this container.
            FABRIC_HEALTH_STATE lastReportedHealthCheckStatusHealth_;
            
            // Time in seconds (measured from 01/01/1970) when last health was reported 
            // corresponding to docker health check event for this container.
            int64 lastReportedHealthCheckStatusTimeInSec_;
        };

    public:
        CodePackage(
            HostingSubsystemHolder const & hostingHolder,
            VersionedServicePackageHolder const & versionedServicePackageHolder,
            ServiceModel::DigestedCodePackageDescription const & description,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            int64 const servicePackageInstanceSeqNum,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            std::wstring const & applicationName,
            ServiceModel::RolloutVersion rolloutVersion,
            ServicePackageInstanceEnvironmentContextSPtr const & envContextSPtr,
            int64 activatorCodePackageInstanceId,
            Common::EnvironmentMap && onDemandActivationEnvironment,
            std::wstring const & runAsId = L"",
            std::wstring const & setupRunas = std::wstring(),
            bool isShared = false,
            ProcessDebugParameters const & debugParameters = ProcessDebugParameters(),
            std::map<std::wstring, std::wstring> const & portBindings = std::map<std::wstring, std::wstring>(),
            bool removeServiceFabricRuntimeAccess = false,
            std::vector<GuestServiceTypeInfo> const & guestServiceTypes = std::vector<GuestServiceTypeInfo>());

        virtual ~CodePackage();

        ULONG GetMaxContinuousFailureCount();
        void SetDebugParameters(ProcessDebugParameters const & debugParameters);

        __declspec(property(get=get_CodePackageContext)) CodePackageContext const & Context;
        inline CodePackageContext const & get_CodePackageContext() const { return this->context_; }

        __declspec(property(get=get_CodePackageInstanceId)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        inline CodePackageInstanceIdentifier const & get_CodePackageInstanceId() const { return this->context_.CodePackageInstanceId; }

        __declspec(property(get = get_ServicePackageInstanceId)) ServicePackageInstanceIdentifier const & ServicePackageInstanceId;
        inline ServicePackageInstanceIdentifier const & get_ServicePackageInstanceId() const { return this->context_.CodePackageInstanceId.ServicePackageInstanceId; }

        __declspec(property(get=get_InstanceId)) int64 const InstanceId;
        int64 const get_InstanceId() const { return this->instanceId_; }

        __declspec(property(get=get_Version)) std::wstring const & Version;
        inline std::wstring const & get_Version() const { return this->description_.CodePackage.Version; }

        __declspec(property(get=get_Description)) ServiceModel::DigestedCodePackageDescription const & Description;
        inline ServiceModel::DigestedCodePackageDescription const & get_Description() const { return this->description_; }

        __declspec(property(get = get_ServicePackageEnvContext)) ServicePackageInstanceEnvironmentContextSPtr const & EnvContext;
        inline ServicePackageInstanceEnvironmentContextSPtr const & get_ServicePackageEnvContext() const { return this->envContext_; }

        __declspec(property(get=get_RunAsId)) std::wstring const & RunAsId;
        std::wstring const & get_RunAsId() const { return this->runAsId_; }

        __declspec(property(get=get_SetupRunAs)) std::wstring const & SetupRunAs;
        std::wstring const & get_SetupRunAs() const { return this->setupRunas_; } 

        __declspec(property(get=get_RolloutVersionValue)) ServiceModel::RolloutVersion const & RolloutVersionValue;
        ServiceModel::RolloutVersion const & get_RolloutVersionValue() const { return this->rollOutVersion_; }

        __declspec(property(get=get_IsShared)) bool const IsShared;
        bool const get_IsShared() const { return this->isShared_; }

        __declspec(property(get = get_IsActivator)) bool const IsActivator;
        bool const get_IsActivator() const { return description_.CodePackage.IsActivator; }

        __declspec(property(get = get_ActivatorCodePackageInstanceId)) int64 ActivatorCodePackageInstanceId;
        int64 get_ActivatorCodePackageInstanceId() const { return activatorCodePackageInstanceId_; }

        __declspec(property(get = get_OnDemandActivationEnvironment)) Common::EnvironmentMap const & OnDemandActivationEnvironment;
        inline Common::EnvironmentMap const & get_OnDemandActivationEnvironment() const { return onDemandActivationEnvironment_; }

        __declspec(property(get=get_removeServiceFabricRuntimeAccess)) bool const RemoveServiceFabricRuntimeAccess;
        bool const get_removeServiceFabricRuntimeAccess() const { return this->removeServiceFabricRuntimeAccess_; }

        __declspec(property(get = get_IsImplicitTypeHost)) bool const IsImplicitTypeHost;
        bool const get_IsImplicitTypeHost() const { return this->isImplicitTypeHost_; }

        __declspec(property(get = get_ProcessDebugParameters)) ProcessDebugParameters const & DebugParameters;
        ProcessDebugParameters const & get_ProcessDebugParameters() const { return this->debugParameters_; }

        __declspec(property(get = get_PortBindings)) std::map<std::wstring, std::wstring> const & PortBindings;
        std::map<std::wstring, std::wstring> const & get_PortBindings() const { return this->portBindings_; }
        
        __declspec(property(get = get_GuestServiceTypeNames)) std::vector<GuestServiceTypeInfo> const & GuestServiceTypes;
        std::vector<GuestServiceTypeInfo> const & get_GuestServiceTypeNames() const { return this->guestServiceTypes_; }

        Common::AsyncOperationSPtr BeginActivate(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndActivate(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDeactivate(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        void EndDeactivate(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginUpdateContext(
            ServiceModel::RolloutVersion const & newCodePackageRolloutVersion,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateContext(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRestartCodePackageInstance(
            int64 instanceIdOfCodePackageInstance,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRestartCodePackageInstance(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginGetContainerInfo(
            int64 instanceIdOfCodePackageInstance,
            wstring const & containerInfoTypeArgument,
            wstring const & containerInfoArgsArgument,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const & operation,
            __out wstring & containerInfo);

        Common::AsyncOperationSPtr BeginApplicationHostCodePackageOperation(
            ApplicationHostCodePackageOperationRequest const & request,
            int64 requestorInstanceId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndApplicationHostCodePackageOperation(
            Common::AsyncOperationSPtr const & operation);

        void OnMainEntryPointStarted(Common::ErrorCode error, int64 instanceId);
        void OnMainEntryPointTerminated(DWORD exitCode, int64 instanceId);

        void OnSetupEntryPointStarted(Common::ErrorCode error, int64 instanceId);
        void OnSetupEntryPointTerminated(DWORD exitCode, int64 instanceId);

        ServiceModel::DeployedCodePackageQueryResult GetDeployedCodePackageQueryResult();

        void RestartCodePackage(int64 instanceId);

        void SendDependentCodePackageEvent(
            CodePackageEventDescription const & eventDesc,
            int64 targetActivatorCodePackageInstanceId);

        void GetMainEntryPointAndCurrentState(
            _Out_ CodePackageInstanceSPtr & mainEntryPoint,
            _Out_ uint64 & currentState);

        Common::ErrorCode TerminateCodePackageExternally(__out Common::TimeSpan & retryDueTime);

        __declspec(property(get = get_VersionServicePackage)) VersionedServicePackage & VersionedServicePackageObj;
        VersionedServicePackage & get_VersionServicePackage() const { return this->versionedServicePackageHolder_.RootedObject; }

        __declspec(property(get = get_BindMounts))  std::map<std::wstring, std::wstring> const & BindMounts;
        inline std::map<std::wstring, std::wstring> const & get_BindMounts() const { return conifgPackagePoliciesBindMounts_; };

        __declspec(property(get = get_EnvVariablesForMounts))  std::map<std::wstring, std::wstring> const & EnvironmentVariableForMounts;
        inline std::map<std::wstring, std::wstring> const & get_EnvVariablesForMounts() const { return envVariblesMountPoints_; };

    protected:
        virtual void OnAbort();

    private:
        __declspec(property(get=get_Hosting)) HostingSubsystem const & Hosting;
        HostingSubsystem const & get_Hosting() const { return this->hostingHolder_.RootedObject; }

        Common::TimeSpan GetDueTime(
            ServiceModel::CodePackageEntryPointStatistics stats, 
            Common::TimeSpan runInterval);

        Common::ErrorCode InitializeSetupEntryPoint();
        Common::ErrorCode InitializeMainEntryPoint(); 
        void FinishSetupEntryPointStartCompleted(Common::AsyncOperationSPtr const & operation);
        void FinishMainEntryPointStartCompleted(Common::AsyncOperationSPtr const & operation);
        void RestartTerminatedCodePackageInstance(int64);
        void CleanupTimer();
        void NotifyMainEntryPointTermination(DWORD exitCode, int64 instanceId);
        void NotifyMainEntryPointDeactivation(CodePackageInstanceSPtr mainEntryPoint);

        void GetHostTypeAndIsolationMode(
            ServiceModel::HostType::Enum & hostType, 
            ServiceModel::HostIsolationMode::Enum & hostIsolationMode);

        void FinishTerminateFabricTypeHostCodePackage(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynhronously);

        void FinishNotifyMainEntryPointTermination(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynhronously,
            int64 instanceId);

        Common::ErrorCode GetEntryPoint(
            int64 const instanceIdOfCodePackageInstance, 
            __out CodePackageInstanceSPtr & entryPoint);

    private:
        class DeactivateAsyncOperation;
        class UpdateContextAsyncOperation;
        class RestartCodePackageInstanceAsyncOperation;
        class GetContainerInfoAsyncOperation;
        class ApplicationHostCodePackageOperation;

        friend CodePackageInstance;

        Common::ErrorCode WriteConfigPackagePolicies(bool update = false);

    private:        
        HostingSubsystemHolder const hostingHolder_;
        VersionedServicePackageHolder const versionedServicePackageHolder_;
        CodePackageInstanceIdentifier const id_;
        int64 const instanceId_;
        ServiceModel::DigestedCodePackageDescription const description_;
        ServicePackageInstanceEnvironmentContextSPtr envContext_;
        std::wstring const runAsId_;
        std::wstring const setupRunas_;
        ServiceModel::RolloutVersion rollOutVersion_;
        CodePackageContext context_;
        RunStats setupEntryPointStats_;
        RunStats mainEntryPointStats_;
        Common::TimeSpan mainEntryPointRunInterval_;
        Common::TimerSPtr retryTimer_;
        bool isShared_;
        ProcessDebugParameters debugParameters_;
        std::map<std::wstring, std::wstring> portBindings_;
        bool isImplicitTypeHost_;
        vector<GuestServiceTypeInfo> const guestServiceTypes_;
        Common::Random randomGenerator_;
        bool removeServiceFabricRuntimeAccess_;

        Common::ExclusiveLock timerLock_;

        CodePackageInstanceSPtr setupEntryPointCodePackageInstance_;
        CodePackageInstanceSPtr mainEntryPointCodePackageInstance_;

        FileChangeMonitor2SPtr lockFileMonitor_;
        std::map<std::wstring, std::wstring> conifgPackagePoliciesBindMounts_;
        std::map<std::wstring, std::wstring> envVariblesMountPoints_;

        //
        // This is set to non-zero value if this CP is
        // activated on-demand by an activator/master CP.
        //
        int64 activatorCodePackageInstanceId_;
        Common::EnvironmentMap onDemandActivationEnvironment_;
    };
}
