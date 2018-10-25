// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'enTP'

inline bool SingleElementBufferEquals(KBuffer::SPtr & one, KBuffer::SPtr & two)
{
    auto oneNum = (ULONG32 *)one->GetBuffer();
    auto twoNum = (ULONG32 *)two->GetBuffer();

    return *oneNum == *twoNum;
}

namespace TStoreTests
{
    using namespace ktl;

    class Enumeration3ReplicaTest : public TStoreTestBase<KString::SPtr, KBuffer::SPtr, KStringComparer, StringStateSerializer, KBufferSerializer>
    {
    public:
        Enumeration3ReplicaTest()
        {
            Setup(3);
            Store->EnableBackgroundConsolidation = false;
        }

        ~Enumeration3ReplicaTest()
        {
            Cleanup();
        }

        Common::Random GetRandom()
        {
            auto seed = Common::Stopwatch::Now().Ticks;
            Common::Random random(static_cast<int>(seed));
            cout << "Random seed (use this seed to reproduce test failures): " << seed << endl;
            return random;
        }

        KString::SPtr CreateString(ULONG32 seed)
        {
            KString::SPtr key;

            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(seed);
            while (seedString.length() < 6) // zero pad the number to make it easier to compare
            {
                seedString = zeroString + seedString;
            }

            wstring str = wstring(L"test_key_") + seedString;
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            Diagnostics::Validate(status);
            return key;
        }

        KBuffer::SPtr CreateBuffer(__in ULONG32 valueToStore)
        {
            KBuffer::SPtr result;
            auto status = KBuffer::Create(sizeof(ULONG32), result, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ULONG32* buffer = (ULONG32 *)result->GetBuffer();
            *buffer = valueToStore;

            return result;
        }

        KSharedArray<KString::SPtr>::SPtr CreateStringSharedArray()
        {
            KSharedArray<KString::SPtr>::SPtr resultSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
            CODING_ERROR_ASSERT(resultSPtr != nullptr);
            return resultSPtr;
        }

        KSharedArray<KBuffer::SPtr>::SPtr CreateBufferSharedArray()
        {
            KSharedArray<KBuffer::SPtr>::SPtr resultSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KBuffer::SPtr>();
            CODING_ERROR_ASSERT(resultSPtr != nullptr);
            return resultSPtr;
        }

        void PopulateExpectedOutputs(
            __inout KSharedArray<ULONG32>::SPtr & addedKeysSPtr,
            __out KSharedArray<KString::SPtr> & expectedKeys,
            __out KSharedArray<KBuffer::SPtr> & expectedValues,
            __in bool useFirstKey=false,
            __in ULONG32 firstKey=-1)
        {
            ULONG32Comparer::SPtr ulong32ComparerSPtr;
            ULONG32Comparer::Create(GetAllocator(), ulong32ComparerSPtr);
            IComparer<ULONG32>::SPtr comparerSPtr = static_cast<IComparer<ULONG32> *>(ulong32ComparerSPtr.RawPtr());

            Sorter<ULONG32>::QuickSort(true, *comparerSPtr, addedKeysSPtr);

            for (ULONG32 i = 0; i < addedKeysSPtr->Count(); i++)
            {
                ULONG32 item = (*addedKeysSPtr)[i];
                if (!useFirstKey || item >= firstKey)
                {
                    expectedKeys.Append(CreateString(item));
                    expectedValues.Append(CreateBuffer(item));
                }
            }
        }

        VersionedItem<KBuffer::SPtr>::SPtr CreateVersionedItem(__in LONG64 sequenceNumber, __in KBuffer::SPtr & value)
        {
            InsertedVersionedItem<KBuffer::SPtr>::SPtr insertedItem = nullptr;
            auto status = InsertedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), insertedItem);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            insertedItem->InitializeOnApply(sequenceNumber, value);

            return insertedItem.DownCast<VersionedItem<KBuffer::SPtr>>();
        }

        void VerifySortedEnumerable(
            __in IEnumerator<KString::SPtr> & enumerable,
            __in KSharedArray<KString::SPtr> & expectedKeys)
        {
            auto keyComparer = Store->KeyComparerSPtr;
            KSharedPtr<IEnumerator<KString::SPtr>> enumerableSPtr = &enumerable;

            if (enumerableSPtr->MoveNext() == false)
            {
                ASSERT_IFNOT(expectedKeys.Count() == 0, "Expected count was {0} but got no items", expectedKeys.Count());
                return;
            }

            KString::SPtr previous = enumerableSPtr->Current();
            ULONG count = 1;

            while (enumerableSPtr->MoveNext())
            {
                KString::SPtr current = enumerableSPtr->Current();
                KString::SPtr expectedKey = expectedKeys[count];
                ASSERT_IFNOT(keyComparer->Compare(expectedKey, current) == 0, "Unexpected key");
                ASSERT_IFNOT(keyComparer->Compare(previous, current) <= 0, "Expected previous to be less than current");
                previous = current;
                count++;
            }

            ASSERT_IFNOT(expectedKeys.Count() == count, "Expected count to be {0} but got {1}", expectedKeys.Count(), count);
        }

        ktl::Awaitable<void> VerifySortedEnumerableAsync(
            __in IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>> & enumerable,
            __in KSharedArray<KString::SPtr> & expectedKeys,
            __in KSharedArray<KBuffer::SPtr> & expectedValues)
        {
            CODING_ERROR_ASSERT(expectedKeys.Count() == expectedValues.Count());

            auto keyComparer = Store->KeyComparerSPtr;
            KSharedPtr<IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>> enumerableSPtr = &enumerable;

            if (co_await enumerableSPtr->MoveNextAsync(CancellationToken::None) == false)
            {
                ASSERT_IFNOT(expectedKeys.Count() == 0, "Expected count was {0} but got no items", expectedKeys.Count());
                co_return;
            }

            KString::SPtr previousKey = enumerableSPtr->GetCurrent().Key;
            ULONG count = 1;

            while (co_await enumerableSPtr->MoveNextAsync(CancellationToken::None) != false)
            {
                KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>> current = enumerableSPtr->GetCurrent();

                KBuffer::SPtr currentValue = current.Value.Value;
                KString::SPtr expectedKey = expectedKeys[count];
                KBuffer::SPtr expectedValue = expectedValues[count];

                ASSERT_IFNOT(keyComparer->Compare(expectedKey, current.Key) == 0, "Unexpected key");
                ASSERT_IFNOT(keyComparer->Compare(previousKey, current.Key) <= 0, "Expected previous to be less than current");
                ASSERT_IFNOT(SingleElementBufferEquals(currentValue, expectedValue), "Unexpected value");
                previousKey = current.Key;
                count++;
            }

            ASSERT_IFNOT(expectedKeys.Count() == count, "Expected count to be {0} but got {1}", expectedKeys.Count(), count);

            co_return;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

#pragma region test functions
    public:
        ktl::Awaitable<void> Enumerate_DifferentialState_ShouldSucceed_Test()
        {
            auto random = GetRandom();
            LONG32 numItems = 250;

            KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();

            for (LONG32 i = 0; i < numItems; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    auto newKey = random.Next(99999);
                    KeyValuePair<LONG64, KBuffer::SPtr> value;
                    bool keyExists = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None);
                    if (keyExists)
                    {
                        co_await txn->AbortAsync();
                        continue;
                    }

                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None);

                    addedKeys->Append(newKey);
                    co_await txn->CommitAsync();
                }
            }

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

            // Test
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];

                {
                    // Keys only enumeration
                    auto txn = CreateWriteTransaction(*store);
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    auto enumerator = co_await store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr);
                    VerifySortedEnumerable(*enumerator, *expectedKeys);
                    co_await txn->AbortAsync();
                }

                {
                    // Keys and Values enumeration
                    auto txn = CreateWriteTransaction(*store);
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    auto enumerator = co_await store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr);
                    co_await VerifySortedEnumerableAsync(*enumerator, *expectedKeys, *expectedValues);
                    co_await txn->AbortAsync();
                }
            }
            co_return;
        }

        ktl::Awaitable<void> Enumerate_Differential_DifferentVersionedItemTypesAndOrders_ShouldSucceed_Test()
        {
            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();

            // Add Update
            expectedKeys->Append(CreateString(0));
            expectedValues->Append(CreateBuffer(1));

            // Add Remove: 1

            // Setup Differential state
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(0), DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(1), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(1), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Test
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
            
                {
                    // Keys only enumeration
                    auto txn = CreateWriteTransaction(*store);
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    auto enumerator = co_await store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr);
                    VerifySortedEnumerable(*enumerator, *expectedKeys);
                    co_await txn->AbortAsync();
                }

                {
                    // Keys and Values enumeration
                    auto txn = CreateWriteTransaction(*store);
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    auto enumerator = co_await store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr);
                    co_await VerifySortedEnumerableAsync(*enumerator, *expectedKeys, *expectedValues);
                    co_await txn->AbortAsync();
                }
            }
            co_return;
        }

        ktl::Awaitable<void> Enumerate_DifferentialState_DeltaContainsDeletedVersion_ShouldSucceed_Test()
        {
            KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();
            for (ULONG32 i = 0; i < 10; i++)
            {
                addedKeys->Append(i);
            }
        
            // Add
            for (ULONG32 i = 0; i < 10; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync();

            // Remove
            for (ULONG32 i = 0; i < 10; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(i), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync();

            // Add again
            for (ULONG32 i = 0; i < 10; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync();

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

            // Test
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];

                {
                    // Keys only enumeration
                    auto txn = CreateWriteTransaction(*store);
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    auto enumerator = co_await store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr);
                    VerifySortedEnumerable(*enumerator, *expectedKeys);
                    co_await txn->AbortAsync();
                }

                {
                    // Keys and Values enumeration
                    auto txn = CreateWriteTransaction(*store);
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    auto enumerator = co_await store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr);
                    co_await VerifySortedEnumerableAsync(*enumerator, *expectedKeys, *expectedValues);
                    co_await txn->AbortAsync();
                }
            }
            co_return;
        }

        ktl::Awaitable<void> Enumerate_DifferentialState_AddList_ShouldSucceed_Test()
        {
            ULONG32 invalidValue = MAXULONG32;
            ULONG32 validValue = MAXULONG32 >> 1;

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
        
            expectedKeys->Append(CreateString(0));
            expectedKeys->Append(CreateString(1));
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            expectedValues->Append(CreateBuffer(validValue));
            expectedValues->Append(CreateBuffer(validValue));
            expectedValues->Append(CreateBuffer(validValue));
            expectedValues->Append(CreateBuffer(validValue));
            expectedValues->Append(CreateBuffer(validValue));

            // Prepare Conslidated
            for (ULONG32 i = 3; i < 8; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync();

            for (ULONG32 i = 3; i < 6; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(i), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync();
        
            // Prepare differential
            {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(validValue), DefaultTimeout, CancellationToken::None);
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(invalidValue), DefaultTimeout, CancellationToken::None);
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(2), CreateBuffer(invalidValue), DefaultTimeout, CancellationToken::None);
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(validValue), DefaultTimeout, CancellationToken::None);
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(invalidValue), DefaultTimeout, CancellationToken::None);
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(5), CreateBuffer(invalidValue), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
            }

            {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(validValue), DefaultTimeout, CancellationToken::None);
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(2), DefaultTimeout, CancellationToken::None);
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(validValue), DefaultTimeout, CancellationToken::None);
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(5), DefaultTimeout, CancellationToken::None);
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(validValue), DefaultTimeout, CancellationToken::None);
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(7), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
            }
        
            // Test
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];

                {
                    // Keys only enumeration
                    auto txn = CreateWriteTransaction(*store);
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    auto enumerator = co_await store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr);
                    VerifySortedEnumerable(*enumerator, *expectedKeys);
                    co_await txn->AbortAsync();
                }

                {
                    // Keys and Values enumeration
                    auto txn = CreateWriteTransaction(*store);
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    auto enumerator = co_await store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr);
                    co_await VerifySortedEnumerableAsync(*enumerator, *expectedKeys, *expectedValues);
                    co_await txn->AbortAsync();
                }
            }
            co_return;
        }

        ktl::Awaitable<void> Enumerate_SnapshotComponent_OneItemPerComponent_ShouldSucceed_Test()
        {
            KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();
        
            // Prepare consolidation
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(0), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();

                addedKeys->Append(0);
            }

            co_await CheckpointAsync();

            // Prepare differential
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();

                addedKeys->Append(1);
            }

            // Prepare transactions
            auto primaryStore = Store;
            auto secondaryOne = (*Stores)[1];
            auto secondaryTwo = (*Stores)[2];

            auto primarySnapshotTxn = CreateWriteTransaction(*primaryStore);
            primarySnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto secondaryOneSnapshotTxn = CreateWriteTransaction(*secondaryOne);
            secondaryOneSnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto secondaryTwoSnapshotTxn = CreateWriteTransaction(*secondaryTwo);
            secondaryTwoSnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        
            // Read to take snapshot
            co_await VerifyKeyExistsAsync(*primaryStore, *primarySnapshotTxn->StoreTransactionSPtr, CreateString(0), nullptr, CreateBuffer(0), SingleElementBufferEquals);
            co_await VerifyKeyExistsAsync(*secondaryOne, *secondaryOneSnapshotTxn->StoreTransactionSPtr, CreateString(0), nullptr, CreateBuffer(0), SingleElementBufferEquals);
            co_await VerifyKeyExistsAsync(*secondaryTwo, *secondaryTwoSnapshotTxn->StoreTransactionSPtr, CreateString(0), nullptr, CreateBuffer(0), SingleElementBufferEquals);

            // Move items in differential to LC-1
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(2), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Move items in differential to snapshot
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(0), DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(1), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Move removes in differential to consolidated
            co_await CheckpointAsync();

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

            // Test Primary
            {
                auto keyEnumerator = co_await primaryStore->CreateKeyEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await primaryStore->CreateEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Test Secondary One
            {
                auto keyEnumerator = co_await secondaryOne->CreateKeyEnumeratorAsync(*secondaryOneSnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await secondaryOne->CreateEnumeratorAsync(*secondaryOneSnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Test Secondary Two
            {
                auto keyEnumerator = co_await secondaryTwo->CreateKeyEnumeratorAsync(*secondaryTwoSnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await secondaryTwo->CreateEnumeratorAsync(*secondaryTwoSnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Close transactions
            co_await primarySnapshotTxn->AbortAsync();
            co_await secondaryOneSnapshotTxn->AbortAsync();
            co_await secondaryTwoSnapshotTxn->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> Enumerate_SnapshotComponent_ManyItemsPerComponent_ShouldSucceed_Test()
        {
            ULONG32 itemsPerComponent = 100;
            KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();

            // Prepare consolidation
            for (ULONG32 i = 0; i < itemsPerComponent; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                    addedKeys->Append(i);
                }
            }

            co_await CheckpointAsync();

            // Prepare differential
            for (ULONG32 i = itemsPerComponent; i < 2 * itemsPerComponent; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                    addedKeys->Append(i);
                }
            }

            // Prepare transactions
            auto primaryStore = Store;
            auto secondaryOne = (*Stores)[1];
            auto secondaryTwo = (*Stores)[2];

            auto primarySnapshotTxn = CreateWriteTransaction(*primaryStore);
            primarySnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto secondaryOneSnapshotTxn = CreateWriteTransaction(*secondaryOne);
            secondaryOneSnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto secondaryTwoSnapshotTxn = CreateWriteTransaction(*secondaryTwo);
            secondaryTwoSnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        
            // Read to take snapshot
            co_await VerifyKeyExistsAsync(*primaryStore, *primarySnapshotTxn->StoreTransactionSPtr, CreateString(0), nullptr, CreateBuffer(0), SingleElementBufferEquals);
            co_await VerifyKeyExistsAsync(*secondaryOne, *secondaryOneSnapshotTxn->StoreTransactionSPtr, CreateString(0), nullptr, CreateBuffer(0), SingleElementBufferEquals);
            co_await VerifyKeyExistsAsync(*secondaryTwo, *secondaryTwoSnapshotTxn->StoreTransactionSPtr, CreateString(0), nullptr, CreateBuffer(0), SingleElementBufferEquals);

            // Move items in differential to LC-1
            for (ULONG32 i = itemsPerComponent; i < 2 * itemsPerComponent; i++)
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i+1), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Move items in differential to snapshot
            {
                auto txn = CreateWriteTransaction();
                for (ULONG32 i = 0; i < 2 * itemsPerComponent; i++)
                {
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(i), DefaultTimeout, CancellationToken::None);
                }
                co_await txn->CommitAsync();
            }

            // Move removes in differential to consolidated
            co_await CheckpointAsync();

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

            // Test Primary
            {
                auto keyEnumerator = co_await primaryStore->CreateKeyEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await primaryStore->CreateEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Test Secondary One
            {
                auto keyEnumerator = co_await secondaryOne->CreateKeyEnumeratorAsync(*secondaryOneSnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await secondaryOne->CreateEnumeratorAsync(*secondaryOneSnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Test Secondary Two
            {
                auto keyEnumerator = co_await secondaryTwo->CreateKeyEnumeratorAsync(*secondaryTwoSnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await secondaryTwo->CreateEnumeratorAsync(*secondaryTwoSnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Close transactions
            co_await primarySnapshotTxn->AbortAsync();
            co_await secondaryOneSnapshotTxn->AbortAsync();
            co_await secondaryTwoSnapshotTxn->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> Enumerate_SnapshotComponent_RandomlyPopulated_ShouldSucceed_Test()
        {
            auto random = GetRandom();
            LONG32 itemsPerComponent = 300;
            KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();

            // Populate differential
            for (LONG32 i = 0; i < itemsPerComponent; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    auto newKey = random.Next(99999);
                    KeyValuePair<LONG64, KBuffer::SPtr> value;
                    bool keyExists = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None);
                    if (keyExists)
                    {
                        i--;
                        co_await txn->AbortAsync();
                        continue;
                    }

                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None);

                    addedKeys->Append(newKey);
                    co_await txn->CommitAsync();
                }
            }

            // Prepare transactions
            auto primaryStore = Store;
            auto secondaryOne = (*Stores)[1];
            auto secondaryTwo = (*Stores)[2];

            auto primarySnapshotTxn = CreateWriteTransaction(*primaryStore);
            primarySnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto secondaryOneSnapshotTxn = CreateWriteTransaction(*secondaryOne);
            secondaryOneSnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto secondaryTwoSnapshotTxn = CreateWriteTransaction(*secondaryTwo);
            secondaryTwoSnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Read to take snapshot
            co_await VerifyKeyExistsAsync(*primaryStore, *primarySnapshotTxn->StoreTransactionSPtr, CreateString((*addedKeys)[0]), nullptr, CreateBuffer((*addedKeys)[0]), SingleElementBufferEquals);
            co_await VerifyKeyExistsAsync(*secondaryOne, *secondaryOneSnapshotTxn->StoreTransactionSPtr, CreateString((*addedKeys)[0]), nullptr, CreateBuffer((*addedKeys)[0]), SingleElementBufferEquals);
            co_await VerifyKeyExistsAsync(*secondaryTwo, *secondaryTwoSnapshotTxn->StoreTransactionSPtr, CreateString((*addedKeys)[0]), nullptr, CreateBuffer((*addedKeys)[0]), SingleElementBufferEquals);

            // Move items in differential to LC-1
            for (ULONG32 i = 0; i < addedKeys->Count(); i++)
            {
                auto key = (*addedKeys)[i];

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(key), CreateBuffer(key + 1), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            // Move items in differential to snapshot
            {
                auto txn = CreateWriteTransaction();
                for (ULONG32 i = 0; i < addedKeys->Count(); i++)
                {
                    auto key = (*addedKeys)[i];
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(key), DefaultTimeout, CancellationToken::None);
                }
                co_await txn->CommitAsync();
            }

            // Move removes in differential to consolidated
            co_await CheckpointAsync();

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

            // Test Primary
            {
                auto keyEnumerator = co_await primaryStore->CreateKeyEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await primaryStore->CreateEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Test Secondary One
            {
                auto keyEnumerator = co_await secondaryOne->CreateKeyEnumeratorAsync(*secondaryOneSnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await secondaryOne->CreateEnumeratorAsync(*secondaryOneSnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Test Secondary Two
            {
                auto keyEnumerator = co_await secondaryTwo->CreateKeyEnumeratorAsync(*secondaryTwoSnapshotTxn->StoreTransactionSPtr);
                auto keyValueEnumerator = co_await secondaryTwo->CreateEnumeratorAsync(*secondaryTwoSnapshotTxn->StoreTransactionSPtr);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Close transactions
            co_await primarySnapshotTxn->AbortAsync();
            co_await secondaryOneSnapshotTxn->AbortAsync();
            co_await secondaryTwoSnapshotTxn->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> Enumerate_SnapshotComponent_RandomlyPopulated_RandomFirstKey_ShouldSucceed_Test()
        {
            auto random = GetRandom();
            LONG32 itemsPerComponent = 300;
            KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();

            // Populate differential
            for (LONG32 i = 0; i < itemsPerComponent; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    auto newKey = random.Next(99999);
                    KeyValuePair<LONG64, KBuffer::SPtr> value;
                    bool keyExists = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None);
                    if (keyExists)
                    {
                        i--;
                        co_await txn->AbortAsync();
                        continue;
                    }

                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None);

                    addedKeys->Append(newKey);
                    co_await txn->CommitAsync();
                }
            }

            // Prepare transactions
            auto primaryStore = Store;
            auto secondaryOne = (*Stores)[1];
            auto secondaryTwo = (*Stores)[2];

            auto primarySnapshotTxn = CreateWriteTransaction(*primaryStore);
            primarySnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto secondaryOneSnapshotTxn = CreateWriteTransaction(*secondaryOne);
            secondaryOneSnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto secondaryTwoSnapshotTxn = CreateWriteTransaction(*secondaryTwo);
            secondaryTwoSnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Read to take snapshot
            co_await VerifyKeyExistsAsync(*primaryStore, *primarySnapshotTxn->StoreTransactionSPtr, CreateString((*addedKeys)[0]), nullptr, CreateBuffer((*addedKeys)[0]), SingleElementBufferEquals);
            co_await VerifyKeyExistsAsync(*secondaryOne, *secondaryOneSnapshotTxn->StoreTransactionSPtr, CreateString((*addedKeys)[0]), nullptr, CreateBuffer((*addedKeys)[0]), SingleElementBufferEquals);
            co_await VerifyKeyExistsAsync(*secondaryTwo, *secondaryTwoSnapshotTxn->StoreTransactionSPtr, CreateString((*addedKeys)[0]), nullptr, CreateBuffer((*addedKeys)[0]), SingleElementBufferEquals);

            // Move items in differential to LC-1
            for (ULONG32 i = 0; i < addedKeys->Count(); i++)
            {
                auto key = (*addedKeys)[i];

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(key), CreateBuffer(key + 1), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            // Move items in differential to snapshot
            {
                auto txn = CreateWriteTransaction();
                for (ULONG32 i = 0; i < addedKeys->Count(); i++)
                {
                    auto key = (*addedKeys)[i];
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(key), DefaultTimeout, CancellationToken::None);
                }
                co_await txn->CommitAsync();
            }

            // Move removes in differential to consolidated
            co_await CheckpointAsync();

            auto firstKeySeed = random.Next(99999);
            auto firstKey = CreateString(firstKeySeed);

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues, true, firstKeySeed);


            // Test Primary
            {
                auto keyEnumerator = co_await primaryStore->CreateKeyEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr, firstKey);
                auto keyValueEnumerator = co_await primaryStore->CreateEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr, firstKey, true, firstKey, false);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Test Secondary One
            {
                auto keyEnumerator = co_await secondaryOne->CreateKeyEnumeratorAsync(*secondaryOneSnapshotTxn->StoreTransactionSPtr, firstKey);
                auto keyValueEnumerator = co_await secondaryOne->CreateEnumeratorAsync(*secondaryOneSnapshotTxn->StoreTransactionSPtr, firstKey, true, firstKey, false);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Test Secondary Two
            {
                auto keyEnumerator = co_await secondaryTwo->CreateKeyEnumeratorAsync(*secondaryTwoSnapshotTxn->StoreTransactionSPtr, firstKey);
                auto keyValueEnumerator = co_await secondaryTwo->CreateEnumeratorAsync(*secondaryTwoSnapshotTxn->StoreTransactionSPtr, firstKey, true, firstKey, false);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Close transactions
            co_await primarySnapshotTxn->AbortAsync();
            co_await secondaryOneSnapshotTxn->AbortAsync();
            co_await secondaryTwoSnapshotTxn->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> Enumerate_SnapshotComponent_RandomlyPopulated_RandomFirstKey_WithWriteset_ShouldSucceed_Test()
        {
            auto random = GetRandom();
            LONG32 itemsPerComponent = 300;
            KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();

            // Populate differential
            for (LONG32 i = 0; i < itemsPerComponent; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    auto newKey = random.Next(99999);
                    KeyValuePair<LONG64, KBuffer::SPtr> value;
                    bool keyExists = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None);
                    if (keyExists)
                    {
                        i--;
                        co_await txn->AbortAsync();
                        continue;
                    }

                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None);

                    addedKeys->Append(newKey);
                    co_await txn->CommitAsync();
                }
            }

            // Prepare transactions
            auto primaryStore = Store;

            auto primarySnapshotTxn = CreateWriteTransaction(*primaryStore);
            primarySnapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Read to take snapshot
            co_await VerifyKeyExistsAsync(*primaryStore, *primarySnapshotTxn->StoreTransactionSPtr, CreateString((*addedKeys)[0]), nullptr, CreateBuffer((*addedKeys)[0]), SingleElementBufferEquals);

            // Move items in differential to LC-1
            for (ULONG32 i = 0; i < addedKeys->Count(); i++)
            {
                auto key = (*addedKeys)[i];

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(key), CreateBuffer(key + 1), DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            // Move items in differential to snapshot
            {
                auto txn = CreateWriteTransaction();
                for (ULONG32 i = 0; i < addedKeys->Count(); i++)
                {
                    auto key = (*addedKeys)[i];
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(key), DefaultTimeout, CancellationToken::None);
                }
                co_await txn->CommitAsync();
            }

            // Add to writeset
            for (LONG32 i = 0; i < itemsPerComponent; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    auto newKey = random.Next(99999);
                    KeyValuePair<LONG64, KBuffer::SPtr> value;
                    bool keyExists = co_await Store->ConditionalGetAsync(*primarySnapshotTxn->StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None);
                    if (keyExists)
                    {
                        i--;
                        continue;
                    }

                    co_await Store->AddAsync(*primarySnapshotTxn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None);

                    addedKeys->Append(newKey);
                }
            }

            // Move removes in differential to consolidated
            co_await CheckpointAsync();

            auto firstKeySeed = random.Next(99999);
            auto firstKey = CreateString(firstKeySeed);

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues, true, firstKeySeed);


            // Test Primary
            {
                auto keyEnumerator = co_await primaryStore->CreateKeyEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr, firstKey);
                auto keyValueEnumerator = co_await primaryStore->CreateEnumeratorAsync(*primarySnapshotTxn->StoreTransactionSPtr, firstKey, true, firstKey, false);
                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                co_await VerifySortedEnumerableAsync(*keyValueEnumerator, *expectedKeys, *expectedValues);
            }

            // Close transactions
            co_await primarySnapshotTxn->AbortAsync();
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(Enumeration3ReplicaTestSuite, Enumeration3ReplicaTest)

    BOOST_AUTO_TEST_CASE(Enumerate_DifferentialState_ShouldSucceed)
    {
        SyncAwait(Enumerate_DifferentialState_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_Differential_DifferentVersionedItemTypesAndOrders_ShouldSucceed)
    {
        SyncAwait(Enumerate_Differential_DifferentVersionedItemTypesAndOrders_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_DifferentialState_DeltaContainsDeletedVersion_ShouldSucceed)
    {
        SyncAwait(Enumerate_DifferentialState_DeltaContainsDeletedVersion_ShouldSucceed_Test());
    }


    // Test focuses on how addList is populated in reaction to current consolidated..
    // Tested Cases
    // # || Differential || Consolidated -> AddList
    // 0    A               {}              T
    // 1    U, A            {}              T
    // 2    R, A            {}              T
    // 3    A               R A/U           T
    // 4    U, A            R A/U           T
    // 5    R, A            R A/U           T
    // 6    U               A/U             F
    // 7    R               A/U             F
    BOOST_AUTO_TEST_CASE(Enumerate_DifferentialState_AddList_ShouldSucceed)
    {
        SyncAwait(Enumerate_DifferentialState_AddList_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_SnapshotComponent_OneItemPerComponent_ShouldSucceed)
    {
        SyncAwait(Enumerate_SnapshotComponent_OneItemPerComponent_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_SnapshotComponent_ManyItemsPerComponent_ShouldSucceed)
    {
        SyncAwait(Enumerate_SnapshotComponent_ManyItemsPerComponent_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_SnapshotComponent_RandomlyPopulated_ShouldSucceed)
    {
        SyncAwait(Enumerate_SnapshotComponent_RandomlyPopulated_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_SnapshotComponent_RandomlyPopulated_RandomFirstKey_ShouldSucceed)
    {
        SyncAwait(Enumerate_SnapshotComponent_RandomlyPopulated_RandomFirstKey_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_SnapshotComponent_RandomlyPopulated_RandomFirstKey_WithWriteset_ShouldSucceed)
    {
        SyncAwait(Enumerate_SnapshotComponent_RandomlyPopulated_RandomFirstKey_WithWriteset_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
