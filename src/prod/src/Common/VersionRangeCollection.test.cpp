// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
#define VERIFY(condition, debug) \
    VERIFY_IS_TRUE(condition, wformatString("{0}", debug).c_str());

#define VERIFY2(condition, debug1, debug2) \
    VERIFY_IS_TRUE(condition, wformatString("{0}, {1}", debug1, debug2).c_str());

#define VERIFY_RANGE_COUNT( Vrc, Count ) \
    VERIFY(Count == Vrc.VersionRanges.size(), Vrc);

#define VERIFY_RANGE( Vrc, Index, Start, End ) \
    VERIFY(Start == Vrc.VersionRanges[Index].StartVersion, Vrc); \
    VERIFY(End == Vrc.VersionRanges[Index].EndVersion, Vrc); \

    using namespace std;

    StringLiteral const TestComponent("TestVersionRangeCollection");

    class TestVersionRangeCollection
    {
    protected:
        TestVersionRangeCollection() { }

        void TestMergeRandomHelper(int interation, Random &);
        void TestMergeLargeCollectionsHelper(size_t count);
        void VerifyMerge(VersionRangeCollection const & baseline, VersionRangeCollection const & merged);

        static bool enablePerformanceTest;
    };

    bool TestVersionRangeCollection::enablePerformanceTest = false;

    BOOST_FIXTURE_TEST_SUITE(TestVersionRangeCollectionSuite,TestVersionRangeCollection)

    BOOST_AUTO_TEST_CASE(TestConstruction)
    {
        VersionRangeCollection vrc;

        // Test default constructor.
        vrc = VersionRangeCollection();

        VERIFY_IS_TRUE(vrc.IsEmpty);

        // Test construction with a single VersionRange.
        int64 startVersion = 5;
        int64 endVersion = 15;
        vrc = VersionRangeCollection(5, 15);

        VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
        VERIFY_ARE_EQUAL(startVersion, vrc.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(endVersion, vrc.VersionRanges[0].EndVersion);

        // Test copy constructor.
        VersionRangeCollection vrcCopy(vrc);

        VERIFY_ARE_EQUAL(vrcCopy.VersionRanges.size(), vrc.VersionRanges.size());
        VERIFY_ARE_EQUAL(vrcCopy.VersionRanges[0].StartVersion, vrc.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(vrcCopy.VersionRanges[0].EndVersion, vrc.VersionRanges[0].EndVersion);
    }

    BOOST_AUTO_TEST_CASE(TestAddAndRemove)
    {
        VersionRangeCollection vrc1 = VersionRangeCollection();

        VERIFY_IS_TRUE(vrc1.Add(VersionRange(10, 15)));
        VERIFY_IS_TRUE(vrc1.Add(VersionRange(0, 5)));
        VERIFY_IS_TRUE(vrc1.Add(VersionRange(3, 12)));
        VERIFY_IS_TRUE(vrc1.Add(VersionRange(20, 50)));

        VERIFY_ARE_EQUAL(2u, vrc1.VersionRanges.size());
        VERIFY_ARE_EQUAL(0, vrc1.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(15, vrc1.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(20, vrc1.VersionRanges[1].StartVersion);
        VERIFY_ARE_EQUAL(50, vrc1.VersionRanges[1].EndVersion);

        VersionRangeCollection vrc2 = VersionRangeCollection(40, 80);
        
        VERIFY_IS_TRUE(vrc2.Test_Add(VersionRangeCollection(vrc1)));

        VERIFY_ARE_EQUAL(2u, vrc2.VersionRanges.size());
        VERIFY_ARE_EQUAL(0, vrc2.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(15, vrc2.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(20, vrc2.VersionRanges[1].StartVersion);
        VERIFY_ARE_EQUAL(80, vrc2.VersionRanges[1].EndVersion);

        vrc2.Remove(vrc1);

        VERIFY_ARE_EQUAL(1u, vrc2.VersionRanges.size());
        VERIFY_ARE_EQUAL(50, vrc2.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(80, vrc2.VersionRanges[0].EndVersion);

        vrc2.Remove(VersionRange(40, 80));

        VERIFY_IS_TRUE(vrc2.IsEmpty);
    }

    BOOST_AUTO_TEST_CASE(TestOptimizedRemove)
    {
        VersionRangeCollection vrc1;
        VersionRangeCollection vrc2;

        // empty ranges

        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc2.AddRange(0, 1);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 1);
        vrc2.Clear();
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 1);

        // remove self

        vrc1.Remove(vrc1);
        VERIFY_RANGE_COUNT(vrc1, 0);

        // single version
        
        vrc1.AddRange(0, 1);
        vrc2.AddRange(-1, 0);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 1);

        vrc2.Clear();
        vrc2.AddRange(1, 2);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 1);

        vrc2.Clear();
        vrc2.AddRange(-1, 1);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 1);
        vrc2.Clear();
        vrc2.AddRange(0, 2);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 1);
        vrc2.Clear();
        vrc2.AddRange(0, 1);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        // single range 2 versions
        
        vrc1.AddRange(0, 2);
        vrc2.Clear();
        vrc2.AddRange(-1, 1);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 1, 2);

        vrc1.AddRange(0, 2);
        vrc2.Clear();
        vrc2.AddRange(-1, 2);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 2);
        vrc2.Clear();
        vrc2.AddRange(-1, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 2);
        vrc2.Clear();
        vrc2.AddRange(0, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 2);
        vrc2.Clear();
        vrc2.AddRange(1, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 1);

        vrc1.AddRange(0, 2);
        vrc2.Clear();
        vrc2.AddRange(2, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 2);

        // single range 3 versions
        
        vrc1.AddRange(0, 3);
        vrc2.Clear();
        vrc2.AddRange(0, 1);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 1, 3);

        vrc1.AddRange(0, 3);
        vrc2.Clear();
        vrc2.AddRange(0, 2);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 2, 3);

        vrc1.AddRange(0, 3);
        vrc2.Clear();
        vrc2.AddRange(0, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 3);
        vrc2.Clear();
        vrc2.AddRange(1, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 1);

        vrc1.AddRange(0, 3);
        vrc2.Clear();
        vrc2.AddRange(2, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 2);

        vrc1.AddRange(0, 3);
        vrc2.Clear();
        vrc2.AddRange(1, 2);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 2, 3);

        // single range 4 versions

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(0, 1);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 1, 4);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(0, 2);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 2, 4);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(0, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 3, 4);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(0, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(1, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 1);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(2, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 2);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(3, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 3);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(1, 2);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 2, 4);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(2, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 2);
        VERIFY_RANGE(vrc1, 1, 3, 4);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(1, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 3, 4);

        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(0, 1);
        vrc2.AddRange(2, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 1, 2);
        VERIFY_RANGE(vrc1, 1, 3, 4);
        
        vrc1.AddRange(0, 4);
        vrc2.Clear();
        vrc2.AddRange(1, 2);
        vrc2.AddRange(3, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 2, 3);

        // single range 5 versions

        vrc1.AddRange(0, 5);
        vrc2.Clear();
        vrc2.AddRange(1, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 4, 5);

        vrc1.AddRange(0, 5);
        vrc2.Clear();
        vrc2.AddRange(2, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 2);
        VERIFY_RANGE(vrc1, 1, 3, 5);

        vrc1.AddRange(0, 5);
        vrc2.Clear();
        vrc2.AddRange(0, 1);
        vrc2.AddRange(2, 3);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 1, 2);
        VERIFY_RANGE(vrc1, 1, 3, 5);

        vrc1.AddRange(0, 5);
        vrc2.Clear();
        vrc2.AddRange(0, 1);
        vrc2.AddRange(3, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 1, 3);
        VERIFY_RANGE(vrc1, 1, 4, 5);

        vrc1.AddRange(0, 5);
        vrc2.Clear();
        vrc2.AddRange(1, 2);
        vrc2.AddRange(4, 5);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 2, 4);

        vrc1.AddRange(0, 5);
        vrc2.Clear();
        vrc2.AddRange(2, 3);
        vrc2.AddRange(4, 5);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 2);
        VERIFY_RANGE(vrc1, 1, 3, 4);

        vrc1.AddRange(0, 5);
        vrc2.Clear();
        vrc2.AddRange(1, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 4, 5);

        vrc1.AddRange(0, 5);
        vrc2.Clear();
        vrc2.AddRange(1, 2);
        vrc2.AddRange(3, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 2, 3);
        VERIFY_RANGE(vrc1, 2, 4, 5);

        // single range 7 versions

        vrc1.AddRange(0, 7);
        vrc2.Clear();
        vrc2.AddRange(1, 2);
        vrc2.AddRange(3, 4);
        vrc2.AddRange(5, 6);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 4);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 2, 3);
        VERIFY_RANGE(vrc1, 2, 4, 5);
        VERIFY_RANGE(vrc1, 3, 6, 7);

        // two ranges, single range remove

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(0, 1);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 1, 5);
        VERIFY_RANGE(vrc1, 1, 6, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(0, 5);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 6, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(4, 5);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 4);
        VERIFY_RANGE(vrc1, 1, 6, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(6, 7);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 7, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(6, 11);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 1);
        VERIFY_RANGE(vrc1, 0, 0, 5);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(10, 11);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 6, 10);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(1, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 4, 5);
        VERIFY_RANGE(vrc1, 2, 6, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(7, 10);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 6, 7);
        VERIFY_RANGE(vrc1, 2, 10, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(4, 7);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 4);
        VERIFY_RANGE(vrc1, 1, 7, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(2, 9);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 2);
        VERIFY_RANGE(vrc1, 1, 9, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(1, 10);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 10, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(6, 11);
        vrc2.Clear();
        vrc2.AddRange(0, 11);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        // two ranges, two range remove

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(-3, -2);
        vrc2.AddRange(-1, 0);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 8, 13);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(13, 14);
        vrc2.AddRange(15, 16);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 8, 13);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(5, 6);
        vrc2.AddRange(7, 8);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 8, 13);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(1, 2);
        vrc2.AddRange(3, 4);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 4);
        VERIFY_RANGE(vrc1, 0, 0, 1);
        VERIFY_RANGE(vrc1, 1, 2, 3);
        VERIFY_RANGE(vrc1, 2, 4, 5);
        VERIFY_RANGE(vrc1, 3, 8, 13);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(9, 10);
        vrc2.AddRange(11, 12);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 4);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 8, 9);
        VERIFY_RANGE(vrc1, 2, 10, 11);
        VERIFY_RANGE(vrc1, 3, 12, 13);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(0, 2);
        vrc2.AddRange(8, 10);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 2, 5);
        VERIFY_RANGE(vrc1, 1, 10, 13);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(3, 5);
        vrc2.AddRange(11, 13);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 3);
        VERIFY_RANGE(vrc1, 1, 8, 11);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(3, 10);
        vrc2.AddRange(12, 13);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 3);
        VERIFY_RANGE(vrc1, 1, 10, 12);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(0, 1);
        vrc2.AddRange(3, 10);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 1, 3);
        VERIFY_RANGE(vrc1, 1, 10, 13);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(0, 13);
        vrc2.AddRange(14, 15);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(-2, -1);
        vrc2.AddRange(0, 13);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc2.Clear();
        vrc2.AddRange(-1, 6);
        vrc2.AddRange(7, 14);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        // three ranges, single range remove

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(8, 9);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 9, 13);
        VERIFY_RANGE(vrc1, 2, 16, 21);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(12, 13);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 8, 12);
        VERIFY_RANGE(vrc1, 2, 16, 21);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(8, 13);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 16, 21);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(4, 17);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 2);
        VERIFY_RANGE(vrc1, 0, 0, 4);
        VERIFY_RANGE(vrc1, 1, 17, 21);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(0, 21);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);

        // three ranges, three range remove

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(0, 2);
        vrc2.AddRange(3, 10);
        vrc2.AddRange(11, 18);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 2, 3);
        VERIFY_RANGE(vrc1, 1, 10, 11);
        VERIFY_RANGE(vrc1, 2, 18, 21);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(3, 10);
        vrc2.AddRange(11, 18);
        vrc2.AddRange(19, 21);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 0, 3);
        VERIFY_RANGE(vrc1, 1, 10, 11);
        VERIFY_RANGE(vrc1, 2, 18, 19);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(2, 3);
        vrc2.AddRange(10, 11);
        vrc2.AddRange(18, 19);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 6);
        VERIFY_RANGE(vrc1, 0, 0, 2);
        VERIFY_RANGE(vrc1, 1, 3, 5);
        VERIFY_RANGE(vrc1, 2, 8, 10);
        VERIFY_RANGE(vrc1, 3, 11, 13);
        VERIFY_RANGE(vrc1, 4, 16, 18);
        VERIFY_RANGE(vrc1, 5, 19, 21);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(-1, 0);
        vrc2.AddRange(5, 7);
        vrc2.AddRange(13, 15);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 8, 13);
        VERIFY_RANGE(vrc1, 2, 16, 21);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(5, 7);
        vrc2.AddRange(13, 15);
        vrc2.AddRange(21, 22);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 3);
        VERIFY_RANGE(vrc1, 0, 0, 5);
        VERIFY_RANGE(vrc1, 1, 8, 13);
        VERIFY_RANGE(vrc1, 2, 16, 21);

        vrc1.AddRange(0, 5);
        vrc1.AddRange(8, 13);
        vrc1.AddRange(16, 21);
        vrc2.Clear();
        vrc2.AddRange(0, 5);
        vrc2.AddRange(8, 13);
        vrc2.AddRange(16, 21);
        vrc1.Remove(vrc2);
        VERIFY_RANGE_COUNT(vrc1, 0);
    }

    BOOST_AUTO_TEST_CASE(TestOptimizedRemovePerformance)
    {
        if (!TestVersionRangeCollection::enablePerformanceTest) 
        { 
            Trace.WriteInfo(TestComponent, "*** TestOptimizedRemovePerformance disabled");
            return; 
        }

        Stopwatch stopwatch;
        VersionRangeCollection vrc1;
        VersionRangeCollection vrc2;

        VersionRangeCollection vrcOptimized;
        VersionRangeCollection vrcOrig;

        size_t count = 100000;

        for (auto ix=0; ix<count; ix += 5)
        {
            vrc1.AddRange(ix, ix + 4);
            vrc2.AddRange(ix + 1, ix + 2);
        }

        stopwatch.Restart();
        vrc1.Remove(vrc2);
        stopwatch.Stop();
        vrcOptimized = vrc1;

        Trace.WriteInfo(TestComponent, "*** Optimized = {0} ({1})", stopwatch.Elapsed, vrc1.VersionRanges.size());

        for (auto ix=0; ix<count; ix += 5)
        {
            vrc1.AddRange(ix, ix + 4);
            vrc2.AddRange(ix + 1, ix + 2);
        }

        stopwatch.Restart();
        vrc1.Test_Remove(vrc2);
        stopwatch.Stop();
        vrcOrig = vrc1;

        Trace.WriteInfo(TestComponent, "*** Orig = {0} ({1})", stopwatch.Elapsed, vrc1.VersionRanges.size());

        VERIFY(vrcOptimized == vrcOrig, wformatString("optimized={0} orig={1}", vrcOptimized.VersionRanges.size(), vrcOrig.VersionRanges.size()));
    }

    BOOST_AUTO_TEST_CASE(TestContains)
    {
        VersionRangeCollection vrc;

        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_FALSE(vrc.Contains(0));
        VERIFY_IS_FALSE(vrc.Contains(1));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(0, 1)));

        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_FALSE(vrc.Contains(1));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(2, 3)));

        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_FALSE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(1, 2)));

        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(-2, -1)));

        VERIFY_IS_FALSE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(-3, -2)));

        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(-5, -4)));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(4, 5)));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));
        VERIFY_IS_TRUE(vrc.Contains(4));
        VERIFY_IS_FALSE(vrc.Contains(5));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(6, 8)));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));
        VERIFY_IS_TRUE(vrc.Contains(4));
        VERIFY_IS_FALSE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_FALSE(vrc.Contains(8));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(10, 12)));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));
        VERIFY_IS_TRUE(vrc.Contains(4));
        VERIFY_IS_FALSE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_FALSE(vrc.Contains(8));
        VERIFY_IS_FALSE(vrc.Contains(9));
        VERIFY_IS_TRUE(vrc.Contains(10));
        VERIFY_IS_TRUE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(15, 16)));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));
        VERIFY_IS_TRUE(vrc.Contains(4));
        VERIFY_IS_FALSE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_FALSE(vrc.Contains(8));
        VERIFY_IS_FALSE(vrc.Contains(9));
        VERIFY_IS_TRUE(vrc.Contains(10));
        VERIFY_IS_TRUE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_TRUE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(2, 6)));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_TRUE(vrc.Contains(3));
        VERIFY_IS_TRUE(vrc.Contains(4));
        VERIFY_IS_TRUE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_FALSE(vrc.Contains(8));
        VERIFY_IS_FALSE(vrc.Contains(9));
        VERIFY_IS_TRUE(vrc.Contains(10));
        VERIFY_IS_TRUE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_TRUE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(-4, -2)));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_TRUE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_TRUE(vrc.Contains(3));
        VERIFY_IS_TRUE(vrc.Contains(4));
        VERIFY_IS_TRUE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_FALSE(vrc.Contains(8));
        VERIFY_IS_FALSE(vrc.Contains(9));
        VERIFY_IS_TRUE(vrc.Contains(10));
        VERIFY_IS_TRUE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_TRUE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(5, 11)));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_TRUE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_TRUE(vrc.Contains(3));
        VERIFY_IS_TRUE(vrc.Contains(4));
        VERIFY_IS_TRUE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_TRUE(vrc.Contains(8));
        VERIFY_IS_TRUE(vrc.Contains(9));
        VERIFY_IS_TRUE(vrc.Contains(10));
        VERIFY_IS_TRUE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_TRUE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));

        VERIFY_IS_TRUE(vrc.Add(VersionRange(numeric_limits<int64>::max()-1, numeric_limits<int64>::max())));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_TRUE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_TRUE(vrc.Contains(3));
        VERIFY_IS_TRUE(vrc.Contains(4));
        VERIFY_IS_TRUE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_TRUE(vrc.Contains(8));
        VERIFY_IS_TRUE(vrc.Contains(9));
        VERIFY_IS_TRUE(vrc.Contains(10));
        VERIFY_IS_TRUE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_TRUE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max() - 2));
        VERIFY_IS_TRUE(vrc.Contains(numeric_limits<int64>::max() - 1));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max()));

        vrc.Remove(VersionRange(3, 6));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_TRUE(vrc.Contains(-5));
        VERIFY_IS_TRUE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));
        VERIFY_IS_FALSE(vrc.Contains(4));
        VERIFY_IS_FALSE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_TRUE(vrc.Contains(8));
        VERIFY_IS_TRUE(vrc.Contains(9));
        VERIFY_IS_TRUE(vrc.Contains(10));
        VERIFY_IS_TRUE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_TRUE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max() - 2));
        VERIFY_IS_TRUE(vrc.Contains(numeric_limits<int64>::max() - 1));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max()));

        vrc.Remove(VersionRange(-5, -3));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_FALSE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_TRUE(vrc.Contains(-3));
        VERIFY_IS_TRUE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_TRUE(vrc.Contains(0));
        VERIFY_IS_TRUE(vrc.Contains(1));
        VERIFY_IS_TRUE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));
        VERIFY_IS_FALSE(vrc.Contains(4));
        VERIFY_IS_FALSE(vrc.Contains(5));
        VERIFY_IS_TRUE(vrc.Contains(6));
        VERIFY_IS_TRUE(vrc.Contains(7));
        VERIFY_IS_TRUE(vrc.Contains(8));
        VERIFY_IS_TRUE(vrc.Contains(9));
        VERIFY_IS_TRUE(vrc.Contains(10));
        VERIFY_IS_TRUE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_TRUE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max() - 2));
        VERIFY_IS_TRUE(vrc.Contains(numeric_limits<int64>::max() - 1));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max()));

        vrc.Remove(VersionRange(-10, 20));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_FALSE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_FALSE(vrc.Contains(-3));
        VERIFY_IS_FALSE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_FALSE(vrc.Contains(0));
        VERIFY_IS_FALSE(vrc.Contains(1));
        VERIFY_IS_FALSE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));
        VERIFY_IS_FALSE(vrc.Contains(4));
        VERIFY_IS_FALSE(vrc.Contains(5));
        VERIFY_IS_FALSE(vrc.Contains(6));
        VERIFY_IS_FALSE(vrc.Contains(7));
        VERIFY_IS_FALSE(vrc.Contains(8));
        VERIFY_IS_FALSE(vrc.Contains(9));
        VERIFY_IS_FALSE(vrc.Contains(10));
        VERIFY_IS_FALSE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_FALSE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max() - 2));
        VERIFY_IS_TRUE(vrc.Contains(numeric_limits<int64>::max() - 1));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max()));

        vrc.Remove(VersionRange(numeric_limits<int64>::max()-1, numeric_limits<int64>::max()));

        VERIFY_IS_FALSE(vrc.Contains(-6));
        VERIFY_IS_FALSE(vrc.Contains(-5));
        VERIFY_IS_FALSE(vrc.Contains(-4));
        VERIFY_IS_FALSE(vrc.Contains(-3));
        VERIFY_IS_FALSE(vrc.Contains(-2));
        VERIFY_IS_FALSE(vrc.Contains(-1));
        VERIFY_IS_FALSE(vrc.Contains(0));
        VERIFY_IS_FALSE(vrc.Contains(1));
        VERIFY_IS_FALSE(vrc.Contains(2));
        VERIFY_IS_FALSE(vrc.Contains(3));
        VERIFY_IS_FALSE(vrc.Contains(4));
        VERIFY_IS_FALSE(vrc.Contains(5));
        VERIFY_IS_FALSE(vrc.Contains(6));
        VERIFY_IS_FALSE(vrc.Contains(7));
        VERIFY_IS_FALSE(vrc.Contains(8));
        VERIFY_IS_FALSE(vrc.Contains(9));
        VERIFY_IS_FALSE(vrc.Contains(10));
        VERIFY_IS_FALSE(vrc.Contains(11));
        VERIFY_IS_FALSE(vrc.Contains(12));
        VERIFY_IS_FALSE(vrc.Contains(13));
        VERIFY_IS_FALSE(vrc.Contains(14));
        VERIFY_IS_FALSE(vrc.Contains(15));
        VERIFY_IS_FALSE(vrc.Contains(16));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max() - 2));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max() - 1));
        VERIFY_IS_FALSE(vrc.Contains(numeric_limits<int64>::max()));
    }

    BOOST_AUTO_TEST_CASE(TestRemoveExact)
    {
        {
            VersionRangeCollection vrc;

            vrc.Remove(-1);
            VERIFY_ARE_EQUAL(0, vrc.VersionRanges.size());

            vrc.Remove(0);
            VERIFY_ARE_EQUAL(0, vrc.VersionRanges.size());

            vrc.Remove(1);
            VERIFY_ARE_EQUAL(0, vrc.VersionRanges.size());
        }

        {
            VersionRangeCollection vrcOrig(0, 1);
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(-1);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(0);
            VERIFY_ARE_EQUAL(0, vrc.VersionRanges.size());

            vrc = vrcOrig;

            vrc.Remove(1);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 2);
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(-1);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(0);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(1 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(1);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(2);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 3);
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(-1);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(0);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(1 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(1);
            VERIFY_ARE_EQUAL(2u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(2);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(3);
            VERIFY_ARE_EQUAL(1u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 1);
            vrcOrig.Add(VersionRange(2, 3));
            vrcOrig.Add(VersionRange(4, 5));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(-1);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(0);
            VERIFY_ARE_EQUAL(2u, vrc.VersionRanges.size());
            VERIFY(2 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(1);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[2].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 1);
            vrcOrig.Add(VersionRange(2, 3));
            vrcOrig.Add(VersionRange(4, 5));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(1);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(2);
            VERIFY_ARE_EQUAL(2u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(3);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[2].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 1);
            vrcOrig.Add(VersionRange(2, 3));
            vrcOrig.Add(VersionRange(4, 5));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(3);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(4);
            VERIFY_ARE_EQUAL(2u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(5);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[2].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 2);
            vrcOrig.Add(VersionRange(3, 5));
            vrcOrig.Add(VersionRange(6, 8));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(0);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(1 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(1);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(2);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 2);
            vrcOrig.Add(VersionRange(3, 5));
            vrcOrig.Add(VersionRange(6, 8));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(3);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(4);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(5);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 2);
            vrcOrig.Add(VersionRange(3, 5));
            vrcOrig.Add(VersionRange(6, 8));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(6);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(7);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(8);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 3);
            vrcOrig.Add(VersionRange(4, 7));
            vrcOrig.Add(VersionRange(8, 11));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(0);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(1 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(1);
            VERIFY_ARE_EQUAL(4u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(1 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[2].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[3].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[3].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(2);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(2 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(3);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[2].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 3);
            vrcOrig.Add(VersionRange(4, 7));
            vrcOrig.Add(VersionRange(8, 11));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(4);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(5);
            VERIFY_ARE_EQUAL(4u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(5 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[2].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[3].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[3].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(6);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(6 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(7);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[2].EndVersion, vrc);
        }

        {
            VersionRangeCollection vrcOrig(0, 3);
            vrcOrig.Add(VersionRange(4, 7));
            vrcOrig.Add(VersionRange(8, 11));
            VersionRangeCollection vrc = vrcOrig;

            vrc.Remove(8);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(9 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(9);
            VERIFY_ARE_EQUAL(4u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(9 == vrc.VersionRanges[2].EndVersion, vrc);
            VERIFY(10 == vrc.VersionRanges[3].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[3].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(10);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(10 == vrc.VersionRanges[2].EndVersion, vrc);

            vrc = vrcOrig;

            vrc.Remove(11);
            VERIFY_ARE_EQUAL(3u, vrc.VersionRanges.size());
            VERIFY(0 == vrc.VersionRanges[0].StartVersion, vrc);
            VERIFY(3 == vrc.VersionRanges[0].EndVersion, vrc);
            VERIFY(4 == vrc.VersionRanges[1].StartVersion, vrc);
            VERIFY(7 == vrc.VersionRanges[1].EndVersion, vrc);
            VERIFY(8 == vrc.VersionRanges[2].StartVersion, vrc);
            VERIFY(11 == vrc.VersionRanges[2].EndVersion, vrc);
        }
    }

    // Test basic functionality of Merge()
    //
    BOOST_AUTO_TEST_CASE(TestMerge)
    {
        VersionRangeCollection vrc1;

        // merge with empty collection
        vrc1.Merge(VersionRangeCollection(3, 6));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(3 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(6 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // range is subset of existing
        vrc1.Merge(VersionRangeCollection(3, 4));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(3 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(6 == vrc1.VersionRanges[0].EndVersion, vrc1)

        vrc1.Merge(VersionRangeCollection(4, 6));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(3 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(6 == vrc1.VersionRanges[0].EndVersion, vrc1)

        vrc1.Merge(VersionRangeCollection(4, 5));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(3 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(6 == vrc1.VersionRanges[0].EndVersion, vrc1)

        vrc1.Merge(VersionRangeCollection(3, 6));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(3 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(6 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // range extends start version only
        vrc1.Merge(VersionRangeCollection(2, 4));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(2 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(6 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // range extends end version only
        vrc1.Merge(VersionRangeCollection(4, 7));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(2 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(7 == vrc1.VersionRanges[0].EndVersion, vrc1)
        
        // range extends both start and end versions
        vrc1.Merge(VersionRangeCollection(1, 8));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(8 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // range adds a hole exactly at end
        vrc1.Merge(VersionRangeCollection(9, 15));
        VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(8 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(9 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(15 == vrc1.VersionRanges[1].EndVersion, vrc1)

        // range merges hole exactly
        vrc1.Merge(VersionRangeCollection(8, 9));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(15 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // range adds a hole past end
        vrc1.Merge(VersionRangeCollection(17, 20));
        VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(15 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(17 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(20 == vrc1.VersionRanges[1].EndVersion, vrc1)
        
        // range merges hole overlapping start version
        vrc1.Merge(VersionRangeCollection(14, 17));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(20 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // range adds a hole past end
        vrc1.Merge(VersionRangeCollection(25, 30));
        VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(20 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(25 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(30 == vrc1.VersionRanges[1].EndVersion, vrc1)
        
        // range merges hole overlapping end version
        vrc1.Merge(VersionRangeCollection(20, 26));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(30 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // add two holes
        vrc1.Merge(VersionRangeCollection(31, 32));
        vrc1.Merge(VersionRangeCollection(33, 34));
        VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(30 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(31 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(32 == vrc1.VersionRanges[1].EndVersion, vrc1)
        VERIFY(33 == vrc1.VersionRanges[2].StartVersion, vrc1)
        VERIFY(34 == vrc1.VersionRanges[2].EndVersion, vrc1)
        
        // range merges across two holes
        vrc1.Merge(VersionRangeCollection(30, 33));
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(34 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // add two holes at once
        VersionRangeCollection vrc2(35,36);
        vrc2.Merge(VersionRangeCollection(37, 38));
        vrc1.Merge(vrc2);
        VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(34 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(35 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(36 == vrc1.VersionRanges[1].EndVersion, vrc1)
        VERIFY(37 == vrc1.VersionRanges[2].StartVersion, vrc1)
        VERIFY(38 == vrc1.VersionRanges[2].EndVersion, vrc1)

        // add two more holes and merge both collections (don't clear vrc2)
        vrc2.Merge(VersionRangeCollection(39, 40));
        vrc2.Merge(VersionRangeCollection(41, 42));
        vrc1.Merge(vrc2);
        VERIFY(5u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(34 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(35 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(36 == vrc1.VersionRanges[1].EndVersion, vrc1)
        VERIFY(37 == vrc1.VersionRanges[2].StartVersion, vrc1)
        VERIFY(38 == vrc1.VersionRanges[2].EndVersion, vrc1)
        VERIFY(39 == vrc1.VersionRanges[3].StartVersion, vrc1)
        VERIFY(40 == vrc1.VersionRanges[3].EndVersion, vrc1)
        VERIFY(41 == vrc1.VersionRanges[4].StartVersion, vrc1)
        VERIFY(42 == vrc1.VersionRanges[4].EndVersion, vrc1)

        // fill in all holes
        vrc2.Clear();
        vrc2.Merge(VersionRangeCollection(34, 35));
        vrc2.Merge(VersionRangeCollection(36, 37));
        vrc2.Merge(VersionRangeCollection(38, 39));
        vrc2.Merge(VersionRangeCollection(40, 41));
        vrc1.Merge(vrc2);
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(42 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // input only needs to be in sorted non-decreasing start version order
        vrc2.Clear();
        vrc2.Merge(VersionRangeCollection(1, 5));
        vrc2.Merge(VersionRangeCollection(1, 4));
        vrc2.Merge(VersionRangeCollection(1, 3));
        vrc2.Merge(VersionRangeCollection(2, 3));
        vrc2.Merge(VersionRangeCollection(3, 4));
        vrc2.Merge(VersionRangeCollection(3, 7));
        vrc2.Merge(VersionRangeCollection(3, 5));
        vrc2.Merge(VersionRangeCollection(3, 4));
        vrc2.Merge(VersionRangeCollection(10, 15));
        vrc2.Merge(VersionRangeCollection(10, 14));
        vrc2.Merge(VersionRangeCollection(10, 13));
        vrc2.Merge(VersionRangeCollection(40, 42));
        vrc2.Merge(VersionRangeCollection(40, 41));
        vrc1.Merge(vrc2);
        VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(42 == vrc1.VersionRanges[0].EndVersion, vrc1)

        // add two holes at once
        vrc2.Clear();
        vrc2.Merge(VersionRangeCollection(43, 45));
        vrc2.Merge(VersionRangeCollection(46, 48));
        vrc1.Merge(vrc2);
        VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(42 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(43 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(45 == vrc1.VersionRanges[1].EndVersion, vrc1)
        VERIFY(46 == vrc1.VersionRanges[2].StartVersion, vrc1)
        VERIFY(48 == vrc1.VersionRanges[2].EndVersion, vrc1)

        // test special-case optimization of merging to end: equal to start version
        vrc1.Merge(VersionRangeCollection(46, 49));
        VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(42 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(43 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(45 == vrc1.VersionRanges[1].EndVersion, vrc1)
        VERIFY(46 == vrc1.VersionRanges[2].StartVersion, vrc1)
        VERIFY(49 == vrc1.VersionRanges[2].EndVersion, vrc1)
        
        // test special-case optimization of merging to end: greater than start version
        vrc1.Merge(VersionRangeCollection(48, 50));
        VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(42 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(43 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(45 == vrc1.VersionRanges[1].EndVersion, vrc1)
        VERIFY(46 == vrc1.VersionRanges[2].StartVersion, vrc1)
        VERIFY(50 == vrc1.VersionRanges[2].EndVersion, vrc1)
        
        // test special-case optimization of merging to end: equal to end version
        vrc1.Merge(VersionRangeCollection(50, 51));
        VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(42 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(43 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(45 == vrc1.VersionRanges[1].EndVersion, vrc1)
        VERIFY(46 == vrc1.VersionRanges[2].StartVersion, vrc1)
        VERIFY(51 == vrc1.VersionRanges[2].EndVersion, vrc1)
        
        // test special-case optimization of merging to end: greater than end version
        vrc1.Merge(VersionRangeCollection(52, 53));
        VERIFY(4u == vrc1.VersionRanges.size(), vrc1)
        VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
        VERIFY(42 == vrc1.VersionRanges[0].EndVersion, vrc1)
        VERIFY(43 == vrc1.VersionRanges[1].StartVersion, vrc1)
        VERIFY(45 == vrc1.VersionRanges[1].EndVersion, vrc1)
        VERIFY(46 == vrc1.VersionRanges[2].StartVersion, vrc1)
        VERIFY(51 == vrc1.VersionRanges[2].EndVersion, vrc1)
        VERIFY(52 == vrc1.VersionRanges[3].StartVersion, vrc1)
        VERIFY(53 == vrc1.VersionRanges[3].EndVersion, vrc1)
    }

    BOOST_AUTO_TEST_CASE(TestSplit)
    {
        // test empty range collections
        {
            VersionRangeCollection vrc1;
            VersionRangeCollection vrc2;

            vrc1.Split(-1, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
        }

        {
            VersionRangeCollection vrc1;
            VersionRangeCollection vrc2;

            vrc1.Split(0, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
        }

        {
            VersionRangeCollection vrc1;
            VersionRangeCollection vrc2;

            vrc1.Split(1, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
        }

        // test single range of size 1 (start and end only)
        {
            VersionRangeCollection vrc1(0, 1);
            VersionRangeCollection vrc2;

            vrc1.Split(-1, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(1 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 1);
            VersionRangeCollection vrc2;

            vrc1.Split(0, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(1 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 1);
            VersionRangeCollection vrc2;

            vrc1.Split(1, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
        }

        {
            VersionRangeCollection vrc1(0, 1);
            VersionRangeCollection vrc2;

            vrc1.Split(2, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
        }

        // test single range of size > 1 (start, middle, and end)
        {
            VersionRangeCollection vrc1(0, 2);
            VersionRangeCollection vrc2;

            vrc1.Split(-1, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            VersionRangeCollection vrc2;

            vrc1.Split(0, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            VersionRangeCollection vrc2;

            vrc1.Split(1, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(1 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            VersionRangeCollection vrc2;

            vrc1.Split(2, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
        }

        // test two ranges each of size 1
        {
            VersionRangeCollection vrc1(0, 1);
            vrc1.Add(VersionRange(2, 3));
            VersionRangeCollection vrc2;

            vrc1.Split(-1, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(2u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(1 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[1].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 1);
            vrc1.Add(VersionRange(2, 3));
            VersionRangeCollection vrc2;

            vrc1.Split(0, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(2u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(1 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[1].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 1);
            vrc1.Add(VersionRange(2, 3));
            VersionRangeCollection vrc2;

            vrc1.Split(1, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(2 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 1);
            vrc1.Add(VersionRange(2, 3));
            VersionRangeCollection vrc2;

            vrc1.Split(2, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(2 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 1);
            vrc1.Add(VersionRange(2, 3));
            VersionRangeCollection vrc2;

            vrc1.Split(3, vrc2);

            VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].EndVersion, vrc1)
        }

        {
            VersionRangeCollection vrc1(0, 1);
            vrc1.Add(VersionRange(2, 3));
            VersionRangeCollection vrc2;

            vrc1.Split(4, vrc2);

            VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].EndVersion, vrc1)
        }

        // test two ranges each of size > 1
        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            VersionRangeCollection vrc2;

            vrc1.Split(-1, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(2u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[1].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            VersionRangeCollection vrc2;

            vrc1.Split(0, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(2u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[1].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            VersionRangeCollection vrc2;

            vrc1.Split(1, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(2u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(1 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[1].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            VersionRangeCollection vrc2;

            vrc1.Split(2, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            VersionRangeCollection vrc2;

            vrc1.Split(3, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            VersionRangeCollection vrc2;

            vrc1.Split(4, vrc2);

            VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(4 == vrc1.VersionRanges[1].EndVersion, vrc1)
            VERIFY(4 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            VersionRangeCollection vrc2;

            vrc1.Split(5, vrc2);

            VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(5 == vrc1.VersionRanges[1].EndVersion, vrc1)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            VersionRangeCollection vrc2;

            vrc1.Split(6, vrc2);

            VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(5 == vrc1.VersionRanges[1].EndVersion, vrc1)
        }

        // test three ranges (middle range exists)
        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(-1, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(3u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[1].EndVersion, vrc2)
            VERIFY(6 == vrc2.VersionRanges[2].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[2].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(0, vrc2);

            VERIFY(0u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(3u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[1].EndVersion, vrc2)
            VERIFY(6 == vrc2.VersionRanges[2].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[2].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(1, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(3u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(1 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(1 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(2 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(3 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[1].EndVersion, vrc2)
            VERIFY(6 == vrc2.VersionRanges[2].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[2].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(2, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(2u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(6 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[1].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(3, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(2u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(6 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[1].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(4, vrc2);

            VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(2u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(4 == vrc1.VersionRanges[1].EndVersion, vrc1)
            VERIFY(4 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(5 == vrc2.VersionRanges[0].EndVersion, vrc2)
            VERIFY(6 == vrc2.VersionRanges[1].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[1].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(5, vrc2);

            VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(5 == vrc1.VersionRanges[1].EndVersion, vrc1)
            VERIFY(6 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(6, vrc2);

            VERIFY(2u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(5 == vrc1.VersionRanges[1].EndVersion, vrc1)
            VERIFY(6 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(7, vrc2);

            VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(5 == vrc1.VersionRanges[1].EndVersion, vrc1)
            VERIFY(6 == vrc1.VersionRanges[2].StartVersion, vrc1)
            VERIFY(7 == vrc1.VersionRanges[2].EndVersion, vrc1)
            VERIFY(7 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(8 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(8, vrc2);

            VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(5 == vrc1.VersionRanges[1].EndVersion, vrc1)
            VERIFY(6 == vrc1.VersionRanges[2].StartVersion, vrc1)
            VERIFY(8 == vrc1.VersionRanges[2].EndVersion, vrc1)
        }

        {
            VersionRangeCollection vrc1(0, 2);
            vrc1.Add(VersionRange(3, 5));
            vrc1.Add(VersionRange(6, 8));
            VersionRangeCollection vrc2;

            vrc1.Split(9, vrc2);

            VERIFY(3u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(0u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(0 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(2 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[1].StartVersion, vrc1)
            VERIFY(5 == vrc1.VersionRanges[1].EndVersion, vrc1)
            VERIFY(6 == vrc1.VersionRanges[2].StartVersion, vrc1)
            VERIFY(8 == vrc1.VersionRanges[2].EndVersion, vrc1)
        }

        // test hole size > 1
        {
            VersionRangeCollection vrc1(1, 3);
            vrc1.Add(VersionRange(7, 9));
            VersionRangeCollection vrc2;

            vrc1.Split(4, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(7 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(9 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(1, 3);
            vrc1.Add(VersionRange(7, 9));
            VersionRangeCollection vrc2;

            vrc1.Split(5, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(7 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(9 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }

        {
            VersionRangeCollection vrc1(1, 3);
            vrc1.Add(VersionRange(7, 9));
            VersionRangeCollection vrc2;

            vrc1.Split(6, vrc2);

            VERIFY(1u == vrc1.VersionRanges.size(), vrc1)
            VERIFY(1u == vrc2.VersionRanges.size(), vrc2)
            VERIFY(1 == vrc1.VersionRanges[0].StartVersion, vrc1)
            VERIFY(3 == vrc1.VersionRanges[0].EndVersion, vrc1)
            VERIFY(7 == vrc2.VersionRanges[0].StartVersion, vrc2)
            VERIFY(9 == vrc2.VersionRanges[0].EndVersion, vrc2)
        }
    }

    BOOST_AUTO_TEST_CASE(TestSplitRandom)
    {
        Random rand(static_cast<int>(DateTime::Now().Ticks));

        int iterations = 50;
        for (auto ix=0; ix<iterations; ++ix)
        {
            VersionRangeCollection vrc1;
            VersionRangeCollection vrc2;

            int maxRangeSize = 10;
            int maxRangeCount = 10;
            int64 baseValue = 0;

            int rangeCount = rand.Next(0, maxRangeCount);
            for (auto jx=0; jx<rangeCount; ++jx)
            {
                auto start = baseValue + rand.Next(0, maxRangeSize);
                auto end = start + rand.Next(1, maxRangeSize);
                baseValue = end;

                vrc1.Add(VersionRange(start, end));
            }

            int64 split = rand.Next(-2, static_cast<int>(baseValue) + 2);

            Trace.WriteInfo(TestComponent, "*** Splitting {0} at {1}", vrc1, split);

            vrc1.Split(split, vrc2);

            Trace.WriteInfo(TestComponent, "*** Result {0} / {1}", vrc1, vrc2);

            for (auto const & range : vrc1.VersionRanges)
            {
                VERIFY(split > range.StartVersion, range);
                VERIFY(split >= range.EndVersion, range);
            }

            for (auto const & range : vrc2.VersionRanges)
            {
                VERIFY(split <= range.StartVersion, range);
                VERIFY(split < range.EndVersion, range);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(TestMergeRandom)
    {
        Random rand(static_cast<int>(DateTime::Now().Ticks));

        int iterations = 50;
        for (auto ix=0; ix<iterations; ++ix)
        {
            this->TestMergeRandomHelper(ix, rand);
        }
    }

    // Test that the non-optimized Add() and optimized Merge()
    // produces the same results
    //

    BOOST_AUTO_TEST_CASE(TestOptimizedMergePerformance)
    {
        if (!TestVersionRangeCollection::enablePerformanceTest) 
        { 
            Trace.WriteInfo(TestComponent, "*** TestOptimizedMergePerformance disabled");
            return; 
        }

        vector<size_t> counts;
        counts.push_back(200);
        counts.push_back(2000);
        counts.push_back(20000);
        counts.push_back(40000);
        counts.push_back(80000);
        counts.push_back(100000);

        for (auto ix=0; ix<counts.size(); ++ix)
        {
            this->TestMergeLargeCollectionsHelper(counts[ix]);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestVersionRangeCollection::TestMergeRandomHelper(int iteration, Random & rand)
    {
        VersionRangeCollection incoming;
        VersionRangeCollection vrcadd;
        VersionRangeCollection vrcmerge;

        int maxRangeSize = 10;
        int maxRangeCount = 10;

        int rangeCount1 = rand.Next(0, maxRangeCount);
        int rangeCount2 = rand.Next(0, maxRangeCount);

        for (auto ix=0; ix<rangeCount1; ++ix)
        {
            int startVersion = rand.Next(0, rangeCount1 * maxRangeSize / 2);
            int endVersion = rand.Next(startVersion + 1, startVersion + 1 + maxRangeSize );

            incoming.Add(VersionRange(startVersion, endVersion));
        }

        for (auto ix=0; ix<rangeCount2; ++ix)
        {
            int startVersion = rand.Next(0, rangeCount2 * maxRangeSize / 2);
            int endVersion = rand.Next(startVersion + 1, startVersion + 1 + maxRangeSize );

            vrcadd.Add(VersionRange(startVersion, endVersion));
            vrcmerge.Merge(VersionRangeCollection(startVersion, endVersion));
        }

        Trace.WriteInfo(TestComponent, "*** TestMergeRandom {0}: {1} into {2} & {3}", iteration, incoming, vrcadd, vrcmerge);

        vrcadd.Test_Add(incoming);
        vrcmerge.Merge(incoming);

        Trace.WriteInfo(TestComponent, "Result {0}: {1} & {2}", iteration, vrcadd, vrcmerge);

        this->VerifyMerge(vrcadd, vrcmerge);
    }

    // Tests the performance of merging large collections
    //
    void TestVersionRangeCollection::TestMergeLargeCollectionsHelper(size_t count)
    {
        Stopwatch stopwatch;
        VersionRangeCollection incoming;
        VersionRangeCollection vrcadd;
        VersionRangeCollection vrcmerge;

        Trace.WriteInfo(TestComponent, "*** MergeLargeCollections ({0})", count);

        for (auto ix=0; ix<count; ix+=2)
        {
            incoming.Merge(VersionRangeCollection(ix,ix+1));
        }

        stopwatch.Restart();
        vrcmerge.Merge(incoming);
        stopwatch.Stop();

        Trace.WriteInfo(TestComponent, "Merge: {0}", stopwatch.Elapsed);
        Trace.WriteInfo(TestComponent, "{0} : {1}", vrcmerge.VersionRanges.size(), vrcmerge.VersionRanges);

        stopwatch.Restart();
        vrcadd.Test_Add(incoming);
        stopwatch.Stop();

        Trace.WriteInfo(TestComponent, "Add: {0}", stopwatch.Elapsed);
        Trace.WriteInfo(TestComponent, "{0} : {1}", vrcadd.VersionRanges.size(), vrcadd.VersionRanges);

        this->VerifyMerge(vrcadd, vrcmerge);
    }

    void TestVersionRangeCollection::VerifyMerge(VersionRangeCollection const & baseline, VersionRangeCollection const & merged)
    {
        VERIFY2(baseline.VersionRanges.size() == merged.VersionRanges.size(), baseline, merged)

        for (auto ix=0; ix<baseline.VersionRanges.size(); ++ix)
        {
            VERIFY(baseline.VersionRanges[ix].StartVersion == merged.VersionRanges[ix].StartVersion, ix)
            VERIFY(baseline.VersionRanges[ix].EndVersion == merged.VersionRanges[ix].EndVersion, ix)
        }
    }
}
