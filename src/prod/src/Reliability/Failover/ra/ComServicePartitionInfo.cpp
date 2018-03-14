// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Reliability;
using namespace ReconfigurationAgentComponent;

ComServicePartitionInfo::ComServicePartitionInfo(
    Common::Guid const & partitionId,
    ConsistencyUnitDescription const & consistencyUnitDescription)
{
    singletonPartitionInfo_.Reserved = NULL;
    
    int64RangePartitionInfo_.LowKey = 0; 
    int64RangePartitionInfo_.HighKey = 0; 
    int64RangePartitionInfo_.Reserved = NULL;

    namedPartitionInfo_.Name = NULL;
    namedPartitionInfo_.Reserved = NULL;

    value_.Kind = consistencyUnitDescription.PartitionKind;
    switch (value_.Kind)
    {
        case FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
            singletonPartitionInfo_.Id = partitionId.AsGUID();
            value_.Value = &singletonPartitionInfo_;
            break;

        case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
            int64RangePartitionInfo_.Id = partitionId.AsGUID();
            int64RangePartitionInfo_.LowKey = consistencyUnitDescription.LowKey;
            int64RangePartitionInfo_.HighKey = consistencyUnitDescription.HighKey;
            value_.Value = &int64RangePartitionInfo_;
            break;

        case FABRIC_SERVICE_PARTITION_KIND_NAMED:
            namedPartitionInfo_.Id = partitionId.AsGUID();
            partitionName_ = consistencyUnitDescription.PartitionName;
            namedPartitionInfo_.Name = partitionName_.c_str();
            value_.Value = &namedPartitionInfo_;
            break;
        default: 
            Common::Assert::CodingError("Unknown FABRIC_SERVICE_PARTITION_KIND");
    }
}
