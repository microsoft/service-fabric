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
    const int DeletedDurationSeconds = 5;
    const int DroppedDurationSeconds = 10;
    wstring ShortName = L"SP1";
}

class TestStateMachine_StateCleanup : public StateMachineTestBase
{
protected:
    TestStateMachine_StateCleanup()
    {
        Test.UTContext.Clock.SetManualMode();
        start_ = Test.UTContext.Clock.DateTimeNow();
        SetConfig(DeletedDurationSeconds, DroppedDurationSeconds);
    }

    void SetConfig(int deletedDuration, int droppedDuration)
    {
        Test.UTContext.Config.DeletedFailoverUnitTombstoneDurationEntry.Test_SetValue(TimeSpan::FromSeconds(deletedDuration));
        Test.UTContext.Config.DroppedFailoverUnitTombstoneDurationEntry.Test_SetValue(TimeSpan::FromSeconds(droppedDuration));
        Test.UTContext.Config.MinimumIntervalBetweenPeriodicStateCleanupEntry.Test_SetValue(TimeSpan::FromSeconds(0));
    }

    void AddFT(bool isDeleted, bool isCleanupPending)
    {
        wstring flags = L"";
        if (isDeleted)
        {
            flags += L"L";
        }

        if (isCleanupPending)
        {
            flags += L"P";
        }

        state_ = wformatString("C None 000/000/411 1:1 {0}", flags);
        Test.AddFT(ShortName, state_);

        Test.GetFT(ShortName).Test_SetLastUpdatedTime(start_);
    }

    void AdvanceTime(int seconds)
    {
        Test.UTContext.Clock.AdvanceTimeBySeconds(seconds);
    }

    void VerifyNoOp()
    {
        Test.ValidateFT(ShortName, state_);
    }

    void VerifyDeleted()
    {
        Test.ValidateFTNotPresent(ShortName);
    }

    void Execute()
    {
        RA.StateCleanupWorkManager.Request(L"a");
        Drain();
    }

    wstring state_;
    Common::DateTime start_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_StateCleanupSuite, TestStateMachine_StateCleanup)

BOOST_AUTO_TEST_CASE(DeletedFTBeforeCleanupIsNotExecuted)
{
    AddFT(true, true);

    Execute();

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(DeletedFTAfterCleanupIsDeleted)
{
    AddFT(true, true);

    AdvanceTime(DeletedDurationSeconds + 1);

    Execute();

    VerifyDeleted();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
