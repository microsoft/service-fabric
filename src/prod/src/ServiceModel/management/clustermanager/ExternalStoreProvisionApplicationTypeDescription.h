// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {    
        class ExternalStoreProvisionApplicationTypeDescription 
            : public ProvisionApplicationTypeDescriptionBase
        {
            DENY_COPY(ExternalStoreProvisionApplicationTypeDescription)
        
        public:
            ExternalStoreProvisionApplicationTypeDescription();
            
            ExternalStoreProvisionApplicationTypeDescription(ExternalStoreProvisionApplicationTypeDescription && other) = default;
            ExternalStoreProvisionApplicationTypeDescription & operator = (ExternalStoreProvisionApplicationTypeDescription && other) = default;

            virtual ~ExternalStoreProvisionApplicationTypeDescription();

            __declspec(property(get = get_AppTypeName)) std::wstring const & ApplicationTypeName;
            std::wstring const & get_AppTypeName() const { return appTypeName_; }
            
            __declspec(property(get = get_AppTypeVersion)) std::wstring const & ApplicationTypeVersion;
            std::wstring const & get_AppTypeVersion() const { return appTypeVersion_; }

            __declspec(property(get = get_ApplicationPackageDownloadUri)) std::wstring const & ApplicationPackageDownloadUri;
            std::wstring const & get_ApplicationPackageDownloadUri() const { return applicationPackageDownloadUri_; }

            std::wstring TakeAppTypeName() { return std::move(appTypeName_); }
            std::wstring TakeAppTypeVersion() { return std::move(appTypeVersion_); }
            std::wstring TakeApplicationPackageDownloadUri() { return std::move(applicationPackageDownloadUri_); }

            Common::ErrorCode FromPublicApi(FABRIC_EXTERNAL_STORE_PROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription);

            static Common::ErrorCode IsValidSfpkgDownloadPath(std::wstring const & downloadPath);
            Common::ErrorCode CheckIsValid() const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeName, appTypeName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeVersion, appTypeVersion_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationPackageDownloadUri, applicationPackageDownloadUri_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring appTypeName_;
            std::wstring appTypeVersion_;
            std::wstring applicationPackageDownloadUri_;
        };

        DEFINE_PROVISION_APPLICATION_TYPE_ACTIVATOR( ExternalStoreProvisionApplicationTypeDescription, FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE )
    }
}
