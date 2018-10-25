// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TestCommon
    {
        class TestStatefulServicePartition final
            : public Utilities::IStatefulPartition
        {
            K_FORCE_SHARED(TestStatefulServicePartition)

        public:
            static SPtr Create(__in KAllocator & allocator);
            static SPtr Create(__in KGuid partitionId,__in KAllocator & allocator);

            NTSTATUS GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** BufferedValue) override;
            
            NTSTATUS GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* ReadStatus) override;

            NTSTATUS GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* WriteStatus) override;
            
            NTSTATUS ReportFault(__in FABRIC_FAULT_TYPE FaultType) override;

        public: // Test APIs.
            void SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus);
            void SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus);

        private:
            TestStatefulServicePartition(__in KGuid partitionId);

        private:
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus_;
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus_;
            FABRIC_SERVICE_PARTITION_INFORMATION partitionInfo_;
            FABRIC_SINGLETON_PARTITION_INFORMATION singletonPartitionInfo_;
        };
    }
}
