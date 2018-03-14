// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "LargeInteger.Test.Constants.h"


namespace Common
{
    using namespace std;

    class LargeIntegerTests
    {
    };

    // TODO: Add Roundtrip Test for ToString and TryParse
    // TODO: Add Test for ToString - various numbers to ensure that padding works as expected
    // TODO: Add a test that verifies that we are really using Large Integer and let this test fail if SMALLINTEGER is defined

    // Tests to ensure that the constants defined in the LargeInteger class are correct.
    // It also tests the TryParse method.
    BOOST_FIXTURE_TEST_SUITE(LargeIntegerTestsSuite,LargeIntegerTests)

    BOOST_AUTO_TEST_CASE(ConstantsAreCorrectTest)
    {
        // Test for LargeInteger::MaxValue
        LargeIntegerTestConstants consts;
        LargeInteger maxFromString;
        bool retVal = LargeInteger::TryParse(L"ffffffffffffffffffffffffffffffff", maxFromString);

        VERIFY_IS_TRUE(retVal);
        VERIFY_IS_TRUE(consts.MaxMax == maxFromString);

        // Test for LargeInteger::Zero
        LargeInteger zeroFromString;
        retVal = LargeInteger::TryParse(L"0", zeroFromString);
        VERIFY_IS_TRUE(retVal);
        VERIFY_IS_TRUE(consts.ZeroZero == zeroFromString);

        // Test for LargeInteger::One
        LargeInteger oneFromString;
        retVal = LargeInteger::TryParse(L"1", oneFromString);
        VERIFY_IS_TRUE(retVal);
        VERIFY_IS_TRUE(consts.ZeroOne == oneFromString);
    }


    // Tests to ensure that the comparison operators are correct.
    BOOST_AUTO_TEST_CASE(ComparisonOperatrsAreCorrectTest)
    {
        LargeIntegerTestConstants consts;

        VERIFY_IS_TRUE(consts.MaxMax > consts.MaxZero);
        VERIFY_IS_TRUE(consts.MaxMax >= consts.MaxZero);
        VERIFY_IS_TRUE(consts.MaxMax > consts.ZeroMax);
        VERIFY_IS_TRUE(consts.MaxMax >= consts.ZeroMax);
        VERIFY_IS_TRUE(consts.MaxMax > consts.ZeroZero);
        VERIFY_IS_TRUE(consts.MaxMax >= consts.ZeroZero);
        VERIFY_IS_TRUE(consts.MaxMax == consts.MaxMax);
        VERIFY_IS_TRUE(consts.MaxMax <= consts.MaxMax);
        VERIFY_IS_TRUE(consts.MaxMax >= consts.MaxMax);
        VERIFY_IS_TRUE(consts.MaxMax > consts.Test1);
        VERIFY_IS_TRUE(consts.MaxMax >= consts.Test1);
        VERIFY_IS_TRUE(consts.MaxMax > consts.Test2);
        VERIFY_IS_TRUE(consts.MaxMax >= consts.Test2);
        VERIFY_IS_TRUE(consts.MaxMax > consts.Test3);
        VERIFY_IS_TRUE(consts.MaxMax >= consts.Test3);

        VERIFY_IS_TRUE(consts.Test3 > consts.MaxZero);
        VERIFY_IS_TRUE(consts.Test3 >= consts.MaxZero);
        VERIFY_IS_TRUE(consts.Test3 > consts.ZeroMax);
        VERIFY_IS_TRUE(consts.Test3 >= consts.ZeroMax);
        VERIFY_IS_TRUE(consts.Test3 > consts.ZeroZero);
        VERIFY_IS_TRUE(consts.Test3 >= consts.ZeroZero);
        VERIFY_IS_TRUE(consts.Test3 == consts.Test3);
        VERIFY_IS_TRUE(consts.Test3 <= consts.Test3);
        VERIFY_IS_TRUE(consts.Test3 >= consts.Test3);
        VERIFY_IS_TRUE(consts.Test3 > consts.Test1);
        VERIFY_IS_TRUE(consts.Test3 >= consts.Test1);
        VERIFY_IS_TRUE(consts.Test3 > consts.Test2);
        VERIFY_IS_TRUE(consts.Test3 >= consts.Test2);
        VERIFY_IS_TRUE(consts.Test3 <= consts.MaxMax);
        VERIFY_IS_TRUE(consts.Test3 < consts.MaxMax);

        VERIFY_IS_TRUE(consts.MaxZero <= consts.MaxZero);
        VERIFY_IS_TRUE(consts.MaxZero == consts.MaxZero);
        VERIFY_IS_TRUE(consts.MaxZero >= consts.MaxZero);
        VERIFY_IS_TRUE(consts.MaxZero > consts.ZeroMax);
        VERIFY_IS_TRUE(consts.MaxZero >= consts.ZeroMax);
        VERIFY_IS_TRUE(consts.MaxZero > consts.ZeroZero);
        VERIFY_IS_TRUE(consts.MaxZero >= consts.ZeroZero);
        VERIFY_IS_TRUE(consts.MaxZero <= consts.Test3);
        VERIFY_IS_TRUE(consts.MaxZero < consts.Test3);
        VERIFY_IS_TRUE(consts.MaxZero > consts.Test1);
        VERIFY_IS_TRUE(consts.MaxZero >= consts.Test1);
        VERIFY_IS_TRUE(consts.MaxZero > consts.Test2);
        VERIFY_IS_TRUE(consts.MaxZero >= consts.Test2);
        VERIFY_IS_TRUE(consts.MaxZero <= consts.MaxMax);

        VERIFY_IS_TRUE(consts.Test2 <= consts.MaxZero);
        VERIFY_IS_TRUE(consts.Test2 < consts.MaxZero);
        VERIFY_IS_TRUE(consts.Test2 > consts.ZeroMax);
        VERIFY_IS_TRUE(consts.Test2 >= consts.ZeroMax);
        VERIFY_IS_TRUE(consts.Test2 > consts.ZeroZero);
        VERIFY_IS_TRUE(consts.Test2 >= consts.ZeroZero);
        VERIFY_IS_TRUE(consts.Test2 == consts.Test2);
        VERIFY_IS_TRUE(consts.Test2 <= consts.Test2);
        VERIFY_IS_TRUE(consts.Test2 >= consts.Test2);
        VERIFY_IS_TRUE(consts.Test2 > consts.Test1);
        VERIFY_IS_TRUE(consts.Test2 >= consts.Test1);
        VERIFY_IS_TRUE(consts.Test2 <= consts.Test3);
        VERIFY_IS_TRUE(consts.Test2 < consts.Test3);
        VERIFY_IS_TRUE(consts.Test2 <= consts.MaxMax);
        VERIFY_IS_TRUE(consts.Test2 < consts.MaxMax);

        VERIFY_IS_TRUE(consts.ZeroMax <= consts.MaxZero);
        VERIFY_IS_TRUE(consts.ZeroMax < consts.MaxZero);
        VERIFY_IS_TRUE(consts.ZeroMax == consts.ZeroMax);
        VERIFY_IS_TRUE(consts.ZeroMax <= consts.ZeroMax);
        VERIFY_IS_TRUE(consts.ZeroMax >= consts.ZeroMax);
        VERIFY_IS_TRUE(consts.ZeroMax > consts.ZeroZero);
        VERIFY_IS_TRUE(consts.ZeroMax <= consts.Test3);
        VERIFY_IS_TRUE(consts.ZeroMax < consts.Test3);
        VERIFY_IS_TRUE(consts.ZeroMax <= consts.Test2);
        VERIFY_IS_TRUE(consts.ZeroMax < consts.Test2);
        VERIFY_IS_TRUE(consts.ZeroMax > consts.Test1);
        VERIFY_IS_TRUE(consts.ZeroMax >= consts.Test1);
        VERIFY_IS_TRUE(consts.ZeroMax <= consts.MaxMax);
        VERIFY_IS_TRUE(consts.ZeroMax < consts.MaxMax);

        VERIFY_IS_TRUE(consts.Test1 <= consts.MaxZero);
        VERIFY_IS_TRUE(consts.Test1 < consts.MaxZero);
        VERIFY_IS_TRUE(consts.Test1 <= consts.ZeroMax);
        VERIFY_IS_TRUE(consts.Test1 < consts.ZeroMax);
        VERIFY_IS_TRUE(consts.Test1 > consts.ZeroZero);
        VERIFY_IS_TRUE(consts.Test1 >= consts.ZeroZero);
        VERIFY_IS_TRUE(consts.Test1 == consts.Test1);
        VERIFY_IS_TRUE(consts.Test1 <= consts.Test1);
        VERIFY_IS_TRUE(consts.Test1 >= consts.Test1);
        VERIFY_IS_TRUE(consts.Test1 <= consts.Test2);
        VERIFY_IS_TRUE(consts.Test1 < consts.Test2);
        VERIFY_IS_TRUE(consts.Test1 <= consts.Test3);
        VERIFY_IS_TRUE(consts.Test1 < consts.Test3);
        VERIFY_IS_TRUE(consts.Test1 <= consts.MaxMax);
        VERIFY_IS_TRUE(consts.Test1 < consts.MaxMax);

        VERIFY_IS_TRUE(consts.ZeroZero <= consts.MaxZero);
        VERIFY_IS_TRUE(consts.ZeroZero < consts.MaxZero);
        VERIFY_IS_TRUE(consts.ZeroZero <= consts.ZeroMax);
        VERIFY_IS_TRUE(consts.ZeroZero < consts.ZeroMax);
        VERIFY_IS_TRUE(consts.ZeroZero == consts.ZeroZero);
        VERIFY_IS_TRUE(consts.ZeroZero >= consts.ZeroZero);
        VERIFY_IS_TRUE(consts.ZeroZero <= consts.ZeroZero);
        VERIFY_IS_TRUE(consts.ZeroZero <= consts.Test3);
        VERIFY_IS_TRUE(consts.ZeroZero < consts.Test3);
        VERIFY_IS_TRUE(consts.ZeroZero <= consts.Test2);
        VERIFY_IS_TRUE(consts.ZeroZero < consts.Test2);
        VERIFY_IS_TRUE(consts.ZeroZero <= consts.Test1);
        VERIFY_IS_TRUE(consts.ZeroZero < consts.Test1);
        VERIFY_IS_TRUE(consts.ZeroZero <= consts.MaxMax);
        VERIFY_IS_TRUE(consts.ZeroZero < consts.MaxMax);
    }

    // Tests to ensure that the mathematical operators are correct.
    BOOST_AUTO_TEST_CASE(MathematicalOperatrsAreCorrectTest)
    {
        LargeIntegerTestConstants consts;
        LargeInteger temp = consts.MaxMax;
        VERIFY_IS_TRUE(temp++ == consts.MaxMax);
        VERIFY_IS_TRUE(temp == consts.MaxMaxPOne);
        temp = consts.MaxMax;
        VERIFY_IS_TRUE(++temp == consts.MaxMaxPOne);
        temp = consts.MaxMax;
        VERIFY_IS_TRUE(temp-- == consts.MaxMax);
        VERIFY_IS_TRUE(temp == consts.MaxMaxMOne);
        temp = consts.MaxMax;
        VERIFY_IS_TRUE(--temp == consts.MaxMaxMOne);

        temp = consts.Test3;
        VERIFY_IS_TRUE(temp++ == consts.Test3);
        VERIFY_IS_TRUE(temp == consts.Test3POne);
        temp = consts.Test3;
        VERIFY_IS_TRUE(++temp == consts.Test3POne);
        temp = consts.Test3;
        VERIFY_IS_TRUE(temp-- == consts.Test3);
        VERIFY_IS_TRUE(temp == consts.Test3MOne);
        temp = consts.Test3;
        VERIFY_IS_TRUE(--temp == consts.Test3MOne);

        temp = consts.MaxZero;
        VERIFY_IS_TRUE(temp++ == consts.MaxZero);
        VERIFY_IS_TRUE(temp == consts.MaxZeroPOne);
        temp = consts.MaxZero;
        VERIFY_IS_TRUE(++temp == consts.MaxZeroPOne);
        temp = consts.MaxZero;
        VERIFY_IS_TRUE(temp-- == consts.MaxZero);
        VERIFY_IS_TRUE(temp == consts.MaxZeroMOne);
        temp = consts.MaxZero;
        VERIFY_IS_TRUE(--temp == consts.MaxZeroMOne);

        temp = consts.Test2;
        VERIFY_IS_TRUE(temp++ == consts.Test2);
        VERIFY_IS_TRUE(temp == consts.Test2POne);
        temp = consts.Test2;
        VERIFY_IS_TRUE(++temp == consts.Test2POne);
        temp = consts.Test2;
        VERIFY_IS_TRUE(temp-- == consts.Test2);
        VERIFY_IS_TRUE(temp == consts.Test2MOne);
        temp = consts.Test2;
        VERIFY_IS_TRUE(--temp == consts.Test2MOne);

        temp = consts.ZeroMax;
        VERIFY_IS_TRUE(temp++ == consts.ZeroMax);
        VERIFY_IS_TRUE(temp == consts.ZeroMaxPOne);
        temp = consts.ZeroMax;
        VERIFY_IS_TRUE(++temp == consts.ZeroMaxPOne);
        temp = consts.ZeroMax;
        VERIFY_IS_TRUE(temp-- == consts.ZeroMax);
        VERIFY_IS_TRUE(temp == consts.ZeroMaxMOne);
        temp = consts.ZeroMax;
        VERIFY_IS_TRUE(--temp == consts.ZeroMaxMOne);

        temp = consts.Test1;
        VERIFY_IS_TRUE(temp++ == consts.Test1);
        VERIFY_IS_TRUE(temp == consts.Test1POne);
        temp = consts.Test1;
        VERIFY_IS_TRUE(++temp == consts.Test1POne);
        temp = consts.Test1;
        VERIFY_IS_TRUE(temp-- == consts.Test1);
        VERIFY_IS_TRUE(temp == consts.Test1MOne);
        temp = consts.Test1;
        VERIFY_IS_TRUE(--temp == consts.Test1MOne);

        temp = consts.ZeroZero;
        VERIFY_IS_TRUE(temp++ == consts.ZeroZero);
        VERIFY_IS_TRUE(temp == consts.ZeroZeroPOne);
        temp = consts.ZeroZero;
        VERIFY_IS_TRUE(++temp == consts.ZeroZeroPOne);
        temp = consts.ZeroZero;
        VERIFY_IS_TRUE(temp-- == consts.ZeroZero);
        VERIFY_IS_TRUE(temp == consts.ZeroZeroMOne);
        temp = consts.ZeroZero;
        VERIFY_IS_TRUE(--temp == consts.ZeroZeroMOne);

        VERIFY_IS_TRUE(consts.MaxMax + consts.F == consts.MaxMaxPF);
        VERIFY_IS_TRUE(consts.Test3 + consts.F == consts.Test3PF);
        VERIFY_IS_TRUE(consts.MaxZero + consts.F == consts.MaxZeroPF);
        VERIFY_IS_TRUE(consts.Test2 + consts.F == consts.Test2PF);
        VERIFY_IS_TRUE(consts.ZeroMax + consts.F == consts.ZeroMaxPF);
        VERIFY_IS_TRUE(consts.Test1 + consts.F == consts.Test1PF);
        VERIFY_IS_TRUE(consts.ZeroZero + consts.F == consts.ZeroZeroPF);

        temp = consts.MaxMax;
        temp += consts.F;
        VERIFY_IS_TRUE(temp == consts.MaxMaxPF);
        temp = consts.Test3;
        temp += consts.F;
        VERIFY_IS_TRUE(temp == consts.Test3PF);
        temp = consts.MaxZero;
        temp += consts.F;
        VERIFY_IS_TRUE(temp == consts.MaxZeroPF);
        temp = consts.Test2;
        temp += consts.F;
        VERIFY_IS_TRUE(temp == consts.Test2PF);
        temp = consts.ZeroMax;
        temp += consts.F;
        VERIFY_IS_TRUE(temp == consts.ZeroMaxPF);
        temp = consts.Test1;
        temp += consts.F;
        VERIFY_IS_TRUE(temp == consts.Test1PF);
        temp = consts.ZeroZero;
        temp += consts.F;
        VERIFY_IS_TRUE(temp == consts.ZeroZeroPF);

        VERIFY_IS_TRUE(consts.MaxMax * consts.F == consts.MaxMaxTF);
        VERIFY_IS_TRUE(consts.Test3 * consts.F == consts.Test3TF);
        VERIFY_IS_TRUE(consts.MaxZero * consts.F == consts.MaxZeroTF);
        VERIFY_IS_TRUE(consts.Test2 * consts.F == consts.Test2TF);
        VERIFY_IS_TRUE(consts.ZeroMax * consts.F == consts.ZeroMaxTF);
        VERIFY_IS_TRUE(consts.Test1 * consts.F == consts.Test1TF);
        VERIFY_IS_TRUE(consts.ZeroZero * consts.F == consts.ZeroZeroTF);
        VERIFY_IS_TRUE(consts.ZeroOne * consts.F == consts.ZeroOneTF);
        VERIFY_IS_TRUE(consts.Test2 * consts.Test3 == consts.Test3 * consts.Test2);
        VERIFY_IS_TRUE(consts.Test1 * (consts.Test2 * consts.Test3) == (consts.Test1 * consts.Test2) * consts.Test3);
        VERIFY_IS_TRUE(consts.Test1 * (consts.Test2 + consts.Test3) == consts.Test1 * consts.Test2 + consts.Test1 * consts.Test3);
    }

    // Tests to ensure that the shift operators are correct.
    BOOST_AUTO_TEST_CASE(ShiftOperatrsAreCorrectTest)
    {
        LargeIntegerTestConstants consts;
        VERIFY_IS_TRUE(consts.MaxMax << 4 == consts.MaxMaxL4);
        VERIFY_IS_TRUE(consts.Test3 << 4 == consts.Test3L4);
        VERIFY_IS_TRUE(consts.MaxZero << 4 == consts.MaxZeroL4);
        VERIFY_IS_TRUE(consts.Test2 << 4 == consts.Test2L4);
        VERIFY_IS_TRUE(consts.ZeroMax << 4 == consts.ZeroMaxL4);
        VERIFY_IS_TRUE(consts.Test1 << 4 == consts.Test1L4);
        VERIFY_IS_TRUE(consts.ZeroZero << 4 == consts.ZeroZeroL4);
        VERIFY_IS_TRUE(consts.MaxMax >> 4 == consts.MaxMaxR4);
        VERIFY_IS_TRUE(consts.Test3 >> 4 == consts.Test3R4);
        VERIFY_IS_TRUE(consts.MaxZero >> 4 == consts.MaxZeroR4);
        VERIFY_IS_TRUE(consts.Test2 >> 4 == consts.Test2R4);
        VERIFY_IS_TRUE(consts.ZeroMax >> 4 == consts.ZeroMaxR4);
        VERIFY_IS_TRUE(consts.Test1 >> 4 == consts.Test1R4);
        VERIFY_IS_TRUE(consts.ZeroZero >> 4 == consts.ZeroZeroR4);

        LargeInteger temp = consts.MaxMax;
        temp <<= 4;
        VERIFY_IS_TRUE(temp == consts.MaxMaxL4);
        temp = consts.Test3;
        temp <<= 4;
        VERIFY_IS_TRUE(temp == consts.Test3L4);
        temp = consts.MaxZero;
        temp <<= 4;
        VERIFY_IS_TRUE(temp == consts.MaxZeroL4);
        temp = consts.Test2;
        temp <<= 4;
        VERIFY_IS_TRUE(temp == consts.Test2L4);
        temp = consts.ZeroMax;
        temp <<= 4;
        VERIFY_IS_TRUE(temp == consts.ZeroMaxL4);
        temp = consts.Test1;
        temp <<= 4;
        VERIFY_IS_TRUE(temp == consts.Test1L4);
        temp = consts.ZeroZero;
        temp <<= 4;
        VERIFY_IS_TRUE(temp == consts.ZeroZeroL4);
        temp = consts.MaxMax;
        temp >>= 4;
        VERIFY_IS_TRUE(temp == consts.MaxMaxR4);
        temp = consts.Test3;
        temp >>= 4;
        VERIFY_IS_TRUE(temp == consts.Test3R4);
        temp = consts.MaxZero;
        temp >>= 4;
        VERIFY_IS_TRUE(temp == consts.MaxZeroR4);
        temp = consts.Test2;
        temp >>= 4;
        VERIFY_IS_TRUE(temp == consts.Test2R4);
        temp = consts.ZeroMax;
        temp >>= 4;
        VERIFY_IS_TRUE(temp == consts.ZeroMaxR4);
        temp = consts.Test1;
        temp >>= 4;
        VERIFY_IS_TRUE(temp == consts.Test1R4);
        temp = consts.ZeroZero;
        temp >>= 4;
        VERIFY_IS_TRUE(temp == consts.ZeroZeroR4);
    }

    // Tests to ensure that the logical operators are correct.
    BOOST_AUTO_TEST_CASE(LogicalOperatrsAreCorrectTest)
    {
        LargeIntegerTestConstants consts;

        VERIFY_IS_TRUE((consts.MaxMax & consts.MaxMax) == consts.MaxMax);
        VERIFY_IS_TRUE((consts.MaxMax & consts.MaxZero) == consts.MaxZero);
        VERIFY_IS_TRUE((consts.MaxMax & consts.ZeroMax) == consts.ZeroMax);
        VERIFY_IS_TRUE((consts.MaxMax & consts.ZeroZero) == consts.ZeroZero);
        VERIFY_IS_TRUE((consts.MaxMax & consts.Test1) == consts.Test1);
        VERIFY_IS_TRUE((consts.MaxMax & consts.Test2) == consts.Test2);
        VERIFY_IS_TRUE((consts.MaxMax & consts.Test3) == consts.Test3);

        VERIFY_IS_TRUE((consts.Test1 & consts.MaxZero) == consts.Test1AndMaxZero);
        VERIFY_IS_TRUE((consts.Test1 & consts.ZeroMax) == consts.Test1AndZeroMax);
        VERIFY_IS_TRUE((consts.Test1 & consts.ZeroZero) == consts.ZeroZero);
        VERIFY_IS_TRUE((consts.Test1 & consts.Test1) == consts.Test1);
        VERIFY_IS_TRUE((consts.Test1 & consts.Test2) == consts.Test1AndTest2);
        VERIFY_IS_TRUE((consts.Test1 & consts.Test3) == consts.Test1AndTest3);

        VERIFY_IS_TRUE((consts.MaxZero & consts.MaxZero) == consts.MaxZero);
        VERIFY_IS_TRUE((consts.MaxZero & consts.ZeroMax) == consts.ZeroZero);
        VERIFY_IS_TRUE((consts.MaxZero & consts.ZeroZero) == consts.ZeroZero);
        VERIFY_IS_TRUE((consts.MaxZero & consts.Test2) == consts.Test2AndMaxZero);
        VERIFY_IS_TRUE((consts.MaxZero & consts.Test3) == consts.Test3AndMaxZero);

        VERIFY_IS_TRUE((consts.ZeroMax & consts.ZeroMax) == consts.ZeroMax);
        VERIFY_IS_TRUE((consts.ZeroMax & consts.ZeroZero) == consts.ZeroZero);
        VERIFY_IS_TRUE((consts.ZeroMax & consts.Test2) == consts.Test2AndZeroMax);
        VERIFY_IS_TRUE((consts.ZeroMax & consts.Test3) == consts.Test3AndZeroMax);

        VERIFY_IS_TRUE((consts.ZeroZero & consts.ZeroZero) == consts.ZeroZero);
        VERIFY_IS_TRUE((consts.ZeroZero & consts.Test2) == consts.ZeroZero);
        VERIFY_IS_TRUE((consts.ZeroZero & consts.Test3) == consts.ZeroZero);

        VERIFY_IS_TRUE((consts.Test2 & consts.Test2) == consts.Test2);
        VERIFY_IS_TRUE((consts.Test2 & consts.Test3) == consts.Test2AndTest3);

        VERIFY_IS_TRUE((consts.Test3 & consts.Test3) == consts.Test3);

        VERIFY_IS_TRUE((consts.MaxMax | consts.MaxMax) == consts.MaxMax);
        VERIFY_IS_TRUE((consts.MaxMax | consts.MaxZero) == consts.MaxMax);
        VERIFY_IS_TRUE((consts.MaxMax | consts.ZeroMax) == consts.MaxMax);
        VERIFY_IS_TRUE((consts.MaxMax | consts.ZeroZero) == consts.MaxMax);
        VERIFY_IS_TRUE((consts.MaxMax | consts.Test1) == consts.MaxMax);
        VERIFY_IS_TRUE((consts.MaxMax | consts.Test2) == consts.MaxMax);
        VERIFY_IS_TRUE((consts.MaxMax | consts.Test3) == consts.MaxMax);

        VERIFY_IS_TRUE((consts.Test1 | consts.MaxZero) == consts.Test1OrMaxZero);
        VERIFY_IS_TRUE((consts.Test1 | consts.ZeroMax) == consts.Test1OrZeroMax);
        VERIFY_IS_TRUE((consts.Test1 | consts.ZeroZero) == consts.Test1);
        VERIFY_IS_TRUE((consts.Test1 | consts.Test1) == consts.Test1);
        VERIFY_IS_TRUE((consts.Test1 | consts.Test2) == consts.Test1OrTest2);
        VERIFY_IS_TRUE((consts.Test1 | consts.Test3) == consts.Test1OrTest3);

        VERIFY_IS_TRUE((consts.MaxZero | consts.MaxZero) == consts.MaxZero);
        VERIFY_IS_TRUE((consts.MaxZero | consts.ZeroMax) == consts.MaxMax);
        VERIFY_IS_TRUE((consts.MaxZero | consts.ZeroZero) == consts.MaxZero);
        VERIFY_IS_TRUE((consts.MaxZero | consts.Test2) == consts.Test2OrMaxZero);
        VERIFY_IS_TRUE((consts.MaxZero | consts.Test3) == consts.Test3OrMaxZero);

        VERIFY_IS_TRUE((consts.ZeroMax | consts.ZeroMax) == consts.ZeroMax);
        VERIFY_IS_TRUE((consts.ZeroMax | consts.ZeroZero) == consts.ZeroMax);
        VERIFY_IS_TRUE((consts.ZeroMax | consts.Test2) == consts.Test2OrZeroMax);
        VERIFY_IS_TRUE((consts.ZeroMax | consts.Test3) == consts.Test3OrZeroMax);

        VERIFY_IS_TRUE((consts.ZeroZero | consts.ZeroZero) == consts.ZeroZero);
        VERIFY_IS_TRUE((consts.ZeroZero | consts.Test2) == consts.Test2);
        VERIFY_IS_TRUE((consts.ZeroZero | consts.Test3) == consts.Test3);

        VERIFY_IS_TRUE((consts.Test2 | consts.Test2) == consts.Test2);
        VERIFY_IS_TRUE((consts.Test2 | consts.Test3) == consts.Test2OrTest3);

        VERIFY_IS_TRUE((consts.Test3 | consts.Test3) == consts.Test3);

        LargeInteger temp = consts.MaxMax;
        temp &= consts.MaxMax;
        VERIFY_IS_TRUE(temp == consts.MaxMax);
        temp = consts.MaxMax;
        temp &= consts.MaxZero;
        VERIFY_IS_TRUE(temp == consts.MaxZero);
        temp = consts.MaxMax;
        temp &= consts.ZeroMax;
        VERIFY_IS_TRUE(temp == consts.ZeroMax);
        temp = consts.MaxMax;
        temp &= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);
        temp = consts.MaxMax;
        temp &= consts.Test1;
        VERIFY_IS_TRUE(temp == consts.Test1);
        temp = consts.MaxMax;
        temp &= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test2);
        temp = consts.MaxMax;
        temp &= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test3);

        temp = consts.Test1;
        temp &= consts.MaxZero;
        VERIFY_IS_TRUE(temp == consts.Test1AndMaxZero);
        temp = consts.Test1;
        temp &= consts.ZeroMax;
        VERIFY_IS_TRUE(temp == consts.Test1AndZeroMax);
        temp = consts.Test1;
        temp &= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);
        temp = consts.Test1;
        temp &= consts.Test1;
        VERIFY_IS_TRUE(temp == consts.Test1);
        temp = consts.Test1;
        temp &= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test1AndTest2);
        temp = consts.Test1;
        temp &= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test1AndTest3);

        temp = consts.MaxZero;
        temp &= consts.MaxZero;
        VERIFY_IS_TRUE(temp == consts.MaxZero);
        temp = consts.MaxZero;
        temp &= consts.ZeroMax;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);
        temp = consts.MaxZero;
        temp &= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);
        temp = consts.MaxZero;
        temp &= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test2AndMaxZero);
        temp = consts.MaxZero;
        temp &= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test3AndMaxZero);

        temp = consts.ZeroMax;
        temp &= consts.ZeroMax;
        VERIFY_IS_TRUE(temp == consts.ZeroMax);
        temp = consts.ZeroMax;
        temp &= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);
        temp = consts.ZeroMax;
        temp &= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test2AndZeroMax);
        temp = consts.ZeroMax;
        temp &= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test3AndZeroMax);

        temp = consts.ZeroZero;
        temp &= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);
        temp = consts.ZeroZero;
        temp &= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);
        temp = consts.ZeroZero;
        temp &= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);

        temp = consts.Test2;
        temp &= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test2);
        temp = consts.Test2;
        temp &= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test2AndTest3);

        temp = consts.Test3;
        temp &= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test3);

        temp = consts.MaxMax;
        temp |= consts.MaxMax;
        VERIFY_IS_TRUE(temp == consts.MaxMax);
        temp = consts.MaxMax;
        temp |= consts.MaxZero;
        VERIFY_IS_TRUE(temp == consts.MaxMax);
        temp = consts.MaxMax;
        temp |= consts.MaxZero;
        VERIFY_IS_TRUE(temp == consts.MaxMax);
        temp = consts.MaxMax;
        temp |= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.MaxMax);
        temp = consts.MaxMax;
        temp |= consts.Test1;
        VERIFY_IS_TRUE(temp == consts.MaxMax);
        temp = consts.MaxMax;
        temp |= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.MaxMax);
        temp = consts.MaxMax;
        temp |= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.MaxMax);

        temp = consts.Test1;
        temp |= consts.MaxZero;
        VERIFY_IS_TRUE(temp == consts.Test1OrMaxZero);
        temp = consts.Test1;
        temp |= consts.ZeroMax;
        VERIFY_IS_TRUE(temp == consts.Test1OrZeroMax);
        temp = consts.Test1;
        temp |= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.Test1);
        temp = consts.Test1;
        temp |= consts.Test1;
        VERIFY_IS_TRUE(temp == consts.Test1);
        temp = consts.Test1;
        temp |= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test1OrTest2);
        temp = consts.Test1;
        temp |= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test1OrTest3);

        temp = consts.MaxZero;
        temp |= consts.MaxZero;
        VERIFY_IS_TRUE(temp == consts.MaxZero);
        temp = consts.MaxZero;
        temp |= consts.ZeroMax;
        VERIFY_IS_TRUE(temp == consts.MaxMax);
        temp = consts.MaxZero;
        temp |= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.MaxZero);
        temp = consts.MaxZero;
        temp |= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test2OrMaxZero);
        temp = consts.MaxZero;
        temp |= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test3OrMaxZero);

        temp = consts.ZeroMax;
        temp |= consts.ZeroMax;
        VERIFY_IS_TRUE(temp == consts.ZeroMax);
        temp = consts.ZeroMax;
        temp |= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroMax);
        temp = consts.ZeroMax;
        temp |= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test2OrZeroMax);
        temp = consts.ZeroMax;
        temp |= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test3OrZeroMax);

        temp = consts.ZeroZero;
        temp |= consts.ZeroZero;
        VERIFY_IS_TRUE(temp == consts.ZeroZero);
        temp = consts.ZeroZero;
        temp |= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test2);
        temp = consts.ZeroZero;
        temp |= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test3);

        temp = consts.Test2;
        temp |= consts.Test2;
        VERIFY_IS_TRUE(temp == consts.Test2);
        temp = consts.Test2;
        temp |= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test2OrTest3);

        temp = consts.Test3;
        temp |= consts.Test3;
        VERIFY_IS_TRUE(temp == consts.Test3);
    }

    // Tests to ensure that the IsSmallerPart function is correct.
    BOOST_AUTO_TEST_CASE(IsSmallerPartTest)
    {
        LargeIntegerTestConstants consts;
        VERIFY_IS_TRUE(!consts.MaxMax.IsSmallerPart());
        VERIFY_IS_TRUE(consts.ZeroMax.IsSmallerPart());
        VERIFY_IS_TRUE(!consts.MaxZero.IsSmallerPart());
        VERIFY_IS_TRUE(consts.ZeroZero.IsSmallerPart());
    }

    // Tests if multiplication is correct by comparing against products of random multi-word integers
    BOOST_AUTO_TEST_CASE(MultiplicationIsCorrectTestRandom)
    {
        StopwatchTime stopwatchTime(Stopwatch::Now() + TimeSpan::FromMilliseconds(100));
        TimeSpan remain = stopwatchTime.RemainingDuration();
        bool matched = true;
        while (remain > TimeSpan::Zero)
        {
            auto x = BigInteger::RandomBigInteger_();
            auto y = BigInteger::RandomBigInteger_();

            auto a = x.ToLargeInteger();
            a *= y.ToLargeInteger();

            x *= y;

            matched = (x.ToLargeInteger() == a);
            if (!matched) break;

            remain = stopwatchTime.RemainingDuration();
        }

        VERIFY_IS_TRUE(matched);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
