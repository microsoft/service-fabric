// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestFailoverManagerId
{
protected:
    TestFailoverManagerId()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestFailoverManagerId()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

private:
};

bool TestFailoverManagerId::TestSetup()
{
    return true;
}

bool TestFailoverManagerId::TestCleanup()
{
    return true;
}

void RunTest(FailoverManagerId const & fmId, Guid const & g, bool expected, wstring const & tag)
{
    Verify::AreEqual(expected, fmId.IsOwnerOf(g), tag);
    Verify::AreEqual(expected, fmId.IsOwnerOf(FailoverUnitId(g)), L"FTId Test: " + tag);
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestFailoverManagerIdSuite, TestFailoverManagerId)

BOOST_AUTO_TEST_CASE(FMPartitionIdIsOwnedByFmm)
{
    Guid g;
    RunTest(*FailoverManagerId::Fmm, Reliability::Constants::FMServiceGuid, true, L"Fmm, Fmm");
    RunTest(*FailoverManagerId::Fmm, g, false, L"Fmm, other");

    RunTest(*FailoverManagerId::Fm, Reliability::Constants::FMServiceGuid, false, L"Fm, Fmm");
    RunTest(*FailoverManagerId::Fm, g, true, L"Fm, other");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
