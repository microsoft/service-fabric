// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class FabricUpgradeContext : public RolloutContext
        {
            DENY_COPY(FabricUpgradeContext)

        public:
            static const RolloutContextType::Enum ContextType;

            FabricUpgradeContext();

            FabricUpgradeContext(
                FabricUpgradeContext &&);

            FabricUpgradeContext & operator=(
                FabricUpgradeContext &&);

            FabricUpgradeContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                FabricUpgradeDescription const &,
                Common::FabricVersion const &,
                uint64 upgradeInstance);

            FabricUpgradeContext(
                Common::ComponentRoot const &,
                Store::ReplicaActivityId const &,
                FabricUpgradeDescription const &,
                Common::FabricVersion const &,
                uint64 upgradeInstance);

            virtual ~FabricUpgradeContext();

            __declspec(property(get=get_UpgradeDescription)) FabricUpgradeDescription const & UpgradeDescription;
            FabricUpgradeDescription const & get_UpgradeDescription() const { return upgradeDescription_; }
            FabricUpgradeDescription && TakeUpgradeDescription() { return std::move(upgradeDescription_); }

            __declspec(property(get=get_MutableUpgradeDescription)) FabricUpgradeDescription & MutableUpgradeDescription;
            FabricUpgradeDescription & get_MutableUpgradeDescription() { return upgradeDescription_; }

            __declspec(property(get=get_UpgradeState, put=set_UpgradeState)) FabricUpgradeState::Enum const & UpgradeState;
            FabricUpgradeState::Enum const & get_UpgradeState() const { return upgradeState_; }
            void set_UpgradeState(FabricUpgradeState::Enum value) { upgradeState_ = value; }

            __declspec(property(get=get_CurrentVersion)) Common::FabricVersion const & CurrentVersion;
            Common::FabricVersion const & get_CurrentVersion() const { return currentVersion_; }

            __declspec(property(get=get_InProgressUpgradeDomain)) std::wstring const & InProgressUpgradeDomain;
            std::wstring const & get_InProgressUpgradeDomain() const { return inProgressUpgradeDomain_; }

            __declspec(property(get=get_CompletedUpgradeDomains)) std::vector<std::wstring> const & CompletedUpgradeDomains;
            std::vector<std::wstring> const & get_CompletedUpgradeDomains() const { return completedUpgradeDomains_; }

            __declspec(property(get=get_PendingUpgradeDomains)) std::vector<std::wstring> const & PendingUpgradeDomains;
            std::vector<std::wstring> const & get_PendingUpgradeDomains() const { return pendingUpgradeDomains_; }

            __declspec(property(get=get_RollforwardInstance)) uint64 RollforwardInstance;
            uint64 get_RollforwardInstance() const { return upgradeInstance_; }

            __declspec(property(get=get_RollbackInstance)) uint64 RollbackInstance;
            uint64 get_RollbackInstance() const { return (upgradeInstance_ + 1); }

            __declspec(property(get=get_UpgradeCompletionInstance)) uint64 UpgradeCompletionInstance;
            uint64 get_UpgradeCompletionInstance() const { return (upgradeInstance_ + 2); }

            __declspec(property(get=get_IsUpgradeCompletedState)) bool IsUpgradeCompletedState;
            bool get_IsUpgradeCompletedState() const 
            { 
                return upgradeState_ == FabricUpgradeState::CompletedRollforward || 
                    upgradeState_ == FabricUpgradeState::CompletedRollback; 
            }

            __declspec(property(get = get_IsAutoBaseline)) bool IsAutoBaseline;
            bool get_IsAutoBaseline() const { return isAutoBaseline_; }

            __declspec(property(get=get_IsRollforwardState)) bool IsRollforwardState;
            bool get_IsRollforwardState() const { return upgradeState_ == FabricUpgradeState::RollingForward; }

            __declspec(property(get=get_IsRollbackState)) bool IsRollbackState;
            bool get_IsRollbackState() const { return upgradeState_ == FabricUpgradeState::RollingBack; }

            __declspec(property(get=get_IsInterrupted)) bool IsInterrupted;
            bool get_IsInterrupted() const { return isInterrupted_; }

            __declspec(property(get=get_IsFMRequestModified)) bool IsFMRequestModified;
            bool get_IsFMRequestModified() const { return isFMRequestModified_; }
            void ClearIsFMRequestModified() { isFMRequestModified_ = false; }

            __declspec(property(get=get_IsHealthPolicyModified)) bool IsHealthPolicyModified;
            bool get_IsHealthPolicyModified() const { return isHealthPolicyModified_; }
            void SetIsHealthPolicyModified(bool value) { isHealthPolicyModified_ = value; }

            __declspec(property(get=get_LastHealthCheckResult)) bool LastHealthCheckResult;
            bool get_LastHealthCheckResult() const { return lastHealthCheckResult_; }

            __declspec(property(get=get_HealthCheckElapsedTime)) Common::TimeSpan const HealthCheckElapsedTime;
            Common::TimeSpan const get_HealthCheckElapsedTime() const { return healthCheckElapsedTime_; }

            __declspec(property(get=get_OverallUpgradeElapsedTime)) Common::TimeSpan const OverallUpgradeElapsedTime;
            Common::TimeSpan const get_OverallUpgradeElapsedTime() const { return overallUpgradeElapsedTime_; }

            __declspec(property(get=get_UpgradeDomainElapsedTime)) Common::TimeSpan const UpgradeDomainElapsedTime;
            Common::TimeSpan const get_UpgradeDomainElapsedTime() const { return upgradeDomainElapsedTime_; }

            __declspec(property(get=get_ManifestHealthPolicy)) ServiceModel::ClusterHealthPolicy const ManifestHealthPolicy;
            ServiceModel::ClusterHealthPolicy const get_ManifestHealthPolicy() const { return manifestHealthPolicy_; }

            __declspec(property(get = get_ManifestUpgradeHealthPolicy)) ServiceModel::ClusterUpgradeHealthPolicy const ManifestUpgradeHealthPolicy;
            ServiceModel::ClusterUpgradeHealthPolicy const get_ManifestUpgradeHealthPolicy() const { return manifestUpgradeHealthPolicy_; }

            __declspec(property(get=get_HealthEvaluationReasons)) std::vector<ServiceModel::HealthEvaluation> const & UnhealthyEvaluations;
            std::vector<ServiceModel::HealthEvaluation> const & get_HealthEvaluationReasons() const { return unhealthyEvaluations_; }
            std::vector<ServiceModel::HealthEvaluation> && TakeUnhealthyEvaluations() { return std::move(unhealthyEvaluations_); }

            void OnFailRolloutContext() override { } // no-op

            __declspec(property(get=get_IsPreparingUpgrade)) bool IsPreparingUpgrade;
            bool get_IsPreparingUpgrade() const { return isPreparingUpgrade_; }
            void ResetPreparingUpgrade() { isPreparingUpgrade_ = false; }

            __declspec(property(get=get_CurrentUpgradeDomainProgress)) Reliability::UpgradeDomainProgress const & CurrentUpgradeDomainProgress;
            Reliability::UpgradeDomainProgress const & get_CurrentUpgradeDomainProgress() const { return currentUpgradeDomainProgress_; }
            Reliability::UpgradeDomainProgress && TakeCurrentUpgradeDomainProgress() { return std::move(currentUpgradeDomainProgress_); }

            __declspec(property(get=get_CommonUpgradeContextData)) ClusterManager::CommonUpgradeContextData & CommonUpgradeContextData;
            ClusterManager::CommonUpgradeContextData & get_CommonUpgradeContextData() { return commonUpgradeContextData_; }
            ClusterManager::CommonUpgradeContextData && TakeCommonUpgradeContextData() { return std::move(commonUpgradeContextData_); }

            void SetHealthCheckElapsedTimeToWaitDuration(Common::TimeSpan const elapsed);
            void SetLastHealthCheckResult(bool isHealthy);
            void UpdateHealthCheckElapsedTime(Common::TimeSpan const elapsed);
            void UpdateHealthMonitoringTimeouts(Common::TimeSpan const elapsed);
            void UpdateHealthEvaluationReasons(std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations);
            void ClearHealthEvaluationReasons();
            
            void ResetUpgradeDomainTimeouts();
            void SetRollingBack(bool goalStateExists);
            void RevertToManualUpgrade();

            void MarkAsBaseline();
            
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            void UpdateInProgressUpgradeDomain(std::wstring && upgradeDomain);
            void UpdateCompletedUpgradeDomains(std::vector<std::wstring> && upgradeDomains);
            void UpdatePendingUpgradeDomains(std::vector<std::wstring> && upgradeDomains);

            void UpdateUpgradeProgress(Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress);

            Common::ErrorCode StartUpgrading(
                Store::StoreTransaction const &,
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                FabricUpgradeDescription const &);

            Common::ErrorCode CompleteRollforward(Store::StoreTransaction const &);
            Common::ErrorCode CompleteRollback(Store::StoreTransaction const &);

            void RefreshUpgradeInstance(uint64 instance);

            bool TryInterrupt();
            bool TryUpdateHealthPolicies(FabricUpgradeDescription const &);
            bool TryModifyUpgrade(
                FabricUpgradeUpdateDescription const &,
                __out std::wstring & validationErrorMessage);
                
            FABRIC_FIELDS_21(
                upgradeDescription_, 
                upgradeState_, 
                currentVersion_,
                inProgressUpgradeDomain_,
                completedUpgradeDomains_,
                pendingUpgradeDomains_,
                upgradeInstance_,
                isInterrupted_,
                healthCheckElapsedTime_,
                overallUpgradeElapsedTime_,
                upgradeDomainElapsedTime_,
                manifestHealthPolicy_,
                lastHealthCheckResult_,
                isFMRequestModified_,
                isHealthPolicyModified_,
                unhealthyEvaluations_,
                currentUpgradeDomainProgress_,
                manifestUpgradeHealthPolicy_,
                isPreparingUpgrade_,
                commonUpgradeContextData_,
                isAutoBaseline_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            FabricUpgradeDescription upgradeDescription_;
            FabricUpgradeState::Enum upgradeState_;
            Common::FabricVersion currentVersion_;
            std::wstring inProgressUpgradeDomain_;
            std::vector<std::wstring> completedUpgradeDomains_;
            std::vector<std::wstring> pendingUpgradeDomains_;

            uint64 upgradeInstance_;    // Used by FM to detect stale upgrade requests
            bool isInterrupted_;        // flags that this upgrade has been interrupted by the client and should be rolled back
            bool isPreparingUpgrade_;   // flags that this upgrade is still loading the cluster snapshot in preparation for upgrade (added in 3.1)

            // persist elapsed times across failover
            Common::TimeSpan healthCheckElapsedTime_;
            Common::TimeSpan overallUpgradeElapsedTime_;
            Common::TimeSpan upgradeDomainElapsedTime_;

            // persist cluster health evaluation policy from cluster manifest at the time the upgrade starts
            // since the upgrade itself may change the cluster manifest and hence the policy
            //
            ServiceModel::ClusterHealthPolicy manifestHealthPolicy_;
            ServiceModel::ClusterUpgradeHealthPolicy manifestUpgradeHealthPolicy_;

            bool lastHealthCheckResult_;
            bool isFMRequestModified_;
            bool isHealthPolicyModified_;
            bool isAutoBaseline_;

            std::vector<ServiceModel::HealthEvaluation> unhealthyEvaluations_;
            Reliability::UpgradeDomainProgress currentUpgradeDomainProgress_;

            // Many of the preceding members can technically be pulled
            // into this class, but must remain outside to preserve
            // backwards compatibility.
            //
            ClusterManager::CommonUpgradeContextData commonUpgradeContextData_;
        };
    }
}
