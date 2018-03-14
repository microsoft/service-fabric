// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data;
    using namespace Data::Utilities;

    class SortTest
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        enum Mode
        {
            Ascending = 0,
            Descending = 1,
            Random = 2,
            Same = 3
        };

        static LONG32 Compare(__in LONG32 const & itemOne, __in LONG32 const & itemTwo)
        {
            return itemOne > itemTwo ? 1 : (itemOne < itemTwo ? -1 : 0);
        }

        SortTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~SortTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        KSharedArray<LONG32>::SPtr CreateLONG32Array()
        {
            KSharedArray<LONG32>::SPtr outputArraySPtr = _new(SORTTEST_TAG, GetAllocator()) KSharedArray<LONG32>();
            CODING_ERROR_ASSERT(outputArraySPtr != nullptr);
            CODING_ERROR_ASSERT(NT_SUCCESS(outputArraySPtr->Status()));
            return outputArraySPtr;
        }

        void Populate(__in Mode type, __in LONG32 count, __inout KSharedArray<LONG32>::SPtr const & array)
        {
            switch (type)
            {
            case Mode::Ascending:
            {
                for (LONG32 i = 0; i < count; i++)
                {
                    NTSTATUS status = array->Append(i);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
                break;
            }
            case Mode::Descending:
            {
                for (LONG32 i = count - 1; i >= 0; i--)
                {
                    NTSTATUS status = array->Append(i);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
                break;
            }
            case Mode::Random:
            {
                Common::Random random_ = Common::Random();

                for (LONG32 i = 0; i < count; i++)
                {
                    LONG32 randomNum = static_cast<LONG32>(random_.Next());
                    NTSTATUS status = array->Append(randomNum);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
                break;
            }
            case Mode::Same:
            {
                for (LONG32 i = 0; i < count; i++)
                {
                    NTSTATUS status = array->Append(1);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
                break;
            }
            }
        }

        void Verify(__in bool ascending, __in KSharedArray<LONG32>::SPtr const & array)
        {
            Verify(0, array->Count() - 1, ascending, array);
        }

        void Verify(__in ULONG left, __in ULONG right, __in bool ascending, __in KSharedArray<LONG32>::SPtr const & array)
        {
            LONG32 sign = ascending ? -1 : 1;
            for (ULONG i = left; i <= right; i++)
            {
                if (i == left)
                {
                    continue;
                }
                else
                {
                    CODING_ERROR_ASSERT(sign * (*array)[i] <= (*array)[i - 1]);
                }
            }
        }

    private:
        KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(SortTestSuite, SortTest)

    //
    // Scenario:        Sort the empty array
    // Expected Result: Array is empty
    // 
    BOOST_AUTO_TEST_CASE(Sort_Empty_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Sort<LONG32>::QuickSort(false, Compare, array);

        CODING_ERROR_ASSERT(array->Count() == 0);
    }

    //
    // Scenario:        Sort the one item array
    // Expected Result: Array has one item
    // 
    BOOST_AUTO_TEST_CASE(Sort_OneItem_Success)
    {
        NTSTATUS status;

        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        status = array->Append(1);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Sort<LONG32>::QuickSort(false, Compare, array);

        CODING_ERROR_ASSERT(array->Count() == 1);
    }

    //
    // Scenario:        Sort all items in a ascending array
    // Expected Result: All items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_AllAscendingItems_Descending_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Ascending, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(false, Compare, array);

        Verify(false, array);
    }

    //
    // Scenario:        Sort all items in a ascending array
    // Expected Result: All items in ascending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_AllAscendingItems_Ascending_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Ascending, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(true, Compare, array);

        Verify(true, array);
    }

    //
    // Scenario:        Sort all items in a descending array
    // Expected Result: All items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_AllDescendingItems_Descending_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Descending, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(false, Compare, array);

        Verify(false, array);
    }

    //
    // Scenario:        Sort all items in a descending array
    // Expected Result: All items in ascending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_AllDescendingItems_Ascending_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Descending, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(true, Compare, array);

        Verify(true, array);
    }

    //
    // Scenario:        Sort all items in a random array
    // Expected Result: All items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_AllRandomItems_Descending_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Random, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(false, Compare, array);
       
        Verify(false, array);
    }

    //
    // Scenario:        Sort all items in a random array
    // Expected Result: All items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_AllRandomItems_Ascending_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Random, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(true, Compare, array);

        Verify(true, array);
    }

    //
    // Scenario:        Sort all items in a array which all items are the same
    // Expected Result: All items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_AllItems_Same_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Same, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(false, Compare, array);

        Verify(false, array);
    }

    //
    // Scenario:        Sort partial items in a ascending array
    // Expected Result: Partial items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_PartialItems_Ascending_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Ascending, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(0, static_cast<ULONG>(5), false, Compare, array);

        Verify(0, static_cast<ULONG>(5), false, array);
    }

    //
    // Scenario:        Sort partial items in a descending array
    // Expected Result: Partial items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_PartialItems_Descending_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Descending, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(0, static_cast<ULONG>(5), false, Compare, array);

        Verify(0, static_cast<ULONG>(5), false, array);
    }

    //
    // Scenario:        Sort partial items in a random array
    // Expected Result: Partial items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_PartialItems_Random_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Random, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(0, static_cast<ULONG>(5), false, Compare, array);

        Verify(0, static_cast<ULONG>(5), false, array);
    }

    //
    // Scenario:        Sort partial items in a array which all items are the same
    // Expected Result: Partial items in descending order
    // 
    BOOST_AUTO_TEST_CASE(Sort_PartialItems_Same_Success)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();

        Populate(Mode::Same, static_cast<ULONG>(10), array);

        Sort<LONG32>::QuickSort(0, static_cast<ULONG>(5), false, Compare, array);

        Verify(0, static_cast<ULONG>(5), false, array);
    }

    //
    // Scenario:        The BinarySearch takes in the descending array and one search value and return the position if the item exists,
    //                  Otherwise return the right position to insert the item to keep the array in descending order
    //                  In this test, the array has items, 10, 8, 6, 4, 2, 0
    // Expected Result: So search 11 should return 0, 10 should return 0, 9 should return 1, 7 should return 2, 5 should return 3, 
    //                  3 should return 4, 1 should return 5, 0 should return 5
    // 
    BOOST_AUTO_TEST_CASE(BinarySearch_Descending)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();
        NTSTATUS status;

        for (LONG32 i = 10; i >= 0; i = i - 2)
        {
            status = array->Append(i);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        LONG32 pos = Sort<LONG32>::BinarySearch(16, false, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 0);
        pos = Sort<LONG32>::BinarySearch(11, false, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 0);
        pos = Sort<LONG32>::BinarySearch(10, false, Compare, array);
        CODING_ERROR_ASSERT(pos == 0);
        pos = Sort<LONG32>::BinarySearch(9, false, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 1);
        pos = Sort<LONG32>::BinarySearch(7, false, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 2);
        pos = Sort<LONG32>::BinarySearch(5, false, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 3);
        pos = Sort<LONG32>::BinarySearch(3, false, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 4);
        pos = Sort<LONG32>::BinarySearch(1, false, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 5);
        pos = Sort<LONG32>::BinarySearch(0, false, Compare, array);
        CODING_ERROR_ASSERT(pos == 5);
    }

    //
    // Scenario:        The BinarySearch takes in the ascending array and one search value and return the position if the item exists,
    //                  Otherwise return the right position to insert the item to keep the array in ascending order
    //                  In this test, the array has items, 1, 3, 5, 7, 9
    // Expected Result: So search 0 should return 0, 1 should return 0, 2 should return 1, 4 should return 2, 6 should return 3, 
    //                  8 should return 4, 10 should return 5
    // 
    BOOST_AUTO_TEST_CASE(BinarySearch_Ascending)
    {
        KSharedArray<LONG32>::SPtr array = CreateLONG32Array();
        NTSTATUS status;

        for (LONG32 i = 1; i <= 9; i = i + 2)
        {
            status = array->Append(i);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        LONG32 pos = Sort<LONG32>::BinarySearch(0, true, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 0);
        pos = Sort<LONG32>::BinarySearch(1, true, Compare, array);
        CODING_ERROR_ASSERT(pos == 0);
        pos = Sort<LONG32>::BinarySearch(2, true, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 1);
        pos = Sort<LONG32>::BinarySearch(4, true, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 2);
        pos = Sort<LONG32>::BinarySearch(6, true, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 3);
        pos = Sort<LONG32>::BinarySearch(8, true, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 4);
        pos = Sort<LONG32>::BinarySearch(10, true, Compare, array);
        CODING_ERROR_ASSERT(pos < 0 && (~pos) == 5);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
