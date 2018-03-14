// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "../testcommon/TestCommon.Public.h"
#include "ConcurrentDictionaryTest.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data;
    using namespace Data::Utilities;

    BOOST_FIXTURE_TEST_SUITE(ConcurrentDictionaryTestSuite, ConcurrentDictionaryTest)

    BOOST_AUTO_TEST_CASE(Api_AddGetRemove_Success)
    {
        NTSTATUS status;
        KAllocator& allocator = ConcurrentDictionaryTest::GetAllocator();

        // Expected
        ULONG expectedKey = 10;
        ULONG expectedValue = 20;

        ConcurrentDictionary<ULONG, ULONG>::SPtr concurrentDictionarySPtr = nullptr;
        status = ConcurrentDictionary<ULONG, ULONG>::Create(allocator, concurrentDictionarySPtr);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        bool addResult = concurrentDictionarySPtr->TryAdd(expectedKey, expectedValue);
        CODING_ERROR_ASSERT(addResult == true);

        ULONG outValue = 0;
        bool getResult = concurrentDictionarySPtr->TryGetValue(expectedKey, outValue);
        CODING_ERROR_ASSERT(getResult == true);
        CODING_ERROR_ASSERT(outValue == expectedValue);

        bool removeResult = concurrentDictionarySPtr->TryRemove(expectedKey, outValue);
        CODING_ERROR_ASSERT(removeResult == true);
        CODING_ERROR_ASSERT(outValue == expectedValue);
    }

    BOOST_AUTO_TEST_CASE(AddGetRemove_KUriCSPtrKey_Success)
    {
        NTSTATUS status;
        KAllocator& allocator = ConcurrentDictionaryTest::GetAllocator();

        // Expected
        KUri::CSPtr itemA1;
        KUri::Create(KStringView(L"fabric:/a"), allocator, (KUri::SPtr&)itemA1);

        KUri::CSPtr itemA2;
        KUri::Create(KStringView(L"fabric:/a"), allocator, (KUri::SPtr&)itemA2);

        KUri::CSPtr itemB;
        KUri::Create(KStringView(L"fabric:/b"), allocator, (KUri::SPtr&)itemB);

        // Setup
        ConcurrentDictionary<KUri::CSPtr, KUri::CSPtr>::SPtr concurrentDictionarySPtr = nullptr;
        status = ConcurrentDictionary<KUri::CSPtr, KUri::CSPtr>::Create(allocator, concurrentDictionarySPtr);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        // Adds
        bool addResult = concurrentDictionarySPtr->TryAdd(itemA1, itemA1);
        CODING_ERROR_ASSERT(addResult);

        addResult = concurrentDictionarySPtr->TryAdd(itemA2, itemA2);
        CODING_ERROR_ASSERT(addResult == false);

        addResult = concurrentDictionarySPtr->TryAdd(itemB, itemB);
        CODING_ERROR_ASSERT(addResult);

        // TryGetValue
        KUri::CSPtr outA;
        bool getResult = concurrentDictionarySPtr->TryGetValue(itemA1, outA);
        CODING_ERROR_ASSERT(getResult == true);
        CODING_ERROR_ASSERT(itemA1->Compare(*outA) == TRUE);

        // TryRemove
        KUri::CSPtr outB;
        bool removeResult = concurrentDictionarySPtr->TryRemove(itemB, outB);
        CODING_ERROR_ASSERT(removeResult);
        CODING_ERROR_ASSERT(itemB->Compare(*outB) == TRUE);

        // Count
        CODING_ERROR_ASSERT(concurrentDictionarySPtr->Count == 1);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_Add1)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(1, 1, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(5, 1, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(1, 1, 2, 5000);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_Update1)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(1, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(5, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(1, 2, 5000);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_Read1)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(1, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(5, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(1, 2, 5000);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_Remove1)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(1, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(5, 1, 1000);
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(1, 5, 2001);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_GetOrAdd)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(1, 1, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(5, 1, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(1, 1, 2, 5000);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
