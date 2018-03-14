// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class RollingUpgradeUpdateDescription 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:

        RollingUpgradeUpdateDescription();
        RollingUpgradeUpdateDescription(RollingUpgradeUpdateDescription &&);

        __declspec(property(get=get_RollingUpgradeMode)) std::shared_ptr<ServiceModel::RollingUpgradeMode::Enum> const & RollingUpgradeMode;
        __declspec(property(get=get_ForceRestart)) std::shared_ptr<bool> const & ForceRestart;
        __declspec(property(get=get_ReplicaSetCheckTimeout)) std::shared_ptr<Common::TimeSpan> const & ReplicaSetCheckTimeout;
        __declspec(property(get=get_FailureAction)) std::shared_ptr<ServiceModel::MonitoredUpgradeFailureAction::Enum> const & FailureAction;
        __declspec(property(get=get_HealthCheckWaitDuration)) std::shared_ptr<Common::TimeSpan> const & HealthCheckWaitDuration;
        __declspec(property(get=get_HealthCheckStableDuration)) std::shared_ptr<Common::TimeSpan> const & HealthCheckStableDuration;
        __declspec(property(get=get_HealthCheckRetryTimeout)) std::shared_ptr<Common::TimeSpan> const & HealthCheckRetryTimeout;
        __declspec(property(get=get_UpgradeTimeout)) std::shared_ptr<Common::TimeSpan> const & UpgradeTimeout;
        __declspec(property(get=get_UpgradeDomainTimeout)) std::shared_ptr<Common::TimeSpan> const & UpgradeDomainTimeout;

        std::shared_ptr<ServiceModel::RollingUpgradeMode::Enum> const & get_RollingUpgradeMode() const { return rollingUpgradeMode_; }
        std::shared_ptr<bool> const & get_ForceRestart() const { return forceRestart_; }
        std::shared_ptr<Common::TimeSpan> const & get_ReplicaSetCheckTimeout() const { return replicaSetCheckTimeout_; }
        std::shared_ptr<ServiceModel::MonitoredUpgradeFailureAction::Enum> const & get_FailureAction() const { return failureAction_; }
        std::shared_ptr<Common::TimeSpan> const & get_HealthCheckWaitDuration() const { return healthCheckWait_; }
        std::shared_ptr<Common::TimeSpan> const & get_HealthCheckStableDuration() const { return healthCheckStable_; }
        std::shared_ptr<Common::TimeSpan> const & get_HealthCheckRetryTimeout() const { return healthCheckRetry_; }
        std::shared_ptr<Common::TimeSpan> const & get_UpgradeTimeout() const { return upgradeTimeout_; }
        std::shared_ptr<Common::TimeSpan> const & get_UpgradeDomainTimeout() const { return upgradeDomainTimeout_; }

        bool operator == (RollingUpgradeUpdateDescription const & other) const;
        bool operator != (RollingUpgradeUpdateDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        static bool HasRollingUpgradeUpdateDescriptionFlags(DWORD flags);
        bool IsValidForRollback();

        Common::ErrorCode FromPublicApi(
            FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS const &, 
            FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION const &);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUMP(ServiceModel::Constants::RollingUpgradeMode, rollingUpgradeMode_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ForceRestart, forceRestart_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ReplicaSetCheckTimeoutInMilliseconds, replicaSetCheckTimeout_)
            SERIALIZABLE_PROPERTY_ENUMP(ServiceModel::Constants::FailureAction, failureAction_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::HealthCheckWaitDurationInMilliseconds, healthCheckWait_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::HealthCheckStableDurationInMilliseconds, healthCheckStable_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::HealthCheckRetryTimeoutInMilliseconds, healthCheckRetry_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeTimeoutInMilliseconds, upgradeTimeout_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDomainTimeoutInMilliseconds, upgradeDomainTimeout_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_09(
            rollingUpgradeMode_,
            forceRestart_,
            replicaSetCheckTimeout_,
            failureAction_,
            healthCheckWait_,
            healthCheckStable_,
            healthCheckRetry_,
            upgradeTimeout_,
            upgradeDomainTimeout_);
        
    private:
        //
        // Nullable to support JSON - these correspond to the flags in
        // FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS
        //
        std::shared_ptr<ServiceModel::RollingUpgradeMode::Enum> rollingUpgradeMode_;
        std::shared_ptr<bool> forceRestart_;
        std::shared_ptr<Common::TimeSpan> replicaSetCheckTimeout_;
        std::shared_ptr<ServiceModel::MonitoredUpgradeFailureAction::Enum> failureAction_;
        std::shared_ptr<Common::TimeSpan> healthCheckWait_;
        std::shared_ptr<Common::TimeSpan> healthCheckStable_;
        std::shared_ptr<Common::TimeSpan> healthCheckRetry_;
        std::shared_ptr<Common::TimeSpan> upgradeTimeout_;
        std::shared_ptr<Common::TimeSpan> upgradeDomainTimeout_;
    };

    using RollingUpgradeUpdateDescriptionSPtr = std::shared_ptr<RollingUpgradeUpdateDescription>;
}
