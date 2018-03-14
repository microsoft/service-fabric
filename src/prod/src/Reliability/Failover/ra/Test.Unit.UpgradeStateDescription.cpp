// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade::ReliabilityUnitTest;

class TestUpgradeStateDescription
{
protected:
    void VerifyTimerState(
        Common::TimeSpan expected, 
        FailoverConfig & config,
        UpgradeStateDescription const & description, 
        wstring const & name)
    {
        Verify::IsTrue(description.IsTimer, wformatString("{0}: IsTimer", name));
        Verify::AreEqual(expected, description.GetRetryInterval(config), wformatString("{0}: IsTimer", name));
    }
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestUpgradeStateDescriptionSuite,TestUpgradeStateDescription)

BOOST_AUTO_TEST_CASE(TimerStateWithConstantRetryInterval)
{
    FailoverConfigContainer configContainer;
    TimeSpan timeout = TimeSpan::FromSeconds(5);
    UpgradeStateDescription desc(UpgradeStateName::Test1, timeout, UpgradeStateName::Test2, UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);
    VerifyTimerState(timeout, configContainer.Config, desc, L"Constant");
}

BOOST_AUTO_TEST_CASE(TimerStateWithVariableRetryInterva)
{
    TimeSpan timeout = TimeSpan::FromSeconds(5);
    FailoverConfigContainer configContainer;
    configContainer.Config.RAUpgradeProgressCheckInterval = timeout;
        
    UpgradeStateDescription desc(
        UpgradeStateName::Test1, 
        [](FailoverConfig const & cfg)
        {
            return cfg.RAUpgradeProgressCheckInterval;
        }, 
        UpgradeStateName::Test2, 
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);

    VerifyTimerState(timeout, configContainer.Config, desc, L"Constant");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
