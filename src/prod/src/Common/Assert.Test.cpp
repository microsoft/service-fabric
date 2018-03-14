// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "AssertWF.h"

#pragma warning (disable:4702)

namespace Common
{
    class AssertTests
    {
    };

    BOOST_FIXTURE_TEST_SUITE(AssertTestsSuite,AssertTests)

    BOOST_AUTO_TEST_CASE(ThrowCodingErrorTest)
    {    
        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(Assert::CodingError("A"), std::system_error);
    }    

    BOOST_AUTO_TEST_CASE(AssertTextFormattingTest)
    {
        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        // Formatting cannot be validated during this test. Perhaps trace validation can properly test this.
        VERIFY_THROWS(Common::Assert::CodingError("Text {0}, {1}, {2}", "D", "E", 7), std::system_error);
    }

    BOOST_AUTO_TEST_SUITE_END()
}

#pragma warning (default:4702)
