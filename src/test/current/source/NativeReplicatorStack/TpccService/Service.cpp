// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <SfStatus.h>

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TestableService;
using namespace TpccService;
using namespace TxnReplicator;
using namespace Data;

static const StringLiteral TraceComponent("TpccService");

static const wstring testSPType = L"TpccSPType";

static const ULONG SERVICE_TAG = 'svtg';

static void TraceException(
    __in const StringLiteral& component,
    __in ktl::Exception& exception,
    __in KAllocator& allocator)
{
    auto e = exception.GetInnerException<ktl::Exception>();
    while (e != nullptr)
    {
        KDynStringA stackString(allocator);
        bool getStack = e->ToString(stackString);
        UNREFERENCED_PARAMETER(getStack);

        Trace.WriteError(component, "---- From Within Exception ----");
        Trace.WriteError(component, Data::Utilities::ToStringLiteral(stackString));
        e = exception.GetInnerException<ktl::Exception>();
    }
}

const wstring Service::warehousesSPName = L"fabric:/Warehouses";
const wstring Service::districtsSPName = L"fabric:/Districts";
const wstring Service::customersSPName = L"fabric:/Customers";
const wstring Service::newOrdersSPName = L"fabric:/NewOrders";
const wstring Service::ordersSPName = L"fabric:/Orders";
const wstring Service::orderLineSPName = L"fabric:/OrderLine";
const wstring Service::itemsSPName = L"fabric:/Items";
const wstring Service::stockSPName = L"fabric:/Stock";
const wstring Service::historySPName = L"fabric:/History";

Service::Service(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : TestableServiceBase(partitionId, replicaId, root)
    , storeWareHouses_(nullptr)
    , storeDistricts_(nullptr)
    , storeCustomers_(nullptr)
    , storeNewOrders_(nullptr)
    , storeOrders_(nullptr)
    , storeOrderLine_(nullptr)
    , storeItems_(nullptr)
    , storeStock_(nullptr)
    , storeHistory_(nullptr)
{
}

Service::~Service()
{
}

HRESULT Service::BeginOpen(
    /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
    /* [in] */ IFabricStatefulServicePartition *statefulServicePartition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    Trace.WriteInfo(TraceComponent, "Begin Open...");
    partition_.SetAndAddRef((IFabricStatefulServicePartition2*)statefulServicePartition);
    return StatefulServiceBase::BeginOpen(openMode, statefulServicePartition, callback, context);
}

HRESULT Service::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [out][retval] */ IFabricReplicator **replicationEngine)
{
    Trace.WriteInfo(TraceComponent, "End Open...");
    return StatefulServiceBase::EndOpen(context, replicationEngine);
}

HRESULT Service::BeginChangeRole(
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    Trace.WriteInfo(TraceComponent, "Begin Change Role to {0}", (int)newRole);
    if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        Trace.WriteInfo(TraceComponent, "Now Primary");
    }
    return StatefulServiceBase::BeginChangeRole(newRole, callback, context);
}

HRESULT Service::EndChangeRole(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceEndpoint)
{
    Trace.WriteInfo(TraceComponent, "End Change Role...");
    return StatefulServiceBase::EndChangeRole(context, serviceEndpoint);
}

HRESULT Service::BeginClose(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    Trace.WriteInfo(TraceComponent, "Begin Close...");
    return StatefulServiceBase::BeginClose(callback, context);
}

HRESULT Service::EndClose(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    Trace.WriteInfo(TraceComponent, "End Close...");
    return StatefulServiceBase::EndClose(context);
}

void Service::Abort()
{
    Trace.WriteInfo(TraceComponent, "Abort...");
    StatefulServiceBase::Abort();
}

ComPointer<IFabricStateProvider2Factory> Service::GetStateProviderFactory()
{
    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();
    return TestComStateProvider2Factory::Create(this->ReplicaId, allocator);
}

ComPointer<IFabricDataLossHandler> Service::GetDataLossHandler()
{
    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();
    return TxnReplicator::TestCommon::TestComProxyDataLossHandler::Create(allocator);
}

ktl::Awaitable<void> Service::GetOrCreateStateProvider(__in const wstring& providerName)
{
    using namespace Data;
    using namespace Data::TStore;
    Trace.WriteInfo(TraceComponent, "----- Start GetOrCreateStateProvider -----");
    TxnReplicator::Transaction::SPtr txn;
    THROW_ON_FAILURE(TxReplicator->CreateTransaction(txn));
    KFinally([&] { txn->Dispose(); });
    TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
    bool alreadyExist = false;
    NTSTATUS status = co_await TxReplicator->GetOrAddAsync(
        *txn,
        &providerName[0],
        &testSPType[0],
        providerSP,
        alreadyExist);
    THROW_ON_FAILURE(status);
    THROW_ON_FAILURE(co_await txn->CommitAsync());
    Trace.WriteInfo(TraceComponent, "----- End GetOrCreateStateProvider -----");
}

template <class TKey, class TValue>
ktl::Awaitable<void> Service::PopulateStateProvider(
    __in const wstring& providerName,
    __out KSharedPtr<Data::TStore::IStore<TKey, TValue>> & store)
{
    using namespace Data;
    using namespace Data::TStore;

    Trace.WriteInfo(TraceComponent, "----- Start PopulateStateProvider -----");

    TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
    NTSTATUS status = TxReplicator->Get(&providerName[0], providerSP);

    if (NT_SUCCESS(status))
    {
        store = dynamic_cast<IStore<TKey, TValue>*>(providerSP.RawPtr());
    }
    else if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        Trace.WriteError(TraceComponent, "Failed to get stateprovider {0}.", providerName);
        store = nullptr;
    }
    else
    {
        throw ktl::Exception(status);
    }

    Trace.WriteInfo(TraceComponent, "----- End PopulateStateProvider -----");

    co_return;
}

ktl::Awaitable<void> Service::PopulateStateProviders()
{
    Trace.WriteInfo(TraceComponent, "----- Start PopulateStateProviders -----");

#define GET_OR_CREATE_STORE(store, name)\
    if (Role == FABRIC_REPLICA_ROLE_PRIMARY)\
    {\
        co_await GetOrCreateStateProvider(name);\
    }\
    co_await PopulateStateProvider(name, store);

    GET_OR_CREATE_STORE(storeWareHouses_, warehousesSPName);
    GET_OR_CREATE_STORE(storeDistricts_, districtsSPName);
    GET_OR_CREATE_STORE(storeCustomers_, customersSPName);
    GET_OR_CREATE_STORE(storeNewOrders_, newOrdersSPName);
    GET_OR_CREATE_STORE(storeOrders_, ordersSPName);
    GET_OR_CREATE_STORE(storeOrderLine_, orderLineSPName);
    GET_OR_CREATE_STORE(storeItems_, itemsSPName);
    GET_OR_CREATE_STORE(storeStock_, stockSPName);
    GET_OR_CREATE_STORE(storeHistory_, historySPName);

#undef GET_OR_CREATE_STORE

    Trace.WriteInfo(TraceComponent, "----- End PopulateStateProviders -----");
    co_return;
}

ktl::Awaitable<void> Service::WaitForPrimaryRoleAsync()
{
    Trace.WriteInfo(TraceComponent, "----- Start WaitForPrimaryRoleAsync -----");

    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();
    while (Role != FABRIC_REPLICA_ROLE_PRIMARY)
    {
        Trace.WriteWarning(TraceComponent, "Not Primary. Waiting...");
        NTSTATUS status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
    }

    Trace.WriteInfo(TraceComponent, "----- End WaitForPrimaryRoleAsync -----");

    co_return;
}

ktl::Awaitable<void> Service::DoWorkAsync()
{
    Trace.WriteInfo(TraceComponent, "----- Start DoWorkAsync -----");

    // wait to be primary
    co_await WaitForPrimaryRoleAsync();

    try
    {
        // Get or create stores
        co_await PopulateStateProviders();

        // Populate stores
        co_await PopulateTpccWorkloadOnPrimaryAsync();

        // Start work
        co_await NewOrderTpccWorkloadOnPrimaryAsync();
    }
    catch (ktl::Exception& exception)
    {
        switch (exception.GetStatus())
        {
            case SF_STATUS_NOT_PRIMARY:
            {
                Trace.WriteInfo(TraceComponent, "SF_STATUS_NOT_PRIMARY exception");
                TraceException(TraceComponent, exception, GetAllocator());
                throw exception;
            }
            case SF_STATUS_OBJECT_CLOSED:
            {
                Trace.WriteInfo(TraceComponent, "SF_STATUS_OBJECT_CLOSED exception");
                TraceException(TraceComponent, exception, GetAllocator());
                throw exception;
            }
            case SF_STATUS_OBJECT_DISPOSED:
            {
                Trace.WriteInfo(TraceComponent, "SF_STATUS_OBJECT_DISPOSED exception");
                TraceException(TraceComponent, exception, GetAllocator());
                throw exception;
            }
            case SF_STATUS_INVALID_OPERATION:
            {
                Trace.WriteInfo(TraceComponent, "SF_STATUS_INVALID_OPERATION exception");
                TraceException(TraceComponent, exception, GetAllocator());
                throw exception;
            }
            case STATUS_CANCELLED:
            {
                Trace.WriteInfo(TraceComponent, "STATUS_CANCELLED exception");
                TraceException(TraceComponent, exception, GetAllocator());
                throw exception;
            }
            default:
            {
                Trace.WriteInfo(TraceComponent, "Unknown exception");
                TraceException(TraceComponent, exception, GetAllocator());
                throw;
            }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End DoWorkOnClientRequestAsync -----");

    co_return;
}

ktl::Awaitable<TestableServiceProgress> Service::GetProgressAsync()
{
    Trace.WriteInfo(TraceComponent, "----- Start GetProgressOnClientRequestAsync -----");

    co_await PopulateStateProviders();

    TestableServiceProgress progress;

#define GET_PROGRESS(store)\
    if (store)\
    {\
        progress.CustomProgress.push_back(store->Count);\
    }\
    else\
    {\
        progress.CustomProgress.push_back(0);\
    }

    // Get the number of records in each store.
    GET_PROGRESS(storeWareHouses_);
    GET_PROGRESS(storeDistricts_);
    GET_PROGRESS(storeCustomers_);
    GET_PROGRESS(storeNewOrders_);
    GET_PROGRESS(storeOrders_);
    GET_PROGRESS(storeOrderLine_);
    GET_PROGRESS(storeItems_);
    GET_PROGRESS(storeStock_);
    GET_PROGRESS(storeHistory_);

#undef GET_PROGRESS

    THROW_ON_FAILURE(TxReplicator->GetLastCommittedSequenceNumber(progress.LastCommittedSequenceNumber));

    FABRIC_EPOCH e;
    THROW_ON_FAILURE(TxReplicator->GetCurrentEpoch(e));

    progress.DataLossNumber = e.DataLossNumber;

    Trace.WriteInfo(TraceComponent, "----- End GetProgressOnClientRequestAsync -----");

    co_return progress;
}

ServiceMetrics Service::GetServiceMetrics()
{
    Trace.WriteInfo(TraceComponent, "----- Start GetServiceMetrics -----");

    ServiceMetrics metrics;

#if !defined(PLATFORM_UNIX)
    PROCESS_MEMORY_COUNTERS counters;

    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(PROCESS_MEMORY_COUNTERS)))
    {
        metrics.MemoryUsage = static_cast<LONG64>(counters.PagefileUsage);
        Trace.WriteInfo(TraceComponent, "Metrics: {0}", metrics);
    }
    else {
        Trace.WriteError(TraceComponent, "Failed to get process memory info");
    }
#endif

    Trace.WriteInfo(TraceComponent, "----- End GetServiceMetrics -----");

    return metrics;
}

ktl::Awaitable<void> Service::PopulateTpccWorkloadOnPrimaryAsync()
{
    Trace.WriteInfo(TraceComponent, "----- Start PopulateTpccWorkloadOnPrimaryAsync -----");

    KAllocator & allocator = GetAllocator();

    NTSTATUS status = STATUS_SUCCESS;
    bool succeeded = true;

    Random rnd(0);

    int countWarehouseCreated = 2;
    int countDistrictsCreatedPerWarehouse = 10;
    int countCustomersCreatedPerDistrict = 1000;
    int countItemsCreated = 2000;
    int countPopulate = 0;

    TimeSpan timeout = TimeSpan::MaxValue;

    // Check to see if the warehouses have been already populated.
    try {
        LONG64 countWarehouses = storeWareHouses_->Count;
        if (countWarehouses < 2) {
            countPopulate++;

            TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
            if (replicatorTransaction != nullptr)
            {
                KFinally([&] { replicatorTransaction->Dispose(); });

                // Create store transaction.
                Data::TStore::IStoreTransaction<int, WarehouseValue::SPtr>::SPtr rwtxWarehouse = nullptr;
                storeWareHouses_->CreateOrFindTransaction(*replicatorTransaction, rwtxWarehouse);

                // Add warehouses.
                for (int x = 0; x < countWarehouseCreated; ++x) {
                    WarehouseValue::SPtr warehouse = nullptr;
                    status = WarehouseValue::Create(allocator, warehouse);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create Warehouse.");
                    warehouse->Name = VariableText<10>::GetValue();
                    warehouse->Street_1 = VariableText<20>::GetValue();
                    warehouse->Street_2 = VariableText<20>::GetValue();
                    warehouse->City = VariableText<20>::GetValue();
                    warehouse->State = FixedText<2>::GetValue();
                    warehouse->Zip = FixedText<9>::GetValue();
                    warehouse->Tax = 9.5;
                    warehouse->Ytd = 0;

                    KeyValuePair<LONG64, WarehouseValue::SPtr> temp;
                    succeeded = co_await storeWareHouses_->ConditionalGetAsync(
                        *rwtxWarehouse,
                        x,
                        timeout,
                        temp,
                        ktl::CancellationToken::None);
                    if (!succeeded) {
                        co_await storeWareHouses_->AddAsync(
                            *rwtxWarehouse,
                            x,
                            warehouse,
                            timeout,
                            ktl::CancellationToken::None);
                    }
                }

                co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
            }
        }

        // Check to see if the districts have been already populated.
        LONG64 countDistricts = storeDistricts_->Count;
        if (countDistricts < 20) {
            countPopulate++;

            TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
            if (replicatorTransaction != nullptr)
            {
                KFinally([&] { replicatorTransaction->Dispose(); });

                // Create store transaction.
                Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rwtxDistricts = nullptr;
                storeDistricts_->CreateOrFindTransaction(*replicatorTransaction, rwtxDistricts);

                // Add districts (10 per warehouse).
                for (int x = 0; x < countWarehouseCreated; ++x) {
                    for (int y = 0; y < countDistrictsCreatedPerWarehouse; ++y) {
                        DistrictKey::SPtr districtKey = nullptr;
                        status = DistrictKey::Create(allocator, districtKey);
                        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create district key");
                        districtKey->Warehouse = x;
                        districtKey->District = y;

                        DistrictValue::SPtr districtValue = nullptr;
                        status = DistrictValue::Create(allocator, districtValue);
                        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create district value");
                        districtValue->Name = VariableText<10>::GetValue();
                        districtValue->Street_1 = VariableText<20>::GetValue();
                        districtValue->Street_2 = VariableText<20>::GetValue();
                        districtValue->City = VariableText<20>::GetValue();
                        districtValue->State = FixedText<2>::GetValue();
                        districtValue->Zip = FixedText<9>::GetValue();
                        districtValue->Tax = 9.5;
                        districtValue->Ytd = 0;
                        districtValue->NextOrderId = 0;

                        KeyValuePair<LONG64, DistrictValue::SPtr> temp;
                        succeeded = co_await storeDistricts_->ConditionalGetAsync(
                            *rwtxDistricts,
                            districtKey,
                            timeout,
                            temp,
                            ktl::CancellationToken::None);
                        if (!succeeded) {
                            co_await storeDistricts_->AddAsync(
                                *rwtxDistricts,
                                districtKey,
                                districtValue,
                                timeout,
                                ktl::CancellationToken::None);
                        }
                    }
                }

                co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
            }
        }

        LONG64 countCustomers = storeCustomers_->Count;
        if (countCustomers < 20000) {
            countPopulate++;

            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            // Create store transaction.
            Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rwtxDistricts = nullptr;
            storeDistricts_->CreateOrFindTransaction(*tx, rwtxDistricts);
            rwtxDistricts->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
            auto enumerateDistricts = co_await storeDistricts_->CreateKeyEnumeratorAsync(*rwtxDistricts);
            while (enumerateDistricts->MoveNext()) {
                DistrictKey::SPtr x = enumerateDistricts->Current();

                int customerId = 0;
                for (int y = 0; y < countCustomersCreatedPerDistrict / 100; ++y) {
                    TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
                    if (replicatorTransaction != nullptr)
                    {
                        KFinally([&] { replicatorTransaction->Dispose(); });

                        // Create store transaction.
                        Data::TStore::IStoreTransaction<CustomerKey::SPtr, CustomerValue::SPtr>::SPtr rwtxCustomers = nullptr;
                        storeCustomers_->CreateOrFindTransaction(*replicatorTransaction, rwtxCustomers);

                        for (int z = 0; z < countCustomersCreatedPerDistrict / 10; ++z) {
                            CustomerKey::SPtr customerKey = nullptr;
                            status = CustomerKey::Create(allocator, customerKey);
                            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create customer key");
                            customerKey->Warehouse = x->Warehouse;
                            customerKey->District = x->District;
                            customerKey->Customer = customerId++;

                            CustomerValue::SPtr customerValue = nullptr;
                            status = CustomerValue::Create(allocator, customerValue);
                            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create customer value");
                            customerValue->FirstName = VariableText<16>::GetValue();
                            customerValue->MiddleName = FixedText<2>::GetValue();
                            customerValue->LastName = VariableText<16>::GetValue();
                            customerValue->Street_1 = VariableText<20>::GetValue();
                            customerValue->Street_2 = VariableText<20>::GetValue();
                            customerValue->City = VariableText<20>::GetValue();
                            customerValue->State = FixedText<2>::GetValue();
                            customerValue->Zip = FixedText<9>::GetValue();
                            customerValue->Phone = FixedText<16>::GetValue();
                            //customerValue->Since = DateTime.UtcNow;
                            customerValue->Credit = FixedText<2>::GetValue();
                            customerValue->CreditLimit = rnd.Next(1000, 10000);
                            customerValue->Discount = 0;
                            customerValue->Balance = 1000; // DOUBLE_MAX;
                            customerValue->Ytd = 0;
                            customerValue->PaymentCount = 0;
                            customerValue->DeliveryCount = 0;
                            //customerValue->Data = new byte[rnd.Next(1, 501)];

                            KeyValuePair<LONG64, CustomerValue::SPtr> temp;
                            succeeded = co_await storeCustomers_->ConditionalGetAsync(
                                *rwtxCustomers,
                                customerKey,
                                timeout,
                                temp,
                                ktl::CancellationToken::None);
                            if (!succeeded) {
                                co_await storeCustomers_->AddAsync(
                                    *rwtxCustomers,
                                    customerKey,
                                    customerValue,
                                    timeout,
                                    ktl::CancellationToken::None);
                            }
                        }

                        co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
                    }
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        // Check to see if the districts have been already populated.
        LONG64 countItems = storeItems_->Count;
        if (countItems < 2000) {
            countPopulate++;

            long itemId = 0;
            for (int y = 0; y < countItemsCreated / 100; ++y) {
                TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
                if (replicatorTransaction != nullptr)
                {
                    KFinally([&] { replicatorTransaction->Dispose(); });

                    // Create store transaction.
                    Data::TStore::IStoreTransaction<LONG64, ItemValue::SPtr>::SPtr rwtxItems = nullptr;
                    storeItems_->CreateOrFindTransaction(*replicatorTransaction, rwtxItems);
                    Data::TStore::IStoreTransaction<StockKey::SPtr, StockValue::SPtr>::SPtr rwtxStock = nullptr;
                    storeStock_->CreateOrFindTransaction(*replicatorTransaction, rwtxStock);

                    for (int z = 0; z < countItemsCreated / 10; ++z) {
                        long currentItemId = itemId++;

                        ItemValue::SPtr itemValue = nullptr;
                        status = ItemValue::Create(allocator, itemValue);
                        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create item value");
                        itemValue->Image = rnd.Next();
                        itemValue->Name = VariableText<24>::GetValue();
                        itemValue->Price = rnd.Next(10, 101);
                        itemValue->Data = VariableText<50>::GetValue();

                        KeyValuePair<LONG64, ItemValue::SPtr> temp;
                        succeeded = co_await storeItems_->ConditionalGetAsync(
                            *rwtxItems,
                            currentItemId,
                            timeout,
                            temp,
                            ktl::CancellationToken::None);
                        if (!succeeded) {
                            co_await storeItems_->AddAsync(
                                *rwtxItems,
                                currentItemId,
                                itemValue,
                                timeout,
                                ktl::CancellationToken::None);
                        }

                        for (int x = 0; x < countWarehouseCreated; ++x) {
                            StockKey::SPtr stockKey = nullptr;
                            status = StockKey::Create(allocator, stockKey);
                            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create stock key");
                            stockKey->Warehouse = x;
                            stockKey->Stock = currentItemId;
                            StockValue::SPtr stockValue = nullptr;
                            status = StockValue::Create(allocator, stockValue);
                            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create stock value");
                            stockValue->Quantity = INT_MAX;
                            stockValue->District1 = FixedText<24>::GetValue();
                            stockValue->District2 = FixedText<24>::GetValue();
                            stockValue->District3 = FixedText<24>::GetValue();
                            stockValue->District4 = FixedText<24>::GetValue();
                            stockValue->District5 = FixedText<24>::GetValue();
                            stockValue->District6 = FixedText<24>::GetValue();
                            stockValue->District7 = FixedText<24>::GetValue();
                            stockValue->District8 = FixedText<24>::GetValue();
                            stockValue->District9 = FixedText<24>::GetValue();
                            stockValue->District10 = FixedText<24>::GetValue();
                            stockValue->Ytd = 0;
                            stockValue->OrderCount = 0;
                            stockValue->RemoteCount = 0;
                            stockValue->Data = VariableText<50>::GetValue();

                            KeyValuePair<LONG64, StockValue::SPtr> lTemp;
                            succeeded = co_await storeStock_->ConditionalGetAsync(
                                *rwtxStock,
                                stockKey,
                                timeout,
                                lTemp,
                                ktl::CancellationToken::None);
                            if (!succeeded) {
                                co_await storeStock_->AddAsync(
                                    *rwtxStock,
                                    stockKey,
                                    stockValue,
                                    timeout,
                                    ktl::CancellationToken::None);
                            }
                        }
                    }

                    co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
                }
            }
        }

        if (countPopulate > 0) {
            co_await SecondaryWorkloadTransaction1(false);
            co_await SecondaryWorkloadTransaction2(false);
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
            case SF_STATUS_TIMEOUT:
            {
                Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
                break;
            }
            default:
            {
                throw;
            }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End PopulateTpccWorkloadOnPrimaryAsync -----");
    co_return;
}


ktl::Awaitable<void> Service::SecondaryWorkloadTransaction1(bool shouldYield)
{
    Trace.WriteInfo(TraceComponent, "----- Start SecondaryWorkloadTransaction1 -----");

    NTSTATUS status = STATUS_SUCCESS;
    KAllocator & allocator = GetAllocator();
    int countWarehouses = 1;
    int countDistricts = 0;
    int countCustomers = 0;
    int countHistory = 0;
    int countItems = 0;
    int countNewOrders = 0;
    int countOrders = 0;
    int countOrderLine = 0;
    int countStock = 0;

    try
    {
        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<int, WarehouseValue::SPtr>::SPtr rtxWarehouse = nullptr;
            storeWareHouses_->CreateOrFindTransaction(*tx, rtxWarehouse);
            rtxWarehouse->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            int prevKey = INT_MIN;
            auto enumerateWarehouse = co_await storeWareHouses_->CreateKeyEnumeratorAsync(*rtxWarehouse, prevKey);
            while (enumerateWarehouse->MoveNext())
            {
                int currKey = enumerateWarehouse->Current();
                ASSERT_IFNOT(currKey >= prevKey, "Warehouse currkey < prevKey in enumeration");
                prevKey = currKey;
                countWarehouses++;
                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rtxDistrict = nullptr;
            storeDistricts_->CreateOrFindTransaction(*tx, rtxDistrict);
            rtxDistrict->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Key Comparer
            DistrictKeyComparer::SPtr keyComparer = nullptr;
            status = DistrictKeyComparer::Create(allocator, keyComparer);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create district key comparer");

            // start key
            DistrictKey::SPtr prevKey = nullptr;
            status = DistrictKey::Create(allocator, prevKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create district key");
            prevKey->Warehouse = INT_MIN;
            prevKey->District = INT_MIN;

            auto enumerateDistricts = co_await storeDistricts_->CreateKeyEnumeratorAsync(*rtxDistrict, prevKey);
            while (enumerateDistricts->MoveNext())
            {
                DistrictKey::SPtr currKey = enumerateDistricts->Current();
                ASSERT_IFNOT(keyComparer->Compare(currKey, prevKey) >= 0, "District currkey < prevKey in enumeration");
                prevKey = currKey;
                countDistricts++;
                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<CustomerKey::SPtr, CustomerValue::SPtr>::SPtr rtxCustomers = nullptr;
            storeCustomers_->CreateOrFindTransaction(*tx, rtxCustomers);
            rtxCustomers->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Key comparer
            CustomerKeyComparer::SPtr keyComparer = nullptr;
            status = CustomerKeyComparer::Create(allocator, keyComparer);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create customer key comparer");

            // start key
            CustomerKey::SPtr prevKey = nullptr;
            status = CustomerKey::Create(allocator, prevKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create customer key");
            prevKey->Warehouse = INT_MIN;
            prevKey->District = INT_MIN;
            prevKey->Customer = INT_MIN;

            auto enumerateCustomers = co_await storeCustomers_->CreateKeyEnumeratorAsync(*rtxCustomers, prevKey);
            while (enumerateCustomers->MoveNext())
            {
                CustomerKey::SPtr currKey = enumerateCustomers->Current();
                ASSERT_IFNOT(keyComparer->Compare(currKey, prevKey) >= 0, "Customer currKey < prevKey in enumeration");
                prevKey = currKey;
                countCustomers++;
                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<KGuid, HistoryValue::SPtr>::SPtr rtxHistory = nullptr;
            storeHistory_->CreateOrFindTransaction(*tx, rtxHistory);
            rtxHistory->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // key comparer
            GuidComparer::SPtr keyComparer = nullptr;
            status = GuidComparer::Create(allocator, keyComparer);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create guid comparer");

            // start key
            KGuid prevKey(Guid::Empty().AsGUID());

            auto enumerateHistory = co_await storeHistory_->CreateKeyEnumeratorAsync(*rtxHistory, prevKey);
            while (enumerateHistory->MoveNext())
            {
                KGuid currKey = enumerateHistory->Current();
                ASSERT_IFNOT(keyComparer->Compare(currKey, prevKey) >= 0, "History currKey < prevKey in enumeration");
                prevKey = currKey;
                countHistory++;
                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<LONG64, ItemValue::SPtr>::SPtr rtxItems = nullptr;
            storeItems_->CreateOrFindTransaction(*tx, rtxItems);
            rtxItems->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // start key
            LONG64 prevKey = LONG64_MIN;

            auto enumerateItems = co_await storeItems_->CreateKeyEnumeratorAsync(*rtxItems, prevKey);
            while (enumerateItems->MoveNext())
            {
                LONG64 currKey = enumerateItems->Current();
                ASSERT_IFNOT(currKey >= prevKey, "Store currKey < prevKey in enumeration");
                prevKey = currKey;
                countItems++;
                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<NewOrderKey::SPtr, NewOrderValue::SPtr>::SPtr rtxNewOrders = nullptr;
            storeNewOrders_->CreateOrFindTransaction(*tx, rtxNewOrders);
            rtxNewOrders->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // key comparer
            NewOrderKeyComparer::SPtr keyComparer = nullptr;
            status = NewOrderKeyComparer::Create(allocator, keyComparer);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create NewOrderKeyComparer");

            // start key
            NewOrderKey::SPtr prevKey = nullptr;
            status = NewOrderKey::Create(allocator, prevKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create NewOrderKey");
            prevKey->Warehouse = INT_MIN;
            prevKey->District = INT_MIN;
            prevKey->Order = LONG64_MIN;

            auto enumerateNewOrder = co_await storeNewOrders_->CreateKeyEnumeratorAsync(*rtxNewOrders, prevKey);
            while (enumerateNewOrder->MoveNext())
            {
                NewOrderKey::SPtr currKey = enumerateNewOrder->Current();
                ASSERT_IFNOT(keyComparer->Compare(currKey, prevKey) >= 0, "NewOrder currKey < prevKey in enumeration");
                prevKey = currKey;
                countNewOrders++;
                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<OrderLineKey::SPtr, OrderLineValue::SPtr>::SPtr rtxOrderLine = nullptr;
            storeOrderLine_->CreateOrFindTransaction(*tx, rtxOrderLine);
            rtxOrderLine->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // key comparer
            OrderLineKeyComparer::SPtr keyComparer = nullptr;
            status = OrderLineKeyComparer::Create(allocator, keyComparer);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderLineKeyComparer");

            // start key
            OrderLineKey::SPtr prevKey = nullptr;
            status = OrderLineKey::Create(allocator, prevKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderLineKey");
            prevKey->Warehouse = INT_MIN;
            prevKey->District = INT_MIN;
            prevKey->Order = LONG64_MIN;
            prevKey->Item = LONG64_MIN;
            prevKey->Number = INT_MIN;

            auto enumerateOrderLine = co_await storeOrderLine_->CreateKeyEnumeratorAsync(*rtxOrderLine, prevKey);
            while (enumerateOrderLine->MoveNext())
            {
                OrderLineKey::SPtr currKey = enumerateOrderLine->Current();
                ASSERT_IFNOT(keyComparer->Compare(currKey, prevKey) >= 0, "OrderLine currKey < prevKey in enumeration");
                prevKey = currKey;
                countOrderLine++;
                if (shouldYield) {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<OrderKey::SPtr, OrderValue::SPtr>::SPtr rtxOrders = nullptr;
            storeOrders_->CreateOrFindTransaction(*tx, rtxOrders);
            rtxOrders->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // key comparer
            OrderKeyComparer::SPtr keyComparer = nullptr;
            status = OrderKeyComparer::Create(allocator, keyComparer);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderKeyComparer");

            // start key
            OrderKey::SPtr prevKey = nullptr;
            status = OrderKey::Create(allocator, prevKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderKey");
            prevKey->Warehouse = INT_MIN;
            prevKey->District = INT_MIN;
            prevKey->Order = LONG64_MIN;

            auto enumerateOrder = co_await storeOrders_->CreateKeyEnumeratorAsync(*rtxOrders);
            while (enumerateOrder->MoveNext())
            {
                OrderKey::SPtr currKey = enumerateOrder->Current();
                ASSERT_IFNOT(keyComparer->Compare(currKey, prevKey) >= 0, "Order currKey < prevKey in enumeration");
                prevKey = currKey;
                countOrders++;
                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<StockKey::SPtr, StockValue::SPtr>::SPtr rtxStock = nullptr;
            storeStock_->CreateOrFindTransaction(*tx, rtxStock);
            rtxStock->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // key comparer
            StockKeyComparer::SPtr keyComparer = nullptr;
            status = StockKeyComparer::Create(allocator, keyComparer);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create StockKeyComparer");

            // start key
            StockKey::SPtr prevKey = nullptr;
            status = StockKey::Create(allocator, prevKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create StockKey");
            prevKey->Warehouse = INT_MIN;
            prevKey->Stock = LONG64_MIN;

            auto enumerateStock = co_await storeStock_->CreateKeyEnumeratorAsync(*rtxStock);
            while (enumerateStock->MoveNext())
            {
                StockKey::SPtr currKey = enumerateStock->Current();
                ASSERT_IFNOT(keyComparer->Compare(currKey, prevKey) >= 0, "Stock currKey < prevKey in enumeration");
                prevKey = currKey;
                countStock++;
                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
        case SF_STATUS_TIMEOUT:
        {
            Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
            break;
        }
        default:
        {
            throw;
        }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End SecondaryWorkloadTransaction1 -----");
    co_return;
}

ktl::Awaitable<void> Service::SecondaryWorkloadTransaction2(bool shouldYield)
{
    Trace.WriteInfo(TraceComponent, "----- Start SecondaryWorkloadTransaction2 -----");

    KAllocator & allocator = GetAllocator();
    NTSTATUS status = STATUS_SUCCESS;
    int countWarehouses = 0;
    int countDistricts = 0;
    int countCustomers = 0;
    int countHistory = 0;
    int countItems = 0;
    int countNewOrders = 0;
    int countOrders = 0;
    int countOrderLine = 0;
    int countStock = 0;

    try
    {
        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<int, WarehouseValue::SPtr>::SPtr rtxWarehouse = nullptr;
            storeWareHouses_->CreateOrFindTransaction(*tx, rtxWarehouse);
            rtxWarehouse->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateWarehouse = co_await storeWareHouses_->CreateEnumeratorAsync(*rtxWarehouse);
            while (co_await enumerateWarehouse->MoveNextAsync(ktl::CancellationToken::None))
            {
                countWarehouses++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rtxDistrict = nullptr;
            storeDistricts_->CreateOrFindTransaction(*tx, rtxDistrict);
            rtxDistrict->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateDistricts = co_await storeDistricts_->CreateEnumeratorAsync(*rtxDistrict);
            while (co_await enumerateDistricts->MoveNextAsync(ktl::CancellationToken::None))
            {
                countDistricts++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<CustomerKey::SPtr, CustomerValue::SPtr>::SPtr rtxCustomers = nullptr;
            storeCustomers_->CreateOrFindTransaction(*tx, rtxCustomers);
            rtxCustomers->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateCustomers = co_await storeCustomers_->CreateEnumeratorAsync(*rtxCustomers);
            while (co_await enumerateCustomers->MoveNextAsync(ktl::CancellationToken::None))
            {
                countCustomers++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<KGuid, HistoryValue::SPtr>::SPtr rtxHistory = nullptr;
            storeHistory_->CreateOrFindTransaction(*tx, rtxHistory);
            rtxHistory->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateHistory = co_await storeHistory_->CreateEnumeratorAsync(*rtxHistory);
            while (co_await enumerateHistory->MoveNextAsync(ktl::CancellationToken::None))
            {
                countHistory++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<LONG64, ItemValue::SPtr>::SPtr rtxItems = nullptr;
            storeItems_->CreateOrFindTransaction(*tx, rtxItems);
            rtxItems->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateItems = co_await storeItems_->CreateEnumeratorAsync(*rtxItems);
            while (co_await enumerateItems->MoveNextAsync(ktl::CancellationToken::None))
            {
                countItems++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<NewOrderKey::SPtr, NewOrderValue::SPtr>::SPtr rtxNewOrders = nullptr;
            storeNewOrders_->CreateOrFindTransaction(*tx, rtxNewOrders);
            rtxNewOrders->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateNewOrder = co_await storeNewOrders_->CreateEnumeratorAsync(*rtxNewOrders);
            while (co_await enumerateNewOrder->MoveNextAsync(ktl::CancellationToken::None))
            {
                countNewOrders++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<OrderLineKey::SPtr, OrderLineValue::SPtr>::SPtr rtxOrderLine = nullptr;
            storeOrderLine_->CreateOrFindTransaction(*tx, rtxOrderLine);
            rtxOrderLine->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateOrderLine = co_await storeOrderLine_->CreateEnumeratorAsync(*rtxOrderLine);
            while (co_await enumerateOrderLine->MoveNextAsync(ktl::CancellationToken::None))
            {
                countOrderLine++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<OrderKey::SPtr, OrderValue::SPtr>::SPtr rtxOrders = nullptr;
            storeOrders_->CreateOrFindTransaction(*tx, rtxOrders);
            rtxOrders->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateOrder = co_await storeOrders_->CreateEnumeratorAsync(*rtxOrders);
            while (co_await enumerateOrder->MoveNextAsync(ktl::CancellationToken::None))
            {
                countOrders++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }

        {
            TxnReplicator::Transaction::SPtr tx;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
            KFinally([&] { tx->Dispose(); });

            Data::TStore::IStoreTransaction<StockKey::SPtr, StockValue::SPtr>::SPtr rtxStock = nullptr;
            storeStock_->CreateOrFindTransaction(*tx, rtxStock);
            rtxStock->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerateStock = co_await storeStock_->CreateEnumeratorAsync(*rtxStock);
            while (co_await enumerateStock->MoveNextAsync(ktl::CancellationToken::None))
            {
                countStock++;

                if (shouldYield)
                {
                    status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
                }
            }

            THROW_ON_FAILURE(co_await tx->CommitAsync());
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
        case SF_STATUS_TIMEOUT:
        {
            Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
            break;
        }
        default:
        {
            throw;
        }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End SecondaryWorkloadTransaction2 -----");
    co_return;
}

ktl::Awaitable<void> Service::NewOrderTransactionLockBased(int index)
{
    Trace.WriteInfo(TraceComponent, "----- Start NewOrderTransactionLockBased -----");

    KAllocator & allocator = GetAllocator();

    Random rnd(GetTickCount() + index);
    int countWarehouseCreated = 2;
    int countDistrictsCreatedPerWarehouse = 10;
    int countCustomersCreatedPerDistrict = 1000;
    int countItems = 1000;
    TimeSpan timeout = TimeSpan::FromSeconds(5);

    // Generate warehouse.
    int warehouse = rnd.Next(0, countWarehouseCreated);

    // Generate district.
    int district = rnd.Next(0, countDistrictsCreatedPerWarehouse);

    // Generate customer.
    int customer = rnd.Next(0, countCustomersCreatedPerDistrict);

    // Generate order line count.
    int orderLineCount = rnd.Next(5, 16);

    // Generate outcome.
    bool commit = (1 != rnd.Next(1, 101)) ? true : false;

    try {
        TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
        if (replicatorTransaction != nullptr)
        {
            KFinally([&] {replicatorTransaction->Dispose(); });

            // Retrieve warehouse.
            Data::TStore::IStoreTransaction<int, WarehouseValue::SPtr>::SPtr rwtxWarehouse = nullptr;
            storeWareHouses_->CreateOrFindTransaction(*replicatorTransaction, rwtxWarehouse);
            KeyValuePair<LONG64, WarehouseValue::SPtr> resultWarehouse;
            bool succeeded = co_await storeWareHouses_->ConditionalGetAsync(
                *rwtxWarehouse,
                warehouse,
                timeout,
                resultWarehouse,
                ktl::CancellationToken::None);
            ASSERT_IFNOT(succeeded, "Failed to get warehouse");

            // Retrieve district.
            Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rwtxDistrict = nullptr;
            storeDistricts_->CreateOrFindTransaction(*replicatorTransaction, rwtxDistrict);
            DistrictKey::SPtr districtKey = nullptr;
            NTSTATUS status = DistrictKey::Create(allocator, districtKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create DistrictKey");
            districtKey->Warehouse = warehouse;
            districtKey->District = district;
            rwtxDistrict->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
            KeyValuePair<LONG64, DistrictValue::SPtr> resultDistrict;
            succeeded = co_await storeDistricts_->ConditionalGetAsync(
                *rwtxDistrict,
                districtKey,
                timeout,
                resultDistrict,
                ktl::CancellationToken::None);
            ASSERT_IFNOT(succeeded, "Failed to get district");

            // Update the next order id in the district.
            DistrictValue::SPtr updatedDisctrictValue = nullptr;
            status = DistrictValue::Create(allocator, updatedDisctrictValue);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create DistrictValue");
            updatedDisctrictValue->Name = resultDistrict.Value->Name;
            updatedDisctrictValue->Street_1 = resultDistrict.Value->Street_1;
            updatedDisctrictValue->Street_2 = resultDistrict.Value->Street_2;
            updatedDisctrictValue->City = resultDistrict.Value->City;
            updatedDisctrictValue->State = resultDistrict.Value->State;
            updatedDisctrictValue->Zip = resultDistrict.Value->Zip;
            updatedDisctrictValue->Tax = resultDistrict.Value->Tax;
            updatedDisctrictValue->Ytd = resultDistrict.Value->Ytd;
            updatedDisctrictValue->NextOrderId = resultDistrict.Value->NextOrderId + 1;
            co_await storeDistricts_->ConditionalUpdateAsync(
                *rwtxDistrict,
                districtKey,
                updatedDisctrictValue,
                timeout,
                ktl::CancellationToken::None);

            // Select customer.
            double customerDiscount = 0;
            {
                TxnReplicator::Transaction::SPtr tx;
                THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
                KFinally([&] {tx->Dispose(); });

                Data::TStore::IStoreTransaction<CustomerKey::SPtr, CustomerValue::SPtr>::SPtr customerTxn = nullptr;
                storeCustomers_->CreateOrFindTransaction(*tx, customerTxn);
                CustomerKey::SPtr customerKey = nullptr;
                status = CustomerKey::Create(allocator, customerKey);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create CustomerKey");
                customerKey->Customer = customer;
                customerKey->District = district;
                customerKey->Warehouse = warehouse;
                KeyValuePair<LONG64, CustomerValue::SPtr> resultCustomer;
                succeeded = co_await storeCustomers_->ConditionalGetAsync(
                    *customerTxn,
                    customerKey,
                    timeout,
                    resultCustomer,
                    ktl::CancellationToken::None);
                ASSERT_IFNOT(succeeded, "Failed to get customer");
                customerDiscount = resultCustomer.Value->Discount;

                THROW_ON_FAILURE(co_await tx->CommitAsync());
            }

            // Create stock transaction.
            Data::TStore::IStoreTransaction<StockKey::SPtr, StockValue::SPtr>::SPtr stockTxn = nullptr;
            storeStock_->CreateOrFindTransaction(*replicatorTransaction, stockTxn);
            stockTxn->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;

            // Generate order lines.
            bool allLocal = true;
            vector<OrderLineKey::SPtr> orderLineKeys;
            vector<OrderLineValue::SPtr> orderLineValues;
            for (int x = 0; x < orderLineCount; ++x) {
                // Generate item.
                long item = rnd.Next(0, countItems);

                // Retrieve item.
                KeyValuePair<LONG64, ItemValue::SPtr> resultItem;
                {
                    TxnReplicator::Transaction::SPtr tx;
                    THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
                    KFinally([&] {tx->Dispose(); });

                    Data::TStore::IStoreTransaction<LONG64, ItemValue::SPtr>::SPtr itemTxn = nullptr;
                    storeItems_->CreateOrFindTransaction(*tx, itemTxn);
                    succeeded = co_await storeItems_->ConditionalGetAsync(
                        *itemTxn,
                        item,
                        timeout,
                        resultItem,
                        ktl::CancellationToken::None);
                    ASSERT_IFNOT(succeeded, "Failed to get item");

                    THROW_ON_FAILURE(co_await tx->CommitAsync());
                }

                // Generate supplying warehouse.
                int supplyWarehouse = (1 == rnd.Next(1, 101)) ? (int)((0 == warehouse) ? 1 : 0) : warehouse;
                if (supplyWarehouse != warehouse)
                {
                    allLocal = false;
                }

                OrderLineKey::SPtr orderLineKey = nullptr;
                status = OrderLineKey::Create(allocator, orderLineKey);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderLineKey");
                orderLineKey->District = district;
                orderLineKey->Item = item;
                orderLineKey->Number = x;
                orderLineKey->Order = resultDistrict.Value->NextOrderId;
                orderLineKey->SupplyWarehouse = supplyWarehouse;
                orderLineKey->Warehouse = warehouse;

                OrderLineValue::SPtr orderLineValue = nullptr;
                status = OrderLineValue::Create(allocator, orderLineValue);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderLineValue");
                orderLineValue->Quantity = rnd.Next(1, 9);
                orderLineValue->Amount = orderLineValue->Quantity * resultItem.Value->Price * (1 + resultWarehouse.Value->Tax + resultDistrict.Value->Tax) * (1 - customerDiscount);
                //orderLineValue->Delivery = DateTime.UtcNow;
                orderLineValue->Info = VariableText<24>::GetValue();

                // Retrieve stock.
                StockKey::SPtr stockKey = nullptr;
                status = StockKey::Create(allocator, stockKey);
                stockKey->Warehouse = warehouse;
                stockKey->Stock = item;
                KeyValuePair<LONG64, StockValue::SPtr> resultStock;
                succeeded = co_await storeStock_->ConditionalGetAsync(
                    *stockTxn,
                    stockKey,
                    timeout,
                    resultStock,
                    ktl::CancellationToken::None);
                ASSERT_IFNOT(succeeded, "Failed to get stock");

                // Update stock.
                StockValue::SPtr updatedStockValue = nullptr;
                status = StockValue::Create(allocator, updatedStockValue);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create stock");
                updatedStockValue->Quantity = resultStock.Value->Quantity - orderLineValue->Quantity;
                updatedStockValue->District1 = resultStock.Value->District1;
                updatedStockValue->District2 = resultStock.Value->District2;
                updatedStockValue->District3 = resultStock.Value->District3;
                updatedStockValue->District4 = resultStock.Value->District4;
                updatedStockValue->District5 = resultStock.Value->District5;
                updatedStockValue->District6 = resultStock.Value->District6;
                updatedStockValue->District7 = resultStock.Value->District7;
                updatedStockValue->District8 = resultStock.Value->District8;
                updatedStockValue->District9 = resultStock.Value->District9;
                updatedStockValue->District10 = resultStock.Value->District10;
                updatedStockValue->Ytd = resultStock.Value->Ytd + orderLineValue->Amount;
                updatedStockValue->OrderCount = resultStock.Value->OrderCount;
                updatedStockValue->RemoteCount = resultStock.Value->RemoteCount;
                updatedStockValue->Data = resultStock.Value->Data;
                if (supplyWarehouse == warehouse)
                {
                    updatedStockValue->OrderCount++;
                }
                else
                {
                    updatedStockValue->RemoteCount++;
                }

                co_await storeStock_->ConditionalUpdateAsync(
                    *stockTxn,
                    stockKey,
                    updatedStockValue,
                    timeout,
                    ktl::CancellationToken::None);

                orderLineKeys.push_back(orderLineKey);
                orderLineValues.push_back(orderLineValue);
            }

            // Generate new order.
            Data::TStore::IStoreTransaction<NewOrderKey::SPtr, NewOrderValue::SPtr>::SPtr newOrderTxn = nullptr;
            storeNewOrders_->CreateOrFindTransaction(*replicatorTransaction, newOrderTxn);
            NewOrderKey::SPtr newOrderKey = nullptr;
            status = NewOrderKey::Create(allocator, newOrderKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create NewOrderKey");
            newOrderKey->Warehouse = warehouse;
            newOrderKey->District = district;
            newOrderKey->Order = resultDistrict.Value->NextOrderId;

            NewOrderValue::SPtr newOrderValue = nullptr;
            status = NewOrderValue::Create(allocator, newOrderValue);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create NewOrderValue");
            co_await storeNewOrders_->AddAsync(
                *newOrderTxn,
                newOrderKey,
                newOrderValue,
                timeout,
                ktl::CancellationToken::None);

            // Generate order.
            Data::TStore::IStoreTransaction<OrderKey::SPtr, OrderValue::SPtr>::SPtr ordersTxn = nullptr;
            storeOrders_->CreateOrFindTransaction(*replicatorTransaction, ordersTxn);
            OrderKey::SPtr orderKey = nullptr;
            status = OrderKey::Create(allocator, orderKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderKey");
            orderKey->Customer = customer;
            orderKey->District = district;
            orderKey->Order = resultDistrict.Value->NextOrderId;
            orderKey->Warehouse = warehouse;
            OrderValue::SPtr orderValue = nullptr;
            status = OrderValue::Create(allocator, orderValue);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderValue");
            orderValue->Carrier = 0;
            //orderValue->Entry = DateTime.UtcNow;
            orderValue->LineCount = orderLineCount;
            orderValue->Local = allLocal;

            co_await storeOrders_->AddAsync(
                *ordersTxn,
                orderKey,
                orderValue,
                timeout,
                ktl::CancellationToken::None);

            // Add new order lines.
            Data::TStore::IStoreTransaction<OrderLineKey::SPtr, OrderLineValue::SPtr>::SPtr rwtxOrderLine = nullptr;
            storeOrderLine_->CreateOrFindTransaction(*replicatorTransaction, rwtxOrderLine);
            for (int orderLine = 0; orderLine < orderLineKeys.size(); ++orderLine) {
                co_await storeOrderLine_->AddAsync(
                    *rwtxOrderLine,
                    orderLineKeys[orderLine],
                    orderLineValues[orderLine],
                    timeout,
                    ktl::CancellationToken::None);
            }

            if (commit) {
                co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
            }
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
            case SF_STATUS_TIMEOUT:
            {
                Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
                break;
            }
            default:
            {
                throw;
            }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End NewOrderTransactionLockBased -----");

    co_return;
}

ktl::Awaitable<void> Service::NewOrderTransactionVersionBased(int index)
{
    Trace.WriteInfo(TraceComponent, "----- Start NewOrderTransactionVersionBased -----");

    KAllocator & allocator = GetAllocator();

    Random rnd(GetTickCount() + index);
    int countWarehouseCreated = 2;
    int countDistrictsCreatedPerWarehouse = 10;
    int countCustomersCreatedPerDistrict = 1000;
    int countItems = 1000;
    TimeSpan timeout = TimeSpan::FromSeconds(5);

    // Generate warehouse.
    int warehouse = rnd.Next(0, countWarehouseCreated);

    // Generate district.
    int district = rnd.Next(0, countDistrictsCreatedPerWarehouse);

    // Generate order line count.
    int orderLineCount = rnd.Next(5, 16);

    // Generate customer.
    int customer = rnd.Next(0, countCustomersCreatedPerDistrict);

    // Generate outcome.
    bool commit = (1 != rnd.Next(1, 101)) ? true : false;

    try {
        bool done = false;
        while (!done) {
            TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
            if (replicatorTransaction != nullptr)
            {
                KFinally([&] {replicatorTransaction->Dispose(); });

                // Retrieve warehouse.
                Data::TStore::IStoreTransaction<int, WarehouseValue::SPtr>::SPtr rwtxWarehouse = nullptr;
                storeWareHouses_->CreateOrFindTransaction(*replicatorTransaction, rwtxWarehouse);
                KeyValuePair<LONG64, WarehouseValue::SPtr> resultWarehouse;
                bool succeeded = co_await storeWareHouses_->ConditionalGetAsync(
                    *rwtxWarehouse,
                    warehouse,
                    timeout,
                    resultWarehouse,
                    ktl::CancellationToken::None);
                ASSERT_IFNOT(succeeded, "Failed to get warehouse");

                // Retrieve district.
                Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rwtxDistrict = nullptr;
                storeDistricts_->CreateOrFindTransaction(*replicatorTransaction, rwtxDistrict);
                DistrictKey::SPtr districtKey = nullptr;
                NTSTATUS status = DistrictKey::Create(allocator, districtKey);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create DistrictKey");
                districtKey->Warehouse = warehouse;
                districtKey->District = district;
                KeyValuePair<LONG64, DistrictValue::SPtr> resultDistrict;
                succeeded = co_await storeDistricts_->ConditionalGetAsync(
                    *rwtxDistrict,
                    districtKey,
                    timeout,
                    resultDistrict,
                    ktl::CancellationToken::None);
                ASSERT_IFNOT(succeeded, "Failed to get district");

                // Attempt to update the next order id in the district.
                DistrictValue::SPtr updatedDisctrictValue = nullptr;
                status = DistrictValue::Create(allocator, updatedDisctrictValue);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create DistrictValue");
                updatedDisctrictValue->Name = resultDistrict.Value->Name;
                updatedDisctrictValue->Street_1 = resultDistrict.Value->Street_1;
                updatedDisctrictValue->Street_2 = resultDistrict.Value->Street_2;
                updatedDisctrictValue->City = resultDistrict.Value->City;
                updatedDisctrictValue->State = resultDistrict.Value->State;
                updatedDisctrictValue->Zip = resultDistrict.Value->Zip;
                updatedDisctrictValue->Tax = resultDistrict.Value->Tax;
                updatedDisctrictValue->Ytd = resultDistrict.Value->Ytd;
                updatedDisctrictValue->NextOrderId = resultDistrict.Value->NextOrderId + 1;
                bool updateDistrictOutcome = co_await storeDistricts_->ConditionalUpdateAsync(
                    *rwtxDistrict,
                    districtKey,
                    updatedDisctrictValue,
                    timeout,
                    ktl::CancellationToken::None);
                if (!updateDistrictOutcome) {
                    // goto Abort;
                    // TODO: trace
                    continue;
                }

                // Select customer.
                double customerDiscount = 0;
                {
                    TxnReplicator::Transaction::SPtr tx;
                    THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
                    KFinally([&] {tx->Dispose(); });

                    Data::TStore::IStoreTransaction<CustomerKey::SPtr, CustomerValue::SPtr>::SPtr customerTxn = nullptr;
                    storeCustomers_->CreateOrFindTransaction(*tx, customerTxn);
                    CustomerKey::SPtr customerKey = nullptr;
                    status = CustomerKey::Create(allocator, customerKey);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create CustomerKey");
                    customerKey->Customer = customer;
                    customerKey->District = district;
                    customerKey->Warehouse = warehouse;
                    KeyValuePair<LONG64, CustomerValue::SPtr> resultCustomer;
                    succeeded = co_await storeCustomers_->ConditionalGetAsync(
                        *customerTxn,
                        customerKey,
                        timeout,
                        resultCustomer,
                        ktl::CancellationToken::None);
                    ASSERT_IFNOT(succeeded, "Failed to get customer");

                    customerDiscount = resultCustomer.Value->Discount;

                    THROW_ON_FAILURE(co_await tx->CommitAsync());
                }

                // Create stock transaction.
                Data::TStore::IStoreTransaction<StockKey::SPtr, StockValue::SPtr>::SPtr stockTxn = nullptr;
                storeStock_->CreateOrFindTransaction(*replicatorTransaction, stockTxn);

                // Generate order lines.
                bool allLocal = true;
                std::vector<OrderLineKey::SPtr> orderLineKeys;
                std::vector<OrderLineValue::SPtr> orderLineValues;
                for (int x = 0; x < orderLineCount; ++x) {
                    // Generate item.
                    long item = rnd.Next(0, countItems);

                    // Retrieve item.
                    KeyValuePair<LONG64, ItemValue::SPtr> resultItem;
                    {
                        TxnReplicator::Transaction::SPtr tx;
                        THROW_ON_FAILURE(TxReplicator->CreateTransaction(tx));
                        KFinally([&] {tx->Dispose(); });

                        Data::TStore::IStoreTransaction<LONG64, ItemValue::SPtr>::SPtr itemTxn = nullptr;
                        storeItems_->CreateOrFindTransaction(*tx, itemTxn);
                        succeeded = co_await storeItems_->ConditionalGetAsync(
                            *itemTxn,
                            item,
                            timeout,
                            resultItem,
                            ktl::CancellationToken::None);
                        ASSERT_IFNOT(succeeded, "Failed to get iterm");

                        THROW_ON_FAILURE(co_await tx->CommitAsync());
                    }

                    // Generate supplying warehouse.
                    int supplyWarehouse = (1 == rnd.Next(1, 101)) ? (int)((0 == warehouse) ? 1 : 0) : warehouse;
                    if (supplyWarehouse != warehouse)
                    {
                        allLocal = false;
                    }

                    OrderLineKey::SPtr orderLineKey = nullptr;
                    status = OrderLineKey::Create(allocator, orderLineKey);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderLineKey");
                    orderLineKey->District = district;
                    orderLineKey->Item = item;
                    orderLineKey->Number = x;
                    orderLineKey->Order = resultDistrict.Value->NextOrderId;
                    orderLineKey->SupplyWarehouse = supplyWarehouse;
                    orderLineKey->Warehouse = warehouse;

                    OrderLineValue::SPtr orderLineValue = nullptr;
                    status = OrderLineValue::Create(allocator, orderLineValue);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderLineValue");
                    orderLineValue->Quantity = rnd.Next(1, 9);
                    orderLineValue->Amount = orderLineValue->Quantity * resultItem.Value->Price * (1 + resultWarehouse.Value->Tax + resultDistrict.Value->Tax) * (1 - customerDiscount);
                    //orderLineValue.Delivery = DateTime.UtcNow;
                    orderLineValue->Info = VariableText<24>::GetValue();

                    // Retrieve stock.
                    StockKey::SPtr stockKey = nullptr;
                    status = StockKey::Create(allocator, stockKey);
                    stockKey->Warehouse = warehouse;
                    stockKey->Stock = item;
                    KeyValuePair<LONG64, StockValue::SPtr> resultStock;
                    succeeded = co_await storeStock_->ConditionalGetAsync(
                        *stockTxn,
                        stockKey,
                        timeout,
                        resultStock,
                        ktl::CancellationToken::None);
                    ASSERT_IFNOT(succeeded, "Failed to get stock");

                    // Update stock.
                    StockValue::SPtr updatedStockValue = nullptr;
                    status = StockValue::Create(allocator, updatedStockValue);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create StockValue");
                    updatedStockValue->Quantity = resultStock.Value->Quantity - orderLineValue->Quantity;
                    updatedStockValue->District1 = resultStock.Value->District1;
                    updatedStockValue->District2 = resultStock.Value->District2;
                    updatedStockValue->District3 = resultStock.Value->District3;
                    updatedStockValue->District4 = resultStock.Value->District4;
                    updatedStockValue->District5 = resultStock.Value->District5;
                    updatedStockValue->District6 = resultStock.Value->District6;
                    updatedStockValue->District7 = resultStock.Value->District7;
                    updatedStockValue->District8 = resultStock.Value->District8;
                    updatedStockValue->District9 = resultStock.Value->District9;
                    updatedStockValue->District10 = resultStock.Value->District10;
                    updatedStockValue->Ytd = resultStock.Value->Ytd + orderLineValue->Amount;
                    updatedStockValue->OrderCount = resultStock.Value->OrderCount;
                    updatedStockValue->RemoteCount = resultStock.Value->RemoteCount;
                    updatedStockValue->Data = resultStock.Value->Data;
                    if (supplyWarehouse == warehouse)
                    {
                        updatedStockValue->OrderCount++;
                    }
                    else
                    {
                        updatedStockValue->RemoteCount++;
                    }

                    bool updateStockOutcome = co_await storeStock_->ConditionalUpdateAsync(
                        *stockTxn,
                        stockKey,
                        updatedStockValue,
                        timeout,
                        ktl::CancellationToken::None);
                    if (!updateStockOutcome) {
                        // goto Abort;
                        // TODO: trace
                        continue;
                    }

                    orderLineKeys.push_back(orderLineKey);
                    orderLineValues.push_back(orderLineValue);
                }

                // Generate new order.
                Data::TStore::IStoreTransaction<OrderKey::SPtr, OrderValue::SPtr>::SPtr ordersTxn = nullptr;
                storeOrders_->CreateOrFindTransaction(*replicatorTransaction, ordersTxn);

                OrderKey::SPtr orderKey = nullptr;
                status = OrderKey::Create(allocator, orderKey);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderKey");
                orderKey->Customer = customer;
                orderKey->District = district;
                orderKey->Order = resultDistrict.Value->NextOrderId;
                orderKey->Warehouse = warehouse;
                OrderValue::SPtr orderValue = nullptr;
                status = OrderValue::Create(allocator, orderValue);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderValue");
                orderValue->Carrier = 0;
                //orderValue->Entry = DateTime.UtcNow;
                orderValue->LineCount = orderLineCount;
                orderValue->Local = allLocal;

                co_await storeOrders_->AddAsync(
                    *ordersTxn,
                    orderKey,
                    orderValue,
                    timeout,
                    ktl::CancellationToken::None);

                // Add new order lines.
                Data::TStore::IStoreTransaction<OrderLineKey::SPtr, OrderLineValue::SPtr>::SPtr rwtxOrderLine = nullptr;
                storeOrderLine_->CreateOrFindTransaction(*replicatorTransaction, rwtxOrderLine);
                for (int orderLine = 0; orderLine < orderLineKeys.size(); ++orderLine) {
                    co_await storeOrderLine_->AddAsync(
                        *rwtxOrderLine,
                        orderLineKeys[orderLine],
                        orderLineValues[orderLine],
                        timeout,
                        ktl::CancellationToken::None);
                }

                // Complete replicator transaction.
                if (commit)
                {
                    co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
                }

                done = true;
            }
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
            case SF_STATUS_TIMEOUT:
            {
                Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
                break;
            }
            default:
            {
                throw;
            }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End NewOrderTransactionVersionBased -----");
    co_return;
}

ktl::Awaitable<void> Service::NewPaymentTransactionLockBased(int index)
{
    Trace.WriteInfo(TraceComponent, "----- Start NewPaymentTransactionLockBased -----");

    KAllocator & allocator = GetAllocator();

    Random rnd(GetTickCount() + index);
    int countWarehouseCreated = 2;
    int countDistrictsCreatedPerWarehouse = 10;
    int countCustomersCreatedPerDistrict = 1000;
    TimeSpan timeout = TimeSpan::FromSeconds(5);

    // Generate warehouse.
    int warehouse = rnd.Next(0, countWarehouseCreated);

    // Generate district.
    int district = rnd.Next(0, countDistrictsCreatedPerWarehouse);

    // Generate customer.
    int customer = rnd.Next(0, countCustomersCreatedPerDistrict);

    // Generate payment.
    double payment = rnd.Next(1, 50001);
    //DateTime paymentDate = DateTime.UtcNow;

    try {
        KGuid historyKey(Guid::NewGuid().AsGUID());
        HistoryValue::SPtr historyValue = nullptr;
        NTSTATUS status = HistoryValue::Create(allocator, historyValue);
        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create HistoryValue");

        // Start new order transaction.
        TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
        if (replicatorTransaction != nullptr)
        {
            KFinally([&] {replicatorTransaction->Dispose(); });

            // Retrieve warehouse.
            Data::TStore::IStoreTransaction<int, WarehouseValue::SPtr>::SPtr rwtxWarehouse = nullptr;
            storeWareHouses_->CreateOrFindTransaction(*replicatorTransaction, rwtxWarehouse);
            rwtxWarehouse->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
            KeyValuePair<LONG64, WarehouseValue::SPtr> resultWarehouse;
            bool succeeded = co_await storeWareHouses_->ConditionalGetAsync(
                *rwtxWarehouse,
                warehouse,
                timeout,
                resultWarehouse,
                ktl::CancellationToken::None);
            ASSERT_IFNOT(succeeded, "Failed to get warehouses");

            // Update warehouse Ytd.
            WarehouseValue::SPtr updatedWarehouseValue = nullptr;
            status = WarehouseValue::Create(allocator, updatedWarehouseValue);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create WarehouseValue");
            updatedWarehouseValue->Name = resultWarehouse.Value->Name;
            updatedWarehouseValue->Street_1 = resultWarehouse.Value->Street_1;
            updatedWarehouseValue->Street_2 = resultWarehouse.Value->Street_2;
            updatedWarehouseValue->City = resultWarehouse.Value->City;
            updatedWarehouseValue->State = resultWarehouse.Value->State;
            updatedWarehouseValue->Zip = resultWarehouse.Value->Zip;
            updatedWarehouseValue->Tax = resultWarehouse.Value->Tax;
            updatedWarehouseValue->Ytd = resultWarehouse.Value->Ytd + payment;
            co_await storeWareHouses_->ConditionalUpdateAsync(
                *rwtxWarehouse,
                warehouse,
                updatedWarehouseValue,
                timeout,
                ktl::CancellationToken::None);

            // Retrieve district.
            Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rwtxDistrict = nullptr;
            storeDistricts_->CreateOrFindTransaction(*replicatorTransaction, rwtxDistrict);
            DistrictKey::SPtr districtKey = nullptr;
            status = DistrictKey::Create(allocator, districtKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create DistrictKey");
            districtKey->Warehouse = warehouse;
            districtKey->District = district;
            rwtxDistrict->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
            KeyValuePair<LONG64, DistrictValue::SPtr> resultDistrict;
            succeeded = co_await storeDistricts_->ConditionalGetAsync(
                *rwtxDistrict,
                districtKey,
                timeout,
                resultDistrict,
                ktl::CancellationToken::None);
            ASSERT_IFNOT(succeeded, "Failed to get district");

            // Update district Ytd.
            DistrictValue::SPtr updatedDisctrictValue = nullptr;
            status = DistrictValue::Create(allocator, updatedDisctrictValue);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create DistrictValue");
            updatedDisctrictValue->Name = resultDistrict.Value->Name;
            updatedDisctrictValue->Street_1 = resultDistrict.Value->Street_1;
            updatedDisctrictValue->Street_2 = resultDistrict.Value->Street_2;
            updatedDisctrictValue->City = resultDistrict.Value->City;
            updatedDisctrictValue->State = resultDistrict.Value->State;
            updatedDisctrictValue->Zip = resultDistrict.Value->Zip;
            updatedDisctrictValue->Tax = resultDistrict.Value->Tax;
            updatedDisctrictValue->Ytd = resultDistrict.Value->Ytd + payment;
            updatedDisctrictValue->NextOrderId = resultDistrict.Value->NextOrderId;
            co_await storeDistricts_->ConditionalUpdateAsync(
                *rwtxDistrict,
                districtKey,
                updatedDisctrictValue,
                timeout,
                ktl::CancellationToken::None);

            // Retrieve customer.
            Data::TStore::IStoreTransaction<CustomerKey::SPtr, CustomerValue::SPtr>::SPtr rwtxCustomer = nullptr;
            storeCustomers_->CreateOrFindTransaction(*replicatorTransaction, rwtxCustomer);
            CustomerKey::SPtr customerKey = nullptr;
            status = CustomerKey::Create(allocator, customerKey);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create CustomerKey");
            customerKey->Warehouse = warehouse;
            customerKey->District = district;
            customerKey->Customer = customer;
            rwtxCustomer->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
            KeyValuePair<LONG64, CustomerValue::SPtr> resultCustomer;
            succeeded = co_await storeCustomers_->ConditionalGetAsync(
                *rwtxCustomer,
                customerKey,
                timeout,
                resultCustomer,
                ktl::CancellationToken::None);
            ASSERT_IFNOT(succeeded, "Failed to get customer");

            // Update customer Balance, PaymentCount, Data and Ytd.
            CustomerValue::SPtr updatedCustomerValue = nullptr;
            status = CustomerValue::Create(allocator, updatedCustomerValue);
            updatedCustomerValue->FirstName = resultCustomer.Value->FirstName;
            updatedCustomerValue->MiddleName = resultCustomer.Value->MiddleName;
            updatedCustomerValue->LastName = resultCustomer.Value->LastName;
            updatedCustomerValue->Street_1 = resultCustomer.Value->Street_1;
            updatedCustomerValue->Street_2 = resultCustomer.Value->Street_2;
            updatedCustomerValue->City = resultCustomer.Value->City;
            updatedCustomerValue->State = resultCustomer.Value->State;
            updatedCustomerValue->Zip = resultCustomer.Value->Zip;
            updatedCustomerValue->Phone = resultCustomer.Value->Phone;
            //updatedCustomerValue->Since = resultCustomer.Value->Since;
            updatedCustomerValue->Credit = resultCustomer.Value->Credit;
            updatedCustomerValue->CreditLimit = resultCustomer.Value->CreditLimit;
            updatedCustomerValue->Discount = resultCustomer.Value->Discount;
            updatedCustomerValue->Balance = resultCustomer.Value->Balance - payment;
            updatedCustomerValue->Ytd = resultCustomer.Value->Ytd + payment;
            updatedCustomerValue->PaymentCount = resultCustomer.Value->PaymentCount + 1;
            updatedCustomerValue->DeliveryCount = resultCustomer.Value->DeliveryCount;
            updatedCustomerValue->Data = resultCustomer.Value->Data;
            co_await storeCustomers_->ConditionalUpdateAsync(
                *rwtxCustomer,
                customerKey,
                updatedCustomerValue,
                timeout,
                ktl::CancellationToken::None);

            // Update history.
            Data::TStore::IStoreTransaction<KGuid, HistoryValue::SPtr>::SPtr rwtxHistory = nullptr;
            storeHistory_->CreateOrFindTransaction(*replicatorTransaction, rwtxHistory);
            historyValue->Warehouse = warehouse;
            historyValue->District = district;
            historyValue->CustomerWarehouse = warehouse;
            historyValue->District = district;
            //historyValue->Date = DateTime.UtcNow;
            historyValue->Amount = payment;
            historyValue->Customer = customer;
            historyValue->Data = VariableText<24>::GetValue();
            co_await storeHistory_->AddAsync(
                *rwtxHistory,
                historyKey,
                historyValue,
                timeout,
                ktl::CancellationToken::None);

            co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
        }

        TxnReplicator::Transaction::SPtr replicatorTransactionForHistoryUpdate = co_await SafeCreateReplicatorTransactionAsync();
        if (replicatorTransactionForHistoryUpdate != nullptr)
        {
            KFinally([&] {replicatorTransactionForHistoryUpdate->Dispose(); });

            Data::TStore::IStoreTransaction<KGuid, HistoryValue::SPtr>::SPtr rwtxHistory = nullptr;
            storeHistory_->CreateOrFindTransaction(*replicatorTransactionForHistoryUpdate, rwtxHistory);
            co_await storeHistory_->ConditionalUpdateAsync(
                *rwtxHistory,
                historyKey,
                historyValue,
                timeout,
                ktl::CancellationToken::None);
            co_await storeHistory_->ConditionalUpdateAsync(
                *rwtxHistory,
                historyKey,
                historyValue,
                timeout,
                ktl::CancellationToken::None);

            co_await SafeTerminateReplicatorTransactionAsync(replicatorTransactionForHistoryUpdate, true);
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
            case SF_STATUS_TIMEOUT:
            {
                Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
                break;
            }
            default:
            {
                throw;
            }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End NewPaymentTransactionLockBased -----");
    co_return;
}

ktl::Awaitable<void> Service::NewPaymentTransactionVersionBased(int index)
{
    Trace.WriteInfo(TraceComponent, "----- Start NewPaymentTransactionVersionBased -----");

    KAllocator & allocator = GetAllocator();

    Random rnd(GetTickCount() + index);
    int countWarehouseCreated = 2;
    int countDistrictsCreatedPerWarehouse = 10;
    int countCustomersCreatedPerDistrict = 1000;
    TimeSpan timeout = TimeSpan::FromSeconds(5);

    // Generate warehouse.
    int warehouse = rnd.Next(0, countWarehouseCreated);

    // Generate district.
    int district = rnd.Next(0, countDistrictsCreatedPerWarehouse);

    // Generate customer.
    int customer = rnd.Next(0, countCustomersCreatedPerDistrict);

    // Generate payment.
    double payment = rnd.Next(1, 50001);
    //DateTime paymentDate = DateTime.UtcNow;

    try
    {
        bool done = false;
        while (!done)
        {
            // Start new order transaction.
            TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
            if (replicatorTransaction != nullptr)
            {
                KFinally([&] {replicatorTransaction->Dispose(); });

                // Retrieve warehouse.
                Data::TStore::IStoreTransaction<int, WarehouseValue::SPtr>::SPtr rwtxWarehouse = nullptr;
                storeWareHouses_->CreateOrFindTransaction(*replicatorTransaction, rwtxWarehouse);
                KeyValuePair<LONG64, WarehouseValue::SPtr> resultWarehouse;
                bool succeeded = co_await storeWareHouses_->ConditionalGetAsync(
                    *rwtxWarehouse,
                    warehouse,
                    timeout,
                    resultWarehouse,
                    ktl::CancellationToken::None);
                ASSERT_IFNOT(succeeded, "Failed to get warehouse");

                // Attempt to update warehouse Ytd.
                WarehouseValue::SPtr updatedWarehouseValue = nullptr;
                NTSTATUS status = WarehouseValue::Create(allocator, updatedWarehouseValue);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create WarehouseValue");
                updatedWarehouseValue->Name = resultWarehouse.Value->Name;
                updatedWarehouseValue->Street_1 = resultWarehouse.Value->Street_1;
                updatedWarehouseValue->Street_2 = resultWarehouse.Value->Street_2;
                updatedWarehouseValue->City = resultWarehouse.Value->City;
                updatedWarehouseValue->State = resultWarehouse.Value->State;
                updatedWarehouseValue->Zip = resultWarehouse.Value->Zip;
                updatedWarehouseValue->Tax = resultWarehouse.Value->Tax;
                updatedWarehouseValue->Ytd = resultWarehouse.Value->Ytd + payment;
                bool updateWarehouseOutcome = co_await storeWareHouses_->ConditionalUpdateAsync(
                    *rwtxWarehouse,
                    warehouse,
                    updatedWarehouseValue,
                    timeout,
                    ktl::CancellationToken::None);
                if (!updateWarehouseOutcome) {
                    // TODO: trace
                    continue;
                }

                // Retrieve district.
                Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rwtxDistrict = nullptr;
                storeDistricts_->CreateOrFindTransaction(*replicatorTransaction, rwtxDistrict);
                DistrictKey::SPtr districtKey = nullptr;
                status = DistrictKey::Create(allocator, districtKey);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create DistrictKey");
                districtKey->Warehouse = warehouse;
                districtKey->District = district;
                KeyValuePair<LONG64, DistrictValue::SPtr> resultDistrict;
                succeeded = co_await storeDistricts_->ConditionalGetAsync(
                    *rwtxDistrict,
                    districtKey,
                    timeout,
                    resultDistrict,
                    ktl::CancellationToken::None);
                ASSERT_IFNOT(succeeded, "Failed to get district");

                // Attempt to update district Ytd.
                DistrictValue::SPtr updatedDisctrictValue = nullptr;
                status = DistrictValue::Create(allocator, updatedDisctrictValue);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create DistrictValue");
                updatedDisctrictValue->Name = resultDistrict.Value->Name;
                updatedDisctrictValue->Street_1 = resultDistrict.Value->Street_1;
                updatedDisctrictValue->Street_2 = resultDistrict.Value->Street_2;
                updatedDisctrictValue->City = resultDistrict.Value->City;
                updatedDisctrictValue->State = resultDistrict.Value->State;
                updatedDisctrictValue->Zip = resultDistrict.Value->Zip;
                updatedDisctrictValue->Tax = resultDistrict.Value->Tax;
                updatedDisctrictValue->Ytd = resultDistrict.Value->Ytd + payment;
                updatedDisctrictValue->NextOrderId = resultDistrict.Value->NextOrderId;
                bool updateDistrictOutcome = co_await storeDistricts_->ConditionalUpdateAsync(
                    *rwtxDistrict,
                    districtKey,
                    updatedDisctrictValue,
                    timeout,
                    ktl::CancellationToken::None);
                if (!updateDistrictOutcome) {
                    // TODO: trace
                    continue;
                }

                // Retrieve customer.
                Data::TStore::IStoreTransaction<CustomerKey::SPtr, CustomerValue::SPtr>::SPtr rwtxCustomer = nullptr;
                storeCustomers_->CreateOrFindTransaction(*replicatorTransaction, rwtxCustomer);
                CustomerKey::SPtr customerKey = nullptr;
                status = CustomerKey::Create(allocator, customerKey);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create CustomerKey");
                customerKey->Warehouse = warehouse;
                customerKey->District = district;
                customerKey->Customer = customer;
                KeyValuePair<LONG64, CustomerValue::SPtr> resultCustomer;
                succeeded = co_await storeCustomers_->ConditionalGetAsync(
                    *rwtxCustomer,
                    customerKey,
                    timeout,
                    resultCustomer,
                    ktl::CancellationToken::None);
                ASSERT_IFNOT(succeeded, "Failed to get customer");

                // Attempt to update customer Balance, PaymentCount, Data and Ytd.
                CustomerValue::SPtr updatedCustomerValue = nullptr;
                status = CustomerValue::Create(allocator, updatedCustomerValue);
                updatedCustomerValue->FirstName = resultCustomer.Value->FirstName;
                updatedCustomerValue->MiddleName = resultCustomer.Value->MiddleName;
                updatedCustomerValue->LastName = resultCustomer.Value->LastName;
                updatedCustomerValue->Street_1 = resultCustomer.Value->Street_1;
                updatedCustomerValue->Street_2 = resultCustomer.Value->Street_2;
                updatedCustomerValue->City = resultCustomer.Value->City;
                updatedCustomerValue->State = resultCustomer.Value->State;
                updatedCustomerValue->Zip = resultCustomer.Value->Zip;
                updatedCustomerValue->Phone = resultCustomer.Value->Phone;
                //updatedCustomerValue->Since = resultCustomer.Value->Since;
                updatedCustomerValue->Credit = resultCustomer.Value->Credit;
                updatedCustomerValue->CreditLimit = resultCustomer.Value->CreditLimit;
                updatedCustomerValue->Discount = resultCustomer.Value->Discount;
                updatedCustomerValue->Balance = resultCustomer.Value->Balance - payment;
                updatedCustomerValue->Ytd = resultCustomer.Value->Ytd + payment;
                updatedCustomerValue->PaymentCount = resultCustomer.Value->PaymentCount + 1;
                updatedCustomerValue->DeliveryCount = resultCustomer.Value->DeliveryCount;
                updatedCustomerValue->Data = resultCustomer.Value->Data;
                bool updateCustomerOutcome = co_await storeCustomers_->ConditionalUpdateAsync(
                    *rwtxCustomer,
                    customerKey,
                    updatedCustomerValue,
                    timeout,
                    ktl::CancellationToken::None);
                if (!updateCustomerOutcome) {
                    // TODO: trace
                    continue;
                }

                // Update history.
                Data::TStore::IStoreTransaction<KGuid, HistoryValue::SPtr>::SPtr rwtxHistory = nullptr;
                storeHistory_->CreateOrFindTransaction(*replicatorTransaction, rwtxHistory);
                KGuid historyKey(Guid::NewGuid().AsGUID());
                HistoryValue::SPtr historyValue = nullptr;
                status = HistoryValue::Create(allocator, historyValue);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create HistoryValue");
                historyValue->Warehouse = warehouse;
                historyValue->District = district;
                historyValue->CustomerWarehouse = warehouse;
                historyValue->District = district;
                //historyValue->Date = DateTime.UtcNow;
                historyValue->Amount = payment;
                historyValue->Customer = customer;
                historyValue->Data = VariableText<24>::GetValue();
                co_await storeHistory_->AddAsync(
                    *rwtxHistory,
                    historyKey,
                    historyValue,
                    timeout,
                    ktl::CancellationToken::None);

                co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);

                done = true;
            }
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
            case SF_STATUS_TIMEOUT:
            {
                Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
                break;
            }
            default:
            {
                throw;
            }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End NewPaymentTransactionVersionBased -----");
    co_return;
}

ktl::Awaitable<void> Service::NewDeliveryTransactionLockBased(int index)
{
    Trace.WriteInfo(TraceComponent, "----- Start NewDeliveryTransactionLockBased -----");

    KAllocator & allocator = GetAllocator();

    NTSTATUS status = STATUS_SUCCESS;
    bool succeeded = true;
    Random rnd(GetTickCount() + index);
    TimeSpan timeout = TimeSpan::FromSeconds(5);

    // Generate carrier.
    int carrier = rnd.Next(0, 10);

    // Delivery date.
    //DateTime delivery = DateTime.UtcNow.AddDays(rnd.Next(1, 3));
    try {
        // Start new order transaction.
        TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
        if (replicatorTransaction = nullptr)
        {
            KFinally([&] {replicatorTransaction->Dispose(); });

            Data::TStore::IStoreTransaction<DistrictKey::SPtr, DistrictValue::SPtr>::SPtr rtxDistricts = nullptr;
            storeDistricts_->CreateOrFindTransaction(*replicatorTransaction, rtxDistricts);
            rtxDistricts->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;

            // Select lowest new order.
            Data::TStore::IStoreTransaction<NewOrderKey::SPtr, NewOrderValue::SPtr>::SPtr rwtxNewOrders = nullptr;
            storeNewOrders_->CreateOrFindTransaction(*replicatorTransaction, rwtxNewOrders);
            rwtxNewOrders->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;

            // Select order to update.
            Data::TStore::IStoreTransaction<OrderKey::SPtr, OrderValue::SPtr>::SPtr rwtxOrders = nullptr;
            storeOrders_->CreateOrFindTransaction(*replicatorTransaction, rwtxOrders);
            rwtxOrders->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;

            // Select order line to update.
            Data::TStore::IStoreTransaction<OrderLineKey::SPtr, OrderLineValue::SPtr>::SPtr rwtxOrderLine = nullptr;
            storeOrderLine_->CreateOrFindTransaction(*replicatorTransaction, rwtxOrderLine);
            rwtxOrderLine->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;

            // Select customer to update.
            Data::TStore::IStoreTransaction<CustomerKey::SPtr, CustomerValue::SPtr>::SPtr rwtxCustomer = nullptr;
            storeCustomers_->CreateOrFindTransaction(*replicatorTransaction, rwtxCustomer);

            // Iterate over districts.
            int countOrders = 0;
            auto enumerateDistricts = co_await storeDistricts_->CreateEnumeratorAsync(*rtxDistricts);
            while (co_await enumerateDistricts->MoveNextAsync(ktl::CancellationToken::None)) {
                KeyValuePair<DistrictKey::SPtr, KeyValuePair<LONG64, DistrictValue::SPtr>> districtItem = enumerateDistricts->GetCurrent();

                NewOrderKey::SPtr newOrderKey = nullptr;
                auto enumerateNewOrder = co_await storeNewOrders_->CreateEnumeratorAsync(*rwtxNewOrders);
                while (co_await enumerateNewOrder->MoveNextAsync(ktl::CancellationToken::None)) {
                    NewOrderKey::SPtr x = enumerateNewOrder->GetCurrent().Key;
                    if (x->Warehouse == districtItem.Key->Warehouse && x->District == districtItem.Key->District) {
                        newOrderKey = x;
                        break;
                    }
                }

                if (newOrderKey != nullptr) {
                    countOrders++;

                    // Delete the new order.
                    succeeded = co_await storeNewOrders_->ConditionalRemoveAsync(
                        *rwtxNewOrders,
                        newOrderKey,
                        timeout,
                        ktl::CancellationToken::None);
                    ASSERT_IFNOT(succeeded, "Failed to remove new order");

                    // Update the order.
                    KeyValuePair<OrderKey::SPtr, KeyValuePair<LONG64, OrderValue::SPtr>> order;
                    auto enumerateOrder = co_await storeOrders_->CreateEnumeratorAsync(*rwtxOrders);
                    while (co_await enumerateOrder->MoveNextAsync(ktl::CancellationToken::None)) {
                        OrderKey::SPtr x = enumerateOrder->GetCurrent().Key;
                        if (x->Warehouse == districtItem.Key->Warehouse &&
                            x->District == districtItem.Key->District &&
                            x->Order == newOrderKey->Order)
                        {
                            order = enumerateOrder->GetCurrent();
                            break;
                        }
                    }
                    ASSERT_IFNOT(order.Value.Value != nullptr, "OrderValue == NULL");

                    // Update order.
                    OrderValue::SPtr updatedOrderValue = nullptr;
                    status = OrderValue::Create(allocator, updatedOrderValue);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderValue");
                    updatedOrderValue->Carrier = carrier;
                    //updatedOrderValue->Entry = order.Value->Entry;
                    updatedOrderValue->LineCount = order.Value.Value->LineCount;
                    updatedOrderValue->Local = order.Value.Value->Local;
                    co_await storeOrders_->ConditionalUpdateAsync(
                        *rwtxOrders,
                        order.Key,
                        updatedOrderValue,
                        timeout,
                        ktl::CancellationToken::None);

                    // Update all order line for this order.
                    double amount = 0;
                    auto orderLineEnumerate = co_await storeOrderLine_->CreateEnumeratorAsync(*rwtxOrderLine);
                    while (co_await orderLineEnumerate->MoveNextAsync(ktl::CancellationToken::None)) {
                        KeyValuePair<OrderLineKey::SPtr, KeyValuePair<LONG64, OrderLineValue::SPtr>> orderLine = orderLineEnumerate->GetCurrent();
                        OrderLineKey::SPtr x = orderLine.Key;
                        if (x->Warehouse == districtItem.Key->Warehouse &&
                            x->District == districtItem.Key->District &&
                            x->Order == newOrderKey->Order)
                        {
                            amount += orderLine.Value.Value->Amount;

                            // Update order line.
                            OrderLineValue::SPtr updatedOrderLine = nullptr;
                            status = OrderLineValue::Create(allocator, updatedOrderLine);
                            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create OrderLineValue");
                            updatedOrderLine->Quantity = orderLine.Value.Value->Quantity;
                            updatedOrderLine->Amount = orderLine.Value.Value->Amount;
                            //updatedOrderLine->Delivery = delivery;
                            updatedOrderLine->Info = VariableText<24>::GetValue();

                            co_await storeOrderLine_->ConditionalUpdateAsync(
                                *rwtxOrderLine,
                                orderLine.Key,
                                updatedOrderLine,
                                timeout,
                                ktl::CancellationToken::None);
                        }
                    }

                    CustomerKey::SPtr customerKey = nullptr;
                    status = CustomerKey::Create(allocator, customerKey);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create CustomerKey");
                    customerKey->Warehouse = order.Key->Warehouse;
                    customerKey->District = order.Key->District;
                    customerKey->Customer = order.Key->Customer;
                    KeyValuePair<LONG64, CustomerValue::SPtr> resultCustomer;
                    succeeded = co_await storeCustomers_->ConditionalGetAsync(
                        *rwtxCustomer,
                        customerKey,
                        timeout,
                        resultCustomer,
                        ktl::CancellationToken::None);
                    ASSERT_IFNOT(succeeded, "Failed ot get customer");

                    // Update customer.
                    CustomerValue::SPtr updatedCustomerValue = nullptr;
                    status = CustomerValue::Create(allocator, updatedCustomerValue);
                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create CustomerValue");
                    updatedCustomerValue->FirstName = resultCustomer.Value->FirstName;
                    updatedCustomerValue->MiddleName = resultCustomer.Value->MiddleName;
                    updatedCustomerValue->LastName = resultCustomer.Value->LastName;
                    updatedCustomerValue->Street_1 = resultCustomer.Value->Street_1;
                    updatedCustomerValue->Street_2 = resultCustomer.Value->Street_2;
                    updatedCustomerValue->City = resultCustomer.Value->City;
                    updatedCustomerValue->State = resultCustomer.Value->State;
                    updatedCustomerValue->Zip = resultCustomer.Value->Zip;
                    updatedCustomerValue->Phone = resultCustomer.Value->Phone;
                    //updatedCustomerValue->Since = resultCustomer.Value.Since;
                    updatedCustomerValue->Credit = resultCustomer.Value->Credit;
                    updatedCustomerValue->CreditLimit = resultCustomer.Value->CreditLimit;
                    updatedCustomerValue->Discount = resultCustomer.Value->Discount;
                    updatedCustomerValue->Balance = amount;
                    updatedCustomerValue->Ytd = resultCustomer.Value->Ytd;
                    updatedCustomerValue->PaymentCount = resultCustomer.Value->PaymentCount;
                    updatedCustomerValue->DeliveryCount = resultCustomer.Value->DeliveryCount + 1;
                    updatedCustomerValue->Data = resultCustomer.Value->Data;
                    co_await storeCustomers_->ConditionalUpdateAsync(
                        *rwtxCustomer,
                        customerKey,
                        updatedCustomerValue,
                        timeout,
                        ktl::CancellationToken::None);
                }
            }

            if (countOrders == 0) {
                // TODO: trace
            }

            // Commit replicator transaction.
            co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
            case SF_STATUS_TIMEOUT:
            {
                Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
                break;
            }
            default:
            {
                throw;
            }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End NewDeliveryTransactionLockBased -----");
    co_return;
}

ktl::Awaitable<void> Service::ClearOldOrdersTransactionLockBased()
{
    Trace.WriteInfo(TraceComponent, "----- Start ClearOldOrdersTransactionLockBased -----");

    KAllocator & allocator = GetAllocator();
    NTSTATUS status = STATUS_SUCCESS;

    Random rnd(GetTickCount());
    int countCustomersCreatedPerDistrict = 1000;
    TimeSpan timeout = TimeSpan::FromSeconds(5);

    // Generate customer.
    int customer = rnd.Next(0, countCustomersCreatedPerDistrict);

    try {
        TxnReplicator::Transaction::SPtr replicatorTransaction = co_await SafeCreateReplicatorTransactionAsync();
        if (replicatorTransaction != nullptr)
        {
            KFinally([&] {replicatorTransaction->Dispose(); });

            // Enumerate all orders.
            Data::TStore::IStoreTransaction<OrderKey::SPtr, OrderValue::SPtr>::SPtr rwtxOrders = nullptr;
            storeOrders_->CreateOrFindTransaction(*replicatorTransaction, rwtxOrders);
            rwtxOrders->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
            auto enumerateOrders = co_await storeOrders_->CreateEnumeratorAsync(*rwtxOrders);
            while (co_await enumerateOrders->MoveNextAsync(ktl::CancellationToken::None)) {
                KeyValuePair<OrderKey::SPtr, KeyValuePair<LONG64, OrderValue::SPtr>> order = enumerateOrders->GetCurrent();
                if (order.Key->Customer == customer) {
                    bool newOrderDoesNotExist = false;
                    {
                        Data::TStore::IStoreTransaction<NewOrderKey::SPtr, NewOrderValue::SPtr>::SPtr rtxNewOrders = nullptr;
                        storeNewOrders_->CreateOrFindTransaction(*replicatorTransaction, rtxNewOrders);

                        NewOrderKey::SPtr newOrderKey = nullptr;
                        status = NewOrderKey::Create(allocator, newOrderKey);
                        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create NewOrderKey");
                        newOrderKey->Warehouse = order.Key->Warehouse;
                        newOrderKey->District = order.Key->District;
                        newOrderKey->Order = order.Key->Order;

                        KeyValuePair<LONG64, NewOrderValue::SPtr> temp;
                        newOrderDoesNotExist = co_await storeNewOrders_->ConditionalGetAsync(
                            *rtxNewOrders,
                            newOrderKey,
                            timeout,
                            temp,
                            ktl::CancellationToken::None);
                    }

                    if (!newOrderDoesNotExist) {
                        // Delete old order.
                        co_await storeOrders_->ConditionalRemoveAsync(
                            *rwtxOrders,
                            order.Key,
                            timeout,
                            ktl::CancellationToken::None);

                        // Delete the order lines associated with this old order.
                        Data::TStore::IStoreTransaction<OrderLineKey::SPtr, OrderLineValue::SPtr>::SPtr rwtxOrderLine = nullptr;
                        storeOrderLine_->CreateOrFindTransaction(*replicatorTransaction, rwtxOrderLine);
                        rwtxOrderLine->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;

                        auto enumerateOrderLine = co_await storeOrderLine_->CreateEnumeratorAsync(*rwtxOrderLine);
                        while (co_await enumerateOrderLine->MoveNextAsync(ktl::CancellationToken::None)) {
                            KeyValuePair<OrderLineKey::SPtr, KeyValuePair<LONG64, OrderLineValue::SPtr>> ol = enumerateOrderLine->GetCurrent();
                            if (ol.Key->Order == order.Key->Order &&
                                ol.Key->District == order.Key->District &&
                                ol.Key->Warehouse == order.Key->Warehouse)
                            {
                                co_await storeOrderLine_->ConditionalRemoveAsync(
                                    *rwtxOrderLine,
                                    ol.Key,
                                    timeout,
                                    ktl::CancellationToken::None);
                            }
                        }
                    }
                }
            }

            co_await SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
        }
    }
    catch (ktl::Exception& e)
    {
        switch (e.GetStatus())
        {
            case SF_STATUS_TIMEOUT:
            {
                Trace.WriteWarning(TraceComponent, "SF_STATUS_TIMEOUT exception.");
                break;
            }
            default:
            {
                throw;
            }
        }
    }

    Trace.WriteInfo(TraceComponent, "----- End ClearOldOrdersTransactionLockBased -----");
    co_return;
}

ktl::Awaitable<void> Service::ClearHistoryTable(int index)
{
    Trace.WriteInfo(TraceComponent, "----- Start ClearHistoryTable -----");

    //bool removeDone = false;
    //TimeSpan timeout = TimeSpan::FromSeconds(5);

    //LONG64 countHistoryItems = storeHistory_->Count;
    //if (countHistoryItems > 64) {
    //	TxnReplicator::Transaction::SPtr tx = TxReplicator->CreateTransaction();
    //	KFinally([&] {tx->Dispose(); });

    //	Data::TStore::IStoreTransaction<KGuid, HistoryValue::SPtr>::SPtr rwtxHistory = nullptr;
    //	storeHistory_->CreateOrFindTransaction(*tx, rwtxHistory);

    //	auto enumerateHistory = co_await storeHistory_->CreateEnumeratorAsync(*rwtxHistory);
    //	while (co_await enumerateHistory->MoveNextAsync(CancellationToken::None)) {
    //		KeyValuePair<KGuid, KeyValuePair<LONG64, HistoryValue::SPtr>> history = enumerateHistory->GetCurrent();
    //		co_await storeHistory_->ConditionalRemoveAsync(
    //			*rwtxHistory,
    //			history.Key,
    //			timeout);
    //	}

    //	co_await tx->CommitAsync();

    //	removeDone = true;
    //}

    Trace.WriteInfo(TraceComponent, "----- End ClearHistoryTable -----");
    co_return;
}

ktl::Awaitable<void> Service::Workload1(int indexCapture, bool lockBased)
{
    Trace.WriteInfo(TraceComponent, "----- Start Workload1 -----");

    for (int y = 0; y < 2; ++y) {
        co_await NewOrderTransactionLockBased(indexCapture);
        if (lockBased) {
            co_await NewPaymentTransactionLockBased(indexCapture);
        }
        else {
            co_await NewPaymentTransactionVersionBased(indexCapture);
        }
    }

    co_await NewDeliveryTransactionLockBased(indexCapture);
    co_await ClearOldOrdersTransactionLockBased();

    Trace.WriteInfo(TraceComponent, "----- End Workload1 -----");
    co_return;
}

ktl::Awaitable<void> Service::Workload2(int indexCapture, bool lockBased)
{
    Trace.WriteInfo(TraceComponent, "----- Start Workload2 -----");

    for (int y = 0; y < 2; ++y) {
        co_await NewOrderTransactionVersionBased(indexCapture);
        if (lockBased) {
            co_await NewPaymentTransactionLockBased(indexCapture);
        }
        else {
            co_await NewPaymentTransactionVersionBased(indexCapture);
        }
    }

    co_await NewDeliveryTransactionLockBased(indexCapture);
    co_await ClearOldOrdersTransactionLockBased();

    Trace.WriteInfo(TraceComponent, "----- End Workload2 -----");
    co_return;
}

ktl::Awaitable<void> Service::NewOrderTpccWorkloadOnPrimaryAsync()
{
    Trace.WriteInfo(TraceComponent, "----- Start NewOrderTpccWorkloadOnPrimaryAsync -----");

    Trace.WriteInfo(TraceComponent, "Workload starting");

    KAllocator & allocator = GetAllocator();
    NTSTATUS status = STATUS_SUCCESS;
    Random rnd(0);
    int index = 0;
    for (int times = 0; times < 16; times++) {
        Trace.WriteInfo(TraceComponent, "Iteration {0}", times);

        // start new order transactions
        KSharedArray<ktl::Awaitable<void>>::SPtr workloads = _new('xxxx', allocator) KSharedArray<ktl::Awaitable<void>>();
        ASSERT_IFNOT(workloads != nullptr, "Failed to create KSharedArray");
        for (int x = 0; x < 8; ++x) {
            int indexCapture = index++;
            bool lockBased = 0 == rnd.Next() % 2;
            if (lockBased) {
                lockBased = 0 == rnd.Next() % 2;
                status = workloads->Append(Ktl::Move(Workload1(indexCapture, lockBased)));
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to append workload");
            }
            else {
                lockBased = 0 == rnd.Next() % 2;
                status = workloads->Append(Ktl::Move(Workload2(indexCapture, lockBased)));
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to append workload");
            }
        }

        status = workloads->Append(Ktl::Move(SecondaryWorkloadTransaction2(false)));
        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to append workload");

        if (workloads->Count() > 0) {
            co_await Utilities::TaskUtilities<void>::WhenAll(*workloads);
        }

        Trace.WriteInfo(TraceComponent, "Worload done");
    }

    Trace.WriteInfo(TraceComponent, "----- End NewOrderTpccWorkloadOnPrimaryAsync -----");

    co_return;
}

ktl::Awaitable<TxnReplicator::Transaction::SPtr> Service::SafeCreateReplicatorTransactionAsync()
{
    NTSTATUS status = STATUS_SUCCESS;
    KAllocator & allocator = GetAllocator();
    TxnReplicator::Transaction::SPtr replicatorTransaction = nullptr;

    while (true)
    {
        bool doneCreateTx = false;
        try
        {
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
            doneCreateTx = true;
        }
        catch (ktl::Exception& e)
        {
            // retry ?
            TraceException(TraceComponent, e, allocator);
            co_return replicatorTransaction;
        }

        if (doneCreateTx)
        {
            break;
        }

        status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
    }

    co_return replicatorTransaction;
}

ktl::Awaitable<void> Service::SafeTerminateReplicatorTransactionAsync(
    TxnReplicator::Transaction::SPtr replicatorTransaction,
    bool commit)
{
    NTSTATUS status = STATUS_SUCCESS;
    KAllocator & allocator = GetAllocator();
    while (true)
    {
        bool doneCommitTx = false;
        try
        {
            if (commit)
            {
                THROW_ON_FAILURE(co_await replicatorTransaction->CommitAsync());
            }
            else
            {
                replicatorTransaction->Abort();
            }

            doneCommitTx = true;
        }
        catch (ktl::Exception& e)
        {
            // Retry ?
            TraceException(TraceComponent, e, allocator);
            co_return;
        }

        if (doneCommitTx)
        {
            break;
        }
        
        status = co_await KTimer::StartTimerAsync(allocator, SERVICE_TAG, 100, nullptr);
        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to start timer");
    }

    co_return;
}
