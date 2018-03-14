// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Math.h"
#include <stdlib.h>

namespace Common
{
    BOOST_AUTO_TEST_SUITE2(MathTests)

    BOOST_AUTO_TEST_CASE(RoundToTest)
    {
        double value = 1234.5678;
        double result = RoundTo(value, 2);
        VERIFY_ARE_EQUAL2(result, 1234.57);

        value = 1234.5678;
        result = RoundTo(value, 4);
        VERIFY_ARE_EQUAL2(result, 1234.5678);

        value = 1234;
        result = RoundTo(value, 0);
        VERIFY_ARE_EQUAL2(result, 1234);

        value = 1234.5;
        result = RoundTo(value, 1);
        VERIFY_ARE_EQUAL2(result, 1234.5);

        value = -45.67;
        result = RoundTo(value, 1);
        VERIFY_ARE_EQUAL2(result, -45.7);

        value = -1.12;
        result = RoundTo(value, 1);
        VERIFY_ARE_EQUAL2(result, -1.1);
    }

    BOOST_AUTO_TEST_SUITE_END()
};
