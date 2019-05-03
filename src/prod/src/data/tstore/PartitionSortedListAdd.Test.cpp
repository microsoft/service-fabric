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
    using namespace std;
    using namespace ktl;
    using namespace Data::TStore;
    using namespace TxnReplicator;

    class PartitionSortedListAddTest
    {
    public:
        PartitionSortedListAddTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            KInvariant(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~PartitionSortedListAddTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

    private:
        KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(PartitionSortedListAddTestSuite, PartitionSortedListAddTest)

    BOOST_AUTO_TEST_CASE(PartitionedList_Add_ShouldSucceed)
    {
        auto allocator = PartitionSortedListAddTest::GetAllocator();

        KSharedPtr<IntComparer> comparerSptr = nullptr;
        NTSTATUS status = IntComparer::Create(*allocator, comparerSptr);
        KInvariant(status == STATUS_SUCCESS);

        KSharedPtr<PartitionedList<int, int>> partionedListSPtr = nullptr;
        PartitionedList<int, int>::Create(8192, *allocator, partionedListSPtr);

        int expectedlastKey = 5;

        for (int i = 0; i <= 5; i++)
        {
            partionedListSPtr->Add(i, i);
        }

        int actualLastKey = partionedListSPtr->GetLastKey();

        KInvariant(expectedlastKey == actualLastKey);
    }

    BOOST_AUTO_TEST_CASE(PartitionedSortedList_Add_Update_ShouldSucceed)
    {
        auto allocator = PartitionSortedListAddTest::GetAllocator();
        try
        {
            KSharedPtr<IntComparer> comparerSptr = nullptr;
            NTSTATUS status = IntComparer::Create(*allocator, comparerSptr);
            KInvariant(status == STATUS_SUCCESS);

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = nullptr;
            status = PartitionedSortedList<int, int>::Create(*comparerSptr, *allocator, sortedListSptr);
            KInvariant(status == STATUS_SUCCESS);

            int key = 5;
            int value = 5;
            int updatedValue = 6;

            sortedListSptr->Add(key, value);
            sortedListSptr->UpdateValue(key, updatedValue);
            int actualUpdatedValue = sortedListSptr->GetValue(key);
            KInvariant(actualUpdatedValue == updatedValue);

            int actualValue = -1;
            sortedListSptr->TryGetValue(key, actualValue);
            KInvariant(actualValue == updatedValue);

            actualValue = -1;
            sortedListSptr->TryGetValue(key, actualValue);
            KInvariant(actualValue == updatedValue);
        }
        catch (Exception & e)
        {
            KDynStringA exceptionMessage(*allocator);
            bool result = e.ToString(exceptionMessage);
            if (result)
            {
                puts(exceptionMessage);
            }

            throw;
        }
    }

    BOOST_AUTO_TEST_CASE(PartitionedSortedList_Add_Enumerate_ShouldContainAllTheKeys)
    {
        auto allocator = PartitionSortedListAddTest::GetAllocator();
        try
        {
            KSharedPtr<IntComparer> comparerSptr = nullptr;
            NTSTATUS status = IntComparer::Create(*allocator, comparerSptr);
            KInvariant(status == STATUS_SUCCESS);

            KSharedPtr<PartitionedSortedList<int, int>> sortedListSptr = nullptr;
            status = PartitionedSortedList<int, int>::Create(*comparerSptr, *allocator, sortedListSptr);
            KInvariant(status == STATUS_SUCCESS);

            for (int i = 0; i < 10; i++)
            {
                sortedListSptr->Add(i, i);
            }

            auto enumeratorSPtr = sortedListSptr->GetKeys();

            int expectedKey = 0;
            while (enumeratorSPtr->MoveNext() != false)
            {
                int key = enumeratorSPtr->Current();
                KInvariant(expectedKey == key);
                expectedKey++;
            }
        }
        catch (Exception & e)
        {
            KDynStringA exceptionMessage(*allocator);
            bool result = e.ToString(exceptionMessage);
            if (result)
            {
                puts(exceptionMessage);
            }

            throw;
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}


