// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestStateMachine_GetLSN
{
protected:
    TestStateMachine_GetLSN() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_GetLSN() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void GetLsnClosedTestCase(wstring const & initial, wstring const & finalExpected, wstring const & replyExpected);
    void AddFT(wstring const & state)
    {
        GetScenarioTest().AddFT(L"SP1", state);
    }

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    ScenarioTest & RunTest(wstring const & initial);

    ScenarioTest & RunTestAndVerify(
        wstring const & initial,
        function<void(ScenarioTest &)> func,
        wstring const & expected);

    ScenarioTestHolderUPtr holder_;
    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_GetLSN::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();
    scenarioTest_ = ScenarioTest::Create();
    return true;
}

bool TestStateMachine_GetLSN::TestCleanup()
{
    holder_.reset();
    return scenarioTest_->Cleanup();
}

ScenarioTest & TestStateMachine_GetLSN::RunTest(wstring const & initial)
{
    auto & scenarioTest = holder_->Recreate();

    scenarioTest.AddFT(L"SP1", initial);

    return scenarioTest;
}

ScenarioTest & TestStateMachine_GetLSN::RunTestAndVerify(wstring const & initial, function<void(ScenarioTest&)> func, wstring const & expected)
{
    auto & scenarioTest = RunTest(initial);

    func(scenarioTest);

    scenarioTest.ValidateFT(L"SP1", expected);

    return scenarioTest;
}

void TestStateMachine_GetLSN::GetLsnClosedTestCase(wstring const & initial, wstring const & finalExpected, wstring const & replyExpected)
{
    auto ftShortName = L"SP1";

    if (!initial.empty())
    {
        scenarioTest_->AddFT(ftShortName, initial);
    }

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(2, ftShortName, L"411/522 [S/S IB U 1:1]");

    if (finalExpected.empty())
    {
        scenarioTest_->ValidateFTIsNull(ftShortName);
    }
    else
    {
        scenarioTest_->ValidateFT(ftShortName, finalExpected);
    }

    scenarioTest_->ValidateRAMessage<MessageType::GetLSNReply>(2, ftShortName, replyExpected);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_GetLSNSuite, TestStateMachine_GetLSN)

BOOST_AUTO_TEST_CASE(GetLsnCreateReplicaRace)
{
    // Regression test for #1489084
    //1. Primary RA sends GetLSN to secondary
    //2. Secondary sends GetStatus to RAP
    //3. RAP completes but reply is delayed
    //4. Primary then retries and GEtLSN completes
    //5. Primary sends CreateReplica
    //6. Secondary completes create and resets SenderNode
    //7. GetLSNReply at step 3 arrives but asserts at RA saying invalid node is the target

    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 {411:0} 2:1 CM [N/N/S SB U N F 2:1] [N/N/P RD U N F 1:1]");
    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(1, L"SP1", L"411/422 [N/S RD U 2:1]");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorGetStatusReply>(L"SP1", L"000/422 [N/S RD U 2:1 1 1] Success -");

    scenarioTest_->ResetAll();
    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(1, L"SP1", L"411/422 [S/S IB U 2:1]");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaOpenReply>(L"SP1", L"000/422 [N/I RD U 2:1] Success -");

    // This should not assert
    scenarioTest_->ResetAll();
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorGetStatusReply>(L"SP1", L"000/422 [N/S RD U 2:1 1 1] Success -");

    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/422 {411:0} 2:1 CM [S/N/I IB U N F 2:1] [P/N/P RD U N F 1:1]");
    scenarioTest_->ValidateNoMessages();
}

BOOST_AUTO_TEST_CASE(PartialDeactivationEpochSupport_NonDeactivationEpochReplicaIsMostAdvanced)
{
    scenarioTest_->AddFT(L"SP1", L"O Phase1_GetLSN 411/000/522 {411:0} 1:1 CM [S/N/P RD U N F 1:1 2 2 411:0] [P/N/S SB U N F 2:1 3 3 411:0] [S/N/S RD U RA F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, L"SP1", L"411/522 {000:-1} [S/S RD U 3:1 4 4] Success");

    scenarioTest_->ValidateFMMessage<MessageType::ChangeConfiguration>(L"SP1", L"411/522 [N/P RD U 1:1 -1 -1] [N/S SB U 2:1 -1 -1] [N/S RD U 3:1 4 4]");
}

BOOST_AUTO_TEST_CASE(PartialDeactivationEpochSupport_DeactivationEpochReplicaIsMostAdvanced)
{
    scenarioTest_->AddFT(L"SP1", L"O Phase1_GetLSN 411/000/522 {411:0} 1:1 CM [S/N/P RD U N F 1:1 1 6 411:0] [P/N/S SB U N F 2:1 3 3 411:0] [S/N/S RD U RA F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, L"SP1", L"411/522 {000:-1} [S/S RD U 3:1 4 4] Success");

    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 411/000/522 {411:0} 1:1 CMN [S/N/P RD U N F 1:1 1 6 411:0] [P/N/S SB U N F 2:1 3 3 411:0] [S/N/S RD U N F 3:1 4 4 000:-1]");
}

BOOST_AUTO_TEST_CASE(GetLsn_NullReplica_ReturnsNotFound)
{
    GetLsnClosedTestCase(L"", L"", L"411/522 {000:-1} [S/S IB U 1:1] NotFound");
}

BOOST_AUTO_TEST_CASE(GetLsn_DifferentReplica_ReturnsNotFound)
{
    GetLsnClosedTestCase(L"C None 000/000/522 3:3 -", L"C None 000/000/522 3:3 -", L"411/522 {000:-1} [S/S IB U 1:1] NotFound");
}

BOOST_AUTO_TEST_CASE(GetLsn_SameReplica_ReturnsDropped)
{
    GetLsnClosedTestCase(L"C None 000/000/522 1:1 -", L"C None 000/000/522 1:1 -", L"000/522 {000:-1} [N/N DD U 1:1 n] Success");
}

BOOST_AUTO_TEST_CASE(GetLsn_SameReplica_DifferentInstance_ReturnsDropped)
{
    // It is important that the same instance id is returned
    // GetLSN Reply cannot handle a different instance id
    GetLsnClosedTestCase(L"C None 000/000/522 1:2 -", L"C None 000/000/522 1:2 -", L"000/522 {000:-1} [N/N DD U 1:1 n] Success");
}

BOOST_AUTO_TEST_CASE(GetLsn_DeactivationInfoInvalid_ReturnsLsn)
{
    auto & test = holder_->Recreate();

    test.AddFT(L"SP1", L"O None 000/000/411 {000:-1} 1:1 CM [N/N/S RD U N F 1:1]");

    test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(2, L"SP1", L"411/522 [S/S RD U 1:1]");

    test.ValidateNoRAMessage();
    test.ValidateRAPMessage<MessageType::ReplicatorUpdateEpochAndGetStatus>(L"SP1");
}

BOOST_AUTO_TEST_CASE(GetLsn_ResetsSenderNode)
{
    auto & test = holder_->Recreate();
    test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/P RD U N F 3:1]");
    
    test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(3, L"SP1", L"411/422 [S/S RD U 1:1]");
    Verify::AreEqual(test.GetFT(L"SP1").SenderNode, Reader::ReadHelper<ReplicaAndNodeId>(L"3:1").CreateNodeInstance(), L"Sender node");

    test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(L"SP1", L"411/422 [S/S RD U 1:1 100 500] Success -");
    Verify::AreEqual(test.GetFT(L"SP1").SenderNode, ReconfigurationAgent::InvalidNode, L"Sender node");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
