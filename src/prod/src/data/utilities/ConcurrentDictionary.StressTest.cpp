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

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_Add1)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(1, 1, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(5, 1, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(1, 1, 2, 50000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(1, 1, 5, 20000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(4, 0, 4, 20000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(16, 31, 4, 20000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(64, 5, 5, 50000);
        ConcurrentDictionaryTest::RunDictionaryTest_Add1(5, 5, 5, 250000);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_Update1)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(1, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(5, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(1, 2, 50000);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(1, 5, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(4, 4, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(15, 5, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(64, 5, 50000);
        ConcurrentDictionaryTest::RunDictionaryTest_Update1(5, 5, 250000);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_Read1)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(1, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(5, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(1, 2, 50000);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(1, 5, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(4, 4, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(15, 5, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(64, 5, 50000);
        ConcurrentDictionaryTest::RunDictionaryTest_Read1(5, 5, 250000);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_Remove1)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(1, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(5, 1, 10000);
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(1, 5, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(4, 4, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(15, 5, 20001);
        ConcurrentDictionaryTest::RunDictionaryTest_Remove1(64, 5, 50000);
    }

    BOOST_AUTO_TEST_CASE(RunDictionaryTest_GetOrAdd)
    {
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(1, 1, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(5, 1, 1, 100000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(1, 1, 2, 50000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(1, 1, 5, 20000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(4, 0, 4, 20000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(16, 31, 4, 20000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(64, 5, 5, 50000);
        ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(5, 5, 5, 250000);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
