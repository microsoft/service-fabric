// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PartitionInfo 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        PartitionInfo();

        PartitionInfo(__int64 lowKeyInclusive, __int64 highKeyInclusive);

        explicit PartitionInfo(std::wstring const & partitionName);

        explicit PartitionInfo(FABRIC_PARTITION_SCHEME scheme);

        bool RangeContains(PartitionKey const & key) const;

        bool RangeContains(PartitionInfo const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext &) const;

        bool operator == (const PartitionInfo range2) const
        {
            if (scheme_ != range2.scheme_)
            {
                return false;
            }

            switch (scheme_)
            {
            case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
                return lowKeyInclusive_ == range2.lowKeyInclusive_ && highKeyInclusive_ == range2.highKeyInclusive_;
            case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
                return partitionName_ == range2.partitionName_;
            default:
                return true;
            }
        }

        bool operator != (const PartitionInfo range2) const
        {
            return !(*this == range2);
        }

        bool operator < (PartitionInfo const & other) const
        {
            if (scheme_ != other.scheme_)
            {
                return false;
            }

            switch (scheme_)
            {
            case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
                if (lowKeyInclusive_ <= other.highKeyInclusive_ &&
                    highKeyInclusive_ >= other.lowKeyInclusive_)
                {
                    return false;
                }
                else
                {
                    return highKeyInclusive_ < other.lowKeyInclusive_;
                }
            case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
                return partitionName_ < other.partitionName_;
            default:
                return true;
            }
        }        

        __declspec(property(get=get_PartitionScheme)) FABRIC_PARTITION_SCHEME PartitionScheme;
        inline FABRIC_PARTITION_SCHEME get_PartitionScheme() const { return scheme_; }

        __declspec(property(get=get_LowKeyInclusive)) __int64 LowKey;
        inline __int64 get_LowKeyInclusive() const 
        { 
            ASSERT_IFNOT(scheme_ == FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE, "Cannot access int range for key scheme_ {0}", static_cast<int>(scheme_));
            return lowKeyInclusive_;
        }

        __declspec(property(get=get_HighKeyInclusive)) __int64 HighKey;
        inline __int64 get_HighKeyInclusive() const 
        { 
            ASSERT_IFNOT(scheme_ == FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE, "Cannot access int range for key scheme_ {0}", static_cast<int>(scheme_));
            return highKeyInclusive_;
        }

        __declspec(property(get=get_PartitionName)) std::wstring const & PartitionName;
        inline std::wstring const & get_PartitionName() const 
        { 
            ASSERT_IFNOT(scheme_ == FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED, "Cannot access partition name for key scheme_ {0}", static_cast<int>(scheme_));
            return partitionName_; 
        }

        FABRIC_FIELDS_04(scheme_, lowKeyInclusive_, highKeyInclusive_, partitionName_);

        // scheme is initialized with singleton, so dynamic serialization is 0
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        FABRIC_PARTITION_SCHEME scheme_;
        __int64 lowKeyInclusive_;
        __int64 highKeyInclusive_;
        std::wstring partitionName_;
    };
}
