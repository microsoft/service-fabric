// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        // Class used on the wire, to be sent to CM.
        // For versions <= 6.0, the class is created based on the provided FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION,
        // which will have only information for the provision based on build path.
        // For versions > 6.0, the public COM API creates one of the ImageStoreProvisionApplicationTypeDescription or ExternalStoreProvisionApplicationTypeDescription
        // and creates this class using their fields.
        // This class combines all the fields to be sent to CM to be able to address both requests from versions <= 6.0, where there was just one type of provision.
        class ProvisionApplicationTypeDescription
            : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(ProvisionApplicationTypeDescription)
            DEFAULT_COPY_ASSIGNMENT(ProvisionApplicationTypeDescription)
        
        public:
            ProvisionApplicationTypeDescription();

            virtual ~ProvisionApplicationTypeDescription();

            __declspec(property(get=get_AppTypeBuildPath)) std::wstring const & ApplicationTypeBuildPath;
            std::wstring const & get_AppTypeBuildPath() const { return appTypeBuildPath_; }

            __declspec(property(get=get_IsAsync)) bool IsAsync;
            bool get_IsAsync() const { return isAsync_; }

            __declspec(property(get = get_AppTypeName)) std::wstring const & ApplicationTypeName;
            std::wstring const & get_AppTypeName() const { return appTypeName_; }

            __declspec(property(get = get_AppTypeVersion)) std::wstring const & ApplicationTypeVersion;
            std::wstring const & get_AppTypeVersion() const { return appTypeVersion_; }

            __declspec(property(get = get_ApplicationPackageDownloadUri)) std::wstring const & ApplicationPackageDownloadUri;
            std::wstring const & get_ApplicationPackageDownloadUri() const { return applicationPackageDownloadUri_; }

            __declspec(property(get = get_ApplicationPackageCleanupPolicy)) ApplicationPackageCleanupPolicy::Enum ApplicationPackageCleanupPolicy;
            ApplicationPackageCleanupPolicy::Enum get_ApplicationPackageCleanupPolicy() const { return applicationPackageCleanupPolicy_; }

            Common::ErrorCode FromPublicApi(FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_BASE const & publicDescriptionBase);
            
            Common::ErrorCode FromPublicApi(FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription);
            
            Common::ErrorCode Update(ProvisionApplicationTypeDescriptionBaseSPtr && description);

            bool NeedsDownload() const { return !applicationPackageDownloadUri_.empty(); }

            Common::ErrorCode CheckIsValid() const;

            FABRIC_FIELDS_06(appTypeBuildPath_, isAsync_, appTypeName_, appTypeVersion_, applicationPackageDownloadUri_, applicationPackageCleanupPolicy_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeBuildPath, appTypeBuildPath_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Async, isAsync_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeName, appTypeName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeVersion, appTypeVersion_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationPackageDownloadUri, applicationPackageDownloadUri_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ApplicationPackageCleanupPolicy, applicationPackageCleanupPolicy_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            Common::ErrorCode FromPublicApi(FABRIC_EXTERNAL_STORE_PROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription);

            Common::ErrorCode TraceAndGetError(
                Common::ErrorCodeValue::Enum errorValue,
                std::wstring && message) const;

        private:
            std::wstring appTypeBuildPath_;
            bool isAsync_;
            std::wstring appTypeName_;
            std::wstring appTypeVersion_;
            std::wstring applicationPackageDownloadUri_;
            ApplicationPackageCleanupPolicy::Enum applicationPackageCleanupPolicy_;
        };
    }
}
