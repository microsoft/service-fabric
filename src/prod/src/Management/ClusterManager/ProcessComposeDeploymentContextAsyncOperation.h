// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        //
        // This asyncoperation tracks the docker compose application creation. The ComposeDeploymentContext
        // is the persisted context tracking the application creation. The ApplicationTypeContext and
        // ApplicationContext are tracked in memory and persisted after the provision and 
        // after the create application steps are complete.
        //
        class ProcessComposeDeploymentContextAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessComposeDeploymentContextAsyncOperation(
                __in RolloutManager &,
                __in ComposeDeploymentContext &,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            __declspec(property(get=get_Context)) ComposeDeploymentContext & Context;
            ComposeDeploymentContext & get_Context() const { return context_; }

            void OnStart(Common::AsyncOperationSPtr const &) override;

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            class ProcessComposeDeploymentProvisionAsyncOperation;
            class CreateComposeDeploymentContextAsyncOperation;

            void StartProvisionApplicationType(Common::AsyncOperationSPtr const &);
            void OnProvisionComposeDeploymentComplete(Common::AsyncOperationSPtr const &, bool);
            void OnCommitProvisionComposeDeploymentFailureComplete(Common::ErrorCode , Common::AsyncOperationSPtr const &, bool);

            void StartCreateApplication(Common::AsyncOperationSPtr const & thisSPtr);
            void OnCreateApplicationComplete(Common::AsyncOperationSPtr const &, bool);
            void OnCommitCreateComposeDeploymentFailureComplete(Common::ErrorCode , Common::AsyncOperationSPtr const &, bool);
            Common::ErrorCode GetStoreDataComposeDeploymentInstance(__out StoreDataComposeDeploymentInstance &);

            typedef std::function<void(Common::AsyncOperationSPtr const &)> RetryCallback;
            void TryScheduleRetry(Common::ErrorCode const &, Common::AsyncOperationSPtr const &, RetryCallback const &);
            bool IsRetryable(Common::ErrorCode const & error) const;
            void StartTimer(Common::AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback, Common::TimeSpan const delay);
            Common::ErrorCode RefreshContext();

            ComposeDeploymentContext & context_;
            ApplicationTypeContext applicationTypeContext_;
            std::wstring applicationManifestId_;
            ApplicationContext applicationContext_;
            Common::TimerSPtr timerSPtr_;
            Common::ExclusiveLock timerLock_;
        };
    }
}
