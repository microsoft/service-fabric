// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <SfStatus.h>

using namespace ktl;
using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TStoreLonghaul;
using namespace TxnReplicator;
using namespace Data;
using namespace TestableService;

static const StringLiteral TraceComponent("TStoreLonghaulService");
KStringView const TypeName = L"TestStateProvider";

// Schedule work on client request.
ktl::Awaitable<void> Service::DoWorkAsync()
{
    // Protect against double DoWorkAsync call.
    // If doWork is in progress, just return. Otherwise set the flag to true.
    if (doWorkInProgress_)
    {
        co_return;
    }
    else
    {
        doWorkInProgress_ = true;
    }

    Common::Random random = Common::Random();
    int seed = random.Next() % 3 + 5;
    //int seed = 5;
    Trace.WriteInfo(TraceComponent, "{0}:DoWorkAsync: Seed is {1}.", replicaId_, seed);

    try
    {
        switch (seed)
        {
        case 0:
        {
            co_await VerifySerialCreationDeletionAsync(AddMode::AddAsync, GetMode::Get);
            break;
        }
        case 1:
        {
            co_await VerifySerialCreationDeletionAsync(AddMode::GetOrAddAsync, GetMode::Get);
            break;
        }
        case 2:
        {
            co_await VerifyConcurrentCreationDeletionAsync(AddMode::AddAsync, GetMode::Get);
            break;
        }
        case 3:
        {
            co_await VerifyConcurrentCreationDeletionAsync(AddMode::GetOrAddAsync, GetMode::Get);
            break;
        }
        case 4:
        {
            co_await VerifyConcurrentCreationDeletionAndGetAsync();
            break;
        }
        case 5:
        {
            co_await VerifyConcurrentCreationDeletionWithTransactionOnStoresAsync();
            break;
        }
        case 6:
        {
            co_await VerifyConcurrentCreationWithTransactionOnStoresAsync();
            break;
        }
        case 7:
        {
            co_await VerifyStoreCreationWithTransactionsOnStoreAsync();
            break;
        }
        default:
        {
            // Should not get here.
            CODING_ERROR_ASSERT(false);
        }
        }
    }
    catch (ktl::Exception & exception)
    {
        switch (exception.GetStatus())
        {
        case SF_STATUS_NOT_PRIMARY:
        {
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        case SF_STATUS_OBJECT_CLOSED:
        {
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        case SF_STATUS_OBJECT_DISPOSED:
        {
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        case SF_STATUS_INVALID_OPERATION:
        {
            // TODO: Cover this case since state provider not registered throw invalid operation exception.
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        case STATUS_CANCELLED:
        {
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        default:
        {
            // TODO: need to think through all the exceptions it might throw
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw;
        }
        }
    }

    Trace.WriteInfo(TraceComponent, "{0}:DoWorkAsync: Completed.", replicaId_);
    doWorkInProgress_ = false;

    co_return;
}

// Get progress, it gets the state providers count on the replica 
ktl::Awaitable<TestableServiceProgress> Service::GetProgressAsync()
{
    Trace.WriteInfo(TraceComponent, "{0}:GetProgressAsync: Started.", replicaId_);
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Verification
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    ULONG count = 0;

    while (enumerator->MoveNext())
    {
        ++count;
    }

    Trace.WriteInfo(TraceComponent, "{0}:GetProgressAsync: The state provider count is {1}", replicaId_, count);

    // Get the number of state providers in each store.
    TestableServiceProgress progress;
    progress.CustomProgress.push_back(count);

    status = TxReplicator->GetLastCommittedSequenceNumber(progress.LastCommittedSequenceNumber);
    THROW_ON_FAILURE(status);

    FABRIC_EPOCH e;
    status = TxReplicator->GetCurrentEpoch(e);
    THROW_ON_FAILURE(status);

    progress.DataLossNumber = e.DataLossNumber;
    progress.ConfigurationNumber = e.ConfigurationNumber;

    Trace.WriteInfo(TraceComponent, "{0}:GetProgressAsync: Completed.", replicaId_);

    co_return progress;
}

// Serial Creation and Deletion test.
// 1. Delete all the state providers added by this test case, with certain prefix.
// 2. Add state providers to the StateManager.
// 3. Verify all the state providers are added as expected.
// 4. Verify enumeration.
ktl::Awaitable<void> Service::VerifySerialCreationDeletionAsync(
    __in AddMode addMode,
    __in GetMode getMode)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // The prefix used to checkout the existence of state providers.
    KStringView prefix = L"fabric:/serial";
    ULONG numOfStateProviders = 30;
    IStateProvider2::SPtr stateProvider;

    // Populate state providers name list
    KArray<KUri::CSPtr> nameList(GetAllocator(), numOfStateProviders);
    for (ULONG index = 0; index < numOfStateProviders; ++index)
    {
        KUri::CSPtr nameSPtr = GetStateProviderName(TestCase::Serially, index);
        status = nameList.Append(nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:VerifySerialCreationDeletionAsync: Append name to list must be successful.", 
            replicaId_);
    }

    // Delete state providers if any exists.
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    while (enumerator->MoveNext())
    {
        auto currentKey = enumerator->Current().Key;
        bool result = IsPrefix(*currentKey, prefix);
        if (result)
        {
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));

            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await TxReplicator->RemoveAsync(
                *txnSPtr,
                *currentKey);
            THROW_ON_FAILURE(status);

            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    // Add state providers
    for (KUri::CSPtr nameSPtr : nameList)
    {
        Transaction::SPtr txnSPtr;
        THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
        KFinally([&] { txnSPtr->Dispose(); });

        if (addMode == AddMode::AddAsync)
        {
            status = co_await TxReplicator->AddAsync(
                *txnSPtr,
                *nameSPtr,
                TypeName);
            THROW_ON_FAILURE(status);
        }
        else
        {
            ASSERT_IFNOT(
                addMode == AddMode::GetOrAddAsync, 
                "{0}:VerifySerialCreationDeletionAsync: AddMode must be GetOrAddAsync.",
                replicaId_);
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                *nameSPtr,
                TypeName,
                stateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            ASSERT_IFNOT(
                alreadyExist == false,
                "{0}:VerifySerialCreationDeletionAsync: Add a state provider.",
                replicaId_);
        }

        co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
    }

    // Verify all the state providers are added
    for (KUri::CSPtr nameSPtr : nameList)
    {
        if (getMode == GetMode::Get)
        {
            status = TxReplicator->Get(*nameSPtr, stateProvider);
            if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
            {
                throw ktl::Exception(status);
            }

            ASSERT_IFNOT(
                NT_SUCCESS(status), 
                "{0}:VerifySerialCreationDeletionAsync: State provider must be added.",
                replicaId_);
        }
        else
        {
            ASSERT_IFNOT(
                getMode == GetMode::GetOrAdd, 
                "{0}:VerifySerialCreationDeletionAsync: GetMode must be GetOrAddAsync.",
                replicaId_);
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                *nameSPtr,
                TypeName,
                stateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            ASSERT_IFNOT(
                alreadyExist,
                "{0}:VerifySerialCreationDeletionAsync: Get a state provider.",
                replicaId_);
            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    VerifyEnumeration(TestCase::Serially, numOfStateProviders);

    co_return;
}

// Concurrent Creation and Deletion test.
// 1. Delete all the state providers added by this test case, with certain prefix.
// 2. Concurrently add state providers to the StateManager.
// 3. Verify all the state providers are added as expected.
// 4. Verify the enumeration.
ktl::Awaitable<void> Service::VerifyConcurrentCreationDeletionAsync(
    __in AddMode addMode,
    __in GetMode getMode)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ULONG concurrentExecutions = 20;
    ULONG workLoadPerTask = 25;

    KArray<Awaitable<void>> taskArray(GetAllocator());
    
    AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
    status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:VerifyConcurrentCreationDeletionAsync: Failed to create the AwaitableCompletionSource.",
        replicaId_);

    for (ULONG i = 0; i < concurrentExecutions; ++i)
    {
        status = taskArray.Append(WorkLoadTask(*signalCompletion, i, workLoadPerTask, addMode, getMode));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:VerifyConcurrentCreationDeletionAsync: Failed to append WorkLoadTask to taskArray.",
            replicaId_);
    }

    signalCompletion->SetResult(true);

    co_await (Utilities::TaskUtilities<void>::WhenAll(taskArray));

    VerifyEnumeration(TestCase::ConcurrentSameTask, concurrentExecutions * workLoadPerTask);

    co_return;
}

ktl::Awaitable<void> Service::VerifyConcurrentCreationDeletionWithTransactionOnStoresAsync()
{
    int noOfStores = 5;
    int storeTransactions = 20;
    std::wstring generalPrefix = L"storetxwithdelete";
    int concurrentExecutions = 5;
    int startPrefixCount = -1;

    KArray<ktl::Awaitable<void>> workLoad(GetAllocator());

    for (int x = 0; x < concurrentExecutions; ++x)
    {
        startPrefixCount++;
        std::wstring prefix = generalPrefix + prefixCharacters[startPrefixCount];

        auto work = [=](std::wstring prefix) -> ktl::Awaitable<void>
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            // step 1: Delete stores if any.
            {
                Transaction::SPtr replicatorTransaction;
                THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
                KFinally([&] { replicatorTransaction->Dispose(); });

                Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
                status = TxReplicator->CreateEnumerator(false, enumerator);
                THROW_ON_FAILURE(status);
                while (enumerator->MoveNext())
                {
                    auto currentKey = enumerator->Current().Key;
                    std::wstring storeName(static_cast<wchar_t *>((*currentKey).Get(KUri::UriFieldType::eRaw)));
                    if ((storeName.find(L"count") == 0) || !(storeName.find(prefix) == 0))
                    {
                        continue;
                    }

                    status = co_await TxReplicator->RemoveAsync(
                        *replicatorTransaction,
                        *currentKey
                    );
                    THROW_ON_FAILURE(status);
                }

                co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, TxnAction::Commit);
            }

            // step 2: Create stores.
            {
                Transaction::SPtr replicatorTransaction;
                THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
                KFinally([&] { replicatorTransaction->Dispose(); });

                for (int i = 1; i <= noOfStores; ++i)
                {
                    std::wstring storeName = prefix + L"fabric:/Store" + std::to_wstring(i);

                    status = co_await TxReplicator->AddAsync(
                        *replicatorTransaction,
                        &storeName[0],
                        TypeName
                    );
                    THROW_ON_FAILURE(status);
                }

                co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, TxnAction::Commit);
            }

            // step 3: Add value
            {
                Transaction::SPtr replicatorTransaction;
                THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
                KFinally([&] { replicatorTransaction->Dispose(); });

                for (int i = 1; i <= noOfStores; ++i)
                {
                    std::wstring storeName = prefix + L"fabric:/Store" + std::to_wstring(i);

                    TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
                    status = TxReplicator->Get(&storeName[0], providerSP);
                    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
                    {
                        throw ktl::Exception(status);
                    }

                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to find stateprovider {0}", storeName);

                    KSharedPtr<Data::TStore::IStore<LONG64, KString::SPtr>> store = nullptr;
                    store = dynamic_cast<Data::TStore::IStore<LONG64, KString::SPtr>*>(providerSP.RawPtr());

                    Data::TStore::IStoreTransaction<LONG64, KString::SPtr>::SPtr rwtx1 = nullptr;
                    store->CreateOrFindTransaction(*replicatorTransaction, rwtx1);

                    KString::SPtr value;
                    KString::Create(value, GetAllocator(), L"add");
                    for (int storeTx = 0; storeTx < storeTransactions; ++storeTx)
                    {
                        co_await store->AddAsync(
                            *rwtx1,
                            storeTx,
                            value,
                            TimeSpan::MaxValue,
                            ktl::CancellationToken::None
                        );
                    }
                }

                co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, TxnAction::Commit);
            }

            // step 4: Update value
            {
                Transaction::SPtr replicatorTransaction;
                THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
                KFinally([&] { replicatorTransaction->Dispose(); });

                for (int i = 1; i <= noOfStores; ++i)
                {
                    std::wstring storeName = prefix + L"fabric:/Store" + std::to_wstring(i);
                    TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
                    status = TxReplicator->Get(&storeName[0], providerSP);
                    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
                    {
                        throw ktl::Exception(status);
                    }

                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to find stateprovider {0}", storeName);

                    KSharedPtr<Data::TStore::IStore<LONG64, KString::SPtr>> store = nullptr;
                    store = dynamic_cast<Data::TStore::IStore<LONG64, KString::SPtr>*>(providerSP.RawPtr());

                    Data::TStore::IStoreTransaction<LONG64, KString::SPtr>::SPtr rwtx1 = nullptr;
                    store->CreateOrFindTransaction(*replicatorTransaction, rwtx1);

                    KString::SPtr value;
                    KString::Create(value, GetAllocator(), L"Update");
                    for (int storeTx = 0; storeTx < storeTransactions; ++storeTx)
                    {
                        co_await store->ConditionalUpdateAsync(
                            *rwtx1,
                            storeTx,
                            value,
                            TimeSpan::MaxValue,
                            ktl::CancellationToken::None
                        );
                    }
                }

                co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, TxnAction::Commit);
            }

            co_return;
        };

        workLoad.Append(Ktl::Move(work(prefix)));
    }

    co_await Utilities::TaskUtilities<void>::WhenAll(workLoad);

    co_return;
}

ktl::Awaitable<void> Service::VerifyConcurrentCreationWithTransactionOnStoresAsync()
{
    int noOfStores = 5;
    int storeTransactions = 20;
    std::wstring generalPrefix = L"storetxwithcreate";
    int concurrentExecutions = 5;
    int startPrefixCount = -1;

    KArray<ktl::Awaitable<void>> workLoad(GetAllocator());

    for (int x = 0; x < concurrentExecutions; ++x)
    {
        startPrefixCount++;
        std::wstring prefix = generalPrefix + prefixCharacters[startPrefixCount];
        auto work = [=](std::wstring prefix) -> ktl::Awaitable<void>
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            {
                Transaction::SPtr replicatorTransaction;
                THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
                KFinally([&] { replicatorTransaction->Dispose(); });

                for (int i = 1; i <= noOfStores; ++i)
                {
                    std::wstring storeName = prefix + L"fabric:/Store" + Guid::NewGuid().ToString();

                    TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
                    bool alreadyExist = false;
                    status = co_await TxReplicator->GetOrAddAsync(
                        *replicatorTransaction,
                        &storeName[0],
                        TypeName,
                        providerSP,
                        alreadyExist);
                    THROW_ON_FAILURE(status);
                }

                co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, TxnAction::Commit);
            }

            {
                Transaction::SPtr replicatorTransaction;
                THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
                KFinally([&] { replicatorTransaction->Dispose(); });

                Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
                status = TxReplicator->CreateEnumerator(false, enumerator);
                THROW_ON_FAILURE(status);
                while (enumerator->MoveNext())
                {
                    auto currentKey = enumerator->Current().Key;
                    std::wstring storeName(static_cast<wchar_t *>((*currentKey).Get(KUri::UriFieldType::eRaw)));
                    if (!(storeName.find(prefix) == 0))
                    {
                        continue;
                    }

                    TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
                    status = TxReplicator->Get(*currentKey, providerSP);
                    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
                    {
                        throw ktl::Exception(status);
                    }

                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to find stateprovider");

                    KSharedPtr<Data::TStore::IStore<LONG64, KString::SPtr>> store = nullptr;
                    store = dynamic_cast<Data::TStore::IStore<LONG64, KString::SPtr>*>(providerSP.RawPtr());

                    Data::TStore::IStoreTransaction<LONG64, KString::SPtr>::SPtr rwtx1 = nullptr;
                    store->CreateOrFindTransaction(*replicatorTransaction, rwtx1);

                    KString::SPtr value;
                    KString::Create(value, GetAllocator(), L"add");
                    for (int storeTx = 0; storeTx < storeTransactions; ++storeTx)
                    {
                        KeyValuePair<LONG64, KString::SPtr> temp;
                        bool succeeded = co_await store->ConditionalGetAsync(
                            *rwtx1,
                            storeTx,
                            TimeSpan::MaxValue,
                            temp,
                            ktl::CancellationToken::None
                        );
                        if (!succeeded)
                        {
                            co_await store->AddAsync(
                                *rwtx1,
                                storeTx,
                                value,
                                TimeSpan::MaxValue,
                                ktl::CancellationToken::None
                            );
                        }
                        else
                        {
                            co_await store->ConditionalUpdateAsync(
                                *rwtx1,
                                storeTx,
                                value,
                                TimeSpan::MaxValue,
                                ktl::CancellationToken::None
                            );
                        }
                    }
                }

                co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, TxnAction::Commit);
            }

            {
                Transaction::SPtr replicatorTransaction;
                THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
                KFinally([&] { replicatorTransaction->Dispose(); });

                Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
                status = TxReplicator->CreateEnumerator(false, enumerator);
                THROW_ON_FAILURE(status);
                while (enumerator->MoveNext())
                {
                    auto currentKey = enumerator->Current().Key;
                    std::wstring storeName(static_cast<wchar_t *>((*currentKey).Get(KUri::UriFieldType::eRaw)));
                    if (!(storeName.find(prefix) == 0))
                    {
                        continue;
                    }

                    TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
                    status = TxReplicator->Get(*currentKey, providerSP);
                    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
                    {
                        throw ktl::Exception(status);
                    }

                    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to find stateprovider");


                    KSharedPtr<Data::TStore::IStore<LONG64, KString::SPtr>> store = nullptr;
                    store = dynamic_cast<Data::TStore::IStore<LONG64, KString::SPtr>*>(providerSP.RawPtr());

                    Data::TStore::IStoreTransaction<LONG64, KString::SPtr>::SPtr rwtx1 = nullptr;
                    store->CreateOrFindTransaction(*replicatorTransaction, rwtx1);

                    KString::SPtr value;
                    KString::Create(value, GetAllocator(), L"Update");
                    for (int storeTx = 0; storeTx < storeTransactions; ++storeTx)
                    {
                        co_await store->ConditionalUpdateAsync(
                            *rwtx1,
                            storeTx,
                            value,
                            TimeSpan::MaxValue,
                            ktl::CancellationToken::None
                        );
                    }
                }

                co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, TxnAction::Commit);
            }

            co_return;
        };

        workLoad.Append(Ktl::Move(work(prefix)));
    }

    co_await Utilities::TaskUtilities<void>::WhenAll(workLoad);

    co_return;
}

ktl::Awaitable<void> Service::VerifyStoreCreationWithTransactionsOnStoreAsync()
{
    int noOfStores = 20;
    std::wstring countingStoreName = L"fabric:/CountingStore";
    bool abort = true;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KSharedPtr<Data::TStore::IStore<LONG64, KString::SPtr>> countingStore = nullptr;

    {
        Transaction::SPtr replicatorTransaction;
        THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
        KFinally([&] { replicatorTransaction->Dispose(); });

        TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
        bool alreadyExist = false;
        status = co_await TxReplicator->GetOrAddAsync(
            *replicatorTransaction,
            &countingStoreName[0],
            TypeName,
            providerSP,
            alreadyExist);
        THROW_ON_FAILURE(status);
        co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, TxnAction::Commit);
        countingStore = dynamic_cast<Data::TStore::IStore<LONG64, KString::SPtr>*>(providerSP.RawPtr());
    }

    // step 1: Create stores
    {
        Transaction::SPtr replicatorTransaction;
        THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
        KFinally([&] { replicatorTransaction->Dispose(); });

        Data::TStore::IStoreTransaction<LONG64, KString::SPtr>::SPtr rwtx1 = nullptr;
        countingStore->CreateOrFindTransaction(*replicatorTransaction, rwtx1);
        for (int i = 1; i <= noOfStores; ++i)
        {
            std::wstring storeName = L"fabric:/Store_" + Guid::NewGuid().ToString();
            TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *replicatorTransaction,
                &storeName[0],
                TypeName,
                providerSP,
                alreadyExist);
            THROW_ON_FAILURE(status);
            KString::SPtr value;
            KString::Create(value, GetAllocator(), &storeName[0]);
            co_await countingStore->ConditionalUpdateAsync(
                *rwtx1,
                i,
                value,
                TimeSpan::MaxValue,
                ktl::CancellationToken::None
            );
        }

        Common::Random random = Common::Random();
        abort = random.Next() % 3 == 2;
        co_await SafeTerminateReplicatorTransactionAsync(*replicatorTransaction, abort ? TxnAction::Abort : TxnAction::Commit);
    }

    // step 2: Verify that the stores match
    bool result = co_await DoesStoresMatchTheCountingStore(countingStore);
    ASSERT_IFNOT(result, "Missing stores");

    co_return;
}

ktl::Awaitable<bool> Service::DoesStoresMatchTheCountingStore(KSharedPtr<Data::TStore::IStore<LONG64, KString::SPtr>> store)
{
    Transaction::SPtr replicatorTransaction;
    THROW_ON_FAILURE(TxReplicator->CreateTransaction(replicatorTransaction));
    KFinally([&] { replicatorTransaction->Dispose(); });

    Data::TStore::IStoreTransaction<LONG64, KString::SPtr>::SPtr rtx = nullptr;
    store->CreateOrFindTransaction(*replicatorTransaction, rtx);
    rtx->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

    auto enumerator = co_await store->CreateEnumeratorAsync(*rtx);
    while (co_await enumerator->MoveNextAsync(ktl::CancellationToken::None))
    {
        KeyValuePair<LONG64, KeyValuePair<LONG64, KString::SPtr>> temp = enumerator->GetCurrent();
        KString::SPtr storeName = temp.Value.Value;

        TxnReplicator::IStateProvider2::SPtr providerSP = nullptr;
        NTSTATUS status = TxReplicator->Get(*storeName, providerSP);
        if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
        {
            throw ktl::Exception(status);
        }

        if (!NT_SUCCESS(status))
        {
            co_return false;
        }
    }

    co_return true;
}

ktl::Awaitable<void> Service::WorkLoadTask(
    __in AwaitableCompletionSource<bool> & signalCompletion,
    __in ULONG taskID,
    __in ULONG numOfStateProvider,
    __in AddMode addMode,
    __in GetMode getMode)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
    co_await tempCompletion->GetAwaitable();

    IStateProvider2::SPtr outStateProvider;

    wstring prefixString = wformatString("fabric:/concurrent/{0}", taskID);
    KStringView prefix(&prefixString[0]);

    // Populate state providers name list
    KArray<KUri::CSPtr> nameList(GetAllocator(), numOfStateProvider);
    for (ULONG index = 0; index < numOfStateProvider; ++index)
    {
        KUri::CSPtr nameSPtr = GetStateProviderName(TestCase::ConcurrentSameTask, taskID * numOfStateProvider + index, taskID);
        status = nameList.Append(nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:WorkLoadTask: Append name to list must be successful.",
            replicaId_);
    }

    // Delete state providers if any exists.
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    while (enumerator->MoveNext())
    {
        auto currentKey = enumerator->Current().Key;
        bool result = IsPrefix(*currentKey, prefix);
        if (result)
        {
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await TxReplicator->RemoveAsync(
                *txnSPtr,
                *currentKey);
            THROW_ON_FAILURE(status);

            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    // Add state providers
    for (KUri::CSPtr nameSPtr : nameList)
    {
        Transaction::SPtr txnSPtr;
        THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
        KFinally([&] { txnSPtr->Dispose(); });

        if (addMode == AddMode::AddAsync)
        {
            status = co_await TxReplicator->AddAsync(
                *txnSPtr,
                *nameSPtr,
                TypeName);
            THROW_ON_FAILURE(status);
        }
        else
        {
            ASSERT_IFNOT(
                addMode == AddMode::GetOrAddAsync, 
                "{0}:WorkLoadTask: AddMode must be GetOrAddAsync.",
                replicaId_);
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                *nameSPtr,
                TypeName,
                outStateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            ASSERT_IFNOT(
                alreadyExist == false,
                "{0}:WorkLoadTask: Add a state provider.",
                replicaId_);
        }

        co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
    }

    // Verify all the state providers are added
    for (KUri::CSPtr nameSPtr : nameList)
    {
        if (getMode == GetMode::Get)
        {
            status = TxReplicator->Get(*nameSPtr, outStateProvider);
            if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
            {
                throw ktl::Exception(status);
            }

            ASSERT_IFNOT(
                NT_SUCCESS(status), 
                "{0}:WorkLoadTask: State provider must be added.",
                replicaId_);
        }
        else
        {
            ASSERT_IFNOT(
                getMode == GetMode::GetOrAdd, 
                "{0}:WorkLoadTask: GetMode must be GetOrAddAsync.",
                replicaId_);
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                *nameSPtr,
                TypeName,
                outStateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            ASSERT_IFNOT(
                alreadyExist,
                "{0}:WorkLoadTask: Get a state provider.",
                replicaId_);
            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    co_return;
}

// Concurrently run 3 tasks, one task keeps on doing GetOrAdd, 
// one task keeps on deleting state provider and one task that keeps on getting state provider
// #9735493: Call AddOperation on state provider after we add or add the state provider.
ktl::Awaitable<void> Service::VerifyConcurrentCreationDeletionAndGetAsync()
{
    KStringView prefix = L"fabric:/concurrent/sp";
    ULONG taskCount = 30;

    KUri::CSPtr nameSPtr = GetStateProviderName(TestCase::ConcurrentDifferentTask, 0);

    KArray<Awaitable<void>> taskArray(GetAllocator());

    AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
    NTSTATUS status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    status = taskArray.Append(GetOrAddTask(*signalCompletion, *nameSPtr, taskCount));
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    status = taskArray.Append(DeletingTask(*signalCompletion, *nameSPtr, taskCount));
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    status = taskArray.Append(GetTask(*signalCompletion, *nameSPtr, taskCount));
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    signalCompletion->SetResult(true);

    co_await (Utilities::TaskUtilities<void>::WhenAll(taskArray));

    Trace.WriteInfo(
        TraceComponent, 
        "{0}:VerifyConcurrentCreationDeletionAndGetAsync: All tasks completed, now delete the state provider if it still exists.", 
        replicaId_);

    // Delete the test state provider if it still exists.
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    while (enumerator->MoveNext())
    {
        auto currentKey = enumerator->Current().Key;
        bool result = IsPrefix(*currentKey, prefix);
        if (result)
        {
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await TxReplicator->RemoveAsync(
                *txnSPtr,
                *currentKey);
            THROW_ON_FAILURE(status);

            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    co_return;
}

ktl::Awaitable<void> Service::GetOrAddTask(
    __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
    __in KUriView const & stateProviderName,
    __in ULONG taskCount)
{
    IStateProvider2::SPtr outStateProvider = nullptr;

    AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
    co_await tempCompletion->GetAwaitable();

    for (ULONG i = 0; i < taskCount; ++i)
    {
        // GetOrAdd state provider with the name specified
        {
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            NTSTATUS status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                stateProviderName,
                TypeName,
                outStateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    co_return;
}

ktl::Awaitable<void> Service::DeletingTask(
    __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
    __in KUriView const & stateProviderName,
    __in ULONG taskCount)
{
    AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
    co_await tempCompletion->GetAwaitable();

    for (ULONG i = 0; i < taskCount; ++i)
    {
        // Delete the state provider if it exists.
        {
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });

            try
            {
                NTSTATUS status = co_await TxReplicator->RemoveAsync(
                    *txnSPtr,
                    stateProviderName);
                THROW_ON_FAILURE(status);

                co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
            }
            catch (Exception & exception)
            {
                if (exception.GetStatus() == SF_STATUS_NOT_PRIMARY)
                {
                    throw;
                }

                // The remove call may failed in the situation that two Remove calls happened in a row
                // Then Remove call will throw not exist exception.
                ASSERT_IFNOT(
                    exception.GetStatus() == SF_STATUS_NAME_DOES_NOT_EXIST,
                    "{0}:DeletingTask: RemoveAsync call throw exception {1}, the expected exception is {2}.",
                    replicaId_,
                    exception.GetStatus(),
                    SF_STATUS_NAME_DOES_NOT_EXIST);
            }
        }
    }

    co_return;
}

ktl::Awaitable<void> Service::GetTask(
    __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
    __in KUriView const & stateProviderName,
    __in ULONG taskCount)
{
    IStateProvider2::SPtr outStateProvider = nullptr;

    AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
    co_await tempCompletion->GetAwaitable();

    for (ULONG i = 0; i < taskCount; ++i)
    {
        // The Get call may return STATUS_SUCCESS or SF_STATUS_NAME_DOES_NOT_EXIST based on Add/Remove sequence. So we don't care the result.
        // Need to get the result when we add AddOpertion call to state provider.
        NTSTATUS status = TxReplicator->Get(stateProviderName, outStateProvider);
        if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
        {
            throw ktl::Exception(status);
        }
    }

    co_return;
}

// For different test case, we construct different state provider name
// Serially test name example: "fabric:/serial/sp/0", the number after sp stands 
//     for the StateProvider Id.
// Concurrently test name example: "fabric:/concurrent/0/sp/1", the first number
//     indicates the task id, the second number is the StateProvider Id.
KUri::CSPtr Service::GetStateProviderName(
    __in TestCase testCase, 
    __in ULONG id, 
    __in ULONG taskId)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KUri::CSPtr nameSPtr = nullptr;

    switch (testCase)
    {
    case TestCase::Serially:
    {
        wstring name = wformatString("fabric:/serial/sp/{0}", id);
        status = KUri::Create(&name[0], GetAllocator(), nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:GetStateProviderName: Failed to Create KUri in Serially case.",
            replicaId_);
        break;
    }
    case TestCase::ConcurrentSameTask:
    {
        wstring name = wformatString("fabric:/concurrent/{0}/sp/{1}", taskId, id);
        status = KUri::Create(&name[0], GetAllocator(), nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:GetStateProviderName: Failed to Create KUri in ConcurrentSameTask case.",
            replicaId_);
        break;
    }
    case TestCase::ConcurrentDifferentTask:
    {
        wstring name = wformatString("fabric:/concurrent/sp/{0}", id);
        status = KUri::Create(&name[0], GetAllocator(), nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:GetStateProviderName: Failed to Create KUri in ConcurrentDifferentTask case.",
            replicaId_);
        break;
    }
    default:
    {
        ASSERT_IFNOT(
            false, 
            "{0}:GetStateProviderName: Should not reach here.",
            replicaId_);
    }
    }

    return nameSPtr;
}

ktl::Awaitable<void> Service::SafeTerminateReplicatorTransactionAsync(
    __in TxnReplicator::Transaction & replicatorTransaction,
    __in TxnAction txnAction)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    while (true)
    {
        bool doneCommitTx = false;
        try
        {
            if (txnAction == TxnAction::Commit)
            {
                THROW_ON_FAILURE(co_await replicatorTransaction.CommitAsync());
            }
            else
            {
                replicatorTransaction.Abort();
            }

            doneCommitTx = true;
        }
        catch (ktl::Exception & e)
        {
            Trace.WriteError(TraceComponent, "{0}:SafeTerminateReplicatorTransactionAsync: Status: {1}.", replicaId_, e.GetStatus());
            co_return;
        }

        if (doneCommitTx)
        {
            break;
        }

        status = co_await KTimer::StartTimerAsync(GetAllocator(), 'tsLD', 100, nullptr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:SafeTerminateReplicatorTransactionAsync: Failed to start timer",
            replicaId_);
    }

    co_return;
}

void Service::VerifyEnumeration(
    __in TestCase testCase,
    __in ULONG expectedCount)
{
    KStringView prefix = testCase == TestCase::Serially ? L"fabric:/serial" : L"fabric:/concurrent";

    ULONG count = 0;
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    NTSTATUS status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    while (enumerator->MoveNext())
    {
        auto currentKey = enumerator->Current().Key;
        bool result = IsPrefix(*currentKey, prefix);
        if (result)
        {
            ++count;
        }
    }

    ASSERT_IFNOT(
        count == expectedCount,
        "{0}:VerifyEnumeration: Current count is {1}, and expected count is {2}.",
        replicaId_,
        count,
        expectedCount);
}

bool Service::IsPrefix(
    __in KUriView const & name,
    __in KStringView & prefix)
{
    // TODO: KDynUri creation with const KUriView
    KDynUri candidate(static_cast<KUriView>(name), GetAllocator());
    KDynUri tempPrefix(prefix, GetAllocator());

    return tempPrefix.IsPrefixFor(candidate) == TRUE ? true : false;
}

KAllocator& Service::GetAllocator()
{
    return this->KtlSystemValue->PagedAllocator();
}

ComPointer<IFabricStateProvider2Factory> Service::GetStateProviderFactory()
{
    return TestComStateProvider2Factory::Create(GetAllocator());
}

ComPointer<IFabricDataLossHandler> Service::GetDataLossHandler()
{
    // False indicates no state change.
    return TxnReplicator::TestCommon::TestComProxyDataLossHandler::Create(GetAllocator());
}

Service::Service(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : TestableServiceBase(partitionId, replicaId, root)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , doWorkInProgress_(false)
{
}
