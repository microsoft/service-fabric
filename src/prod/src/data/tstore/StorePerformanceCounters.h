// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class StorePerformanceCounters;
        using StorePerformanceCountersSPtr = std::shared_ptr<StorePerformanceCounters>;

        class StorePerformanceCounters
        {
            DENY_COPY(StorePerformanceCounters)
        public:
            BEGIN_COUNTER_SET_DEFINITION(
                L"6AD19602-3502-453F-ACD7-1C6CE3FA0F8A",
                L"Native Store",
                L"Counters for Native Store",
                Common::PerformanceCounterSetInstanceType::Multiple)
                COUNTER_DEFINITION(
                    1,
                    Common::PerformanceCounterType::RawData64,
                    L"Item Count",
                    L"Number of items in the store")
                COUNTER_DEFINITION(
                    2,
                    Common::PerformanceCounterType::RawData64,
                    L"Disk Size",
                    L"Total disk size, in bytes, of checkpoint files for the store")
                COUNTER_DEFINITION(
                    3,
                    Common::PerformanceCounterType::RawData64,
                    L"Memory Size",
                    L"Total size, in bytes, of in-memory data for the store")
                COUNTER_DEFINITION(
                    4,
                    Common::PerformanceCounterType::RawData64,
                    L"Checkpoint File Write Bytes/sec",
                    L"Number of bytes written per second for the last checkpoint file")
                COUNTER_DEFINITION(
                    5,
                    Common::PerformanceCounterType::RawData64,
                    L"Store Copy Disk Transfer Bytes/sec",
                    L"Number of disk bytes read (on primary) or written (on secondary) per second on store copy")
            END_COUNTER_SET_DEFINITION()

            DECLARE_COUNTER_INSTANCE(ItemCount)
            DECLARE_COUNTER_INSTANCE(DiskSize)
            DECLARE_COUNTER_INSTANCE(MemorySize)
            DECLARE_COUNTER_INSTANCE(CheckpointFileWriteBytesPerSec)
            DECLARE_COUNTER_INSTANCE(CopyDiskTransferBytesPerSec)

            BEGIN_COUNTER_SET_INSTANCE(StorePerformanceCounters)
                DEFINE_COUNTER_INSTANCE(ItemCount, 1)
                DEFINE_COUNTER_INSTANCE(DiskSize, 2)
                DEFINE_COUNTER_INSTANCE(MemorySize, 3)
                DEFINE_COUNTER_INSTANCE(CheckpointFileWriteBytesPerSec, 4)
                DEFINE_COUNTER_INSTANCE(CopyDiskTransferBytesPerSec, 5)
            END_COUNTER_SET_INSTANCE()

        public:
            static StorePerformanceCountersSPtr CreateInstance(
                __in Common::Guid const& partitionId,
                __in FABRIC_REPLICA_ID const& replicaId,
                __in FABRIC_STATE_PROVIDER_ID const& stateProviderId);
        };
    }
}
