// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // This is type used to send the data from the internal interface as JSON stream.
    //
    class ApplicationUpgradeProgress : public Common::IFabricJsonSerializable
    {
    public:
        ApplicationUpgradeProgress();
        Common::ErrorCode FromInternalInterface(Api::IApplicationUpgradeProgressResultPtr &appUpgDesc);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Name, applicationName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::TypeName, applicationTypeName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::TargetApplicationTypeVersion, targetApplicationTypeVersion_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDomains, upgradeDomainStatus_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::UpgradeState, upgradeState_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NextUpgradeDomain, nextUpgradeDomain_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::RollingUpgradeMode, rollingUpgradeMode_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDescription, upgradeDescription_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDurationInMilliseconds, upgradeDuration_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDomainDurationInMilliseconds, upgradeDomainDuration_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UnhealthyEvaluations, unhealthyEvaluations_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CurrentUpgradeDomainProgress, currentUpgradeDomainProgress_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StartTimestampUtc, startTime_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::FailureTimestampUtc, failureTime_);
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::FailureReason, failureReason_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDomainProgressAtFailure, upgradeDomainProgressAtFailure_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeStatusDetails, upgradeStatusDetails_);
            END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        class UpgradeDomainStatus;

        std::wstring applicationName_;
        std::wstring applicationTypeName_;
        std::wstring targetApplicationTypeVersion_;
        std::vector<UpgradeDomainStatus> upgradeDomainStatus_;
        FABRIC_APPLICATION_UPGRADE_STATE upgradeState_;
        std::wstring nextUpgradeDomain_;
        FABRIC_ROLLING_UPGRADE_MODE rollingUpgradeMode_;
        std::shared_ptr<ServiceModel::ApplicationUpgradeDescriptionWrapper> upgradeDescription_;
        Common::TimeSpan upgradeDuration_;
        Common::TimeSpan upgradeDomainDuration_;
        std::vector<ServiceModel::HealthEvaluation> unhealthyEvaluations_;
        Reliability::UpgradeDomainProgress currentUpgradeDomainProgress_;
        Common::DateTime startTime_;
        Common::DateTime failureTime_;
        FABRIC_UPGRADE_FAILURE_REASON failureReason_;
        Reliability::UpgradeDomainProgress upgradeDomainProgressAtFailure_;
        std::wstring upgradeStatusDetails_;

        class UpgradeDomainStatus : public Common::IFabricJsonSerializable
        {
        public:

            UpgradeDomainStatus() {};

            UpgradeDomainStatus(std::wstring const& domainName, FABRIC_UPGRADE_DOMAIN_STATE const& state)
                : name_(domainName)
                , state_(state)
            {}

            __declspec(property(get=get_Name)) std::wstring &Name;
            __declspec(property(get=get_State)) FABRIC_UPGRADE_DOMAIN_STATE &State;

            std::wstring const& get_Name() const { return name_; }
            FABRIC_UPGRADE_DOMAIN_STATE const& get_State() const { return state_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Name, name_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::State, state_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring name_;
            FABRIC_UPGRADE_DOMAIN_STATE state_;
        };
    };

}
