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

namespace
{
    const int DefaultValue = 4;
}

class TestFMMessageThrottle
{
protected:
    TestFMMessageThrottle() : throttle_(config_)
    {
        config_.MaxNumberOfReplicasInMessageToFMEntry.Test_SetValue(DefaultValue);
    }

    void Verify(StopwatchTime now, int expected)
    {
        auto actual = throttle_.GetCount(now);
        Verify::AreEqual(expected, actual);
    }

    void Update(StopwatchTime now, int count)
    {
        throttle_.Update(count, now);
    }

    FailoverConfig config_;
    Node::FMMessageThrottle throttle_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestFMMessageThrottleSuite, TestFMMessageThrottle)

BOOST_AUTO_TEST_CASE(InitialValueIsDefault)
{
    Verify(Stopwatch::Now(), DefaultValue);
}

BOOST_AUTO_TEST_CASE(UpdateDoesNotChangeValue)
{
    Update(Stopwatch::Now(), DefaultValue + 3);

    Verify(Stopwatch::Now(), DefaultValue);
}

BOOST_AUTO_TEST_CASE(ChangingConfigUpdatesValue)
{
    const int val = 301;

    config_.MaxNumberOfReplicasInMessageToFMEntry.Test_SetValue(val);

    Verify(Stopwatch::Now(), val);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
