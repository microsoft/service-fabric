//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ApplicationScopedVolume
        : public ContainerVolumeRef
        {
        public:
            ApplicationScopedVolume() = default;

            bool operator==(ApplicationScopedVolume const & other) const;
            bool operator!=(ApplicationScopedVolume const & other) const;

            __declspec(property(get=get_CreationParameters)) ApplicationScopedVolumeCreationParametersSPtr const & CreationParameters;
            ApplicationScopedVolumeCreationParametersSPtr const & get_CreationParameters() const { return creationParameters_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN();
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::creationParameters, creationParameters_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_CHAIN();
                DYNAMIC_SIZE_ESTIMATION_MEMBER(creationParameters_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const &) const override;

            FABRIC_FIELDS_01(creationParameters_);

        private:
            ApplicationScopedVolumeCreationParametersSPtr creationParameters_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::ApplicationScopedVolume);
