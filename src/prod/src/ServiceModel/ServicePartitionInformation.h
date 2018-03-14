// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServicePartitionInformation
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
        DEFAULT_COPY_CONSTRUCTOR(ServicePartitionInformation)
    
    public:
        ServicePartitionInformation();

        explicit ServicePartitionInformation(Common::Guid const & partitionId);

        ServicePartitionInformation(Common::Guid const & partitionId, std::wstring const & partitionName);

        ServicePartitionInformation(Common::Guid const & partitionId, int64 lowKey, int64 highKey);

        ServicePartitionInformation(__in FABRIC_SERVICE_PARTITION_INFORMATION const& servicePartitionInformation);

        ServicePartitionInformation(ServicePartitionInformation && other);
        ServicePartitionInformation & operator=(ServicePartitionInformation && other);

        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        Common::Guid const & get_PartitionId() const { return partitionId_; }

        __declspec(property(get = get_PartitionKind)) FABRIC_SERVICE_PARTITION_KIND const & PartitionKind;
        FABRIC_SERVICE_PARTITION_KIND const & get_PartitionKind() const { return partitionKind_; }

        __declspec(property(get = get_PartitionName)) std::wstring const & PartitionName;
        std::wstring const & get_PartitionName() const { return partitionName_; }

        __declspec(property(get = get_PartitionLowKey)) int64 const & PartitionLowKey;
        int64 const & get_PartitionLowKey() const { return lowKey_; }

        __declspec(property(get = get_PartitionHighKey)) int64 const & PartitionHighKey;
        int64 const & get_PartitionHighKey() const { return highKey_; }

        bool AreEqualIgnorePartitionId(ServicePartitionInformation const & other) const;

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_PARTITION_INFORMATION & servicePartitionInformation) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_SERVICE_PARTITION_INFORMATION const& servicePartitionInformation);
        void FromPartitionInfo(Common::Guid const &partitionId, Naming::PartitionInfo const &partitionInfo);

        FABRIC_FIELDS_05(partitionKind_, partitionId_, partitionName_, lowKey_, highKey_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServicePartitionKind, partitionKind_)
            SERIALIZABLE_PROPERTY(Constants::Id, partitionId_)
            SERIALIZABLE_PROPERTY_IF(Constants::Name, partitionName_, (partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_NAMED))
            SERIALIZABLE_PROPERTY_IF(Constants::LowKey, lowKey_, (partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE))
            SERIALIZABLE_PROPERTY_IF(Constants::HighKey, highKey_, (partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE))
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_ENUM_ESTIMATION_MEMBER(partitionKind_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        FABRIC_SERVICE_PARTITION_KIND partitionKind_;
        Common::Guid partitionId_;
        std::wstring partitionName_;
        int64 lowKey_;
        int64 highKey_;
    };
}
