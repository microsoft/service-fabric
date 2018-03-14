// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    //
    // This is the JSON serializable/deserializable wrapper for FabricUpgradeDescription
    //
    class FabricUpgradeDescriptionWrapper
        : public Common::IFabricJsonSerializable
    {
    public:
        FabricUpgradeDescriptionWrapper();
        ~FabricUpgradeDescriptionWrapper() {};

        Common::ErrorCode FromPublicApi(FABRIC_UPGRADE_DESCRIPTION const &);
        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_UPGRADE_DESCRIPTION &) const;

        __declspec(property(get=get_CodeVersion, put=set_CodeVersion)) std::wstring &CodeVersion;
        __declspec(property(get=get_ConfigVersion, put=set_ConfigVersion)) std::wstring &ConfigVersion;
        __declspec(property(get=get_UpgradeKind, put=set_UpgradeKind)) FABRIC_UPGRADE_KIND &UpgradeKind;
        __declspec(property(get=get_UpgradeMode, put=set_UpgradeMode)) FABRIC_ROLLING_UPGRADE_MODE &UpgradeMode;
        __declspec(property(get=get_ReplicaChkTimeout, put=set_ReplicaChkTimeout)) ULONG &ReplicaSetTimeoutInSec;
        __declspec(property(get=get_ForceRestart, put=set_ForceRestart)) bool &ForceRestart;
        __declspec(property(get=get_MonitoringPolicy, put=set_MonitoringPolicy)) RollingUpgradeMonitoringPolicy const & MonitoringPolicy;
        __declspec(property(get=get_HealthPolicy, put=set_HealthPolicy)) std::shared_ptr<ClusterHealthPolicy> const & HealthPolicy;
        __declspec(property(get=get_EnableDeltaHealthEvaluation, put=set_EnableDeltaHealthEvaluation)) bool &EnableDeltaHealthEvaluation;
        __declspec(property(get=get_UpgradeHealthPolicy, put=set_UpgradeHealthPolicy)) std::shared_ptr<ClusterUpgradeHealthPolicy> const & UpgradeHealthPolicy;
        __declspec(property(get=get_ApplicationHealthPolicies, put=set_ApplicationHealthPolicies)) ApplicationHealthPolicyMapSPtr const & ApplicationHealthPolicies;

        std::wstring const& get_CodeVersion() const { return codeVersion_; }
        std::wstring const& get_ConfigVersion() const { return configVersion_; }
        FABRIC_UPGRADE_KIND const& get_UpgradeKind() const { return upgradeType_; }
        FABRIC_ROLLING_UPGRADE_MODE const& get_UpgradeMode() const { return rollingUpgradeMode_; }
        ULONG const& get_ReplicaChkTimeout() const { return replicaSetCheckTimeoutInSeconds_; }
        bool get_ForceRestart() const { return forceRestart_; }
        RollingUpgradeMonitoringPolicy const& get_MonitoringPolicy() const { return monitoringPolicy_; }
        std::shared_ptr<ClusterHealthPolicy> const& get_HealthPolicy() const { return healthPolicy_; }
        bool get_EnableDeltaHealthEvaluation() const { return enableDeltaHealthEvaluation_; }
        std::shared_ptr<ClusterUpgradeHealthPolicy> const& get_UpgradeHealthPolicy() const { return upgradeHealthPolicy_; }
        ApplicationHealthPolicyMapSPtr const& get_ApplicationHealthPolicies() const { return applicationHealthPolicies_; }

        void set_CodeVersion(std::wstring const& value) { codeVersion_ = value; }
        void set_ConfigVersion(std::wstring const& value) { configVersion_ = value; }
        void set_UpgradeKind(FABRIC_UPGRADE_KIND const& value) { upgradeType_ = value; }
        void set_UpgradeMode(FABRIC_ROLLING_UPGRADE_MODE const& value) { rollingUpgradeMode_ = value; }
        void set_ReplicaChkTimeout(ULONG value) { replicaSetCheckTimeoutInSeconds_ = value; }
        void set_ForceRestart(bool value) { forceRestart_ = value; }
        void set_MonitoringPolicy(RollingUpgradeMonitoringPolicy const & value) { monitoringPolicy_ = value; }
        void set_HealthPolicy(std::shared_ptr<ClusterHealthPolicy> const & value) { healthPolicy_ = value; }
        void set_EnableDeltaHealthEvaluation(bool value) { enableDeltaHealthEvaluation_ = value; }
        void set_UpgradeHealthPolicy(std::shared_ptr<ClusterUpgradeHealthPolicy> const & value) { upgradeHealthPolicy_ = value; }
        void set_ApplicationHealthPolicies(ApplicationHealthPolicyMapSPtr const & value) { applicationHealthPolicies_ = value; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::CodeVersion, codeVersion_)
            SERIALIZABLE_PROPERTY(Constants::ConfigVersion, configVersion_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::UpgradeKind, upgradeType_)
            SERIALIZABLE_PROPERTY_ENUM_IF(Constants::RollingUpgradeMode, rollingUpgradeMode_, upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::UpgradeReplicaSetCheckTimeoutInSeconds, replicaSetCheckTimeoutInSeconds_, upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::ForceRestart, forceRestart_, upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::MonitoringPolicy, monitoringPolicy_, upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::ClusterHealthPolicy, healthPolicy_, upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::EnableDeltaHealthEvaluation, enableDeltaHealthEvaluation_, upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::ClusterUpgradeHealthPolicy, upgradeHealthPolicy_, upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::ApplicationHealthPolicyMap, applicationHealthPolicies_, upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring codeVersion_;
        std::wstring configVersion_;
        FABRIC_UPGRADE_KIND upgradeType_;
        ULONG replicaSetCheckTimeoutInSeconds_;
        FABRIC_ROLLING_UPGRADE_MODE rollingUpgradeMode_;
        bool forceRestart_;
        RollingUpgradeMonitoringPolicy monitoringPolicy_;
        std::shared_ptr<ClusterHealthPolicy> healthPolicy_;
        bool enableDeltaHealthEvaluation_;
        std::shared_ptr<ClusterUpgradeHealthPolicy> upgradeHealthPolicy_;
        ApplicationHealthPolicyMapSPtr applicationHealthPolicies_;
    };
}
