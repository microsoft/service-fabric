// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
    class PartitionKey : public Serialization::FabricSerializable
    {

    public:
        PartitionKey();
        
        explicit PartitionKey(INTEGER_PARTITION_KEY_RANGE const & integerRangeKey);
        explicit PartitionKey(std::wstring const & stringKey);
        explicit PartitionKey(std::wstring && stringKey);

        PartitionKey(const PartitionKey & other);
        PartitionKey(PartitionKey && other);

        __declspec(property(get=get_KeyType)) FABRIC_PARTITION_KEY_TYPE KeyType;
        inline FABRIC_PARTITION_KEY_TYPE get_KeyType() const { return keyType_; }
        
        __declspec(property(get=get_Int64RangeKey)) INTEGER_PARTITION_KEY_RANGE Int64RangeKey;
        inline INTEGER_PARTITION_KEY_RANGE get_Int64RangeKey() const 
        { 
            ASSERT_IFNOT(keyType_ == FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64, "Wrong key type for the Int64RangeKey property.");
            return integerRangeKey_;
        }
          
        __declspec(property(get=get_StringKey)) std::wstring StringKey;
        inline std::wstring get_StringKey() const 
        { 
            ASSERT_IFNOT(keyType_ == FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING, "Wrong key type for the String property.");
            return stringKey_;
        }

        __declspec(property(get=get_IsWholeService)) bool IsWholeService;
        inline bool get_IsWholeService() const { return isWholeService_; }        

        bool operator ==(PartitionKey const & other) const
        {
            if (KeyType != other.KeyType)
            {
                return false;
            }

            switch(KeyType)
            {
            case FABRIC_PARTITION_KEY_TYPE_INT64:
				return (integerRangeKey_.IntegerKeyLow == other.integerRangeKey_.IntegerKeyLow) 
					&& (integerRangeKey_.IntegerKeyHigh == other.integerRangeKey_.IntegerKeyHigh);
            case FABRIC_PARTITION_KEY_TYPE_STRING:
                return StringKey == other.StringKey;
            default:
                return true;
            }
		}

		static LONG compare(PartitionKey const & firstPartitionId, PartitionKey const & secondPartitionId)
		{
			FABRIC_PARTITION_KEY_TYPE firstKeyType = firstPartitionId.KeyType;
			FABRIC_PARTITION_KEY_TYPE secondKeyType = secondPartitionId.KeyType;

            ASSERT_IF(firstKeyType == FABRIC_PARTITION_KEY_TYPE_INVALID, "invalid key type for ReliableMessagingServicePartition");
            ASSERT_IF(secondKeyType == FABRIC_PARTITION_KEY_TYPE_INVALID, "invalid key type for ReliableMessagingServicePartition");

			switch (secondKeyType)
			{
			case FABRIC_PARTITION_KEY_TYPE_NONE: return (firstKeyType==FABRIC_PARTITION_KEY_TYPE_NONE) ? 0 : 1;
			case FABRIC_PARTITION_KEY_TYPE_INT64: 
				{
					switch (firstKeyType)
					{
					case FABRIC_PARTITION_KEY_TYPE_NONE: return -1;
						// The next case works only for non-overlapping key ranges, we should never arrive here with overlapping key ranges
						// The asserts reflect this assumption
					case FABRIC_PARTITION_KEY_TYPE_INT64: 
						if (firstPartitionId.Int64RangeKey.IntegerKeyHigh > secondPartitionId.Int64RangeKey.IntegerKeyHigh) 
						{
							ASSERT_IFNOT(firstPartitionId.Int64RangeKey.IntegerKeyLow > secondPartitionId.Int64RangeKey.IntegerKeyHigh, "Overlapping Int64 partitions found");
							return 1;
						}
						else if (firstPartitionId.Int64RangeKey.IntegerKeyHigh < secondPartitionId.Int64RangeKey.IntegerKeyHigh) 
						{
							ASSERT_IFNOT(firstPartitionId.Int64RangeKey.IntegerKeyHigh < secondPartitionId.Int64RangeKey.IntegerKeyLow, "Overlapping Int64 partitions found");
							return -1;
						}
						else 
						{
							ASSERT_IFNOT(firstPartitionId.Int64RangeKey.IntegerKeyLow == secondPartitionId.Int64RangeKey.IntegerKeyLow
								         && firstPartitionId.Int64RangeKey.IntegerKeyHigh == secondPartitionId.Int64RangeKey.IntegerKeyHigh,
										 "Overlapping Int64 partitions found");
							return 0;
						}
					case FABRIC_PARTITION_KEY_TYPE_STRING: return -1;
					}			
				}
			case FABRIC_PARTITION_KEY_TYPE_STRING:
				{
					switch (firstKeyType)
					{
					case FABRIC_PARTITION_KEY_TYPE_NONE: return -1;
					case FABRIC_PARTITION_KEY_TYPE_INT64: return 1;
					case FABRIC_PARTITION_KEY_TYPE_STRING: return firstPartitionId.StringKey.compare(secondPartitionId.StringKey);
					}						
				}
			}

            Common::Assert::CodingError("Unexpected outcome for ReliableMessagingServicePartition comparison");			
		}

		// In Find the firstKey is the search key (single Int64 used in CreateOutboundSession) and the second key is the target key (partition range for session manager)
		// This compare function can only be used like a secondary index where the standard one above is used first for insert/delete in a primary index
		static LONG compareForInt64Find(PartitionKey const & firstKey, PartitionKey const & secondKey)
		{
			FABRIC_PARTITION_KEY_TYPE searchKeyType = firstKey.KeyType;
			FABRIC_PARTITION_KEY_TYPE searchTargetKeyType = secondKey.KeyType;

            ASSERT_IF(searchKeyType != FABRIC_PARTITION_KEY_TYPE_INT64, "invalid key type for PartitionKey::CompareForInt64Find");
            ASSERT_IF(searchTargetKeyType != FABRIC_PARTITION_KEY_TYPE_INT64, "invalid key type for PartitionKey::CompareForInt64Find");

			if (firstKey.Int64RangeKey.IntegerKeyHigh > secondKey.Int64RangeKey.IntegerKeyHigh
				&& firstKey.Int64RangeKey.IntegerKeyLow > secondKey.Int64RangeKey.IntegerKeyHigh)
			{
				return 1;
			}
			else if (firstKey.Int64RangeKey.IntegerKeyHigh < secondKey.Int64RangeKey.IntegerKeyHigh
				&& firstKey.Int64RangeKey.IntegerKeyHigh < secondKey.Int64RangeKey.IntegerKeyLow) 
			{
				return -1;
			}
			else 
			{
		        bool trueEquality = false;
				bool searchCase = false;
				if (firstKey.Int64RangeKey.IntegerKeyLow == secondKey.Int64RangeKey.IntegerKeyLow
								&& firstKey.Int64RangeKey.IntegerKeyHigh == secondKey.Int64RangeKey.IntegerKeyHigh)
				{
					trueEquality = true;
				}

				if (firstKey.Int64RangeKey.IntegerKeyLow >= secondKey.Int64RangeKey.IntegerKeyLow
								&& firstKey.Int64RangeKey.IntegerKeyHigh <= secondKey.Int64RangeKey.IntegerKeyHigh)
				{
					searchCase = (firstKey.Int64RangeKey.IntegerKeyLow == firstKey.Int64RangeKey.IntegerKeyHigh);
				}

				ASSERT_IFNOT(trueEquality || searchCase, "Invalid Int64 partitions found in PartitionKey::CompareForInt64Find equality case");
				return 0;
			}
		}

		Common::ErrorCode FromPublicApi(
            FABRIC_PARTITION_KEY_TYPE keyType,
            const void * partitionKey);

		bool IsValidTargetKey()
		{
			switch (keyType_)
			{
			case FABRIC_PARTITION_KEY_TYPE_NONE: return true;
			case FABRIC_PARTITION_KEY_TYPE_INT64: return integerRangeKey_.IntegerKeyHigh == integerRangeKey_.IntegerKeyLow;
			case FABRIC_PARTITION_KEY_TYPE_STRING: return true;
			}

			Common::Assert::CodingError("Invalid key type in PartitionKey::IsValidTargetKey");
		}

        FABRIC_FIELDS_05(keyType_, integerRangeKey_.IntegerKeyLow, integerRangeKey_.IntegerKeyHigh, stringKey_, isWholeService_);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        void WriteToEtw(uint16 contextSequenceId) const;

        // Used by CITs only
        static PartitionKey Test_GetWholeServicePartitionKey();

    private:
        FABRIC_PARTITION_KEY_TYPE keyType_;
		INTEGER_PARTITION_KEY_RANGE integerRangeKey_;
        std::wstring stringKey_;

		// isWholeService is a flag that signifies "all partitions" a kind of wildcard functionality
		// meant to be signified by a NULL partition key that does not have a key type of NONE
		// not currently a supoported feature in naming: see defect 847196
        bool isWholeService_;
    };

	class dummy
	{
	public:
		void method(const PartitionKey & )
		{
		}

	private:

		struct AvlNode : public PartitionKey
		{
			AvlNode(__in const PartitionKey& KeyVal) :
				PartitionKey(KeyVal)
			{
			}
        
			AvlNode(__in const PartitionKey& KeyVal, __in const ReliableSessionServiceSPtr& Src) :
				PartitionKey(KeyVal), Object(Src)
			{
			}
            
			ReliableSessionServiceSPtr Object;            
		};   
	};

}
