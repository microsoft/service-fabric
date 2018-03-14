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

    StringLiteral TraceRangeTest("NodeIdRangeTest");

    class NodeIdRangeTests
    {
    protected:
        LargeInteger GetRangeSize(NodeIdRange const & range);
        LargeInteger GetOverlapSize(NodeIdRange const & range1, NodeIdRange const & range2);
        void TestMerge(NodeIdRange const & range1, NodeIdRange const & range2);
        void TestSubtract(NodeIdRange const & range, NodeIdRange const & exclude);
        void TestSubtractRanges(NodeIdRange const & range, vector<NodeIdRange> const & excludes);
    };

    BOOST_FIXTURE_TEST_SUITE(NodeIdRangeTestsSuite,NodeIdRangeTests)

    BOOST_AUTO_TEST_CASE(SimpleTest)
    {
        NodeIdRange range1(LargeInteger(0, 100), LargeInteger(0, 200));
        NodeIdRange range2(LargeInteger(0, 50), LargeInteger(0, 250));
        NodeIdRange range3(LargeInteger(0, 150), LargeInteger(0, 200));
        NodeIdRange range4(NodeIdRange::Full);
        NodeIdRange range5(LargeInteger(0, 200), LargeInteger(0, 210));
        NodeIdRange range6(LargeInteger(0, 250), LargeInteger(0, 50));
        NodeIdRange range7(LargeInteger(0, 250), LargeInteger(0, 250));

        VERIFY_IS_TRUE(range1.Contains(range1));
        VERIFY_IS_TRUE(!range1.Contains(range2));
        VERIFY_IS_TRUE(range1.Contains(range3));
        VERIFY_IS_TRUE(!range3.Contains(range4));
        VERIFY_IS_TRUE(range4.Contains(range3));
        VERIFY_IS_TRUE(range6.Contains(range7));

        VERIFY_IS_TRUE(!range1.Contains(LargeInteger(0, 250)));
        VERIFY_IS_TRUE(!NodeIdRange::Empty.Contains(LargeInteger(0, 250)));
        VERIFY_IS_TRUE(range4.Contains(LargeInteger(0, 250)));
        VERIFY_IS_TRUE(range6.Contains(LargeInteger(0, 250)));
        VERIFY_IS_TRUE(range7.Contains(LargeInteger(0, 250)));

        VERIFY_IS_TRUE(!range1.Disjoint(range1));
        VERIFY_IS_TRUE(!range1.Disjoint(range4));
        VERIFY_IS_TRUE(range1.Disjoint(range6));
        VERIFY_IS_TRUE(range5.Disjoint(range6));
        VERIFY_IS_TRUE(!range4.Disjoint(range6));
        VERIFY_IS_TRUE(!range6.Disjoint(range7));
        VERIFY_IS_TRUE(range1.Disjoint(range6));

        VERIFY_IS_TRUE(NodeIdRange::Empty != NodeIdRange::Full);
        VERIFY_IS_TRUE(NodeIdRange::Empty != NodeIdRange(NodeId(LargeInteger::One), NodeId::MinNodeId));
        VERIFY_IS_TRUE(NodeIdRange::Full == NodeIdRange(NodeId(LargeInteger::One), NodeId::MinNodeId));
    }

    BOOST_AUTO_TEST_CASE(MergeTest)
    {
        TestMerge(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)));
        TestMerge(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 50), LargeInteger(0, 250)));
        TestMerge(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 150), LargeInteger(0, 250)));
        TestMerge(NodeIdRange(LargeInteger(0, 150), LargeInteger(0, 250)), NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)));
        TestMerge(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 150), LargeInteger(0, 200)));
        TestMerge(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 180), LargeInteger(0, 150)));
    }

    BOOST_AUTO_TEST_CASE(SubtractTest)
    {
        TestSubtract(NodeIdRange::Full, NodeIdRange::Full);
        TestSubtract(NodeIdRange::Full, NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)));
        TestSubtract(NodeIdRange::Full, NodeIdRange(LargeInteger(0, 200), LargeInteger(0, 100)));

        TestSubtract(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)));
        TestSubtract(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 50), LargeInteger(0, 70)));
        TestSubtract(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 50), LargeInteger(0, 110)));
        TestSubtract(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 50), LargeInteger(0, 210)));
        TestSubtract(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 300), LargeInteger(0, 150)));
        TestSubtract(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 150), LargeInteger(0, 160)));
        TestSubtract(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), NodeIdRange(LargeInteger(0, 150), LargeInteger(0, 210)));

        TestSubtract(NodeIdRange(LargeInteger(0, 200), LargeInteger(0, 100)), NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)));
        TestSubtract(NodeIdRange(LargeInteger(0, 200), LargeInteger(0, 100)), NodeIdRange(LargeInteger(0, 150), LargeInteger(0, 300)));
        TestSubtract(NodeIdRange(LargeInteger(0, 200), LargeInteger(0, 100)), NodeIdRange(LargeInteger(0, 150), LargeInteger(0, 50)));
        TestSubtract(NodeIdRange(LargeInteger(0, 200), LargeInteger(0, 100)), NodeIdRange(LargeInteger(0, 250), LargeInteger(0, 50)));
        TestSubtract(NodeIdRange(LargeInteger(0, 200), LargeInteger(0, 100)), NodeIdRange(LargeInteger(0, 250), LargeInteger(0, 100)));
    }

    BOOST_AUTO_TEST_CASE(SubtractRangesTest)
    {
        vector<NodeIdRange> excludes;
        excludes.push_back(NodeIdRange::Full);

        TestSubtractRanges(NodeIdRange::Full, excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), excludes);

        excludes.clear();
        excludes.push_back(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)));

        TestSubtractRanges(NodeIdRange::Full, excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 200)), excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 50), LargeInteger(0, 300)), excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 200), LargeInteger(0, 100)), excludes);

        excludes.push_back(NodeIdRange(LargeInteger(0, 201), LargeInteger(0, 300)));
        TestSubtractRanges(NodeIdRange::Full, excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 300)), excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 50), LargeInteger(0, 40)), excludes);

        excludes.push_back(NodeIdRange(LargeInteger(0, 400), LargeInteger(0, 500)));
        TestSubtractRanges(NodeIdRange::Full, excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 300)), excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 100), LargeInteger(0, 500)), excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 150), LargeInteger(0, 350)), excludes);
        TestSubtractRanges(NodeIdRange(LargeInteger(0, 201), LargeInteger(0, 300)), excludes);
    }

    BOOST_AUTO_TEST_SUITE_END()

    LargeInteger NodeIdRangeTests::GetRangeSize(NodeIdRange const & range)
    {
        return (range.IsFull || range.IsEmpty ? LargeInteger::Zero : range.Begin.SuccDist(range.End) + LargeInteger::One);
    }

    LargeInteger NodeIdRangeTests::GetOverlapSize(NodeIdRange const & range1, NodeIdRange const & range2)
    {
        if (range2.Contains(range1))
        {
            return GetRangeSize(range1);
        }

        if (range1.Contains(range2))
        {
            return GetRangeSize(range2);
        }

        LargeInteger result = LargeInteger::Zero;
        if (range2.Contains(range1.Begin))
        {
            result = result + range1.Begin.SuccDist(range2.End) + LargeInteger::One;
        }

        if (range2.Contains(range1.End))
        {
            result = result + range2.Begin.SuccDist(range1.End) + LargeInteger::One;
        }

        return result;
    }

    void NodeIdRangeTests::TestMerge(NodeIdRange const & range1, NodeIdRange const & range2)
    {
        NodeIdRange result = NodeIdRange::Merge(range1, range2);
        VERIFY_IS_TRUE(result.Contains(range1));
        VERIFY_IS_TRUE(result.Contains(range2));
        VERIFY_IS_TRUE(GetRangeSize(result) + GetOverlapSize(range1, range2) == GetRangeSize(range1) + GetRangeSize(range2));
    }

    void NodeIdRangeTests::TestSubtract(NodeIdRange const & range, NodeIdRange const & exclude)
    {
        NodeIdRange range1, range2;
        range.Subtract(exclude, range1, range2);

        Trace.WriteInfo(TraceRangeTest, "Range {0} exclude {1} returned {2} and {3}",
            range, exclude, range1, range2);

        if (!range1.IsEmpty)
        {
            VERIFY_IS_TRUE(range.Contains(range1) && exclude.Disjoint(range1));
            if (!range2.IsEmpty)
            {
                VERIFY_IS_TRUE(range1.Disjoint(range2) && !range1.IsSuccAdjacent(range2) && !range1.IsPredAdjacent(range2));
            }
        }

        if (!range2.IsEmpty)
        {
            VERIFY_IS_TRUE((range2.IsEmpty || range.Contains(range2)) && exclude.Disjoint(range2));
        }

        VERIFY_IS_TRUE(GetRangeSize(range) == GetOverlapSize(range, exclude) + GetRangeSize(range1) + GetRangeSize(range2));
    }

    void NodeIdRangeTests::TestSubtractRanges(NodeIdRange const & range, vector<NodeIdRange> const & excludes)
    {
        vector<NodeIdRange> result;

        range.Subtract(excludes, result);
        Trace.WriteInfo(TraceRangeTest, "Range {0} exclude {1} returned {2}",
            range, excludes, result);

        LargeInteger overlapSize = LargeInteger::Zero;
        for (size_t i = 0; i < result.size(); i++)
        {
            VERIFY_IS_TRUE(!result[i].IsEmpty);
            VERIFY_IS_TRUE(range.Contains(result[i]));

            for (size_t j = 0; j < excludes.size(); j++)
            {
                VERIFY_IS_TRUE(result[i].Disjoint(excludes[j]));
            }

            overlapSize = overlapSize + GetOverlapSize(range, result[i]);
        }

        for (size_t i = 0; i + 1 < result.size(); i++)
        {
            for (size_t j = i + 1; j < result.size(); j++)
            {
                VERIFY_IS_TRUE(result[i].Disjoint(result[j]));
            }
        }

        for (size_t i = 0; i < excludes.size(); i++)
        {
            overlapSize = overlapSize + GetOverlapSize(range, excludes[i]);
        }

        VERIFY_IS_TRUE(GetRangeSize(range) == overlapSize);
    }
}
