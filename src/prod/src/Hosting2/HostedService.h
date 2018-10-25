// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HostedServiceInstance;
    // 
    // This type represents a Hosted service that is described in activator config.
    //
    class HostedService :
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        DENY_COPY(HostedService)

        STATEMACHINE_STATES_07(Inactive, Initializing, Initialized, Updating, Deactivating, Deactivated, Failed);        
        STATEMACHINE_TRANSITION(Initializing, Inactive|Initialized);
        STATEMACHINE_TRANSITION(Initialized, Initializing | Updating);
        STATEMACHINE_TRANSITION(Updating, Initialized);
        STATEMACHINE_TRANSITION(Deactivating, Initialized);
        STATEMACHINE_TRANSITION(Deactivated, Deactivating);
        STATEMACHINE_TRANSITION(Failed, Deactivating|Updating);
        STATEMACHINE_ABORTED_TRANSITION(Initialized|Failed|Deactivated);
        STATEMACHINE_TERMINAL_STATES(Aborted|Deactivated);

    public:
        struct RunStats
        {
            RunStats();

            DWORD LastExitCode;
            Common::DateTime LastActivationTime;
            Common::DateTime LastExitTime;
            Common::DateTime LastSuccessfulActivationTime;
            Common::DateTime LastSuccessfulExitTime;
            ULONG ActivationFailureCount;
            ULONG CountinousActivationFailureCount;
            ULONG ExitFailureCount;
            ULONG ContinousExitFailureCount;
            ULONG ActivationCount;
            ULONG ExitCount;            

            void UpdateProcessActivationStats(bool success);
            void UpdateProcessExitStats(DWORD exitCode);
            ULONG GetMaxContinuousFailureCount();
            void WriteTo( __in Common::TextWriter& w, Common::FormatOptions const &) const;
        };

    public:
        HostedService(
            HostedServiceActivationManagerHolder const & activatorHolder,
            std::wstring const & serviceName,
            std::wstring const & exeName,
            std::wstring const & arguments,
            std::wstring const & workingDirectory,
            std::wstring const & environment, 
            bool ctrlCSpecified,
            std::wstring const & serviceNodeName,
            UINT port = 0,
            std::wstring const & protocol = std::wstring(),
            SecurityUserSPtr const & runAs = SecurityUserSPtr(),
            std::wstring const& sslCertificateFindValue = std::wstring(),
            std::wstring const& sslCertStoreLocation = std::wstring(),
            Common::X509FindType::Enum sslCertificateFindType = Common::X509FindType::FindByThumbprint,
            ServiceModel::ResourceGovernancePolicyDescription const & rgPolicyDescription = ServiceModel::ResourceGovernancePolicyDescription());
        virtual ~HostedService();        

       static void Create(
            HostedServiceActivationManagerHolder const & activatorHolder,
            std::wstring const & serviceName,
            std::wstring const & exeName,
            std::wstring const & arguments,
            std::wstring const & workingDirectory,
            std::wstring const & environment, 
            bool ctrlCSpecified,
            std::wstring const & serviceNodeName,
            UINT port,
            std::wstring const & protocol,
            SecurityUserSPtr const & runAs,
            std::wstring const & sslCertificateFindValue,
            std::wstring const & sslCertStoreLocation,
            Common::X509FindType::Enum sslCertificateFindType,
            ServiceModel::ResourceGovernancePolicyDescription const & rgPolicyDescription,
            __out HostedServiceSPtr & hostedService
            );

        ULONG GetMaxContinuousFailureCount();

        __declspec(property(get=get_RunAs)) SecurityUserSPtr const & RunAs;
        SecurityUserSPtr const & get_RunAs() const { return this->securityUser_; }        

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_ServiceId)) std::wstring const & ServiceId;
        std::wstring const & get_ServiceId() const { return serviceId_; }

        __declspec(property(get = get_ServiceNodeName)) std::wstring const & ServiceNodeName;
        std::wstring const & get_ServiceNodeName() const { return serviceNodeName_; }

        ProcessDescription const & GetProcessDescription();


        Common::AsyncOperationSPtr BeginActivate(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndActivate(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDeactivate(
            bool gracefulStop, 
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeactivate(
            Common::AsyncOperationSPtr const & operation);

         Common::AsyncOperationSPtr BeginUpdate(
            std::wstring const & section,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
         Common::AsyncOperationSPtr BeginUpdate(
            HostedServiceParameters const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndUpdate(
            Common::AsyncOperationSPtr const & operation);

        void TerminateServiceInstance();

        bool RescheduleServiceActivation(DWORD exitCode);

        bool IsRGPolicyUpdateNeeded(Common::StringMap const & entries);
        bool IsRGPolicyUpdateNeeded(HostedServiceParameters const & params);

        void UpdateRGPolicy();

        bool IsUpdateNeeded(SecurityUserSPtr const & secUser, std::wstring const & certificateThumbprint, std::wstring const & arguments);

       __declspec(property(get=get_HostedServiceActivationManager)) HostedServiceActivationManager & ActivationManager;
        HostedServiceActivationManager & get_HostedServiceActivationManager() { return this->activatorHolder_.RootedObject; }

        __declspec(property(get = get_RgPolicyDescription)) ServiceModel::ResourceGovernancePolicyDescription & RgPolicyDescription;
        ServiceModel::ResourceGovernancePolicyDescription & get_RgPolicyDescription() { return rgPolicyDescription_; }

        __declspec(property(get = get_RgPolicyUpdateDescription)) ServiceModel::ResourceGovernancePolicyDescription & RgPolicyUpdateDescription;
        ServiceModel::ResourceGovernancePolicyDescription & get_RgPolicyUpdateDescription() { return rgPolicyUpdateDescription_; }

        __declspec(property(get = get_IsInRgPolicyUpdate)) bool IsInRgPolicyUpdate;
        bool get_IsInRgPolicyUpdate() { return isInRgPolicyUpdate_; }
        
        Common::ErrorCode AddChildService(std::wstring const & childServiceName);
        void ClearChildServices(std::vector<std::wstring> & childServiceNames);

    protected:
        virtual void OnAbort();
        bool CanScheduleUpdate();

    private:

        Common::TimeSpan GetDueTime(RunStats stats, Common::TimeSpan runInterval);

        std::wstring GetServiceNameForCGroupOrJobObject();

        Common::ErrorCode InitializeHostedServiceInstance(); 
        Common::ErrorCode CreateAndStartHostedServiceInstance();;
        Common::ErrorCode UpdateCredentials();
        Common::ErrorCode ConfigurePortSecurity(bool cleanup);

        bool RequireServiceRestart(SecurityUserSPtr const & secUser);
        bool IsSecurityUserUpdated(SecurityUserSPtr const & secUser);
        bool IsThumbprintUpdated(std::wstring const & thumbprint);
        bool IsArgumentsUpdated(std::wstring const & arguments);
        void FinishServiceInstanceStartCompleted(Common::AsyncOperationSPtr const & operation);

    private:        
        HostedServiceActivationManagerHolder const activatorHolder_;
        std::wstring serviceName_;
        std::wstring serviceId_;
        bool disabled_;
        bool ctrlCSpecified_;
        std::wstring serviceNodeName_;
        std::vector<std::wstring> childServiceNames_;
        Common::RwLock childServiceLock_;

        UINT port_;
        ServiceModel::ProtocolType::Enum protocol_;
        bool needPortAcls_;
        ProcessDescriptionUPtr processDescription_;
        SecurityUserSPtr securityUser_;
        std::wstring sslCertStoreLocation_;
        std::wstring sslCertFindValue_;
        Common::X509FindType::Enum sslCertFindType_;

        const Common::TimeSpan runInterval_;
        Common::TimerSPtr runTimer_;
        RunStats stats_;
        Common::RwLock statsLock_;

        Common::TimerSPtr updateTimer_;
        Common::RwLock timerLock_;

        HostedServiceInstanceSPtr hostedServiceInstance_;
        ServiceModel::ResourceGovernancePolicyDescription rgPolicyDescription_;
        ServiceModel::ResourceGovernancePolicyDescription rgPolicyUpdateDescription_;
        bool isInRgPolicyUpdate_;

        class ActivateAsyncOperation;
        class DeactivateAsyncOperation;        
        class UpdateAsyncOperation;

        friend HostedServiceInstance;
        friend HostedServiceActivationManager;
    };
}
