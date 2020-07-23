// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // 
    // This type represents a service that is part of application.
    //
    class ApplicationService :
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        DENY_COPY(ApplicationService)

        STATEMACHINE_STATES_06(Inactive, Starting, Started, Stopping, Stopped, Failed);        
        STATEMACHINE_TRANSITION(Starting, Inactive);
        STATEMACHINE_TRANSITION(Started, Starting);
        STATEMACHINE_TRANSITION(Stopping, Started);
        STATEMACHINE_TRANSITION(Stopped, Stopping);
        STATEMACHINE_TRANSITION(Failed, Starting|Stopping);
        STATEMACHINE_ABORTED_TRANSITION(Started|Stopped|Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Stopped);

    public:
        ApplicationService(
            ProcessActivationManagerHolder const & activatorHolder,
            std::wstring const & parentId,
            std::wstring const & appServiceId,
            ProcessDescription const & processDescription,
            std::wstring const & fabricBinFolder_,
            SecurityUserSPtr const & runAs = SecurityUserSPtr(),
            BOOL containerHost = false,
            ContainerDescription const & containerDescription = ContainerDescription());
        virtual ~ApplicationService();        


        __declspec(property(get=get_RunAs)) SecurityUserSPtr const & RunAs;
        SecurityUserSPtr const & get_RunAs() const { return this->securityUser_; }        

        __declspec(property(get=get_ApplicationServiceId)) std::wstring const & ApplicationServiceId;
        std::wstring const & get_ApplicationServiceId() const { return appServiceId_; }

        __declspec(property(get = get_ActivationContext)) IProcessActivationContextSPtr const & ActivationContext;
        IProcessActivationContextSPtr const & get_ActivationContext() const { return activationContext_; }

        __declspec(property(get=get_ParentId)) std::wstring const & ParentId;
        std::wstring const & get_ParentId() const { return parentId_; }

        __declspec(property(get = get_ContainerId)) std::wstring const & ContainerId;
        std::wstring const & get_ContainerId() const { return containerId_; }

        __declspec(property(get = get_ContainerDescription)) ContainerDescription const & ContainerDescriptionObj;
        ContainerDescription const & get_ContainerDescription() const { return containerDescription_; }

        __declspec(property(get = get_ProcessDescription)) ProcessDescription const & ProcessDescriptionObj;
        ProcessDescription const & get_ProcessDescription() const { return processDescription_; }

        __declspec(property(get = get_IsContainerHost)) BOOL IsContainerHost;
        BOOL get_IsContainerHost() const { return isContainerHost_; }

        __declspec(property(get = get_IsContainerRoot)) BOOL IsContainerRoot;
        BOOL get_IsContainerRoot() const { return (isContainerHost_ && containerDescription_.IsContainerRoot); }

        __declspec(property(get=get_ProcessActivationManager)) ProcessActivationManager & ActivationManager;
        ProcessActivationManager & get_ProcessActivationManager() { return this->activatorHolder_.RootedObject; }


        Common::AsyncOperationSPtr BeginActivate(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndActivate(
            Common::AsyncOperationSPtr const & operation,
            DWORD & processId);

        Common::AsyncOperationSPtr BeginDeactivate(
            bool isGraceful,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeactivate(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginMeasureResourceUsage(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndMeasureResourceUsage(
            Common::AsyncOperationSPtr const & operation,
            Management::ResourceMonitor::ResourceMeasurement & resourceMeasurement);

        void OnServiceProcessTerminated(DWORD exitCode);

        Common::ErrorCode GetProcessId(__out DWORD & processId);

        bool IsApplicationHostProcess(DWORD processId);
        void ResumeProcessIfNeeded();

        bool IsSecurityUserLocalSystem();

        void SetTerminatedExternally(BOOL terminatedExternally);

        bool Test_VerifyLimitsEnforced(std::vector<double> & cpuLimitCP, std::vector<uint64> & memoryLimitCP);
        bool Test_VerifyContainerGroupSettings();


    protected:
        virtual void OnAbort();

    private:
        ProcessActivatorUPtr const & GetProcessActivator() const;

        void CleanupContainerNetworks();

    private:        
        ProcessActivationManagerHolder const activatorHolder_;
        std::wstring appServiceId_;
        std::wstring const parentId_;
        ProcessDescription processDescription_;
        SecurityUserSPtr securityUser_;
        std::wstring fabricBinFolder_;
        ContainerDescription containerDescription_;
        BOOL isContainerHost_;
        Common::atomic_bool terminatedExternally_;
        std::wstring containerId_;
        IProcessActivationContextSPtr activationContext_;
        Common::RwLock rwlock_;

        class ActivateAsyncOperation;
        class DeactivateAsyncOperation;
        class ResumeAsyncOperation;
        class MeasureResourceUsageAsyncOperation;
    };
}