//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        //
        // This async operation is used to cleanup the single instance deployment context and the associated
        // application and application type contexts. The cleanup can be triggered 
        // 1. During explicit delete call from user. In this case the state of the single instance deployment's rolloutcontext
        //    is marked 'DeletePending' when we accept the user's call. In this flow, the single instance deployment context's
        //    state becomes deleting and at the end of the cleanup the single insatnce deployment context should be deleted.
        // 2. During rollback of a failed create. In this case the state of the single instance deployment's rolloutcontext
        //    is marked 'Failed'. The application context is rolled back(it might not be present in the store if the failure is during
        //    application create) and the application type context is deleted and the single instance deployment context is left in the failed state,
        //    so that its status can be queried later. The user can subsequently delete the context via the delete api.
        //
        class DeleteSingleInstanceDeploymentContextAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            DeleteSingleInstanceDeploymentContextAsyncOperation(
                __in RolloutManager &,
                __in SingleInstanceDeploymentContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        protected:
            void OnInnerDeleteApplicationComplete(Common::AsyncOperationSPtr const &, bool);

            typedef std::function<void(Common::AsyncOperationSPtr const &)> RetryCallback;
            void TryScheduleRetry(Common::ErrorCode const &, Common::AsyncOperationSPtr const &, RetryCallback const &);
            bool IsRetryable(Common::ErrorCode const & error) const;
            void StartTimer(Common::AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback, Common::TimeSpan const delay);
            Common::ErrorCode RefreshContext();

            SingleInstanceDeploymentContext & singleInstanceDeploymentContext_;        
            ApplicationContext applicationContext_;

        private:
            void DeleteApplication(Common::AsyncOperationSPtr const &);
            void TransitionToUnprovisioning(Common::AsyncOperationSPtr const &, Store::StoreTransaction &);
            virtual void InnerDeleteApplication(Common::AsyncOperationSPtr const &);
            void OnDeleteApplicationComplete(Common::AsyncOperationSPtr const &, bool);

            void UnprovisionApplicationType(Common::AsyncOperationSPtr const &);
            void InnerUnprovisionApplicationType(Common::AsyncOperationSPtr const &);
            void OnInnerUnprovisionComplete(Common::AsyncOperationSPtr const &, bool);
            Common::ErrorCode UpdateContextStatusAfterUnprovision(Store::StoreTransaction &);
            void FinishDelete(Common::AsyncOperationSPtr const &, Store::StoreTransaction &);
            virtual void OnFinishDeleteComplete(Common::AsyncOperationSPtr const &, bool);
            virtual void CompleteDeletion(Common::AsyncOperationSPtr const &);

            Common::ErrorCode GetApplicationContextForDeleting(Store::StoreTransaction const &);
            Common::ErrorCode GetApplicationTypeContextForDeleting(Store::StoreTransaction const &, ApplicationTypeContext &) const;
            Common::ErrorCode GetStoreDataComposeDeploymentInstances(Store::StoreTransaction const &);

            ApplicationTypeContext applicationTypeContext_;
            Common::TimerSPtr timerSPtr_;
            Common::ExclusiveLock timerLock_;

            class InnerDeleteApplicationTypeContextAsyncOperation;
        };
    }
}
