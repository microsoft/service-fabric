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
    const int Interval = 5;
}

class TestLinearRetryPolicy : public TestBase
{
protected:
    TestLinearRetryPolicy();

    void Clear()
    {
        policy_.Clear();
    }

    void OnRetry(int seconds)
    {
        policy_.OnRetry(GetTime(seconds));
    }

    bool ShouldRetry(int seconds)
    {
        return policy_.ShouldRetry(GetTime(seconds));
    }

    void VerifyShouldRetry(int seconds, bool expected)
    {
        auto actual = ShouldRetry(seconds);
        Verify::AreEqual(expected, actual, L"should retry");
    }

    StopwatchTime GetTime(int seconds) const
    {
        return start_ + TimeSpan::FromSeconds(seconds);
    }

    LinearRetryPolicy policy_;
    TimeSpanConfigEntry config_;
    Common::StopwatchTime start_;
};

TestLinearRetryPolicy::TestLinearRetryPolicy() : start_(Stopwatch::Now()), policy_(config_)
{
    config_.Test_SetValue(TimeSpan::FromSeconds(Interval));
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestLinearRetryPolicySuite, TestLinearRetryPolicy)

BOOST_AUTO_TEST_CASE(FirstRetryWithTimeLessThanInterval)
{
    VerifyShouldRetry(1, true);
}

BOOST_AUTO_TEST_CASE(FirstRetryWithTimeEqualToInterval)
{
    VerifyShouldRetry(5, true);
}

BOOST_AUTO_TEST_CASE(FirstRetryWithTimeGreaterThanInterval)
{
    VerifyShouldRetry(6, true);
}

BOOST_AUTO_TEST_CASE(OnRetryShouldUpdateTheTime)
{
    OnRetry(4);

    VerifyShouldRetry(8, false);
}

BOOST_AUTO_TEST_CASE(OnRetryWithGreaterThanInterval)
{
    OnRetry(4);

    VerifyShouldRetry(9, true);
}

BOOST_AUTO_TEST_CASE(ClearShouldResetTheState)
{
    OnRetry(4);

    Clear();

    VerifyShouldRetry(5, true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
