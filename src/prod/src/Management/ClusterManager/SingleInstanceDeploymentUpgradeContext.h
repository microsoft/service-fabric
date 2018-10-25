//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class SingleInstanceDeploymentUpgradeContext : public RolloutContext
        {
        public:
            SingleInstanceDeploymentUpgradeContext();
            SingleInstanceDeploymentUpgradeContext(SingleInstanceDeploymentUpgradeContext const &) = default;

            SingleInstanceDeploymentUpgradeContext(std::wstring const &deploymentName);

            SingleInstanceDeploymentUpgradeContext(
                Common::ComponentRoot const & replica,
                ClientRequestSPtr const & request,
                std::wstring const &deploymentName,
                DeploymentType::Enum deploymentType,
                Common::NamingUri const &applicationName,
                std::shared_ptr<ApplicationUpgradeDescription> const &upgradeDescription,
                ServiceModelTypeName const &typeName,
                ServiceModelVersion const &currentTypeVersion,
                ServiceModelVersion const &targetTypeVersion);

            DEFAULT_MOVE_CONSTRUCTOR(SingleInstanceDeploymentUpgradeContext);
            DEFAULT_MOVE_ASSIGNMENT(SingleInstanceDeploymentUpgradeContext);

            __declspec(property(get=get_DeploymentName)) std::wstring const &DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            __declspec(property(get=get_DeploymentType)) DeploymentType::Enum DeploymentType;
            DeploymentType::Enum get_DeploymentType() const { return deploymentType_; }

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const &ApplicationName;
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get=get_CurrentTypeVersion)) ServiceModelVersion const & CurrentTypeVersion;
            ServiceModelVersion const & get_CurrentTypeVersion() const { return currentTypeVersion_; }

            __declspec(property(get=get_TargetTypeVersion)) ServiceModelVersion const & TargetTypeVersion;
            ServiceModelVersion const & get_TargetTypeVersion() const { return targetTypeVersion_; }

            __declspec(property(get = get_VersionToUnprovision, put = put_VersionToUnprovision)) ServiceModelVersion const & VersionToUnprovision;
            ServiceModelVersion const & get_VersionToUnprovision() const { return versionToUnprovision_; }
            void put_VersionToUnprovision(ServiceModelVersion const & value) { versionToUnprovision_ = value; }

            __declspec(property(get = get_TypeName)) ServiceModelTypeName const & TypeName;
            ServiceModelTypeName const & get_TypeName() const { return appTypeName_; }

            __declspec(property(get = get_CurrentStatus)) ServiceModel::SingleInstanceDeploymentUpgradeState::Enum CurrentStatus;
            ServiceModel::SingleInstanceDeploymentUpgradeState::Enum get_CurrentStatus() { return currentStatus_; }

            __declspec(property(get = get_StatusDetails, put = put_StatusDetails)) std::wstring const & StatusDetails;
            std::wstring const & get_StatusDetails() const { return statusDetails_; }
            void put_StatusDetails(std::wstring const & value) { statusDetails_ = value; }

            __declspec(property(get = get_Interrupted)) bool Interrupted;
            bool get_Interrupted() const { return isInterrupted_; }

            __declspec(property(get = get_UpgradeDescription)) ApplicationUpgradeDescription const &UpgradeDescription;
            ApplicationUpgradeDescription const & get_UpgradeDescription() const { return *upgradeDescription_; }
            ApplicationUpgradeDescription && TakeUpgradeDescription() { return std::move(*upgradeDescription_); }

            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            void OnFailRolloutContext() override { } // no-op

            bool ShouldProvision() { return currentStatus_ == ServiceModel::SingleInstanceDeploymentUpgradeState::Enum::ProvisioningTarget; }
            bool ShouldUpgrade() { return (currentStatus_ == ServiceModel::SingleInstanceDeploymentUpgradeState::Enum::RollingForward) || 
                                          (currentStatus_ == ServiceModel::SingleInstanceDeploymentUpgradeState::Enum::RollingBack); }
            bool ShouldUnprovision() { return currentStatus_ == ServiceModel::SingleInstanceDeploymentUpgradeState::Enum::UnprovisioningCurrent ||
                currentStatus_ == ServiceModel::SingleInstanceDeploymentUpgradeState::Enum::UnprovisioningTarget; }

            Common::ErrorCode UpdateUpgradeStatus(Store::StoreTransaction const &storeTx, ServiceModel::SingleInstanceDeploymentUpgradeState::Enum const status);

            Common::ErrorCode TryInterrupt(Store::StoreTransaction const &storeTx);

            Common::ErrorCode FinishUpgrading(Store::StoreTransaction const &storeTx);
            Common::ErrorCode StartRollback(Store::StoreTransaction const &storeTx, std::wstring const &);
            Common::ErrorCode StartUpgrade(Store::StoreTransaction const &storeTx, std::wstring const &);
            Common::ErrorCode StartUnprovision(Store::StoreTransaction const &storeTx, std::wstring const &);
            Common::ErrorCode StartUnprovisionTarget(Store::StoreTransaction const &storeTx, std::wstring const &);
            void ResetUpgradeDescription() { upgradeDescription_ = nullptr; }

        FABRIC_FIELDS_11(
            deploymentName_,
            applicationName_,
            upgradeDescription_,
            appTypeName_,
            currentTypeVersion_,
            targetTypeVersion_,
            isInterrupted_,
            versionToUnprovision_,
            currentStatus_,
            statusDetails_,
            deploymentType_)

        protected:
            virtual std::wstring ConstructKey() const override;

        private:
            void InnerUpdateUpgradeStatus(ServiceModel::SingleInstanceDeploymentUpgradeState::Enum const status);

            std::wstring deploymentName_;
            DeploymentType::Enum deploymentType_;
            Common::NamingUri applicationName_;
            ServiceModelTypeName appTypeName_;
            ServiceModelVersion currentTypeVersion_;
            ServiceModelVersion targetTypeVersion_;
            bool isInterrupted_;

            ServiceModelVersion versionToUnprovision_;
            ServiceModel::SingleInstanceDeploymentUpgradeState::Enum currentStatus_;
            std::wstring statusDetails_;

            //
            // The upgrade description is kept in this context till the new application type is generated
            // and provisioned. After that, this is kept in the ApplicationUpgradeContext.
            //
            std::shared_ptr<ApplicationUpgradeDescription> upgradeDescription_;
        };
    }
}
