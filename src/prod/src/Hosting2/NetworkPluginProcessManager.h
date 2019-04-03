// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // 
    // This class manages lifetime of the specified network plugin process.
    //
    class NetworkPluginProcessManager :
        public INetworkPluginProcessManager,
        public Common::RootedObject, 
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(NetworkPluginProcessManager)

    public:
        NetworkPluginProcessManager(
            Common::ComponentRoot const & root,
            ProcessActivationManager & processActivationManager,
            NetworkPluginProcessRestartedCallback const & callback,
            std::wstring const & pluginName,
            std::wstring const & pluginSockPath,
            bool pluginSetup,
            Common::TimeSpan initTimeout,
            Common::TimeSpan activationExceptionInterval,
            Common::TimeSpan activationRetryBackoffInterval,
            int activationMaxFailureCount
        );

        virtual ~NetworkPluginProcessManager();

    protected:

        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

    private:
        void OnProcessTerminated(DWORD exitCode, Common::ErrorCode error);
        void OnProcessStartCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
        void CleanupTimer();
        void RestartPlugin();
        void ShutdownPlugin();
        bool IsFabricHostClosing();

    private:        
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        ProcessActivationManager & processActivationManager_;
        NetworkPluginProcessRestartedCallback networkPluginProcessRestartedCallback_;
        IProcessActivationContextSPtr activationContext_;
        DateTime lastProcessTerminationTime_;
        Common::TimerSPtr retryTimer_;
        Common::RwLock retryTimerLock_;
        bool retryOnFailure_;
        int retryCount_;
        wstring pluginName_;
        wstring pluginSockPath_;
        bool pluginSetup_;
        Common::TimeSpan initTimeout_;
        Common::TimeSpan activationExceptionInterval_;
        Common::TimeSpan activationRetryBackoffInterval_;
        int activationMaxFailureCount_;
    };
}