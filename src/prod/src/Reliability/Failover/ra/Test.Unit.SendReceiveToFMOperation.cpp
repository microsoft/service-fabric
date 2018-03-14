// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Common;
using namespace std;

class TestSendReceiveToFMOperation
{
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestSendReceiveToFMOperationSuite,TestSendReceiveToFMOperation)

BOOST_AUTO_TEST_CASE(DelayCalculatorWithJitterTest)
{
    auto baseInterval = TimeSpan::FromSeconds(10);
    auto maxJitter = 0.3;
    auto maxJitterInterval = TimeSpan::FromSeconds(3);
        
    auto seed = 3;
    Random r(seed);
        
    SendReceiveToFMOperation::DelayCalculator delayCalculator(baseInterval, maxJitter);

    // Set the seed on the delay calculator
    delayCalculator.Test_Random.Reseed(seed);

    auto expectedDelay = baseInterval + TimeSpan::FromSeconds(maxJitterInterval.TotalSeconds() * r.NextDouble());
    auto actualDelay = delayCalculator.GetDelay();

    Verify::AreEqual(expectedDelay, actualDelay, L"Jitter: Delay");
}

BOOST_AUTO_TEST_CASE(DelayCalculatorWithIntervalLessThanOneDoesNotUseJitter)
{
    auto baseInterval = TimeSpan::FromSeconds(0.5);
    auto maxJitter = 0.3;

    auto seed = 3;
    Random r(seed);

    SendReceiveToFMOperation::DelayCalculator delayCalculator(baseInterval, maxJitter);

    // Set the seed on the delay calculator
    delayCalculator.Test_Random.Reseed(seed);

    auto expectedDelay = baseInterval;
    auto actualDelay = delayCalculator.GetDelay();

    Verify::AreEqual(expectedDelay, actualDelay, L"Jitter: Delay");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
