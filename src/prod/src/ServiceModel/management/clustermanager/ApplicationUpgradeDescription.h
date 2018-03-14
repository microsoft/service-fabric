// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ApplicationUpgradeDescription : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(ApplicationUpgradeDescription)
            DEFAULT_COPY_ASSIGNMENT(ApplicationUpgradeDescription)

        public:

            ApplicationUpgradeDescription();

            explicit ApplicationUpgradeDescription(Common::NamingUri const &);
            
            ApplicationUpgradeDescription(
                Common::NamingUri const &,
                std::wstring const & targetApplicationTypeVersion,
                std::map<std::wstring, std::wstring> const & applicationParameters,
                ServiceModel::UpgradeType::Enum,
                ServiceModel::RollingUpgradeMode::Enum,
                Common::TimeSpan replicaSetCheckTimeout,
                ServiceModel::RollingUpgradeMonitoringPolicy const &,
                ServiceModel::ApplicationHealthPolicy const &,
                bool isHealthPolicyValid);

            ApplicationUpgradeDescription(ApplicationUpgradeDescription &&);

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get=get_TargetApplicationTypeVersion)) std::wstring const & TargetApplicationTypeVersion;
            __declspec(property(get=get_ApplicationParameters)) std::map<std::wstring, std::wstring> const & ApplicationParameters;
            __declspec(property(get=get_UpgradeType)) ServiceModel::UpgradeType::Enum const & UpgradeType;
            __declspec(property(get=get_RollingUpgradeMode)) ServiceModel::RollingUpgradeMode::Enum const & RollingUpgradeMode;
            __declspec(property(get=get_ReplicaSetCheckTimeout)) Common::TimeSpan const & ReplicaSetCheckTimeout;
            __declspec(property(get=get_MonitoringPolicy)) ServiceModel::RollingUpgradeMonitoringPolicy const & MonitoringPolicy;
            __declspec(property(get=get_HealthPolicy)) ServiceModel::ApplicationHealthPolicy const & HealthPolicy;
            __declspec(property(get=get_IsHealthPolicyValid)) bool IsHealthPolicyValid;

            __declspec(property(get=get_IsInternalMonitored )) bool IsInternalMonitored;
            __declspec(property(get=get_IsUnmonitoredManual)) bool IsUnmonitoredManual;
            __declspec(property(get=get_IsHealthMonitored)) bool IsHealthMonitored;

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            std::wstring const & get_TargetApplicationTypeVersion() const { return targetApplicationTypeVersion_; }
            std::map<std::wstring, std::wstring> const & get_ApplicationParameters() const { return applicationParameters_; }
            ServiceModel::UpgradeType::Enum const & get_UpgradeType() const { return upgradeType_; }
            ServiceModel::RollingUpgradeMode::Enum const & get_RollingUpgradeMode() const { return rollingUpgradeMode_; }
            Common::TimeSpan const & get_ReplicaSetCheckTimeout() const { return replicaSetCheckTimeout_; }
            ServiceModel::RollingUpgradeMonitoringPolicy const & get_MonitoringPolicy() const { return monitoringPolicy_; }
            ServiceModel::ApplicationHealthPolicy const & get_HealthPolicy() const { return healthPolicy_; }

            // Set*() are used to build a fake rollback description for reading the upgrade progress
            //
            void SetTargetVersion(std::wstring const & value) { targetApplicationTypeVersion_ = value; }
            void SetApplicationParameters(std::map<std::wstring, std::wstring> const & value) { applicationParameters_ = value; }
            void SetUpgradeMode(ServiceModel::RollingUpgradeMode::Enum value) { rollingUpgradeMode_ = value; }
            void SetReplicaSetCheckTimeout(Common::TimeSpan const value) { replicaSetCheckTimeout_ = value; }

            bool get_IsHealthPolicyValid() const { return isHealthPolicyValid_; }
            bool get_IsInternalMonitored() const;
            bool get_IsUnmonitoredManual() const;
            bool get_IsHealthMonitored() const;

            Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_UPGRADE_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_APPLICATION_UPGRADE_DESCRIPTION &) const;

            Common::ErrorCode FromWrapper(ServiceModel::ApplicationUpgradeDescriptionWrapper const &);
            void ToWrapper(__out ServiceModel::ApplicationUpgradeDescriptionWrapper &) const;

            bool TryModify(ApplicationUpgradeUpdateDescription const &, __out std::wstring & validationErrorMessage);
            bool TryValidate(__out std::wstring & validationErrorMessage) const;

            bool EqualsIgnoreHealthPolicies(ApplicationUpgradeDescription const & other) const;
            bool operator == (ApplicationUpgradeDescription const & other) const;
            bool operator != (ApplicationUpgradeDescription const & other) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            FABRIC_FIELDS_09(
                applicationName_, 
                targetApplicationTypeVersion_, 
                applicationParameters_, 
                upgradeType_, 
                rollingUpgradeMode_,
                replicaSetCheckTimeout_,
                monitoringPolicy_,
                healthPolicy_,
                isHealthPolicyValid_);

        private:

            friend class ModifyUpgradeHelper;

            Common::NamingUri applicationName_;
            std::wstring targetApplicationTypeVersion_;
            std::map<std::wstring, std::wstring> applicationParameters_;
            ServiceModel::UpgradeType::Enum upgradeType_;

            // Rolling upgrade policy
            ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode_;
            Common::TimeSpan replicaSetCheckTimeout_;
            ServiceModel::RollingUpgradeMonitoringPolicy monitoringPolicy_;
            ServiceModel::ApplicationHealthPolicy healthPolicy_;
            bool isHealthPolicyValid_;
        };
    }
}
