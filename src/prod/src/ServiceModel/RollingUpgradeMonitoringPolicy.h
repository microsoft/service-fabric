// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class RollingUpgradeMonitoringPolicy 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        static Common::Global<RollingUpgradeMonitoringPolicy> Default;

        RollingUpgradeMonitoringPolicy();

        RollingUpgradeMonitoringPolicy(
            MonitoredUpgradeFailureAction::Enum,
            Common::TimeSpan const healthCheckWaitDuration,
            Common::TimeSpan const healthCheckStableDuration,
            Common::TimeSpan const healthCheckRetryTimeout,
            Common::TimeSpan const upgradeTimeout,
            Common::TimeSpan const upgradeDomainTimeout);

        virtual ~RollingUpgradeMonitoringPolicy();

        __declspec(property(get=get_FailureAction)) MonitoredUpgradeFailureAction::Enum const FailureAction;
        __declspec(property(get=get_HealthCheckWaitDuration)) Common::TimeSpan const HealthCheckWaitDuration;
        __declspec(property(get=get_HealthCheckStableDuration)) Common::TimeSpan const HealthCheckStableDuration;
        __declspec(property(get=get_HealthCheckRetryTimeout)) Common::TimeSpan const HealthCheckRetryTimeout;
        __declspec(property(get=get_UpgradeTimeout)) Common::TimeSpan const UpgradeTimeout;
        __declspec(property(get=get_UpgradeDomainTimeout)) Common::TimeSpan const UpgradeDomainTimeout;

        MonitoredUpgradeFailureAction::Enum const get_FailureAction() const { return failureAction_; }
        Common::TimeSpan const get_HealthCheckWaitDuration() const { return healthCheckWaitDuration_; }
        Common::TimeSpan const get_HealthCheckStableDuration() const { return healthCheckStableDuration_; }
        Common::TimeSpan const get_HealthCheckRetryTimeout() const { return healthCheckRetryTimeout_; }
        Common::TimeSpan const get_UpgradeTimeout() const { return upgradeTimeout_; }
        Common::TimeSpan const get_UpgradeDomainTimeout() const { return upgradeDomainTimeout_; }

        Common::ErrorCode FromPublicApi(FABRIC_ROLLING_UPGRADE_MONITORING_POLICY const &);
        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_ROLLING_UPGRADE_MONITORING_POLICY &) const;

        bool TryValidate(__out std::wstring & validationErrorMessage) const;

        void Modify(RollingUpgradeUpdateDescription const &);

        bool operator == (RollingUpgradeMonitoringPolicy const & other) const;
        bool operator != (RollingUpgradeMonitoringPolicy const & other) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::FailureAction, failureAction_)
            SERIALIZABLE_PROPERTY(Constants::HealthCheckWaitDurationInMilliseconds, healthCheckWaitDuration_)
            SERIALIZABLE_PROPERTY(Constants::HealthCheckStableDurationInMilliseconds, healthCheckStableDuration_)
            SERIALIZABLE_PROPERTY(Constants::HealthCheckRetryTimeoutInMilliseconds, healthCheckRetryTimeout_)
            SERIALIZABLE_PROPERTY(Constants::UpgradeTimeoutInMilliseconds, upgradeTimeout_)
            SERIALIZABLE_PROPERTY(Constants::UpgradeDomainTimeoutInMilliseconds, upgradeDomainTimeout_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_06(
            failureAction_,
            healthCheckWaitDuration_,
            healthCheckRetryTimeout_,
            upgradeTimeout_,
            upgradeDomainTimeout_,
            // added in v2.2
            healthCheckStableDuration_);

    private:
        static DWORD ToPublicTimeInSeconds(Common::TimeSpan const & time);
        static Common::TimeSpan FromPublicTimeInSeconds(DWORD time);

        MonitoredUpgradeFailureAction::Enum failureAction_;
        Common::TimeSpan healthCheckWaitDuration_;
        Common::TimeSpan healthCheckStableDuration_;
        Common::TimeSpan healthCheckRetryTimeout_;
        Common::TimeSpan upgradeTimeout_;
        Common::TimeSpan upgradeDomainTimeout_;
    };
}
