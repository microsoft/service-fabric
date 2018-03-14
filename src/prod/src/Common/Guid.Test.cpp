// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"


namespace Common
{
    using namespace std;

    class GuidTests
    {
    };
    BOOST_FIXTURE_TEST_SUITE(GuidTestsSuite,GuidTests)

    BOOST_AUTO_TEST_CASE(TestGuidSerializeDeserialize)
    {
        Guid g1 = Guid::NewGuid();
        wstring str = g1.ToString();
        Guid g2;
        Guid::TryParse(str, g2);
        VERIFY_ARE_EQUAL(g1.ToStringA(), g2.ToStringA());
    }

    BOOST_AUTO_TEST_CASE(ToString_Format_N_Success)
    {
        wstring expectedString(L"000007c300060010000102040810217f");

        Guid inputGuid(1987, 6, 16, 0, 1, 2, 4, 8, 16, 33, 127);
        wstring outputString = inputGuid.ToString('N');

        VERIFY_ARE_EQUAL(expectedString, outputString);
    }

    BOOST_AUTO_TEST_CASE(ToString_Format_D_Success)
    {
        wstring expectedString(L"000007c3-0006-0010-0001-02040810217f");

        Guid inputGuid(1987, 6, 16, 0, 1, 2, 4, 8, 16, 33, 127);
        wstring outputString = inputGuid.ToString('D');

        VERIFY_ARE_EQUAL(expectedString, outputString);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
