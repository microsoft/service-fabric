//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;

const StringLiteral TraceType("OverlayReservationManagerTest");

class OverlayNetworkReservationManagerTest
{
protected:
    OverlayNetworkReservationManagerTest() : manager_() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup)
    ~OverlayNetworkReservationManagerTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup)

    std::unordered_set<OverlayNetworkResourceSPtr> OverlayNetworkReservationManagerTest::CreateNetworkResources(int count, uint start, uint delta);

    void VerifyCounts(
        int added, int removed, 
        int expectedFree, int expectedReserved, int expectedAdded, int expectedRemoved);

    OverlayNetworkReservationManager manager_;
};

BOOST_FIXTURE_TEST_SUITE(OverlayNetworkReservationManagerTestSuite, OverlayNetworkReservationManagerTest)

BOOST_AUTO_TEST_CASE(InitialLoadTest)
{
    std::unordered_set<OverlayNetworkResourceSPtr> networkResourcesToBeAdded = this->CreateNetworkResources(2, 1, 1);
    std::unordered_set<OverlayNetworkResourceSPtr> networkResourcesToBeRemoved;
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    VerifyCounts(added, removed, 0, 0, -1, -1);

    this->manager_.AddNetworkResources(networkResourcesToBeAdded, networkResourcesToBeRemoved, conflicts, added, removed);

    VerifyCounts(added, removed, 2, 0, 2, 0);
}

BOOST_AUTO_TEST_SUITE_END()

std::unordered_set<OverlayNetworkResourceSPtr> OverlayNetworkReservationManagerTest::CreateNetworkResources(int count, uint start, uint delta)
{
    std::unordered_set<OverlayNetworkResourceSPtr> result;
    uint ip = start;
    uint64 mac = start + delta;

    for (int i = 0; i < count; i++)
    {
        auto nr = make_shared<OverlayNetworkResource>(ip, mac);
        result.insert(nr);
        ip += delta;
    }

    return result;
}

void OverlayNetworkReservationManagerTest::VerifyCounts(
    int added, int removed, 
    int expectedFree, int expectedReserved, int expectedAdded, int expectedRemoved)
{
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, expectedFree);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, expectedReserved);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, expectedFree + expectedReserved);
    VERIFY_ARE_EQUAL(added, expectedAdded);
    VERIFY_ARE_EQUAL(removed, expectedRemoved);
}

bool OverlayNetworkReservationManagerTest::Setup()
{
    return true;
}

bool OverlayNetworkReservationManagerTest::Cleanup()
{
    return true;
}
