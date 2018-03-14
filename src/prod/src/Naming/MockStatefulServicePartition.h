// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// *************
// For CITs only
// *************

namespace Naming
{
    class MockStatefulServicePartition
       : public IFabricStatefulServicePartition3
       , public Common::ComUnknownBase
    {
        DENY_COPY(MockStatefulServicePartition);

        BEGIN_COM_INTERFACE_LIST(MockStatefulServicePartition)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricStatefulServicePartition2)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition,IFabricStatefulServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition1,IFabricStatefulServicePartition1)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition2,IFabricStatefulServicePartition2)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition3, IFabricStatefulServicePartition3)
        END_COM_INTERFACE_LIST()


    public:
        explicit MockStatefulServicePartition(
            Common::Guid const & partitionId)
            : servicePartitionInfo_()
        {
            rangePartitionInfo_.Id = partitionId.AsGUID();
            rangePartitionInfo_.LowKey = 0;
            rangePartitionInfo_.HighKey = 0;
            rangePartitionInfo_.Reserved = NULL;
            servicePartitionInfo_.Kind = ::FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE;
            servicePartitionInfo_.Value = reinterpret_cast<void*>(&rangePartitionInfo_);
        }

        void Open()
        {
            ::FABRIC_SEQUENCE_NUMBER currentProgress;
            HRESULT hr = stateProviderCPtr_->GetLastCommittedSequenceNumber(&currentProgress);
            ASSERT_IFNOT(SUCCEEDED(hr), "GetLastCommittedSequenceNumber failed: hr = {0}", hr);

            mockReplicatorCPtr_->InitializeSequenceNumber(currentProgress);
        }

        void Close()
        {
            stateProviderCPtr_.SetNoAddRef(NULL);
            mockReplicatorCPtr_.SetNoAddRef(NULL);
        }

        HRESULT STDMETHODCALLTYPE GetPartitionInfo(__out ::FABRIC_SERVICE_PARTITION_INFORMATION const ** value)
        {
            if (value == NULL) { return E_POINTER; }

            *value = &servicePartitionInfo_;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetReadStatus(__out ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS * readStatus)
        {
            if (readStatus == NULL) { return E_POINTER; }

            *readStatus = ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetWriteStatus(__out ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS * writeStatus)
        {
            if (writeStatus == NULL) { return E_POINTER; }

            *writeStatus = ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE CreateReplicator(
            __in IFabricStateProvider * stateProvider,
            __in_opt ::FABRIC_REPLICATOR_SETTINGS const *,
            __out IFabricReplicator ** replicator,
            __out IFabricStateReplicator ** stateReplicator)
        {
            if (stateProvider == NULL || replicator == NULL || stateReplicator == NULL) { return E_POINTER; }

            HRESULT hr = stateProvider->QueryInterface(IID_IFabricStateProvider, reinterpret_cast<void**>(stateProviderCPtr_.InitializationAddress()));
            ASSERT_IFNOT(SUCCEEDED(hr), "stateProvider does not support IFabricStateProvider");

            mockReplicatorCPtr_ = Common::make_com<MockReplicator>();
            hr = mockReplicatorCPtr_->QueryInterface(IID_IFabricReplicator, reinterpret_cast<void**>(replicator));
            ASSERT_IFNOT(SUCCEEDED(hr), "MockReplicator does not support IFabricReplicator");

            hr = mockReplicatorCPtr_->QueryInterface(IID_IFabricStateReplicator, reinterpret_cast<void**>(stateReplicator));
            ASSERT_IFNOT(SUCCEEDED(hr), "MockReplicator does not support IFabricStateReplicator");

            return hr;
        }

        HRESULT STDMETHODCALLTYPE ReportLoad(__in ULONG metricCount, __in ::FABRIC_LOAD_METRIC const * metrics)
        {
            UNREFERENCED_PARAMETER(metricCount);
            UNREFERENCED_PARAMETER(metrics);

            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE ReportFault(__in ::FABRIC_FAULT_TYPE faultType)
        {
            UNREFERENCED_PARAMETER(faultType);

            return E_NOTIMPL;
        }

    public:
        
        // IFabricStatefulServicePartition3

        virtual HRESULT ReportReplicaHealth2(const FABRIC_HEALTH_INFORMATION *, FABRIC_HEALTH_REPORT_SEND_OPTIONS const *) override { return E_NOTIMPL; };

        HRESULT ReportPartitionHealth2(const FABRIC_HEALTH_INFORMATION *, FABRIC_HEALTH_REPORT_SEND_OPTIONS const *) override { return E_NOTIMPL; }

        // IFabricStatefulServicePartition2

        virtual HRESULT ReportReplicaHealth(const FABRIC_HEALTH_INFORMATION *) override { return E_NOTIMPL; };

        HRESULT ReportPartitionHealth(const FABRIC_HEALTH_INFORMATION *) override { return E_NOTIMPL; }

        // IFabricStatefulServicePartition1

        HRESULT ReportMoveCost(FABRIC_MOVE_COST) override { return E_NOTIMPL; }

    private:
        ::FABRIC_SERVICE_PARTITION_INFORMATION servicePartitionInfo_;
        ::FABRIC_INT64_RANGE_PARTITION_INFORMATION rangePartitionInfo_;

        Common::ComPointer<IFabricStateProvider> stateProviderCPtr_;
        Common::ComPointer<MockReplicator> mockReplicatorCPtr_;
    };
}
