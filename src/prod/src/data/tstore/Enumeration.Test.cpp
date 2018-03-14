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

    class EnumerationTest : public TStoreTestBase<KString::SPtr, KBuffer::SPtr, KStringComparer, StringStateSerializer, KBufferSerializer>
    {
    public:
        EnumerationTest()
        {
            Setup(1);
            Store->EnableBackgroundConsolidation = true;
        }
        
        ~EnumerationTest()
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
            while (seedString.length() != 5) // zero pad the number to make it easier to compare
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

            ULONG32* buffer = (ULONG32 *) result->GetBuffer();
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

        ULONG32 PopulateWithRandomNewKey(Common::Random & random)
        {
            auto txn = CreateWriteTransaction();
            auto newKey = PopulateWithRandomNewKey(*txn, random);
            SyncAwait(txn->CommitAsync());
            return newKey;
        }

        ULONG32 PopulateWithRandomNewKey(WriteTransaction<KString::SPtr, KBuffer::SPtr> & transaction, Common::Random & random)
        {
            while (true)
            {
                auto newKey = random.Next(99999);
                KeyValuePair<LONG64, KBuffer::SPtr> value;
                bool keyExists = SyncAwait(Store->ConditionalGetAsync(*transaction.StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None));
                if (keyExists)
                {
                    continue;
                }

                SyncAwait(Store->AddAsync(*transaction.StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None));
                return newKey;
            }
        }

        void PopulateExpectedOutputs(
            __inout KSharedArray<ULONG32>::SPtr & addedKeysSPtr, 
            __out KSharedArray<KString::SPtr> & expectedKeys, 
            __out KSharedArray<KBuffer::SPtr> & expectedValues,
            __in bool useFirstKey = false,
            __in ULONG32 firstKey = -1)
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

        void VerifySortedEnumerable(
            __in IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>> & enumerable, 
            __in KSharedArray<KString::SPtr> & expectedKeys,
            __in KSharedArray<KBuffer::SPtr> & expectedValues)
        {
            CODING_ERROR_ASSERT(expectedKeys.Count() == expectedValues.Count());

            auto keyComparer = Store->KeyComparerSPtr;
            KSharedPtr<IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>> enumerableSPtr = &enumerable;

            if (SyncAwait(enumerableSPtr->MoveNextAsync(CancellationToken::None)) == false)
            {
                ASSERT_IFNOT(expectedKeys.Count() == 0, "Expected count was {0} but got no items", expectedKeys.Count());
                return;
            }

            KString::SPtr previousKey = enumerableSPtr->GetCurrent().Key;
            ULONG count = 1;

            while (SyncAwait(enumerableSPtr->MoveNextAsync(CancellationToken::None)) != false)
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
        }

      Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(EnumerationTestSuite, EnumerationTest)

    BOOST_AUTO_TEST_CASE(WriteSetStoreComponent_GetSortedKeyEnumerable_ShouldBeSorted)
    {
        auto keyComparerSPtr = Store->KeyComparerSPtr;
        WriteSetStoreComponent<KString::SPtr, KBuffer::SPtr>::SPtr componentSPtr;
        WriteSetStoreComponent<KString::SPtr, KBuffer::SPtr>::Create(K_DefaultHashFunction, *keyComparerSPtr, GetAllocator(), componentSPtr);

        
        for (LONG32 i = 1024; i >= 0; i--)
        {
            auto key = CreateString(i);
            auto buf = CreateBuffer(i);
            auto item = CreateVersionedItem(i, buf);
            componentSPtr->Add(false, key, *item);
        }

        auto firstKey = CreateString(10);
        auto lastKey = CreateString(99);
        auto enumerableSPtr = componentSPtr->GetSortedKeyEnumerable(true, firstKey, true, lastKey);
         
        auto expectedKeys = CreateStringSharedArray();
        for (LONG32 i = 10; i <= 99; i++)
        {
            auto key = CreateString(i);
            expectedKeys->Append(key);
        }

        VerifySortedEnumerable(*enumerableSPtr, *expectedKeys);
    }

#pragma region ConcurrentSkipList Filterable Enumerator
    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_FilterableEnumerator_UnspecifiedStartKey_AllKeysShouldBeSorted)
    {
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            expectedKeys->Append(key);
        }

        typedef ConcurrentSkipListFilterableEnumerator<KString::SPtr, KBuffer::SPtr> FilterableEnumerator;

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = nullptr;
        FilterableEnumerator::Create(*skipList, enumerator);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_FilterableEnumerator_BeforeStartKey_AllKeysShouldBeSorted)
    {
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            expectedKeys->Append(key);
        }

        typedef ConcurrentSkipListFilterableEnumerator<KString::SPtr, KBuffer::SPtr> FilterableEnumerator;

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = nullptr;
        FilterableEnumerator::Create(*skipList, enumerator);
        bool moved = enumerator->MoveTo(CreateString(3)); // Should still start at 4
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_FilterableEnumerator_AtStartKey_AllKeysShouldBeSorted)
    {
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            expectedKeys->Append(key);
        }

        typedef ConcurrentSkipListFilterableEnumerator<KString::SPtr, KBuffer::SPtr> FilterableEnumerator;

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = nullptr;
        FilterableEnumerator::Create(*skipList, enumerator);
        bool moved = enumerator->MoveTo(CreateString(4));
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_FilterableEnumerator_WithinRangeNotInList_AllKeysShouldBeSorted)
    {
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            if (i >= 7)
            {
                expectedKeys->Append(key);
            }
        }

        typedef ConcurrentSkipListFilterableEnumerator<KString::SPtr, KBuffer::SPtr> FilterableEnumerator;

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = nullptr;
        FilterableEnumerator::Create(*skipList, enumerator);
        bool moved = enumerator->MoveTo(CreateString(7)); // Should start at 8
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_FilterableEnumerator_WithinRangeInList_AllKeysShouldBeSorted)
    {
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            if (i >= 8)
            {
                expectedKeys->Append(key);
            }
        }

        typedef ConcurrentSkipListFilterableEnumerator<KString::SPtr, KBuffer::SPtr> FilterableEnumerator;

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = nullptr;
        FilterableEnumerator::Create(*skipList, enumerator);
        bool moved = enumerator->MoveTo(CreateString(8)); // Should start at 8
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_FilterableEnumerator_AtEnd_AllKeysShouldBeSorted)
    {
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            if (i >= 18)
            {
                expectedKeys->Append(key);
            }
        }

        typedef ConcurrentSkipListFilterableEnumerator<KString::SPtr, KBuffer::SPtr> FilterableEnumerator;

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = nullptr;
        FilterableEnumerator::Create(*skipList, enumerator);
        bool moved = enumerator->MoveTo(CreateString(18)); 
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_FilterableEnumerator_AfterEnd_AllKeysShouldBeSorted)
    {
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        ConcurrentSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);
        }

        typedef ConcurrentSkipListFilterableEnumerator<KString::SPtr, KBuffer::SPtr> FilterableEnumerator;

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = nullptr;
        FilterableEnumerator::Create(*skipList, enumerator);
        bool moved = enumerator->MoveTo(CreateString(21)); 
        CODING_ERROR_ASSERT(moved == false);
    }
#pragma endregion

#pragma region FastSkipList Filterable Enumerator
    BOOST_AUTO_TEST_CASE(FastSkipList_FilterableEnumerator_UnspecifiedStartKey_AllKeysShouldBeSorted)
    {
        FastSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        FastSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            expectedKeys->Append(key);
        }

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = skipList->GetKeys();
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_FilterableEnumerator_BeforeStartKey_AllKeysShouldBeSorted)
    {
        FastSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        FastSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            expectedKeys->Append(key);
        }

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = skipList->GetKeys();
        bool moved = enumerator->MoveTo(CreateString(3)); // Should still start at 4
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_FilterableEnumerator_AtStartKey_AllKeysShouldBeSorted)
    {
        FastSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        FastSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            expectedKeys->Append(key);
        }

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = skipList->GetKeys();
        bool moved = enumerator->MoveTo(CreateString(4));
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_FilterableEnumerator_WithinRangeNotInList_AllKeysShouldBeSorted)
    {
        FastSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        FastSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            if (i >= 7)
            {
                expectedKeys->Append(key);
            }
        }

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = skipList->GetKeys();
        bool moved = enumerator->MoveTo(CreateString(7)); // Should start at 8
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_FilterableEnumerator_WithinRangeInList_AllKeysShouldBeSorted)
    {
        FastSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        FastSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            if (i >= 8)
            {
                expectedKeys->Append(key);
            }
        }

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = skipList->GetKeys();
        bool moved = enumerator->MoveTo(CreateString(8)); // Should start at 8
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_FilterableEnumerator_AtEnd_AllKeysShouldBeSorted)
    {
        FastSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        FastSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);

            if (i >= 18)
            {
                expectedKeys->Append(key);
            }
        }

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = skipList->GetKeys();
        bool moved = enumerator->MoveTo(CreateString(18)); 
        CODING_ERROR_ASSERT(moved);
        
        IEnumerator<KString::SPtr>::SPtr ienumerator = static_cast<IEnumerator<KString::SPtr> *>(enumerator.RawPtr());
        VerifySortedEnumerable(*ienumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_FilterableEnumerator_AfterEnd_AllKeysShouldBeSorted)
    {
        FastSkipList<KString::SPtr, KBuffer::SPtr>::SPtr skipList = nullptr;
        FastSkipList<KString::SPtr, KBuffer::SPtr>::Create(Store->KeyComparerSPtr, GetAllocator(), skipList);

        auto expectedKeys = CreateStringSharedArray();

        for (ULONG32 i = 4; i < 20; i += 2)
        {
            auto key = CreateString(i);
            auto value = CreateBuffer(i);
            skipList->TryAdd(key, value);
        }

        IFilterableEnumerator<KString::SPtr>::SPtr enumerator = skipList->GetKeys();
        bool moved = enumerator->MoveTo(CreateString(21)); 
        CODING_ERROR_ASSERT(moved == false);
    }

#pragma endregion

#pragma region SortedSequenceMergeEnumerator tests
    BOOST_AUTO_TEST_CASE(SortedSequenceMergeEnumerator_Merge_TwoNonEmptySameSize_ShouldbeSorted)
    {
        auto keyComparerSPtr = Store->KeyComparerSPtr;

        KSharedArray<KString::SPtr>::SPtr array1 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array1->Append(CreateString(3));
        array1->Append(CreateString(7));
        array1->Append(CreateString(11));
        IEnumerator<KString::SPtr>::SPtr enumerator1;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array1, GetAllocator(), enumerator1);

        KSharedArray<KString::SPtr>::SPtr array2 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array2->Append(CreateString(1));
        array2->Append(CreateString(5));
        array2->Append(CreateString(9));
        IEnumerator<KString::SPtr>::SPtr enumerator2;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array2, GetAllocator(), enumerator2);
        
        KSharedArray<IEnumerator<KString::SPtr>::SPtr>::SPtr enumerators = _new(ALLOC_TAG, GetAllocator()) KSharedArray<IEnumerator<KString::SPtr>::SPtr>();
        enumerators->Append(enumerator1);
        enumerators->Append(enumerator2);

        IEnumerator<KString::SPtr>::SPtr mergedEnumerator;
        KString::SPtr defaultKey = nullptr;
        SortedSequenceMergeEnumerator<KString::SPtr>::Create(*enumerators, *keyComparerSPtr, false, defaultKey, false, defaultKey, GetAllocator(), mergedEnumerator);

        auto expectedKeys = CreateStringSharedArray();
        for (LONG32 i = 1; i <= 11; i += 2)
        {
            auto key = CreateString(i);
            expectedKeys->Append(key);
        }

        VerifySortedEnumerable(*mergedEnumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(SortedSequenceMergeEnumerator_Merge_OneEmtpyOneNonEmpty_ShouldbeSorted)
    {
        auto keyComparerSPtr = Store->KeyComparerSPtr;

        KSharedArray<KString::SPtr>::SPtr array1 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array1->Append(CreateString(3));
        array1->Append(CreateString(7));
        array1->Append(CreateString(11));
        IEnumerator<KString::SPtr>::SPtr enumerator1;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array1, GetAllocator(), enumerator1);

        KSharedArray<KString::SPtr>::SPtr array2 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        IEnumerator<KString::SPtr>::SPtr enumerator2;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array2, GetAllocator(), enumerator2);
        
        KSharedArray<IEnumerator<KString::SPtr>::SPtr>::SPtr enumerators = _new(ALLOC_TAG, GetAllocator()) KSharedArray<IEnumerator<KString::SPtr>::SPtr>();
        enumerators->Append(enumerator1);
        enumerators->Append(enumerator2);

        IEnumerator<KString::SPtr>::SPtr mergedEnumerator;
        KString::SPtr defaultKey = nullptr;
        SortedSequenceMergeEnumerator<KString::SPtr>::Create(*enumerators, *keyComparerSPtr, false, defaultKey, false, defaultKey, GetAllocator(), mergedEnumerator);

        auto expectedKeys = CreateStringSharedArray();
        for (LONG32 i = 3; i <= 11; i += 4)
        {
            auto key = CreateString(i);
            expectedKeys->Append(key);
        }

        VerifySortedEnumerable(*mergedEnumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(SortedSequenceMergeEnumerator_Merge_ThreeEnumeratorsDifferentLengths_ShouldbeSorted)
    {
        auto keyComparerSPtr = Store->KeyComparerSPtr;

        KSharedArray<KString::SPtr>::SPtr array1 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array1->Append(CreateString(3));
        array1->Append(CreateString(7));
        array1->Append(CreateString(11));
        IEnumerator<KString::SPtr>::SPtr enumerator1;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array1, GetAllocator(), enumerator1);

        KSharedArray<KString::SPtr>::SPtr array2 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        IEnumerator<KString::SPtr>::SPtr enumerator2;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array2, GetAllocator(), enumerator2);

        KSharedArray<KString::SPtr>::SPtr array3 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array3->Append(CreateString(5));
        array3->Append(CreateString(9));
        IEnumerator<KString::SPtr>::SPtr enumerator3;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array3, GetAllocator(), enumerator3);
        
        KSharedArray<IEnumerator<KString::SPtr>::SPtr>::SPtr enumerators = _new(ALLOC_TAG, GetAllocator()) KSharedArray<IEnumerator<KString::SPtr>::SPtr>();
        enumerators->Append(enumerator1);
        enumerators->Append(enumerator2);
        enumerators->Append(enumerator3);

        IEnumerator<KString::SPtr>::SPtr mergedEnumerator;
        KString::SPtr defaultKey = nullptr;
        SortedSequenceMergeEnumerator<KString::SPtr>::Create(*enumerators, *keyComparerSPtr, false, defaultKey, false, defaultKey, GetAllocator(), mergedEnumerator);

        auto expectedKeys = CreateStringSharedArray();
        for (LONG32 i = 3; i <= 11; i += 2)
        {
            auto key = CreateString(i);
            expectedKeys->Append(key);
        }

        VerifySortedEnumerable(*mergedEnumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(SortedSequenceMergeEnumerator_Merge_TwoEnumeratorsExactSame_ShouldBeSortedNoDuplicates)
    {
        auto keyComparerSPtr = Store->KeyComparerSPtr;

        KSharedArray<KString::SPtr>::SPtr array1 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array1->Append(CreateString(3));
        array1->Append(CreateString(7));
        array1->Append(CreateString(11));
        IEnumerator<KString::SPtr>::SPtr enumerator1;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array1, GetAllocator(), enumerator1);

        KSharedArray<KString::SPtr>::SPtr array2 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array2->Append(CreateString(3));
        array2->Append(CreateString(7));
        array2->Append(CreateString(11));
        IEnumerator<KString::SPtr>::SPtr enumerator2;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array2, GetAllocator(), enumerator2);
        
        KSharedArray<IEnumerator<KString::SPtr>::SPtr>::SPtr enumerators = _new(ALLOC_TAG, GetAllocator()) KSharedArray<IEnumerator<KString::SPtr>::SPtr>();
        enumerators->Append(enumerator1);
        enumerators->Append(enumerator2);

        IEnumerator<KString::SPtr>::SPtr mergedEnumerator;
        KString::SPtr defaultKey = nullptr;
        SortedSequenceMergeEnumerator<KString::SPtr>::Create(*enumerators, *keyComparerSPtr, false, defaultKey, false, defaultKey, GetAllocator(), mergedEnumerator);

        auto expectedKeys = CreateStringSharedArray();
        for (LONG32 i = 3; i <= 11; i += 4)
        {
            auto key = CreateString(i);
            expectedKeys->Append(key);
        }

        VerifySortedEnumerable(*mergedEnumerator, *expectedKeys);
    }

    BOOST_AUTO_TEST_CASE(SortedSequenceMergeEnumerator_Merge_TwoEnumeratorsSameElements_ShouldBeSortedNoDuplicates)
    {
        auto keyComparerSPtr = Store->KeyComparerSPtr;

        KSharedArray<KString::SPtr>::SPtr array1 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array1->Append(CreateString(1));
        array1->Append(CreateString(1));
        array1->Append(CreateString(1));
        IEnumerator<KString::SPtr>::SPtr enumerator1;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array1, GetAllocator(), enumerator1);

        KSharedArray<KString::SPtr>::SPtr array2 = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
        array2->Append(CreateString(1));
        array2->Append(CreateString(1));
        array2->Append(CreateString(1));
        IEnumerator<KString::SPtr>::SPtr enumerator2;
        KSharedArrayEnumerator<KString::SPtr>::Create(*array2, GetAllocator(), enumerator2);
        
        KSharedArray<IEnumerator<KString::SPtr>::SPtr>::SPtr enumerators = _new(ALLOC_TAG, GetAllocator()) KSharedArray<IEnumerator<KString::SPtr>::SPtr>();
        enumerators->Append(enumerator1);
        enumerators->Append(enumerator2);

        IEnumerator<KString::SPtr>::SPtr mergedEnumerator;
        KString::SPtr defaultKey = nullptr;
        SortedSequenceMergeEnumerator<KString::SPtr>::Create(*enumerators, *keyComparerSPtr, false, defaultKey, false, defaultKey, GetAllocator(), mergedEnumerator);

        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(1));

        VerifySortedEnumerable(*mergedEnumerator, *expectedKeys);
    }
#pragma endregion

    BOOST_AUTO_TEST_CASE(Enumerate_EmptyStoreKeys_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();

        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_EmptyStoreKeyValues_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        auto expectedValues = CreateBufferSharedArray();

        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(1));
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

#pragma region Enumerate WriteSet unit tests
    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_WithFirstKeyExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(3);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_WithFirstKeyNotExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(2);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_WithFirstKeyExists_WithLastKeyExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(3);
            KString::SPtr lastKey = CreateString(4);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_WithFirstKeyExists_WithLastKeyNotExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(3);
            KString::SPtr lastKey = CreateString(5);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_WithFirstKeyNotExists_WithLastKeyExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(2);
            KString::SPtr lastKey = CreateString(4);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_WithFirstKeyNotExists_WithLastKeyNotExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(2);
            KString::SPtr lastKey = CreateString(5);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_WithFirstAndLastKeySame_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(3);
            KString::SPtr lastKey = CreateString(3);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeys_WithFirstAndLastKeySameNotExist_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(2);
            KString::SPtr lastKey = CreateString(2);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(2), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(0), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(1), DefaultTimeout, CancellationToken::None));

            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(1));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(0));
            expectedValues->Append(CreateBuffer(1));
            expectedValues->Append(CreateBuffer(2));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstKeyExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(3);
            KString::SPtr lastKey;
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, false));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(3));
            expectedValues->Append(CreateBuffer(4));
            expectedValues->Append(CreateBuffer(6));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstKeyNotExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(2);
            KString::SPtr lastKey;
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, false));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(3));
            expectedValues->Append(CreateBuffer(4));
            expectedValues->Append(CreateBuffer(6));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithLastKeyExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey;
            KString::SPtr lastKey = CreateString(3);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, false, lastKey, true));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(1));
            expectedKeys->Append(CreateString(3));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(1));
            expectedValues->Append(CreateBuffer(3));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithLastKeyNotExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey;
            KString::SPtr lastKey = CreateString(2);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, false, lastKey, true));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(1));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(1));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstKeyExists_WithLastKeyExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(3);
            KString::SPtr lastKey = CreateString(4);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(3));
            expectedValues->Append(CreateBuffer(4));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstKeyExists_WithLastKeyNotExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(3);
            KString::SPtr lastKey = CreateString(5);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(3));
            expectedValues->Append(CreateBuffer(4));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstKeyNotExists_WithLastKeyExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(2);
            KString::SPtr lastKey = CreateString(4);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(3));
            expectedValues->Append(CreateBuffer(4));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstKeyNotExists_WithLastKeyNotExists_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(2);
            KString::SPtr lastKey = CreateString(5);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(3));
            expectedValues->Append(CreateBuffer(4));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstAndLastKeySame_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(3);
            KString::SPtr lastKey = CreateString(3);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(3));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(3));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstAndLastKeySameNotExist_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(2);
            KString::SPtr lastKey = CreateString(2);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetKeyValues_WithFirstAndLastOutOfBound_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));

            KString::SPtr firstKey = CreateString(0);
            KString::SPtr lastKey = CreateString(7);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(1));
            expectedKeys->Append(CreateString(3));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(1));
            expectedValues->Append(CreateBuffer(3));
            expectedValues->Append(CreateBuffer(4));
            expectedValues->Append(CreateBuffer(6));
            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);
            SyncAwait(txn->CommitAsync());
        }
    }
#pragma endregion

    BOOST_AUTO_TEST_CASE(Enumerate_DifferentialStateKeyValues_ShouldSucceed)
    {
       {
          auto txn = CreateWriteTransaction();
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(2), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(0), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
          SyncAwait(txn->CommitAsync());
       }

       {
          auto txn = CreateWriteTransaction();
          txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Enum::Snapshot;
          auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

          auto expectedKeys = CreateStringSharedArray();
          expectedKeys->Append(CreateString(1));
          expectedKeys->Append(CreateString(4));
          expectedKeys->Append(CreateString(6));

          auto expectedValues = CreateBufferSharedArray();
          expectedValues->Append(CreateBuffer(0));
          expectedValues->Append(CreateBuffer(1));
          expectedValues->Append(CreateBuffer(2));

          VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

          SyncAwait(txn->AbortAsync());
       }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_DifferentialState_SnapshotKeyValues_ShouldSucceed)
    {
       {
          auto txn = CreateWriteTransaction();
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(2), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(0), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
          SyncAwait(txn->CommitAsync());
       }

       // Start the snapshot transaction
       auto snapshottedTxn = CreateWriteTransaction();
       snapshottedTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

       // Read to start snapping visibility LSN
       SyncAwait(VerifyKeyExistsAsync(*Store, *snapshottedTxn->StoreTransactionSPtr, CreateString(4), nullptr, CreateBuffer(1), SingleElementBufferEquals));

       // Update once
       {
          auto txn = CreateWriteTransaction();
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(7), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(8), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
          SyncAwait(txn->CommitAsync());
       }

       // Update again
       {
          auto txn = CreateWriteTransaction();
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(10), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(7), DefaultTimeout, CancellationToken::None));
          SyncAwait(txn->CommitAsync());
       }

       {
          // Verify the updates
          SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(4), nullptr, CreateBuffer(7), SingleElementBufferEquals));
          SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(1), nullptr, CreateBuffer(10), SingleElementBufferEquals));
          SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(6), nullptr, CreateBuffer(9), SingleElementBufferEquals));
       }

       {
          auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*snapshottedTxn->StoreTransactionSPtr));

          auto expectedKeys = CreateStringSharedArray();
          expectedKeys->Append(CreateString(1));
          expectedKeys->Append(CreateString(4));
          expectedKeys->Append(CreateString(6));

          auto expectedValues = CreateBufferSharedArray();
          expectedValues->Append(CreateBuffer(0));
          expectedValues->Append(CreateBuffer(1));
          expectedValues->Append(CreateBuffer(2));

          VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

          SyncAwait(snapshottedTxn->AbortAsync());
       }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_DifferentialState_SnapshotKeyValues_DuringConsolidation_ShouldSucceed)
    {
       {
          auto txn = CreateWriteTransaction();
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(2), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(0), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
          SyncAwait(txn->CommitAsync());
       }

       // Start the snapshot transaction
       auto snapshottedTxn = CreateWriteTransaction();
       snapshottedTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

       // Read to start snapping visibility LSN
       SyncAwait(VerifyKeyExistsAsync(*Store, *snapshottedTxn->StoreTransactionSPtr, CreateString(4), nullptr, CreateBuffer(1), SingleElementBufferEquals));

       // Update once
       {
          auto txn = CreateWriteTransaction();
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(7), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(8), DefaultTimeout, CancellationToken::None));
          SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
          SyncAwait(txn->CommitAsync());
       }

       Checkpoint();

       {
          // Verify the updates
          SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(4), nullptr, CreateBuffer(9), SingleElementBufferEquals));
          SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(1), nullptr, CreateBuffer(8), SingleElementBufferEquals));
          SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(6), nullptr, CreateBuffer(7), SingleElementBufferEquals));
       }

       {
          auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*snapshottedTxn->StoreTransactionSPtr));

          auto expectedKeys = CreateStringSharedArray();
          expectedKeys->Append(CreateString(1));
          expectedKeys->Append(CreateString(4));
          expectedKeys->Append(CreateString(6));

          auto expectedValues = CreateBufferSharedArray();
          expectedValues->Append(CreateBuffer(0));
          expectedValues->Append(CreateBuffer(1));
          expectedValues->Append(CreateBuffer(2));

          VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

          SyncAwait(snapshottedTxn->AbortAsync());
       }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedKeyValues_ShouldSucceed)
    {
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(2), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(0), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Enum::Snapshot;
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

            auto expectedKeys = CreateStringSharedArray();
            expectedKeys->Append(CreateString(1));
            expectedKeys->Append(CreateString(4));
            expectedKeys->Append(CreateString(6));

            auto expectedValues = CreateBufferSharedArray();
            expectedValues->Append(CreateBuffer(0));
            expectedValues->Append(CreateBuffer(1));
            expectedValues->Append(CreateBuffer(2));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_SnapshotKeyValues_FromConsolidation_ShouldSucceed)
    {
        {
            // Populate the store with existing data - a snapshot of this will be enumerated
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(2), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(0), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Start the snapshot transaction
        auto snapshottedTxn = CreateWriteTransaction();
        snapshottedTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

        // Read to start snapping visibility LSN
        SyncAwait(VerifyKeyExistsAsync(*Store, *snapshottedTxn->StoreTransactionSPtr, CreateString(4), nullptr, CreateBuffer(1), SingleElementBufferEquals));
        SyncAwait(VerifyKeyExistsAsync(*Store, *snapshottedTxn->StoreTransactionSPtr, CreateString(1), nullptr, CreateBuffer(0), SingleElementBufferEquals));
        SyncAwait(VerifyKeyExistsAsync(*Store, *snapshottedTxn->StoreTransactionSPtr, CreateString(6), nullptr, CreateBuffer(2), SingleElementBufferEquals));

        {
            // Update with other values outside of snapshotted txn
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(7), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(8), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            // Verify the updates
            SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(4), nullptr, CreateBuffer(9), SingleElementBufferEquals));
            SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(1), nullptr, CreateBuffer(8), SingleElementBufferEquals));
            SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(6), nullptr, CreateBuffer(7), SingleElementBufferEquals));
        }

        Checkpoint();

        {
            // Update again with other values outside of snapshotted txn
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(5), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            // Verify the updates
            SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(4), nullptr, CreateBuffer(6), SingleElementBufferEquals));
            SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(1), nullptr, CreateBuffer(5), SingleElementBufferEquals));
            SyncAwait(VerifyKeyExistsAsync(*Store, CreateString(6), nullptr, CreateBuffer(4), SingleElementBufferEquals));
        }

        // Enumerate the snapshotted transaction
        auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*snapshottedTxn->StoreTransactionSPtr));

        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(1));
        expectedKeys->Append(CreateString(4));
        expectedKeys->Append(CreateString(6));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(0));
        expectedValues->Append(CreateBuffer(1));
        expectedValues->Append(CreateBuffer(2));

        VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

        SyncAwait(snapshottedTxn->AbortAsync());
    }

#pragma region Consolidated-Differential-Writeset Unit Test Keys Only
    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeys_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(4));
        expectedKeys->Append(CreateString(9));
        expectedKeys->Append(CreateString(14));
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));
        expectedKeys->Append(CreateString(51));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeys_WithFirstKeyExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));
        expectedKeys->Append(CreateString(51));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(18);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeys_WithFirstKeyNotExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));
        expectedKeys->Append(CreateString(51));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(30);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeys_WithFirstKeyExists_WithLastKeyExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(9));
        expectedKeys->Append(CreateString(14));
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(9);
            KString::SPtr lastKey = CreateString(24);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeys_WithFirstKeyExists_WithLastKeyNotExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(14));
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(14);
            KString::SPtr lastKey = CreateString(50);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeys_WithFirstKeyNotExists_WithLastKeyExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(16);
            KString::SPtr lastKey = CreateString(20);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeys_WithFirstKeyNotExists_WithLastKeyNotExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(4));
        expectedKeys->Append(CreateString(9));
        expectedKeys->Append(CreateString(14));
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));
        expectedKeys->Append(CreateString(51));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(1);
            KString::SPtr lastKey = CreateString(60);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeys_WithFirstAndLastKey_EmptySubset_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(40);
            KString::SPtr lastKey = CreateString(45);
            auto enumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, lastKey));

            VerifySortedEnumerable(*enumerator, *expectedKeys);

            SyncAwait(txn->CommitAsync());
        }
    }
#pragma endregion 

#pragma region Consolidated-Differential-Writeset Keys and Values
    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeyValues_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(4));
        expectedKeys->Append(CreateString(9));
        expectedKeys->Append(CreateString(14));
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));
        expectedKeys->Append(CreateString(51));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(4));
        expectedValues->Append(CreateBuffer(9));
        expectedValues->Append(CreateBuffer(14));
        expectedValues->Append(CreateBuffer(18));
        expectedValues->Append(CreateBuffer(20));
        expectedValues->Append(CreateBuffer(24));
        expectedValues->Append(CreateBuffer(31));
        expectedValues->Append(CreateBuffer(33));
        expectedValues->Append(CreateBuffer(38));
        expectedValues->Append(CreateBuffer(46));
        expectedValues->Append(CreateBuffer(51));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeyValues_WithFirstKeyExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));
        expectedKeys->Append(CreateString(51));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(18));
        expectedValues->Append(CreateBuffer(20));
        expectedValues->Append(CreateBuffer(24));
        expectedValues->Append(CreateBuffer(31));
        expectedValues->Append(CreateBuffer(33));
        expectedValues->Append(CreateBuffer(38));
        expectedValues->Append(CreateBuffer(46));
        expectedValues->Append(CreateBuffer(51));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(18);
            KString::SPtr lastKey = nullptr;
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, false));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeyValues_WithFirstKeyNotExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));
        expectedKeys->Append(CreateString(51));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(31));
        expectedValues->Append(CreateBuffer(33));
        expectedValues->Append(CreateBuffer(38));
        expectedValues->Append(CreateBuffer(46));
        expectedValues->Append(CreateBuffer(51));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(30);
            KString::SPtr lastKey = nullptr;
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, false));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeyValues_WithFirstKeyExists_WithLastKeyExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(9));
        expectedKeys->Append(CreateString(14));
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(9));
        expectedValues->Append(CreateBuffer(14));
        expectedValues->Append(CreateBuffer(18));
        expectedValues->Append(CreateBuffer(20));
        expectedValues->Append(CreateBuffer(24));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(9);
            KString::SPtr lastKey = CreateString(24);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, true));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeyValues_WithFirstKeyExists_WithLastKeyNotExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(14));
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(14));
        expectedValues->Append(CreateBuffer(18));
        expectedValues->Append(CreateBuffer(20));
        expectedValues->Append(CreateBuffer(24));
        expectedValues->Append(CreateBuffer(31));
        expectedValues->Append(CreateBuffer(33));
        expectedValues->Append(CreateBuffer(38));
        expectedValues->Append(CreateBuffer(46));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(14);
            KString::SPtr lastKey = CreateString(50);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, true));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeyValues_WithFirstKeyNotExists_WithLastKeyExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(18));
        expectedValues->Append(CreateBuffer(20));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(16);
            KString::SPtr lastKey = CreateString(20);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, true));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeyValues_WithFirstKeyNotExists_WithLastKeyNotExists_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(4));
        expectedKeys->Append(CreateString(9));
        expectedKeys->Append(CreateString(14));
        expectedKeys->Append(CreateString(18));
        expectedKeys->Append(CreateString(20));
        expectedKeys->Append(CreateString(24));
        expectedKeys->Append(CreateString(31));
        expectedKeys->Append(CreateString(33));
        expectedKeys->Append(CreateString(38));
        expectedKeys->Append(CreateString(46));
        expectedKeys->Append(CreateString(51));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(4));
        expectedValues->Append(CreateBuffer(9));
        expectedValues->Append(CreateBuffer(14));
        expectedValues->Append(CreateBuffer(18));
        expectedValues->Append(CreateBuffer(20));
        expectedValues->Append(CreateBuffer(24));
        expectedValues->Append(CreateBuffer(31));
        expectedValues->Append(CreateBuffer(33));
        expectedValues->Append(CreateBuffer(38));
        expectedValues->Append(CreateBuffer(46));
        expectedValues->Append(CreateBuffer(51));

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();

        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(1);
            KString::SPtr lastKey = CreateString(60);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, true));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ConsolidatedDifferentialWritesetKeyValues_WithFirstAndLastKey_EmptySubset_ShouldSucceed)
    {
        auto expectedKeys = CreateStringSharedArray();
        auto expectedValues = CreateBufferSharedArray();

        // Populate the store with keys that will be moved to consolidated
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(20), CreateBuffer(20), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(38), CreateBuffer(38), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(104), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(151), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(131), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        // Checkpoint to move keys to consolidated
        Checkpoint();
        // Populate the store with keys that will be in differential
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(146), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(14), CreateBuffer(14), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(251), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(18), CreateBuffer(18), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(31), CreateBuffer(31), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        {
            // Populate the store with keys that will be in writeset
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(33), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(51), CreateBuffer(51), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(24), CreateBuffer(24), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(9), CreateBuffer(9), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(46), CreateBuffer(46), DefaultTimeout, CancellationToken::None));
            
            KString::SPtr firstKey = CreateString(40);
            KString::SPtr lastKey = CreateString(45);
            auto enumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr, firstKey, true, lastKey, true));

            VerifySortedEnumerable(*enumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->CommitAsync());
        }
    }
#pragma endregion

#pragma region Writeset Tests
    BOOST_AUTO_TEST_CASE(Enumerate_EmptyWriteset_DifferentialStateNotEmpty_NoOperationsFromWriteSet_ShouldSucceed)
    {
        ULONG32 numItemsInStore = 16;
        ULONG32 numItemsInWriteSet = 0;
        UNREFERENCED_PARAMETER(numItemsInWriteSet);

        auto expectedKeys = CreateStringSharedArray();
        auto expectedValues = CreateBufferSharedArray();
        
        // Setup differential state
        for (ULONG32 i = 0; i < numItemsInStore; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
                expectedKeys->Append(CreateString(i));
                expectedValues->Append(CreateBuffer(i));
            }
        }
        
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));
            auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

            VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
            VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSetLarge_NoOperationsFromWriteSet_ShouldSucceed)
    {
        auto random = GetRandom();
        LONG32 numItemsInStore = 16;
        LONG32 numItemsInWriteSet = 5000;

        KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();
        
        // Setup differential state
        for (LONG32 i = 0; i < numItemsInStore; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
                addedKeys->Append(i);
            }
        }
        
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            for (LONG32 i = 0; i < numItemsInWriteSet; i++)
            {
                auto newKey = random.Next(99999);
                
                KeyValuePair<LONG64, KBuffer::SPtr> value;
                bool keyExists = SyncAwait(Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None));
                if (keyExists)
                {
                    i--;
                    continue;
                }

                // Setup writeset
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None));

                addedKeys->Append(newKey);
            }

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

            auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));
            auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

            VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
            VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->AbortAsync());
        }
    }
#pragma endregion

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSet_EmptyDifferentialState_WritingToWriteSetDuringEnumeration_NewWritesMustNotAppear)
    {
        auto random = GetRandom();
        LONG32 numItemsInWriteSet = 1000;
        LONG32 maxKeyValue = 5000;
        auto maxKey = CreateString(maxKeyValue); // All keys in enumeration will be smaller than this

        KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();

        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            for (LONG32 i = 0; i < numItemsInWriteSet; i++)
            {
                auto newKey = random.Next(maxKeyValue);
                
                KeyValuePair<LONG64, KBuffer::SPtr> value;
                bool keyExists = SyncAwait(Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None));
                if (keyExists)
                {
                    i--;
                    continue;
                }

                // Setup writeset
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None));

                addedKeys->Append(newKey);
            }

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

            auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));
            auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));
            
            keyEnumerator->MoveNext();
            auto previousKey = keyEnumerator->Current();

            for(LONG32 i = 1; keyEnumerator->MoveNext(); i++)
            {
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(previousKey, keyEnumerator->Current()) <= 0);
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(keyEnumerator->Current(), maxKey) <= 0);
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(keyEnumerator->Current(), (*expectedKeys)[i]) == 0);
                previousKey = keyEnumerator->Current();
                
                // Add a key that shouldn't show up in enumeration
                auto newKey = maxKeyValue + i;
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None));
            }
            
            SyncAwait(keyValueEnumerator->MoveNextAsync(CancellationToken::None));
            previousKey = keyValueEnumerator->GetCurrent().Key;
            for(LONG32 i = 1; SyncAwait(keyValueEnumerator->MoveNextAsync(CancellationToken::None)); i++)
            {
                auto key = keyValueEnumerator->GetCurrent().Key;
                auto value = keyValueEnumerator->GetCurrent().Value.Value;
                auto expectedKey = (*expectedKeys)[i];
                ULONG32* expectedValue = static_cast<ULONG32 *>((*expectedValues)[i]->GetBuffer());
                UNREFERENCED_PARAMETER(expectedKey);
                UNREFERENCED_PARAMETER(expectedValue);

                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(previousKey, key) <= 0);
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(key, maxKey) <= 0);
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(key, (*expectedKeys)[i]) == 0);
                CODING_ERROR_ASSERT(SingleElementBufferEquals(value, (*expectedValues)[i]));

                previousKey = key;
                
                // Add a key that shouldn't show up in enumeration
                auto newKey = maxKeyValue + numItemsInWriteSet + i;
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSet_NotEmptyDifferentialState_WritingToWriteSetDuringEnumeration_NewWritesMustNotAppear)
    {
        auto random = GetRandom();
        LONG32 numItemsInStore = 16;
        LONG32 numItemsInWriteSet = 1000;
        LONG32 maxKeyValue = 5000;
        auto maxKey = CreateString(maxKeyValue); // All keys in enumeration will be smaller than this

        KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();

        // Setup differential state
        for (LONG32 i = 0; i < numItemsInStore; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
                addedKeys->Append(i);
            }
        }

        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            for (LONG32 i = 0; i < numItemsInWriteSet; i++)
            {
                auto newKey = random.Next(maxKeyValue);
                
                KeyValuePair<LONG64, KBuffer::SPtr> value;
                bool keyExists = SyncAwait(Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, CreateString(newKey), DefaultTimeout, value, CancellationToken::None));
                if (keyExists)
                {
                    i--;
                    continue;
                }

                // Setup writeset
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None));

                addedKeys->Append(newKey);
            }

            auto expectedKeys = CreateStringSharedArray();
            auto expectedValues = CreateBufferSharedArray();
            PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

            auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));
            auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));
            
            keyEnumerator->MoveNext();
            auto previousKey = keyEnumerator->Current();

            for(LONG32 i = 1; keyEnumerator->MoveNext(); i++)
            {
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(previousKey, keyEnumerator->Current()) <= 0);
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(keyEnumerator->Current(), maxKey) <= 0);
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(keyEnumerator->Current(), (*expectedKeys)[i]) == 0);
                previousKey = keyEnumerator->Current();
                
                // Add a key that shouldn't show up in enumeration
                auto newKey = maxKeyValue + i;
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None));
            }
            
            SyncAwait(keyValueEnumerator->MoveNextAsync(CancellationToken::None));
            previousKey = keyValueEnumerator->GetCurrent().Key;
            for(LONG32 i = 1; SyncAwait(keyValueEnumerator->MoveNextAsync(CancellationToken::None)); i++)
            {
                auto key = keyValueEnumerator->GetCurrent().Key;
                auto value = keyValueEnumerator->GetCurrent().Value.Value;
                auto expectedKey = (*expectedKeys)[i];
                ULONG32* expectedValue = static_cast<ULONG32 *>((*expectedValues)[i]->GetBuffer());
                UNREFERENCED_PARAMETER(expectedKey);
                UNREFERENCED_PARAMETER(expectedValue);

                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(previousKey, key) <= 0);
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(key, maxKey) <= 0);
                CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(key, (*expectedKeys)[i]) == 0);
                CODING_ERROR_ASSERT(SingleElementBufferEquals(value, (*expectedValues)[i]));

                previousKey = key;
                
                // Add a key that shouldn't show up in enumeration
                auto newKey = maxKeyValue + numItemsInWriteSet + numItemsInStore + i;
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(newKey), CreateBuffer(newKey), DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_WriteSet_DifferentVersionedItemTypes_ReadCorrectly)
    {
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(0));
        expectedKeys->Append(CreateString(1));
        // 2 & 3 removed
        expectedKeys->Append(CreateString(4));

        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(0));
        expectedValues->Append(CreateBuffer(11));
        expectedValues->Append(CreateBuffer(44));

        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Setup Add case
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(0), DefaultTimeout, CancellationToken::None));

            // Setup Add & Update case
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(11), DefaultTimeout, CancellationToken::None));

            // Setup Add & Remove case
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(2), CreateBuffer(2), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(2), DefaultTimeout, CancellationToken::None));

            // Setup Add & Update & Remove case
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(33), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(3), DefaultTimeout, CancellationToken::None));

            // Setup Add & Remove & Add case
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(44), DefaultTimeout, CancellationToken::None));

            auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));
            auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

            VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
            VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);

            SyncAwait(txn->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumerate_AllComponents_OneUniqueItemPerComponent_ShouldSucceed)
    {
        KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();
        for (ULONG32 i = 0; i < 8; i++)
        {
            addedKeys->Append(i);
        }

        // Setup consolidated
        for (ULONG32 i = 0; i < 3; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            Checkpoint();
        }
        
        // Setup delta differentials
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), CreateBuffer(3), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

        }

        Checkpoint();

        // Delta differential two
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), CreateBuffer(4), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        // Setup Differential: Last committed -1
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(5), CreateBuffer(5), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto differentialKey = 5;

        // Setup item for snapshot component
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), CreateBuffer(6), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto snapshotKey = 6;

        // Prepare test transaction
        auto testTxn = CreateWriteTransaction();
        testTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        SyncAwait(VerifyKeyExistsAsync(*Store, *testTxn->StoreTransactionSPtr, CreateString(snapshotKey), nullptr, CreateBuffer(snapshotKey), SingleElementBufferEquals));

        // Setup snapshot component
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(snapshotKey), CreateBuffer(snapshotKey * 11), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(snapshotKey), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Setup Differential: Last committed
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(differentialKey), CreateBuffer(differentialKey * 11), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        // Setup WriteSet
        SyncAwait(Store->AddAsync(*testTxn->StoreTransactionSPtr, CreateString(7), CreateBuffer(7), DefaultTimeout, CancellationToken::None));

        // Test
        auto expectedKeys = CreateStringSharedArray();
        auto expectedValues = CreateBufferSharedArray();
        PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

        auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*testTxn->StoreTransactionSPtr));
        auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*testTxn->StoreTransactionSPtr));

        VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
        VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);
             
        SyncAwait(testTxn->CommitAsync());
    }
    
    BOOST_AUTO_TEST_CASE(Enumerate_AllComponents_ManyItemsPerComponent_ShouldSucceed)
    {
        ULONG32 numItemsPerComponent = 64;
        KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();
        for (ULONG32 i = 0; i < 8 * numItemsPerComponent; i++)
        {
            addedKeys->Append(i);
        }

        auto currentKey = 0;

        // Setup consolidated
        for (ULONG32 i = 0; i < 3; i++)
        {
            for (ULONG32 j = currentKey; j < currentKey + numItemsPerComponent; j++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(j), CreateBuffer(j), DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }
            }
            currentKey += numItemsPerComponent;
            Checkpoint();
        }
        
        // Setup delta differentials
        for (ULONG32 i = currentKey; i < currentKey + numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        Checkpoint();

        // Delta differential two
        currentKey += numItemsPerComponent;
        for (ULONG32 i = currentKey; i < currentKey + numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        Checkpoint();

        // Setup Differential: Last committed -1
        currentKey += numItemsPerComponent;
        for (ULONG32 i = currentKey; i < currentKey + numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        auto differentialKey = currentKey;

        // Setup item for snapshot component
        currentKey += numItemsPerComponent;
        for (ULONG32 i = currentKey; i < currentKey + numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        auto snapshotKey = currentKey;

        // Prepare test transaction
        auto testTxn = CreateWriteTransaction();
        testTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        SyncAwait(VerifyKeyExistsAsync(*Store, *testTxn->StoreTransactionSPtr, CreateString(snapshotKey), nullptr, CreateBuffer(snapshotKey), SingleElementBufferEquals));

        // Setup snapshot component
        for (ULONG32 i = snapshotKey; i < snapshotKey + numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i + 1), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        for (ULONG32 i = snapshotKey; i < snapshotKey + numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        // Setup Differential: Last committed
        for (ULONG32 i = differentialKey; i < differentialKey + numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i + 1), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }
        
        // Setup WriteSet
        currentKey += numItemsPerComponent;
        for (ULONG32 i = currentKey; i < currentKey + numItemsPerComponent; i++)
        {
            SyncAwait(Store->AddAsync(*testTxn->StoreTransactionSPtr, CreateString(i), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
        }

        // Test
        auto expectedKeys = CreateStringSharedArray();
        auto expectedValues = CreateBufferSharedArray();
        PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

        auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*testTxn->StoreTransactionSPtr));
        auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*testTxn->StoreTransactionSPtr));

        VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
        VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);
             
        SyncAwait(testTxn->CommitAsync());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_AllComponents_SingleDuplicateKey_ShouldSucceed)
    {
        const ULONG32 magicValue = 16061987;

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(0), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        // Setup consolidated and delta differetials
        for (ULONG32 i = 1; i <= 3; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            Checkpoint();
        }
        
        // Setup Differential: Last committed -1
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(magicValue), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Prepare test transaction
        auto testTxn = CreateWriteTransaction();
        testTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        SyncAwait(VerifyKeyExistsAsync(*Store, *testTxn->StoreTransactionSPtr, CreateString(0), nullptr, CreateBuffer(magicValue), SingleElementBufferEquals));

        // Setup snapshot component
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(0), CreateBuffer(magicValue + 1), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(0), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        // Setup writeset
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), CreateBuffer(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Test
        auto expectedKeys = CreateStringSharedArray();
        expectedKeys->Append(CreateString(0));
        auto expectedValues = CreateBufferSharedArray();
        expectedValues->Append(CreateBuffer(magicValue));

        auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*testTxn->StoreTransactionSPtr));
        auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*testTxn->StoreTransactionSPtr));

        VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
        VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);
             
        SyncAwait(testTxn->CommitAsync());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_AllComponents_ManyDuplicateKeys_ShouldSucceed)
    {
        const LONG32 numItemsPerComponent = 200;
        const ULONG32 magicValue = 16061987;
        
        for (ULONG32 i = 0; i < numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(0), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        Checkpoint();

        // Setup consolidated and delta differetials
        for (ULONG32 i = 1; i <= 3; i++)
        {
            for (ULONG32 j = 0; j < numItemsPerComponent; j++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(j), CreateBuffer(i), DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }
            }

            Checkpoint();
        }
        
        // Setup Differential: Last committed -1
        for (ULONG32 i = 0; i < numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(magicValue), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        // Prepare test transaction
        auto testTxn = CreateWriteTransaction();
        testTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        for (ULONG32 i = 0; i < numItemsPerComponent; i++)
        {
            SyncAwait(VerifyKeyExistsAsync(*Store, *testTxn->StoreTransactionSPtr, CreateString(i), nullptr, CreateBuffer(magicValue), SingleElementBufferEquals));
        }

        // Setup snapshot component
        for (ULONG32 i = 0; i < numItemsPerComponent; i++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), CreateBuffer(magicValue + 1), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(i), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }
        
        // Setup writeset
        // Should not show up in enumeration
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(numItemsPerComponent), CreateBuffer(numItemsPerComponent), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Test
        auto expectedKeys = CreateStringSharedArray();
        auto expectedValues = CreateBufferSharedArray();
        for (ULONG32 i = 0; i < numItemsPerComponent; i++)
        {
            expectedKeys->Append(CreateString(i));
            expectedValues->Append(CreateBuffer(magicValue));
        }

        auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*testTxn->StoreTransactionSPtr));
        auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*testTxn->StoreTransactionSPtr));

        VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
        VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);
             
        SyncAwait(testTxn->CommitAsync());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_AllComponents_RandomlyPopulated_ShouldSucceed)
    {
        auto random = GetRandom();
        const LONG32 numItemsPerComponent = 200;

        KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();
        
        // Setup initial consolidated state
        for (ULONG32 i = 0; i < numItemsPerComponent; i++)
        {
            auto newKey = PopulateWithRandomNewKey(random);
            addedKeys->Append(newKey);
        }

        Checkpoint();

        // Setup consolidated and delta differetials
        for (ULONG32 i = 1; i <= 3; i++)
        {
            for (ULONG32 j = 0; j < numItemsPerComponent; j++)
            {
                auto newKey = PopulateWithRandomNewKey(random);
                addedKeys->Append(newKey);
            }

            Checkpoint();
        }
        
        // Setup Differential: Last committed -1
        for (ULONG32 i = 0; i < numItemsPerComponent; i++)
        {
            auto newKey = PopulateWithRandomNewKey(random);
            addedKeys->Append(newKey);
        }

        // Prepare test transaction
        auto testTxn = CreateWriteTransaction();
        testTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        for (ULONG32 i = 0; i < addedKeys->Count(); i++)
        {
            auto key = (*addedKeys)[i];
            SyncAwait(VerifyKeyExistsAsync(*Store, *testTxn->StoreTransactionSPtr, CreateString(key), nullptr, CreateBuffer(key), SingleElementBufferEquals));
        }

        // Setup snapshot component
        for (ULONG32 i = 0; i < addedKeys->Count(); i++)
        {
            auto key = (*addedKeys)[i];
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(key), CreateBuffer(key + 1), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(key), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }
        
        // Setup writeset
        for (ULONG32 i = 0; i < numItemsPerComponent; i++)
        {
            auto newKey = PopulateWithRandomNewKey(*testTxn, random);
            addedKeys->Append(newKey);
        }

        // Test
        auto expectedKeys = CreateStringSharedArray();
        auto expectedValues = CreateBufferSharedArray();
        PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

        auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*testTxn->StoreTransactionSPtr));
        auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*testTxn->StoreTransactionSPtr));

        VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
        VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);
             
        SyncAwait(testTxn->CommitAsync());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_AddRemoveTest_ShouldSucceed)
    {
        // Keeping numbers small to reduce test time
        ULONG32 numItems = 64;
        ULONG32 numIterations = 32;

        KSharedArray<ULONG32>::SPtr addedKeys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();
        for (ULONG32 i = 0; i < numItems; i++)
        {
            addedKeys->Append(i);
        }

        for (ULONG32 key = 0; key < numItems; key++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(key), CreateBuffer(key), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        for (ULONG32 i = 0; i < numIterations; i++)
        {
            for (ULONG32 key = 0; key < numItems; key++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, CreateString(key), DefaultTimeout, CancellationToken::None));
                    if (key % 2 == 1)
                    {
                        SyncAwait(txn->CommitAsync());
                    }
                    else
                    {
                        SyncAwait(txn->AbortAsync());
                    }
                }
            }

            Checkpoint();

            for (ULONG32 key = 0; key < numItems; key++)
            {
                {
                    // TODO: Replace with TryAddAsync
                    auto txn = CreateWriteTransaction();
                    KeyValuePair<LONG64, KBuffer::SPtr> result;
                    bool exists = SyncAwait(Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, CreateString(key), DefaultTimeout, result, CancellationToken::None));
                    if (!exists)
                    {
                        SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(key), CreateBuffer(key), DefaultTimeout, CancellationToken::None));
                    }
                    if (key % 2 == 1)
                    {
                        SyncAwait(txn->CommitAsync());
                    }
                    else
                    {
                        SyncAwait(txn->AbortAsync());
                    }
                }
            }

            // Test
            {
                auto txn = CreateWriteTransaction();
                txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                auto expectedKeys = CreateStringSharedArray();
                auto expectedValues = CreateBufferSharedArray();
                PopulateExpectedOutputs(addedKeys, *expectedKeys, *expectedValues);

                auto keyEnumerator = SyncAwait(Store->CreateKeyEnumeratorAsync(*txn->StoreTransactionSPtr));
                auto keyValueEnumerator = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));

                VerifySortedEnumerable(*keyEnumerator, *expectedKeys);
                VerifySortedEnumerable(*keyValueEnumerator, *expectedKeys, *expectedValues);
                SyncAwait(txn->CommitAsync());
            }
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
