// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    class NameRangeTupleTest
    {
    protected:
        void CheckCoveredRanges(
            std::map<NameRangeTuple, int, NameRangeTuple::LessThanComparitor> const & m,
            NameRangeTuple const & range,
            int expectedCovered);
    };

    BOOST_FIXTURE_TEST_SUITE(NameRangeTupleTestSuite,NameRangeTupleTest)

    BOOST_AUTO_TEST_CASE(MapTest)
    {
        std::map<NameRangeTuple, int, NameRangeTuple::LessThanComparitor> m;
        NamingUri a(L"a");
        NameRangeTuple a1(a, PartitionKey(1));
        NameRangeTuple a2(a, PartitionKey(5));
        NameRangeTuple a3(a, PartitionKey(15));
        NameRangeTuple a4(a, PartitionKey(20));
        NamingUri b(L"b");
        NameRangeTuple b1(b, PartitionKey(1));
        NameRangeTuple b2(b, PartitionKey(2));

        m.insert(pair<NameRangeTuple, int>(a1, 1));
        m.insert(pair<NameRangeTuple, int>(a2, 3));
        m.insert(pair<NameRangeTuple, int>(a3, 2));
        m.insert(pair<NameRangeTuple, int>(a4, 6));
        m.insert(pair<NameRangeTuple, int>(b1, 14));
        m.insert(pair<NameRangeTuple, int>(b2, 0));

        CheckCoveredRanges(m, NameRangeTuple(a, PartitionInfo(3, 18)), 2);
        CheckCoveredRanges(m, NameRangeTuple(a, PartitionInfo(1, 20)), 4);
        CheckCoveredRanges(m, NameRangeTuple(a, PartitionInfo(1, 15)), 3);
        CheckCoveredRanges(m, NameRangeTuple(a, PartitionInfo(0, 21)), 4);
        CheckCoveredRanges(m, NameRangeTuple(a, PartitionInfo(18, 40)), 1);
        CheckCoveredRanges(m, NameRangeTuple(a, PartitionInfo(28, 40)), 0);

        CheckCoveredRanges(m, NameRangeTuple(b, PartitionInfo(1, 2)), 2);
        CheckCoveredRanges(m, NameRangeTuple(b, PartitionInfo(1, 10)), 2);
        CheckCoveredRanges(m, NameRangeTuple(b, PartitionInfo(5, 10)), 0);
    }

    BOOST_AUTO_TEST_CASE(MultiMapTest)
    {
        std::multimap<NameRangeTuple, int, NameRangeTuple::LessThanComparitor> m;
        NamingUri a(L"a");
        NameRangeTuple a1(a, PartitionKey(1));
        NameRangeTuple a2(a, PartitionKey(2));
        NameRangeTuple a3(a, PartitionKey(3));
        NamingUri b(L"b");
        NameRangeTuple b1(b, PartitionKey(1));
        NameRangeTuple b2(b, PartitionKey(2));
        NameRangeTuple b3(b, PartitionKey(3));

        m.insert(pair<NameRangeTuple, int>(a1, 1));
        m.insert(pair<NameRangeTuple, int>(a1, 3));
        m.insert(pair<NameRangeTuple, int>(a1, 2));
        m.insert(pair<NameRangeTuple, int>(a3, 6));
        m.insert(pair<NameRangeTuple, int>(a2, 14));

        m.insert(pair<NameRangeTuple, int>(b1, 0));

        auto nameRange = m.equal_range(a1);
        size_t found = 0;
        for (auto it = nameRange.first; it != nameRange.second; ++it)
        {
            ++found;
        }
        VERIFY_ARE_EQUAL(3u, found, L"Find tuple returns correct number of items");

        auto nameRange2 = m.equal_range(b1);
        found = 0;
        for (auto it = nameRange2.first; it != nameRange2.second; ++it)
        {
            ++found;
        }
        VERIFY_ARE_EQUAL(1u, found, L"Find tuple returns correct number of items");

        NameRangeTuple tuple(a, PartitionInfo(0, 10));
        auto nameRange3 = m.equal_range(tuple);
        found = 0;
        for (auto it = nameRange3.first; it != nameRange3.second; ++it)
        {
            ++found;
        }
        VERIFY_ARE_EQUAL(5u, found, L"Find tuple returns correct number of items");
    }

    BOOST_AUTO_TEST_CASE(EqualsTest)
    {
        NameRangeTuple::ContainsComparitor comp;
        NamingUri a(L"A");
        NamingUri b(L"B");

        NameRangeTuple a1(a, PartitionInfo(1, 1));
        NameRangeTuple a2(a, PartitionInfo(2, 2));
        NameRangeTuple b1(b, PartitionInfo(1, 1));
        NameRangeTuple b2(b, PartitionInfo(2, 2));

        VERIFY_IS_FALSE(comp(a1, a2));
        VERIFY_IS_FALSE(comp(b1, b2));
        VERIFY_IS_FALSE(comp(a1, b1));
        VERIFY_IS_FALSE(comp(a2, b2));

        
        VERIFY_IS_FALSE(comp(a2, a1));
        VERIFY_IS_FALSE(comp(b2, b1));
        VERIFY_IS_FALSE(comp(b1, a1));
        VERIFY_IS_FALSE(comp(b2, a2));

        NameRangeTuple a1_2(a, PartitionInfo(1, 2));

        VERIFY_IS_TRUE(comp(a1, a1_2));
        VERIFY_IS_TRUE(comp(a2, a1_2));
        VERIFY_IS_FALSE(comp(b1, a1_2));
        VERIFY_IS_FALSE(comp(b2, a1_2));

        VERIFY_IS_TRUE(comp(a1_2, a1));
        VERIFY_IS_TRUE(comp(a1_2, a2));
        VERIFY_IS_FALSE(comp(a1_2, b1));
        VERIFY_IS_FALSE(comp(a1_2, b2));
        
        NameRangeTuple b3(b, PartitionInfo(3, 3));
        NameRangeTuple b1_3(b, PartitionInfo(1, 3));

        VERIFY_IS_TRUE(comp(b1, b1_3));
        VERIFY_IS_TRUE(comp(b2, b1_3));
        VERIFY_IS_TRUE(comp(b3, b1_3));
        
        VERIFY_IS_TRUE(comp(b1_3, b1));
        VERIFY_IS_TRUE(comp(b1_3, b2));
        VERIFY_IS_TRUE(comp(b1_3, b3));
    }

    BOOST_AUTO_TEST_CASE(LessThanTest)
    {
        NameRangeTuple::LessThanComparitor lessComp;
        NamingUri a(L"A");
        NamingUri b(L"B");

        NameRangeTuple a1(a, PartitionInfo(1, 10));
        NameRangeTuple a2(a, PartitionInfo(11, 20));
        NameRangeTuple a3(a, PartitionInfo(21, 30));
        NameRangeTuple b1(b, PartitionInfo(1, 10));
        NameRangeTuple b2(b, PartitionInfo(11, 20));
        NameRangeTuple b3(b, PartitionInfo(21, 30));

        VERIFY_IS_TRUE(lessComp(a1, a2));
        VERIFY_IS_TRUE(lessComp(a2, a3));
        VERIFY_IS_TRUE(lessComp(b1, b2));
        VERIFY_IS_TRUE(lessComp(b2, b3));
        
        VERIFY_IS_TRUE(lessComp(a1, b1));
        VERIFY_IS_TRUE(lessComp(a3, b1));
        
        VERIFY_IS_FALSE(lessComp(a3, a2));
        VERIFY_IS_FALSE(lessComp(a2, a1));
        VERIFY_IS_FALSE(lessComp(b3, b2));
        VERIFY_IS_FALSE(lessComp(b2, b1));
        
        VERIFY_IS_FALSE(lessComp(b1, a1));
        VERIFY_IS_FALSE(lessComp(b3, a1));

        VERIFY_IS_FALSE(lessComp(a1, a1));
        VERIFY_IS_FALSE(lessComp(a2, a2));
        VERIFY_IS_FALSE(lessComp(b1, b1));
        
        NameRangeTuple a4(a, PartitionInfo(31, 45));
        NameRangeTuple a5(a, PartitionInfo(40, 50));
        NameRangeTuple a6(a, PartitionInfo(46, 49));

        VERIFY_IS_TRUE(lessComp(a4, a6));
        VERIFY_IS_FALSE(lessComp(a5, a6));
        VERIFY_IS_FALSE(lessComp(a6, a5));
    }

    BOOST_AUTO_TEST_CASE(PartitionInfoTypeMismatchTest)
    {
        NameRangeTuple::ContainsComparitor comp;
        NamingUri a(L"A");

        NameRangeTuple singleton(a, PartitionInfo());
        NameRangeTuple named(a, PartitionInfo(L"Test"));
        NameRangeTuple uniform(a, PartitionInfo(100, 200));

        VERIFY_IS_FALSE(comp(singleton, named));
        VERIFY_IS_FALSE(comp(singleton, uniform));
        
        VERIFY_IS_FALSE(comp(named, singleton));
        VERIFY_IS_FALSE(comp(named, uniform));

        VERIFY_IS_FALSE(comp(uniform, singleton));
        VERIFY_IS_FALSE(comp(uniform, named));
    }

    BOOST_AUTO_TEST_SUITE_END()
    
    void NameRangeTupleTest::CheckCoveredRanges(
        std::map<NameRangeTuple, int, NameRangeTuple::LessThanComparitor> const & m,
        NameRangeTuple const & range,
        int expectedCoveredEntries)
    {
        auto itStart = m.lower_bound(range);
        auto itEnd = m.upper_bound(range);
        
        int coveredEntries = 0;
        for (auto it = itStart; it != itEnd; ++it)
        {
            ++coveredEntries;            
        }
        
        VERIFY_ARE_EQUAL(expectedCoveredEntries, coveredEntries, L"Number of covered entries is correct");
    }
}
