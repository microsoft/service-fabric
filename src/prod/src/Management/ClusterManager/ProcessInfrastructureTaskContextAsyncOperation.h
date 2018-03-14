// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessInfrastructureTaskContextAsyncOperation : 
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessInfrastructureTaskContextAsyncOperation(
                __in RolloutManager &,
                __in InfrastructureTaskContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

        private:
            class ProcessNodesAsyncOperationBase;
            class PreProcessNodesAsyncOperation;
            class PostProcessNodesAsyncOperation;
            class HealthCheckAsyncOperation;

            void ScheduleProcessTask(Common::AsyncOperationSPtr const &, Common::TimeSpan const delay);
            void ScheduleProcessTaskRetry(Common::AsyncOperationSPtr const &, Common::ErrorCode const &);

            Common::ErrorCode RefreshContext();

            void ProcessTask(Common::AsyncOperationSPtr const &);

            void PreProcessNodes(Common::AsyncOperationSPtr const &);
            void OnPreProcessNodesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void ReportPreProcessSuccess(Common::AsyncOperationSPtr const &);
            void OnReportPreProcessSuccessComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void OnPreAcked(Common::AsyncOperationSPtr const &);

            void PostProcessNodes(Common::AsyncOperationSPtr const &);
            void OnPostProcessNodesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void PerformClusterHealthCheck(Common::AsyncOperationSPtr const &);
            void OnPerformClusterHealthCheckComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void OnPostAcked(Common::AsyncOperationSPtr const &);

            void UpdateContext(Common::AsyncOperationSPtr const &);

            void Commit(Common::AsyncOperationSPtr const &, Store::StoreTransaction &&);
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            
            void ReportTaskFailure(Common::AsyncOperationSPtr const &, Common::ErrorCode const &);
            void OnReportTaskFailureComplete(Common::AsyncOperationSPtr const &, Common::ErrorCode const &, bool expectedCompletedSynchronously);

            bool IsRetryable(Common::ErrorCode const &);
            bool IsHealthCheckFailure(Common::ErrorCode const &);

            InfrastructureTaskContext & context_;
            Common::TimerSPtr timerSPtr_;
            Common::ExclusiveLock timerLock_;

            Common::Stopwatch healthCheckStopwatch_;
            bool getStatusOnly_;
        };
    }
}
