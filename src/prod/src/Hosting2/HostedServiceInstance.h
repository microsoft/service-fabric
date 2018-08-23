// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{    
    class HostedServiceInstance
        : public Common::StateMachine
        , public Common::ComponentRoot,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        DENY_COPY(HostedServiceInstance)

        STATEMACHINE_STATES_08(Created, Scheduling, Scheduled, Starting, Started, Stopping, Stopped, Failed);
        STATEMACHINE_TRANSITION(Scheduling, Created|Starting);
        STATEMACHINE_TRANSITION(Scheduled, Scheduling);
        STATEMACHINE_TRANSITION(Starting, Scheduled);
        STATEMACHINE_TRANSITION(Started, Starting);
        STATEMACHINE_TRANSITION(Stopping, Started|Scheduled);
        STATEMACHINE_TRANSITION(Stopped, Stopping);
        STATEMACHINE_TRANSITION(Failed, Stopping|Scheduling);
        STATEMACHINE_ABORTED_TRANSITION(Scheduled|Started|Failed|Stopped);
        STATEMACHINE_TERMINAL_STATES(Aborted|Stopped);

    public:

        HostedServiceInstance(
            HostedServiceHolder const & serviceHolder,
            std::wstring const & serviceName,
            Common::TimeSpan runInterval,
            HostedService::RunStats & stats,
            Common::RwLock & statsLock,
            SecurityUserSPtr const & runAs = SecurityUserSPtr());


        virtual ~HostedServiceInstance();

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return serviceName_; }

        Common::AsyncOperationSPtr BeginStart(            
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndStart(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginStop(
            bool gracefulStop, 
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        void EndStop(
            Common::AsyncOperationSPtr const & operation);

        void DisableServiceInstance();

        void TerminateServiceInstance();

        __declspec(property(get=get_HostedServiceActivationManager)) HostedServiceActivationManager & ActivationManager;
        HostedServiceActivationManager & get_HostedServiceActivationManager() { return this->hostedServiceHolder_.RootedObject.ActivationManager; }

        __declspec(property(get = get_IProcessActivationContextSPtr)) IProcessActivationContextSPtr & ActivationContext;
        IProcessActivationContextSPtr & get_IProcessActivationContextSPtr() { return activationContext_; }

        //_declspec(property(get=get_ProcessActivator)) ProcessActivatorUPtr const & ProcessActivator;
        //inline ProcessActivatorUPtr const & get_ProcessActivator() const { return this->hostedServiceHolder_.RootedObject.ActivationManager.ProcessActivator; }

        bool UpdateRunStats(DWORD exitCode);

    protected:
        virtual void OnAbort();

    private:
        void ScheduleStart();
        void OnScheduledStart(Common::AsyncOperationSPtr const & parent);
        void FinishStartCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously);

        void CleanupTimer();

        Common::ErrorCode ConfigurePortSecurity(bool cleanup);

        HostedService::RunStats GetRunStats();

        __declspec(property(get=get_HostedService)) HostedService const & HostedServiceObj;
        inline HostedService const & get_HostedService() const;

    private:
        HostedServiceHolder const hostedServiceHolder_;
        std::wstring serviceName_;
        SecurityUserSPtr securityUser_;
        ProcessDescription const & processDescription_;
        IProcessActivationContextSPtr activationContext_;


        const Common::TimeSpan runInterval_;
        Common::TimerSPtr runTimer_;
        HostedService::RunStats & stats_;
        Common::RwLock & statsLock_;

    private:
        class ActivateAsyncOperation;
        friend class ActivateAsyncOperation;

        class DeactivateAsyncOperation;
        friend class DeactivateAsyncOperation;
    };
}
