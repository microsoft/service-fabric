// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Naming
{
    NameRangeTuple::NameRangeTuple(NamingUri const & name, PartitionInfo const & info)
      : name_(name)
      , info_(info)
    {
    }

    NameRangeTuple::NameRangeTuple(NamingUri const & name, PartitionKey const & key)
      : name_(name)
    {
        switch(key.KeyType)
        {
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64:
            info_ = PartitionInfo(key.Int64Key, key.Int64Key);
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
            info_ = PartitionInfo(key.StringKey);
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE:
            info_ = PartitionInfo();
            break;
        default:
            Common::Assert::CodingError("Unexpected key type {0}.", static_cast<unsigned int>(key.KeyType));
        }
    }

    NameRangeTuple::NameRangeTuple(NamingUri && name, PartitionKey && key)
      : name_(std::move(name))
    {
        switch(key.KeyType)
        {
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64:
            info_ = PartitionInfo(key.Int64Key, key.Int64Key);
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
            info_ = PartitionInfo(std::move(key.StringKey));
            break;
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE:
            info_ = PartitionInfo();
            break;
        default:
            Common::Assert::CodingError("Unexpected key type {0}.", static_cast<unsigned int>(key.KeyType));
        }
    }

    void NameRangeTuple::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << name_<< L":" << info_;
    }

    std::string NameRangeTuple::AddField(Common::TraceEvent & traceEvent, std::string const & name)
    {
        std::string format = "{0}:{1}";
        size_t index = 0;

        traceEvent.AddEventField<Common::Uri>(format, name + ".name", index);
        traceEvent.AddEventField<PartitionInfo>(format, name + ".info", index);
            
        return format;
    }

    void NameRangeTuple::FillEventData(Common::TraceEventContext & context) const
    {
        context.Write<Common::Uri>(name_);
        context.Write<PartitionInfo>(info_);
    }
}
