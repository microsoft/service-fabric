//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class VolumeDescription
            : public ServiceModel::ClientServerMessageBody
            , public Common::ISizeEstimator
        {
            DENY_COPY(VolumeDescription)
        public:
            VolumeDescription();

            virtual ~VolumeDescription() = default;

            virtual Common::ErrorCode Validate() const;

            __declspec(property(get=get_VolumeName, put=set_VolumeName)) std::wstring const & VolumeName;
            std::wstring const & get_VolumeName() const { return volumeName_; }
            void set_VolumeName(std::wstring && value) { volumeName_ = move(value); }

            __declspec(property(get=get_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return description_; }

            __declspec(property(get=get_Provider)) VolumeProvider::Enum const & Provider;
            VolumeProvider::Enum const & get_Provider() const { return provider_; }

            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            static std::shared_ptr<Management::ClusterManager::VolumeDescription> CreateSPtr(VolumeProvider::Enum provider);

            virtual void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

            virtual void RemoveSensitiveInformation() {};

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::KindLowerCase, provider_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::volumeName, volumeName_)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::volumeDescription, description_, !description_.empty())
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(volumeName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)
                DYNAMIC_ENUM_ESTIMATION_MEMBER(provider_)
            END_DYNAMIC_SIZE_ESTIMATION()

            JSON_TYPE_ACTIVATOR_METHOD(VolumeDescription, VolumeProvider::Enum, provider_, CreateSPtr);

            FABRIC_FIELDS_03(
                volumeName_,
                description_,
                provider_);

        protected:
            std::wstring volumeName_;
            std::wstring description_;
            VolumeProvider::Enum provider_;
        };

        using VolumeDescriptionSPtr = std::shared_ptr<Management::ClusterManager::VolumeDescription>;

        class VolumeDescriptionAzureFile : public VolumeDescription
        {
            DENY_COPY(VolumeDescriptionAzureFile)
        public:
            VolumeDescriptionAzureFile() = default;
            virtual ~VolumeDescriptionAzureFile() = default;

            virtual Common::ErrorCode Validate() const override;

            __declspec(property(get=get_AccountName)) std::wstring const & AccountName;
            std::wstring const & get_AccountName() const { return accountName_; }

            __declspec(property(get=get_AccountKey)) std::wstring const & AccountKey;
            std::wstring const & get_AccountKey() const { return accountKey_; }

            __declspec(property(get=get_ShareName)) std::wstring const & ShareName;
            std::wstring const & get_ShareName() const { return shareName_; }

            virtual void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const override;

            void InitializeBaseFromSeaBreezePrivatePreview2Parameters(
                std::wstring && description)
            {
                provider_ = VolumeProvider::Enum::AzureFile;
                description_ = move(description);
            }
            void InitializeFromSeaBreezePrivatePreview2Parameters(
                std::wstring && accountName,
                std::wstring && accountKey,
                std::wstring && shareName)
            {
                accountName_ = move(accountName);
                accountKey_ = move(accountKey);
                shareName_ = move(shareName);
            }

            void TakeBaseForSeaBreezePrivatePreview2QueryResult(
                __out std::wstring & volumeName,
                __out std::wstring & description)
            {
                volumeName = move(volumeName_);
                description = move(description_);
            }
            void TakeForSeaBreezePrivatePreview2QueryResult(
                __out std::wstring & accountName,
                __out std::wstring & accountKey,
                __out std::wstring & shareName)
            {
                accountName = move(accountName_);
                accountKey = move(accountKey_);
                shareName = move(shareName_);
            }

            virtual void RemoveSensitiveInformation() override;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN();
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::accountName, accountName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::accountKey, accountKey_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::shareName, shareName_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_CHAIN();
                DYNAMIC_SIZE_ESTIMATION_MEMBER(accountName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(accountKey_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(shareName_)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_03(
                accountName_,
                accountKey_,
                shareName_)

        private:
            std::wstring accountName_;
            std::wstring accountKey_;
            std::wstring shareName_;
        };

        SERVICE_FABRIC_VOLUME_DISK_TYPE(VolumeDescription, VolumeDescriptionVolumeDiskBase)

        class VolumeDescriptionVolumeDisk : public VolumeDescriptionVolumeDiskBase
        {
            DENY_COPY(VolumeDescriptionVolumeDisk)
        public:
            VolumeDescriptionVolumeDisk() = default;
            virtual ~VolumeDescriptionVolumeDisk() = default;
            virtual Common::ErrorCode Validate() const override;

        private:
            const int MaxVolumeNameLength = 49;
        };
    }
}
