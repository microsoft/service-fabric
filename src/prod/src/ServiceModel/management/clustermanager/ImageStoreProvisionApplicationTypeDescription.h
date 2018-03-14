// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {    
        class ImageStoreProvisionApplicationTypeDescription
            : public ProvisionApplicationTypeDescriptionBase
        {
            DENY_COPY(ImageStoreProvisionApplicationTypeDescription)
        
        public:
            ImageStoreProvisionApplicationTypeDescription();
            
            ImageStoreProvisionApplicationTypeDescription(ImageStoreProvisionApplicationTypeDescription && other) = default;
            ImageStoreProvisionApplicationTypeDescription & operator = (ImageStoreProvisionApplicationTypeDescription && other) = default;
            
            virtual ~ImageStoreProvisionApplicationTypeDescription();

            __declspec(property(get=get_AppTypeBuildPath)) std::wstring const & ApplicationTypeBuildPath;
            std::wstring const & get_AppTypeBuildPath() const { return appTypeBuildPath_; }

            __declspec(property(get = get_ApplicationPackageCleanupPolicy)) ApplicationPackageCleanupPolicy::Enum ApplicationPackageCleanupPolicy;
            ApplicationPackageCleanupPolicy::Enum get_ApplicationPackageCleanupPolicy() const { return applicationPackageCleanupPolicy_; }

            std::wstring TakeAppTypeBuildPath() { return std::move(appTypeBuildPath_); }

            Common::ErrorCode FromPublicApi(FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeBuildPath, appTypeBuildPath_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ApplicationPackageCleanupPolicy, applicationPackageCleanupPolicy_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring appTypeBuildPath_;
            ApplicationPackageCleanupPolicy::Enum applicationPackageCleanupPolicy_;
        };

        DEFINE_PROVISION_APPLICATION_TYPE_ACTIVATOR(ImageStoreProvisionApplicationTypeDescription, FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH)
    }
}
