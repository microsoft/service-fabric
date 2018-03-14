// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class FabricUpgradeDescription : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(FabricUpgradeDescription)
            DEFAULT_COPY_ASSIGNMENT(FabricUpgradeDescription)
        
        public:
            FabricUpgradeDescription();
            
            FabricUpgradeDescription(
                Common::FabricVersion const &,
                ServiceModel::UpgradeType::Enum,
                ServiceModel::RollingUpgradeMode::Enum,
                Common::TimeSpan replicaSetCheckTimeout,
                ServiceModel::RollingUpgradeMonitoringPolicy const &,
                ServiceModel::ClusterHealthPolicy const &,
                bool isHealthPolicyValid,
                bool enableDeltaHealthEvaluation,
                ServiceModel::ClusterUpgradeHealthPolicy const &,
                bool isUpgradeHealthPolicyValid,
                ServiceModel::ApplicationHealthPolicyMapSPtr const &);

            FabricUpgradeDescription(FabricUpgradeDescription &&);

            __declspec(property(get=get_FabricVersion)) Common::FabricVersion const & Version;
            __declspec(property(get=get_UpgradeType)) ServiceModel::UpgradeType::Enum const & UpgradeType;
            __declspec(property(get=get_RollingUpgradeMode)) ServiceModel::RollingUpgradeMode::Enum const & RollingUpgradeMode;
            __declspec(property(get=get_ReplicaSetCheckTimeout)) Common::TimeSpan const & ReplicaSetCheckTimeout;
            __declspec(property(get=get_MonitoringPolicy)) ServiceModel::RollingUpgradeMonitoringPolicy const & MonitoringPolicy;
            __declspec(property(get=get_HealthPolicy)) ServiceModel::ClusterHealthPolicy const & HealthPolicy;
            __declspec(property(get=get_IsHealthPolicyValid)) bool IsHealthPolicyValid;
            __declspec(property(get=get_EnableDeltaHealthEvaluation)) bool EnableDeltaHealthEvaluation;
            __declspec(property(get=get_UpgradeHealthPolicy)) ServiceModel::ClusterUpgradeHealthPolicy const & UpgradeHealthPolicy;
            __declspec(property(get=get_IsUpgradeHealthPolicyValid)) bool IsUpgradeHealthPolicyValid;
            __declspec(property(get=get_ApplicationHealthPolicies)) ServiceModel::ApplicationHealthPolicyMapSPtr const & ApplicationHealthPolicies;

            __declspec(property(get=get_IsInternalMonitored )) bool IsInternalMonitored;
            __declspec(property(get=get_IsUnmonitoredManual)) bool IsUnmonitoredManual;
            __declspec(property(get=get_IsHealthMonitored)) bool IsHealthMonitored;

            Common::FabricVersion const & get_FabricVersion() const { return version_; }
            ServiceModel::UpgradeType::Enum const & get_UpgradeType() const { return upgradeType_; }
            ServiceModel::RollingUpgradeMode::Enum const & get_RollingUpgradeMode() const { return rollingUpgradeMode_; }
            Common::TimeSpan const & get_ReplicaSetCheckTimeout() const { return replicaSetCheckTimeout_; }
            ServiceModel::RollingUpgradeMonitoringPolicy const & get_MonitoringPolicy() const { return monitoringPolicy_; }
            ServiceModel::ClusterHealthPolicy const & get_HealthPolicy() const { return healthPolicy_; }
            ServiceModel::ClusterUpgradeHealthPolicy const & get_UpgradeHealthPolicy() const { return upgradeHealthPolicy_; }
            ServiceModel::ApplicationHealthPolicyMapSPtr const& get_ApplicationHealthPolicies() const { return applicationHealthPolicies_; }

            // Set*() are used to build a fake rollback description for reading the upgrade progress
            //
            void SetTargetVersion(Common::FabricVersion const & value) { version_ = value; }
            void SetUpgradeMode(ServiceModel::RollingUpgradeMode::Enum value) { rollingUpgradeMode_ = value; }
            void SetReplicaSetCheckTimeout(Common::TimeSpan const & value) { replicaSetCheckTimeout_ = value; }

            bool get_EnableDeltaHealthEvaluation() const { return enableDeltaHealthEvaluation_; }
            bool get_IsHealthPolicyValid() const { return isHealthPolicyValid_; }
            bool get_IsUpgradeHealthPolicyValid() const { return isUpgradeHealthPolicyValid_; }
            bool get_IsInternalMonitored() const;
            bool get_IsUnmonitoredManual() const;
            bool get_IsHealthMonitored() const;

            Common::ErrorCode FromWrapper(ServiceModel::FabricUpgradeDescriptionWrapper const&);
            void ToWrapper(__out ServiceModel::FabricUpgradeDescriptionWrapper &) const;

            Common::ErrorCode FromPublicApi(FABRIC_UPGRADE_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_UPGRADE_DESCRIPTION &) const;

            bool TryModify(FabricUpgradeUpdateDescription const &, __out std::wstring & validationErrorMessage);
            bool TryValidate(__out std::wstring & validationErrorMessage) const;

            bool EqualsIgnoreHealthPolicies(FabricUpgradeDescription const & other) const;
            bool operator == (FabricUpgradeDescription const & other) const;
            bool operator != (FabricUpgradeDescription const & other) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            FABRIC_FIELDS_11(
                version_, 
                upgradeType_, 
                rollingUpgradeMode_,
                replicaSetCheckTimeout_,
                monitoringPolicy_,
                healthPolicy_,
                isHealthPolicyValid_,
                enableDeltaHealthEvaluation_,
                upgradeHealthPolicy_,
                isUpgradeHealthPolicyValid_,
                applicationHealthPolicies_);

        private:

            Common::ErrorCode TraceAndGetError(
                Common::ErrorCodeValue::Enum,
                std::wstring &&);

            // Application and Fabric upgrade description fields must remain
            // in derived classes for version compatibility
            //
            friend class ModifyUpgradeHelper;

            Common::FabricVersion version_;
            ServiceModel::UpgradeType::Enum upgradeType_;

            // Rolling upgrade policy
            ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode_;
            Common::TimeSpan replicaSetCheckTimeout_;
            ServiceModel::RollingUpgradeMonitoringPolicy monitoringPolicy_;
            ServiceModel::ClusterHealthPolicy healthPolicy_;
            bool isHealthPolicyValid_;
            bool enableDeltaHealthEvaluation_;
            ServiceModel::ClusterUpgradeHealthPolicy upgradeHealthPolicy_;
            bool isUpgradeHealthPolicyValid_;
            ServiceModel::ApplicationHealthPolicyMapSPtr applicationHealthPolicies_;
        };
    }
}
