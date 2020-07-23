// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ApplicationInfo : public StoreData
        {
            DENY_COPY_ASSIGNMENT(ApplicationInfo);

        public:
            ApplicationInfo();

            ApplicationInfo(ApplicationInfo const& other);

            ApplicationInfo(
				ServiceModel::ApplicationIdentifier const& applicationId,
				Common::NamingUri const& applicationName,
				uint64 instanceId);

            ApplicationInfo(ServiceModel::ApplicationIdentifier const & applicationId,
                Common::NamingUri const & applicationName,
                uint64 instanceId,
                uint64 updateId,
                ApplicationCapacityDescription const & capacityDescription,
                ServiceModel::ServicePackageResourceGovernanceMap const& rgDescription,
                ServiceModel::CodePackageContainersImagesMap const& cpContainersImages);

            ApplicationInfo(
                ApplicationInfo const & other,
                ApplicationUpgradeUPtr && upgrade,
                ApplicationUpgradeUPtr && rollback);

            ApplicationInfo(
                ApplicationInfo const& other,
                uint64 instanceId);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            __declspec (property(get = get_IdString)) std::wstring const& IdString;
            std::wstring const& get_IdString() const { return idString_; }

            __declspec (property(get=get_ApplicationId)) ServiceModel::ApplicationIdentifier const& ApplicationId;
            ServiceModel::ApplicationIdentifier const& get_ApplicationId() const { return applicationId_; }

            __declspec (property(get = get_ApplicationName, put = put_ApplicationName)) Common::NamingUri & ApplicationName;
            Common::NamingUri const& get_ApplicationName() const { return applicationName_; }
            void put_ApplicationName(Common::NamingUri const& applicationName) { applicationName_ = applicationName; }

            __declspec (property(get=get_Instance)) uint64 InstanceId;
            uint64 get_Instance() const { return instanceId_; }

            __declspec (property(get = get_UpdateId, put = put_UpdateId)) uint64 UpdateId;
            uint64 get_UpdateId() const { return updateId_; }
            void put_UpdateId(uint64 updateId) { updateId_ = updateId; }

            __declspec (property(get=get_Upgrade)) ApplicationUpgradeUPtr const & Upgrade;
            ApplicationUpgradeUPtr const & get_Upgrade() const { return upgrade_; }

            __declspec(property(get = get_Rollback)) ApplicationUpgradeUPtr Rollback;
            ApplicationUpgradeUPtr const& get_Rollback() const { return rollback_; }

            __declspec(property(get = get_IsDeleted, put = set_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return isDeleted_; }
            void set_IsDeleted(bool value) { isDeleted_ = value; }

            __declspec (property(get = get_LastUpdateTime)) Common::DateTime LastUpdateTime;
            Common::DateTime get_LastUpdateTime() const { return lastUpdateTime_; }

            __declspec(property(get = get_ApplicationCapacity, put = put_ApplicationCapacity)) ApplicationCapacityDescription & ApplicationCapacity;
            ApplicationCapacityDescription const & get_ApplicationCapacity() const { return capacityDescription_; }
            void put_ApplicationCapacity(ApplicationCapacityDescription const& appCapacity) { capacityDescription_ = appCapacity; }

            __declspec(property (get = get_ResourceGovernanceDescription, put = put_ResourceGovernanceDescription))
                ServiceModel::ServicePackageResourceGovernanceMap ResourceGovernanceDescription;
            ServiceModel::ServicePackageResourceGovernanceMap const& get_ResourceGovernanceDescription() const { return  resourceGovernanceDescription_; }
            void put_ResourceGovernanceDescription(ServiceModel::ServicePackageResourceGovernanceMap const& desc) { resourceGovernanceDescription_ = desc; }

            __declspec(property (get = get_CodePackageContainersImages, put = put_CodePackageContainersImages))
                ServiceModel::CodePackageContainersImagesMap CodePackageContainersImages;
            ServiceModel::CodePackageContainersImagesMap const& get_CodePackageContainersImages() const { return  codePackageContainersImages_; }
            void put_CodePackageContainersImages(ServiceModel::CodePackageContainersImagesMap const& desc) { codePackageContainersImages_ = desc; }

            bool GetUpgradeVersionForServiceType(ServiceModel::ServiceTypeIdentifier const & typeId, ServiceModel::ServicePackageVersionInstance & result) const;
            bool GetUpgradeVersionForServiceType(ServiceModel::ServiceTypeIdentifier const & typeId, ServiceModel::ServicePackageVersionInstance & result, std::wstring const & upgradeDomain) const;

            bool GetRollbackVersionForServiceType(ServiceModel::ServiceTypeIdentifier const & typeId, ServiceModel::ServicePackageVersionInstance & result) const;

            void AddThreadContext(std::vector<BackgroundThreadContextUPtr> & contexts, ApplicationInfoSPtr const & thisSPtr) const;

            __declspec(property(get = get_Networks, put = put_Networks)) StringCollection & Networks;
            StringCollection const & get_Networks() const { return networks_; }
            void put_Networks(StringCollection const& networks) { networks_ = networks; }

            void PostRead(int64 version);
            void PostUpdate(Common::DateTime updateTime);

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

            LoadBalancingComponent::ApplicationDescription GetPLBApplicationDescription() const;

            bool IsPLBSafetyCheckNeeded(ServiceModel::ServicePackageResourceGovernanceMap const & upgradingRG) const;

            std::map<ServiceModel::ServicePackageIdentifier, LoadBalancingComponent::ServicePackageDescription> GetPLBServicePackageDescription(
                    ServiceModel::ServicePackageResourceGovernanceMap const & rgSettings,
                    ServiceModel::CodePackageContainersImagesMap const & containerImages) const;

            FABRIC_FIELDS_12(
                applicationId_,
                instanceId_,
                upgrade_,
                lastUpdateTime_,
                isDeleted_,
                applicationName_,
                updateId_,
                capacityDescription_,
                resourceGovernanceDescription_,
                rollback_,
                codePackageContainersImages_,
                networks_);

        private:

            Common::NamingUri applicationName_;
            ServiceModel::ApplicationIdentifier applicationId_;
            uint64 instanceId_;
            ApplicationUpgradeUPtr upgrade_;
            ApplicationUpgradeUPtr rollback_;

            Common::DateTime lastUpdateTime_;
            bool isDeleted_;

            ApplicationCapacityDescription capacityDescription_;
            uint64 updateId_;

            ServiceModel::ServicePackageResourceGovernanceMap resourceGovernanceDescription_;
            ServiceModel::CodePackageContainersImagesMap codePackageContainersImages_;

            std::wstring idString_;
            StringCollection networks_;
        };
    }
}