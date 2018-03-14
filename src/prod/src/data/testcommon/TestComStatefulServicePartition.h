// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TestCommon
    {
        class TestComStatefulServicePartition final
            : public KObject<TestComStatefulServicePartition>
            , public KShared<TestComStatefulServicePartition>
            , public IFabricStatefulServicePartition
        {
            K_FORCE_SHARED(TestComStatefulServicePartition)

            K_BEGIN_COM_INTERFACE_LIST(TestComStatefulServicePartition)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServicePartition)
                COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition, IFabricStatefulServicePartition)
            K_END_COM_INTERFACE_LIST()
            
        public:
            static SPtr Create(__in KAllocator & allocator);
            static SPtr Create(__in KGuid partitionId,__in KAllocator & allocator);

            // IFabricStatefulServicePartition members
            HRESULT STDMETHODCALLTYPE GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const **bufferedValue);
            HRESULT STDMETHODCALLTYPE GetReadStatus(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus);
            HRESULT STDMETHODCALLTYPE GetWriteStatus(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus);
            HRESULT STDMETHODCALLTYPE CreateReplicator(
                __in ::IFabricStateProvider *stateProvider,
                __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
                __out ::IFabricReplicator **replicator,
                __out ::IFabricStateReplicator **stateReplicator);
            HRESULT STDMETHODCALLTYPE ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics);
            HRESULT STDMETHODCALLTYPE ReportFault(FABRIC_FAULT_TYPE faultType);

        public: // Test APIs.
            void SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus);
            void SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus);

        private:
            TestComStatefulServicePartition(__in KGuid partitionId);

        private:
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus_;
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus_;
            FABRIC_SERVICE_PARTITION_INFORMATION partitionInfo_;
            FABRIC_SINGLETON_PARTITION_INFORMATION singletonPartitionInfo_;
        };
    }
}
