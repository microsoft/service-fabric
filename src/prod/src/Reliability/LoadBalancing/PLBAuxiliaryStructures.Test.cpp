// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Accumulator.h"
#include "AccumulatorWithMinMax.h"
#include "DynamicBitSet.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ServiceModel/federation/NodeId.h"
#include "ServiceModel/federation/NodeIdGenerator.h"

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Reliability::LoadBalancingComponent;

    // Tests for auxiliary structures in PLB (accumulators, bit sets, etc.)
    class TestAuxiliaryStructures
    {
    protected:
        void CompareSets(DynamicBitSet const& set, std::set<size_t> const& controlSet);
    };

    BOOST_FIXTURE_TEST_SUITE(TestAuxiliaryStructuresSuite, TestAuxiliaryStructures)

    BOOST_AUTO_TEST_CASE(AccumulatorBasicTest)
    {
        AccumulatorWithMinMax s(false);
        Federation::NodeId id;
        Federation::NodeIdGenerator::GenerateFromString(L"WorkerRole1.0", id);
        VERIFY_ARE_EQUAL(L"6e2eb710b98296decd1fb3d31b6e54df", id.ToString());

        s.AddOneValue(1, 0, Federation::NodeId::MinNodeId);
        s.AddOneValue(2, 0, Federation::NodeId::MaxNodeId);
        s.AddOneValue(3, 0, id);

        VERIFY_ARE_EQUAL(2.0, s.Average);
        VERIFY_ARE_EQUAL(6.0, s.Sum);
        VERIFY_ARE_EQUAL(14.0, s.SquaredSum);
        VERIFY_ARE_EQUAL(1.0, s.Min);
        VERIFY_ARE_EQUAL(3.0, s.Max);
        VERIFY_ARE_EQUAL(Federation::NodeId::MinNodeId, s.MinNode);
        VERIFY_ARE_EQUAL(id, s.MaxNode);

        s.AdjustOneValue(1, 3, 0, Federation::NodeId::MinNodeId);
        s.AdjustOneValue(3, 10, 0, Federation::NodeId::MinNodeId);

        VERIFY_ARE_EQUAL(5.0, s.Average);
        VERIFY_ARE_EQUAL(15.0, s.Sum);
        VERIFY_ARE_EQUAL(113.0, s.SquaredSum);
        VERIFY_ARE_EQUAL(10.0, s.Max);
    }


    BOOST_AUTO_TEST_CASE(EmptyTest)
    {
        AccumulatorWithMinMax s(false);
        VERIFY_ARE_EQUAL(0.0, s.NormStdDev);
        AccumulatorWithMinMax s2(true);
        VERIFY_ARE_EQUAL(0.0, s2.NormStdDev);
    }

    BOOST_AUTO_TEST_CASE(DynamicBitSetBasicTest)
    {
        // This set will grow
        DynamicBitSet set1(0);
        // This set will never grow
        DynamicBitSet set2(1042);

        VERIFY_ARE_EQUAL(set1.Count, 0);
        VERIFY_ARE_EQUAL(set2.Count, 0);

        // Control structures (we trust in std::set)
        std::set<size_t> controlSet;
        std::vector<size_t> elementsToRemove;

        // Random insert of 100 elements between 1 and 1000
        for (size_t count = 0; count < 100; ++count)
        {
            size_t element = (std::rand() % 1000) + 1;
            set1.Add(element);
            set2.Add(element);
            controlSet.insert(element);
            if ((std::rand() % 2) == 1)
            {
                elementsToRemove.push_back(element);
            }
        }

        CompareSets(set1, controlSet);
        CompareSets(set2, controlSet);

        // Remove elements from the set
        for (size_t element : elementsToRemove)
        {
            set1.Delete(element);
            set2.Delete(element);
            controlSet.erase(element);
        }

        CompareSets(set1, controlSet);
        CompareSets(set2, controlSet);
    }

    BOOST_AUTO_TEST_CASE(DynamicBitSetForEachTest)
    {
        // This set will grow
        DynamicBitSet bitSet(0);

        // Control structures (we trust in std::set)
        std::set<size_t> controlSet;

        // Random insert of 100 elements between 1 and 1000
        for (size_t count = 0; count < 100; ++count)
        {
            size_t element = (std::rand() % 1000) + 1;
            bitSet.Add(element);
            controlSet.insert(element);
        }

        // Special case (around the edges of words in bitmap)
        bitSet.Add(31);
        bitSet.Add(32);
        bitSet.Add(63);
        bitSet.Add(64);
        controlSet.insert(31);
        controlSet.insert(32);
        controlSet.insert(63);
        controlSet.insert(64);

        std::set<size_t> forEachOutput;

        bitSet.ForEach([&forEachOutput](size_t element) -> void
        {
            // Sanity check: can't have the same element 2 times
            VERIFY_IS_FALSE(forEachOutput.find(element) != forEachOutput.end());
            forEachOutput.insert(element);
        });

        // All sets must be the same
        CompareSets(bitSet, controlSet);
        CompareSets(bitSet, forEachOutput);
        VERIFY_ARE_EQUAL(controlSet, forEachOutput);
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestAuxiliaryStructures::CompareSets(DynamicBitSet const& set, std::set<size_t> const& controlSet)
    {
        VERIFY_ARE_EQUAL(set.Count, controlSet.size());
        for (size_t element : controlSet)
        {
            VERIFY_IS_TRUE(set.Check(element));
        }
    }
}
