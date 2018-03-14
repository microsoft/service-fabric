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
        // This async operation is used to cleanup the compose application context and the associated
        // application and application type contexts. The cleanup can be triggered 
        // 1. During explicit delete call from user. In this case the state of the compose application's rolloutcontext
        //    is marked 'DeletePending' when we accept the user's call. In this flow, the docker compose context's
        //    state becomes unprovisioning->deleting and at the end of the cleanup the compose application context should be deleted.
        // 2. During rollback of a failed create. In this case the state of the compose application's rolloutcontext
        //    is marked 'Failed'. The application context is rolled back(it might not be present in the store if the failure is during
        //    application create) and the application type context is deleted and the compose context is left in the failed state,
        //    so that its status can be queried later. The user can subsequently delete the context via the delete api.
        //
        class DeleteComposeDeploymentContextAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            DeleteComposeDeploymentContextAsyncOperation(
                __in RolloutManager &,
                __in ComposeDeploymentContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            void DeleteApplication(Common::AsyncOperationSPtr const &);
            void TransitionToUnprovisioning(Common::AsyncOperationSPtr const &, Store::StoreTransaction &);
            void InnerDeleteApplication(Common::AsyncOperationSPtr const &);
            void OnInnerDeleteApplicationComplete(Common::AsyncOperationSPtr const &, bool);
            Common::ErrorCode UpdateContextStatusAfterDeleteApplication(Store::StoreTransaction &);
            void OnDeleteApplicationComplete(Common::AsyncOperationSPtr const &, bool);

            void UnprovisionApplicationType(Common::AsyncOperationSPtr const &);
            void InnerUnprovisionApplicationType(Common::AsyncOperationSPtr const &);
            void OnInnerUnprovisionComplete(Common::AsyncOperationSPtr const &, bool);
            Common::ErrorCode UpdateContextStatusAfterUnprovision(Store::StoreTransaction &);
            void FinishDelete(Common::AsyncOperationSPtr const &, Store::StoreTransaction &);
            void OnFinishDeleteComplete(Common::AsyncOperationSPtr const &, bool);

            Common::ErrorCode GetApplicationContextForDeleting(Store::StoreTransaction const &);
            Common::ErrorCode GetApplicationTypeContextForDeleting(Store::StoreTransaction const &, ApplicationTypeContext &) const;
            bool ShouldDeleteApplicationContext();
            Common::ErrorCode GetStoreDataComposeDeploymentInstances(Store::StoreTransaction const &);

            typedef std::function<void(Common::AsyncOperationSPtr const &)> RetryCallback;
            void TryScheduleRetry(Common::ErrorCode const &, Common::AsyncOperationSPtr const &, RetryCallback const &);
            bool IsRetryable(Common::ErrorCode const & error) const;
            void StartTimer(Common::AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback, Common::TimeSpan const delay);
            Common::ErrorCode RefreshContext();

            ComposeDeploymentContext & dockerComposeDeploymentContext_;        
            ApplicationContext applicationContext_;
            ApplicationTypeContext applicationTypeContext_;
            Common::TimerSPtr timerSPtr_;
            Common::ExclusiveLock timerLock_;

            class InnerDeleteApplicationContextAsyncOperation;
            class InnerDeleteApplicationTypeContextAsyncOperation;
        };
    }
}
