//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ImageRegistryCredential
        : public ModelType
        {
        public:
            ImageRegistryCredential() = default;

            bool operator==(ImageRegistryCredential const & other) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::containerRegistryServer, containerRegistryServer_, !containerRegistryServer_.empty())
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::containerRegistryUserName, containerRegistryUserName_, !containerRegistryUserName_.empty())
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::containerRegistryPassword, containerRegistryPassword_, !containerRegistryPassword_.empty())
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(containerRegistryServer_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(containerRegistryUserName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(containerRegistryPassword_)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_03(
                containerRegistryServer_,
                containerRegistryUserName_,
                containerRegistryPassword_)

            Common::ErrorCode TryValidate(std::wstring const &) const override;

        private:
            std::wstring containerRegistryServer_;
            std::wstring containerRegistryUserName_;
            std::wstring containerRegistryPassword_;
        };
    }
}
