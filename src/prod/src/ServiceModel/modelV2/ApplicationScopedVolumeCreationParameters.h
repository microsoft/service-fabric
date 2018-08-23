//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ApplicationScopedVolumeCreationParameters
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
            DENY_COPY(ApplicationScopedVolumeCreationParameters)
        public:
            ApplicationScopedVolumeCreationParameters() = default;
            virtual ~ApplicationScopedVolumeCreationParameters() = default;

            virtual bool operator==(ApplicationScopedVolumeCreationParameters const & other) const;
            virtual bool operator!=(ApplicationScopedVolumeCreationParameters const & other) const;

            virtual Common::ErrorCode Validate(std::wstring const & volumeName) const;

            __declspec(property(get=get_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return description_; }

            __declspec(property(get=get_Provider)) ApplicationScopedVolumeProvider::Enum const & Provider;
            ApplicationScopedVolumeProvider::Enum const & get_Provider() const { return provider_; }

            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            static std::shared_ptr<ServiceModel::ModelV2::ApplicationScopedVolumeCreationParameters> CreateSPtr(ApplicationScopedVolumeProvider::Enum provider);

            virtual void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::KindLowerCase, provider_)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::volumeDescription, description_, !description_.empty())
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)
                DYNAMIC_ENUM_ESTIMATION_MEMBER(provider_)
            END_DYNAMIC_SIZE_ESTIMATION()

            JSON_TYPE_ACTIVATOR_METHOD(ApplicationScopedVolumeCreationParameters, ApplicationScopedVolumeProvider::Enum, provider_, CreateSPtr);

            FABRIC_FIELDS_02(
                description_,
                provider_);

        protected:
            Common::ErrorCode Validate() const;

            std::wstring description_;
            ApplicationScopedVolumeProvider::Enum provider_;
        };

        typedef std::shared_ptr<ServiceModel::ModelV2::ApplicationScopedVolumeCreationParameters> ApplicationScopedVolumeCreationParametersSPtr;

        SERVICE_FABRIC_VOLUME_DISK_TYPE(ApplicationScopedVolumeCreationParameters, ApplicationScopedVolumeCreationParametersVolumeDiskBase)

        class ApplicationScopedVolumeCreationParametersVolumeDisk : public ApplicationScopedVolumeCreationParametersVolumeDiskBase
        {
            DENY_COPY(ApplicationScopedVolumeCreationParametersVolumeDisk)
        public:
            ApplicationScopedVolumeCreationParametersVolumeDisk() = default;
            virtual ~ApplicationScopedVolumeCreationParametersVolumeDisk() = default;

            virtual bool operator==(ApplicationScopedVolumeCreationParameters const & other) const override;
            virtual bool operator!=(ApplicationScopedVolumeCreationParameters const & other) const override;

            virtual Common::ErrorCode Validate(std::wstring const & volumeName) const override;

        private:
            const int MaxVolumeNameLength = 10;
        };
    }
}
