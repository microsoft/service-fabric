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

/*
    This class tests that prior to processing a message that can insert into the LFUM the
    node activation state and upgrade state is checked
*/
class TestAddFailoverUnitStalenessChecks
{
protected:
    TestAddFailoverUnitStalenessChecks()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestAddFailoverUnitStalenessChecks()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTest & GetScenarioTest()
    {
        return scenarioTestHolder_->ScenarioTestObj;
    }

    bool Invoke(std::wstring const & ftShortName)
    {
        auto & test = GetScenarioTest();
        msgBody_ = test.ReadObject<ReplicaMessageBody>(ftShortName, L"000/411 [N/I IB U 1:1]");

        return test.RA.CheckIfAddFailoverUnitMessageCanBeProcessed(
            msgBody_,
            [this](ReplicaReplyMessageBody reply)
            {
                wasCalled_ = true;
                replyBody_ = reply;
            });
    }

    void InvokeAndVerifyReturnValue(std::wstring const & ftShortName, bool expectedRV, bool expectedCalled)
    {
        auto actual = Invoke(ftShortName);
        Verify::AreEqual(expectedRV, actual, L"return value");
        Verify::AreEqual(expectedCalled, wasCalled_, L"was called");

        if (wasCalled_)
        {
            Verify::AreEqual(wformatString(msgBody_.FailoverUnitDescription), wformatString(replyBody_.FailoverUnitDescription), L"FTDesc");
            Verify::AreEqual(wformatString(msgBody_.ReplicaDescription), wformatString(replyBody_.ReplicaDescription), L"ReplicaDesc");
        }
    }

    bool wasCalled_;
    ReplicaReplyMessageBody replyBody_;
    ReplicaMessageBody msgBody_;
    ScenarioTestHolderUPtr scenarioTestHolder_;
};

bool TestAddFailoverUnitStalenessChecks::TestSetup()
{
    wasCalled_ = false;
    scenarioTestHolder_ = ScenarioTestHolder::Create();
    return true;
}

bool TestAddFailoverUnitStalenessChecks::TestCleanup()
{
    scenarioTestHolder_.reset();
    return true;
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestAddFailoverUnitStalenessChecksSuite, TestAddFailoverUnitStalenessChecks)

BOOST_AUTO_TEST_CASE(NormalTestReturnsTrue)
{
    InvokeAndVerifyReturnValue(L"SP1", true, false);
}

BOOST_AUTO_TEST_CASE(UpgradingNodeReturnsFalseAndDoesNotSendMessage)
{
    GetScenarioTest().RA.StateObj.OnFabricCodeUpgradeStart();

    InvokeAndVerifyReturnValue(L"SP1", false, false);
}

BOOST_AUTO_TEST_CASE(DeactivatedNodeReturnsFalseAndSendsReply)
{
    GetScenarioTest().DeactivateNode(2);

    InvokeAndVerifyReturnValue(L"SP1", false, true);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
