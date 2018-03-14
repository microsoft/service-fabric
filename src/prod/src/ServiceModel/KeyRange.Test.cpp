// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;
    using namespace Federation;
    using namespace Naming;
    using namespace Reliability;

    class KeyRangeTest
    {
    protected:
    };

    BOOST_FIXTURE_TEST_SUITE(KeyRangeTestSuite, KeyRangeTest)

    BOOST_AUTO_TEST_CASE(KeyRangeWithOneElement)
    {
        KeyRange r(0, 0);
        VERIFY_IS_TRUE(r.ContainsKey(0), L"0");
        VERIFY_IS_FALSE(r.ContainsKey(1), L"1");
        VERIFY_IS_FALSE(r.ContainsKey(-1), L"-1");
        VERIFY_IS_FALSE(r.ContainsKey(std::numeric_limits<__int64>::min()), L"min");
        VERIFY_IS_FALSE(r.ContainsKey(std::numeric_limits<__int64>::max()), L"max");
    }

    BOOST_AUTO_TEST_CASE(KeyRangeIsInclusive)
    {
        KeyRange r(-2, 2);
        VERIFY_IS_TRUE(r.ContainsKey(0), L"0");
        VERIFY_IS_TRUE(r.ContainsKey(1), L"1");
        VERIFY_IS_TRUE(r.ContainsKey(-1), L"-1");
        VERIFY_IS_TRUE(r.ContainsKey(2), L"2");
        VERIFY_IS_TRUE(r.ContainsKey(-2), L"-2");
        VERIFY_IS_FALSE(r.ContainsKey(3), L"3");
        VERIFY_IS_FALSE(r.ContainsKey(-3), L"-3");
        VERIFY_IS_FALSE(r.ContainsKey(std::numeric_limits<__int64>::min()), L"min");
        VERIFY_IS_FALSE(r.ContainsKey(std::numeric_limits<__int64>::max()), L"max");
    }

    BOOST_AUTO_TEST_CASE(KeyRangeWithTwoElements)
    {
        KeyRange r(5, 6);
        VERIFY_IS_TRUE(r.ContainsKey(5), L"5");
        VERIFY_IS_TRUE(r.ContainsKey(6), L"6");
        VERIFY_IS_FALSE(r.ContainsKey(4), L"4");
        VERIFY_IS_FALSE(r.ContainsKey(7), L"7");
        VERIFY_IS_FALSE(r.ContainsKey(std::numeric_limits<__int64>::min()), L"min");
        VERIFY_IS_FALSE(r.ContainsKey(std::numeric_limits<__int64>::max()), L"max");
    }

    BOOST_AUTO_TEST_SUITE_END()
}
