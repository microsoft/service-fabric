// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class StartUpgradeDescription
            : public Serialization::FabricSerializable
        {
        public:

            StartUpgradeDescription();
            StartUpgradeDescription(
                std::wstring  &,
                Common::TimeSpan const &,
                Common::TimeSpan const &,
                Common::TimeSpan const &,
                Common::TimeSpan const &,
                Common::TimeSpan const &,
                BYTE &,
                BYTE &,
                BYTE &,
                BYTE &);

            StartUpgradeDescription(StartUpgradeDescription const &);
            StartUpgradeDescription(StartUpgradeDescription &&);

            __declspec(property(get = get_ClusterConfig)) std::wstring  const & ClusterConfig;
            __declspec(property(get = get_HealthCheckRetryTimeout)) Common::TimeSpan const HealthCheckRetryTimeout;
            __declspec(property(get = get_HealthCheckWaitDurationInSeconds)) Common::TimeSpan const HealthCheckWaitDurationInSeconds;
            __declspec(property(get = get_HealthCheckStableDurationInSeconds)) Common::TimeSpan const HealthCheckStableDurationInSeconds;
            __declspec(property(get = get_UpgradeDomainTimeoutInSeconds)) Common::TimeSpan const UpgradeDomainTimeoutInSeconds;
            __declspec(property(get = get_UpgradeTimeoutInSeconds)) Common::TimeSpan const UpgradeTimeoutInSeconds;
            __declspec(property(get = get_MaxPercentUnhealthyApplications)) BYTE MaxPercentUnhealthyApplications;
            inline BYTE get_MaxPercentUnhealthyApplications() const { return maxPercentUnhealthyApplications_; }
            __declspec(property(get = get_MaxPercentUnhealthyNodes)) BYTE MaxPercentUnhealthyNodes;
            inline BYTE get_MaxPercentUnhealthyNodes() const { return maxPercentUnhealthyNodes_; }
            __declspec(property(get = get_MaxPercentDeltaUnhealthyNodes)) BYTE MaxPercentDeltaUnhealthyNodes;
            inline BYTE get_MaxPercentDeltaUnhealthyNodes() const { return maxPercentDeltaUnhealthyNodes_; }
            __declspec(property(get = get_MaxPercentUpgradeDomainDeltaUnhealthyNodes)) BYTE MaxPercentUpgradeDomainDeltaUnhealthyNodes;
            inline BYTE get_MaxPercentUpgradeDomainDeltaUnhealthyNodes() const { return maxPercentUpgradeDomainDeltaUnhealthyNodes_; }

            std::wstring  const & get_ClusterConfig() const { return clusterConfig_; }
            Common::TimeSpan const get_HealthCheckRetryTimeout() const { return healthCheckRetryTimeout_; }
            Common::TimeSpan const get_HealthCheckWaitDuration() const { return healthCheckWaitDuration_; }
            Common::TimeSpan const get_HealthCheckStableDuration() const { return healthCheckStableDuration_; }
            Common::TimeSpan const get_UpgradeDomainTimeout() const { return upgradeDomainTimeout_; }
            Common::TimeSpan const get_UpgradeTimeout() const { return upgradeTimeout_; }

            bool operator == (StartUpgradeDescription const & other) const;
            bool operator != (StartUpgradeDescription const & other) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode FromPublicApi(FABRIC_START_UPGRADE_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_UPGRADE_DESCRIPTION &) const;

            FABRIC_FIELDS_10(
                clusterConfig_,
                healthCheckRetryTimeout_,
                healthCheckWaitDuration_,
                healthCheckStableDuration_,
                upgradeDomainTimeout_,
                upgradeTimeout_,
                maxPercentUnhealthyApplications_,
                maxPercentUnhealthyNodes_,
                maxPercentDeltaUnhealthyNodes_,
                maxPercentUpgradeDomainDeltaUnhealthyNodes_);

        private:
            std::wstring  clusterConfig_;
            Common::TimeSpan healthCheckRetryTimeout_;
            Common::TimeSpan healthCheckWaitDuration_;
            Common::TimeSpan healthCheckStableDuration_;
            Common::TimeSpan upgradeDomainTimeout_;
            Common::TimeSpan upgradeTimeout_;
            BYTE maxPercentUnhealthyApplications_;
            BYTE maxPercentUnhealthyNodes_;
            BYTE maxPercentDeltaUnhealthyNodes_;
            BYTE maxPercentUpgradeDomainDeltaUnhealthyNodes_;

            Common::TimeSpan FromPublicTimeInSeconds(DWORD time);
            DWORD ToPublicTimeInSeconds(Common::TimeSpan const & time) const;
        };
    }
}
