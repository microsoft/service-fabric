// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "../Common/LargeInteger.Test.Constants.h"

namespace FederationUnitTests
{
    using namespace std;
    using namespace Common;
    using namespace Federation;

    //   UnitTests for the NodeId class. The operators, walks and distances (except min) 
    //   are not tested as they are simple wrappers around LargeInteger operators which have been tested ow.
    //   
    class NodeIdTests
    {
    };

    BOOST_FIXTURE_TEST_SUITE(NodeIdTestsSuite,NodeIdTests)

    BOOST_AUTO_TEST_CASE(MinDistanceIsCorrect)
    {
        LargeIntegerTestConstants consts;
        NodeId id1(consts.MaxMax);
        NodeId id2(consts.MaxZero);
        NodeId id3(consts.ZeroMax);
        NodeId id4(consts.ZeroZero);
        NodeId id5(consts.Test1);
        NodeId id6(consts.Test2);
        NodeId id7(consts.Test3);

        VERIFY_IS_TRUE(id1.MinDist(id2) == consts.ZeroMax);
        VERIFY_IS_TRUE(id1.MinDist(id3) == consts.ZeroMax + LargeInteger::One);
        VERIFY_IS_TRUE(id1.MinDist(id4) == consts.ZeroOne);
        VERIFY_IS_TRUE(id1.MinDist(id5) == consts.Test1 - consts.MaxMax);
        VERIFY_IS_TRUE(id1.MinDist(id6) == consts.Test2MaxMaxMinDist);
        VERIFY_IS_TRUE(id1.MinDist(id7) == consts.Test3MaxMaxMinDist);

        VERIFY_IS_TRUE(id2.MinDist(id3) == consts.ZeroMax - consts.MaxZero);
        VERIFY_IS_TRUE(id2.MinDist(id4) == consts.ZeroMax + LargeInteger::One);
        VERIFY_IS_TRUE(id2.MinDist(id5) == consts.Test1 - consts.MaxZero);
        VERIFY_IS_TRUE(id2.MinDist(id6) == consts.Test2MaxZeroMinDist);
        VERIFY_IS_TRUE(id2.MinDist(id7) == consts.Test3MaxZeroMinDist);

        VERIFY_IS_TRUE(id3.MinDist(id4) == consts.ZeroMax);
        VERIFY_IS_TRUE(id3.MinDist(id5) == consts.ZeroMax - consts.Test1);
        VERIFY_IS_TRUE(id3.MinDist(id6) == consts.Test2ZeroMaxMinDist);
        VERIFY_IS_TRUE(id3.MinDist(id7) == consts.Test3ZeroMaxMinDist);

        VERIFY_IS_TRUE(id4.MinDist(id5) == consts.Test1 - consts.ZeroZero);
        VERIFY_IS_TRUE(id4.MinDist(id6) == consts.Test2ZeroZeroMinDist);
        VERIFY_IS_TRUE(id4.MinDist(id7) == consts.Test3ZeroZeroMinDist);

        VERIFY_IS_TRUE(id5.MinDist(id6) == consts.Test1Test2MinDist);
        VERIFY_IS_TRUE(id5.MinDist(id7) == consts.Test1Test3MinDist);
        VERIFY_IS_TRUE(id6.MinDist(id7) == consts.Test2Test3MinDist);
    }

    // Tests the correctness of the Precedes function.
    BOOST_AUTO_TEST_CASE(PrecedesIsCorrect)
    {
        LargeIntegerTestConstants consts;
        NodeId id1(consts.MaxMax);
        NodeId id2(consts.MaxZero);
        NodeId id3(consts.ZeroMax);
        NodeId id4(consts.ZeroZero);

        VERIFY_IS_TRUE(!id1.Precedes(id2));
        VERIFY_IS_TRUE(id1.Precedes(id3));
        VERIFY_IS_TRUE(id1.Precedes(id4));
        VERIFY_IS_TRUE(id2.Precedes(id3));
        VERIFY_IS_TRUE(id2.Precedes(id4));
        VERIFY_IS_TRUE(!id3.Precedes(id4));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
