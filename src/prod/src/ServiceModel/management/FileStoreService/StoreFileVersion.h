// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class StoreFileVersion 
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
            DEFAULT_COPY_CONSTRUCTOR(StoreFileVersion)
        
        public:
            StoreFileVersion();

            StoreFileVersion(
                int64 const epochDataLossNumber, 
                int64 const epochConfigurationNumber, 
                uint64 const versionNumber);

            StoreFileVersion const & operator = (StoreFileVersion const & other);
            StoreFileVersion const & operator = (StoreFileVersion && other);

            bool operator == (StoreFileVersion const & other) const;
            bool operator != (StoreFileVersion const & other) const;

            __declspec(property(get=get_VersionNumber)) uint64 VersionNumber;
            uint64 get_VersionNumber() const { return versionNumber_; }

            __declspec(property(get=get_EpochDataLossNumber)) int64 const EpochDataLossNumber;
            int64 const get_EpochDataLossNumber() const { return epochDataLossNumber_; }

            __declspec(property(get = get_EpochConfigurationNumber)) int64 const EpochConfigurationNumber;
            int64 const get_EpochConfigurationNumber() const { return epochConfigurationNumber_; }
                        
            std::wstring ToString() const;
            virtual void WriteTo(
                Common::TextWriter & w, 
                Common::FormatOptions const &) const;

            static bool IsStoreFileVersion(std::wstring const & stringValue);

            static StoreFileVersion Default;

            FABRIC_FIELDS_03(
                epochDataLossNumber_, 
                versionNumber_, 
                epochConfigurationNumber_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::EpochDataLossNumber, epochDataLossNumber_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::VersionNumber, versionNumber_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::EpochConfigurationNumber, epochConfigurationNumber_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(epochDataLossNumber_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(epochConfigurationNumber_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(versionNumber_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            int64 epochDataLossNumber_;
            int64 epochConfigurationNumber_;
            uint64 versionNumber_;
        };
    }
}
