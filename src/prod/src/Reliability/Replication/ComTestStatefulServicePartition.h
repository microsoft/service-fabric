// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReplicationUnitTest
{
    class ComTestStatefulServicePartition : 
        public IFabricStatefulServicePartition, private Common::ComUnknownBase
    {
        DENY_COPY(ComTestStatefulServicePartition)

        COM_INTERFACE_LIST1(
            ComTestStatefulServicePartition,
            IID_IFabricStatefulServicePartition,
            IFabricStatefulServicePartition)
        
    public:
        explicit ComTestStatefulServicePartition(Common::Guid const & partitionId);
        
        virtual ~ComTestStatefulServicePartition() {}

        HRESULT __stdcall GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const ** bufferedValue);
        HRESULT __stdcall GetReadStatus(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus);
        HRESULT __stdcall GetWriteStatus(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus);
        HRESULT __stdcall CreateReplicator(
            __in ::IFabricStateProvider *stateProvider,
            __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
            __out ::IFabricReplicator **replicator,
            __out ::IFabricStateReplicator **stateReplicator);
        HRESULT __stdcall ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics);
        HRESULT __stdcall ReportFault(FABRIC_FAULT_TYPE faultType);
        void SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus);
        void SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus);
        
    private:
        FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus_;
        FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus_;
        FABRIC_SERVICE_PARTITION_INFORMATION partitionInfo_;
        FABRIC_SINGLETON_PARTITION_INFORMATION singletonPartitionInfo_;
    };
}
