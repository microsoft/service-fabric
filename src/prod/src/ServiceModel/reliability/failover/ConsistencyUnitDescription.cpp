// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Reliability
{
    ConsistencyUnitDescription & ConsistencyUnitDescription::operator=(ConsistencyUnitDescription && other)
    {
        if (this != &other)
        {
            consistencyUnitId_ = std::move(other.consistencyUnitId_);
            lowKeyInclusive_ = std::move(other.lowKeyInclusive_);
            highKeyInclusive_ = std::move(other.highKeyInclusive_);
            partitionName_ = std::move(other.partitionName_);
            partitionKind_ = std::move(other.partitionKind_);
        }
        return *this;
    }

    bool ConsistencyUnitDescription::CompareForSort(ConsistencyUnitDescription const & left, ConsistencyUnitDescription const & right)
    {
        if (left.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE && right.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
        {
            return left.highKeyInclusive_ < right.lowKeyInclusive_;
        }
        else if (left.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_NAMED && right.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_NAMED)
        {
            return left.partitionName_ < right.partitionName_;
        }
        else if (left.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_SINGLETON && right.PartitionKind == FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
        {
            return left.consistencyUnitId_ < right.consistencyUnitId_;
        }
        else
        {
            Assert::CodingError(
                "Cannot compare different partition kinds: {0}, {1}", 
                static_cast<int>(left.PartitionKind), 
                static_cast<int>(right.PartitionKind));
        }
    }

    void ConsistencyUnitDescription::WriteTo(__in TextWriter& w, FormatOptions const&) const
    {
        w << consistencyUnitId_ << ", " << static_cast<int>(partitionKind_);

        switch (partitionKind_)
        {
        case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
            w << " : [" << lowKeyInclusive_ << ", " << highKeyInclusive_ << "]";
            break;
        case FABRIC_SERVICE_PARTITION_KIND_NAMED:
            w << " : \"" << partitionName_ << "\"";
            break;
        }
    }

    void ConsistencyUnitDescription::WriteToEtw(uint16 contextSequenceId) const
    {
        wstring partitionKindDetails;
        switch (partitionKind_)
        {
        case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
            partitionKindDetails = wformatString(" : [{0}, {1}]", lowKeyInclusive_, highKeyInclusive_);
            break;
        case FABRIC_SERVICE_PARTITION_KIND_NAMED:
            partitionKindDetails = wformatString(" : \"{0}\"", partitionName_);
            break;
        }

        ServiceModel::ServiceModelEventSource::Trace->ConsistencyUnitDescription(
            contextSequenceId,
            consistencyUnitId_.Guid,
            static_cast<int>(partitionKind_),
            partitionKindDetails);
    }
}
