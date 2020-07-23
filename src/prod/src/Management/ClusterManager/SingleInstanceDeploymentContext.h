//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class SingleInstanceDeploymentContext : public DeletableRolloutContext
        {
            DENY_COPY(SingleInstanceDeploymentContext)

        public:
            SingleInstanceDeploymentContext();

            explicit SingleInstanceDeploymentContext(
                std::wstring const & deploymentName);

            DEFAULT_MOVE_CONSTRUCTOR(SingleInstanceDeploymentContext);

            SingleInstanceDeploymentContext(
                Common::ComponentRoot const & replica,
                ClientRequestSPtr const & clientRequest,
                std::wstring const & deploymentName,
                DeploymentType::Enum const deploymentType,
                Common::NamingUri const & applicationName,
                ServiceModel::SingleInstanceDeploymentStatus::Enum const status,
                ServiceModelApplicationId applicationId,
                uint64 globalInstanceCount,
                ServiceModelTypeName appTypeName,
                ServiceModelVersion appTypeVersion,
                wstring statusDetails);

            __declspec(property(get=get_DeploymentName)) std::wstring const & DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            __declspec(property(get=get_DeploymentType)) DeploymentType::Enum DeploymentType;
            DeploymentType::Enum get_DeploymentType() const { return deploymentType_; }

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get=get_DeploymentStatus, put=put_DeploymentStatus)) ServiceModel::SingleInstanceDeploymentStatus::Enum DeploymentStatus;
            ServiceModel::SingleInstanceDeploymentStatus::Enum get_DeploymentStatus() const { return deploymentStatus_; }
            void put_DeploymentStatus(ServiceModel::SingleInstanceDeploymentStatus::Enum const value) { deploymentStatus_ = value; }

            __declspec(property(get=get_IsUpgrading)) bool IsUpgrading;
            bool get_IsUpgrading() const { return deploymentStatus_ == ServiceModel::SingleInstanceDeploymentStatus::Upgrading; }

            __declspec(property(get=get_ApplicationId, put=put_ApplicationId)) ServiceModelApplicationId const & ApplicationId;
            ServiceModelApplicationId const & get_ApplicationId() const { return applicationId_; }
            void put_ApplicationId(ServiceModelApplicationId const & value) { applicationId_ = value; }

            __declspec(property(get=get_GlobalInstanceCount, put=put_GlobalInstanceCount)) uint64 const & GlobalInstanceCount;
            uint64 const & get_GlobalInstanceCount() const { return globalInstanceCount_; }
            void put_GlobalInstanceCount(uint64 const value) { globalInstanceCount_ = value; }

            __declspec(property(get=get_TypeName)) ServiceModelTypeName const & TypeName;
            ServiceModelTypeName const & get_TypeName() const { return appTypeName_; }

            __declspec(property(get=get_TypeVersion, put=put_TypeVersion)) ServiceModelVersion const & TypeVersion;
            ServiceModelVersion const & get_TypeVersion() const { return appTypeVersion_; }
            void put_TypeVersion(ServiceModelVersion const& value) { appTypeVersion_ = value; }
            
            __declspec(property(get=get_NewTypeVersion, put=put_NewTypeVersion)) ServiceModelVersion const & NewTypeVersion;
            ServiceModelVersion const & get_NewTypeVersion() const { return appTypeNewVersion_; }
            void put_NewTypeVersion(ServiceModelVersion const & value) { appTypeNewVersion_ = value; }

           __declspec(property(get=get_StatusDetails, put=put_StatusDetails)) std::wstring const & StatusDetails;
            std::wstring const & get_StatusDetails() const { return statusDetails_; }
            void put_StatusDetails(std::wstring const & value) { statusDetails_ = value; }

            __declspec(property(get = get_PendingDefaultServices)) std::vector<ServiceModelServiceNameEx> const & PendingDefaultServices;
            std::vector<ServiceModelServiceNameEx> const & get_PendingDefaultServices() const { return pendingDefaultServices_; }

            Common::ErrorCode UpdateDeploymentStatus(Store::StoreTransaction const & storeTx, ServiceModel::SingleInstanceDeploymentStatus::Enum const );

            void OnFailRolloutContext() override { /* no-op */ }

            void AddPendingDefaultService(ServiceModelServiceNameEx &&);
            void ClearPendingDefaultServices();
 
            ServiceModel::SingleInstanceDeploymentQueryResult ToQueryResult() const;

            virtual std::wstring const & get_Type() const override;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const override;

            Common::ErrorCode StartDeleting(Store::StoreTransaction const &);
            Common::ErrorCode StartReplacing(Store::StoreTransaction const &);
            Common::ErrorCode FinishCreating(Store::StoreTransaction const &, std::wstring const &);
            Common::ErrorCode SwitchReplaceToCreate(Store::StoreTransaction const & storeTx) override;
            Common::ErrorCode StartUpgrading(Store::StoreTransaction const &);
            Common::ErrorCode ClearUpgrading(Store::StoreTransaction const &, std::wstring const &);

            FABRIC_FIELDS_11(
                deploymentName_,
                deploymentType_,
                applicationName_,
                deploymentStatus_,
                applicationId_,
                globalInstanceCount_,
                appTypeName_,
                appTypeVersion_,
                pendingDefaultServices_,
                statusDetails_,
                appTypeNewVersion_);

        protected:
            virtual std::wstring ConstructKey() const override;

        private:
            void InnerUpdateStatus(ServiceModel::SingleInstanceDeploymentStatus::Enum const);

            std::wstring deploymentName_;
            DeploymentType::Enum deploymentType_;
            Common::NamingUri applicationName_;
            ServiceModel::SingleInstanceDeploymentStatus::Enum deploymentStatus_;
            ServiceModelApplicationId applicationId_;

            // Increments globally for each application instance created of any application type.
            // Used to generate application Id.
            //
            uint64 globalInstanceCount_;

            ServiceModelTypeName appTypeName_;
            ServiceModelVersion appTypeVersion_;
            // The new version for PUT replacement.
            ServiceModelVersion appTypeNewVersion_;

            Common::ExclusiveLock pendingDefaultServicesLock_;
            std::vector<ServiceModelServiceNameEx> pendingDefaultServices_;

            std::wstring statusDetails_;
        };
    }
}
