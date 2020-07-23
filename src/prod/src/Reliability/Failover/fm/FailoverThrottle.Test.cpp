// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"



namespace FailoverManagerUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability;
    using namespace Reliability::FailoverManagerComponent;

    class TestThrottleConfig
    {
    };

    BOOST_FIXTURE_TEST_SUITE(FailoverThrottleTestSuite, TestThrottleConfig)

    BOOST_AUTO_TEST_CASE(TestThrottling)
    {
        FailoverConfig::GetConfig().MinSecondsBetweenQueueFullRejectMessages = TimeSpan::FromSeconds(3);
        FailoverThrottle throttle(FailoverConfig::GetConfig().MinSecondsBetweenQueueFullRejectMessagesEntry);

        //First call should return true as throttling hasn't yet started
        VERIFY_IS_TRUE(throttle.IsThrottling());
        //The very next call should throttle
        VERIFY_IS_FALSE(throttle.IsThrottling());
        //Wait for the time we passed on the constructor as the throttle time
        Sleep(3000);
        //After the throttle time passed, should return true
        VERIFY_IS_TRUE(throttle.IsThrottling());

        //Should only return true once, restart the timer and return false while the timer runs
        VERIFY_IS_FALSE(throttle.IsThrottling());
        Sleep(1000);
        VERIFY_IS_FALSE(throttle.IsThrottling());
        Sleep(2000);

        VERIFY_IS_TRUE(throttle.IsThrottling());
    }

    BOOST_AUTO_TEST_CASE(TestThrottlingDynamicConfigLoading)
    {
        FailoverConfig::GetConfig().MinSecondsBetweenQueueFullRejectMessages = TimeSpan::FromSeconds(3);
        FailoverThrottle throttle(FailoverConfig::GetConfig().MinSecondsBetweenQueueFullRejectMessagesEntry);

        //First call should return true as throttling hasn't yet started
        VERIFY_IS_TRUE(throttle.IsThrottling());
        //The very next call should throttle
        VERIFY_IS_FALSE(throttle.IsThrottling());

        //Change the wait time to check it changes dynamically
        FailoverConfig::GetConfig().MinSecondsBetweenQueueFullRejectMessages = TimeSpan::FromSeconds(50);
        //Wait for the time we passed on the constructor as the throttle time
        Sleep(3000);
        //We did not wait enough time after the config change, this should return false
        VERIFY_IS_FALSE(throttle.IsThrottling());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
