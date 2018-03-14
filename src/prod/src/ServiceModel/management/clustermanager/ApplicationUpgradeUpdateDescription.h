// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ApplicationUpgradeUpdateDescription 
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(ApplicationUpgradeUpdateDescription)
        public:

            ApplicationUpgradeUpdateDescription();
            ApplicationUpgradeUpdateDescription(ApplicationUpgradeUpdateDescription &&);

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get=get_UpgradeType)) ServiceModel::UpgradeType::Enum const & UpgradeType;
            __declspec(property(get=get_UpdateDescription)) std::shared_ptr<ServiceModel::RollingUpgradeUpdateDescription> const & UpdateDescription;
            __declspec(property(get=get_HealthPolicy)) std::shared_ptr<ServiceModel::ApplicationHealthPolicy> const & HealthPolicy;

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            ServiceModel::UpgradeType::Enum const & get_UpgradeType() const { return upgradeType_; }
            std::shared_ptr<ServiceModel::RollingUpgradeUpdateDescription> const & get_UpdateDescription() const { return updateDescription_; }
            std::shared_ptr<ServiceModel::ApplicationHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

            bool operator == (ApplicationUpgradeUpdateDescription const & other) const;
            bool operator != (ApplicationUpgradeUpdateDescription const & other) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION const &);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Name, applicationName_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::UpgradeKind, upgradeType_)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::UpdateDescription, updateDescription_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::ApplicationHealthPolicy, healthPolicy_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_04(
                applicationName_, 
                upgradeType_, 
                updateDescription_,
                healthPolicy_);

        private:

            friend class ModifyUpgradeHelper;

            Common::NamingUri applicationName_;
            ServiceModel::UpgradeType::Enum upgradeType_;
            std::shared_ptr<ServiceModel::RollingUpgradeUpdateDescription> updateDescription_;
            std::shared_ptr<ServiceModel::ApplicationHealthPolicy> healthPolicy_;
        };
    }
}
