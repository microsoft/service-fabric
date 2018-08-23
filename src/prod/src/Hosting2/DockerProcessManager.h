// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // 
    // This class manages lifetime of docker service process.
    //
    class DockerProcessManager :
        public Common::RootedObject, 
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        DENY_COPY(DockerProcessManager)

        STATEMACHINE_STATES_08(Inactive, Initializing, Initialized, Terminating, Terminated, Deactivating, Deactivated, Failed);
        STATEMACHINE_TRANSITION(Initializing, Inactive |Terminated);
        STATEMACHINE_TRANSITION(Initialized, Initializing);
        STATEMACHINE_TRANSITION(Terminating, Initialized|Initializing);
        STATEMACHINE_TRANSITION(Terminated, Initializing | Terminating);
        STATEMACHINE_TRANSITION(Deactivating, Initialized | Terminated| Inactive);
        STATEMACHINE_TRANSITION(Deactivated, Deactivating);
        STATEMACHINE_TRANSITION(Failed, Initializing|Deactivating|Terminated);
        STATEMACHINE_ABORTED_TRANSITION(Inactive | Initialized | Terminated | Deactivated | Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Deactivated);

    public:
        _declspec(property(get = get_ActivationManager)) FabricActivationManager const & ActivationManager;
        inline FabricActivationManager const & get_ActivationManager() const;

        //
        // This should be called only after DockerProcessManager has been initialized.
        //
        _declspec(property(get = get_IsDockerServicePresent)) bool IsDockerServicePresent;
        inline bool get_IsDockerServicePresent() const { return isDockerServicePresent_; }

        _declspec(property(get = get_IsDockerServiceManaged)) bool IsDockerServiceManaged;
        inline bool get_IsDockerServiceManaged() const { return isDockerServiceManaged_; }

    public:
        DockerProcessManager(
            Common::ComponentRoot const & root,
            ContainerEngineTerminationCallback const & callback,
            ProcessActivationManager & processActivationManager);

        virtual ~DockerProcessManager();

        Common::AsyncOperationSPtr BeginInitialize(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndInitialize(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDeactivate(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeactivate(
            Common::AsyncOperationSPtr const & operation);

        void OnProcessTerminated(DWORD exitCode, Common::ErrorCode error);

    protected:
        virtual void OnAbort();

    private:
        Common::ErrorCode CheckDockerServicePresence();
        Common::ErrorCode ShutdownDockerService();
        Common::ErrorCode StartDockerService();
        bool IsWinServerOrLinux();
        bool IsFabricHostClosing();
        Common::TimeSpan UpdateExitStatsAndGetDueTime(_Out_ ULONG & continousExitCount);
        void OnContinousExitFailureResetTimeout(TimerSPtr const & timer, ULONG exitCount);
        void ScheduleStart(TimeSpan const & dueTime);
        void CleanupStatTimer();
        Common::ErrorCode ConfigureDockerPidFile(std::wstring const& workingDirectory, _Out_ std::wstring & dockerpidFile);
        void CleanupHNSEndpoints();

    private:
        ProcessActivationManager & processActivationManager_;
        IProcessActivationContextSPtr activationContext_;
        ContainerEngineTerminationCallback callback_;
        Common::RwLock lock_;
        bool isDockerServicePresent_;
        bool isDockerServiceManaged_;
        ULONG continuousExitCount_;
        Common::TimerSPtr resetStatstimer_;

        class DeactivateAsyncOperation;
        class InitializeAsyncOperation;
#ifndef PLATFORM_UNIX
        class HNSEndpointCleanup;
        std::unique_ptr<HNSEndpointCleanup> hnsEndpointCleanup_;
#endif
    };
}
