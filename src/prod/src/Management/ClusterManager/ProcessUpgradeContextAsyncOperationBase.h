// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        template <class TUpgradeContext, class TUpgradeRequestBody>
        class ProcessUpgradeContextAsyncOperationBase : public ProcessRolloutContextAsyncOperationBase
        {
        public:

            ProcessUpgradeContextAsyncOperationBase(
                __in RolloutManager &,
                __in TUpgradeContext &,
                std::string const & traceComponent,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ProcessUpgradeContextAsyncOperationBase() { }

            virtual void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        protected:

            //
            // Properties
            //

            __declspec (property(get=get_UpgradeContext)) TUpgradeContext & UpgradeContext;
            TUpgradeContext & get_UpgradeContext() const { return upgradeContext_; }

            __declspec (property(get=get_TraceComponent)) Common::StringLiteral TraceComponent;
            Common::StringLiteral get_TraceComponent() const { return Common::StringLiteral(&traceComponent_[0], &traceComponent_[traceComponent_.size()]); }

            __declspec (property(get=get_IsConfigOnly,put=set_IsConfigOnly)) bool IsConfigOnly;
            bool get_IsConfigOnly() const { return isConfigOnly_; }
            void set_IsConfigOnly(bool value) { isConfigOnly_ = value; }

            __declspec (property(get = get_HealthAggregator)) HealthManager::IHealthAggregator & HealthAggregator;
            HealthManager::IHealthAggregator & get_HealthAggregator() const { return *healthAggregator_; }

        protected:

            struct UpgradeReplyDetails;
            typedef std::shared_ptr<UpgradeReplyDetails> UpgradeReplyDetailsSPtr;

            void TryRefreshContextAndScheduleRetry(
                Common::ErrorCode const & error,
                Common::AsyncOperationSPtr const & thisSPtr,
                RetryCallback const & callback);

            void TryScheduleRetry(
                Common::ErrorCode const & error,
                Common::AsyncOperationSPtr const & thisSPtr,
                RetryCallback const & callback);

            void ResetUpgradeContext(TUpgradeContext &&);

            //
            // Minor differences between Application and Fabric upgrade
            //

            virtual Reliability::RSMessage const & GetUpgradeMessageTemplate() = 0;
            virtual Common::TimeSpan GetUpgradeStatusPollInterval() = 0;
            virtual Common::TimeSpan GetHealthCheckInterval() = 0;

            virtual Common::ErrorCode LoadRollforwardMessageBody(__out TUpgradeRequestBody &) = 0;
            virtual Common::ErrorCode LoadRollbackMessageBody(__out TUpgradeRequestBody &) = 0;

            virtual void TraceQueryableUpgradeStart() = 0;
            virtual void TraceQueryableUpgradeDomainComplete(std::vector<std::wstring> & recentlyCompletedUDs) = 0;
            virtual std::wstring GetTraceAnalysisContext() = 0;

            virtual Common::AsyncOperationSPtr BeginInitializeUpgrade(
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) = 0;
            virtual Common::ErrorCode EndInitializeUpgrade(Common::AsyncOperationSPtr const &) = 0;

            virtual Common::AsyncOperationSPtr BeginOnValidationError(Common::AsyncCallback const &, Common::AsyncOperationSPtr const &) = 0;
            virtual Common::ErrorCode EndOnValidationError(Common::AsyncOperationSPtr const &) = 0;
            virtual Common::ErrorCode OnFinishStartUpgrade(Store::StoreTransaction const &) = 0;
            virtual Common::ErrorCode OnProcessFMReply(Store::StoreTransaction const &) = 0;
            virtual Common::ErrorCode OnRefreshUpgradeContext(Store::StoreTransaction const &) = 0;

            virtual void FinalizeUpgrade(Common::AsyncOperationSPtr const &) = 0;

            virtual Common::AsyncOperationSPtr BeginUpdateHealthPolicyForHealthCheck(
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) = 0;
            virtual Common::ErrorCode EndUpdateHealthPolicy(Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode EvaluateHealth(
                Common::AsyncOperationSPtr const &,
                __out bool & isHealthy, 
                __out std::vector<ServiceModel::HealthEvaluation> &) = 0;

            virtual Common::ErrorCode LoadVerifiedUpgradeDomains(
                __in Store::StoreTransaction &,
                __out std::vector<std::wstring> &) = 0;
            virtual Common::ErrorCode UpdateVerifiedUpgradeDomains(
                __in Store::StoreTransaction &,
                std::vector<std::wstring> &&) = 0;

            virtual void StartSendRequest(Common::AsyncOperationSPtr const &);
            virtual void PrepareStartRollback(Common::AsyncOperationSPtr const &) = 0;
            virtual void FinalizeRollback(Common::AsyncOperationSPtr const &) = 0;

        protected:

            void RestartUpgrade(Common::AsyncOperationSPtr const &);

        private:
            
            // Common upgrade workflow (CM/FM protocol and health checks).
            //
            // FinishRequestToFM() is the entry point for the remainder
            // of the workflow when completing or rolling back an upgrade
            // after the CM/FM protocol has completed.
            //
            // There is less common code between Application and Fabric
            // upgrades when finishing up.
            //

            void StartUpgrade(Common::AsyncOperationSPtr const &);
            void OnInitializeUpgradeComplete(
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously);

        protected:
            virtual void FinishStartUpgrade(Common::AsyncOperationSPtr const &);
        private:
            void OnFinishStartUpgradeCommitComplete(
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously);

            void RequestToFM(Common::AsyncOperationSPtr const &);
            void OnRequestToFMComplete(
                Common::AsyncOperationSPtr const &, 
                bool expectedCompletedSynchronously);

            void ProcessFMReply(
                UpgradeReplyDetailsSPtr &&, 
                Common::AsyncOperationSPtr const &);
            void OnFMReplyCommitComplete(
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously);

            void FinishRequestToFM(Common::AsyncOperationSPtr const &);

            void CheckHealth(Common::AsyncOperationSPtr const &);

            void UpdateHealthPolicy(Common::AsyncOperationSPtr const &);
            void OnUpdateHealthPolicyComplete(
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously);

            void CommitCheckHealthProgress(bool useDelay, Common::AsyncOperationSPtr const &);
            void OnCheckHealthCommitComplete(
                bool useDelay,
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously);

            void FinishCheckHealth(Common::AsyncOperationSPtr const &);
            void OnFinishCheckHealthCommitComplete(
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously);

            void FailCheckHealth(Common::AsyncOperationSPtr const &);
            void OnFailCheckHealthCommitComplete(
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously);

            void ScheduleHandleValidationFailure(Common::ErrorCode const & validationError, Common::AsyncOperationSPtr const &);
            void HandleValidationFailure(Common::ErrorCode const & validationError, Common::AsyncOperationSPtr const &);
            void OnHandleValidationFailureComplete(Common::ErrorCode const & validationError, Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        private:

            //
            // Upgrade workflow helper functions
            //

            class CommitUpgradeContextAsyncOperation;

        protected:

            ServiceModel::UpgradeType::Enum GetUpgradeType() const;

            Common::TimeSpan GetRollforwardReplicaSetCheckTimeout() const;
            Common::TimeSpan GetRollbackReplicaSetCheckTimeout() const;

            Common::AsyncOperationSPtr BeginUpdateAndCommitUpgradeContext(
                Store::StoreTransaction &&,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginResetUpgradeDomainAndCommitUpgradeContext(
                Store::StoreTransaction &&,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginCommitUpgradeContext(
                Store::StoreTransaction &&,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode EndCommitUpgradeContext(
                Common::AsyncOperationSPtr const &);

            template <class TStoreData>
            Common::ErrorCode ClearVerifiedUpgradeDomains(Store::StoreTransaction const &, __inout TStoreData &);

        protected:

            //
            // Tracing and Timer helper functions
            //

            using RetryCallback = std::function<void(Common::AsyncOperationSPtr const &)>;

            void ResetImageStoreErrorCount();
            Common::ErrorCode RefreshUpgradeContext();
            Common::ErrorCode TraceAndGetErrorDetails(
                Common::ErrorCodeValue::Enum, 
                std::wstring && msg) const;
            void SetCommonContextDataForUpgrade();
            bool TrySetCommonContextDataForFailure();
            void SetCommonContextDataForRollback(bool goalStateExists);

            bool IsHealthCheckWaitDurationElapsed() const;
            bool IsHealthCheckStableDurationElapsed() const;
            bool IsHealthCheckRetryTimeoutExpired() const;
            bool IsOverallUpgradeTimeoutExpired() const;
            bool IsUpgradeDomainTimeoutExpired() const;
            bool IsUpgradeTimeoutExpired() const;
            bool IsUpgradeTimeoutExpired(__out UpgradeFailureReason::Enum &) const;

            Common::TimeSpan GetCommunicationTimeout() const;
            Common::TimeSpan GetImageBuilderTimeout() const;

            bool ShouldScheduleRetry(Common::ErrorCode const &, Common::AsyncOperationSPtr const &);
            bool IsRetryable(Common::ErrorCode const &);
            bool IsFmReplyRetryable(Common::ErrorCode const &) const;
            bool IsValidationError(Common::ErrorCode const &) const;
            bool IsServiceOperationFatalError(Common::ErrorCode const &) const;
            void StartTimer(Common::AsyncOperationSPtr const &, RetryCallback const &, Common::TimeSpan const);

            std::wstring GetUpgradeContextStringTrace() const;

        protected:

            //
            // Retry helper functions
            //

            void TryScheduleStartUpgrade(
                Common::ErrorCode const &,
                Common::AsyncOperationSPtr const &);

            void ScheduleRequestToFM(Common::AsyncOperationSPtr const &);
            void ScheduleRequestToFM(Common::TimeSpan const delay, Common::AsyncOperationSPtr const &);

            void ScheduleCheckHealth(Common::TimeSpan const delay, Common::AsyncOperationSPtr const &);

            bool TryGetRequestForFM(Common::AsyncOperationSPtr const &, __out Transport::MessageUPtr &);
            void ModifyRequestToFMBodies();
            Common::ErrorCode LoadRollbackMessageBody();

        private:

            // Needs to be refreshed every iteration
            TUpgradeContext & upgradeContext_;

            // Needs to be repopulated on failover
            TUpgradeRequestBody rollforwardRequestBody_;
            TUpgradeRequestBody rollbackRequestBody_;

            std::string traceComponent_;

            std::vector<std::wstring> verifiedUpgradeDomains_;
            int imageStoreErrorCount_;
            bool isConfigOnly_;

            Common::TimerSPtr timerSPtr_;
            Common::ExclusiveLock timerLock_;

            Common::Stopwatch healthCheckProgressStopwatch_;
            Common::Stopwatch upgradeProgressStopwatch_;

            HealthManager::IHealthAggregatorSPtr healthAggregator_;
        };
    }
}
