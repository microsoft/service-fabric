// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TStoreTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data::TStore;
    using namespace TxnReplicator;

    class PartitionSortedListTest
    {
    public:
        PartitionSortedListTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~PartitionSortedListTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        template< typename TKey, typename TValue>
        KSharedPtr<PartitionedSortedList<TKey, TValue>> CreatePartionedSortedList(int maxSublistSize=8192)
        {
            KAllocator& allocator = PartitionSortedListTest::GetAllocator();
            KSharedPtr<IntComparer> comparerSptr = nullptr;
            NTSTATUS status = IntComparer::Create(allocator, comparerSptr);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

            KSharedPtr<PartitionedSortedList<TKey, TValue>> sortedListSptr = nullptr;
            status = PartitionedSortedList<TKey, TValue>::Create(*comparerSptr, allocator, sortedListSptr, maxSublistSize);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
            return sortedListSptr;
        }

        static void AddItems(
            __in KSharedPtr<PartitionedSortedList<int, int>>& sortedList,
            __in int startingValue,
            __in int count)
        {
            for (int i = startingValue; i < startingValue + count; i++)
            {
                int key = i;
                sortedList->Add(key, key);
            }
        }

        static void VerifyItems(
            __in KSharedPtr<PartitionedSortedList<int, int>>& sortedList,
            __in int startingValue,
            __in int count)
        {
            VERIFY_ARE_EQUAL(count, sortedList->Count());
            int index = startingValue;
            auto enumeratorSPtr = sortedList->GetKeys();
            while (enumeratorSPtr->MoveNext())
            {
                KeyValuePair<int, int> entry = enumeratorSPtr->Current();
                int key = entry.Key;
                int value = sortedList->GetValue(key);
                VERIFY_ARE_EQUAL(index, key);
                VERIFY_ARE_EQUAL(index, value);
                index++;
            }
        }

        static void DiscontinuouslyAddItems(
            __in KSharedPtr<PartitionedSortedList<int, int>>& sortedList,
            __in int startingValue,
            __in int endingValue)
        {
            for (int i = startingValue; i < endingValue; i = i + 2)
            {
                int key = i;
                sortedList->Add(key, key);
            }
        }

        static void ContainsKeyTest(
            __in KSharedPtr<PartitionedSortedList<int, int>>& sortedListSptr,
            __in int numberOfItems,
            __in int numberOfReads,
            __in bool isRandom)
        {
            AddItems(sortedListSptr, 0, numberOfItems);
            VERIFY_ARE_EQUAL(numberOfItems, sortedListSptr->Count());

            Random random;
            for (int i = 0; i < numberOfReads; i++)
            {
                int key = isRandom ? random.Next(0, numberOfItems) : i % numberOfItems;
                bool isSuccess = sortedListSptr->ContainsKey(key);
                CODING_ERROR_ASSERT(isSuccess);
            }

            VERIFY_ARE_EQUAL(numberOfItems, sortedListSptr->Count());
        }

        static void RandomVerifyDiscontinuouslyAdd(
            __in KSharedPtr<PartitionedSortedList<int, int>>& sortedList,
            __in int startingValue,
            __in int endingValue,
            __in int times)
        {
            Random random;
            for (int i = 0; i < times; i++)
            {
                int key = random.Next(endingValue - startingValue) + startingValue;
                int outValue = -1;
                bool containsKeySuccess = sortedList->ContainsKey(key);
                bool tryGetValueSuccess = sortedList->TryGetValue(key, outValue);
                if ((key - startingValue) % 2 == 1)
                {
                    CODING_ERROR_ASSERT(!containsKeySuccess);
                    CODING_ERROR_ASSERT(!tryGetValueSuccess);
                    CODING_ERROR_ASSERT(outValue == -1);
                }
                else
                {
                    CODING_ERROR_ASSERT(containsKeySuccess);
                    CODING_ERROR_ASSERT(tryGetValueSuccess);
                    CODING_ERROR_ASSERT(outValue == key);
                }
            }
        }

        static void VerifyRandomCallGetKeyAndValue(
            __in KSharedPtr<PartitionedSortedList<int, int>>& sortedList,
            __in int startingValue,
            __in int endingValue)
        {
            int count = 0;
            Random random;
            for (int i = startingValue; i < endingValue; i++)
            {
                int outValue = -1;
                bool isSuccess = (random.Next(2) == 0 ? sortedList->ContainsKey(i) : sortedList->TryGetValue(i, outValue));

                if ((i - startingValue) % 2 == 1)
                {
                    CODING_ERROR_ASSERT(!isSuccess);
                }
                else
                {
                    CODING_ERROR_ASSERT(isSuccess);
                    count++;
                }
            }

            VERIFY_ARE_EQUAL(count, sortedList->Count());
        }

        static void TryGetValueTest(
            __in KSharedPtr<PartitionedSortedList<int, int>>& sortedList,
            __in int numberOfItems,
            __in int numberOfReads,
            __in bool isRandom)
        {
            AddItems(sortedList, 0, numberOfItems);
            VERIFY_ARE_EQUAL(numberOfItems, sortedList->Count());

            Random random;
            for (int i = 0; i < numberOfReads; i++)
            {
                int key = isRandom ? random.Next(0, numberOfItems) : i % numberOfItems;
                int value = -1;
                bool isSuccess = sortedList->TryGetValue(key, value);

                CODING_ERROR_ASSERT(isSuccess);
                VERIFY_ARE_EQUAL(key, value);
            }

            VERIFY_ARE_EQUAL(numberOfItems, sortedList->Count());
        }

    private:
        KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> PartitionedList_Add_ShouldSucceed_Test()
        {
            KAllocator& allocator = PartitionSortedListTest::GetAllocator();

            KSharedPtr<IntComparer> comparerSptr = nullptr;
            NTSTATUS status = IntComparer::Create(allocator, comparerSptr);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

            KSharedPtr<PartitionedList<int, int>> partionedListSPtr = nullptr;
            PartitionedList<int, int>::Create(8192, allocator, partionedListSPtr);

            int expectedlastKey = 5;

            for (int i = 0; i <= 5; i++)
            {
                partionedListSPtr->Add(i, i);
            }

            int actualLastKey = partionedListSPtr->GetLastKey();
            CODING_ERROR_ASSERT(expectedlastKey == actualLastKey);
            co_return;
        }

        ktl::Awaitable<void> Add_Update_ShouldSucceed_Test()
        {
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();

            int key = 5;
            int value = 20;
            int updatedValue = 6;

            sortedListSptr->Add(key, value);
            int val = sortedListSptr->GetValue(key);
            CODING_ERROR_ASSERT(val == value);

            sortedListSptr->UpdateValue(key, updatedValue);
            int actualUpdatedValue = sortedListSptr->GetValue(key);
            CODING_ERROR_ASSERT(actualUpdatedValue == updatedValue);

            int actualValue = -1;
            sortedListSptr->TryGetValue(key, actualValue);
            CODING_ERROR_ASSERT(actualValue == updatedValue);

            actualValue = -1;
            sortedListSptr->TryGetValue(key, actualValue);
            CODING_ERROR_ASSERT(actualValue == updatedValue);

            VERIFY_ARE_EQUAL(1, sortedListSptr->Count());
            co_return;
        }

        ktl::Awaitable<void> Add_Enumerate_ShouldContainAllTheKeys_Test()
        {
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            AddItems(sortedListSptr, 0, 10);
            VerifyItems(sortedListSptr, 0, 10);
            co_return;
        }

        ktl::Awaitable<void> Add_OneItems_WithNullValue_Test()
        {
            KSharedPtr<PartitionedSortedList<int, int*>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int*>();
            sortedListSptr->Add(0, nullptr);
            int* outValue = nullptr;
            bool isSuccess = sortedListSptr->TryGetValue(0, outValue);
            CODING_ERROR_ASSERT(isSuccess);

            VERIFY_ARE_EQUAL(1, sortedListSptr->Count());
            co_return;
        }

        ktl::Awaitable<void> Add_MillionItems_ShouldContainsAllTheAddedElements_Test()
        {
            int numberOfItems = 1000000;
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            AddItems(sortedListSptr, 0, numberOfItems);
            VerifyItems(sortedListSptr, 0, numberOfItems);
            co_return;
        }

        ktl::Awaitable<void> ContainsKey_EmptyListItemExsit_False_Test()
        {
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            bool isSuccess = sortedListSptr->ContainsKey(0);
            CODING_ERROR_ASSERT(!isSuccess);
            co_return;
        }

        ktl::Awaitable<void> ContainsKey_OneItemNotAddExist_False_Test()
        {
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            sortedListSptr->Add(0, 0);

            bool isSuccess = sortedListSptr->ContainsKey(-1);
            CODING_ERROR_ASSERT(!isSuccess);
            co_return;
        }

        ktl::Awaitable<void> ContainsKey_OneItemAddIntExsit_True_Test()
        {
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            sortedListSptr->Add(0, 1);

            bool isSuccess = sortedListSptr->ContainsKey(0);
            CODING_ERROR_ASSERT(isSuccess);
            int value = sortedListSptr->GetValue(0);
            CODING_ERROR_ASSERT(value == 1);
            co_return;
        }

        ktl::Awaitable<void> ContainsKey_InOrderInt_True_Test()
        {
            int numberOfItems = 100000;
            int numberOfReads = 1000000;

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            ContainsKeyTest(sortedListSptr, numberOfItems, numberOfReads, false);
            co_return;
        }

        ktl::Awaitable<void> ContainsKey_RandomInt_True_Test()
        {
            int numberOfItems = 100000;
            int numberOfReads = 1000000;

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            ContainsKeyTest(sortedListSptr, numberOfItems, numberOfReads, true);
            co_return;
        }

        ktl::Awaitable<void> Enumeration_mutipleTimes_shouldSucceed_Test()
        {
            int numberOfItems = 100000;

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            AddItems(sortedListSptr, 0, numberOfItems);
            VerifyItems(sortedListSptr, 0, numberOfItems);
            VerifyItems(sortedListSptr, 0, numberOfItems);
            co_return;
        }

        ktl::Awaitable<void> Discontinuously_AddItems_Random_Verify_Test()
        {
            int startingValue = 0;
            int endingValue = 1000;
            int times = 1000;

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            DiscontinuouslyAddItems(sortedListSptr, startingValue, endingValue);
            RandomVerifyDiscontinuouslyAdd(sortedListSptr, startingValue, endingValue, times);
            co_return;
        }

        ktl::Awaitable<void> Discontinuously_AddItems_Random_CallGetKeyAndValue_Test()
        {
            int startingValue = 0;
            int endingValue = 1000;

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            DiscontinuouslyAddItems(sortedListSptr, startingValue, endingValue);
            VerifyRandomCallGetKeyAndValue(sortedListSptr, startingValue, endingValue);
            co_return;
        }

        ktl::Awaitable<void> TryGetValue_InOrder_Int_True_Test()
        {
            int numberOfItems = 100000;
            int numberOfReads = 1000000;

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            TryGetValueTest(sortedListSptr, numberOfItems, numberOfReads, false);
            co_return;
        }

        ktl::Awaitable<void> TryGetValue_Random_Int_True_Test()
        {
            int numberOfItems = 100000;
            int numberOfReads = 1000000;

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
            TryGetValueTest(sortedListSptr, numberOfItems, numberOfReads, true);
            co_return;
        }

        ktl::Awaitable<void> TryGetIndex_ShouldSucceed_Test()
        {
            // List: [ [2,4,6], [8,10,12], [14,16,18] ]
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>(3);
            for (int i = 2; i <= 18; i += 2)
            {
                sortedListSptr->Add(i, i);
            }

            Index expectedIndexes[] = {
                Index(~0, ~0), Index(0, 0),
                Index(~0, ~1), Index(0, 1),
                Index(~0, ~2), Index(0, 2),

                Index(~1, ~0), Index(1, 0),
                Index(~1, ~1), Index(1, 1),
                Index(~1, ~2), Index(1, 2),

                Index(~2, ~0), Index(2, 0),
                Index(~2, ~1), Index(2, 1),
                Index(~2, ~2), Index(2, 2),

                Index(~3, ~0)
            };

            for (int key = 1; key <= 19; key++)
            {
                Index expectedIndex = expectedIndexes[key-1];
                Index actualIndex(-100, -100);
                bool indexFound = sortedListSptr->TryGetIndex(key, actualIndex);
                CODING_ERROR_ASSERT(indexFound == (expectedIndex.PartitionIndex >= 0));
                CODING_ERROR_ASSERT(expectedIndex.PartitionIndex == actualIndex.PartitionIndex);
                CODING_ERROR_ASSERT(expectedIndex.ItemIndex == actualIndex.ItemIndex);

                auto item = KeyValuePair<int, int>(-1, -1);
                bool valueFound = sortedListSptr->TryGetKeyValue(actualIndex, item);
                CODING_ERROR_ASSERT(indexFound == valueFound);
                if (valueFound)
                {
                    CODING_ERROR_ASSERT(item.Key == key);
                    CODING_ERROR_ASSERT(item.Value == key);
                }
            }
            co_return;
        }

        ktl::Awaitable<void> Enumerate_ShouldSucceed_Test()
        {
            // List: [ [2,4,6], [8,10,12], [14,16,18] ]
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>(3);
            for (int i = 2; i <= 18; i += 2)
            {
                sortedListSptr->Add(i, i);
            }

            for (int startKey = 1; startKey <= 19; startKey++)
            {
                auto enumerator = sortedListSptr->GetEnumerator();
                bool moved = enumerator->MoveTo(KeyValuePair<int, int>(startKey, -1));

                int firstKey = (startKey % 2 == 0) ? startKey : startKey + 1;

                if (firstKey > 18)
                {
                    CODING_ERROR_ASSERT(moved == false);
                    bool hasNext = enumerator->MoveNext();
                    CODING_ERROR_ASSERT(hasNext == false);
                    continue;
                }

                CODING_ERROR_ASSERT(moved);

                for (int key = firstKey; key <= 18; key += 2)
                {
                    bool hasNext = enumerator->MoveNext();

                    CODING_ERROR_ASSERT(hasNext);

                    KeyValuePair<int, int> item = enumerator->Current();
                    CODING_ERROR_ASSERT(item.Key == key);
                    CODING_ERROR_ASSERT(item.Value == key);
                }

                bool hasNext = enumerator->MoveNext();
                CODING_ERROR_ASSERT(hasNext == false);
            }
            co_return;
        }

        ktl::Awaitable<void> EnumerateKeys_ShouldSucceed_Test()
        {
            // List: [ [2,4,6], [8,10,12], [14,16,18] ]
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>(3);
            for (int i = 2; i <= 18; i += 2)
            {
                sortedListSptr->Add(i, i);
            }

            for (int startKey = 1; startKey <= 19; startKey++)
            {
                auto keyValueEnumerator = sortedListSptr->GetEnumerator();

                PartitionedSortedListKeysFilterableEnumerator<int, int>::SPtr enumerator = nullptr;
                PartitionedSortedListKeysFilterableEnumerator<int, int>::Create(*keyValueEnumerator, GetAllocator(), enumerator);

                bool moved = enumerator->MoveTo(startKey);

                int firstKey = (startKey % 2 == 0) ? startKey : startKey + 1;

                if (firstKey > 18)
                {
                    CODING_ERROR_ASSERT(moved == false);
                    bool hasNext = enumerator->MoveNext();
                    CODING_ERROR_ASSERT(hasNext == false);
                    continue;
                }

                CODING_ERROR_ASSERT(moved);

                for (int key = firstKey; key <= 18; key += 2)
                {
                    bool hasNext = enumerator->MoveNext();

                    CODING_ERROR_ASSERT(hasNext);

                    int item = enumerator->Current();
                    CODING_ERROR_ASSERT(item == key);
                }

                bool hasNext = enumerator->MoveNext();
                CODING_ERROR_ASSERT(hasNext == false);
            }
            co_return;
        }

        ktl::Awaitable<void> Enumerate_EmptyList_Test()
        {
            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>(3);

            auto keyValueEnumerator = sortedListSptr->GetEnumerator();

            PartitionedSortedListKeysFilterableEnumerator<int, int>::SPtr enumerator = nullptr;
            PartitionedSortedListKeysFilterableEnumerator<int, int>::Create(*keyValueEnumerator, GetAllocator(), enumerator);

            CODING_ERROR_ASSERT(enumerator->MoveNext() == false);

            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(PartitionSortedListAddTestSuite, PartitionSortedListTest)

    BOOST_AUTO_TEST_CASE(PartitionedList_Add_ShouldSucceed)
    {
        SyncAwait(PartitionedList_Add_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_Update_ShouldSucceed)
    {
        SyncAwait(Add_Update_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_Enumerate_ShouldContainAllTheKeys)
    {
        SyncAwait(Add_Enumerate_ShouldContainAllTheKeys_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_OneItems_WithNullValue)
    {
        SyncAwait(Add_OneItems_WithNullValue_Test());
    }

    //TODO:NULL Key is not applicable currently.
    //BOOST_AUTO_TEST_CASE(Add_OneItems_WithNullKey)
    //{
    //    KSharedPtr<PartitionedSortedList<int*, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int*, int>();

    //    try
    //    {
    //        sortedListSptr->Add(nullptr, 0);
    //        CODING_ERROR_ASSERT(false);
    //    }
    //    catch (Exception)
    //    {
    //        VERIFY_ARE_EQUAL(0, sortedListSptr->Count());
    //    }
    //}

    BOOST_AUTO_TEST_CASE(Add_MillionItems_ShouldContainsAllTheAddedElements)
    {
        SyncAwait(Add_MillionItems_ShouldContainsAllTheAddedElements_Test());
    }

    //The size limit feature is not added yet.
    /*BOOST_AUTO_TEST_CASE(Negative_Add_PassLimit_InvalidOperationException)
    {
        int maxNumberOfSubLists = 4;
        int maxSizeOfSubList = 8;
        int numberOfItems = (maxNumberOfSubLists * maxSizeOfSubList) + 1;
        //TODO: Add max size and limit.
        KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int, int>();
        try
        {
            AddItems(sortedListSptr, 0, numberOfItems);
            CODING_ERROR_ASSERT(false);
        }
        catch (Exception)
        {
        }
    }*/
    
    //TODO: NULL key not supported
    /*BOOST_AUTO_TEST_CASE(ContainsKey_Null_Key)
    {
        KSharedPtr<PartitionedSortedList<int*, int>> sortedListSptr = PartitionSortedListTest::CreatePartionedSortedList<int*, int>();
        try
        {
            bool isSuccess = sortedListSptr->ContainsKey(nullptr);
            CODING_ERROR_ASSERT(false);
        }
        catch (Exception)
        {
        }
    }*/

    BOOST_AUTO_TEST_CASE(ContainsKey_EmptyListItemExsit_False)
    {
        SyncAwait(ContainsKey_EmptyListItemExsit_False_Test());
    }

    BOOST_AUTO_TEST_CASE(ContainsKey_OneItemNotAddExist_False)
    {
        SyncAwait(ContainsKey_OneItemNotAddExist_False_Test());
    }

    BOOST_AUTO_TEST_CASE(ContainsKey_OneItemAddIntExsit_True)
    {
        SyncAwait(ContainsKey_OneItemAddIntExsit_True_Test());
    }


    BOOST_AUTO_TEST_CASE(ContainsKey_InOrderInt_True)
    {
        SyncAwait(ContainsKey_InOrderInt_True_Test());
    }

    BOOST_AUTO_TEST_CASE(ContainsKey_RandomInt_True)
    {
        SyncAwait(ContainsKey_RandomInt_True_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumeration_mutipleTimes_shouldSucceed)
    {
        SyncAwait(Enumeration_mutipleTimes_shouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Discontinuously_AddItems_Random_Verify)
    {
        SyncAwait(Discontinuously_AddItems_Random_Verify_Test());
    }

    BOOST_AUTO_TEST_CASE(Discontinuously_AddItems_Random_CallGetKeyAndValue)
    {
        SyncAwait(Discontinuously_AddItems_Random_CallGetKeyAndValue_Test());
    }

    BOOST_AUTO_TEST_CASE(TryGetValue_InOrder_Int_True)
    {
        SyncAwait(TryGetValue_InOrder_Int_True_Test());
    }


    BOOST_AUTO_TEST_CASE(TryGetValue_Random_Int_True)
    {
        SyncAwait(TryGetValue_Random_Int_True_Test());
    }

    BOOST_AUTO_TEST_CASE(TryGetIndex_ShouldSucceed)
    {
        SyncAwait(TryGetIndex_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_ShouldSucceed)
    {
        SyncAwait(Enumerate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(EnumerateKeys_ShouldSucceed)
    {
        SyncAwait(EnumerateKeys_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_EmptyList)
    {
        SyncAwait(Enumerate_EmptyList_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}


