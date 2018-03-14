// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Node;
using namespace Infrastructure;
using namespace ServiceModel;
using namespace Common;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestExpiringMap
{
protected:
    TestExpiringMap() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestExpiringMap() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void Insert(int key, int value, int64 timeTicks) const;
    void Assign(int key, int value, int64 timeTicks) const;
    void PerformCleanup(int64 timeTicks) const;

    void VerifyValue(int key, int value) const;
    void VerifyNotInMap(int key) const;

    unique_ptr<ExpiringMap<int, int>> map_;
    unique_ptr<UnitTestContext> testContext_;
};

bool TestExpiringMap::TestSetup()
{
    testContext_ = UnitTestContext::Create();

    map_ = make_unique<ExpiringMap<int, int>>();

    return true;
}

void TestExpiringMap::Insert(int key, int value, int64 timeTicks) const
{
    auto it = map_->insert(make_pair(key, value), StopwatchTime(timeTicks));
}

void TestExpiringMap::Assign(int key, int value, int64 timeTicks) const
{
    auto it = map_->insert_or_assign(key, value, StopwatchTime(timeTicks));
}

void TestExpiringMap::PerformCleanup(int64 timeTicks) const
{
    map_->PerformCleanup(StopwatchTime(timeTicks));
}

void TestExpiringMap::VerifyValue(int key, int value) const
{
    auto it = map_->find(key);
    VERIFY_ARE_EQUAL(value, it->second.first);
}

void TestExpiringMap::VerifyNotInMap(int key) const
{
    auto it = map_->find(key);
    VERIFY_ARE_EQUAL(map_->end(), it);
}

bool TestExpiringMap::TestCleanup()
{
    testContext_->Cleanup();
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestExpiringMapSuite, TestExpiringMap)

BOOST_AUTO_TEST_CASE(InsertNew_Value)
{
    Insert(1, 2, 0);
    VerifyValue(1, 2);
}

BOOST_AUTO_TEST_CASE(InsertNew_Cleanup)
{
    Insert(1, 2, 0);
    PerformCleanup(1);
    VerifyNotInMap(1);
}

BOOST_AUTO_TEST_CASE(InsertNew_NoCleanup)
{
    Insert(1, 2, 10);
    PerformCleanup(1);
    VerifyValue(1, 2);
}

BOOST_AUTO_TEST_CASE(AssignNew_Value)
{
    Assign(1, 2, 0);
    VerifyValue(1, 2);
}

BOOST_AUTO_TEST_CASE(AssignNew_Cleanup)
{
    Assign(1, 2, 0);
    PerformCleanup(1);
    VerifyNotInMap(1);
}

BOOST_AUTO_TEST_CASE(AssignNew_NoCleanup)
{
    Assign(1, 2, 10);
    PerformCleanup(1);
    VerifyValue(1, 2);
}

BOOST_AUTO_TEST_CASE(AssignExisting_Value)
{
    Assign(1, 2, 0);
    Assign(1, 5, 0);

    VerifyValue(1, 5);
}

BOOST_AUTO_TEST_CASE(AssignExisting_Cleanup)
{
    Assign(1, 2, 0);
    Assign(1, 5, 5);

    PerformCleanup(10);
    VerifyNotInMap(1);
}

BOOST_AUTO_TEST_CASE(AssignExisting_NoCleanup)
{
    Assign(1, 2, 0);
    Assign(1, 5, 15);

    PerformCleanup(10);
    VerifyValue(1, 5);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
