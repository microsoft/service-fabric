// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ComposeDeploymentContext : public DeletableRolloutContext
        {
            DENY_COPY(ComposeDeploymentContext)

        public:
            ComposeDeploymentContext();

            explicit ComposeDeploymentContext(
                Common::NamingUri const &);

            explicit ComposeDeploymentContext(
                std::wstring const &);

            ComposeDeploymentContext(
                ComposeDeploymentContext &&);

            ComposeDeploymentContext & operator=(
                ComposeDeploymentContext &&);

            ComposeDeploymentContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                std::wstring const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                uint64 globalInstanceCount,
                ServiceModelTypeName const &,
                ServiceModelVersion const &);

            __declspec(property(get=get_DeploymentName)) std::wstring const & DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get=get_ApplicationId)) ServiceModelApplicationId const & ApplicationId;
            ServiceModelApplicationId const & get_ApplicationId() const { return applicationId_; }

            __declspec(property(get=get_GlobalInstanceCount)) uint64 const & GlobalInstanceCount;
            uint64 const & get_GlobalInstanceCount() const { return globalInstanceCount_; }

            __declspec(property(get=get_TypeName)) ServiceModelTypeName const & TypeName;
            ServiceModelTypeName const & get_TypeName() const { return appTypeName_; }

            __declspec(property(get=get_TypeVersion, put=put_TypeVersion)) ServiceModelVersion const & TypeVersion;
            ServiceModelVersion const & get_TypeVersion() const { return appTypeVersion_; }
            void put_TypeVersion(ServiceModelVersion const& value) { appTypeVersion_ = value; }

            void OnFailRolloutContext() override { /* no-op */ }

            void AddPendingDefaultService(ServiceModelServiceNameEx &&);
            void ClearPendingDefaultServices();

            __declspec(property(get=get_ComposeDeploymentStatus, put=set_ComposeDeploymentStatus)) ServiceModel::ComposeDeploymentStatus::Enum ComposeDeploymentStatus;
            ServiceModel::ComposeDeploymentStatus::Enum const get_ComposeDeploymentStatus() const { return composeDeploymentStatus_; }
            void set_ComposeDeploymentStatus(ServiceModel::ComposeDeploymentStatus::Enum const value) { composeDeploymentStatus_ = value; }
            Common::ErrorCode UpdateComposeDeploymentStatus(Store::StoreTransaction const & storeTx, ServiceModel::ComposeDeploymentStatus::Enum const);

            __declspec(property(get=get_StatusDetails, put=put_StatusDetails)) std::wstring const & StatusDetails;
            std::wstring const & get_StatusDetails() const { return statusDetails_; }
            void put_StatusDetails(std::wstring const & value) { statusDetails_ = value; }

            __declspec(property(get=get_IsProvisionCompleted)) bool IsProvisionCompleted;
            bool get_IsProvisionCompleted() const { return composeDeploymentStatus_ == ServiceModel::ComposeDeploymentStatus::Creating || composeDeploymentStatus_ == ServiceModel::ComposeDeploymentStatus::Ready; }

            __declspec(property(get = get_IsApplicationDeleteComplete)) bool IsApplicationDeleteComplete;
            bool get_IsApplicationDeleteComplete() const { return composeDeploymentStatus_ == ServiceModel::ComposeDeploymentStatus::Unprovisioning; }

            __declspec(property(get = get_IsUpgrading)) bool IsUpgrading;
            bool get_IsUpgrading() const { return composeDeploymentStatus_ == ServiceModel::ComposeDeploymentStatus::Upgrading; }
            ServiceModel::ComposeDeploymentStatusQueryResult ToQueryResult() const;

            __declspec(property(get = get_PendingDefaultServices)) std::vector<ServiceModelServiceNameEx> const & PendingDefaultServices;
            std::vector<ServiceModelServiceNameEx> const & get_PendingDefaultServices() const { return pendingDefaultServices_; }

            virtual std::wstring const & get_Type() const override;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const override;
            
            //
            // Helper methods to update the ComposeContext state as well as the 
            // rollout manager specific state during create/delete.
            //
            Common::ErrorCode StartCreating(Store::StoreTransaction &);
            Common::ErrorCode StartDeleting(Store::StoreTransaction &);
            Common::ErrorCode StartUpgrading(Store::StoreTransaction &);
            Common::ErrorCode FinishCreating(Store::StoreTransaction &, std::wstring const &);
            Common::ErrorCode ClearUpgrading(Store::StoreTransaction &, std::wstring const &);

            FABRIC_FIELDS_10(
                deploymentName_,
                applicationName_, 
                applicationId_, 
                globalInstanceCount_, 
                appTypeName_,
                appTypeVersion_,
                composeDeploymentStatus_,
                pendingDefaultServices_,
                isForceDelete_,
                statusDetails_);
        
        protected:
            virtual std::wstring ConstructKey() const override;

        private:
            void InnerUpdateComposeDeploymentStatus(ServiceModel::ComposeDeploymentStatus::Enum const);

            std::wstring deploymentName_;
            Common::NamingUri applicationName_;
            ServiceModelApplicationId applicationId_;

            // Increments globally for each application instance created of any application type.
            // Used to generate application Id.
            //
            uint64 globalInstanceCount_;

            ServiceModelTypeName appTypeName_;
            ServiceModelVersion appTypeVersion_;

            Common::ExclusiveLock pendingDefaultServicesLock_;
            std::vector<ServiceModelServiceNameEx> pendingDefaultServices_;

            ServiceModel::ComposeDeploymentStatus::Enum composeDeploymentStatus_;

            std::wstring statusDetails_;
        };
    }
}
