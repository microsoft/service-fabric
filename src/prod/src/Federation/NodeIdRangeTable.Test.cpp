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

    class NodeIdRangeTableTests
    {
    };

    BOOST_FIXTURE_TEST_SUITE(NodeIdRangeTableTestsSuite,NodeIdRangeTableTests)

    BOOST_AUTO_TEST_CASE(SimpleTestFull)
    {
        NodeIdRangeTable table;
        VERIFY_IS_FALSE(table.AreAllRangesReceived);
        table.AddRange(NodeIdRange::Full);
        VERIFY_IS_TRUE(table.AreAllRangesReceived);
    }

    BOOST_AUTO_TEST_CASE(SimpleTestWrap)
    {
        NodeIdRangeTable table;
        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        NodeId end(LargeInteger::RandomLargeInteger_());
        NodeId start(end.IdValue + LargeInteger::One);

        table.AddRange(NodeIdRange(start, end));
        VERIFY_IS_TRUE(table.AreAllRangesReceived);
    }

    BOOST_AUTO_TEST_CASE(SimpleTestAddPartialThenFull)
    {
        NodeIdRangeTable table;
        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        NodeId start(LargeInteger::RandomLargeInteger_());
        NodeId end(start.IdValue + LargeInteger(0, 100));

        table.AddRange(NodeIdRange(start, end));

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        table.AddRange(NodeIdRange::Full);

        VERIFY_IS_TRUE(table.AreAllRangesReceived);
    }

    BOOST_AUTO_TEST_CASE(BasicTest)
    {
        NodeIdRangeTable table;

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        NodeIdRange first(NodeId::MinNodeId, NodeId(LargeInteger(0, 150)));
        NodeIdRange second(first.End.IdValue + LargeInteger::One, NodeId(LargeInteger(150, 150)));
        NodeIdRange third(second.End + LargeInteger::One, NodeId::MaxNodeId);

        table.AddRange(second);

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        for (auto range : table)
        {
            VERIFY_IS_TRUE(range == first || range == third);
        }

        table.AddRange(first);

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        for (auto range : table)
        {
            VERIFY_IS_TRUE(range == third);
        }

        table.AddRange(third);

        VERIFY_IS_TRUE(table.AreAllRangesReceived);
    }

    BOOST_AUTO_TEST_CASE(DuplicateTest)
    {
        NodeIdRangeTable table;

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        NodeIdRange first(NodeId::MinNodeId, NodeId(LargeInteger(0, 150)));
        NodeIdRange second(first.End.IdValue + LargeInteger::One, NodeId(LargeInteger(150, 150)));
        NodeIdRange third(second.End + LargeInteger::One, NodeId::MaxNodeId);

        table.AddRange(second);
        table.AddRange(second); // Add twice

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        for (auto range : table)
        {
            VERIFY_IS_TRUE(range == first || range == third);
        }

        table.AddRange(first);

        NodeIdRange overlapDuplicate(NodeId(LargeInteger(0, 80)), NodeId(LargeInteger(80, 80)));

        table.AddRange(overlapDuplicate);

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        for (auto range : table)
        {
            VERIFY_IS_TRUE(range == third);
        }

        table.AddRange(third);

        VERIFY_IS_TRUE(table.AreAllRangesReceived);

        NodeIdRange overlapDuplicate2(NodeId(LargeInteger(80, 80)), NodeId(LargeInteger(0, 80)));

        table.AddRange(overlapDuplicate2);

        VERIFY_IS_TRUE(table.AreAllRangesReceived);
    }

    BOOST_AUTO_TEST_CASE(WrappingRangeTest)
    {
        NodeIdRangeTable table;

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        NodeIdRange first(NodeId(LargeInteger(150, 150)), NodeId(LargeInteger(0, 150)));
        NodeIdRange second(first.End.IdValue + LargeInteger::One, first.Begin.IdValue - LargeInteger::One);

        table.AddRange(first);

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        for (auto range : table)
        {
            VERIFY_IS_TRUE(range == second);
        }

        table.AddRange(second);

        VERIFY_IS_TRUE(table.AreAllRangesReceived);
    }

    // An FederationTest was failing because of a bug in the RangeTable, this CIT used traces
    // to recreate the scenario to find and fix the bug.
    BOOST_AUTO_TEST_CASE(FromFederationTest)
    {
        NodeIdRangeTable table;

        vector<pair<wstring, wstring>> nodeIdRanges;
        nodeIdRanges.push_back(make_pair(L"80000000000000000000000000002801", L"8"));
        nodeIdRanges.push_back(make_pair(L"9", L"18"));
        nodeIdRanges.push_back(make_pair(L"39", L"48"));
        nodeIdRanges.push_back(make_pair(L"49", L"58"));
        nodeIdRanges.push_back(make_pair(L"19", L"28"));
        nodeIdRanges.push_back(make_pair(L"29", L"38"));
        nodeIdRanges.push_back(make_pair(L"881", L"3000"));
        nodeIdRanges.push_back(make_pair(L"89", L"c8"));
        nodeIdRanges.push_back(make_pair(L"79", L"88"));
        nodeIdRanges.push_back(make_pair(L"69", L"78"));
        nodeIdRanges.push_back(make_pair(L"59", L"68"));
        nodeIdRanges.push_back(make_pair(L"c9", L"880"));
        nodeIdRanges.push_back(make_pair(L"3001", L"80000000000000000000000000002800"));

        size_t size = nodeIdRanges.size();
        size_t current = 0;

        for (auto pair : nodeIdRanges)
        {
            ++current;

            NodeId id1;
            NodeId id2;

            VERIFY_IS_TRUE(NodeId::TryParse(pair.first, id1));
            VERIFY_IS_TRUE(NodeId::TryParse(pair.second, id2));

            NodeIdRange range(id1, id2);

            table.AddRange(range);

            if (current < size)
            {
                VERIFY_IS_FALSE(table.AreAllRangesReceived);
            }
            else
            {
                VERIFY_IS_TRUE(table.AreAllRangesReceived);
            }
        }

        VERIFY_IS_TRUE(table.AreAllRangesReceived);
    }

    BOOST_AUTO_TEST_CASE(OverlappedRangeTest)
    {
        NodeIdRangeTable table;

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        NodeIdRange first(NodeId::MinNodeId, NodeId(LargeInteger(0, 90)));

        NodeIdRange third(NodeId(LargeInteger(90, 120)), NodeId::MaxNodeId);

        NodeIdRange second(first.End.IdValue - LargeInteger(0, 50), third.Begin.IdValue + LargeInteger(0, 50));

        table.AddRange(first);
        table.AddRange(third);

        NodeIdRange hole(first.End.IdValue + LargeInteger::One, third.Begin.IdValue - LargeInteger::One);

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        for (auto range : table)
        {
            VERIFY_IS_TRUE(range == hole);
        }

        table.AddRange(second);

        VERIFY_IS_TRUE(table.AreAllRangesReceived);
    }

    BOOST_AUTO_TEST_CASE(RandomTest)
    {
        NodeIdRangeTable table;

        VERIFY_IS_FALSE(table.AreAllRangesReceived);

        int iterations = 0;

        while(!table.AreAllRangesReceived)
        {
            ++iterations;

            VERIFY_IS_TRUE(iterations < 10000);

            NodeId start(LargeInteger::RandomLargeInteger_());
            NodeId end(LargeInteger::RandomLargeInteger_());
            NodeIdRange range(start, end);

            table.AddRange(range);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
