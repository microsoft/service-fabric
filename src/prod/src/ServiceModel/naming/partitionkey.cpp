// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace std;

    INITIALIZE_SIZE_ESTIMATION(PartitionKey)

    PartitionKey::PartitionKey()
      : keyType_(FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE)
      , intKey_(0)
      , stringKey_()
      , isWholeService_(false)
    {
    }

    PartitionKey::PartitionKey(__int64 intKey)
      : keyType_(FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64)
      , intKey_(intKey)
      , stringKey_()
      , isWholeService_(false)
    {
    }

    PartitionKey::PartitionKey(std::wstring && stringKey)
      : keyType_(FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING)
      , intKey_(0)
      , stringKey_(std::move(stringKey))
      , isWholeService_(false)
    {
        if (stringKey_.empty())
        {
            Assert::TestAssert("Named partition can't have an empty name");
        }
    }

    PartitionKey::PartitionKey(std::wstring const & stringKey)
      : keyType_(FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING)
      , intKey_(0)
      , stringKey_(stringKey)
      , isWholeService_(false)
    {
        if (stringKey_.empty())
        {
            Assert::TestAssert("Named partition can't have an empty name");
        }
    }

    PartitionKey::PartitionKey(PartitionKey && other)
      : keyType_(other.keyType_)
      , stringKey_(std::move(other.stringKey_))
      , intKey_(other.intKey_)
      , isWholeService_(other.isWholeService_)
    {
    }

    ErrorCode PartitionKey::FromPublicApi(
        FABRIC_PARTITION_KEY_TYPE keyType,
        const void * partitionKey)
    {
        if (partitionKey == NULL && keyType != FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE)
        {
            // Notifications for all partitions are not currently supported.
            // In the future, we can consider NULL partition as a request for all partitions.
            // Defect 847196: support notifications for all partitions tracks this.
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
                intKey_ = *(reinterpret_cast<const __int64*>(partitionKey));
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
            w << intKey_;
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
            w << stringKey_;
            break;
        default:
            w << "[" << "Unknown value " << static_cast<int>(keyType_)<< "]";
            break;
        }
    }

    string PartitionKey::AddField(Common::TraceEvent &traceEvent, string const& name)
    {
        traceEvent.AddField<wstring>(name);
        return string();
    }

    void PartitionKey::FillEventData(TraceEventContext & context) const
    {
        switch(keyType_)
        {
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INVALID:
            context.WriteCopy(L"-invalid-");
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE:
            {
                wstring data = wformatString("NONE(all={0})", isWholeService_);
                context.WriteCopy(data);
                break;
            }
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64:
            {
                wstring data = wformatString("{0}", intKey_);
                context.WriteCopy(data);
                break;
            }
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
            context.WriteCopy(stringKey_);
            break;
        default:
            {
                wstring data = wformatString("Unknown value {0}", static_cast<int>(keyType_));
                context.WriteCopy(data);
                break;
            }
        }
    }

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
