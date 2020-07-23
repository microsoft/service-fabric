// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class Service : public TestableService::TestableServiceBase
    {
    public:
        static const std::wstring warehousesSPName;
        static const std::wstring districtsSPName;
        static const std::wstring customersSPName;
        static const std::wstring newOrdersSPName;
        static const std::wstring ordersSPName;
        static const std::wstring orderLineSPName;
        static const std::wstring itemsSPName;
        static const std::wstring stockSPName;
        static const std::wstring historySPName;

    public:
        Service(
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);

        virtual ~Service();

        virtual HRESULT STDMETHODCALLTYPE BeginOpen(
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        virtual HRESULT STDMETHODCALLTYPE EndOpen(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicationEngine) override;

        virtual HRESULT STDMETHODCALLTYPE BeginChangeRole(
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        virtual HRESULT STDMETHODCALLTYPE EndChangeRole(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceEndpoint) override;

        virtual HRESULT STDMETHODCALLTYPE BeginClose(
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        virtual HRESULT STDMETHODCALLTYPE EndClose(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        virtual void STDMETHODCALLTYPE Abort() override;

    protected:
        //
        // Return a StateProviderFactory to the caller. This method will be called by the StatefulServiceBase.
        //
        virtual Common::ComPointer<IFabricStateProvider2Factory> GetStateProviderFactory() override;

        //
        // Return a DataLossHandler to the caller. This method will be called by the StatefulServiceBase.
        //
        virtual Common::ComPointer<IFabricDataLossHandler> GetDataLossHandler() override;

    private:
        //
        // Get ktl allocator
        //
        KAllocator& GetAllocator()
        {
            return this->KtlSystemValue->PagedAllocator();
        }

        //
        // Try create the state provider with the specified 'providerName'. If the state provider
        // already exists, this method is a no-op.
        //
        ktl::Awaitable<void> GetOrCreateStateProvider(__in const wstring& providerName);

        //
        // Get or create the stores with the specified 'providerName' from replicator.
        //
        template <class TKey, class TValue>
        ktl::Awaitable<void> PopulateStateProvider(
            __in const wstring& providerame,
            __out KSharedPtr<Data::TStore::IStore<TKey, TValue>> & store);

        //
        // Populate all state providers.
        //
        ktl::Awaitable<void> PopulateStateProviders();

        //
        // Wait until this replica becomes primary
        //
        ktl::Awaitable<void> WaitForPrimaryRoleAsync();

        //
        // Schedule work on client request
        //
        virtual ktl::Awaitable<void> DoWorkAsync() override;

        //
        // Get process memory info
        //
        ServiceMetrics GetServiceMetrics();

        //
        // Get the result of DoWorkOnClientRequestAsync. In our case, we simply return
        // the number of records in the store.
        //
        virtual ktl::Awaitable<TestableService::TestableServiceProgress> GetProgressAsync() override;

        //
        // Populate the stores. Get ready for the following workload.
        //
        ktl::Awaitable<void> PopulateTpccWorkloadOnPrimaryAsync();

        ktl::Awaitable<void> NewOrderTransactionLockBased(int index);
        ktl::Awaitable<void> NewOrderTransactionVersionBased(int index);
        ktl::Awaitable<void> NewPaymentTransactionLockBased(int index);
        ktl::Awaitable<void> NewPaymentTransactionVersionBased(int index);
        ktl::Awaitable<void> NewDeliveryTransactionLockBased(int index);
        ktl::Awaitable<void> ClearOldOrdersTransactionLockBased();
        ktl::Awaitable<void> SecondaryWorkloadTransaction1(bool shouldYield);
        ktl::Awaitable<void> SecondaryWorkloadTransaction2(bool shouldYield);
        ktl::Awaitable<void> ClearHistoryTable(int index);

        ktl::Awaitable<void> Workload1(int indexCapture, bool lockBased);
        ktl::Awaitable<void> Workload2(int indexCapture, bool lockBased);

        //
        // Test logic
        //
        ktl::Awaitable<void> NewOrderTpccWorkloadOnPrimaryAsync();

        //
        // Create replicator transaction
        //
        ktl::Awaitable<TxnReplicator::Transaction::SPtr> SafeCreateReplicatorTransactionAsync();
        //
        // Commit the specified 'replicatorTransaction'.
        //
        ktl::Awaitable<void> SafeTerminateReplicatorTransactionAsync(
            TxnReplicator::Transaction::SPtr replicatorTransaction,
            bool commit);

    private:
        Common::ComPointer<IFabricStatefulServicePartition> partition_;

        KSharedPtr<Data::TStore::IStore<int, WarehouseValue::SPtr>> storeWareHouses_;
        KSharedPtr<Data::TStore::IStore<DistrictKey::SPtr, DistrictValue::SPtr>> storeDistricts_;
        KSharedPtr<Data::TStore::IStore<CustomerKey::SPtr, CustomerValue::SPtr>> storeCustomers_;
        KSharedPtr<Data::TStore::IStore<NewOrderKey::SPtr, NewOrderValue::SPtr>> storeNewOrders_;
        KSharedPtr<Data::TStore::IStore<OrderKey::SPtr, OrderValue::SPtr>> storeOrders_;
        KSharedPtr<Data::TStore::IStore<OrderLineKey::SPtr, OrderLineValue::SPtr>> storeOrderLine_;
        KSharedPtr<Data::TStore::IStore<LONG64, ItemValue::SPtr>> storeItems_;
        KSharedPtr<Data::TStore::IStore<StockKey::SPtr, StockValue::SPtr>> storeStock_;
        KSharedPtr<Data::TStore::IStore<KGuid, HistoryValue::SPtr>> storeHistory_;
    };
}
