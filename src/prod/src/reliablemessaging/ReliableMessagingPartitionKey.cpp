// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Stdafx.h"

namespace ReliableMessaging
{
    using namespace Common;
    using namespace std;

    PartitionKey::PartitionKey()
      : keyType_(FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE)
      , isWholeService_(false)
    {
    }

    PartitionKey::PartitionKey(INTEGER_PARTITION_KEY_RANGE const & integerRangeKey)
      : keyType_(FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64)
      , integerRangeKey_(integerRangeKey)
      , isWholeService_(false)
    {
		TESTASSERT_IFNOT(integerRangeKey.IntegerKeyLow <= integerRangeKey.IntegerKeyHigh, "Invalid Integer Key Range for Partition");
    }

    PartitionKey::PartitionKey(std::wstring && stringKey)
      : keyType_(FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING)
      , stringKey_(std::move(stringKey))
      , isWholeService_(false)
    {
        TESTASSERT_IF(stringKey_.empty(), "Named partition can't have an empty name");
    }

    PartitionKey::PartitionKey(std::wstring const & stringKey)
      : keyType_(FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING)
      , stringKey_(stringKey)
      , isWholeService_(false)
    {
        TESTASSERT_IF(stringKey_.empty(), "Named partition can't have an empty name");
    }

    PartitionKey::PartitionKey(const PartitionKey & other)
      : keyType_(other.keyType_)
      , stringKey_(other.stringKey_)
      , integerRangeKey_(other.integerRangeKey_)
      , isWholeService_(other.isWholeService_)
    {
    }

    PartitionKey::PartitionKey(PartitionKey && other)
      : keyType_(other.keyType_)
      , stringKey_(std::move(other.stringKey_))
      , integerRangeKey_(other.integerRangeKey_)
      , isWholeService_(other.isWholeService_)
    {
    }

    ErrorCode PartitionKey::FromPublicApi(
        FABRIC_PARTITION_KEY_TYPE keyType,
        const void * partitionKey)
    {
        if (partitionKey == nullptr && keyType != FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE)
        {
            // TODO: Notifications for all partitions are not currently supported;
            // in the future, we can consider NULL partition as a request for all partitions.
            // defect 847196: support notifications for all partitions tracks this
            isWholeService_ = true;
            return ErrorCode::FromHResult(E_POINTER);
        }
        
        switch (keyType)
        {
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE:
            {
                keyType_ = FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE;
                return ErrorCode(ErrorCodeValue::Success);
            }
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64:
            {
                keyType_ = FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64;
                integerRangeKey_ = *(reinterpret_cast<const INTEGER_PARTITION_KEY_RANGE*>(partitionKey));
                return ErrorCode(ErrorCodeValue::Success);
            }
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
            {
                wstring pk(reinterpret_cast<LPCWSTR>(partitionKey));
                if (pk.empty())
                {
                    return ErrorCode(ErrorCodeValue::InvalidArgument);
                }

                keyType_ = FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING;
                stringKey_ = move(pk);
                return ErrorCode(ErrorCodeValue::Success);
            }
        default:
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }
    }


    void PartitionKey::WriteTo(__in TextWriter & w, FormatOptions const &) const
    {
        switch(keyType_)
        {
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INVALID:
            w << "[-invalid-]";
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE:
            w << (isWholeService_ ? "(all)" : "(none)");
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64:
			w << "Lowest: " << integerRangeKey_.IntegerKeyLow << " Highest: " << integerRangeKey_.IntegerKeyHigh;
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
            w << stringKey_;
            break;
        default:
            w << "[" << "Unknown value " << static_cast<int>(keyType_) << "]";
            break;
        }
    }

	/*
    void PartitionKey::WriteToEtw(uint16 contextSequenceId) const
    {
        switch(keyType_)
        {
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INVALID:
            NamingCommonEventSource::EventSource->InvalidEnumValue(
                contextSequenceId);
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE:
            NamingCommonEventSource::EventSource->PartitionKeySingleton(
                contextSequenceId,
                isWholeService_);
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64:
            NamingCommonEventSource::EventSource->PartitionKeyInt64(
                contextSequenceId,
                intKey_);
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
            NamingCommonEventSource::EventSource->PartitionKeyNamed(
                contextSequenceId,
                stringKey_);
            break;
        default:
            NamingCommonEventSource::EventSource->UnknownEnumValue(
                contextSequenceId,
                static_cast<int>(keyType_));
            break;
        }
    }
	*/



	


    std::wstring PartitionKey::ToString() const
    {
        std::wstring text;
        Common::StringWriter writer(text);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return text;
    }

    PartitionKey PartitionKey::Test_GetWholeServicePartitionKey() 
    { 
        PartitionKey pk; 
        pk.isWholeService_ = true; 
        return pk; 
    }

}
