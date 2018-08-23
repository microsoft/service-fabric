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
            : public IStatefulPartition
        {
            K_FORCE_SHARED(Partition)

        public:

            static NTSTATUS Create(
                __in IFabricStatefulServicePartition & fabricPartition,
                __in KAllocator & allocator,
                __out IStatefulPartition::SPtr & result);

            NTSTATUS GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** partitionInfo) override;

            NTSTATUS GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* readStatus) override;

            NTSTATUS GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* writeStatus) override;

            NTSTATUS ReportFault(__in FABRIC_FAULT_TYPE faultType) override;

        private:

            Partition(__in IFabricStatefulServicePartition & FabricServicePartition);

            Common::ComPointer<IFabricStatefulServicePartition> fabricServicePartition_;
        };
    }
}
