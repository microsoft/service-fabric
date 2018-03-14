// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FederationUnitTests
{
    using namespace std;
    using namespace Common;
    using namespace Federation;

    class RoutingTokenTests
    {
    };

    BOOST_FIXTURE_TEST_SUITE(RoutingTokenTestsSuite,RoutingTokenTests)

    BOOST_AUTO_TEST_CASE(TestEmptyTokenConstructor)
    {
        RoutingToken token = RoutingToken();
        VERIFY_IS_TRUE(token.IsEmpty);
        VERIFY_IS_TRUE(!token.IsFull);
        VERIFY_IS_TRUE(token.Version == 0);

        RoutingToken token2 = token;
        VERIFY_IS_TRUE(token2.IsEmpty);
        VERIFY_IS_TRUE(!token2.IsFull);
        VERIFY_IS_TRUE(token2.Version == 0);

    }

    BOOST_AUTO_TEST_CASE(TestVersionIsCorrect)
    {
        RoutingToken token(NodeIdRange::Empty, 1);
        VERIFY_IS_TRUE(token.Version == 1);
        VERIFY_IS_TRUE(!token.IsRecoverySafe(0));
        VERIFY_IS_TRUE(token.IsRecoverySafe(1));

        RoutingToken token1(NodeIdRange::Empty, 3ULL << sizeof(uint) * 8);
        VERIFY_IS_TRUE(!token1.IsMergeSafe(1ULL << sizeof(uint) * 8));
        VERIFY_IS_TRUE(token1.IsMergeSafe((3ULL << sizeof(uint) * 8) - 1));
        token1.IncrementRecoveryVersion();
        VERIFY_IS_TRUE(token1.Version == 4ULL << sizeof(uint) * 8);
    }

    BOOST_AUTO_TEST_CASE(TestRangeArithmeticIsCorrect)
    {
        NodeId middle(LargeInteger(4, 0));
        RoutingToken token(NodeIdRange(NodeId(LargeInteger(3, 3)), NodeId::MaxNodeId), 1);
        VERIFY_IS_TRUE(token.Contains(middle));
        RoutingToken token1(NodeIdRange(NodeId::MinNodeId, NodeId(LargeInteger(3, 2))), 1);
        VERIFY_IS_TRUE(token.IsPredAdjacent(token1));
        VERIFY_IS_TRUE(token1.IsSuccAdjacent(token));
    }

    BOOST_AUTO_TEST_CASE(TestAccept)
    {
        NodeId start1(LargeInteger(0, 7));
        NodeId end1(LargeInteger(1, 1));
        NodeId start2(LargeInteger(9,1));
        NodeId end2(LargeInteger(0,6));
        NodeId start3(LargeInteger(1,2));
        NodeId end3(LargeInteger(3,3));
        RoutingToken token1(NodeIdRange(start1, end1), 1);
        RoutingToken token2(NodeIdRange(start2, end2), 1);
        RoutingToken token3(NodeIdRange(start3, end3), 1);
        RoutingToken token4(NodeIdRange(start1, end1), 0);
        RoutingToken token5(NodeIdRange(start1, start1), 1);

        VERIFY_IS_TRUE(token1.Accept(token2, NodeId(LargeInteger(9,2))));
        VERIFY_IS_TRUE(token1.Range.Begin == start2);
        VERIFY_IS_TRUE(token1.Range.End == end1);
        VERIFY_IS_TRUE(token1.Accept(token3, NodeId(LargeInteger(1,3))));
        VERIFY_IS_TRUE(token1.Range.Begin == start2);
        VERIFY_IS_TRUE(token1.Range.End == end3);
        VERIFY_IS_TRUE(!token2.Accept(token3, NodeId(LargeInteger(2,4))));
        VERIFY_IS_TRUE(!token5.Accept(token3, NodeId(LargeInteger(9,2))));
    }

    BOOST_AUTO_TEST_CASE(AcceptThrow)
    {
        NodeId start1(LargeInteger(0, 7));
        NodeId end1(LargeInteger(1, 1));
        RoutingToken token1(NodeIdRange(start1, end1), 1);
        RoutingToken token2(NodeIdRange(start1, end1), 0);

        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(token2.Accept(token1, NodeId(LargeInteger(0, 8))), std::system_error); // microsoft::windows_error::assertion_failure
    }

    BOOST_AUTO_TEST_SUITE_END()

//#pragma region Helper Methods
//        const wstring prefix = L"addr_";
//
//        NodeConfig CreateNodeConfig(size_t low)
//        {
//            NodeId id(LargeInteger(0, low));
//            return NodeConfig(prefix + id.ToString(), id);
//        }
//
//        FederationPartnerNodeHeader CreateNodeHeader(size_t low, NodePhase::Enum phase)
//        {
//            NodeId id(LargeInteger(0, low));
//            return FederationPartnerNodeHeader(NodeInstance(id, 0), phase, prefix + id.ToString());
//        }
//
//        PartnerNodeSPtr CreatePtr(size_t low, NodePhase::Enum phase, SiteNodeSPtr siteNode)
//        {
//            return make_shared<PartnerNode>(CreateNodeHeader(low, phase), siteNode);
//        }
//
//        NodeIdRange CreateRange(uint64 low1, uint64 high1, uint64 low2, uint64 high2, bool isFull)
//        {
//            NodeId id1(LargeInteger(low1, high1));
//            NodeId id2(LargeInteger(low2, high2));
//            return NodeIdRange(id1, id2, isFull);
//        }
//#pragma endregion Helper Methods
}
