// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ComProxyStatefulServicePartition
        {
        public:
            ComProxyStatefulServicePartition();
            explicit ComProxyStatefulServicePartition(
                Common::ComPointer<IFabricStatefulServicePartition> && servicePartition);
            
            ComProxyStatefulServicePartition(ComProxyStatefulServicePartition const & other);
            ComProxyStatefulServicePartition(ComProxyStatefulServicePartition && other);
            ComProxyStatefulServicePartition & operator=(ComProxyStatefulServicePartition && other);

            Common::ErrorCode GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const ** partitionInfo ) const;

            FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus() const;
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus() const;
            Common::ErrorCode ReportFault(::FABRIC_FAULT_TYPE faultType) const;
            
        private:
            Common::ComPointer<IFabricStatefulServicePartition> servicePartition_;
        };
    }
}
