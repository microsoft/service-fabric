// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "WaitHandle.h"

namespace Common
{
    BOOST_AUTO_TEST_SUITE(TestWaitHandle)

    BOOST_AUTO_TEST_CASE(AutoResetEventTest)
    {
        AutoResetEvent a(false);

        BOOST_REQUIRE(a.WaitOne(0) == false);

        a.Set();
        BOOST_REQUIRE(a.WaitOne(0));

        BOOST_REQUIRE(a.WaitOne(0) == false);
    }

    BOOST_AUTO_TEST_CASE(ManualResetEventTest)
    {
        ManualResetEvent a(false);

        BOOST_REQUIRE(a.WaitOne(0) == false);

        a.Set();
        BOOST_REQUIRE(a.WaitOne(0));
        BOOST_REQUIRE(a.WaitOne(0));

        a.Reset();
        BOOST_REQUIRE(a.WaitOne(0) == false);
    }

    BOOST_AUTO_TEST_SUITE_END()
} // end namespace Common
