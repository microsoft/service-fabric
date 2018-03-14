// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class MigrateDataAsyncOperation : 
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            MigrateDataAsyncOperation(
                __in RolloutManager &,
                Common::ActivityId const &,
                std::vector<ApplicationTypeContext> &&,
                std::vector<ApplicationContext> &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            void MigrateApplicationTypes(Common::AsyncOperationSPtr const &);
            void OnCommitManifestsComplete(size_t index, Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void ScheduleMigrateApplicationTypesOrComplete(Common::AsyncOperationSPtr const &, Common::ErrorCode const &);

            void MigrateApplications(Common::AsyncOperationSPtr const &);
            void OnReportApplicationPolicyComplete(__in ApplicationContext &, Common::AsyncOperationSPtr const &, Common::ErrorCode const &);
            void OnCommitApplicationContextComplete(__in ApplicationContext &, Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void FinishMigrateApplications(__in ApplicationContext &, Common::AsyncOperationSPtr const &, Common::ErrorCode const &);
            void ScheduleMigrateApplicationsOrComplete(Common::AsyncOperationSPtr const &, Common::ErrorCode const &);

            bool TryPrepareForRetry(Common::ErrorCode const &);

            Common::TimeSpan timeout_;
            Common::ErrorCode error_;
            Common::atomic_long pendingMigrations_;
            std::vector<ApplicationTypeContext> applicationTypesToMigrate_;
            std::vector<ApplicationContext> applicationsToMigrate_;

            std::vector<StoreDataApplicationManifest> appManifestData_;
            std::vector<StoreDataServiceManifest> serviceManifestData_;
            Common::TimerSPtr retryTimer_;
            Common::ExclusiveLock timerLock_;
        };
    }
}
