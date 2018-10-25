//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        // This async operation tracks single instance deployment.
        // SingleInstanceDeploymentContext is the persisted context tracking creation.
        // ApplicationTypeContext and ApplicationContext were created before hand and will be updated after provision and application creation completes.
        // Upon creation failure, ApplicationTypeContext and ApplicationContext will need to be deleted and recycled.
        class ProcessSingleInstanceDeploymentContextAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessSingleInstanceDeploymentContextAsyncOperation(
                __in RolloutManager &,
                __in SingleInstanceDeploymentContext &,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            __declspec(property(get=get_Context)) SingleInstanceDeploymentContext & Context;
            SingleInstanceDeploymentContext & get_Context() const { return context_;}

            void OnStart(Common::AsyncOperationSPtr const &) override;

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);
        
        private:
            class CreateSingleInstanceApplicationContextAsyncOperation;

            void BuildApplicationPackage(Common::AsyncOperationSPtr const &);
            void OnCreateApplicationComplete(Common::AsyncOperationSPtr const &, bool);
            void OnCommitCreateDeploymentFailureComplete(Common::ErrorCode const &, Common::AsyncOperationSPtr const &, bool);
            void TryScheduleRetry(Common::ErrorCode const &, Common::AsyncOperationSPtr const &, RetryCallback const &);
            bool IsRetryable(Common::ErrorCode const & error) const;
            void StartTimer(Common::AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback, Common::TimeSpan const delay);
            Common::ErrorCode RefreshContext();

            SingleInstanceDeploymentContext & context_;
            ApplicationContext applicationContext_;
            ApplicationTypeContext applicationTypeContext_;
            Common::TimerSPtr timerSPtr_;
            Common::ExclusiveLock timerLock_;
        };
    }
}
