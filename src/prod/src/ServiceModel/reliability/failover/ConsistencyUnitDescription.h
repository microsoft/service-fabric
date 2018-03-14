// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ConsistencyUnitDescription : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(ConsistencyUnitDescription)
        DEFAULT_COPY_ASSIGNMENT(ConsistencyUnitDescription)
        
    public:
        ConsistencyUnitDescription()
            : consistencyUnitId_(), lowKeyInclusive_(0), highKeyInclusive_(0), partitionName_(L""), partitionKind_(FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
        {
        }

        explicit ConsistencyUnitDescription(Common::Guid const & guid)
            : consistencyUnitId_(guid), lowKeyInclusive_(0), highKeyInclusive_(0), partitionName_(L""), partitionKind_(FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
        {
        }

        explicit ConsistencyUnitDescription(Reliability::ConsistencyUnitId const & consistencyUnitId)
            : consistencyUnitId_(consistencyUnitId), lowKeyInclusive_(0), highKeyInclusive_(0), partitionName_(L""), partitionKind_(FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
        {
        }

        ConsistencyUnitDescription(
            Reliability::ConsistencyUnitId const & consistencyUnitId, 
            __int64 lowKeyInclusive, 
            __int64 highKeyExclusive)
            : consistencyUnitId_(consistencyUnitId),
              lowKeyInclusive_(lowKeyInclusive),
              highKeyInclusive_(highKeyExclusive),
              partitionName_(L""),
              partitionKind_(FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
        {
        }

        ConsistencyUnitDescription(
            Reliability::ConsistencyUnitId consistencyUnitId, 
            std::wstring const & partitionName)
            : consistencyUnitId_(consistencyUnitId),
              lowKeyInclusive_(0),
              highKeyInclusive_(0),
              partitionName_(partitionName),
              partitionKind_(FABRIC_SERVICE_PARTITION_KIND_NAMED)
        {
        }

        ConsistencyUnitDescription(
            ConsistencyUnitDescription && other)
            : consistencyUnitId_(std::move(other.consistencyUnitId_)),
              lowKeyInclusive_(std::move(other.lowKeyInclusive_)),
              highKeyInclusive_(std::move(other.highKeyInclusive_)),
              partitionName_(std::move(other.partitionName_)),
              partitionKind_(std::move(other.partitionKind_))
        {
        }

        ConsistencyUnitDescription & operator=(ConsistencyUnitDescription && other);

        __declspec(property(get=get_ConsistencyUnitId)) Reliability::ConsistencyUnitId ConsistencyUnitId;
        inline Reliability::ConsistencyUnitId get_ConsistencyUnitId() const { return consistencyUnitId_; }

        __declspec(property(get=get_LowKeyInclusive)) __int64 LowKey;
        inline __int64 get_LowKeyInclusive() const { return lowKeyInclusive_; }

        __declspec(property(get=get_HighKeyInclusive)) __int64 HighKey;
        inline __int64 get_HighKeyInclusive() const { return highKeyInclusive_; }
        
        __declspec(property(get=get_PartitionName)) std::wstring const & PartitionName;
        inline std::wstring const & get_PartitionName() const { return partitionName_; }

        __declspec(property(get=get_PartitionKind)) FABRIC_SERVICE_PARTITION_KIND PartitionKind;
        inline FABRIC_SERVICE_PARTITION_KIND get_PartitionKind() const { return partitionKind_; }

        __declspec(property(get = get_KeyRange)) Reliability::KeyRange KeyRange;
        Reliability::KeyRange get_KeyRange() const { return Reliability::KeyRange(lowKeyInclusive_, highKeyInclusive_); }


        static bool CompareForSort(ConsistencyUnitDescription const & left, ConsistencyUnitDescription const & right);

        void WriteTo(__in Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_05(consistencyUnitId_, lowKeyInclusive_, highKeyInclusive_, partitionName_, partitionKind_);

    private:
        Reliability::ConsistencyUnitId consistencyUnitId_;
        __int64 lowKeyInclusive_;
        __int64 highKeyInclusive_;
        std::wstring partitionName_;
        FABRIC_SERVICE_PARTITION_KIND partitionKind_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ConsistencyUnitDescription);
