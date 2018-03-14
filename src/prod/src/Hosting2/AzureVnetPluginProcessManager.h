// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // 
    // This class manages lifetime of the azure vnet plugin process.
    //
    class AzureVnetPluginProcessManager :
        public IAzureVnetPluginProcessManager,
        public Common::RootedObject, 
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(AzureVnetPluginProcessManager)

    public:
        AzureVnetPluginProcessManager(
            Common::ComponentRootSPtr const & root,
            ProcessActivationManager & processActivationManager);

        virtual ~AzureVnetPluginProcessManager();

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
        void RestartAzureVnetPlugin();
        void ShutdownAzureVnetPlugin();

    private:        
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        ProcessActivationManager & processActivationManager_;
        DateTime lastProcessTerminationTime_;
        Common::TimerSPtr retryTimer_;
        Common::RwLock retryTimerLock_;
        bool retryOnFailure_;
        int retryCount_;
    };
}
