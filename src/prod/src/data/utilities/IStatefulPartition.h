// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class IStatefulPartition
            : public KObject<IStatefulPartition>
            , public KShared<IStatefulPartition>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(IStatefulPartition);

        public:
            virtual NTSTATUS GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** partitionInfo) = 0;

            virtual NTSTATUS GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* readStatus) = 0;

            virtual NTSTATUS GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* writeStatus) = 0;

            virtual NTSTATUS ReportFault(__in FABRIC_FAULT_TYPE faultType) = 0;
        };

        inline IStatefulPartition::IStatefulPartition() {}
        inline IStatefulPartition::~IStatefulPartition() {}
    }
}
