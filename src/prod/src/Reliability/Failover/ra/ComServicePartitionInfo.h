// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComServicePartitionInfo 
        {
            DENY_COPY(ComServicePartitionInfo)

            public:
                ComServicePartitionInfo(
                    Common::Guid const & partitionId,
                    Reliability::ConsistencyUnitDescription const & consistencyUnitDescription);

                __declspec(property(get=get_Value)) FABRIC_SERVICE_PARTITION_INFORMATION const * Value;
                inline FABRIC_SERVICE_PARTITION_INFORMATION const * get_Value() const { return &value_; }

            private:
                FABRIC_SERVICE_PARTITION_INFORMATION value_;
                FABRIC_SINGLETON_PARTITION_INFORMATION singletonPartitionInfo_;
                FABRIC_INT64_RANGE_PARTITION_INFORMATION int64RangePartitionInfo_;
                FABRIC_NAMED_PARTITION_INFORMATION namedPartitionInfo_;
                std::wstring partitionName_;
        };
    }
}
