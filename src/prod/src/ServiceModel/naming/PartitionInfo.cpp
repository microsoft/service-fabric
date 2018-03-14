// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    INITIALIZE_SIZE_ESTIMATION(PartitionInfo)
        
    PartitionInfo::PartitionInfo()
      : scheme_(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON)
      , lowKeyInclusive_(0)
      , highKeyInclusive_(0)
      , partitionName_()
    {
    }

    PartitionInfo::PartitionInfo(__int64 lowKeyInclusive, __int64 highKeyInclusive) 
      : scheme_(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE)
      , lowKeyInclusive_(lowKeyInclusive)
      , highKeyInclusive_(highKeyInclusive)
      , partitionName_()
    {
    }

    PartitionInfo::PartitionInfo(wstring const & partitionName) 
      : scheme_(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED)
      , lowKeyInclusive_(0)
      , highKeyInclusive_(0)
      , partitionName_(partitionName)
    {
        if (partitionName_.empty())
        {
            Assert::TestAssert("Named partition can't have an empty name");
        }
    }

    PartitionInfo::PartitionInfo(FABRIC_PARTITION_SCHEME scheme)
      : scheme_(scheme)
      , lowKeyInclusive_(0)
      , highKeyInclusive_(0)
      , partitionName_()
    {
        ASSERT_IFNOT(
            scheme_ == FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON,
            "Wrong constructor for scheme_ {0}",
            static_cast<int>(scheme_));
    }

    bool PartitionInfo::RangeContains(PartitionKey const & key) const
    {
        switch (scheme_)
        {
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
            if (key.KeyType != FABRIC_PARTITION_KEY_TYPE_INT64)
            {
                return false;
            }
            return lowKeyInclusive_ <= key.Int64Key && key.Int64Key <= highKeyInclusive_;
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
            if (key.KeyType != FABRIC_PARTITION_KEY_TYPE_STRING)
            {
                return false;
            }
            return partitionName_ == key.StringKey;
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON:
            return key.KeyType == FABRIC_PARTITION_KEY_TYPE_NONE;
        default:
            Assert::CodingError("Unknown FABRIC_PARTITION_SCHEME {0}", static_cast<int>(scheme_));
        }
    }

    bool PartitionInfo::RangeContains(PartitionInfo const & other) const
    {
        if (PartitionScheme != other.PartitionScheme)
        {
            return false;
        }

        switch (PartitionScheme)
        {
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
            return (LowKey <= other.HighKey &&
                    HighKey >= other.LowKey);
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
            return PartitionName == other.PartitionName;
        default:
            return true;
        }
    }

    void PartitionInfo::WriteTo(TextWriter & w, FormatOptions const &) const
    {
        switch (scheme_)
        {
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
            w << "[" << lowKeyInclusive_ << "," << highKeyInclusive_ << "]";
            break;
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
            w << "[" << partitionName_ << "]";
            break;
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON:
            w << "[" << "-singleton-" << "]";
            break;
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_INVALID:
            w << "[" << "-invalid-" << "]";
            break;
        default:
            w << "[" << "Unknown value " << static_cast<int>(scheme_) << "]";
            break;
        }
    }

    string PartitionInfo::AddField(Common::TraceEvent &traceEvent, string const& name)
    {
        traceEvent.AddField<wstring>(name);
        return string();
    }

    void PartitionInfo::FillEventData(TraceEventContext &context) const
    {
        switch (scheme_)
        {
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
            {
                wstring data = wformatString("[{0}-{1}]", lowKeyInclusive_, highKeyInclusive_);
                context.WriteCopy(data);
                break;
            }
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
            {
                wstring data = wformatString("[{0}]", partitionName_);
                context.WriteCopy(data);
                break;
            }
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON:
            context.WriteCopy(L"[-singleton-]");
            break;
        case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_INVALID:
            context.WriteCopy(L"-invalid-");
            break;
        default:
            {
                wstring data = wformatString("Unknown value {0}", static_cast<int>(scheme_));
                context.WriteCopy(data);
                break;
            }
        }
    }

    std::wstring PartitionInfo::ToString() const
    {
        std::wstring text;
        Common::StringWriter writer(text);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return text;
    }
}
