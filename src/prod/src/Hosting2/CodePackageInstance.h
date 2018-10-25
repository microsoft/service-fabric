// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    typedef std::function<void(Common::ErrorCode, int64)> StartCallback;
    typedef std::function<void(DWORD, int64)> TerminationCallback;
    
    class CodePackageInstance :
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {    
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        DENY_COPY(CodePackageInstance)

        STATEMACHINE_STATES_09(Created, Scheduling, Scheduled, Starting, Started, Updating, Stopping, Stopped, Failed);
        STATEMACHINE_TRANSITION(Scheduling, Created|Starting);
        STATEMACHINE_TRANSITION(Scheduled, Scheduling);
        STATEMACHINE_TRANSITION(Starting, Scheduled);
        STATEMACHINE_TRANSITION(Started, Starting|Updating);
        STATEMACHINE_TRANSITION(Updating, Started);
        STATEMACHINE_TRANSITION(Stopping, Started|Scheduled);
        STATEMACHINE_TRANSITION(Stopped, Stopping);
        STATEMACHINE_TRANSITION(Failed, Updating|Stopping|Scheduling);
        STATEMACHINE_ABORTED_TRANSITION(Scheduled|Started|Failed|Stopped);
        STATEMACHINE_TERMINAL_STATES(Aborted|Stopped);

    public:
        CodePackageInstance(
            CodePackageHolder const & codePackageHolder,
            ServiceModel::EntryPointDescription const & entryPoint,
            StartCallback const & entryPointStartedCallback,
            TerminationCallback const & entryPointTerminatedCallback,
            bool isShared,
            bool isSetupEntryPoint,
            bool isActivator,
            Common::TimeSpan runInterval,
            CodePackage::RunStats & stats,
            std::wstring const & runAsId = L"");

        ~CodePackageInstance();

        __declspec(property(get=get_EntryPointDescription)) ServiceModel::EntryPointDescription const & EntryPoint;
        inline ServiceModel::EntryPointDescription const & get_EntryPointDescription() const { return this->entryPoint_; }

        __declspec(property(get=get_RunAsId)) std::wstring const & RunAsId;
        inline std::wstring const & get_RunAsId() const { return this->runAsId_; }

        __declspec(property(get=get_IsolationPolicyType)) ServiceModel::CodePackageIsolationPolicyType::Enum IsolationPolicyType;
        inline ServiceModel::CodePackageIsolationPolicyType::Enum get_IsolationPolicyType() const 
        {
            return this->entryPoint_.IsolationPolicy; 
        }

        __declspec(property(get=get_IsShared)) bool IsShared;
        inline bool get_IsShared() const { return this->isShared_; }

        __declspec(property(get = get_IsActivator)) bool IsActivator;
        inline bool get_IsActivator() const { return isActivator_; }

        __declspec(property(get = get_IsContainerHost)) bool IsContainerHost;
        inline bool get_IsContainerHost() const
        { 
            return (entryPoint_.EntryPointType == ServiceModel::EntryPointType::ContainerHost);
        }

        __declspec(property(get=get_IsSetupEntryPoint)) bool IsSetupEntryPoint;
        inline bool get_IsSetupEntryPoint() const { return isSetupEntryPoint_; }

        __declspec(property(get=get_CodePackageContext)) CodePackageContext const & Context;
        CodePackageContext const & get_CodePackageContext() const;

        __declspec(property(get = get_CodePackageInstanceId)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        CodePackageInstanceIdentifier const & get_CodePackageInstanceId() const;

        __declspec(property(get=get_Version)) std::wstring const & Version;
        std::wstring const & get_Version() const;

        __declspec(property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const;

        __declspec(property(get=get_IsPackageShared)) bool IsPackageShared;
        inline bool get_IsPackageShared() const { return this->CodePackageObj.IsShared; }

        __declspec(property(get = get_ProcessDebugParameters)) ProcessDebugParameters const & DebugParameters;
        ProcessDebugParameters const & get_ProcessDebugParameters() const { return this->CodePackageObj.DebugParameters; }

        __declspec(property(get = get_PortBindings)) std::map<std::wstring, std::wstring> const & PortBindings;
        std::map<std::wstring, std::wstring> const & get_PortBindings() const { return this->CodePackageObj.PortBindings; }

        __declspec(property(get = get_EnvironmentVariablesDescription)) ServiceModel::EnvironmentVariablesDescription const & EnvVariablesDescription;
        ServiceModel::EnvironmentVariablesDescription const & get_EnvironmentVariablesDescription() const 
        { 
            return this->CodePackageObj.Description.CodePackage.EnvironmentVariables; 
        }

        __declspec(property(get = get_ServicePackageEnvContext)) ServicePackageInstanceEnvironmentContextSPtr const & EnvContext;
        inline ServicePackageInstanceEnvironmentContextSPtr const & get_ServicePackageEnvContext() const { return this->CodePackageObj.EnvContext; }

        __declspec(property(get=get_InstanceId)) int64 const InstanceId;
        int64 const get_InstanceId() const { return this->instanceId_; }

        __declspec(property(get = get_ResourceGovernancePolicy)) ServiceModel::ResourceGovernancePolicyDescription const & ResourceGovernancePolicy;
        ServiceModel::ResourceGovernancePolicyDescription const & get_ResourceGovernancePolicy() const 
        { 
            return this->CodePackageObj.Description.ResourceGovernancePolicy; 
        }

        __declspec(property(get = get_ContainerPolicies)) ServiceModel::ContainerPoliciesDescription const & ContainerPolicies;
        ServiceModel::ContainerPoliciesDescription const & get_ContainerPolicies() const 
        { 
            return this->CodePackageObj.Description.ContainerPolicies; 
        }

        __declspec(property(get = get_CountinuousFailureCount)) ULONG ContinuousFailureCount;
        ULONG get_CountinuousFailureCount() const { return this->codePackageHolder_.RootedObject.GetMaxContinuousFailureCount(); }

        Common::DateTime GetLastActivationTime();

        Common::AsyncOperationSPtr BeginStart(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndStart(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginStop(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        void EndStop(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginUpdateContext(
            CodePackageContext const & updatedContext,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateContext(
            Common::AsyncOperationSPtr const & operation);
            
        Common::AsyncOperationSPtr BeginGetContainerInfo(
            wstring const & containerInfoType,
            wstring const & containerInfoArgs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const & operation,
            __out wstring & containerInfo);

        Common::AsyncOperationSPtr BeginApplicationHostCodePackageOperation(
            ApplicationHostCodePackageOperationRequest const & request,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndApplicationHostCodePackageOperation(
            Common::AsyncOperationSPtr const & operation);

        void OnEntryPointTerminated(
            CodePackageActivationId const & activationId,
            DWORD exitCode, 
            bool ignoreReporting);

        void OnHealthCheckStatusChanged(
            CodePackageActivationId const & activationId, 
            ContainerHealthStatusInfo const & healthStatusInfo);

        ServiceModel::CodePackageEntryPoint GetCodePackageEntryPoint();

        void SendDependentCodePackageEvent(
            CodePackageEventDescription const & eventDesc);

        Common::ErrorCode TerminateExternally();

        __declspec(property(get = get_CodePackage)) CodePackage const & CodePackageObj;
        inline CodePackage const & get_CodePackage() const { return this->codePackageHolder_.RootedObject; }

        __declspec(property(get = get_ApplicationHostManager)) ApplicationHostManagerUPtr const & ApplicationHostManagerObj;
        inline ApplicationHostManagerUPtr const & get_ApplicationHostManager() const 
        { 
            return this->CodePackageObj.Hosting.ApplicationHostManagerObj;
        }
    
    protected:
        virtual void OnAbort();

    private:
        void ScheduleStart();
        void OnScheduledStart(Common::AsyncOperationSPtr const & parent);
        void FinishStartCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously);
        void CleanupTimer();
        ServiceModel::CodePackageEntryPointStatistics GetRunStats();
        void GetCodePackageActivationId(__out CodePackageActivationId & activationId);
        void ResetCodePackageActivationId();

    private:
        class StartAsyncOperation;
        class StopAsyncOperation;
        class UpdateAsyncOperation;
        class GetContaninerInfoAsyncOperation;
        class ApplicationHostCodePackageOperation;

    private:
        ServiceModel::EntryPointDescription const entryPoint_;
        std::wstring const runAsId_;
        TerminationCallback const entryPointTerminatedCallback_;
        StartCallback const entryPointStartedCallback_;
        bool isShared_;
        bool isActivator_;
        const bool isSetupEntryPoint_;
        const Common::TimeSpan runInterval_;

        CodePackage::RunStats & stats_;

        CodePackageHolder const codePackageHolder_;
        CodePackageActivationIdUPtr activationId_;
        CodePackageRuntimeInformationSPtr codePackageRuntimeInformation_;

        Common::TimerSPtr runTimer_;
        Common::TimeSpan dueTime_;

        int64 const instanceId_;
    };
}
