// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessFabricUpgradeContextAsyncOperation : 
            public ProcessUpgradeContextAsyncOperationBase<FabricUpgradeContext, Reliability::UpgradeFabricRequestMessageBody>
        {
        public:

            // ***************************************************************
            // Upgrades are processed slightly differently from other requests
            // (CreateApplication, CreateService, etc.). An upgrade operation
            // will complete the client request once the operation has been
            // accepted (i.e. will failover correctly) by the CM.
            //
            // The upgrade operation will then be processed in the background
            // by the CM. The client is expected to poll the CM for the upgrade
            // status.
            // ***************************************************************
            //
            ProcessFabricUpgradeContextAsyncOperation(
                __in RolloutManager &,
                __in FabricUpgradeContext &,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        protected:

            virtual Reliability::RSMessage const & GetUpgradeMessageTemplate();
            virtual Common::TimeSpan GetUpgradeStatusPollInterval();
            virtual Common::TimeSpan GetHealthCheckInterval();

            virtual Common::ErrorCode LoadRollforwardMessageBody(__out Reliability::UpgradeFabricRequestMessageBody &);
            virtual Common::ErrorCode LoadRollbackMessageBody(__out Reliability::UpgradeFabricRequestMessageBody &);

            virtual void TraceQueryableUpgradeStart();
            virtual void TraceQueryableUpgradeDomainComplete(std::vector<std::wstring> & recentlyCompletedUDs);
            virtual std::wstring GetTraceAnalysisContext();

            virtual Common::AsyncOperationSPtr BeginInitializeUpgrade(
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndInitializeUpgrade(Common::AsyncOperationSPtr const &);

            virtual Common::AsyncOperationSPtr BeginOnValidationError(Common::AsyncCallback const &, Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndOnValidationError(Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode OnFinishStartUpgrade(Store::StoreTransaction const &);
            virtual Common::ErrorCode OnProcessFMReply(Store::StoreTransaction const &);
            virtual Common::ErrorCode OnRefreshUpgradeContext(Store::StoreTransaction const &);

            virtual void FinalizeUpgrade(Common::AsyncOperationSPtr const &) override;
            virtual void FinalizeRollback(Common::AsyncOperationSPtr const &) override;

            virtual Common::AsyncOperationSPtr BeginUpdateHealthPolicyForHealthCheck(
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndUpdateHealthPolicy(Common::AsyncOperationSPtr const &);

            virtual Common::ErrorCode EvaluateHealth(
                Common::AsyncOperationSPtr const &,
                __out bool & isHealthy, 
                __out std::vector<ServiceModel::HealthEvaluation> &);

            virtual Common::ErrorCode LoadVerifiedUpgradeDomains(
                __in Store::StoreTransaction &,
                __out std::vector<std::wstring> &);
            virtual Common::ErrorCode UpdateVerifiedUpgradeDomains(
                __in Store::StoreTransaction &,
                std::vector<std::wstring> &&);

        private:

            //
            // Upgrade success
            //

            void PrepareFinishUpgrade(Common::AsyncOperationSPtr const &);
            void OnPrepareFinishUpgradeComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void FinishUpgrade(Common::AsyncOperationSPtr const &);
            void OnFinishUpgradeCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void TrySchedulePrepareFinishUpgrade(Common::ErrorCode const &, Common::AsyncOperationSPtr const &);

        private:

            //
            // Upgrade rollback
            //

            void PrepareStartRollback(Common::AsyncOperationSPtr const &);
            void OnPrepareStartRollbackComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void StartRollback(Common::AsyncOperationSPtr const &);
            void OnStartRollbackCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void TrySchedulePrepareStartRollback(Common::ErrorCode const &, Common::AsyncOperationSPtr const &);

            void OnFinalizeRollbackCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void TryScheduleFinalizeRollback(Common::ErrorCode const &, Common::AsyncOperationSPtr const &);

        private:
            class InitializeUpgradeAsyncOperation;

            //
            // Misc. helpers
            //

            Common::AsyncOperationSPtr BeginUpdateHealthPolicy(
                ServiceModel::ClusterHealthPolicySPtr const &, 
                ServiceModel::ClusterUpgradeHealthPolicySPtr const &,
                ServiceModel::ApplicationHealthPolicyMapSPtr const &,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode LoadMessageBody(
                Common::FabricVersion const &,
                Common::TimeSpan const replicaSetCheckTimeout,
                uint64 instanceId,
                bool isMonitored,
                bool isManual,
                bool isRollback,
                __out Reliability::UpgradeFabricRequestMessageBody &);

            std::unique_ptr<StoreDataClusterUpgradeStateSnapshot> clusterStateSnapshotData_;
        };
    }
}
