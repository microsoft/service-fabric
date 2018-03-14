// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Common;
using namespace std;
using namespace ReliabilityUnitTest;

class TestRetryableErrorStateThresholdCollection
{
protected:
    void Verify(
        FailoverConfig const & config,
        RetryableErrorStateThresholdCollection const & collection,
        RetryableErrorStateName::Enum state,
        int warning,
        int drop,
        int restart,
        int error)
    {
        auto threshold = collection.GetThreshold(state, config);

        Verify::AreEqual(warning, threshold.WarningThreshold, wformatString("{0}: Warning", state));
        Verify::AreEqual(drop, threshold.DropThreshold, wformatString("{0}: Drop", state));
        Verify::AreEqual(restart, threshold.RestartThreshold, wformatString("{0}: Restart", state));
        Verify::AreEqual(error, threshold.ErrorThreshold, wformatString("{0}: Error", state));
    }
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestRetryableErrorStateThresholdCollectionSuite,TestRetryableErrorStateThresholdCollection)

BOOST_AUTO_TEST_CASE(VerifyDefaultsAreCorrect)
{
    FailoverConfigContainer configWrapper;

    auto thresholdCollection = RetryableErrorStateThresholdCollection::CreateDefault();

    Verify(configWrapper.Config, thresholdCollection, RetryableErrorStateName::None, INT_MAX, INT_MAX, INT_MAX, INT_MAX);
    Verify(configWrapper.Config, thresholdCollection, RetryableErrorStateName::ReplicaOpen, 10, 40, INT_MAX, INT_MAX);
    Verify(configWrapper.Config, thresholdCollection, RetryableErrorStateName::ReplicaReopen, 10, 40320, INT_MAX, INT_MAX);
    Verify(configWrapper.Config, thresholdCollection, RetryableErrorStateName::FindServiceRegistrationAtOpen, INT_MAX, INT_MAX, INT_MAX, INT_MAX);
    Verify(configWrapper.Config, thresholdCollection, RetryableErrorStateName::FindServiceRegistrationAtReopen, INT_MAX, INT_MAX, INT_MAX, INT_MAX);
    Verify(configWrapper.Config, thresholdCollection, RetryableErrorStateName::FindServiceRegistrationAtDrop, 240, 1920, INT_MAX, INT_MAX);
    Verify(configWrapper.Config, thresholdCollection, RetryableErrorStateName::ReplicaChangeRoleAtCatchup, INT_MAX, INT_MAX, 10, 2);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
