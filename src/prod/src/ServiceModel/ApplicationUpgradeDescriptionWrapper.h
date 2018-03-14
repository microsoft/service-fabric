// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationUpgradeDescriptionWrapper
        : public Common::IFabricJsonSerializable
    {
    public:
        ApplicationUpgradeDescriptionWrapper();
        virtual ~ApplicationUpgradeDescriptionWrapper() {};

        Common::ErrorCode FromPublicApi(__in FABRIC_APPLICATION_UPGRADE_DESCRIPTION const &appDesc);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_APPLICATION_UPGRADE_DESCRIPTION &) const;

        __declspec(property(get=get_AppName, put=set_AppName)) std::wstring &ApplicationName;
        __declspec(property(get=get_TargetAppTypeVersion, put=set_TargetAppTypeVersion)) std::wstring &TargetApplicationTypeVersion;
        __declspec(property(get=get_UpgradeKind, put=set_UpgradeKind)) ServiceModel::UpgradeType::Enum &UpgradeKind;
        __declspec(property(get=get_ParamList, put=set_ParamList)) std::map<std::wstring, std::wstring> &Parameters;
        __declspec(property(get=get_UpgradeMode, put=set_UpgradeMode)) ServiceModel::RollingUpgradeMode::Enum &UpgradeMode;
        __declspec(property(get=get_ForceRestart, put=set_ForceRestart)) bool &ForceRestart;
        __declspec(property(get=get_ReplicaChkTimeout, put=set_ReplicaChkTimeout)) ULONG &ReplicaSetTimeoutInSec;
        __declspec(property(get=get_MonitoringPolicy, put=set_MonitoringPolicy)) RollingUpgradeMonitoringPolicy const & MonitoringPolicy;
        __declspec(property(get=get_HealthPolicy, put=set_HealthPolicy)) std::shared_ptr<ApplicationHealthPolicy> const & HealthPolicy;

        std::wstring const& get_AppName() const { return applicationName_; }
        std::wstring const& get_TargetAppTypeVersion() const { return targetAppTypeVersion_; }
        ServiceModel::UpgradeType::Enum const& get_UpgradeKind() const { return upgradeType_; }
        std::map<std::wstring, std::wstring> const& get_ParamList() const { return parameterList_; }
        ServiceModel::RollingUpgradeMode::Enum const& get_UpgradeMode() const { return rollingUpgradeMode_; }
        ULONG const& get_ReplicaChkTimeout() const { return replicaSetCheckTimeoutInSeconds_; }
        bool const& get_ForceRestart() const { return forceRestart_; }
        RollingUpgradeMonitoringPolicy const& get_MonitoringPolicy() const { return monitoringPolicy_; }
        std::shared_ptr<ApplicationHealthPolicy> const& get_HealthPolicy() const { return healthPolicy_; }

        void set_AppName(std::wstring const& value) { applicationName_ = value; }
        void set_TargetAppTypeVersion(std::wstring const& value) { targetAppTypeVersion_ = value; }
        void set_UpgradeKind(ServiceModel::UpgradeType::Enum const& value) { upgradeType_ = value; }
        void set_ParamList(std::map<std::wstring, std::wstring> &&value) { parameterList_ = move(value); }
        void set_UpgradeMode(ServiceModel::RollingUpgradeMode::Enum const& value) { rollingUpgradeMode_ = value; }
        void set_ReplicaChkTimeout(ULONG value) { replicaSetCheckTimeoutInSeconds_ = value; }
        void set_ForceRestart(bool value) { forceRestart_ = value; }
        void set_MonitoringPolicy(RollingUpgradeMonitoringPolicy const & value) { monitoringPolicy_ = value; }
        void set_HealthPolicy(std::shared_ptr<ApplicationHealthPolicy> const & value) { healthPolicy_ = value; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::TargetApplicationTypeVersion, targetAppTypeVersion_)
            SERIALIZABLE_PROPERTY(Constants::Parameters, parameterList_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::UpgradeKind, upgradeType_)
            SERIALIZABLE_PROPERTY_ENUM_IF(Constants::RollingUpgradeMode, rollingUpgradeMode_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::UpgradeReplicaSetCheckTimeoutInSeconds, replicaSetCheckTimeoutInSeconds_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::ForceRestart, forceRestart_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::MonitoringPolicy, monitoringPolicy_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
            SERIALIZABLE_PROPERTY_IF(Constants::ApplicationHealthPolicy, healthPolicy_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationName_;
        std::wstring targetAppTypeVersion_;
        std::map<std::wstring, std::wstring> parameterList_;
        ServiceModel::UpgradeType::Enum upgradeType_;
        ULONG replicaSetCheckTimeoutInSeconds_;
        ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode_;
        bool forceRestart_;
        RollingUpgradeMonitoringPolicy monitoringPolicy_;
        std::shared_ptr<ApplicationHealthPolicy> healthPolicy_;
    };
}
