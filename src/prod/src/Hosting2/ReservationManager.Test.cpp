// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;

const StringLiteral TraceType("ReservationManagerTest");

class ReservationManagerTest
{
protected:
    ReservationManagerTest() : manager_() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup)
    ~ReservationManagerTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup)

    list<uint> *CreateIPs(int count, uint start, uint delta);
    void VerifyCounts(
        int added, int removed, 
        int expectedFree, int expectedReserved, int expectedAdded, int expectedRemoved);

    ReservationManager manager_;
};

BOOST_FIXTURE_TEST_SUITE(ReservationManagerTestSuite, ReservationManagerTest)

BOOST_AUTO_TEST_CASE(InitialLoadTest)
{
    list<uint> *newIPs = this->CreateIPs(2, 1000, 1);
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    VerifyCounts(added, removed, 0, 0, -1, -1);

    this->manager_.AddIPs(*newIPs, conflicts, added, removed);

    VerifyCounts(added, removed, 2, 0, 2, 0);

    delete newIPs;
}

BOOST_AUTO_TEST_CASE(UpdateLoadNoReservationsTest)
{
    list<uint> *firstIPs = this->CreateIPs(10, 1000, 10);
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    VerifyCounts(added, removed, 10, 0, 10, 0);

    // load exactly the same ones again

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    VerifyCounts(added, removed, 10, 0, 0, 0);

    // add a new IP into the set

    firstIPs->push_back(2000);

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    VerifyCounts(added, removed, 11, 0, 1, 0);

    // remove an IP from the set.

    uint first = firstIPs->front();
    firstIPs->pop_front();
    firstIPs->pop_front();
    firstIPs->push_back(first);

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    VerifyCounts(added, removed, 10, 0, 0, 1);

    // both add and remove an IP from the set.

    first = firstIPs->front();
    firstIPs->pop_front();
    firstIPs->pop_front();
    firstIPs->push_back(first);
    firstIPs->push_back(3000);

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    VerifyCounts(added, removed, 10, 0, 1, 1);

    delete firstIPs;
}

BOOST_AUTO_TEST_CASE(BasicReservationTest)
{
    list<uint> *firstIPs = this->CreateIPs(10, 1000, 10);
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    // Reserve a single entry
    uint ip;
    uint ip2;

    bool result = this->manager_.Reserve(L"Test", ip);

    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    result = this->manager_.Reserve(L"Test", ip2);

    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(ip, ip2);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    delete firstIPs;
}

BOOST_AUTO_TEST_CASE(BasicReleaseTest)
{
    list<uint> *firstIPs = this->CreateIPs(10, 1000, 10);
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    // Reserve a single entry
    uint ip;

    bool result = this->manager_.Reserve(L"Test", ip);

    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    // Try to release something else

    result = this->manager_.Release(L"Test2");

    VERIFY_IS_FALSE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    // Now release the actual reservation

    result = this->manager_.Release(L"Test");

    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 10);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 0);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    // Now do it again (expecting it to not be found this time)

    result = this->manager_.Release(L"Test");

    VERIFY_IS_FALSE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 10);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 0);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    delete firstIPs;
}

BOOST_AUTO_TEST_CASE(ExhaustPoolTest)
{
    list<uint> *firstIPs = this->CreateIPs(10, 1000, 10);
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    std::set<uint> returnedIPs;

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    uint ip;
    for (int i = 0; i < 10; i++)
    {
        wstringstream name;
        name << L"Test" << i;
        bool result = this->manager_.Reserve(name.str(), ip);
        returnedIPs.insert(ip);
        VERIFY_IS_TRUE(result);
    }

    VERIFY_ARE_EQUAL(returnedIPs.size(), 10);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 0);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 10);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    bool exhaust = this->manager_.Reserve(L"ExhaustedIP", ip);

    VERIFY_IS_FALSE(exhaust);
    VERIFY_ARE_EQUAL(ip, ReservationManager::INVALID_IP);

    VERIFY_ARE_EQUAL(returnedIPs.size(), 10);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 0);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 10);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    delete firstIPs;
}

BOOST_AUTO_TEST_CASE(ReserveSpecificTest)
{
    list<uint> *firstIPs = this->CreateIPs(10, 1000, 10);
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    // First, reserve something that exists

    bool result = this->manager_.ReserveSpecific(L"Case1", 1010);

    VERIFY_IS_TRUE(result);

    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    // Now, do it again and see that it is a no-op

    result = this->manager_.ReserveSpecific(L"Case1", 1010);

    VERIFY_IS_TRUE(result);

    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    // Now, try to get a different reservation on that IP and see the failure

    result = this->manager_.ReserveSpecific(L"Case2", 1010);

    VERIFY_IS_FALSE(result);

    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    delete firstIPs;
}

BOOST_AUTO_TEST_CASE(ReserveReleaseCycleTest)
{
    list<uint> *firstIPs = this->CreateIPs(10, 1000, 10);
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    uint ip;
    uint ipMatch;

    bool result = this->manager_.Reserve(L"Test", ip);
    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    result = this->manager_.Release(L"Test");

    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 10);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 0);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    // Now cycle through reserve and release 9 times.  We should not see this
    // IP again during that loop.

    for (int i = 0; i < 9; i++)
    {
        result = this->manager_.Reserve(L"Test", ipMatch);
        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
        VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
        VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

        result = this->manager_.Release(L"Test");

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(this->manager_.FreeCount, 10);
        VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 0);
        VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

        VERIFY_ARE_NOT_EQUAL(ip, ipMatch);
    }

    // The first IP should now be the oldest, so try again and expect that
    // IP to match.

    result = this->manager_.Reserve(L"Test", ipMatch);
    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    result = this->manager_.Release(L"Test");

    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 10);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 0);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    VERIFY_ARE_EQUAL(ip, ipMatch);

    delete firstIPs;
}

BOOST_AUTO_TEST_CASE(ForceConflictOnLoadUpdateTest)
{
    list<uint> *firstIPs = this->CreateIPs(10, 1000, 10);
    unordered_set<wstring> conflicts;
    int added = -1;
    int removed = -1;

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);

    uint ip;
    uint ipMatch;

    bool result = this->manager_.Reserve(L"Test", ip);
    VERIFY_IS_TRUE(result);
    VERIFY_ARE_EQUAL(ip, 1000);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    // now update the ip list by removing the matching ip and adding
    // a new one.  Force the update, and verify that the reservation
    // is noticed as a conflict.

    firstIPs->pop_front();
    firstIPs->push_back(2000);

    this->manager_.AddIPs(*firstIPs, conflicts, added, removed);
    VerifyCounts(added, removed, 10, 0, 1, 1);
    VERIFY_ARE_NOT_EQUAL(conflicts.find(L"Test"), conflicts.end());

    result = this->manager_.Reserve(L"Test", ipMatch);
    VERIFY_IS_TRUE(result);
    VERIFY_ARE_NOT_EQUAL(ip, ipMatch);
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, 9);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, 1);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, 10);

    delete firstIPs;
}

BOOST_AUTO_TEST_SUITE_END()

list<uint> *ReservationManagerTest::CreateIPs(int count, uint start, uint delta)
{
    auto result = new list<uint>();
    uint ip = start;

    for (int i = 0; i < count; i++)
    {
        result->push_back(ip);
        ip += delta;
    }

    return result;
}

void ReservationManagerTest::VerifyCounts(
    int added, int removed, 
    int expectedFree, int expectedReserved, int expectedAdded, int expectedRemoved)
{
    VERIFY_ARE_EQUAL(this->manager_.FreeCount, expectedFree);
    VERIFY_ARE_EQUAL(this->manager_.ReservedCount, expectedReserved);
    VERIFY_ARE_EQUAL(this->manager_.TotalCount, expectedFree + expectedReserved);
    VERIFY_ARE_EQUAL(added, expectedAdded);
    VERIFY_ARE_EQUAL(removed, expectedRemoved);
}

bool ReservationManagerTest::Setup()
{
    return true;
}

bool ReservationManagerTest::Cleanup()
{
    return true;
}
