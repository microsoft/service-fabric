// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    template <class TMetadata>
    class NameProperty 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        NameProperty()
            : metadata_()
            , bytes_()
        {
        }

        NameProperty(NameProperty<TMetadata> const & other)
            : metadata_(other.metadata_)
            , bytes_(other.bytes_)
        {
        }

        NameProperty & operator=(NameProperty<TMetadata> const & other)
        {
            if (this != &other)
            {
                metadata_ = other.metadata_;
                bytes_ = other.bytes_;
            }

            return *this;
        }

        NameProperty(NameProperty<TMetadata> && other)
            : metadata_(std::move(other.metadata_))
            , bytes_(std::move(other.bytes_))
        {
        }

        NameProperty & operator=(NameProperty<TMetadata> && other)
        {
            if (this != &other)
            {
                metadata_ = std::move(other.metadata_);
                bytes_ = std::move(other.bytes_);
            }

            return *this;
        }

        explicit NameProperty(TMetadata && metadata)
            : metadata_(std::move(metadata))
            , bytes_()
        {
        }

        NameProperty(
            TMetadata && metadata,
            std::vector<byte> && bytes)
            : metadata_(std::move(metadata))
            , bytes_(std::move(bytes))
        {
        }

        __declspec(property(get=get_Metadata)) TMetadata const & Metadata;
        inline TMetadata const & get_Metadata() const { return metadata_; }

        __declspec(property(get=get_Bytes)) std::vector<byte> const & Bytes;
        inline std::vector<byte> const & get_Bytes() const { return bytes_; }

        TMetadata && TakeMetadata() { return std::move(metadata_); }
        std::vector<byte> && TakeBytes() { return std::move(bytes_); }

        void ClearBytes() { bytes_.clear(); }

        static size_t EstimateSerializedSize(size_t uriLength, size_t propertyNameLength, size_t byteCount);

        FABRIC_FIELDS_02(metadata_, bytes_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(bytes_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(metadata_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        TMetadata metadata_;
        std::vector<byte> bytes_;
    };

    typedef NameProperty<NamePropertyMetadata> NamePropertyStoreData;
    typedef NameProperty<NamePropertyMetadataResult> NamePropertyResult;

}
