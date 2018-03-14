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

class TestRetryableErrorState
{
protected:
    TestRetryableErrorState() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestRetryableErrorState() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void TestReset()
    {
        state = RetryableErrorState();
    }

    void SetState(RetryableErrorStateName::Enum errorState, int currentFailureCount, int warningThreshold, int dropThreshold, int restartThreshold, int errorThreshold)
    {
        state.Test_Set(errorState, currentFailureCount);
        state.Test_SetThresholds(errorState, warningThreshold, dropThreshold, restartThreshold, errorThreshold);
    }

    void Verify(RetryableErrorStateName::Enum errorState, int currentFailureCount)
    {
        Verify::AreEqual(state.CurrentState, errorState);
        Verify::AreEqual(state.CurrentFailureCount, currentFailureCount);
    }

    void TestEnterState(RetryableErrorStateName::Enum errorState)
    {
        state.EnterState(errorState);
        Verify(errorState, 0);
    }

    void TestOnFailure()
    {
        auto & config = FailoverConfig::GetConfig();
        RetryableErrorAction::Enum actualAction;
        actualAction = state.OnFailure(config);
        Verify::AreEqual(RetryableErrorAction::None == actualAction, true);
        actualAction = state.OnFailure(config);
        Verify::AreEqual(RetryableErrorAction::ReportHealthWarning == actualAction, true);
        actualAction = state.OnFailure(config);
        Verify::AreEqual(RetryableErrorAction::Drop == actualAction, true);
        actualAction = state.OnFailure(config);
        Verify::AreEqual(RetryableErrorAction::Restart == actualAction, true);
        actualAction = state.OnFailure(config);
        Verify::AreEqual(RetryableErrorAction::ReportHealthError == actualAction, true);
    }

    void TestOnSuccessAndTransitionTo(RetryableErrorAction::Enum expectedAction)
    {
        auto actualAction = state.OnSuccessAndTransitionTo(RetryableErrorStateName::Enum::ReplicaOpen, FailoverConfig::GetConfig());
        Verify::AreEqual(expectedAction == actualAction, true);
    }

    void TestIsLastRetryBeforeDrop(bool expected)
    {
        auto actual = state.IsLastRetryBeforeDrop(FailoverConfig::GetConfig());
        Verify::AreEqual(expected, actual);
    }

    void TestGetTraceState(string expected)
    {
        auto actual = state.GetTraceState(FailoverConfig::GetConfig());
        Verify::AreEqual(expected, actual);
    }

    RetryableErrorState state;
};

bool TestRetryableErrorState::TestSetup()
{
    state = RetryableErrorState();
    return true;
}

bool TestRetryableErrorState::TestCleanup()
{
    state.Reset();
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestRetryableErrorStateSuite, TestRetryableErrorState)

BOOST_AUTO_TEST_CASE(EnterStateTest)
{
    TestEnterState(RetryableErrorStateName::ReplicaOpen);
    TestEnterState(RetryableErrorStateName::ReplicaReopen);
    TestEnterState(RetryableErrorStateName::FindServiceRegistrationAtDrop);
    TestEnterState(RetryableErrorStateName::FindServiceRegistrationAtOpen);
    TestEnterState(RetryableErrorStateName::FindServiceRegistrationAtReopen);
    TestEnterState(RetryableErrorStateName::ReplicaChangeRoleAtCatchup);
}

BOOST_AUTO_TEST_CASE(OnFailureTest)
{
    TestReset();
    SetState(RetryableErrorStateName::ReplicaOpen, -1, 1, 2, 3, 4);
    TestOnFailure();
}

BOOST_AUTO_TEST_CASE(OnSuccessAndTransitionToTest)
{
    TestReset();
    int warningThreshold = 1;

    int currentFailureCount = warningThreshold;

    SetState(RetryableErrorStateName::ReplicaOpen, currentFailureCount, warningThreshold, 2, 3, 4);
    TestOnSuccessAndTransitionTo(RetryableErrorAction::ClearHealthReport);

    currentFailureCount = warningThreshold - 1;

    SetState(RetryableErrorStateName::ReplicaOpen, currentFailureCount, warningThreshold, 2, 3, 4);
    TestOnSuccessAndTransitionTo(RetryableErrorAction::None);
}

BOOST_AUTO_TEST_CASE(IsLastRetryBeforeDropTest)
{
    TestReset();
    int dropThreshold = 2;

    int currentFailureCount = dropThreshold - 2;

    SetState(RetryableErrorStateName::ReplicaOpen, currentFailureCount, 1, dropThreshold, 3, 4);
    TestIsLastRetryBeforeDrop(false);

    currentFailureCount = dropThreshold - 1;

    SetState(RetryableErrorStateName::ReplicaOpen, currentFailureCount, 1, dropThreshold, 3, 4);
    TestIsLastRetryBeforeDrop(true);
}

BOOST_AUTO_TEST_CASE(GetTraceStateTest)
{
    TestReset();
    int currentFailureCount = 0;
    int dropThreshold = 2;
    SetState(RetryableErrorStateName::None, currentFailureCount, 1, dropThreshold, 3, 4);
    TestGetTraceState("");
    SetState(RetryableErrorStateName::ReplicaOpen, currentFailureCount, 1, dropThreshold, 3, 4);
    TestGetTraceState(formatString("[{0}/{1}]", currentFailureCount, dropThreshold));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
