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

class TestRetryState : public TestBase
{
protected:
    TestRetryState();

    void Start()
    {
        state_.Start();
    }

    void Finish()
    {
        state_.Finish();
    }

    void VerifyIsPending(bool expected)
    {
        Verify::AreEqual(expected, state_.IsPending, L"IsPending");
    }

    bool ShouldRetry(int seconds)
    {
        int64 unused = 0;
        auto actual = state_.ShouldRetry(GetTime(seconds), unused);

        if (!actual)
        {
            Verify::AreEqual(0, unused, L"Sequence number not 0 when should retry is false");
        }

        return actual;
    }

    int64 GetSequenceNumber(int seconds)
    {
        int64 sequenceNumber = 0;
        auto shouldRetry = state_.ShouldRetry(GetTime(seconds), sequenceNumber);
        Verify::IsTrue(shouldRetry, L"ShouldRetry returned false when looking for seq num");
        return sequenceNumber;
    }

    void VerifyShouldRetry(int seconds, bool expected)
    {
        auto actual = ShouldRetry(seconds);
        Verify::AreEqual(expected, actual, L"Verify Should retry failed");
    }

    void VerifyOnRetry(int64 sequenceNumber, int seconds, bool expected)
    {
        auto actual = OnRetry(sequenceNumber, seconds);
        Verify::AreEqual(expected, actual, L"OnRetry");
    }

    bool OnRetry(int64 sequenceNumber, int seconds)
    {
        return state_.OnRetry(sequenceNumber, GetTime(seconds));
    }

    StopwatchTime GetTime(int seconds) const
    {
        return start_ + TimeSpan::FromSeconds(seconds);
    }

    IRetryPolicySPtr policy_;
    TimeSpanConfigEntry config_;
    Common::StopwatchTime start_;
    RetryState state_;
};

TestRetryState::TestRetryState() :
policy_ (make_shared<LinearRetryPolicy>(config_)),
start_(Stopwatch::Now()),
state_(policy_)
{
    config_.Test_SetValue(TimeSpan::FromSeconds(Interval));
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestRetryStateSuite, TestRetryState)

BOOST_AUTO_TEST_CASE(InitiallyShouldRetryShouldBeFalse)
{
    VerifyIsPending(false);

    VerifyShouldRetry(0, false);
}

BOOST_AUTO_TEST_CASE(RetryShouldHappenAfterStart)
{
    Start();

    VerifyShouldRetry(0, true);

    VerifyIsPending(true);
}

BOOST_AUTO_TEST_CASE(RetryShouldNotHappenIfFinished)
{
    Start();

    Finish();

    VerifyShouldRetry(0, false);

    VerifyIsPending(false);
}

BOOST_AUTO_TEST_CASE(RetryShouldHappenIfStartAfterFinish)
{
    Start();

    Finish();

    Start();

    VerifyShouldRetry(0, true);

    VerifyIsPending(true);
}

BOOST_AUTO_TEST_CASE(RetryShouldHappenIfRestart)
{
    Start();

    Start();

    VerifyShouldRetry(0, true);

    VerifyIsPending(true);
}

BOOST_AUTO_TEST_CASE(RetryPolicyIsUsedForOnRetry)
{
    Start();

    auto seq = GetSequenceNumber(1);

    OnRetry(seq, 1);

    VerifyShouldRetry(2, false);
}

BOOST_AUTO_TEST_CASE(RetryPolicyIsUsedForOnRetry2)
{
    Start();

    auto seq = GetSequenceNumber(1);

    OnRetry(seq, 1);

    VerifyShouldRetry(1 + Interval, true);
}

BOOST_AUTO_TEST_CASE(StaleSequenceNumberIsIgnored)
{
    Start();

    auto seq = GetSequenceNumber(0);

    Start();

    OnRetry(seq, 1);

    VerifyShouldRetry(1, true);
}

BOOST_AUTO_TEST_CASE(RetryPolicyIsClearedAfterStart)
{
    Start();

    auto seq = GetSequenceNumber(2);

    OnRetry(seq, 2);

    Start();

    VerifyShouldRetry(3, true);
}

BOOST_AUTO_TEST_CASE(OnRetryReturnsTrueIfUpdated)
{
    Start();

    VerifyOnRetry(GetSequenceNumber(2), 2, true);
}

BOOST_AUTO_TEST_CASE(OnRetryReturnsFalseIfStale)
{
    Start();

    VerifyOnRetry(-1, 2, false);
}

BOOST_AUTO_TEST_CASE(OnRetryReturnsFalseIfNotRetrying)
{
    Start();

    auto seq = GetSequenceNumber(2);

    Finish();

    VerifyOnRetry(seq, 2, false);
}

BOOST_AUTO_TEST_CASE(OnRetryAssertsIfSequenceNumberIsGreater)
{
    Verify::Asserts([&]() { OnRetry(3, 0); }, L"OnRetryShouldAssert");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
