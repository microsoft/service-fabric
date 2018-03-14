// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class Partition final
            : public KWfStatefulServicePartition
        {
            K_FORCE_SHARED(Partition)

        public:

            static NTSTATUS Create(
                __in IFabricStatefulServicePartition & fabricPartition,
                __in KAllocator & allocator,
                __out KWfStatefulServicePartition::SPtr & result);

            NTSTATUS
                GetPartitionInfo(
                    __out const FABRIC_SERVICE_PARTITION_INFORMATION** BufferedValue
                ) override;

            NTSTATUS
                GetReadStatus(
                    __out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* ReadStatus
                ) override;

            NTSTATUS
                GetWriteStatus(
                    __out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* WriteStatus
                ) override;

            NTSTATUS
                CreateReplicator(
                    __in KWfStateProvider* StateProvider,
                    __in const FABRIC_REPLICATOR_SETTINGS* ReplicatorSettings,
                    __out KSharedPtr<KWfReplicator>& Replicator,
                    __out KSharedPtr<KWfStateReplicator>& StateReplicator
                ) override;

            NTSTATUS
                ReportLoad(
                    __in ULONG MetricCount,
                    __in_ecount(MetricCount) const FABRIC_LOAD_METRIC* Metrics
                ) override;

            NTSTATUS
                ReportFault(
                    __in FABRIC_FAULT_TYPE FaultType
                ) override;

        private:

            Partition(__in IFabricStatefulServicePartition & FabricServicePartition);

            Common::ComPointer<IFabricStatefulServicePartition> fabricServicePartition_;
        };
    }
}
