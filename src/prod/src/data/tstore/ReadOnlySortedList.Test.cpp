// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TStoreTests
{
    using namespace std;
    using namespace Common;
    using namespace ktl;
    using namespace Data::Utilities;

    class ReadOnlySortedListTest
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        ReadOnlySortedListTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~ReadOnlySortedListTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        void ValidateEnumerator(IEnumerator<KeyValuePair<int, int>> & enumerator)
        {
            bool hasItem = enumerator.MoveNext();
            if (!hasItem)
            {
                return;
            }
            int previous = enumerator.Current().Key;
            while (enumerator.MoveNext())
            {
                int current = enumerator.Current().Key;
                CODING_ERROR_ASSERT(previous <= current);
                previous = current;
            }
        }

        void ValidateEnumerator(IEnumerator<int> & enumerator)
        {
            bool hasItem = enumerator.MoveNext();
            if (!hasItem)
            {
                return;
            }
            int previous = enumerator.Current();
            while (enumerator.MoveNext())
            {
                int current = enumerator.Current();
                CODING_ERROR_ASSERT(previous <= current);
                previous = current;
            }
        }

    private:
        KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> CreateEmpty_ShouldSucceed_Test()
        {
            ConcurrentDictionary<int, int>::SPtr dictionarySPtr = nullptr;
            ConcurrentDictionary<int, int>::Create(GetAllocator(), dictionarySPtr);

            IntComparer::SPtr intComparerSPtr;
            auto status = IntComparer::Create(GetAllocator(), intComparerSPtr);
            Diagnostics::Validate(status);
            IComparer<int>::SPtr comparerSPtr = intComparerSPtr.DownCast<IComparer<int>>();

            ReadOnlySortedList<int, int>::SPtr listSPtr = nullptr;
            status = ReadOnlySortedList<int, int>::Create(*dictionarySPtr, *comparerSPtr, true, false, GetAllocator(), listSPtr);
            Diagnostics::Validate(status);

            ASSERT_IFNOT(listSPtr->Count == 0, "Expected count to be 0");

            auto enumerator = listSPtr->GetEnumerator();
            CODING_ERROR_ASSERT(enumerator->MoveNext() == false);
            co_return;
        }

        ktl::Awaitable<void> Enumerate_AddInReverseOrder_ShouldBeSorted_Test()
        {
            ConcurrentDictionary<int, int>::SPtr dictionarySPtr = nullptr;
            ConcurrentDictionary<int, int>::Create(GetAllocator(), dictionarySPtr);
        
            ULONG32 numItems = 6;
            for (int i = numItems; i > 0; i--)
            {
                dictionarySPtr->Add(i, i);
            }

            IntComparer::SPtr intComparerSPtr;
            auto status = IntComparer::Create(GetAllocator(), intComparerSPtr);
            Diagnostics::Validate(status);
            IComparer<int>::SPtr comparerSPtr = intComparerSPtr.DownCast<IComparer<int>>();

            ReadOnlySortedList<int, int>::SPtr listSPtr = nullptr;
            status = ReadOnlySortedList<int, int>::Create(*dictionarySPtr, *comparerSPtr, true, false, GetAllocator(), listSPtr);
            Diagnostics::Validate(status);

            ASSERT_IFNOT(listSPtr->Count == numItems, "Expected count to be {0}, got {1}", numItems, listSPtr->Count);

            auto enumerator = listSPtr->GetEnumerator();
            ValidateEnumerator(*enumerator);
            co_return;
        }

        ktl::Awaitable<void> EnumerateKeys_AddInReverseOrder_ShouldBeSorted_Test()
        {
            ConcurrentDictionary<int, int>::SPtr dictionarySPtr = nullptr;
            ConcurrentDictionary<int, int>::Create(GetAllocator(), dictionarySPtr);
        
            ULONG32 numItems = 6;
            for (int i = numItems; i > 0; i--)
            {
                dictionarySPtr->Add(i, i);
            }

            IntComparer::SPtr intComparerSPtr;
            auto status = IntComparer::Create(GetAllocator(), intComparerSPtr);
            Diagnostics::Validate(status);
            IComparer<int>::SPtr comparerSPtr = intComparerSPtr.DownCast<IComparer<int>>();

            ReadOnlySortedList<int, int>::SPtr listSPtr = nullptr;
            status = ReadOnlySortedList<int, int>::Create(*dictionarySPtr, *comparerSPtr, true, false, GetAllocator(), listSPtr);
            Diagnostics::Validate(status);

            ASSERT_IFNOT(listSPtr->Count == numItems, "Expected count to be {0}, got {1}", numItems, listSPtr->Count);

            auto enumerator = listSPtr->GetKeys();
            ValidateEnumerator(*enumerator);
            co_return;
        }

        ktl::Awaitable<void> EnumerateKeys_SortedInput_ShouldBeSorted_Test()
        {
            ConcurrentDictionary<int, int>::SPtr dictionarySPtr = nullptr;
            ConcurrentDictionary<int, int>::Create(GetAllocator(), dictionarySPtr);
        
            ULONG32 numItems = 6;
            for (ULONG32 i = 0; i < numItems; i++)
            {
                dictionarySPtr->Add(i, i);
            }

            IntComparer::SPtr intComparerSPtr;
            auto status = IntComparer::Create(GetAllocator(), intComparerSPtr);
            Diagnostics::Validate(status);
            IComparer<int>::SPtr comparerSPtr = intComparerSPtr.DownCast<IComparer<int>>();

            ReadOnlySortedList<int, int>::SPtr listSPtr = nullptr;
            status = ReadOnlySortedList<int, int>::Create(*dictionarySPtr, *comparerSPtr, true, true, GetAllocator(), listSPtr);
            Diagnostics::Validate(status);

            ASSERT_IFNOT(listSPtr->Count == numItems, "Expected count to be {0}, got {1}", numItems, listSPtr->Count);

            auto enumerator = listSPtr->GetKeys();
            ValidateEnumerator(*enumerator);
            co_return;
        }

        ktl::Awaitable<void> EnumerateKeys_MoveTo_ShouldBeSorted_Test()
        {
            ConcurrentDictionary<int, int>::SPtr dictionarySPtr = nullptr;
            ConcurrentDictionary<int, int>::Create(GetAllocator(), dictionarySPtr);
        
            for (int i = 2; i <= 18; i += 2)
            {
                dictionarySPtr->Add(i, i);
            }

            IntComparer::SPtr intComparerSPtr;
            auto status = IntComparer::Create(GetAllocator(), intComparerSPtr);
            Diagnostics::Validate(status);
            IComparer<int>::SPtr comparerSPtr = intComparerSPtr.DownCast<IComparer<int>>();

            ReadOnlySortedList<int, int>::SPtr listSPtr = nullptr;
            status = ReadOnlySortedList<int, int>::Create(*dictionarySPtr, *comparerSPtr, true, false, GetAllocator(), listSPtr);
            Diagnostics::Validate(status);

            for (int startKey = 1; startKey <= 19; startKey++)
            {
                auto enumerator = listSPtr->GetKeys();
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

        ktl::Awaitable<void> ContainsKeys_AddInReverseOrder_ShouldSucceed_Test()
        {
            ConcurrentDictionary<int, int>::SPtr dictionarySPtr = nullptr;
            ConcurrentDictionary<int, int>::Create(GetAllocator(), dictionarySPtr);
        
            int numItems = 6;
            for (int i = numItems; i > 0; i--)
            {
                dictionarySPtr->Add(i, i);
            }

            IntComparer::SPtr intComparerSPtr;
            auto status = IntComparer::Create(GetAllocator(), intComparerSPtr);
            Diagnostics::Validate(status);
            IComparer<int>::SPtr comparerSPtr = intComparerSPtr.DownCast<IComparer<int>>();

            ReadOnlySortedList<int, int>::SPtr listSPtr = nullptr;
            status = ReadOnlySortedList<int, int>::Create(*dictionarySPtr, *comparerSPtr, true, false, GetAllocator(), listSPtr);
            Diagnostics::Validate(status);
        
            CODING_ERROR_ASSERT(listSPtr->ContainsKey(0) == false);
            for (int i = 1; i <= numItems; i++)
            {
                CODING_ERROR_ASSERT(listSPtr->ContainsKey(i) == true);
            }
            CODING_ERROR_ASSERT(listSPtr->ContainsKey(numItems+1) == false);
            co_return;
        }

        ktl::Awaitable<void> TryGetValue_AddInReverseOrder_ShouldSucceed_Test()
        {
            ConcurrentDictionary<int, int>::SPtr dictionarySPtr = nullptr;
            ConcurrentDictionary<int, int>::Create(GetAllocator(), dictionarySPtr);
        
            int numItems = 6;
            for (int i = numItems; i > 0; i--)
            {
                dictionarySPtr->Add(i, i);
            }

            IntComparer::SPtr intComparerSPtr;
            auto status = IntComparer::Create(GetAllocator(), intComparerSPtr);
            Diagnostics::Validate(status);
            IComparer<int>::SPtr comparerSPtr = intComparerSPtr.DownCast<IComparer<int>>();

            ReadOnlySortedList<int, int>::SPtr listSPtr = nullptr;
            status = ReadOnlySortedList<int, int>::Create(*dictionarySPtr, *comparerSPtr, true, false, GetAllocator(), listSPtr);
            Diagnostics::Validate(status);
        
            int result;

            CODING_ERROR_ASSERT(listSPtr->TryGetValue(0, result) == false);
            for (int i = 1; i <= numItems; i++)
            {
                CODING_ERROR_ASSERT(listSPtr->TryGetValue(i, result) == true);
                CODING_ERROR_ASSERT(result == i);
            }
            CODING_ERROR_ASSERT(listSPtr->TryGetValue(numItems+1, result) == false);
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(ReadOnlySortedListTestSuite, ReadOnlySortedListTest)

    BOOST_AUTO_TEST_CASE(CreateEmpty_ShouldSucceed)
    {
        SyncAwait(CreateEmpty_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_AddInReverseOrder_ShouldBeSorted)
    {
        SyncAwait(Enumerate_AddInReverseOrder_ShouldBeSorted_Test());
    }

    BOOST_AUTO_TEST_CASE(EnumerateKeys_AddInReverseOrder_ShouldBeSorted)
    {
        SyncAwait(EnumerateKeys_AddInReverseOrder_ShouldBeSorted_Test());
    }
    
    BOOST_AUTO_TEST_CASE(EnumerateKeys_SortedInput_ShouldBeSorted)
    {
        SyncAwait(EnumerateKeys_SortedInput_ShouldBeSorted_Test());
    }

    BOOST_AUTO_TEST_CASE(EnumerateKeys_MoveTo_ShouldBeSorted)
    {
        SyncAwait(EnumerateKeys_MoveTo_ShouldBeSorted_Test());
    }

    BOOST_AUTO_TEST_CASE(ContainsKeys_AddInReverseOrder_ShouldSucceed)
    {
        SyncAwait(ContainsKeys_AddInReverseOrder_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(TryGetValue_AddInReverseOrder_ShouldSucceed)
    {
        SyncAwait(TryGetValue_AddInReverseOrder_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
