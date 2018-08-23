// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::ReplicaUp;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

namespace JobItemType
{
    enum Enum
    {
        Dropped = 0,
        ReadOnly = 1,
        ReadWrite = 2,
    };
}

#pragma region ReplicaUp
class TestStateMachine_ReplicaUp
{
protected:
    TestStateMachine_ReplicaUp() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_ReplicaUp() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void DroppedFTTestHelper(wstring const & ft, wstring const & replicaUpReply);

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_ReplicaUp::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();
    return true;
}

bool TestStateMachine_ReplicaUp::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_ReplicaUp::DroppedFTTestHelper(wstring const & ft, wstring const & replicaUpReply)
{
    // FT was closed and sent by RA in dropped list
    GetScenarioTest().AddFT(ft, L"C None 000/000/411 1:1 -");

    // FM can send in either
    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(replicaUpReply);

    GetScenarioTest().ValidateFT(ft, L"C None 000/000/411 1:1 L");
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReplicaUpSuite, TestStateMachine_ReplicaUp)

BOOST_AUTO_TEST_CASE(FTClearReplicaUpReplyPendingOnDeleteReplica)
{
    // On receiving the DeleteReplica message with the same instance id (restarted replica),
    // the ReplicaUpReplyPending flag should get cleared since the FM knows about this instance
    // of the replica, defect#: 1226155

    GetScenarioTest().AddFT(L"SP1", L"O None 000/000/411 1:2 CMK [N/N/P SB U N F 1:2]");

    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SP1", L"000/411 [N/P RD U 1:2]");

    GetScenarioTest().ValidateFT(L"SP1", L"O None 000/000/411 1:2 CMHlL [N/N/P ID U N F 1:2]");
}

BOOST_AUTO_TEST_CASE(FTClearReplicaUpReplyPendingOnDoReconfiguration)
{
    // On receiving the DoReconfiguration message with the same instance id (restarted replica),
    // the ReplicaUpReplyPending flag should get cleared since the FM knows about this instance
    // of the replica

    GetScenarioTest().AddFT(L"SP1", L"O None 000/000/411 1:2 CMK [N/N/P SB U N F 1:2]");

    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/P RD U 1:2]");

    GetScenarioTest().ValidateFT(L"SP1", L"O Phase1_GetLSN 411/000/422 {411:0} 1:2 CMN [P/N/P SB U RAP F 1:2]");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Health

class TestStateMachine_Health
{
protected:
    TestStateMachine_Health() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_Health() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    // the message has to have replica id = 1 and instance id = 3
    template<MessageType::Enum IncomingMessage>
    void ReplicaCreateReportsHealthTestHelper(wstring const & ftShortName, wstring const & message)
    {
        test_->ProcessFMMessageAndDrain<IncomingMessage>(ftShortName, message);

        test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::OK, ftShortName, 1, 3);

        test_->StateItemHelpers.ResetAll();
    }

    void ReplicaCloseReportsHealthTestHelper(wstring const & ftShortName);

    ScenarioTestUPtr test_;
};

bool TestStateMachine_Health::TestSetup()
{
    test_ = ScenarioTest::Create();
    return true;
}

bool TestStateMachine_Health::TestCleanup()
{
    return test_->Cleanup();
}
void TestStateMachine_Health::ReplicaCloseReportsHealthTestHelper(wstring const & ftShortName)
{
    test_->AddFT(ftShortName, L"O None 000/000/411 1:1 CMHlL [N/N/N ID U N F 1:1]");

    test_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(ftShortName, L"000/411 [N/N RD U 1:1] Success D");

    test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Close, ftShortName, 1, 1);
    test_->StateItemHelpers.ResetAll();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_HealthSuite, TestStateMachine_Health)

BOOST_AUTO_TEST_CASE(ReplicaCreateReportsHealth)
{
    ReplicaCreateReportsHealthTestHelper<MessageType::AddInstance>(L"SL1", L"000/411 [N/N IB U 1:3]");

    ReplicaCreateReportsHealthTestHelper<MessageType::AddPrimary>(L"SP1", L"000/411 [P/P IB U 1:3]");

    ReplicaCreateReportsHealthTestHelper<MessageType::AddPrimary>(L"SV1", L"000/411 [P/P IB U 1:3]");
}

BOOST_AUTO_TEST_CASE(ReplicaRestartReportsHealth)
{
    test_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    test_->ProcessRAPMessageAndDrain<MessageType::ReportFault>(L"SP1", L"000/411 [N/P RD U 1:1] transient");

    test_->ResetAll();
    test_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

    test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Restart, L"SP1", 1, 2);
}

BOOST_AUTO_TEST_CASE(ReplicaCloseReportsHealth)
{
    ReplicaCloseReportsHealthTestHelper(L"SL1");
    ReplicaCloseReportsHealthTestHelper(L"SV1");
    ReplicaCloseReportsHealthTestHelper(L"SP1");
}

BOOST_AUTO_TEST_CASE(ReplicaRestartOnServiceTypeRegisteredReportsHealth)
{
    test_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    test_->ProcessAppHostClosedAndDrain(L"SP1");
    test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Close, L"SP1", 1, 1);
    test_->ResetAll();

    // Register service type
    test_->RA.ProcessServiceTypeRegistered(HostingStub::CreateRegistrationHelper(Default::GetInstance().SP1_SP1_STContext));
    test_->DrainJobQueues();

    test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Restart, L"SP1", 1, 2);
}

BOOST_AUTO_TEST_CASE(ReplicaGoingDownReportsHealth)
{
    test_->AddFT(L"SP1", L"O None 000/000/411 1:1 ACMHc [N/N/P RD U N F 1:1]");

    test_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/N RD U 1:1] Success D");
    test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Close, L"SP1", 1, 1);
}

BOOST_AUTO_TEST_CASE(ReplicaDeleteDoesNotReportHealth)
{
    test_->AddFT(L"SP1", L"C None 000/000/411 1:1 -");

    test_->ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SP1", L"000/411 [N/N DD D 1:1]");
    test_->StateItemHelpers.HealthHelper.ValidateNoReplicaHealthEvent();
}

BOOST_AUTO_TEST_CASE(ReplicaRecreateSecondaryDoesNotReportHealth)
{
    test_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/S SB U N F 1:1] [N/N/P RD U N F 2:1]");

    test_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, L"SP1", L"411/411 [S/S IB U 1:1]");

    test_->DumpFT(L"SP1");
    Verify::IsTrueLogOnError(test_->GetFT(L"SP1").LocalReplicaOpenPending.IsSet, L"OpenPending");
    test_->StateItemHelpers.HealthHelper.ValidateNoReplicaHealthEvent();
}

BOOST_AUTO_TEST_CASE(ReplicaRecreateIdleDoesNotReportHealth)
{
    test_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/I SB U N F 1:1]");

    test_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, L"SP1", L"000/411 [N/I IB U 1:1]");

    test_->DumpFT(L"SP1");
    Verify::IsTrueLogOnError(test_->GetFT(L"SP1").LocalReplicaOpenPending.IsSet, L"OpenPending");
    test_->StateItemHelpers.HealthHelper.ValidateNoReplicaHealthEvent();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Upgrade

class TestStateMachine_Upgrade
{
protected:
    TestStateMachine_Upgrade() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_Upgrade() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void UpgradeTargetingNoFTsTestHelper(wstring const & upgradeMessage);

    void UpgradeWithReplicaRestartTestHelper(wstring const & upgradeMessage);

    void UpgradeWithLowerInstanceDoesNotAffectReplicas(wstring const & upgradeMessage);

#pragma region RA Ready tests

    template<MessageType::Enum TMsg>
    void MessageShouldBeDroppedIfRAIsNotReadyTestHelper(wstring const & message, bool isFM, bool isFmm)
    {
        scenarioTest_->SetNodeUpAckProcessed(isFM, isFmm);
        scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

        scenarioTest_->ProcessFMMessageAndDrain<TMsg>(message);
        VERIFY_IS_TRUE(scenarioTest_->RA.UpgradeMessageProcessorObj.Test_UpgradeMap.empty());
    }

    void UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(bool isFM, bool isFmm);

    void CancelShouldBeDroppedIfRAIsNotReadyTestHelper(bool isFM, bool isFmm);

#pragma endregion

    void FireCloseCompletionCheckTimer(uint64 instanceId);
    void FireDropCompletionCheckTimer(uint64 instanceId);
    void FireReplicaDownCompletionCheckTimer(uint64 instanceId);
    void ValidateUpgradeIsComplete(uint64 instanceId);
    UpgradeMessageProcessor::UpgradeElement const * GetElement(uint64 instanceId);

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_Upgrade::TestSetup()
{
    // Setup all async operations on hosting to complete immediately
    scenarioTest_ = ScenarioTest::Create();

    scenarioTest_->HostingHelper.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();

    // Add additional FT contexts
    // SL2x, SV2x, SP2x with the same info as SL2, SV2 and SP2 except that they are all in service package SP1
    // This is to test the scenario where hosting returns a RuntimeId for restarting replicas
    SingleFTReadContext sl2x = Default::GetInstance().SL2_FTContext;
    sl2x.ShortName = L"SL2x";
    sl2x.STInfo = Default::GetInstance().SL1_SP1_STContext;
    sl2x.STInfo.RuntimeId = L"Runtime2";

    SingleFTReadContext sv2x = Default::GetInstance().SV2_FTContext;
    sv2x.ShortName = L"SV2x";
    sv2x.STInfo = Default::GetInstance().SV1_SP1_STContext;
    sv2x.STInfo.RuntimeId = L"Runtime2";

    SingleFTReadContext sp2x = Default::GetInstance().SP2_FTContext;
    sp2x.ShortName = L"SP2x";
    sp2x.STInfo = Default::GetInstance().SP1_SP1_STContext;
    sp2x.STInfo.RuntimeId = L"Runtime2";

    scenarioTest_->AddFTContext(L"SL2x", sl2x);
    scenarioTest_->AddFTContext(L"SV2x", sv2x);
    scenarioTest_->AddFTContext(L"SP2x", sp2x);

    return true;
}

bool TestStateMachine_Upgrade::TestCleanup()
{
    return scenarioTest_->Cleanup();
}

void TestStateMachine_Upgrade::FireCloseCompletionCheckTimer(uint64 instanceId)
{
    scenarioTest_->FireApplicationUpgradeCloseCompletionCheckTimer(instanceId);
}

void TestStateMachine_Upgrade::FireDropCompletionCheckTimer(uint64 instanceId)
{
    scenarioTest_->FireApplicationUpgradeDropCompletionCheckTimer(instanceId);
}

void TestStateMachine_Upgrade::FireReplicaDownCompletionCheckTimer(uint64 instanceId)
{
    scenarioTest_->FireApplicationUpgradeReplicaDownCompletionCheckTimer(instanceId);
}

UpgradeMessageProcessor::UpgradeElement const * TestStateMachine_Upgrade::GetElement(uint64 instanceId)
{
    return scenarioTest_->GetUpgradeElementForApplicationUpgrade(instanceId);
}

void TestStateMachine_Upgrade::ValidateUpgradeIsComplete(uint64 instanceId)
{
    scenarioTest_->ValidateApplicationUpgradeIsCompleted(instanceId);
}

void TestStateMachine_Upgrade::UpgradeTargetingNoFTsTestHelper(wstring const & upgradeMessage)
{
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(upgradeMessage);
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 | ");
    ValidateUpgradeIsComplete(2);
}

void TestStateMachine_Upgrade::UpgradeWithReplicaRestartTestHelper(wstring const & upgradeMessage)
{
    // Affect only the runtimes in the first service package
    // The scenario is:
    //   6 replicas. Each of the same service package running at v1
    //   3 are running on Runtime1 and other three on Runtime2
    // Upgrade from v1 -> v2 and only replicas of Runtime1 are affected
    // Validate that version on all 6 is updated to v2
    // Validate that only Runtime1 replicas are closed/restarted
    // Validate the close completion check is retried
    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SL1_SP1_STContext);
    scenarioTest_->AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    scenarioTest_->AddFT(L"SL2x", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    scenarioTest_->AddFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->AddFT(L"SV2x", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->AddFT(L"SP2x", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(upgradeMessage);

    scenarioTest_->ResetAll();

    // This should cause it to resend the messages
    FireCloseCompletionCheckTimer(2);
    scenarioTest_->RequestWorkAndDrain(scenarioTest_->StateItemHelpers.ReplicaCloseRetryHelper);

    scenarioTest_->ResetAll();

    // Send ReplicaCloseReply
    // Replica Down is sent for all non services
    scenarioTest_->ProcessRAPMessage<MessageType::ReplicaCloseReply>(L"SL1", L"000/411 [N/N RD U 1:1] Success -");
    scenarioTest_->ProcessRAPMessage<MessageType::ReplicaCloseReply>(L"SV1", L"000/411 [N/N RD U 1:1] Success -");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/N RD U 1:1] Success -");
    scenarioTest_->ValidateFMMessage<MessageType::ReplicaUp>();
    scenarioTest_->ResetAll();

    // Run the timer again and it should complete
    FireCloseCompletionCheckTimer(2);

    // Wait for replica down
    FireReplicaDownCompletionCheckTimer(2);
    scenarioTest_->ResetAll();

    // Process Replica Down Reply for two FT
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SL1 000/000/411 [N/N/N DD D 1:1]} {SV1 000/000/411 [N/N/P DD D 1:1]} |");

    scenarioTest_->DumpFT(L"SL1");

    // Fire the timer again - upgrade should be stuck
    FireReplicaDownCompletionCheckTimer(2);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");

    // Fire the timer again
    scenarioTest_->ResetAll();
    FireReplicaDownCompletionCheckTimer(2);

    // Upgrade should be complete
    ValidateUpgradeIsComplete(2);

    // Stateless and Volatile are closed
    // Persisted is restarted
    scenarioTest_->ValidateFT(L"SL1", L"C None 000/000/411 1:1 LP");
    scenarioTest_->ValidateFT(L"SL2x", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1 (2.0:2.0:2)]");
    scenarioTest_->ValidateFT(L"SV1", L"C None 000/000/411 1:1 LP");
    scenarioTest_->ValidateFT(L"SV2x", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1 (2.0:2.0:2)]");
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2 (2.0:2.0:2)]");
    scenarioTest_->ValidateFT(L"SP2x", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1 (2.0:2.0:2)]");

    // The upgrade reply should contain the persisted service
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 | ");
}

void TestStateMachine_Upgrade::UpgradeWithLowerInstanceDoesNotAffectReplicas(wstring const & upgradeMessage)
{
    // Validate that replicas at a higher instance are not affected
    // The scenario is:
    //   3 replicas (SL/SV/SP) each running v 3.0
    // Upgrade to v2 and replicas are affected
    // Validate that no change happens to the FTs
    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SL1_SP1_STContext);
    scenarioTest_->AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1 (3.0:3.0:3)]");
    scenarioTest_->AddFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1 (3.0:3.0:3)]");
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1 (3.0:3.0:3)]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(upgradeMessage);

    // Validate reply is sent
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 | ");

    // Validate no change to replicas
    scenarioTest_->ValidateFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1 (3.0:3.0:3)]");
    scenarioTest_->ValidateFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1 (3.0:3.0:3)]");
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1 (3.0:3.0:3)]");
}

void TestStateMachine_Upgrade::UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(bool isFM, bool isFmm)
{
    MessageShouldBeDroppedIfRAIsNotReadyTestHelper<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0]", isFM, isFmm);
}

void TestStateMachine_Upgrade::CancelShouldBeDroppedIfRAIsNotReadyTestHelper(bool isFM, bool isFmm)
{
    MessageShouldBeDroppedIfRAIsNotReadyTestHelper<MessageType::CancelApplicationUpgradeRequest>(L"3 Rolling false", isFM, isFmm);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_UpgradeSuite, TestStateMachine_Upgrade)

BOOST_AUTO_TEST_CASE(UpgradeWaitsForReplicasThatWereAlreadyClosing)
{
    // Test a scenario where an FT is being closed (because of say node deactivation and an upgrade request comes)
    // The upgrade must wait for the close to complete
    // Setup:
    // SP1 closing
    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP1_SP1_STContext);
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] | ");
    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

    scenarioTest_->ResetAll();

    FireCloseCompletionCheckTimer(2);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");
    scenarioTest_->ResetAll();

    FireReplicaDownCompletionCheckTimer(2);

    // the upgrade should be complete
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 |");
    ValidateUpgradeIsComplete(2);

    // Validate FTs
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2 (2.0:2.0:2)]");
}

BOOST_AUTO_TEST_CASE(UpgradeWithDeletedServiceTypes1)
{
    // Test a scenario where a service package is removed
    // Setup:
    // Two FTs, one persisted from SP1 and another from SP2
    // Upgrade message in which SP1 is upgraded and SP2 has been removed
    // The upgrade should wait for SP1 to be closed (and then reopened at the end)
    // and for SP2 to be Dropped from the node
    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP1_SP1_STContext);
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->AddFT(L"SL2", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    scenarioTest_->AddFT(L"SV2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->AddFT(L"SP2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] | [SP2 SP2_ST] [SP2 SL2_ST] [SP2 SV2_ST]");

    // The upgrade should be closing SP1
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 AHcCM [N/N/P RD U N F 1:1 (2.0:2.0:2)]");
    scenarioTest_->ValidateFT(L"SP2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->StateItemHelpers.ReplicaCloseRetryHelper.ValidateTimerIsSet();

    scenarioTest_->ResetAll();

    // Validate the retry once
    FireCloseCompletionCheckTimer(2);
    scenarioTest_->RequestWorkAndDrain(scenarioTest_->StateItemHelpers.ReplicaCloseRetryHelper);

    // Now process DeleteReplica From FM
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::RemoveInstance>(L"SL2", L"000/411 [N/N RD U 1:1]");
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV2", L"000/411 [N/P RD U 1:1]");
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SP2", L"000/411 [N/P RD U 1:1]");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SL2", L"000/411 [N/N RD U 1:1] Success D");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SV2", L"000/411 [N/P RD U 1:1] Success D");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP2", L"000/411 [N/P RD U 1:1] Success D");

    // Also let the close complete for SP1
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P SB D 1:1] Success -");

    scenarioTest_->ResetAll();

    // Perform close check again
    // This time we should succeed because the replica has been closed
    FireCloseCompletionCheckTimer(2);

    // Fire the replica down completion check
    FireReplicaDownCompletionCheckTimer(2);

    // The upgrade should still be waiting for replica down for SP1
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");

    scenarioTest_->ResetAll();
    FireReplicaDownCompletionCheckTimer(2);

    // the upgrade should be complete
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 |");
    ValidateUpgradeIsComplete(2);

    // Validate FTs
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2 (2.0:2.0:2)]");
    scenarioTest_->ValidateFT(L"SL2", L"C None 000/000/411 1:1 LP");
    scenarioTest_->ValidateFT(L"SV2", L"C None 000/000/411 1:1 LP");
    scenarioTest_->ValidateFT(L"SP2", L"C None 000/000/411 1:1 LP");
}

BOOST_AUTO_TEST_CASE(UpgradeWithDeletedServiceTypes3)
{
    // Test a scenario where a service package is upgraded and one of the service types in that package is deleted
    // Hosting is requesting the runtime for this service package to go down
    // RA must not close this service
    // Setup:
    // Two FTs, SL1, SV1 both in Runtime1.
    // The service type for SV1 is deleted
    // The runtime is closed for Runtime1
    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SL1_SP1_STContext);
    scenarioTest_->AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    scenarioTest_->AddFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] | [SP1 SV1_ST]");

    scenarioTest_->ResetAll();

    // Validate the retry once
    FireCloseCompletionCheckTimer(2);
    scenarioTest_->RequestWorkAndDrain(scenarioTest_->StateItemHelpers.ReplicaCloseRetryHelper);
    scenarioTest_->ResetAll();

    // Now process DeleteReplica 
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SV1", L"000/411 [N/P RD U 1:1]");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SL1", L"000/411 [N/N RD U 1:1] Success -");

    scenarioTest_->ResetAll();

    // Perform close check again
    // This time we should succeed because the replica has been closed
    FireCloseCompletionCheckTimer(2);

    // The upgrade should still be in WaitForDrop
    Verify::AreEqual(UpgradeStateName::ApplicationUpgrade_WaitForDropCompletion, GetElement(2)->Current->CurrentState);

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SV1", L"000/411 [N/P RD U 1:1] Success D");
    FireDropCompletionCheckTimer(2);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SL1 000/000/411 [N/N/N RD D 1:1]} |");

    scenarioTest_->ResetAll();
    FireReplicaDownCompletionCheckTimer(2);

    // the upgrade should be complete
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 |");
    ValidateUpgradeIsComplete(2);

    // Validate FTs
    scenarioTest_->ValidateFT(L"SL1", L"C None 000/000/411 1:1 -");
    scenarioTest_->ValidateFT(L"SV1", L"C None 000/000/411 1:1 LP");
}

BOOST_AUTO_TEST_CASE(UpgradeThatOnlyCausesTypesToBeDeletedIsNoOp)
{
    // An upgrade which only causes service types to be deleted should be no-op
    // One FT SL1 which is of a service type being deleted
    scenarioTest_->AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] | [SP1 SL1_ST]");
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 |");

    // Validate FTs
    scenarioTest_->ValidateFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(UpgradeTargetingNoFTsIsNoOp_Rolling)
{
    UpgradeTargetingNoFTsTestHelper(L"2 Rolling false");
}

BOOST_AUTO_TEST_CASE(UpgradeTargetingNoFTsIsNoOp_Rolling_NotificationOnly)
{
    UpgradeTargetingNoFTsTestHelper(L"2 Rolling_NotificationOnly false");
}

BOOST_AUTO_TEST_CASE(UpgradeTargetingNoFTsIsNoOp_Rolling_ForceRestart)
{
    UpgradeTargetingNoFTsTestHelper(L"2 Rolling_ForceRestart false");
}

BOOST_AUTO_TEST_CASE(RollingNotificationOnlyUpgradeTestScenario)
{
    UpgradeWithReplicaRestartTestHelper(L"2 Rolling_NotificationOnly false [SP1 2.0]");
}

BOOST_AUTO_TEST_CASE(RollingUpgradeTestScenario)
{
    UpgradeWithReplicaRestartTestHelper(L"2 Rolling false [SP1 2.0]");
}

BOOST_AUTO_TEST_CASE(RollingForceUpgradeTestScenario)
{
    UpgradeWithReplicaRestartTestHelper(L"2 Rolling_ForceRestart false [SP1 2.0]");
}

BOOST_AUTO_TEST_CASE(RollingNotification_UpgradeWithLowerInstanceDoesNotAffectReplicas)
{
    UpgradeWithLowerInstanceDoesNotAffectReplicas(L"2 Rolling_NotificationOnly false");
}

BOOST_AUTO_TEST_CASE(Rolling_UpgradeWithLowerInstanceDoesNotAffectReplicas)
{
    UpgradeWithLowerInstanceDoesNotAffectReplicas(L"2 Rolling false");
}

BOOST_AUTO_TEST_CASE(RollingForceRestart_UpgradeWithLowerInstanceDoesNotAffectReplicas)
{
    UpgradeWithLowerInstanceDoesNotAffectReplicas(L"2 Rolling_ForceRestart false");
}

BOOST_AUTO_TEST_CASE(CancelTest1)
{
    // Sanity to set instance id 2 as too high
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::CancelApplicationUpgradeRequest>(L"2 Rolling false");
    scenarioTest_->ValidateFMMessage<MessageType::CancelApplicationUpgradeReply>(L"2 |");

    scenarioTest_->ResetAll();

    // instance id 1 is dropped
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"1 Rolling false");
    scenarioTest_->ValidateNoMessages();
}

BOOST_AUTO_TEST_CASE(CancelTest2)
{
    scenarioTest_->HostingHelper.HostingObj.DownloadApplicationAsyncApi.SetCompleteAsynchronously();

    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    // Upgrade is stuck on download
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0]");
    scenarioTest_->ResetAll();

    // cancel current instance
    // cancel should be sent
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::CancelApplicationUpgradeRequest>(L"3 Rolling false");
    scenarioTest_->ValidateFMMessage<MessageType::CancelApplicationUpgradeReply>(L"3 |");

    // complete the async op
    scenarioTest_->ResetAll();
    scenarioTest_->UTContext.Hosting.DownloadApplicationAsyncApi.FinishOperationWithSuccess(nullptr);

    // this should send no information as the upgrade is still stuck
    scenarioTest_->ValidateNoMessages();
}

BOOST_AUTO_TEST_CASE(UpgradeShouldReopenReplicasThatWereDownAtStart)
{
    scenarioTest_->SetFindServiceTypeRegistrationState(L"SP1", FindServiceTypeRegistrationError::CategoryName::RetryableErrorForReopen);
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 1 - [N/N/P SB D N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0]");

    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 |");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2 (2.0:2.0:2)]");
}

// App upgrade comes in during EnumerateFT phase
BOOST_AUTO_TEST_CASE(UpgradeShouldNotOpenDownReplicaOnDeactivatedNodeScenario1)
{
	scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
	scenarioTest_->ProcessFMMessage<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0]");
	scenarioTest_->DeactivateNode(2);
	scenarioTest_->DrainJobQueues();
	
	// Validate FTs. Replica is not opened
	scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1 (2.0:2.0:2)]");
}

// Node is deactivated and then app upgrade happened
BOOST_AUTO_TEST_CASE(UpgradeShouldNotOpenDownReplicaOnDeactivatedNodeScenario2)
{
	scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
	scenarioTest_->DeactivateNode(2);
	scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0]");
	
	// Validate FTs. Replica is not opened
	scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1 (2.0:2.0:2)]");
}

// App upgrade comes in during download phase
BOOST_AUTO_TEST_CASE(UpgradeShouldNotOpenDownReplicaOnDeactivatedNodeScenario3)
{
	scenarioTest_->HostingHelper.HostingObj.DownloadApplicationAsyncApi.SetCompleteAsynchronously();
	scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
	scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0]");
	scenarioTest_->DeactivateNode(2);

	scenarioTest_->UTContext.Hosting.DownloadApplicationAsyncApi.FinishOperationWithSuccess(nullptr);
	scenarioTest_->DrainJobQueues();

	// Validate FTs. Replica is not opened
	scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1 (2.0:2.0:2)]");
}

// Node is deactivated after replica closed during app upgrade
BOOST_AUTO_TEST_CASE(UpgradeShouldNotOpenDownReplicaOnDeactivatedNodeScenario4)
{
	scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP1_SP1_STContext);
	scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

	scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] | ");
	scenarioTest_->ResetAll();

	scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

	scenarioTest_->ResetAll();

	scenarioTest_->DeactivateNode(2);

	FireCloseCompletionCheckTimer(2);

	scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");
	scenarioTest_->ResetAll();

	scenarioTest_->DrainJobQueues();

	FireReplicaDownCompletionCheckTimer(2);

	// the upgrade should be complete
	scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 |");
	ValidateUpgradeIsComplete(2);

	// Validate replica is not opened
	scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 1 - [N/N/P SB D N F 1:1 (2.0:2.0:2)]");
}

// Node is deactivated after replica closed during app upgrade, the get activated
BOOST_AUTO_TEST_CASE(UpgradeShouldNotOpenDownReplicaOnDeactivatedNodeScenario5)
{
	scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP1_SP1_STContext);
	scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

	scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] | ");
	scenarioTest_->ResetAll();

	scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

	scenarioTest_->ResetAll();

	scenarioTest_->DeactivateNode(2);

	FireCloseCompletionCheckTimer(2);

	scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");
	scenarioTest_->ResetAll();

	scenarioTest_->ActivateNode(3);

	scenarioTest_->DrainJobQueues();

	FireReplicaDownCompletionCheckTimer(2);

	// the upgrade should be complete
	scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 |");
	ValidateUpgradeIsComplete(2);

	// Validate replica is opened
	scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2 (2.0:2.0:2)]");
}

BOOST_AUTO_TEST_CASE(UpgradeOnlySendsPackagesBeingUpgradedAsPartOfDownloadSpecification)
{
    // In this we have three service packages
    // SP1 and SP2 and RA has replica of both service package
    // Package SP3_NotExist has no replicas on node
    // Replica of SP2 is Closed on the node by the time upgrade arrives
    // Test validates that when RA calls Download on hosting, it only provides SP1
    // It further validates that in Upgrade call to Hosting both SP1 and SP2 and SP3_NotExist are provided

    // Setup code
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->AddClosedFT(L"SP2");

    // Execute upgrade
    // No runtimes are affected so no replica close
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling true [SP1 2.0] [SP2 2.0] [SP3 2.0]");

    // upgrade should now be completed
    scenarioTest_->ValidateApplicationUpgradeIsCompleted(2);

    // Validation of api calls to hosting
    auto & hosting = scenarioTest_->HostingHelper;

    hosting.ValidateDownloadCount(1);
    hosting.ValidateDownloadSpecification(0, 1);
    hosting.ValidateDownloadPackageSpecification(0, 0, 2, 0, L"SP1");

    hosting.ValidateAnalyzeCount(1);
    hosting.ValidateAnalyzeSpecification(0, 1);
    hosting.ValidateAnalyzePackageSpecification(0, 0, 2, 0, L"SP1");

    hosting.ValidateUpgradeCount(1);
    hosting.ValidateUpgradeSpecification(0, 1);
    hosting.ValidateUpgradePackageSpecification(0, 0, 2, 0, L"SP1");
}

BOOST_AUTO_TEST_CASE(UpgradeWithAppHostBeingClosedInTheMiddle)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P SB U N F 1:1]");
    scenarioTest_->AddFT(L"SP2", L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");

    scenarioTest_->HostingHelper.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();

    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP1_SP1_STContext);
    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP2_SP2_STContext);
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] [SP2 2.0] |");

    scenarioTest_->ProcessAppHostClosedAndDrain(L"SP1");
    scenarioTest_->ProcessAppHostClosedAndDrain(L"SP2");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} {SP2 000/000/411 [N/N/P SB D 1:1]} | ");

    // Upgrade should complete
    scenarioTest_->ResetAll();
    scenarioTest_->FireApplicationUpgradeCloseCompletionCheckTimer(2);
    scenarioTest_->ValidateApplicationUpgradeIsCompleted(2);

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2 (2.0:2.0:2)]");

    // Validate the service description version matching ft version
    auto & ft = scenarioTest_->GetFT(L"SP1");
    Verify::AreEqual(ft.LocalReplica.PackageVersionInstance, ft.ServiceDescription.PackageVersionInstance, L"PackageVersionInstance");
}

BOOST_AUTO_TEST_CASE(UpgradeWithReplicaRestart)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    scenarioTest_->HostingHelper.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();

    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP1_SP1_STContext);
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] |");

    scenarioTest_->ResetAll();
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReportFault>(L"SP1", L"000/411 [N/P SB U 1:1] transient");

    FireCloseCompletionCheckTimer(2);

    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");


    FireCloseCompletionCheckTimer(2);

    scenarioTest_->ValidateApplicationUpgradeIsCompleted(2);

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2 (2.0:2.0:2)]");
}

BOOST_AUTO_TEST_CASE(UpgradeWithReplicaDrop)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReportFault>(L"SP1", L"000/411 [N/P SB U 1:1] permanent");

    scenarioTest_->HostingHelper.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();

    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP1_SP1_STContext);
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] |");

    scenarioTest_->ResetAll();


    FireCloseCompletionCheckTimer(2);

    scenarioTest_->ResetAll();

    FireCloseCompletionCheckTimer(2);

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P SB U 1:1] Success -");

    FireCloseCompletionCheckTimer(2);

    scenarioTest_->ValidateApplicationUpgradeIsCompleted(2);

    scenarioTest_->ValidateFT(L"SP1", L"C None 000/000/411 1:1 G");
}

BOOST_AUTO_TEST_CASE(UpgradeShouldBeDroppedIfRAIsNotReady_Fmm)
{
    UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(true, false);
}

BOOST_AUTO_TEST_CASE(UpgradeShouldBeDroppedIfRAIsNotReady_FM)
{
    UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(false, true);
}

BOOST_AUTO_TEST_CASE(UpgradeShouldBeDroppedIfRAIsNotReady_FmmAndFM)
{
    UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(false, false);
}

BOOST_AUTO_TEST_CASE(CancelShouldBeDroppedIfRAIsNotReady_Fmm)
{
    CancelShouldBeDroppedIfRAIsNotReadyTestHelper(true, false);
}

BOOST_AUTO_TEST_CASE(CancelShouldBeDroppedIfRAIsNotReady_FM)
{
    CancelShouldBeDroppedIfRAIsNotReadyTestHelper(false, true);
}

BOOST_AUTO_TEST_CASE(CancelShouldBeDroppedIfRAIsNotReady_FmmAndFM)
{
    CancelShouldBeDroppedIfRAIsNotReadyTestHelper(false, false);
}

BOOST_AUTO_TEST_CASE(UpgradeTerminatesServiceHost)
{
    // Setup:
    // SP1 closing
    scenarioTest_->UTContext.Config.ApplicationUpgradeMaxReplicaCloseDuration = TimeSpan::Zero;

    scenarioTest_->UTContext.Hosting.AddAffectedRuntime(Default::GetInstance().SP1_SP1_STContext);
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] | ");
    scenarioTest_->ResetAll();

    FireCloseCompletionCheckTimer(2);

    scenarioTest_->ProcessAppHostClosedAndDrain(L"SP1");
    FireCloseCompletionCheckTimer(2);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");
    scenarioTest_->ResetAll();

    FireReplicaDownCompletionCheckTimer(2);

    // the upgrade should be complete
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpgradeReply>(L"2 |");
    ValidateUpgradeIsComplete(2);

    // Validate FTs
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2 (2.0:2.0:2)]");

    // validate terminate was called
    scenarioTest_->ValidateTerminateCall(L"SP1");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Upgrade and CreateReplica

class TestStateMachine_PackageVersionInstance
{
protected:
    ScenarioTestHolderUPtr holder_;

    class Version
    {
    public:
        enum Enum
        {
            None,
            V1,
            V2,
            V3
        };

        static ServiceModel::ServicePackageVersionInstance Get(Version::Enum version)
        {
            static const ServiceModel::ServicePackageVersionInstance v1 = Reader::ReadHelper<ServiceModel::ServicePackageVersionInstance>(L"1.0:1.0:1");
            static const ServiceModel::ServicePackageVersionInstance v2 = Reader::ReadHelper<ServiceModel::ServicePackageVersionInstance>(L"2.0:2.0:2");
            static const ServiceModel::ServicePackageVersionInstance v3 = Reader::ReadHelper<ServiceModel::ServicePackageVersionInstance>(L"3.0:3.0:3");

            if (version == Version::None)
            {
                return ServiceModel::ServicePackageVersionInstance::Invalid;
            }
            else if (version == Version::V1)
            {
                return v1;
            }
            else if (version == Version::V2)
            {
                return v2;
            }
            else if (version == Version::V3)
            {
                return v3;
            }
            else
            {
                Assert::CodingError("invalid value");
            }
        }
    };

    void VerifyPackageVersionInstance(wstring const & ftShortName, Version::Enum expected);
    void AddServicePackage(wstring const & ftShortName, Version::Enum version);
    void AddFindServiceTypeRegistrationVersionMismatch(wstring const & ftShortName, Version::Enum version);
    void ProcessAddPrimary(wstring const & ftShortName);
    void UpgradeToV2();

    void Execute(Version::Enum ftVersion, bool isUp, bool hasRegistration, Version::Enum messageVersion, Version::Enum upgradeVersion, Version::Enum expected)
    {
        auto ftVersionInstance = Version::Get(ftVersion);
        auto messageVersionInstance = Version::Get(messageVersion);
        auto upgradeVersionInstance = Version::Get(upgradeVersion);

        auto actual = ReconfigurationAgent::Test_GetServicePackageVersionInstance(ftVersionInstance, isUp, hasRegistration, messageVersionInstance, upgradeVersionInstance);

        Verify::AreEqual(Version::Get(expected), actual);
    }
};

void TestStateMachine_PackageVersionInstance::VerifyPackageVersionInstance(wstring const & ftShortName, Version::Enum expected)
{
    auto & test = holder_->ScenarioTestObj;
    auto & ft = test.GetFT(ftShortName);
    auto expectedVersion = Version::Get(expected);
    Verify::AreEqual(ft.LocalReplica.PackageVersionInstance, expectedVersion, L"LocalReplica");
    Verify::AreEqual(ft.ServiceDescription.PackageVersionInstance, expectedVersion, L"ServiceDescription");
}

void TestStateMachine_PackageVersionInstance::AddServicePackage(wstring const & ftShortName, Version::Enum)
{
    auto upgradedSTContext = Default::GetInstance().SP1_SP1_STContext;
    upgradedSTContext.ServicePackageVersionInstance = Version::Get(Version::V3);

    auto upgradedFTContext = Default::GetInstance().SP1_FTContext;
    upgradedFTContext.STInfo = upgradedSTContext;
    upgradedFTContext.ShortName = ftShortName;

    holder_->ScenarioTestObj.AddFTContext(ftShortName, upgradedFTContext);
}

void TestStateMachine_PackageVersionInstance::UpgradeToV2()
{
    holder_->ScenarioTestObj.ProcessFMMessageAndDrain<MessageType::NodeUpgradeRequest>(L"2 Rolling false [SP1 2.0] |");
}

void TestStateMachine_PackageVersionInstance::AddFindServiceTypeRegistrationVersionMismatch(wstring const &, Version::Enum version)
{
    auto stContext = Default::GetInstance().SP1_SP1_STContext;
    stContext.ServicePackageVersionInstance = Version::Get(version);

    holder_->ScenarioTestObj.HostingHelper.HostingObj.AddFindServiceTypeExpectation(stContext, ErrorCodeValue::HostingServiceTypeRegistrationVersionMismatch);
}

void TestStateMachine_PackageVersionInstance::ProcessAddPrimary(wstring const & ftShortName)
{
    holder_->ScenarioTestObj.ProcessFMMessageAndDrain<MessageType::AddPrimary>(ftShortName, L"411/422 [P/P IB U 1:1]");
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_PackageVersionInstanceSuite, TestStateMachine_PackageVersionInstance)

BOOST_AUTO_TEST_CASE(InitialCreate)
{
    // First time a create replica is received
    // Upgrade table is empty and ft is just created
    // Expect the version to be version in message
    Execute(Version::None, false, false, Version::V1, Version::None, Version::V1);
    Execute(Version::None, false, false, Version::V1, Version::V1, Version::V1);
    Execute(Version::None, false, false, Version::V2, Version::None, Version::V2);
    Execute(Version::None, false, false, Version::V2, Version::V1, Version::V2);
}

BOOST_AUTO_TEST_CASE(InitialCreateInLowerVersionAndNodeHasProcessedUpgrade)
{
    // Node does not have FT but has previously processed upgrade message from v1 -> v2
    // Upgrade table is empty and ft is just created
    // Expect the version to be version in upgrade table: V2
    Execute(Version::None, false, false, Version::V1, Version::V2, Version::V2);
    Execute(Version::None, false, false, Version::V2, Version::V2, Version::V2);
}

BOOST_AUTO_TEST_CASE(CreateReplica)
{
    // App Host went down. Retry of create replica
    Execute(Version::V1, false, false, Version::V1, Version::None, Version::V1);
    Execute(Version::V1, false, false, Version::V1, Version::V1, Version::V1);

    // Replica hasn't been opened yet i.e. Find has returned an error but replica is Up
    Execute(Version::V1, true, false, Version::V1, Version::None, Version::V1);
    Execute(Version::V1, true, false, Version::V1, Version::V1, Version::V1);

    // Misc. retry
    Execute(Version::V1, true, true, Version::V1, Version::None, Version::V1);
    Execute(Version::V1, true, true, Version::V1, Version::V1, Version::V1);
}

BOOST_AUTO_TEST_CASE(CreateReplicaAndNodeHasProcessedUpgrade)
{
    Execute(Version::V1, false, false, Version::V1, Version::V2, Version::V2);

    Execute(Version::V1, true, false, Version::V1, Version::V2, Version::V2);

    Execute(Version::V1, true, true, Version::V1, Version::V2, Version::V1);
}

BOOST_AUTO_TEST_CASE(CreateReplicaWithHigherInstance)
{
    Execute(Version::V1, false, false, Version::V2, Version::None, Version::V2); // App Host is down, retry of create replica
    Execute(Version::V1, false, false, Version::V2, Version::V1, Version::V2);

    Execute(Version::V1, true, false, Version::V2, Version::None, Version::V2); // Reopen is happening but service type registration is not present
    Execute(Version::V1, true, false, Version::V2, Version::V1, Version::V2);

    Execute(Version::V1, true, true, Version::V2, Version::None, Version::V1); // FT is up and ready - do not overwrite - an upgrade will come and update this
    Execute(Version::V1, true, true, Version::V2, Version::V1, Version::V1);
}

BOOST_AUTO_TEST_CASE(CreateReplicaWithHigherInstanceAndNodeHasProcessedUpgrade)
{
    Execute(Version::V1, false, false, Version::V2, Version::V2, Version::V2);

    Execute(Version::V1, true, false, Version::V2, Version::V2, Version::V2);

    Execute(Version::V1, true, true, Version::V2, Version::V2, Version::V1);
}

BOOST_AUTO_TEST_CASE(FTHasHigherInstance)
{
    Execute(Version::V2, false, false, Version::V1, Version::None, Version::V2);
    Execute(Version::V2, false, false, Version::V1, Version::V1, Version::V2);
    Execute(Version::V2, false, false, Version::V1, Version::V2, Version::V2);

    Execute(Version::V2, true, false, Version::V1, Version::None, Version::V2);
    Execute(Version::V2, true, false, Version::V1, Version::V1, Version::V2);
    Execute(Version::V2, true, false, Version::V1, Version::V2, Version::V2);

    Execute(Version::V2, true, true, Version::V1, Version::None, Version::V2);
    Execute(Version::V2, true, true, Version::V1, Version::V1, Version::V2);
    Execute(Version::V2, true, true, Version::V1, Version::V2, Version::V2);

    Execute(Version::V2, false, false, Version::V2, Version::None, Version::V2);
    Execute(Version::V2, false, false, Version::V2, Version::V1, Version::V2);
    Execute(Version::V2, false, false, Version::V2, Version::V2, Version::V2);

    Execute(Version::V2, true, false, Version::V2, Version::None, Version::V2);
    Execute(Version::V2, true, false, Version::V2, Version::V1, Version::V2);
    Execute(Version::V2, true, false, Version::V2, Version::V2, Version::V2);

    Execute(Version::V2, true, true, Version::V2, Version::None, Version::V2);
    Execute(Version::V2, true, true, Version::V2, Version::V1, Version::V2);
    Execute(Version::V2, true, true, Version::V2, Version::V2, Version::V2);
}

BOOST_AUTO_TEST_CASE(IntegrationTest)
{
    holder_ = ScenarioTestHolder::Create();

    holder_->Recreate();

    UpgradeToV2();

    ProcessAddPrimary(L"SP1");

    VerifyPackageVersionInstance(L"SP1", Version::V2);
}

BOOST_AUTO_TEST_CASE(FindServiceTypeRegistration_VersionMismatch_Uses_VersionReturnedByFind)
{
    holder_ = ScenarioTestHolder::Create();

    holder_->Recreate();

    AddServicePackage(L"SP1.2", Version::V3);

    // Set find service type registration to return v2
    AddFindServiceTypeRegistrationVersionMismatch(L"SP1.2", Version::V2);

    ProcessAddPrimary(L"SP1.2");

    VerifyPackageVersionInstance(L"SP1.2", Version::V2);
}

BOOST_AUTO_TEST_CASE(FindServiceTypeRegistration_VersionMismatchHigherThanUpgrade_Uses_VersionReturnedByFind)
{
    holder_ = ScenarioTestHolder::Create();

    holder_->Recreate();

    UpgradeToV2();

    // Set find service type registration to return v3
    AddFindServiceTypeRegistrationVersionMismatch(L"SP1.2", Version::V3);

    // Add primary in v1
    ProcessAddPrimary(L"SP1");

    VerifyPackageVersionInstance(L"SP1", Version::V3);
}

BOOST_AUTO_TEST_CASE(FindServiceTypeRegistration_VersionMismatchLowerThanUpgrade_NoChange)
{
    holder_ = ScenarioTestHolder::Create();

    holder_->Recreate();

    UpgradeToV2();

    AddServicePackage(L"SP1.2", Version::V3);

    // Set find service type registration to return v1
    AddFindServiceTypeRegistrationVersionMismatch(L"SP1.2", Version::V1);

    // Process AddPrimary in V3
    ProcessAddPrimary(L"SP1.2");

    VerifyPackageVersionInstance(L"SP1.2", Version::V3);
}

BOOST_AUTO_TEST_CASE(UpdateServiceSetsPackageVersionInstanceOnServiceDescription)
{
    holder_ = ScenarioTestHolder::Create();

    auto & test = holder_->Recreate();

    AddServicePackage(L"SP1.2", Version::V3);

    ProcessAddPrimary(L"SP1.2");

    test.ProcessFMMessageAndDrain<MessageType::NodeUpdateServiceRequest>(L"SP1 2 1");

    VerifyPackageVersionInstance(L"SP1.2", Version::V3);
    auto & ft = test.GetFT(L"SP1.2");
    Verify::AreEqualLogOnError(ft.ServiceDescription.UpdateVersion, 2, L"SD must be updated as well");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Create Replica
namespace CreateReplicaTest
{
    class ScenarioName
    {
    public:
        enum Enum
        {
            Stateless,
            StatefulVolatileAddPrimary,
            StatefulVolatileCreateIdleReplica,
            StatefulPersistedAddPrimary,
            StatefulPersistedCreateIdleReplica,
            StatefulPersistedRestart
        };
    };

    void WriteToTextWriter(TextWriter & w, ScenarioName::Enum e)
    {
        switch (e)
        {
        case ScenarioName::Stateless:
            w << "Stateless";
            return;

        case ScenarioName::StatefulVolatileAddPrimary:
            w << "StatefulVolatileAddPrimary";
            return;

        case ScenarioName::StatefulPersistedAddPrimary:
            w << "StatefulPersistedAddPrimary";
            return;

        case ScenarioName::StatefulVolatileCreateIdleReplica:
            w << "StatefulVolatileCreateIdleReplica";
            return;

        case ScenarioName::StatefulPersistedCreateIdleReplica:
            w << "StatefulPersistedCreateIdleReplica";
            return;

        case ScenarioName::StatefulPersistedRestart:
            w << "StatefulPersistedRestart";
            return;

        default:
            Assert::CodingError("cannot come here {0}", static_cast<int>(e));
        };
    }

    class StateName
    {
    public:
        enum Enum
        {
            NonExistant,
            Closed,
            ClosedWithLowerInstanceId,
            ICWithoutServiceTypeRegistration,
            ICWithServiceTypeRegistration,
            Open,
            Closing,
            StandByWithoutServiceTypeRegistration,
            StandByWithServiceTypeRegistration,
            Dropped,
            Deleting
        };
    };

    void WriteToTextWriter(TextWriter & w, StateName::Enum e)
    {
        switch (e)
        {
        case StateName::Closed:
            w << "Closed";
            return;

        case StateName::ClosedWithLowerInstanceId:
            w << "ClosedWithLowerInstanceId";
            return;

        case StateName::ICWithoutServiceTypeRegistration:
            w << "ICWithoutServiceTypeRegistration";
            return;

        case StateName::ICWithServiceTypeRegistration:
            w << "ICWithServiceTypeRegistration";
            return;

        case StateName::Open:
            w << "Open";
            return;

        case StateName::Closing:
            w << "Closing";
            return;

        case StateName::Dropped:
            w << "Dropped";
            return;

        case StateName::Deleting:
            w << "Deleting";
            return;

        case StateName::NonExistant:
            w << "NonExistant";
            return;

        case StateName::StandByWithoutServiceTypeRegistration:
            w << "StandByWithoutServiceTypeRegistration";
            return;

        case StateName::StandByWithServiceTypeRegistration:
            w << "StandByWithServiceTypeRegistration";
            return;

        default:
            Assert::CodingError("cannot come here, check verify logic");
        };
    }

    class ReplicaReopenReplyType
    {
    public:
        enum Enum
        {
            Success,
            Failure,
            FailureWithAbort
        };
    };

    class ReplicaOpenReplyType
    {
    public:
        enum Enum
        {
            Success,
            StateChangedOnDataLoss,
            Failure,
            FailureWithAbort
        };
    };

    class ReplicaOpenType
    {
    public:
        enum Enum
        {
            NormalRetry,

            // Final retry is the replica open sent with abort
            FinalRetry
        };
    };

    class ReplicaReopenType
    {
    public:
        enum Enum
        {
            NormalRetry,
            FinalRetry
        };
    };

    class TestStateMachine_ReplicaCreation
    {
        class ScenarioDescription
        {
        public:
            ScenarioDescription(ScenarioName::Enum name, wstring const & ftShortName)
                : ftShortName_(ftShortName),
                name_(name)
            {
            }

            __declspec(property(get = get_FTShortName)) std::wstring const & FTShortName;
            std::wstring const & get_FTShortName() const { return ftShortName_; }

            __declspec(property(get = get_Name)) ScenarioName::Enum Name;
            ScenarioName::Enum get_Name() const { return name_; }

            __declspec(property(get = get_ReplicaId)) int64 ReplicaId;
            int64 get_ReplicaId() const { return 1; }

            __declspec(property(get = get_ReplicaInstanceId)) int64 ReplicaInstanceId;
            int64 get_ReplicaInstanceId() const { return 1; }

            void PerformExternalTrigger(ScenarioTest & test) const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    test.ProcessFMMessageAndDrain<MessageType::AddInstance>(ftShortName_, L"000/422 [N/N RD U 1:1]");
                    return;

                case ScenarioName::StatefulVolatileAddPrimary:
                case ScenarioName::StatefulPersistedAddPrimary:
                    test.ProcessFMMessageAndDrain<MessageType::AddPrimary>(ftShortName_, L"311/422 [N/P RD U 1:1]");
                    return;

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    test.ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, ftShortName_, L"000/422 [N/I RD U 1:1]");
                    return;

                default:
                    Assert::CodingError("cannot come here {0}", static_cast<int>(name_));
                }
            }

            void PerformLocalRetry(ScenarioTest & test) const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                case ScenarioName::StatefulVolatileAddPrimary:
                case ScenarioName::StatefulPersistedAddPrimary:
                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                case ScenarioName::StatefulPersistedRestart:
                    test.RequestWorkAndDrain(test.StateItemHelpers.ReplicaOpenRetryHelper);
                    return;

                default:
                    Assert::CodingError("cannot come here {0}", static_cast<int>(name_));
                }
            }

            wstring GetReplicaOpenMessage(ReplicaOpenType::Enum type) const
            {
                wstring flags;
                flags = type == ReplicaOpenType::NormalRetry ? L"-" : L"A";
                wstring replicaFragment;

                switch (name_)
                {
                case ScenarioName::Stateless:
                    replicaFragment = L"000/422 [N/N IB U 1:1]";
                    break;

                case ScenarioName::StatefulVolatileAddPrimary:
                case ScenarioName::StatefulPersistedAddPrimary:
                    replicaFragment = L"311/422 [N/P IB U 1:1]";
                    break;

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    replicaFragment = L"000/422 [N/I IB U 1:1]";
                    break;

                default:
                    Assert::CodingError("cannot come here {0}", static_cast<int>(name_));
                }

                return wformatString("{0} {1} s", replicaFragment, flags);
            }

            wstring GetReplicaReopenMessage(ReplicaReopenType::Enum type) const
            {
                ASSERT_IF(name_ != ScenarioName::StatefulPersistedRestart, "Must be restart");

                if (type == ReplicaReopenType::NormalRetry)
                {
                    return L"000/422 [N/P SB U 1:1] - s";
                }
                else
                {
                    return L"000/422 [N/P SB U 1:1] A s";
                }
            }

            wstring GetReplicaReopenReply(ReplicaReopenReplyType::Enum type) const
            {
                wstring error = type == ReplicaReopenReplyType::Success ? L"Success" : L"GlobalLeaseLost";
                wstring flags = type == ReplicaReopenReplyType::FailureWithAbort ? L"A" : L"-";
                return L"000/422 [N/P SB U 1:1] " + error + L" " + flags;
            }

            wstring GetReplicaOpenReply(ReplicaOpenReplyType::Enum type) const
            {
                wstring flags = type == ReplicaOpenReplyType::FailureWithAbort ? L"A" : L"-";
                wstring prefix;

                switch (name_)
                {
                case ScenarioName::Stateless:
                    prefix = GetReplicaOpenReplyForStateless(type);
                    break;

                case ScenarioName::StatefulVolatileAddPrimary:
                case ScenarioName::StatefulPersistedAddPrimary:
                    prefix = GetReplicaOpenReplyForStatefulPrimary(type);
                    break;

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    prefix = GetReplicaOpenReplyForStatefulIdle(type);
                    break;

                default:
                    Assert::CodingError("cannot come here {0}", static_cast<int>(name_));
                }

                return prefix + L" " + flags;
            }

            wstring GetStateString(StateName::Enum name) const
            {
                switch (name)
                {
                case StateName::Closed:
                    return GetStateStringForClosed();

                case StateName::ClosedWithLowerInstanceId:
                    return GetStateStringForClosedWithLowerInstanceId();

                case StateName::ICWithoutServiceTypeRegistration:
                    return GetStateStringForICWithoutServiceTypeRegistration();

                case StateName::ICWithServiceTypeRegistration:
                    return GetStateStringForICWithServiceTypeRegistration();

                case StateName::Open:
                    return GetStateStringForOpen();

                case StateName::Closing:
                    return GetStateStringForClosing();

                case StateName::StandByWithoutServiceTypeRegistration:
                    return GetStateStringForStandByWithoutServiceTypeRegistration();

                case StateName::StandByWithServiceTypeRegistration:
                    return GetStateStringForStandByWithServiceTypeRegistration();

                case StateName::Deleting:
                    return GetStateStringForDeleting();

                case StateName::Dropped:
                    return GetStateStringForDropped();

                case StateName::NonExistant:
                default:
                    Assert::CodingError("cannot come here, check verify logic");
                };
            }

            void ValidateFailureMessageOnFSRFailure(ScenarioTest & test, FindServiceTypeRegistrationError::CategoryName::Enum category) const
            {
                ASSERT_IF(category == FindServiceTypeRegistrationError::CategoryName::Success, "Cannot be success");
                ValidateFailureMessageSent(test, FindServiceTypeRegistrationError::Create(category).Error.ReadValue());
            }

            void ValidateFailureMessageOnReplicaOpenFailure(ScenarioTest & test, ReplicaOpenReplyType::Enum replyType) const
            {
                ASSERT_IF(replyType != ReplicaOpenReplyType::Failure && replyType != ReplicaOpenReplyType::FailureWithAbort, "must be failure");
                ValidateFailureMessageSent(test, ErrorCodeValue::GlobalLeaseLost);
            }

            void ValidateFailureMessageOnReplicaReopenFailure(ScenarioTest & test) const
            {
                // Error is not applicable because this is not surfaced to FM
                ValidateFailureMessageSent(test, ErrorCodeValue::CorruptedImageStoreObjectFound);
            }

            void ValidateSuccessMessageSent(ScenarioTest & test) const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    test.ValidateFMMessage<MessageType::AddInstanceReply>(ftShortName_, L"000/422 [N/N RD U 1:1] Success");
                    return;

                case ScenarioName::StatefulVolatileAddPrimary:
                case ScenarioName::StatefulPersistedAddPrimary:
                    test.ValidateFMMessage<MessageType::AddPrimaryReply>(ftShortName_, L"000/422 [N/P RD U 1:1] Success");
                    return;

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    test.ValidateNoFMMessages();
                    test.ValidateRAMessage<MessageType::CreateReplicaReply>(2, ftShortName_, L"000/422 [N/I IB U 1:1] Success");
                    return;

                case ScenarioName::StatefulPersistedRestart:
                    test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false {" + ftShortName_ + L" 000/000/422 [N/N/P SB U 1:1 -1 -1 (1.0:1.0:1)]} |");
                    return;

                default:
                    Assert::CodingError("cannot come here {0}", static_cast<int>(name_));
                }
            }

            static ScenarioDescription Create(ScenarioName::Enum name)
            {
                switch (name)
                {
                case ScenarioName::Stateless:
                    return ScenarioDescription(name, L"SL1");

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulVolatileAddPrimary:
                    return ScenarioDescription(name, L"SV1");

                case ScenarioName::StatefulPersistedAddPrimary:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                case ScenarioName::StatefulPersistedRestart:
                    return ScenarioDescription(name, L"SP1");

                default:
                    Assert::CodingError("cannot come here {0}", static_cast<int>(name));
                }
            }

            ErrorCodeValue::Enum GetErrorCode(ReplicaOpenReplyType::Enum) const
            {
                return ErrorCodeValue::GlobalLeaseLost;
            }

            ErrorCodeValue::Enum GetErrorCode(ReplicaReopenReplyType::Enum) const
            {
                return ErrorCodeValue::GlobalLeaseLost;
            }

        private:
            void ValidateReplicaDroppedSent(ScenarioTest & test) const
            {
                wstring message = GetReplicaDroppedMessageText();

                test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {" + ftShortName_ + L" " + GetReplicaDroppedMessageText() + L"}");
            }

            wstring GetReplicaDroppedMessageText() const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    return L"000/000/422 [N/N/N DD D 1:1 n]";

                case ScenarioName::StatefulVolatileAddPrimary:
                    return L"311/000/422 [N/N/N DD D 1:1 n]";

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                    return L"000/000/422 [N/N/N DD D 1:1 n]";

                case ScenarioName::StatefulPersistedAddPrimary:
                    return L"311/000/422 [N/N/N DD D 1:1 n]";

                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    return L"000/000/422 [N/N/N DD D 1:1 n]";

                case ScenarioName::StatefulPersistedRestart:
                    return L"000/000/422 [N/N/N DD D 1:1 n]";

                default:
                    Assert::CodingError("unknown {0}", static_cast<int>(name_));
                }
            }

            void ValidateFailureMessageSent(ScenarioTest & test, Common::ErrorCodeValue::Enum value) const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    test.ValidateFMMessage<MessageType::AddInstanceReply>(ftShortName_, L"000/422 [N/N IB U 1:1] " + wformatString(value));
                    return;

                case ScenarioName::StatefulVolatileAddPrimary:
                case ScenarioName::StatefulPersistedAddPrimary:
                    test.ValidateFMMessage<MessageType::AddPrimaryReply>(ftShortName_, L"311/422 [N/P IB U 1:1] " + wformatString(value));
                    return;

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    ValidateReplicaDroppedSent(test);
                    test.ValidateRAMessage<MessageType::CreateReplicaReply>(2, ftShortName_, L"000/422 [N/I IB U 1:1] " + wformatString(value));
                    return;

                case ScenarioName::StatefulPersistedRestart:
                    ValidateReplicaDroppedSent(test);
                    return;

                default:
                    Assert::CodingError("cannot come here {0}", static_cast<int>(name_));
                }
            }

            wstring GetStateStringForStandByWithoutServiceTypeRegistration() const
            {
                ASSERT_IF(name_ != ScenarioName::StatefulPersistedRestart, "Must be restart");

                return L"O None 000/000/422 1:1 1 S [N/N/P SB U N F 1:1]";
            }

            wstring GetStateStringForStandByWithServiceTypeRegistration() const
            {
                ASSERT_IF(name_ != ScenarioName::StatefulPersistedRestart, "Must be restart");

                return L"O None 000/000/422 1:1 1 MS [N/N/P SB U N F 1:1]";
            }

            wstring GetStateStringForDeleting() const
            {
                ASSERT_IF(name_ != ScenarioName::StatefulPersistedRestart, "Must be restart");

                return L"O None 000/000/422 1:1 CMLHF [N/N/P ID U N F 1:1]";
            }

            wstring GetStateStringForDropped() const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    return L"C None 000/000/422 1:1 G";

                case ScenarioName::StatefulVolatileAddPrimary:
                    return L"C None 311/000/422 1:1 G";

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                    return L"C None 000/000/422 1:1 G";

                case ScenarioName::StatefulPersistedAddPrimary:
                    return L"C None 311/000/422 1:1 G";

                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    return L"C None 000/000/422 1:1 G";

                case ScenarioName::StatefulPersistedRestart:
                    return L"C None 000/000/422 1:1 G";

                default:
                    Assert::CodingError("unknown {0}", static_cast<int>(name_));
                }
            }

            wstring GetStateStringForOpen() const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    return L"O None 000/000/422 {000:-1} 1:1 CM [N/N/N RD U N F 1:1]";

                case ScenarioName::StatefulVolatileAddPrimary:
                    return L"O None 000/000/422 1:1 CM [N/N/P RD U N F 1:1]";

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                    return L"O None 000/000/422 1:1 CM [N/N/I IB U N F 1:1]";

                case ScenarioName::StatefulPersistedAddPrimary:
                    return L"O None 000/000/422 1:1 CM [N/N/P RD U N F 1:1]";

                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    return L"O None 000/000/422 1:1 CM [N/N/I IB U N F 1:1]";

                case ScenarioName::StatefulPersistedRestart:
                    return L"O None 000/000/422 1:1 CMK [N/N/P SB U N F 1:1]";

                default:
                    Assert::CodingError("unknown {0}", static_cast<int>(name_));
                }
            }

            wstring GetStateStringForICWithServiceTypeRegistration() const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    return L"O None 000/000/422 {000:-1} 1:1 SM [N/N/N IC U N F 1:1]";

                case ScenarioName::StatefulVolatileAddPrimary:
                case ScenarioName::StatefulPersistedAddPrimary:
                    return L"O None 311/000/422 1:1 SM [N/N/P IC U N F 1:1]";

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    return L"O None 000/000/422 1:1 SM (sender:2) [N/N/I IC U N F 1:1]";

                default:
                    Assert::CodingError("unknown {0}", static_cast<int>(name_));
                }
            }

            wstring GetStateStringForClosed() const
            {
                switch (name_)
                {
                case ScenarioName::StatefulVolatileAddPrimary:
                case ScenarioName::StatefulPersistedAddPrimary:
                    return L"C None 311/000/422 1:1 -";

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    return L"C None 000/000/422 1:1 -";

                case ScenarioName::Stateless:
                default:
                    return L"C None 000/000/422 1:1 -";
                };
            }

            wstring GetStateStringForClosedWithLowerInstanceId() const
            {
                return L"C None 000/000/422 1:0 -";
            }

            wstring GetStateStringForICWithoutServiceTypeRegistration() const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    return L"O None 000/000/422 {000:-1} 1:1 S [N/N/N IC U N F 1:1]";

                case ScenarioName::StatefulPersistedAddPrimary:
                case ScenarioName::StatefulVolatileAddPrimary:
                    return L"O None 311/000/422 1:1 S [N/N/P IC U N F 1:1]";

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    return L"O None 000/000/422 1:1 S (sender:2) [N/N/I IC U N F 1:1]";

                default:
                    Assert::CodingError("unknown {0}", static_cast<int>(name_));
                }
            }

            wstring GetStateStringForClosing() const
            {
                switch (name_)
                {
                case ScenarioName::Stateless:
                    return L"O None 000/000/422 {000:-1} 1:1 CMHn [N/N/N IC U N F 1:1]";

                case ScenarioName::StatefulPersistedAddPrimary:
                case ScenarioName::StatefulVolatileAddPrimary:
                    return L"O None 000/000/422 1:1 CMHn [N/N/P IC U N F 1:1]";

                case ScenarioName::StatefulVolatileCreateIdleReplica:
                case ScenarioName::StatefulPersistedCreateIdleReplica:
                    return L"O None 000/000/422 1:1 CMHn [N/N/I IC U N F 1:1]";

                case ScenarioName::StatefulPersistedRestart:
                    return L"O None 000/000/422 1:1 CMHn [N/N/P SB U N F 1:1]";

                default:
                    Assert::CodingError("unknown {0}", static_cast<int>(name_));
                }
            }

            wstring GetReplicaOpenReplyForStateless(ReplicaOpenReplyType::Enum type) const
            {
                ASSERT_IF(name_ != ScenarioName::Stateless, "Can only be called for stateless");

                switch (type)
                {
                case ReplicaOpenReplyType::Failure:
                case ReplicaOpenReplyType::FailureWithAbort:
                    return L"000/422 [N/N RD U 1:1] GlobalLeaseLost";

                case ReplicaOpenReplyType::Success:
                    return L"000/422 [N/N RD U 1:1] Success";

                case ReplicaOpenReplyType::StateChangedOnDataLoss:
                default:
                    Assert::CodingError("unexpected {0}", static_cast<int>(type));
                }
            }

            wstring GetReplicaOpenReplyForStatefulPrimary(ReplicaOpenReplyType::Enum type) const
            {
                ASSERT_IF(name_ != ScenarioName::StatefulVolatileAddPrimary && name_ != ScenarioName::StatefulPersistedAddPrimary, "Can only be called for add primary");

                switch (type)
                {
                case ReplicaOpenReplyType::Failure:
                case ReplicaOpenReplyType::FailureWithAbort:
                    return L"000/422 [N/P RD U 1:1] GlobalLeaseLost";

                case ReplicaOpenReplyType::Success:
                    return L"000/422 [N/P RD U 1:1] Success";

                case ReplicaOpenReplyType::StateChangedOnDataLoss:
                    return L"000/422 [N/P RD U 1:1 2 2] RAProxyStateChangedOnDataLoss";

                default:
                    Assert::CodingError("unexpected {0}", static_cast<int>(type));
                }
            }

            wstring GetReplicaOpenReplyForStatefulIdle(ReplicaOpenReplyType::Enum type) const
            {
                ASSERT_IF(name_ != ScenarioName::StatefulPersistedCreateIdleReplica && name_ != ScenarioName::StatefulVolatileCreateIdleReplica, "Can only be called for add primary");

                switch (type)
                {
                case ReplicaOpenReplyType::Failure:
                case ReplicaOpenReplyType::FailureWithAbort:
                    return L"000/422 [N/I RD U 1:1] GlobalLeaseLost";

                case ReplicaOpenReplyType::Success:
                    return L"000/422 [N/I RD U 1:1] Success";

                case ReplicaOpenReplyType::StateChangedOnDataLoss:
                default:
                    Assert::CodingError("unexpected {0}", static_cast<int>(type));
                }
            }

            wstring ftShortName_;
            ScenarioName::Enum name_;
        };

        class StateDescription
        {
        public:
            StateDescription(StateName::Enum name)
                : expectedFailureCount_(0),
                expectedFailureCountSTR_(0),
                name_(name)
            {
            }

            StateDescription(StateName::Enum name, int expectedFailureCount)
                : name_(name),
                expectedFailureCount_(expectedFailureCount),
                expectedFailureCountSTR_(0)
            {
            }

            StateDescription(StateName::Enum name, int expectedFailureCount, int expectedFailureCountSTR)
                : name_(name),
                expectedFailureCount_(expectedFailureCount),
                expectedFailureCountSTR_(expectedFailureCountSTR)
            {
            }

            __declspec(property(get = get_Name)) StateName::Enum Name;
            StateName::Enum get_Name() const { return name_; }

            __declspec(property(get = get_ExpectedFailureCount)) int ExpectedFailureCount;
            int get_ExpectedFailureCount() const { return expectedFailureCount_; }

            __declspec(property(get = get_ExpectedFailureCountSTR)) int ExpectedFailureCountSTR;
            int get_ExpectedFailureCountSTR() const { return expectedFailureCountSTR_; }

            __declspec(property(get = get_FTExists)) bool FTExists;
            bool get_FTExists() const { return name_ != StateName::NonExistant; }

            RetryableErrorStateName::Enum GetRetryableErrorStateValue() const
            {
                switch (name_)
                {
                case StateName::NonExistant:
                case StateName::Closed:
                case StateName::ClosedWithLowerInstanceId:
                case StateName::Open:
                case StateName::Deleting:
                case StateName::Dropped:
                    return RetryableErrorStateName::None;

                case StateName::ICWithoutServiceTypeRegistration:
                    return RetryableErrorStateName::ReplicaOpen;

                case StateName::ICWithServiceTypeRegistration:
                    return RetryableErrorStateName::ReplicaOpen;

                case StateName::StandByWithoutServiceTypeRegistration:
                    return RetryableErrorStateName::ReplicaReopen;

                case StateName::StandByWithServiceTypeRegistration:
                    return RetryableErrorStateName::ReplicaReopen;

                case StateName::Closing:
                    return RetryableErrorStateName::ReplicaClose;

                default:
                    Assert::CodingError("Unknown {0}", static_cast<int>(name_));
                }
            }

            RetryableErrorStateName::Enum GetSTRRetryableErrorStateValue() const
            {
                switch (name_)
                {
                case StateName::NonExistant:
                case StateName::Closed:
                case StateName::ClosedWithLowerInstanceId:
                case StateName::Open:
                case StateName::Closing:
                case StateName::Deleting:
                case StateName::Dropped:
                    return RetryableErrorStateName::None;

                case StateName::ICWithoutServiceTypeRegistration:
                    return RetryableErrorStateName::FindServiceRegistrationAtOpen;

                case StateName::ICWithServiceTypeRegistration:
                    return RetryableErrorStateName::None;

                case StateName::StandByWithoutServiceTypeRegistration:
                    return RetryableErrorStateName::FindServiceRegistrationAtReopen;

                case StateName::StandByWithServiceTypeRegistration:
                    return RetryableErrorStateName::None;

                default:
                    Assert::CodingError("Unknown {0}", static_cast<int>(name_));
                }
            }

        private:
            StateName::Enum name_;
            int expectedFailureCount_;
            int expectedFailureCountSTR_;
        };

        class Harness
        {
        public:
            Harness(ScenarioDescription const & scenario)
                : scenario_(scenario)
            {
                test_ = ScenarioTest::Create();
            }

            ~Harness()
            {
                test_->Cleanup();
            }

            void SetFTState(StateDescription const & state)
            {
                TestLog::WriteInfo(wformatString("{0}: Setting FT state {1}", scenario_.Name, state.Name));
                if (!state.FTExists)
                {
                    return;
                }

                auto entry = test_->AddFT(scenario_.FTShortName, scenario_.GetStateString(state.Name));
                entry->State.Test_Data.Data->RetryableErrorStateObj.Test_Set(state.GetRetryableErrorStateValue(), state.ExpectedFailureCount);
                entry->State.Test_Data.Data->ServiceTypeRegistrationRetryableErrorStateObj.Test_Set(state.GetSTRRetryableErrorStateValue(), state.ExpectedFailureCountSTR);
            }

            void Verify(StateDescription const & state)
            {
                TestLog::WriteInfo(wformatString("{0}: Verify FT {1}", scenario_.Name, state.Name));

                if (!state.FTExists)
                {
                    test_->ValidateFTNotPresent(scenario_.FTShortName);
                    return;
                }

                // Validate the FT
                test_->ValidateFT(scenario_.FTShortName, scenario_.GetStateString(state.Name));
                test_->ValidateFTRetryableErrorState(scenario_.FTShortName, state.GetRetryableErrorStateValue(), state.ExpectedFailureCount);
            }

            void VerifySTR(StateDescription const & state)
            {
                TestLog::WriteInfo(wformatString("{0}: Verify FT {1}", scenario_.Name, state.Name));

                if (!state.FTExists)
                {
                    test_->ValidateFTNotPresent(scenario_.FTShortName);
                    return;
                }

                // Validate the FT
                test_->ValidateFT(scenario_.FTShortName, scenario_.GetStateString(state.Name));
                test_->ValidateFTSTRRetryableErrorState(scenario_.FTShortName, state.GetSTRRetryableErrorStateValue(), state.ExpectedFailureCountSTR);
            }

            void VerifyReplicaReopenMessage(ReplicaReopenType::Enum type)
            {
                wstring message = scenario_.GetReplicaReopenMessage(type);
                test_->ValidateRAPMessage<MessageType::StatefulServiceReopen>(scenario_.FTShortName, message);
            }

            void VerifyReplicaOpenMessage(ReplicaOpenType::Enum type)
            {
                wstring message = scenario_.GetReplicaOpenMessage(type);
                test_->ValidateRAPMessage<MessageType::ReplicaOpen>(scenario_.FTShortName, message);
            }

            void VerifyNoRAPMessage()
            {
                test_->ValidateNoRAPMessages();
            }

            void VerifyNoHealthEvent()
            {
                test_->ValidateNoReplicaHealthEvent();
            }

            void VerifyClosedHealthEvent()
            {
                test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Close, scenario_.FTShortName, scenario_.ReplicaId, scenario_.ReplicaInstanceId);
            }

            void VerifyWarningHealthEvent(ReplicaOpenReplyType::Enum replyType)
            {
                auto error = scenario_.GetErrorCode(replyType);
                VerifyWarningHealthEvent(error);
            }

            void VerifyWarningHealthEvent(ReplicaReopenReplyType::Enum replyType)
            {
                auto error = scenario_.GetErrorCode(replyType);
                VerifyWarningHealthEvent(error);
            }

            void VerifyWarningHealthEvent(FindServiceTypeRegistrationError::CategoryName::Enum category)
            {
                auto error = FindServiceTypeRegistrationError::Create(category).Error.ReadValue();
                VerifyWarningHealthEvent(error);
            }

            void VerifyFSTWarningHealthEvent(FindServiceTypeRegistrationError::CategoryName::Enum category)
            {
                auto error = FindServiceTypeRegistrationError::Create(category).Error.ReadValue();
                VerifyFSTWarningHealthEvent(error);
            }

            void VerifyWarningHealthEvent(ErrorCodeValue::Enum error)
            {
                test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::OpenWarning, scenario_.FTShortName, scenario_.ReplicaId, scenario_.ReplicaInstanceId, error);
            }

            void VerifyFSTWarningHealthEvent(ErrorCodeValue::Enum error)
            {
                test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::ServiceTypeRegistrationWarning, scenario_.FTShortName, scenario_.ReplicaId, scenario_.ReplicaInstanceId, error);
            }

            void VerifyOKHealthEvent()
            {
                test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::OK, scenario_.FTShortName, scenario_.ReplicaId, scenario_.ReplicaInstanceId, ErrorCodeValue::Success);
            }

            void VerifyClearWarningHealthEvent()
            {
                test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::ClearOpenWarning, scenario_.FTShortName, scenario_.ReplicaId, scenario_.ReplicaInstanceId, ErrorCodeValue::Success);
            }

            void VerifyFSTClearWarningHealthEvent()
            {
                test_->ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::ClearServiceTypeRegistrationWarning, scenario_.FTShortName, scenario_.ReplicaId, scenario_.ReplicaInstanceId, ErrorCodeValue::Success);
            }

            void VerifyNoRAMessage()
            {
                test_->ValidateNoRAMessage();
            }

            void VerifyNoFMMessage()
            {
                test_->ValidateNoFMMessages();
                test_->ValidateNoFmmMessages();
            }

            void VerifyNoFindServiceTypeRegistrationCall()
            {
                test_->HostingHelper.HostingObj.FindServiceTypeApi.VerifyNoCalls();
            }

            void VerifyFailureMessageOnFSRFailure(FindServiceTypeRegistrationError::CategoryName::Enum category)
            {
                // request fm work
                test_->StateItemHelpers.FMMessageRetryHelperObj.Request(*FailoverManagerId::Fm);
                test_->DrainJobQueues();
                scenario_.ValidateFailureMessageOnFSRFailure(*test_, category);
            }

            void VerifyFailureMessageAfterReplicaReopenFailure()
            {
                scenario_.ValidateFailureMessageOnReplicaReopenFailure(*test_);
            }

            void VerifyFailureMessageAfterReplicaOpenFailure(ReplicaOpenReplyType::Enum type)
            {
                scenario_.ValidateFailureMessageOnReplicaOpenFailure(*test_, type);
            }

            void VerifySuccessMessageSent()
            {
                scenario_.ValidateSuccessMessageSent(*test_);
            }

            void PerformReplicaReopenReply(ReplicaReopenReplyType::Enum type)
            {
                wstring message = scenario_.GetReplicaReopenReply(type);
                test_->ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(scenario_.FTShortName, message);
            }

            void MarkDeleted()
            {
                auto & ft = test_->GetFT(scenario_.FTShortName);
                ft.Test_SetLocalReplicaDeleted(true);
            }

            void PerformReplicaOpenReply(ReplicaOpenReplyType::Enum type)
            {
                wstring message = scenario_.GetReplicaOpenReply(type);
                test_->ProcessRAPMessageAndDrain<MessageType::ReplicaOpenReply>(scenario_.FTShortName, message);
            }

            void PerformExternalTrigger()
            {
                TestLog::WriteInfo(wformatString("{0}: External Trigger", scenario_.Name));

                scenario_.PerformExternalTrigger(*test_);
            }

            void PerformLocalRetry()
            {
                TestLog::WriteInfo(wformatString("{0}: Local Retry", scenario_.Name));
                scenario_.PerformLocalRetry(*test_);
            }

            void SetFindServiceTypeRegistrationState(FindServiceTypeRegistrationError::CategoryName::Enum category)
            {
                test_->SetFindServiceTypeRegistrationState(scenario_.FTShortName, category);
            }

            void SetIncrementUsageCountError()
            {
                test_->UTContext.Hosting.IncrementApi.SetCompleteSynchronouslyWithError(ErrorCodeValue::FabricAlreadyInTargetVersion);
            }

            void SetRetryableErrorStateThreshold(RetryableErrorStateName::Enum e, int warningThreshold, int dropThreshold)
            {
                auto & ft = test_->GetFT(scenario_.FTShortName);

                ft.RetryableErrorStateObj.Test_SetThresholds(e, warningThreshold, dropThreshold, INT_MAX, INT_MAX);
            }

            void SetSTRRetryableErrorStateThreshold(RetryableErrorStateName::Enum e, int warningThreshold, int dropThreshold)
            {
                auto & ft = test_->GetFT(scenario_.FTShortName);

                ft.ServiceTypeRegistrationRetryableErrorStateObj.Test_SetThresholds(e, warningThreshold, dropThreshold, INT_MAX, INT_MAX);
            }

            void InvokeServiceTypeRegistered()
            {
                test_->ProcessServiceTypeRegisteredAndDrain(scenario_.FTShortName);
            }

            void DeactivateNode()
            {
                test_->DeactivateNode(2);
            }

            void VerifyClosing()
            {
                Verify(StateDescription(StateName::Closing));
                VerifyNoRAMessage();
                VerifyNoHealthEvent();
                VerifyNoFindServiceTypeRegistrationCall();
            }

            void VerifyClosed()
            {
                Verify(StateDescription(StateName::Closed));
            }
        private:

            ScenarioDescription const scenario_;
            ScenarioTestUPtr test_;
        };

    public:
        void FTCreationTestHelper(StateName::Enum initial, ScenarioName::Enum name);

        void ExternalRetryInICTestHelper(StateName::Enum initial, ScenarioName::Enum name);
        void RetryInICWithoutRegistrationIsNoOpTestHelper(ScenarioName::Enum name);

        void RetryInICWithRegistrationDoesNotRetryMessageToRAPTestHelper(ScenarioName::Enum name);

        void ErrorFromFSRReturnsErrorToSenderAndClosesFTTestHelper(ScenarioName::Enum name, FindServiceTypeRegistrationError::CategoryName::Enum category);

        void FailureToIncrementUsageCountCausesRetryTestHelper(ScenarioName::Enum name);

        void RetryableErrorInFSRIncrementsCountIfBelowThresholdTestHelper(ScenarioName::Enum name);

        void RetryableErrorInFSRIncrementsCountAndReportsWarningIfAtWarningThresholdTestHelper(ScenarioName::Enum name);

        void RetryableErrorInFSRDoesNotReportMultipleWarningsTestHelper(ScenarioName::Enum name);

        void SuccessAfterHealthReportWarningClearsHealthReportTestHelper(ScenarioName::Enum name);

        void ServiceTypeRegisteredSendsReplicaOpenMessageTestHelper(ScenarioName::Enum name);

        void SuccessAtFSRSendsMessageToRAPTestHelper(ScenarioName::Enum name);

        void ExceedThresholdAtFSRDropsReplicaTestHelper(ScenarioName::Enum name);

        void RetryWithServiceTypeRegistrationSendsReplicaOpenMessageTestHelper(ScenarioName::Enum name);

        void FinalRetryWithServiceTypeRegistrationSendsReplicaOpenMessageWithAbortTestHelper(ScenarioName::Enum name);

        void SuccessFromOpenSendsReplyBackToSourceTestHelper(ScenarioName::Enum name);

        void StateChangedOnDataLossFromAddPrimarySendsReplyBackTestHelper(ScenarioName::Enum name);

        void SuccessFromOpenReplyClearsWarningHealthReportTestHelper(ScenarioName::Enum name);

        void ExceedWarningThresholdReportsWarningTestHelper(ScenarioName::Enum name);

        void MultipleWarningsAreNotReportedAtOpenTestHelper(ScenarioName::Enum name);

        void ErrorInOpenIncrementsCountIfBelowThresholdTestHelper(ScenarioName::Enum name);

        void ErrorInOpenDropsReplicaIfAtThresholdTestHelper(ScenarioName::Enum name);

        void DeactivatedNodeAtOpenReplyClosesTheReplicaImmediatelyOnFailureTestHelper(ScenarioName::Enum name);

        void DeactivatedNodeAtOpenReplyClosesTheReplicaOnSuccessTestHelper(ScenarioName::Enum name);

        void RetryableOrUnknownErrorIsNoOpForSBTestHelper(ScenarioName::Enum name, FindServiceTypeRegistrationError::CategoryName::Enum);

        void FatalErrorWithoutRegistrationDropsReplicaForSBTestHelper(ScenarioName::Enum name);

        void ErrorCountExceedingThresholdReportsHealthForSBTestHelper(ScenarioName::Enum name);

        void MultipleWarningHealthReportsAreNotSentForSBTestHelper(ScenarioName::Enum name);

        void ServiceTypeRegisteredSendsReopenMessageTestHelper(ScenarioName::Enum name);

        void SuccessFSRSendsReopenMessageAndUpdatesStateForSBTestHelper(ScenarioName::Enum name);

        void SuccessFSRClearsHealthReportAndSendsReopenIfHealthWasReportedForSBTestHelper(ScenarioName::Enum name);

        void ReopenMessageRetrySendsMessageWithoutAbortFlagTestHelper(ScenarioName::Enum name);

        void FinalReopenMessageRetrySendsMessageWithAbortFlagTestHelper(ScenarioName::Enum name);

        void FailureToIncrementUsageCountCausesRetryAtReopenTestHelper(ScenarioName::Enum name);

        void FailureInFSRForSBAfterThresholdDropsReplicaTestHelper(ScenarioName::Enum name);

        void FailureInReopenIncrementsCountTestHelper(ScenarioName::Enum name);

        void FailureInReopenReportsHealthTestHelper(ScenarioName::Enum name);

        void FailureInReopenDropsReplicaTestHelper(ScenarioName::Enum name);

        void SuccessfulReopenAfterHealthClearsHealthReportTestHelper(ScenarioName::Enum name);

        void SuccessfulReopenSendsResponseTestHelper(ScenarioName::Enum name);

        void DeactivatedNodeStartsCloseAfterSuccessfulReopenTestHelper(ScenarioName::Enum name);

    protected:
        void ExecuteAllReopenScenarios(std::function<void(ScenarioName::Enum)> func);
        void ExecuteAllOpenScenarios(std::function<void(ScenarioName::Enum)> func);
        void ExecuteScenarios(ScenarioName::Enum arr[], std::function<void(ScenarioName::Enum)> func);

        void ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
            Harness & h,
            FindServiceTypeRegistrationError::CategoryName::Enum error,
            int initialErrorCount,
            int warningThreshold,
            int dropThreshold);

        void ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(
            Harness & h,
            FindServiceTypeRegistrationError::CategoryName::Enum error,
            int initialErrorCount,
            int warningThreshold,
            int dropThreshold);

        void ExecuteFindServiceTypeRegistrationWithErrorTest(
            Harness & h,
            StateName::Enum initial,
            FindServiceTypeRegistrationError::CategoryName::Enum error,
            int initialErrorCount,
            int warningThreshold,
            int dropThreshold);

        void ExecuteOpenWithErrortest(
            Harness & h,
            ReplicaOpenReplyType::Enum type,
            int initialErrorCount,
            int warningThreshold,
            int dropThreshold);

        void ExecuteReopenWithErrorTest(
            Harness & h,
            ReplicaReopenReplyType::Enum type,
            int initialErrorCount,
            int warningThreshold,
            int dropThreshold);
    };

    void TestStateMachine_ReplicaCreation::ExecuteAllOpenScenarios(std::function<void(ScenarioName::Enum)> func)
    {
        func(ScenarioName::Stateless);
        func(ScenarioName::StatefulPersistedAddPrimary);
        func(ScenarioName::StatefulPersistedCreateIdleReplica);
        func(ScenarioName::StatefulVolatileAddPrimary);
        func(ScenarioName::StatefulVolatileCreateIdleReplica);
    }

    void TestStateMachine_ReplicaCreation::ExecuteAllReopenScenarios(std::function<void(ScenarioName::Enum)> func)
    {
        func(ScenarioName::StatefulPersistedRestart);
    }

    void TestStateMachine_ReplicaCreation::ExecuteFindServiceTypeRegistrationWithErrorTest(
        Harness & harness,
        StateName::Enum initial,
        FindServiceTypeRegistrationError::CategoryName::Enum error,
        int initialErrorCount,
        int warningThreshold,
        int dropThreshold)
    {
        auto initState = StateDescription(initial, initialErrorCount, initialErrorCount);
        harness.SetFTState(initState);
        harness.SetRetryableErrorStateThreshold(initState.GetRetryableErrorStateValue(), INT_MAX, INT_MAX);
        harness.SetSTRRetryableErrorStateThreshold(initState.GetSTRRetryableErrorStateValue(), warningThreshold, dropThreshold);
        harness.SetFindServiceTypeRegistrationState(error);

        harness.PerformLocalRetry();
    }

    void TestStateMachine_ReplicaCreation::ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(
        Harness & harness,
        FindServiceTypeRegistrationError::CategoryName::Enum error,
        int initialErrorCount,
        int warningThreshold,
        int dropThreshold)
    {
        ExecuteFindServiceTypeRegistrationWithErrorTest(harness, StateName::StandByWithoutServiceTypeRegistration, error, initialErrorCount, warningThreshold, dropThreshold);
    }

    void TestStateMachine_ReplicaCreation::ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
        Harness & harness,
        FindServiceTypeRegistrationError::CategoryName::Enum error,
        int initialErrorCount,
        int warningThreshold,
        int dropThreshold)
    {
        ExecuteFindServiceTypeRegistrationWithErrorTest(harness, StateName::ICWithoutServiceTypeRegistration, error, initialErrorCount, warningThreshold, dropThreshold);
    }

    void TestStateMachine_ReplicaCreation::ExecuteOpenWithErrortest(
        Harness & harness,
        ReplicaOpenReplyType::Enum type,
        int initialErrorCount,
        int warningThreshold,
        int dropThreshold)
    {
        harness.SetFTState(StateDescription(StateName::ICWithServiceTypeRegistration, initialErrorCount));
        harness.SetRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaOpen, warningThreshold, dropThreshold);
        harness.PerformReplicaOpenReply(type);
    }

    void TestStateMachine_ReplicaCreation::ExecuteReopenWithErrorTest(
        Harness & harness,
        ReplicaReopenReplyType::Enum type,
        int initialErrorCount,
        int warningThreshold,
        int dropThreshold)
    {
        harness.SetFTState(StateDescription(StateName::StandByWithServiceTypeRegistration, initialErrorCount));
        harness.SetRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaReopen, warningThreshold, dropThreshold);
        harness.PerformReplicaReopenReply(type);
    }

    void TestStateMachine_ReplicaCreation::FTCreationTestHelper(StateName::Enum initial, ScenarioName::Enum name)
    {
        // Verify that from an initial state the FT goes into the created state and FSR is called
        Harness harness(ScenarioDescription::Create(name));

        harness.SetFTState(StateDescription(initial));
        harness.SetFindServiceTypeRegistrationState(FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);
        harness.PerformExternalTrigger();

        harness.VerifySTR(StateDescription(StateName::ICWithoutServiceTypeRegistration, 0, 1));
        harness.VerifyOKHealthEvent();
        harness.VerifyNoRAMessage();
        harness.VerifyNoFMMessage();
        harness.VerifyNoRAPMessage();
    }

    void TestStateMachine_ReplicaCreation::ExternalRetryInICTestHelper(StateName::Enum initial, ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));
        harness.SetFTState(StateDescription(initial));
        harness.PerformExternalTrigger();

        harness.Verify(StateDescription(initial));
        harness.VerifyNoRAPMessage();
        harness.VerifyNoRAMessage();
        harness.VerifyNoHealthEvent();
        harness.VerifyNoFindServiceTypeRegistrationCall();
    }

    void TestStateMachine_ReplicaCreation::ErrorFromFSRReturnsErrorToSenderAndClosesFTTestHelper(ScenarioName::Enum name, FindServiceTypeRegistrationError::CategoryName::Enum category)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
            harness,
            category,
            1,
            INT_MAX,
            INT_MAX);

        harness.Verify(StateDescription(StateName::Closed));
        harness.VerifyClosedHealthEvent();
        harness.VerifyFailureMessageOnFSRFailure(category);
    }

    void TestStateMachine_ReplicaCreation::FailureToIncrementUsageCountCausesRetryTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        auto initState = StateDescription(StateName::ICWithoutServiceTypeRegistration, 1);
        harness.SetFTState(initState);
        harness.SetRetryableErrorStateThreshold(initState.GetRetryableErrorStateValue(), 5, 10);
        harness.SetFindServiceTypeRegistrationState(FindServiceTypeRegistrationError::CategoryName::Success);
        harness.SetIncrementUsageCountError();
        harness.PerformLocalRetry();

        harness.Verify(initState);
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::RetryableErrorInFSRIncrementsCountIfBelowThresholdTestHelper(ScenarioName::Enum name)
    {
        auto category = FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen;
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
            harness,
            category,
            5, // initial
            7, // Warning
            8);  // Drop

        int finalErrorCountExpected = 6;

        harness.VerifySTR(StateDescription(StateName::ICWithoutServiceTypeRegistration, 0, finalErrorCountExpected));
        harness.VerifyNoHealthEvent();
        harness.VerifyNoRAPMessage();
    }

    void TestStateMachine_ReplicaCreation::ExceedThresholdAtFSRDropsReplicaTestHelper(ScenarioName::Enum name)
    {
        auto category = FindServiceTypeRegistrationError::CategoryName::FatalErrorForOpen;
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
            harness,
            category,
            5, // initial
            INT_MAX, // Warning
            6);  // Drop

        harness.VerifySTR(StateDescription(StateName::Closed));
        harness.VerifyFailureMessageOnFSRFailure(category);
        harness.VerifyClosedHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::RetryableErrorInFSRIncrementsCountAndReportsWarningIfAtWarningThresholdTestHelper(ScenarioName::Enum name)
    {
        auto category = FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen;
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
            harness,
            category,
            5, // initial
            6, // Warning
            INT_MAX);  // Drop

        harness.VerifySTR(StateDescription(StateName::ICWithoutServiceTypeRegistration, 0, 6));
        harness.VerifyNoRAPMessage();
        harness.VerifyFSTWarningHealthEvent(category);
    }

    void TestStateMachine_ReplicaCreation::RetryableErrorInFSRDoesNotReportMultipleWarningsTestHelper(ScenarioName::Enum name)
    {
        auto category = FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen;
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
            harness,
            category,
            6, // initial
            6, // Warning
            INT_MAX);  // Drop

        harness.VerifySTR(StateDescription(StateName::ICWithoutServiceTypeRegistration, 0, 7));
        harness.VerifyNoRAPMessage();
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::SuccessAfterHealthReportWarningClearsHealthReportTestHelper(ScenarioName::Enum name)
    {
        auto category = FindServiceTypeRegistrationError::CategoryName::Success;
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
            harness,
            category,
            7, // initial
            6, // Warning
            INT_MAX);  // Drop

        harness.VerifySTR(StateDescription(StateName::ICWithServiceTypeRegistration));
        harness.VerifyReplicaOpenMessage(ReplicaOpenType::NormalRetry);
        harness.VerifyFSTClearWarningHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::ServiceTypeRegisteredSendsReplicaOpenMessageTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));
        harness.SetFTState(StateDescription(StateName::ICWithoutServiceTypeRegistration));
        harness.InvokeServiceTypeRegistered();
        harness.VerifySTR(StateDescription(StateName::ICWithServiceTypeRegistration));
        harness.VerifyReplicaOpenMessage(ReplicaOpenType::NormalRetry);
    }

    void TestStateMachine_ReplicaCreation::SuccessAtFSRSendsMessageToRAPTestHelper(ScenarioName::Enum name)
    {
        auto category = FindServiceTypeRegistrationError::CategoryName::Success;
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtOpenWithErrorTest(
            harness,
            category,
            5, // initial
            6, // Warning
            INT_MAX);  // Drop

        harness.VerifySTR(StateDescription(StateName::ICWithServiceTypeRegistration));
        harness.VerifyReplicaOpenMessage(ReplicaOpenType::NormalRetry);
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::RetryWithServiceTypeRegistrationSendsReplicaOpenMessageTestHelper(ScenarioName::Enum name)
    {
        StateDescription initial(StateName::ICWithServiceTypeRegistration, 1);

        Harness harness(ScenarioDescription::Create(name));
        harness.SetFTState(initial);

        harness.PerformLocalRetry();

        harness.Verify(initial);
        harness.VerifyNoFindServiceTypeRegistrationCall();
        harness.VerifyNoHealthEvent();
        harness.VerifyNoFMMessage();
        harness.VerifyNoRAMessage();
        harness.VerifyReplicaOpenMessage(ReplicaOpenType::NormalRetry);
    }

    void TestStateMachine_ReplicaCreation::FinalRetryWithServiceTypeRegistrationSendsReplicaOpenMessageWithAbortTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        harness.SetFTState(StateDescription(StateName::ICWithServiceTypeRegistration, 1));
        harness.SetRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaOpen, 100, 2);
        harness.PerformLocalRetry();

        harness.Verify(StateDescription(StateName::ICWithServiceTypeRegistration, 1));
        harness.VerifyNoFindServiceTypeRegistrationCall();
        harness.VerifyNoHealthEvent();
        harness.VerifyNoFMMessage();
        harness.VerifyNoRAMessage();
        harness.VerifyReplicaOpenMessage(ReplicaOpenType::FinalRetry);
    }

    void TestStateMachine_ReplicaCreation::SuccessFromOpenSendsReplyBackToSourceTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));
        ExecuteOpenWithErrortest(harness, ReplicaOpenReplyType::Success, 0, INT_MAX, INT_MAX);
        harness.Verify(StateDescription(StateName::Open));
        harness.VerifyNoRAPMessage();
        harness.VerifyNoHealthEvent();
        harness.VerifySuccessMessageSent();
    }

    void TestStateMachine_ReplicaCreation::StateChangedOnDataLossFromAddPrimarySendsReplyBackTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));
        ExecuteOpenWithErrortest(harness, ReplicaOpenReplyType::StateChangedOnDataLoss, 0, INT_MAX, INT_MAX);
        harness.Verify(StateDescription(StateName::Open));
        harness.VerifyNoRAPMessage();
        harness.VerifyNoHealthEvent();
        harness.VerifySuccessMessageSent();
    }

    void TestStateMachine_ReplicaCreation::ErrorInOpenIncrementsCountIfBelowThresholdTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));
        ExecuteOpenWithErrortest(harness, ReplicaOpenReplyType::Failure, 5, 10, 20);

        harness.Verify(StateDescription(StateName::ICWithServiceTypeRegistration, 6));
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::ErrorInOpenDropsReplicaIfAtThresholdTestHelper(ScenarioName::Enum name)
    {
        auto replicaOpenReplyType = ReplicaOpenReplyType::FailureWithAbort;

        Harness harness(ScenarioDescription::Create(name));
        ExecuteOpenWithErrortest(harness, replicaOpenReplyType, 5, INT_MAX, 6);

        harness.Verify(StateDescription(StateName::Closed));
        harness.VerifyClosedHealthEvent();
        harness.VerifyFailureMessageAfterReplicaOpenFailure(replicaOpenReplyType);
    }

    void TestStateMachine_ReplicaCreation::SuccessFromOpenReplyClearsWarningHealthReportTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));
        ExecuteOpenWithErrortest(harness, ReplicaOpenReplyType::Success, 6, 6, INT_MAX);

        harness.Verify(StateDescription(StateName::Open));
        harness.VerifyClearWarningHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::ExceedWarningThresholdReportsWarningTestHelper(ScenarioName::Enum name)
    {
        auto replicaOpenReplyType = ReplicaOpenReplyType::Failure;
        Harness harness(ScenarioDescription::Create(name));
        ExecuteOpenWithErrortest(harness, replicaOpenReplyType, 5, 6, INT_MAX);

        harness.Verify(StateDescription(StateName::ICWithServiceTypeRegistration, 6));
        harness.VerifyWarningHealthEvent(replicaOpenReplyType);
        harness.VerifyNoFMMessage();
        harness.VerifyNoRAMessage();
        harness.VerifyNoRAPMessage();
    }

    void TestStateMachine_ReplicaCreation::MultipleWarningsAreNotReportedAtOpenTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));
        ExecuteOpenWithErrortest(harness, ReplicaOpenReplyType::Failure, 6, 6, INT_MAX);

        harness.Verify(StateDescription(StateName::ICWithServiceTypeRegistration, 7));
        harness.VerifyNoHealthEvent();
        harness.VerifyNoFMMessage();
        harness.VerifyNoRAMessage();
        harness.VerifyNoRAPMessage();
    }

    void TestStateMachine_ReplicaCreation::DeactivatedNodeAtOpenReplyClosesTheReplicaImmediatelyOnFailureTestHelper(ScenarioName::Enum name)
    {
        // NOTE: This also verifies no health event
        Harness harness(ScenarioDescription::Create(name));
        harness.DeactivateNode();
        ExecuteOpenWithErrortest(harness, ReplicaOpenReplyType::Failure, 5, 6, 7);

        harness.VerifyClosing();
        harness.VerifyFailureMessageAfterReplicaOpenFailure(ReplicaOpenReplyType::Failure);
    }

    void TestStateMachine_ReplicaCreation::DeactivatedNodeAtOpenReplyClosesTheReplicaOnSuccessTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));
        harness.DeactivateNode();
        ExecuteOpenWithErrortest(harness, ReplicaOpenReplyType::Success, 5, 6, 7);
        harness.PerformReplicaOpenReply(ReplicaOpenReplyType::Success);

        harness.VerifyClosing();
    }

    void TestStateMachine_ReplicaCreation::RetryableOrUnknownErrorIsNoOpForSBTestHelper(ScenarioName::Enum name, FindServiceTypeRegistrationError::CategoryName::Enum e)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(harness, e, 5, 8, 10);

        harness.VerifySTR(StateDescription(StateName::StandByWithoutServiceTypeRegistration, 0, 6));
        harness.VerifyNoRAPMessage();
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::FatalErrorWithoutRegistrationDropsReplicaForSBTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(harness, FindServiceTypeRegistrationError::CategoryName::FatalErrorForReopen, 5, 8, 10);

        harness.Verify(StateDescription(StateName::Dropped));
        harness.VerifyNoRAPMessage();
        harness.VerifyFailureMessageOnFSRFailure(FindServiceTypeRegistrationError::CategoryName::RetryableErrorForReopen);
        harness.VerifyClosedHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::ErrorCountExceedingThresholdReportsHealthForSBTestHelper(ScenarioName::Enum name)
    {
        auto category = FindServiceTypeRegistrationError::CategoryName::RetryableErrorForReopen;
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(harness, category, 5, 6, 10);

        harness.VerifySTR(StateDescription(StateName::StandByWithoutServiceTypeRegistration, 0, 6));
        harness.VerifyFSTWarningHealthEvent(category);
    }

    void TestStateMachine_ReplicaCreation::MultipleWarningHealthReportsAreNotSentForSBTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(harness, FindServiceTypeRegistrationError::CategoryName::RetryableErrorForReopen, 6, 6, 10);

        harness.VerifySTR(StateDescription(StateName::StandByWithoutServiceTypeRegistration, 0, 7));
        harness.VerifyNoRAPMessage();
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::ServiceTypeRegisteredSendsReopenMessageTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        harness.SetFTState(StateDescription(StateName::StandByWithoutServiceTypeRegistration));
        harness.InvokeServiceTypeRegistered();

        harness.VerifySTR(StateDescription(StateName::StandByWithServiceTypeRegistration));
        harness.VerifyReplicaReopenMessage(ReplicaReopenType::NormalRetry);
    }

    void TestStateMachine_ReplicaCreation::SuccessFSRSendsReopenMessageAndUpdatesStateForSBTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(harness, FindServiceTypeRegistrationError::CategoryName::Success, 5, 6, 10);

        harness.VerifySTR(StateDescription(StateName::StandByWithServiceTypeRegistration));
        harness.VerifyReplicaReopenMessage(ReplicaReopenType::NormalRetry);
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::SuccessFSRClearsHealthReportAndSendsReopenIfHealthWasReportedForSBTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(harness, FindServiceTypeRegistrationError::CategoryName::Success, 6, 6, 10);

        harness.VerifySTR(StateDescription(StateName::StandByWithServiceTypeRegistration));
        harness.VerifyReplicaReopenMessage(ReplicaReopenType::NormalRetry);
        harness.VerifyFSTClearWarningHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::FailureInFSRForSBAfterThresholdDropsReplicaTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteFindServiceTypeRegistrationAtReopenWithErrorTest(harness, FindServiceTypeRegistrationError::CategoryName::RetryableErrorForReopen, 6, 6, 7);

        harness.VerifySTR(StateDescription(StateName::Dropped));
        harness.VerifyNoRAPMessage();
        harness.VerifyFailureMessageOnFSRFailure(FindServiceTypeRegistrationError::CategoryName::RetryableErrorForReopen);
        harness.VerifyClosedHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::ReopenMessageRetrySendsMessageWithoutAbortFlagTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        harness.SetFTState(StateDescription(StateName::StandByWithServiceTypeRegistration, 1));
        harness.SetRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaReopen, 100, 3);

        harness.PerformLocalRetry();

        harness.Verify(StateDescription(StateName::StandByWithServiceTypeRegistration, 1));
        harness.VerifyReplicaReopenMessage(ReplicaReopenType::NormalRetry);
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::FinalReopenMessageRetrySendsMessageWithAbortFlagTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        harness.SetFTState(StateDescription(StateName::StandByWithServiceTypeRegistration, 1));
        harness.SetRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaReopen, 100, 2);

        harness.PerformLocalRetry();

        harness.Verify(StateDescription(StateName::StandByWithServiceTypeRegistration, 1));
        harness.VerifyReplicaReopenMessage(ReplicaReopenType::FinalRetry);
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::FailureToIncrementUsageCountCausesRetryAtReopenTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        auto initState = StateDescription(StateName::StandByWithoutServiceTypeRegistration, 1);
        harness.SetFTState(initState);
        harness.SetRetryableErrorStateThreshold(initState.GetRetryableErrorStateValue(), 5, 10);
        harness.SetFindServiceTypeRegistrationState(FindServiceTypeRegistrationError::CategoryName::Success);
        harness.SetIncrementUsageCountError();
        harness.PerformLocalRetry();

        harness.Verify(initState);
        harness.VerifyNoHealthEvent();
    }

    void TestStateMachine_ReplicaCreation::FailureInReopenIncrementsCountTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteReopenWithErrorTest(harness, ReplicaReopenReplyType::Failure, 5, 8, 8);

        harness.Verify(StateDescription(StateName::StandByWithServiceTypeRegistration, 6));
        harness.VerifyNoHealthEvent();
        harness.VerifyNoFMMessage();
    }

    void TestStateMachine_ReplicaCreation::FailureInReopenReportsHealthTestHelper(ScenarioName::Enum name)
    {
        auto msgType = ReplicaReopenReplyType::Failure;
        Harness harness(ScenarioDescription::Create(name));

        ExecuteReopenWithErrorTest(harness, msgType, 7, 8, 9);

        harness.Verify(StateDescription(StateName::StandByWithServiceTypeRegistration, 8));
        harness.VerifyWarningHealthEvent(msgType);
        harness.VerifyNoFMMessage();
    }

    void TestStateMachine_ReplicaCreation::FailureInReopenDropsReplicaTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteReopenWithErrorTest(harness, ReplicaReopenReplyType::FailureWithAbort, 8, 8, 9);

        harness.Verify(StateDescription(StateName::Dropped));
        harness.VerifyClosedHealthEvent();
        harness.VerifyFailureMessageAfterReplicaReopenFailure();
    }

    void TestStateMachine_ReplicaCreation::SuccessfulReopenAfterHealthClearsHealthReportTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteReopenWithErrorTest(harness, ReplicaReopenReplyType::Success, 8, 8, 10);

        harness.Verify(StateDescription(StateName::Open));
        harness.VerifyClearWarningHealthEvent();
        harness.VerifySuccessMessageSent();
    }

    void TestStateMachine_ReplicaCreation::SuccessfulReopenSendsResponseTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        ExecuteReopenWithErrorTest(harness, ReplicaReopenReplyType::Success, 4, 8, 9);

        harness.Verify(StateDescription(StateName::Open));
        harness.VerifyNoHealthEvent();
        harness.VerifySuccessMessageSent();
    }

    void TestStateMachine_ReplicaCreation::DeactivatedNodeStartsCloseAfterSuccessfulReopenTestHelper(ScenarioName::Enum name)
    {
        Harness harness(ScenarioDescription::Create(name));

        harness.SetFTState(StateDescription(StateName::StandByWithServiceTypeRegistration));
        harness.DeactivateNode();

        harness.PerformReplicaReopenReply(ReplicaReopenReplyType::Success);

        harness.Verify(StateDescription(StateName::Closing));
        harness.VerifyNoHealthEvent();
        harness.VerifyNoFMMessage();
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReplicaCreationSuite, TestStateMachine_ReplicaCreation)

    BOOST_AUTO_TEST_CASE(InitialFTCreationFromNullTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { FTCreationTestHelper(StateName::NonExistant, e); });
    }

    BOOST_AUTO_TEST_CASE(FTCreationFromEarlierClosedTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { FTCreationTestHelper(StateName::ClosedWithLowerInstanceId, e); });
    }

    BOOST_AUTO_TEST_CASE(RetryInICWithoutRegistrationIsNoOpTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ExternalRetryInICTestHelper(StateName::ICWithoutServiceTypeRegistration, e); });
    }

    BOOST_AUTO_TEST_CASE(RetryInICWithRegistrationIsNoOpTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ExternalRetryInICTestHelper(StateName::ICWithServiceTypeRegistration, e); });
    }

    BOOST_AUTO_TEST_CASE(FatalErrorFromFSRReturnsErrorToSenderAndClosesFTTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ErrorFromFSRReturnsErrorToSenderAndClosesFTTestHelper(e, FindServiceTypeRegistrationError::CategoryName::FatalErrorForOpen); });
    }

    BOOST_AUTO_TEST_CASE(UnknownErrorFromFSRReturnsErrorToSenderAndClosesFTTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ErrorFromFSRReturnsErrorToSenderAndClosesFTTestHelper(e, FindServiceTypeRegistrationError::CategoryName::UnknownErrorForOpen); });
    }

    BOOST_AUTO_TEST_CASE(FailureToIncrementUsageCountCausesRetryTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { FailureToIncrementUsageCountCausesRetryTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(RetryableErrorInFSRIncrementsCountIfBelowThresholdTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { RetryableErrorInFSRIncrementsCountIfBelowThresholdTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(ExceedThresholdAtFSRDropsReplicaTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ExceedThresholdAtFSRDropsReplicaTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(RetryableErrorInFSRIncrementsCountAndReportsWarningIfAtWarningThresholdTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { RetryableErrorInFSRIncrementsCountAndReportsWarningIfAtWarningThresholdTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(RetryableErrorInFSRDoesNotReportMultipleWarningsTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { RetryableErrorInFSRDoesNotReportMultipleWarningsTestHelper(e); });
    }
    BOOST_AUTO_TEST_CASE(SuccessAfterHealthReportWarningClearsHealthReportTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { SuccessAfterHealthReportWarningClearsHealthReportTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(ServiceTypeRegisteredSendsReplicaOpenMessageTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ServiceTypeRegisteredSendsReplicaOpenMessageTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(SuccessAtFSRSendsMessageToRAPTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { SuccessAtFSRSendsMessageToRAPTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(RetryWithServiceTypeRegistrationSendsReplicaOpenMessage)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { RetryWithServiceTypeRegistrationSendsReplicaOpenMessageTestHelper(e); });
    }
    BOOST_AUTO_TEST_CASE(FinalRetryWithServiceTypeRegistrationSendsReplicaOpenMessageWithAbortTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { FinalRetryWithServiceTypeRegistrationSendsReplicaOpenMessageWithAbortTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(SuccessFromOpenSendsReplyBackToSourceTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { SuccessFromOpenSendsReplyBackToSourceTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(StateChangedOnDataLossFromAddPrimarySendsReplyBackTest)
    {
        StateChangedOnDataLossFromAddPrimarySendsReplyBackTestHelper(ScenarioName::StatefulPersistedAddPrimary);
        StateChangedOnDataLossFromAddPrimarySendsReplyBackTestHelper(ScenarioName::StatefulVolatileAddPrimary);
    }

    BOOST_AUTO_TEST_CASE(ErrorInOpenIncrementsCountIfBelowThresholdTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ErrorInOpenIncrementsCountIfBelowThresholdTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(ErrorInOpenDropsReplicaIfAtThresholdTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ErrorInOpenDropsReplicaIfAtThresholdTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(SuccessFromOpenReplyClearsWarningHealthReportTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { SuccessFromOpenReplyClearsWarningHealthReportTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(ExceedWarningThresholdReportsWarningTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { ExceedWarningThresholdReportsWarningTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(MultipleWarningsAreNotReportedAtOpenTest)
    {
        ExecuteAllOpenScenarios([this](ScenarioName::Enum e) { MultipleWarningsAreNotReportedAtOpenTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(DeactivatedNodeAtOpenReplyClosesTheReplicaImmediatelyOnFailureTest)
    {
        // TODO: Enable this later
        // ExecuteAllOpenScenarios([this] (ScenarioName::Enum e) { DeactivatedNodeAtOpenReplyClosesTheReplicaImmediatelyOnFailureTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(DeactivatedNodeAtOpenReplyClosesTheReplicaOnSuccessTest)
    {
        // TODO: Enable this later
        // ExecuteAllOpenScenarios([this] (ScenarioName::Enum e) { DeactivatedNodeAtOpenReplyClosesTheReplicaOnSuccessTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(RetryWithoutRegistrationIsNoOpForSBTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { RetryableOrUnknownErrorIsNoOpForSBTestHelper(e, FindServiceTypeRegistrationError::CategoryName::RetryableErrorForReopen); });
    }

    BOOST_AUTO_TEST_CASE(UnknownErrorWithoutRegistrationIsNoOpForSBTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { RetryableOrUnknownErrorIsNoOpForSBTestHelper(e, FindServiceTypeRegistrationError::CategoryName::UnknownErrorForReopen); });
    }

    BOOST_AUTO_TEST_CASE(FatalErrorWithoutRegistrationDropsReplicaForSBTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { FatalErrorWithoutRegistrationDropsReplicaForSBTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(ErrorCountExceedingThresholdReportsHealthForSBTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { ErrorCountExceedingThresholdReportsHealthForSBTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(MultipleWarningHealthReportsAreNotSentForSBTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { MultipleWarningHealthReportsAreNotSentForSBTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(ServiceTypeRegisteredSendsReopenMessageTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { ServiceTypeRegisteredSendsReopenMessageTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(SuccessFSRSendsReopenMessageAndUpdatesStateForSBTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { SuccessFSRSendsReopenMessageAndUpdatesStateForSBTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(SuccessFSRClearsHealthReportAndSendsReopenIfHealthWasReportedForSBTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { SuccessFSRClearsHealthReportAndSendsReopenIfHealthWasReportedForSBTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(FailureInFSRForSBAfterThresholdDropsReplicaTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { FailureInFSRForSBAfterThresholdDropsReplicaTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(ReopenMessageRetrySendsMessageWithoutAbortFlagTestHelper1)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { ReopenMessageRetrySendsMessageWithoutAbortFlagTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(FinalReopenMessageRetrySendsMessageWithAbortFlagTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { FinalReopenMessageRetrySendsMessageWithAbortFlagTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(FailureToIncrementUsageCountCausesRetryAtReopenTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { FailureToIncrementUsageCountCausesRetryAtReopenTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(FailureInReopenIncrementsCountTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { FailureInReopenIncrementsCountTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(FailureInReopenReportsHealthTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { FailureInReopenReportsHealthTestHelper(e); });
    }
    BOOST_AUTO_TEST_CASE(FailureInReopenDropsReplicaTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { FailureInReopenDropsReplicaTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(SuccessfulReopenAfterHealthClearsHealthReportTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { SuccessfulReopenAfterHealthClearsHealthReportTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(SuccessfulReopenSendsResponseTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { SuccessfulReopenSendsResponseTestHelper(e); });
    }

    BOOST_AUTO_TEST_CASE(DeactivatedNodeStartsCloseAfterSuccessfulReopenTest)
    {
        ExecuteAllReopenScenarios([this](ScenarioName::Enum e) { DeactivatedNodeStartsCloseAfterSuccessfulReopenTestHelper(e); });
    }

    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // End Namespace CreateReplicaTest

#pragma endregion

#pragma region StalenessChecks
class TestStateMachine_Staleness
{
protected:
    TestStateMachine_Staleness() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_Staleness() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

#pragma region Federation

#pragma region AddInstance
    void AddInstanceLocalReplicaInstanceIdTestHelper(wstring const & message, wstring const & replyMessage);
#pragma endregion

#pragma region AddPrimary
    void AddPrimaryLocalReplicaInstanceIdTestHelper(wstring const & message, wstring const & replyMessage);
#pragma endregion

#pragma region CreateReplica
    void CreateReplica_LocalReplicaInstanceIdGreaterAndFTIsClosedTestHelper(wstring const & input, wstring const & expectedReply);
#pragma endregion

#pragma endregion

#pragma region Configuration Message Body
    // In these tests the replica role etc is not really important
    // Staleness is determined based on replica instances and up/down
    void VerifyBodyIsNotStale(wstring const & ft, wstring const & messageBody);
    void VerifyBodyIsStale(wstring const & ft, wstring const & messageBody);
    void ConfigurationMessageBodyTestHelper(wstring const & ft, wstring const & messageBody, bool expected);
#pragma endregion

#pragma region Member Variables And Functions

    static const uint64 SenderNodeId = 2;

    template<MessageType::Enum T>
    void RunStaleMessageTest(
        wstring const & tag,
        wstring const & ftShortName,
        wstring const & messageBodyStr,
        wstring const & failoverUnitStr,
        JobItemType::Enum jobItemType = JobItemType::Dropped)
    {
        TestLog::WriteInfo(tag);

        ScenarioTestHolder holder;
        ScenarioTest* scenarioTest = &holder.ScenarioTestObj;

        scenarioTest->AddFT(ftShortName, failoverUnitStr);

        auto message = scenarioTest->ParseMessage<T>(ftShortName, messageBodyStr);
        auto metadata = scenarioTest->RA.MessageMetadataTable.LookupMetadata(message);

        GenerationHeader header = GenerationHeader::ReadFromMessage(*message);

        EntityJobItemBaseSPtr jobItem = scenarioTest->RA.MessageHandlerObj.Test_CreateFTJobItem(
                metadata,                                        
                move(message), 
                header, 
                Federation::NodeInstance(Federation::NodeId(LargeInteger(0, SenderNodeId)), 1));

        scenarioTest->RA.JobQueueManager.ScheduleJobItem(jobItem);

        scenarioTest->DrainJobQueues();

        if (jobItemType == JobItemType::Dropped)
        {
            VERIFY_ARE_EQUAL(false, jobItem->HandlerReturnValue);
        }
        else
        {
            VERIFY_ARE_EQUAL(true, jobItem->HandlerReturnValue);
        }

        scenarioTest->ValidateFT(ftShortName, failoverUnitStr);
    }

    template<MessageType::Enum T>
    void RunFinalReplicaOpenStaleMessageTest(
        wstring const & ftShortName,
        wstring const & messageBodyStr,
        wstring const & failoverUnitStr,
        RetryableErrorStateName::Enum rest)
    {
        ScenarioTestHolder holder;
        ScenarioTest* scenarioTest = &holder.ScenarioTestObj;

        scenarioTest->AddFT(ftShortName, failoverUnitStr);
        auto & ft = scenarioTest->GetFT(ftShortName);

        auto currentState = rest;
        int threshold = 5;
        ft.RetryableErrorStateObj.Test_Set(currentState, threshold - 1);
        ft.RetryableErrorStateObj.Test_SetThresholds(currentState, INT_MAX, threshold, INT_MAX, INT_MAX);

        auto message = scenarioTest->ParseMessage<T>(ftShortName, messageBodyStr);

        GenerationHeader header = GenerationHeader::ReadFromMessage(*message);

        auto jobItem = scenarioTest->RA.MessageHandlerObj.Test_CreateFTJobItem(
            scenarioTest->RA.MessageMetadataTable.LookupMetadata(message),
            move(message),
            GenerationHeader(),
            ReconfigurationAgent::InvalidNode);

        auto copy = jobItem;
        scenarioTest->RA.JobQueueManager.ScheduleJobItem(move(copy));

        scenarioTest->DrainJobQueues();

        VERIFY_ARE_EQUAL(true, jobItem->HandlerReturnValue);

        scenarioTest->ValidateFT(ftShortName, failoverUnitStr);
    }

#pragma endregion

};

bool TestStateMachine_Staleness::TestSetup()
{
    return true;
}

bool TestStateMachine_Staleness::TestCleanup()
{
    return true;
}

#pragma region Configuration Message Body

void TestStateMachine_Staleness::ConfigurationMessageBodyTestHelper(wstring const & ftString, wstring const & messageBody, bool expected)
{
    ScenarioTestHolder holder;
    ScenarioTest & scenarioTest = holder.ScenarioTestObj;

    scenarioTest.AddFT(L"SP1", ftString);

    FailoverUnit & ft = scenarioTest.GetFT(L"SP1");

    auto configBody = Reader::ReadHelper<ConfigurationMessageBody>(messageBody, Default::GetInstance().SP1_FTContext);

    bool actual = ft.IsConfigurationMessageBodyStale(configBody);

    wstring message = wformatString("Message: {0}\r\nFT: {1}\r\nExpected: {2}. Actual: {3}", configBody, ft, expected, actual);
    TestLog::WriteInfo(message);

    Verify::AreEqual(expected, actual);
}

void TestStateMachine_Staleness::VerifyBodyIsNotStale(wstring const & ft, wstring const & messageBody)
{
    ConfigurationMessageBodyTestHelper(ft, messageBody, false);
}

void TestStateMachine_Staleness::VerifyBodyIsStale(wstring const & ft, wstring const & messageBody)
{
    ConfigurationMessageBodyTestHelper(ft, messageBody, true);
}

#pragma endregion


void TestStateMachine_Staleness::AddInstanceLocalReplicaInstanceIdTestHelper(wstring const & message, wstring const & replyMessage)
{
    ScenarioTestHolder holder;
    ScenarioTest & context = holder.ScenarioTestObj;

    // FT with replica id 1 and instance 2
    context.AddFT(L"SL1", L"C None 000/000/411 1:2 -");

    context.ProcessFMMessageAndDrain<MessageType::AddInstance>(L"SL1", message);

    // validate that ReplicaUp is sent and addinstancereply are sent
    context.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {SL1 000/000/411 [N/N/N DD D 1:2 n]");
    context.ValidateFMMessage<MessageType::AddInstanceReply>(L"SL1", replyMessage);
}

void TestStateMachine_Staleness::AddPrimaryLocalReplicaInstanceIdTestHelper(wstring const & message, wstring const & replyMessage)
{
    ScenarioTestHolder holder;
    ScenarioTest & context = holder.ScenarioTestObj;

    // FT with replica id 1 and instance 2
    context.AddFT(L"SV1", L"C None 000/000/411 1:2 -");

    context.ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"SV1", message);

    // validate that replica dropped and addinstancereply are sent
    context.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {SV1 000/000/411 [N/N/N DD D 1:2 n]");
    context.ValidateFMMessage<MessageType::AddPrimaryReply>(L"SV1", replyMessage);
}

void TestStateMachine_Staleness::CreateReplica_LocalReplicaInstanceIdGreaterAndFTIsClosedTestHelper(wstring const & input, wstring const & expectedReply)
{
    ScenarioTestHolder holder;
    ScenarioTest & context = holder.ScenarioTestObj;

    // FT with replica id 1 and instance 2
    context.AddFT(L"SV1", L"C None 000/000/411 1:2 -");

    context.ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(1, L"SV1", input);

    context.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {SV1 000/000/411 [N/N/N DD D 1:2 n]");
    context.ValidateRAMessage<MessageType::CreateReplicaReply>(1, L"SV1", expectedReply);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_StalenessSuite, TestStateMachine_Staleness)

BOOST_AUTO_TEST_CASE(MessageWithLowerGenerationNumberIsDropped_FM)
{
    // A message less than receive generation must be dropped of the FM
    // FM Receive generation is 1. All others are three. Incoming message is 4.
    ScenarioTestHolder holder;
    ScenarioTest & scenarioTest = holder.ScenarioTestObj;

    scenarioTest.GenerationStateHelper.SetGeneration(GenerationNumberSource::FM, GenerationInfo(3, 1), GenerationInfo(5, 1), GenerationInfo(3, 1));
    scenarioTest.GenerationStateHelper.SetGeneration(GenerationNumberSource::FMM, GenerationInfo(3, 1), GenerationInfo(3, 1), GenerationInfo(3, 1));

    scenarioTest.AddFT(L"SP1", L"C None 000/000/411 1:1 -");
    scenarioTest.ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"SP1", L"g:1:4,fm 000/422 [N/P RD U 1:2]");
    scenarioTest.ValidateFT(L"SP1", L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(MessageWithLowerGenerationNumberIsDropped_FMM)
{
    // A message less than receive generation must be dropped of the FMM
    // FMM Receive generation is 1. All others are three. Incoming message is 4.
    ScenarioTestHolder holder;
    ScenarioTest & scenarioTest = holder.ScenarioTestObj;

    scenarioTest.GenerationStateHelper.SetGeneration(GenerationNumberSource::FM, GenerationInfo(3, 1), GenerationInfo(3, 1), GenerationInfo(3, 1));
    scenarioTest.GenerationStateHelper.SetGeneration(GenerationNumberSource::FMM, GenerationInfo(3, 1), GenerationInfo(5, 1), GenerationInfo(3, 1));

    scenarioTest.AddFT(L"FM", L"C None 000/000/411 1:1 -");
    scenarioTest.ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"FM", L"g:1:4,fmm 000/422 [N/P RD U 1:2]");
    scenarioTest.ValidateFT(L"FM", L"C None 000/000/411 1:1 -");
}

#pragma region Federation

#pragma region AddInstance

BOOST_AUTO_TEST_CASE(AddInstance_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::AddInstance>(
        L"MessageEpoch Test_Open FT",
        L"SL1",
        L"000/411 [N/N RD U 1:1]",
        L"O None 000/000/422 1:1 CM [N/N/N RD U N F 1:1");

    RunStaleMessageTest<MessageType::AddInstance>(
        L"MessageEpoch Test_Closed FT",
        L"SL1",
        L"000/411 [N/N RD U 1:1]",
        L"C None 000/000/422 1:1 -");
}

BOOST_AUTO_TEST_CASE(AddInstance_LocalReplicaInstanceIdGreaterThanMessage)
{
    // replica in message = 5:6
    // Local instance id = 1:7
    RunStaleMessageTest<MessageType::AddInstance>(
        L"ReplicaInstanceId Test Open FT",
        L"SL1",
        L"000/411 [N/N RD U 1:2]",
        L"O None 000/000/411 1:3 CM [N/N/N RD U N F 1:3]");

    RunStaleMessageTest<MessageType::AddInstance>(
        L"ReplicaInstanceId Closed FT",
        L"SL1",
        L"000/411 [N/N RD U 1:2]",
        L"C None 000/000/411 1:3 -",
        JobItemType::ReadOnly);
}


BOOST_AUTO_TEST_CASE(AddInstance_LocalReplicaInstanceIdGreaterThanMessageFTClosed)
{
    AddInstanceLocalReplicaInstanceIdTestHelper(L"000/411 [N/N RD U 1:1]", L"000/411 [N/N RD U 1:1] StaleRequest");
    AddInstanceLocalReplicaInstanceIdTestHelper(L"000/411 [N/N RD U 2:1]", L"000/411 [N/N RD U 2:1] StaleRequest");
}

BOOST_AUTO_TEST_CASE(AddInstance_LocalReplicaIsTransitioning)
{
    RunStaleMessageTest<MessageType::AddInstance>(
        L"ID FT",
        L"SL1",
        L"000/411 [N/N RD U 1:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/N RD U N F 1:1]");

    RunStaleMessageTest<MessageType::AddInstance>(
        L"IC FT",
        L"SL1",
        L"000/411 [N/N RD U 1:1]",
        L"O None 000/000/411 1:1 S [N/N/N IC U N F 1:1]");
}

#pragma endregion

#pragma region RemoveInstance

/* TODO: Disable temporarily for force delete
BOOST_AUTO_TEST_CASE(RemoveInstance_LocalReplicaIsTransitioning)
{
    RunStaleMessageTest<MessageType::RemoveInstance>(
        L"ID FT",
        L"SL1",
        L"000/411 [N/N RD U 1:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/N RD U N F 1:1]");
}
*/

#pragma endregion

#pragma region AddPrimary

BOOST_AUTO_TEST_CASE(AddPrimary_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::AddPrimary>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [P/P RD U 1:1]",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::AddPrimary>(
        L"MessageEpoch Test_Closed FT",
        L"SP1",
        L"000/411 [P/P RD U 1:1]",
        L"C None 000/000/412 1:1 -");
}

BOOST_AUTO_TEST_CASE(AddPrimary_LocalReplicaInstanceIdGreaterThanMessage)
{
    // replica in message = 1:6
    // Local instance id = 1:7
    RunStaleMessageTest<MessageType::AddPrimary>(
        L"ReplicaInstanceId Test Open FT",
        L"SP1",
        L"000/411 [P/P RD U 1:6]",
        L"O None 000/000/411 1:7 CM [N/N/P RD U N F 1:7]");

    RunStaleMessageTest<MessageType::AddPrimary>(
        L"ReplicaInstanceId Closed FT",
        L"SP1",
        L"000/411 [P/P RD U 1:6]",
        L"C None 000/000/411 1:7 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(AddPrimary_LocalReplicaIsTransitioning)
{
    RunStaleMessageTest<MessageType::AddPrimary>(
        L"Closing",
        L"SP1",
        L"000/411 [P/P RD U 1:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::AddPrimary>(
        L"IC",
        L"SP1",
        L"000/411 [P/P RD U 1:1]",
        L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
}


BOOST_AUTO_TEST_CASE(AddPrimary_LocalReplicaInstanceIdGreaterAndFTIsClosed)
{
    // Same replica id
    AddPrimaryLocalReplicaInstanceIdTestHelper(L"000/411 [N/N RD U 1:1]", L"000/411 [N/N RD U 1:1] StaleRequest");

    // Different replica id
    AddPrimaryLocalReplicaInstanceIdTestHelper(L"000/411 [N/N RD U 2:1]", L"000/411 [N/N RD U 2:1] StaleRequest");
}

#pragma endregion

#pragma region AddReplica

BOOST_AUTO_TEST_CASE(AddReplica_MessageEpochMismatch)
{
    // incoming epoch is less
    RunStaleMessageTest<MessageType::AddReplica>(
        L"MessageEpoch Lesser Test_Open FT",
        L"SP1",
        L"000/411 [I/I RD U 2:1]",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    // incoming epoch is more
    RunStaleMessageTest<MessageType::AddReplica>(
        L"MessageEpoch Greater Test_Open FT",
        L"SP1",
        L"000/412 [I/I RD U 2:1]",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(AddReplica_FTIsClosed)
{
    // incoming epoch matches
    RunStaleMessageTest<MessageType::AddReplica>(
        L"FT Is Closed",
        L"SP1",
        L"000/411 [N/I RD U 2:1]",
        L"C None 000/000/411 1:1 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(AddReplica_LocalReplicaNotOpen)
{
    RunStaleMessageTest<MessageType::AddReplica>(
        L"LocalReplica is Not Open",
        L"SP1",
        L"000/411 [I/I RD U 2:1]",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(AddReplica_LocalReplicaIsTransitioning)
{
    RunStaleMessageTest<MessageType::AddReplica>(
        L"LocalReplica is in close",
        L"SP1",
        L"000/411 [N/I RD U 2:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::AddReplica>(
        L"LocalReplica is IC",
        L"SP1",
        L"000/411 [N/I RD U 2:1]",
        L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(AddReplica_LocalReplicaStandBy)
{
    RunStaleMessageTest<MessageType::AddReplica>(
        L"LocalReplica is SB",
        L"SP1",
        L"000/411 [I/I RD U 2:1]",
        L"O None 000/000/411 1:1 CM [N/N/P SB U N F 1:1]",
        JobItemType::ReadOnly);
}

#pragma endregion

#pragma region RemoveReplica

BOOST_AUTO_TEST_CASE(RemoveReplica_MessageEpochMismatch)
{
    // incoming epoch is less
    RunStaleMessageTest<MessageType::RemoveReplica>(
        L"MessageEpoch Lesser Test_Open FT",
        L"SP1",
        L"000/411 [I/I RD U 2:1]",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    // incoming epoch is more
    RunStaleMessageTest<MessageType::RemoveReplica>(
        L"MessageEpoch Greater Test_Open FT",
        L"SP1",
        L"000/412 [I/I RD U 2:1]",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(RemoveReplica_FTIsClosed)
{
    // incoming epoch matches
    RunStaleMessageTest<MessageType::RemoveReplica>(
        L"FT Is Closed",
        L"SP1",
        L"000/411 [N/I RD U 2:1]",
        L"C None 000/000/411 1:1 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(RemoveReplica_LocalReplicaNotOpen)
{
    RunStaleMessageTest<MessageType::RemoveReplica>(
        L"LocalReplica is Not Open",
        L"SP1",
        L"000/411 [N/I RD U 2:1]",
        L"O None 000/000/411 1:1 - [N/N/P RD D N F 1:1]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(RemoveReplica_LocalReplicaIsTransitioning)
{
    RunStaleMessageTest<MessageType::RemoveReplica>(
        L"LocalReplica is In close",
        L"SP1",
        L"000/411 [N/I RD U 2:1]",
        L"O None 000/000/411 1:1 CHc [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::RemoveReplica>(
        L"LocalReplica is IC",
        L"SP1",
        L"000/411 [N/I RD U 2:1]",
        L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(RemoveReplica_LocalReplicaStandBy)
{
    RunStaleMessageTest<MessageType::RemoveReplica>(
        L"LocalReplica is SB",
        L"SP1",
        L"000/411 [I/I RD U 2:1]",
        L"O None 000/000/411 1:1 CM [N/N/P SB U N F 1:1]",
        JobItemType::ReadOnly);
}

#pragma endregion

#pragma region DoReconfiguration

BOOST_AUTO_TEST_CASE(DoReconfiguration_MessageEpochIsLessThanFTEpoch)
{
    // FT Open
    RunStaleMessageTest<MessageType::DoReconfiguration>(
        L"MessageEpoch is less_FTOpen",
        L"SP1",
        L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]",
        L"O None 000/000/413 1:1 CM [N/N/P RD U N F 1:1]");

    // FT Is Closed
    RunStaleMessageTest<MessageType::DoReconfiguration>(
        L"MessageEpoch is less_FTClosed",
        L"SP1",
        L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]",
        L"C None 000/000/413 1:1 -");
}

BOOST_AUTO_TEST_CASE(DoReconfiguration_LocalReplicaInstanceIdIsGreaterThanMessage)
{
    // Local replica instance id = 1:2
    RunStaleMessageTest<MessageType::DoReconfiguration>(
        L"Instance Id",
        L"SP1",
        L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]",
        L"O None 000/000/411 1:2 CM [N/N/P RD U N F 1:2]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(DoReconfiguration_LocalReplicaIsNotOpen)
{
    RunStaleMessageTest<MessageType::DoReconfiguration>(
        L"LocalReplicaNotOpen",
        L"SP1",
        L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]",
        L"O None 000/000/411 1:1 S [N/N/P SB U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(DoReconfiguration_LocalReplicaIsInTransition)
{
    RunStaleMessageTest<MessageType::DoReconfiguration>(
        L"LocalReplica in close",
        L"SP1",
        L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::DoReconfiguration>(
        L"LocalReplica IC",
        L"SP1",
        L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]",
        L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(DoReconfiguration_AbortPhase2_Retry)
{
    RunStaleMessageTest<MessageType::DoReconfiguration>(
        L"Retry of newer abort phase 2",
        L"SP1",
        L"411/433 [P/P RD U 1:1] [S/S SB U 2:1]",
        L"O Abort_Phase0_Demote 411/000/422 1:1 CM [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1]",
        JobItemType::ReadOnly);

    RunStaleMessageTest<MessageType::DoReconfiguration>(
        L"Retry of older reconfig message that caused abort phase 2 after it has been accepted",
        L"SP1",
        L"411/422 [P/S RD U 1:1] [S/P RD U 2:1] [S/S RD D 3:1]",
        L"O Abort_Phase0_Demote 411/000/422 1:1 CM [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S RD U N F 3:1]",
        JobItemType::ReadOnly);
}

#pragma endregion

#pragma region CreateReplica

BOOST_AUTO_TEST_CASE(CreateReplica_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::CreateReplica>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/I RD U 1:1]",
        L"O None 000/000/412 1:1 CM [N/N/S SB U N F 1:1]");

    RunStaleMessageTest<MessageType::CreateReplica>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/I RD U 1:1]",
        L"C None 000/000/412 1:1 -");
}

BOOST_AUTO_TEST_CASE(CreateReplica_LocalReplicaInstanceIdGreaterThanMessage)
{
    // replica in message = 5:6
    // Local instance id = 1:7
    RunStaleMessageTest<MessageType::CreateReplica>(
        L"ReplicaInstanceId Test Open FT",
        L"SP1",
        L"000/411 [N/I RD U 1:6]",
        L"O None 000/000/411 1:7 CM [N/N/S SB U N F 1:7]");

    RunStaleMessageTest<MessageType::CreateReplica>(
        L"ReplicaInstanceId Closed FT",
        L"SP1",
        L"000/411 [N/I RD U 1:6]",
        L"C None 000/000/411 1:7 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(CreateReplica_LocalReplicaIsInTransition)
{
    RunStaleMessageTest<MessageType::CreateReplica>(
        L"Closing",
        L"SP1",
        L"000/411 [N/I RD U 1:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::CreateReplica>(
        L"ID FT",
        L"SP1",
        L"000/411 [N/I RD U 1:1]",
        L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(CreateReplica_LocalReplicaInstanceIdGreaterAndFTIsClosed)
{
    CreateReplica_LocalReplicaInstanceIdGreaterAndFTIsClosedTestHelper(L"000/411 [N/I IB U 1:1]", L"000/411 [N/I IB U 1:1] StaleRequest");
    CreateReplica_LocalReplicaInstanceIdGreaterAndFTIsClosedTestHelper(L"000/411 [N/I IB U 2:1]", L"000/411 [N/I IB U 2:1] StaleRequest");
}

#pragma endregion

#pragma region GetLSN

BOOST_AUTO_TEST_CASE(GetLSN_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::GetLSN>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/S RD U 1:1]",
        L"O None 000/000/412 1:1 CM [N/N/S RD U N F 1:1]");

    RunStaleMessageTest<MessageType::GetLSN>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/S RD U 1:1]",
        L"C None 000/000/412 1:1 -");
}

BOOST_AUTO_TEST_CASE(GetLSN_LocalReplicaInstanceIdGreaterThanMessage)
{
    // replica in message = 5:6
    // Local instance id = 1:7
    RunStaleMessageTest<MessageType::GetLSN>(
        L"ReplicaInstanceId Test Open FT",
        L"SP1",
        L"000/411 [N/S RD U 1:6]",
        L"O None 000/000/411 1:7 CM [N/N/S RD U N F 1:7]");

    RunStaleMessageTest<MessageType::GetLSN>(
        L"ReplicaInstanceId Closed FT",
        L"SP1",
        L"000/411 [N/S RD U 1:6]",
        L"C None 000/000/411 1:7 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(GetLSN_LocalReplicaIsInTransition)
{
    RunStaleMessageTest<MessageType::GetLSN>(
        L"Inclose FT",
        L"SP1",
        L"000/411 [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/S RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(GetLSN_LocalReplicaIsInCreate)
{
    RunStaleMessageTest<MessageType::GetLSN>(
        L"IC FT",
        L"SP1",
        L"000/411 [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 S [N/N/S IC U N F 1:1]");
}

#pragma endregion

#pragma region Deactivate

BOOST_AUTO_TEST_CASE(Deactivate_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::Deactivate>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 false [N/S RD U 2:1]",
        L"O None 000/000/412 1:1 CM [N/N/S RD U N F 2:1]");

    RunStaleMessageTest<MessageType::Deactivate>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 false [N/S RD U 2:1]",
        L"C None 000/000/412 1:1 -");
}

BOOST_AUTO_TEST_CASE(Deactivate_LocalReplicaIsInTransition)
{
    RunStaleMessageTest<MessageType::Deactivate>(
        L"In close",
        L"SP1",
        L"000/411 false [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/S RD U N F 1:1]");

    RunStaleMessageTest<MessageType::Deactivate>(
        L"IC FT",
        L"SP1",
        L"000/411 false [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 S [N/N/S IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(Deactivate_LocalReplicaIsNotOpen)
{
    RunStaleMessageTest<MessageType::Deactivate>(
        L"NotOpen FT",
        L"SP1",
        L"000/411 false [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 S [N/N/S SB U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(Deactivate_LocalReplicaIsInCreate)
{
    RunStaleMessageTest<MessageType::Deactivate>(
        L"IC FT",
        L"SP1",
        L"000/411 false [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 CM [N/N/S IC U N F 1:1]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(Deactivate_ConfigBodySanityTest)
{
    // Regression test for #1516714
    RunStaleMessageTest<MessageType::Deactivate>(
        L"ConfigBody",
        L"SP1",
        L"000/411 false [P/I RD U 1:1] [S/S RD D 2:1]",
        L"O None 000/000/411 1:1 CM [I/I/I RD U N F 1:1] [N/N/S SB U N F 2:2]",
        JobItemType::ReadOnly);
}

#pragma endregion

#pragma region Activate

BOOST_AUTO_TEST_CASE(Activate_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::Activate>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/S RD U 2:1]",
        L"O None 000/000/412 1:1 CM [N/N/S RD U N F 2:1]");

    RunStaleMessageTest<MessageType::Activate>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/S RD U 2:1]",
        L"C None 000/000/412 1:1 -");
}

BOOST_AUTO_TEST_CASE(Activate_LocalReplicaIsInTransition)
{
    RunStaleMessageTest<MessageType::Activate>(
        L"InClose",
        L"SP1",
        L"000/411 [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 CMHc [N/N/S RD U N F 1:1]");

    RunStaleMessageTest<MessageType::Activate>(
        L"IC",
        L"SP1",
        L"000/411 [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 S [N/N/S IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(Activate_LocalReplicaIsNotOpen)
{
    RunStaleMessageTest<MessageType::Activate>(
        L"Not Open FT",
        L"SP1",
        L"000/411 [N/S RD U 1:1]",
        L"O None 000/000/411 1:1 S [N/N/S SB U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(Activate_LocalReplicaIsInCreate)
{
}

#pragma endregion

#pragma region GetLSNReply

BOOST_AUTO_TEST_CASE(GetLSNReply_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::GetLSNReply>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O Phase1_GetLSN 411/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(GetLSNReply_FTIsInIncorrectReconfigurationStage)
{
    FailoverUnitReconfigurationStage::Enum incorrectReconfigStages[] =
    {
        FailoverUnitReconfigurationStage::Phase2_Catchup,
        FailoverUnitReconfigurationStage::Phase3_Deactivate,
        FailoverUnitReconfigurationStage::Phase4_Activate,
        FailoverUnitReconfigurationStage::None,
        FailoverUnitReconfigurationStage::Abort_Phase0_Demote,
        FailoverUnitReconfigurationStage::Phase0_Demote
    };

    for (int i = 0; i < ARRAYSIZE(incorrectReconfigStages); i++)
    {
        wstring ft = wformatString("O {0} 411/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]", incorrectReconfigStages[i]);

        RunStaleMessageTest<MessageType::GetLSNReply>(
            wformatString("Incorrect reconfig stage {0}", incorrectReconfigStages[i]),
            L"SP1",
            L"411/412 [N/S RD U 2:1] Success",
            ft,
            JobItemType::ReadOnly);
    }
}

BOOST_AUTO_TEST_CASE(GetLSNReply_TargetReplicaInstanceIdMismatch)
{
    RunStaleMessageTest<MessageType::GetLSNReply>(
        L"TargetReplciaInstanceIdIsGreater_RemoteReplicaNotFound",
        L"SP1",
        L"000/412 [N/S RD U 3:1 1 1] Success",
        L"O Phase1_GetLSN 411/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    RunStaleMessageTest<MessageType::GetLSNReply>(
        L"TargetReplciaInstanceIdIsGreater_RemoteReplicaInstanceIdIsGreater",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O Phase1_GetLSN 411/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");

    RunStaleMessageTest<MessageType::GetLSNReply>(
        L"TargetReplciaInstanceIdIsGreater_RemoteReplicaInstanceIdIsGreater",
        L"SP1",
        L"000/411 [N/S RD U 2:3] Success",
        L"O Phase1_GetLSN 411/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");
}

BOOST_AUTO_TEST_CASE(GetLSNReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::GetLSNReply>(
        L"FTIsClosed",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"C None 000/000/411 1:1 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(GetLSNReply_LocalReplicaIsInTransition)
{
    RunStaleMessageTest<MessageType::GetLSNReply>(
        L"LocalReplicaIsInClose",
        L"SP1",
        L"000/412 [N/S RD U 2:1] Success",
        L"O Phase1_GetLSN 411/000/412 1:1 CMHc [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    RunStaleMessageTest<MessageType::GetLSNReply>(
        L"LocalReplicaIsInCreate",
        L"SP1",
        L"000/412 [N/S RD U 2:1] Success",
        L"O Phase1_GetLSN 411/000/412 1:1 S [N/N/P IC U N F 1:1] [N/N/S RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(GetLSNReply_LocalReplicaIsNotOpen)
{
    RunStaleMessageTest<MessageType::GetLSNReply>(
        L"LocalReplicaIsNotOpen",
        L"SP1",
        L"000/412 [N/S RD U 2:1] Success",
        L"O Phase1_GetLSN 411/000/412 1:1 - [N/N/P RD D N F 1:1] [N/N/S RD U N F 2:1]",
        JobItemType::ReadOnly);
}

#pragma endregion

#pragma region DeactivateReply

BOOST_AUTO_TEST_CASE(DeactivateReply_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::DeactivateReply>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"411/412 [N/S RD U 2:1] Success",
        L"O Phase3_Deactivate 411/411/413 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(DeactivateReply_TargetReplicaInstanceIdIsGreater)
{
    RunStaleMessageTest<MessageType::DeactivateReply>(
        L"Target Replica Not Found",
        L"SP1",
        L"411/412 [N/S RD U 3:1] Success",
        L"O Phase3_Deactivate 411/411/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    RunStaleMessageTest<MessageType::DeactivateReply>(
        L"Target Replica Id Is Greater",
        L"SP1",
        L"411/412 [N/S RD U 2:1] Success",
        L"O Phase3_Deactivate 411/411/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");
}

BOOST_AUTO_TEST_CASE(DeactivateReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::DeactivateReply>(
        L"FTIsClosed",
        L"SP1",
        L"411/412 [N/S RD U 2:1] Success",
        L"C None 000/000/412 1:1 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(DeactivateReply_LocalReplicaIsInClose)
{
    RunStaleMessageTest<MessageType::DeactivateReply>(
        L"LocalReplicaIsInDrop",
        L"SP1",
        L"411/412 [N/S RD U 2:1] Success",
        L"O Phase3_Deactivate 411/411/412 1:1 CMHc [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");

    RunStaleMessageTest<MessageType::DeactivateReply>(
        L"LocalReplicaIsInCreate",
        L"SP1",
        L"411/412 [N/S RD U 2:1] Success",
        L"O Phase3_Deactivate 411/411/412 1:1 S [N/N/P IC U N F 1:1] [N/N/S RD U N F 2:2]");
}

BOOST_AUTO_TEST_CASE(DeactivateReply_LocalReplicaIsNotOpen)
{
    RunStaleMessageTest<MessageType::DeactivateReply>(
        L"LocalReplicaIsNotOpen",
        L"SP1",
        L"411/412 [N/S RD U 2:1] Success",
        L"O Phase3_Deactivate 411/411/412 1:1 - [N/N/P SB D N F 1:1] [N/N/S RD U N F 2:1]",
        JobItemType::ReadOnly);
}

#pragma endregion

#pragma region ActivateReply

BOOST_AUTO_TEST_CASE(ActivateReply_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::ActivateReply>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");
}

BOOST_AUTO_TEST_CASE(ActivateReply_TargetReplicaInstanceIdIsGreater)
{
    RunStaleMessageTest<MessageType::ActivateReply>(
        L"Target Replica Id is invalid",
        L"SP1",
        L"000/411 [N/S RD U 3:1] Success",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:3]");

    RunStaleMessageTest<MessageType::ActivateReply>(
        L"Target replica id is greater",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");
}

BOOST_AUTO_TEST_CASE(ActivateReply_TargetReplicaMustBeInCorrectState)
{
    // RemoteReplica is DD is stale
    RunStaleMessageTest<MessageType::ActivateReply>(
        L"RemoteReplica is DD",
        L"SP1",
        L"000/411 [N/S SB U 2:1] Success",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S DD U RA A 2:1]",
        JobItemType::ReadOnly);

    RunStaleMessageTest<MessageType::ActivateReply>(
        L"RemoteReplica is RD, Incoming is SB",
        L"SP1",
        L"000/411 [N/S SB U 2:1] Success",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U RA A 2:1]",
        JobItemType::ReadOnly);

    RunStaleMessageTest<MessageType::ActivateReply>(
        L"RemoteReplica is SB, Incoming is RD",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S SB U RA A 2:1]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ActivateReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::ActivateReply>(
        L"FTIsClosed",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"C None 000/000/411 1:1 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ActivateReply_LocalReplicaIsInClose)
{
    RunStaleMessageTest<MessageType::ActivateReply>(
        L"LocalReplicaIsInClose",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");

    RunStaleMessageTest<MessageType::ActivateReply>(
        L"LocalReplicaIsInCreate",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1] [N/N/S RD U N F 2:2]");
}

BOOST_AUTO_TEST_CASE(ActivateReply_LocalReplicaIsNotOpen)
{
    RunStaleMessageTest<MessageType::ActivateReply>(
        L"LocalReplicaIsNotOpen",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1] [N/N/S RD U N F 2:1]",
        JobItemType::ReadOnly);
}

#pragma endregion

#pragma region CreateReplicaReply

BOOST_AUTO_TEST_CASE(CreateReplicaReply_MessageEpochLessThanFTEpoch)
{
    RunStaleMessageTest<MessageType::CreateReplicaReply>(
        L"MessageEpoch Test_Open FT",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/422 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(CreateReplicaReply_TargetReplicaInstanceIdIsGreater)
{
    RunStaleMessageTest<MessageType::CreateReplicaReply>(
        L"Target replica does not exist",
        L"SP1",
        L"000/411 [N/S RD U 3:1] Success",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:3]");

    RunStaleMessageTest<MessageType::CreateReplicaReply>(
        L"Target replica instance id is greater",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");
}

BOOST_AUTO_TEST_CASE(CreateReplicaReply_TargetReplicaIsDown)
{
    RunStaleMessageTest<MessageType::CreateReplicaReply>(
        L"Target replica is down",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD D N F 2:1]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(CreateReplicaReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::CreateReplicaReply>(
        L"FTIsClosed",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"C None 000/000/411 1:1 -",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(CreateReplicaReply_LocalReplicaIsInTransition)
{
    RunStaleMessageTest<MessageType::CreateReplicaReply>(
        L"LocalReplicaIsInClose",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    RunStaleMessageTest<MessageType::CreateReplicaReply>(
        L"LocalReplicaIsInCreate",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1] [N/N/S RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(CreateReplicaReply_LocalReplicaIsNotOpen)
{
    RunStaleMessageTest<MessageType::CreateReplicaReply>(
        L"LocalReplicaIsNotOpen",
        L"SP1",
        L"000/411 [N/S RD U 2:1] Success",
        L"O None 000/000/411 1:1 - [N/N/P RD D N F 1:1] [N/N/S RD U N F 2:1]",
        JobItemType::ReadOnly);
}
#pragma endregion

#pragma endregion

#pragma region IPC

#pragma region Report Fault

BOOST_AUTO_TEST_CASE(ReportFault_LocalReplicaInstanceIdGreaterThanIncoming)
{
    RunStaleMessageTest<MessageType::ReportFault>(
        L"FT instance id mismatch",
        L"SP1",
        L"000/411 [P/P RD U 1:1] transient",
        L"O None 000/000/411 1:2 CM [N/N/P RD U N F 1:2]");
}

BOOST_AUTO_TEST_CASE(ReportFault_LocalReplicaIsInClose)
{
    RunStaleMessageTest<MessageType::ReportFault>(
        L"LocalReplicaIsInClose",
        L"SP1",
        L"000/411 [N/P RD U 1:1] transient",
        L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::ReportFault>(
        L"LocalReplicaIsInClose",
        L"SV1",
        L"000/411 [N/P RD U 1:1] permanent",
        L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReportFault_LocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::ReportFault>(
        L"LocalReplicaIsDown",
        L"SP1",
        L"000/411 [N/P RD U 1:1] transient",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

#pragma endregion

#pragma region ReplicaOpenReply

BOOST_AUTO_TEST_CASE(ReplicaOpenReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"FT is closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(ReplicaOpenReply_LocalReplicaIsNotInCreate)
{
    RunStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"Local Replica IB",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CM [N/N/P IB U N F 1:1]",
        JobItemType::ReadOnly);

    RunStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");

    RunStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"Local Replica RD",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicaOpenReply_LocalReplicaInstanceIdIsNotExactMatch)
{
    //RunStaleMessageTest<MessageType::ReplicaOpenReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P IC U N F 1:1]",
    //    JobItemType::ReadOnly);

    RunStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"Instance Id lesser in msg",
        L"SP1",
        L"000/411 [P/P RD U 1:2] Success -",
        L"O None 000/000/411 1:3 CM [N/N/P IC U N F 1:3]");
}

BOOST_AUTO_TEST_CASE(ReplicaOpenReply_FTIsOpenAndLocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"Local Replica Is Down",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicaOpenReply_MessageEpochIsLesser)
{
    RunStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"Message Epoch lesser FT closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/412 1:1 -");
}

BOOST_AUTO_TEST_CASE(ReplicaOpenReply_RuntimeDown)
{
    // Regression Test for #1615958
    // If a stale ReplicaOpen arrives after the runtime has gone down it should be dropped
    // In the bug the FT was

    ScenarioTestHolder holder;
    ScenarioTest& scenarioTest = holder.ScenarioTestObj;

    scenarioTest.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/S SB U N F 1:1]");
    scenarioTest.ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, L"SP1", L"000/422 [N/I RD U 1:1]");
    scenarioTest.DumpFT(L"SP1");
    scenarioTest.ResetAll();
    scenarioTest.ProcessRuntimeClosedAndDrain(L"SP1");
    scenarioTest.ResetAll();

    scenarioTest.ProcessRAPMessageAndDrain<MessageType::ReplicaOpenReply>(L"SP1", L"000/422 [N/I RD U 1:1] GlobalLeaseLost -");

    scenarioTest.DumpFT(L"SP1");
}

BOOST_AUTO_TEST_CASE(ReplicaOpenReply_FTIsReadyToAbort)
{
    RunFinalReplicaOpenStaleMessageTest<MessageType::ReplicaOpenReply>(
        L"SL1",
        L"000/411 [N/N RD U 1:1] GlobalLeaseLost -",
        L"O None 000/000/411 1:1 MS [N/N/N IC U N F 1:1]",
        RetryableErrorStateName::ReplicaOpen);
}

#pragma endregion

#pragma region ReplicaCloseReply

BOOST_AUTO_TEST_CASE(ReplicaCloseReply_LocalReplicaInstanceIdIsGreaterThanMessage)
{
    //RunStaleMessageTest<MessageType::ReplicaCloseReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P IC U N F 1:1]",
    //    JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicaCloseReply_MessageEpochIsLesser)
{
    RunStaleMessageTest<MessageType::ReplicaCloseReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::ReplicaCloseReply>(
        L"Message Epoch lesser FT closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/412 1:1 -");
}

#pragma endregion

#pragma region Stateful Service Reopen Reply

BOOST_AUTO_TEST_CASE(StatefulServiceReopenReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::StatefulServiceReopenReply>(
        L"FT is closed",
        L"SP1",
        L"000/411 [P/P SB U 1:1] Success -",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(StatefulServiceReopenReply_LocalReplicaInstanceIdIsGreaterThanMessage)
{
    //RunStaleMessageTest<MessageType::StatefulServiceReopenReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
    //    JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(StatefulServiceReopenReply_LocalReplicaIsInDrop)
{
    RunStaleMessageTest<MessageType::StatefulServiceReopenReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P SB U 1:1] Success -",
        L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(StatefulServiceReopenReply_LocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::StatefulServiceReopenReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P SB U 1:1] Success -",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(StatefulServiceReopenReply_MessageEpochIsLesser)
{
    RunStaleMessageTest<MessageType::StatefulServiceReopenReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P SB U 1:1] Success -",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::StatefulServiceReopenReply>(
        L"Message Epoch lesser FT closed",
        L"SP1",
        L"000/411 [P/P SB U 1:1] Success -",
        L"C None 000/000/412 1:1 -");
}

BOOST_AUTO_TEST_CASE(StatefulServiceReopenReply_FTIsReadyToAbort)
{
    RunFinalReplicaOpenStaleMessageTest<MessageType::StatefulServiceReopenReply>(
        L"SP1",
        L"000/411 [N/P SB U 1:1] GlobalLeaseLost -",
        L"O None 000/000/411 1:1 MS [N/N/P SB U N F 1:1]",
        RetryableErrorStateName::ReplicaReopen);
}

BOOST_AUTO_TEST_CASE(StatefulServiceReopenReply_ReplicaIsNotSB)
{
    RunStaleMessageTest<MessageType::StatefulServiceReopenReply>(
        L"StaleReopenReply for FT where CreateReplica is being processed",
        L"SP1",
        L"000/411 [N/S SB U 1:1] Success -",
        L"O None 000/000/411 1:1 CMS [S/N/I IC U N F 1:1]",
        JobItemType::ReadOnly);
}

#pragma endregion

#pragma region UpdateConfigurationReply

BOOST_AUTO_TEST_CASE(UpdateConfigurationReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::UpdateConfigurationReply>(
        L"FT is closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(UpdateConfigurationReply_LocalReplicaInstanceIdIsGreaterThanMessage)
{
    //RunStaleMessageTest<MessageType::UpdateConfigurationReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
    //    JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(UpdateConfigurationReply_LocalReplicaIsInDrop)
{
    RunStaleMessageTest<MessageType::UpdateConfigurationReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(UpdateConfigurationReply_LocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::UpdateConfigurationReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CM [N/N/P RD D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(UpdateConfigurationReply_MessageEpochIsLesser)
{
    RunStaleMessageTest<MessageType::UpdateConfigurationReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::UpdateConfigurationReply>(
        L"Message Epoch lesser FT closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/412 1:1 -");
}

#pragma endregion

#pragma region ReplicatorBuildIdleReplicaReply

BOOST_AUTO_TEST_CASE(ReplicatorBuildIdleReplicaReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
        L"FT is closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(ReplicatorBuildIdleReplicaReply_LocalReplicaInstanceIdIsGreaterThanMessage)
{
    //RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
    //    JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicatorBuildIdleReplicaReply_LocalReplicaIsInDrop)
{
    RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicatorBuildIdleReplicaReply_LocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicatorBuildIdleReplicaReply_RemoteReplicaInstanceIdMismatch)
{
    // remote instance id greater
    //RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
    //    L"Remote instance id greater",
    //    L"000/411 [P/P RD U 1:1] [N/I RD U 2:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I RD U N F 2:3]");

    /*RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
    L"Remote instance id lesser",
    L"000/411 [P/P RD U 1:1] [N/I RD U 2:2] Success -",
    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I RD U N F 2:1]");*/
}

BOOST_AUTO_TEST_CASE(ReplicatorBuildIdleReplicaReply_RemoteReplicaDoesNotExist)
{
    RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
        L"Remote replica does not exist",
        L"SP1",
        L"000/411 [P/P RD U 1:1] [N/I RD U 2:2] Success -",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicatorBuildIdleReplicaReply_RemoteReplicaIsDown)
{
    RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
        L"Remote replica is down",
        L"SP1",
        L"411/522 [S/P RD U 1:1] [S/I RD U 2:2] Success -",
        L"O Phase2_Catchup 411/000/522 1:1 CM [S/N/P RD U N F 1:1] [S/N/S IB D N R 2:2]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicatorBuildIdleReplicaReply_MessagePrimaryEpochIsLesser)
{
    RunStaleMessageTest<MessageType::ReplicatorBuildIdleReplicaReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/422 1:1 CM [N/N/P RD U N F 1:1]");
}

#pragma endregion

#pragma region ReplicatorRemoveIdleReplicaReply

BOOST_AUTO_TEST_CASE(ReplicatorRemoveIdleReplicaReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
        L"FT is closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(ReplicatorRemoveIdleReplicaReply_LocalReplicaInstanceIdIsGreaterThanMessage)
{
    //RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
    //    JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicatorRemoveIdleReplicaReply_LocalReplicaIsInDrop)
{
    RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicatorRemoveIdleReplicaReply_LocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicatorRemoveIdleReplicaReply_RemoteReplicaInstanceIdMismatch)
{
    // remote instance id greater
    //RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
    //    L"Remote instance id greater",
    //    L"000/411 [P/P RD U 1:1] [N/I RD U 2:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I RD U N F 2:3]");

    RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
        L"Remote instance id lesser",
        L"SP1",
        L"000/411 [P/P RD U 1:1] [N/I RD U 2:1] Success -",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I RD U N F 2:2]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicatorRemoveIdleReplicaReply_RemoteReplicaDoesNotExist)
{
    RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
        L"Remote replica does not exist",
        L"SP1",
        L"000/411 [P/P RD U 1:1] [N/I RD U 2:2] Success -",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicatorRemoveIdleReplicaReply_MessageEpochIsLesser)
{
    RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::ReplicatorRemoveIdleReplicaReply>(
        L"Message Epoch lesser FT closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/412 1:1 -");
}

#pragma endregion

#pragma region ReplicatorGetStatusReply

BOOST_AUTO_TEST_CASE(ReplicatorGetStatusReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::ReplicatorGetStatusReply>(
        L"FT is closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(ReplicatorGetStatusReply_LocalReplicaIsInDrop)
{
    RunStaleMessageTest<MessageType::ReplicatorGetStatusReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicatorGetStatusReply_LocalReplicaInstanceIdIsNotExactMatch)
{
    //RunStaleMessageTest<MessageType::ReplicatorGetStatusReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
    //    JobItemType::ReadOnly);

    RunStaleMessageTest<MessageType::ReplicatorGetStatusReply>(
        L"Instance Id lesser in msg",
        L"SP1",
        L"000/411 [P/P RD U 1:2] Success -",
        L"O None 000/000/411 1:3 CM [N/N/P RD U N F 1:3]");
}

BOOST_AUTO_TEST_CASE(ReplicatorGetStatusReply_FTIsOpenAndLocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::ReplicatorGetStatusReply>(
        L"Local Replica Is Down",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicatorGetStatusReply_MessageEpochIsLesser)
{
    RunStaleMessageTest<MessageType::ReplicatorGetStatusReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::ReplicatorGetStatusReply>(
        L"Message Epoch lesser FT closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/412 1:1 -");
}

BOOST_AUTO_TEST_CASE(ReplicatorGetStatusReply_LocalReplicaRoleIsPrimary)
{
    // Regression test for 1784404

    ScenarioTestHolder holder;
    ScenarioTest & scenarioTest = holder.ScenarioTestObj;

    scenarioTest.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    scenarioTest.ProcessRAPMessageAndDrain<MessageType::ReplicatorGetStatusReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success -");

    scenarioTest.ValidateNoRAMessage();

    scenarioTest.ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");
}

#pragma endregion

#pragma region ReplicatorUpdateEpochAndGetStatusReply

BOOST_AUTO_TEST_CASE(ReplicatorUpdateEpochAndGetStatusReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(
        L"FT is closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(ReplicatorUpdateEpochAndGetStatusReply_LocalReplicaInstanceIdIsGreaterThanMessage)
{
    //RunStaleMessageTest<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
    //    JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(ReplicatorUpdateEpochAndGetStatusReply_LocalReplicaIsInDrop)
{
    RunStaleMessageTest<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicatorUpdateEpochAndGetStatusReply_LocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicatorUpdateEpochAndGetStatusReply_MessageEpochIsLesser)
{
    RunStaleMessageTest<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(
        L"Message Epoch lesser FT closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/412 1:1 -");
}

#pragma endregion

#pragma region CancelCatchupReplicaSetReply

BOOST_AUTO_TEST_CASE(CancelCatchupReplicaSetReply_FTIsClosed)
{
    RunStaleMessageTest<MessageType::CancelCatchupReplicaSetReply>(
        L"FT is closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(CancelCatchupReplicaSetReply_LocalReplicaInstanceIdIsGreaterThanMessage)
{
    //RunStaleMessageTest<MessageType::CancelCatchupReplicaSetReply>(
    //    L"Instance Id greater in msg",
    //    L"000/411 [P/P RD U 1:2] Success -",
    //    L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
    //    JobItemType::ReadOnly);
}

BOOST_AUTO_TEST_CASE(CancelCatchupReplicaSetReply_LocalReplicaIsInDrop)
{
    RunStaleMessageTest<MessageType::CancelCatchupReplicaSetReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(CancelCatchupReplicaSetReply_LocalReplicaIsDown)
{
    RunStaleMessageTest<MessageType::CancelCatchupReplicaSetReply>(
        L"Local Replica ID",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(CancelCatchupReplicaSetReply_MessageEpochIsLesser)
{
    RunStaleMessageTest<MessageType::CancelCatchupReplicaSetReply>(
        L"Message Epoch lesser FT open",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1]");

    RunStaleMessageTest<MessageType::CancelCatchupReplicaSetReply>(
        L"Message Epoch lesser FT closed",
        L"SP1",
        L"000/411 [P/P RD U 1:1] Success -",
        L"C None 000/000/412 1:1 -");
}

#pragma endregion

#pragma endregion // IPC

BOOST_AUTO_TEST_CASE(NewReplicasAreNotStale)
{
    VerifyBodyIsNotStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"000/411 [N/P RD U 1:1] [N/S RD U 2:1]");
}

BOOST_AUTO_TEST_CASE(ReplicaWithLowerInstanceIsStale)
{
    VerifyBodyIsStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:2]",
        L"000/411 [N/P RD U 1:1]");

    VerifyBodyIsStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD D N F 1:2]",
        L"000/411 [N/P RD U 1:1]");

    VerifyBodyIsStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:2]",
        L"000/411 [N/P RD D 1:1]");

    VerifyBodyIsStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD D N F 1:2]",
        L"000/411 [N/P RD D 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicaWithEqualInstance)
{
    VerifyBodyIsNotStale(
        L"O None 000/000/411 1:2 CM [N/N/P RD U N F 1:2]",
        L"000/411 [N/P RD U 1:2]");

    VerifyBodyIsStale(
        L"O None 000/000/411 1:2 CM [N/N/P RD D N F 1:2]",
        L"000/411 [N/P RD U 1:2]");

    VerifyBodyIsNotStale(
        L"O None 000/000/411 1:2 CM [N/N/P RD U N F 1:2]",
        L"000/411 [N/P RD D 1:2]");

    VerifyBodyIsNotStale(
        L"O None 000/000/411 1:2 CM [N/N/P RD D N F 1:2]",
        L"000/411 [N/P RD D 1:2]");
}

BOOST_AUTO_TEST_CASE(ReplicaWithHigherInstance)
{
    VerifyBodyIsNotStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"000/411 [N/P RD U 1:2]");

    VerifyBodyIsNotStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD D N F 1:1]",
        L"000/411 [N/P RD U 1:2]");

    VerifyBodyIsNotStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"000/411 [N/P RD D 1:2]");

    VerifyBodyIsNotStale(
        L"O None 000/000/411 1:1 CM [N/N/P RD D N F 1:1]",
        L"000/411 [N/P RD D 1:2]");
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion //Staleness

#pragma region Reconfiguration

class TestStateMachine_Reconfiguration
{
protected:
    TestStateMachine_Reconfiguration() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_Reconfiguration() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

public:
    void SwapPrimary_CancelCatchup_ResetReplicatorUpdateConfiguration();
    void SwapPrimary_UpdateReplicatorConfigurationShouldBeReset();

    void ActivateMessageReceivedWhileRAPReplyPendingForOtherMessage();
    void DeactivateMessageReceivedForStandByReplica();

    void UpdateReplicatorConfigurationIsNotSetIfDoReconfigurationCompletesTheReconfiguration();
    void CatchupCompleteInSwapPrimaryResetsUpdateReplicatorConfiguration();
    void ErrorOnCatchupCompleteInSwapPrimaryDoesNotResetUpdateReplicatorConfiguration();
    void SwapPrimaryDoesNotResetUpdateReplicatorConfigurationIfItIsNotCatchupComplete();

    void ActivateAfterReplicatorGetStatusIsDropped();

    void DroppedIdleSendsReplicatorRemoveReplica();

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_Reconfiguration::TestSetup()
{
    scenarioTest_ = ScenarioTest::Create();

    return true;
}

bool TestStateMachine_Reconfiguration::TestCleanup()
{
    return scenarioTest_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReconfigurationSuite, TestStateMachine_Reconfiguration)

BOOST_AUTO_TEST_CASE(ActivateClearsSenderNode)
{
    /*
        0. I/S promotes this replica
        1. This replica becomes primary
        2. Swap primary and then a stale uc reply
    */

    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/I IB U N F 1:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::Activate>(2, L"SP1", L"411/412 [P/P RD U 2:1] [I/S RD U 1:1]");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/412 [I/S RD U 1:1] [P/P RD U 2:1] [I/S RD U 1:1] Success -");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/412 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");
    Verify::AreEqual(scenarioTest_->GetFT(L"SP1").SenderNode, ReconfigurationAgent::InvalidNode, L"SenderNode");
}


BOOST_AUTO_TEST_CASE(SwapPrimary_DemoteCompleted_UCReply)
{
    /*
        1. During swap primary demote has completed on the old primary but there is a stale UC reply pending
        2. UC reply arrives
        3. primary should be no op
    */

    scenarioTest_->AddFT(L"SP1", L"O None 411/000/422 1:1 CM [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1] [P/P RD U 1:1] [S/S RD U 2:1] Success -");

    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/422 1:1 CM [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(SwapPrimary_CancelCatchup_ResetReplicatorUpdateConfiguration)
{
    // If the update configuration flag is set on the FT while an abort catchup is pending during swap primary,
    // then the flag should be reset on receiving the reply from RAP, defect#:1236424

    scenarioTest_->AddFT(L"SP1", L"O Abort_Phase0_Demote 411/000/422 1:1 BCM [P/N/S RD U N 0 1:1] [S/N/S DD D N 0 2:1] [S/N/P RD U N 0 3:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::CancelCatchupReplicaSetReply>(L"SP1", L"411/422 [P/S RD U 1:1] Success");

    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/422 1:1 CM [P/N/P RD U N 0 1:1] [S/N/S DD D N 0 2:1] [S/N/S RD U N 0 3:1]");
}

BOOST_AUTO_TEST_CASE(SwapPrimary_ChangeConfigurationNotPerformed)
{
    // During SwapPrimary, ChangeConfiguration will not be performed if a replica exists with better catchup capability
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1] [N/N/S RD U N F 3:1] [N/N/S RD U N F 4:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [S/P RD U 1:1] [P/S RD U 2:1] [S/S RD U 3:1] [S/S RD U 4:1] 10");
    scenarioTest_->DumpFT(L"SP1");

    // Replica one node 1 doesn't have catch up capability
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(L"SP1", L"411/422 [P/S RD U 1:1 0 2] Success");
    scenarioTest_->DumpFT(L"SP1");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(4, L"SP1", L"411/422 {411:0} [N/S RD U 4:1 1 1] Success");
    scenarioTest_->DumpFT(L"SP1");
    
    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(2, L"SP1", L"411/422 {411:0} [N/P RD U 2:1 2 2] Success");
    scenarioTest_->DumpFT(L"SP1");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, L"SP1", L"411/422 {411:0} [N/S RD U 3:1 2 2] Success");
    scenarioTest_->DumpFT(L"SP1");

    // Verify that RA sends Deactivate message to node 4, instead of performing ChangeConfiguration
    scenarioTest_->ValidateRAMessage<MessageType::Deactivate>(4);
}


BOOST_AUTO_TEST_CASE(SwapPrimary_UpdateReplicatorConfigurationShouldBeReset)
{
    // The UpdateReplicatorConfiguration flag should be reset when RA gets a UC Reply from RAP
    // and the replicator configuration matches the RA configuration
    // Regression test for: #1186742
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1] [S/S RD U 3:1]");

    scenarioTest_->ValidateFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CMN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S RD U N F 3:1]");
    scenarioTest_->ResetAll();

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/P RD U 2:1] [S/S RD D 3:1]");

    // Updated message should be sent to RAP for catchup
    scenarioTest_->ValidateRAPMessage<MessageType::UpdateConfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [P/S RD U 1:1] [S/P RD U 2:1] [S/S RD D 3:1] C");
    scenarioTest_->ResetAll();

    // Let UC Reply for the first message come
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1] [P/P RD U 1:1] [S/S RD U 2:1] [S/S RD U 3:1] Success");

    // Update repliator config should still be set
    scenarioTest_->ValidateFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CMBN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S RD D N F 3:1]");
    scenarioTest_->ResetAll();

    // update configuration reply from RAP. now configuration is updated so the flag should be reset
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1] [P/P RD U 1:1] [S/S RD U 2:1] [S/S RD D 3:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CMN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S RD D N F 3:1]");
}

BOOST_AUTO_TEST_CASE(DeactivateMessageReceivedForStandByReplica)
{
    // Regression test for bug: 1456036

    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 2:1 CM [N/N/S SB U N F 2:1] [N/N/P RD U N F 1:1] [N/N/S RD U N F 3:1] [N/N/S RD U N F 4:1] [N/N/S RD U N F 5:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(1, L"SP1", L"411/412 false [S/I SB U 2:1] [P/I RD D 1:1] [S/S RD D 3:1] [S/S RD D 4:1] [S/P RD U 5:1]");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/412 2:1 CM [N/N/I SB U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(UpdateReplicatorConfigurationIsNotSetIfDoReconfigurationCompletesTheReconfiguration)
{
    // Regression test for bug#: 1461235

    scenarioTest_->AddFT(L"SP1", L"O Phase4_Activate 411/411/422 1:1 CMN [P/S/S RD U N F 1:1] [S/S/S RD U N F 2:1] [S/S/S RD U RA F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [P/S RD U 1:1] [S/S RD U 2:1] [S/S RD D 3:1]");

    // UpdateReplicatorConfig should not be set because the reconfig has been completed by the message above
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/422 1:1 CM [N/N/S RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD D N F 3:1]");
}

BOOST_AUTO_TEST_CASE(CatchupCompleteInSwapPrimaryResetsUpdateReplicatorConfiguration)
{
    // Regression test for bug#: 1559085
    // 1. Swap primary with three replicas
    // 2. Catchup All completes but UC Reply is slow
    // 3. One of the replicas is dropped so RA sets UpdateReplicatorConfiguration to true on DoReconfiguration message retry
    // 4. UC Reply comes in. At this time catchup all has completed and the replicator has transitioned to secondary but UpdateReplicatorConfiguration is true because in the reply the replica is Up and Ready (it was created before the replica dropped message came in to RA)

    scenarioTest_->AddFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CMBN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");

    // Slow UpdateConfiguration Reply
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1 2 2] [P/S RD U 1:1 2 2] [S/S RD U 2:1] [S/S RD U 3:1] Success C");

    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/422 {411:0} 1:1 CMN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");
}

BOOST_AUTO_TEST_CASE(UCReplyInSwapShouldNotCompleteReconfig)
{
    /*
        Similar to above
        1. Swap primay
        2. UC reply with catchup arrives and transitions to None
        3. UC reply then arrives again and should be treated as stale
    */
    scenarioTest_->AddFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CMN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1]");

    // Slow UpdateConfiguration Reply
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1 2 2] [P/P RD U 1:1] [S/S RD U 2:1] Success C");
    scenarioTest_->DumpFT(L"SP1");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1 2 2] [P/P RD U 1:1] [S/S RD U 2:1] Success -");

    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/422 {411:0} 1:1 CMN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(ErrorOnCatchupCompleteInSwapPrimaryDoesNotResetUpdateReplicatorConfiguration)
{
    scenarioTest_->AddFT(L"SP1", L"O Phase2_Catchup 411/000/422 {411:0} 1:1 CMB [P/N/P RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");

    // Slow UpdateConfiguration Reply
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1] [S/S RD U 2:1] [S/S RD U 3:1] GlobalLeaseLost C");

    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 411/000/422 {411:0} 1:1 CMB [P/N/P RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");
}

BOOST_AUTO_TEST_CASE(SwapPrimaryDoesNotResetUpdateReplicatorConfigurationIfItIsNotCatchupComplete)
{
    scenarioTest_->AddFT(L"SP1", L"O Phase2_Catchup 411/000/422 {411:0} 1:1 CMBN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1] [S/S RD U 2:1] [S/S RD U 3:1] Success");

    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 411/000/422 {411:0} 1:1 CMBN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");
}

BOOST_AUTO_TEST_CASE(DroppedIdleSendsReplicatorRemoveReplica)
{
    // Regression test for 1776050
    // RA has an Idle RD replica
    // FM sends DoReconfiguration from I/S RD U which gets dropped
    // FM sends DoReconfiguration from I/S DD D
    // RA should remove the replica from idle list
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/I RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [S/S RD U 2:1] [I/S DD D 3:1]");
    scenarioTest_->ValidateRAPMessage<MessageType::ReplicatorRemoveIdleReplica>(L"SP1", L"411/412 [P/P RD U 1:1] [I/I RD D 3:1] -");

    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 411/000/412 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S RD U N F 2:1] [I/N/S RD D N R 3:1]");
}

BOOST_AUTO_TEST_CASE(ICIsSetInActivate)
{
    /*
        Regression test for 10280268

        ic must be set during phase4 because it tells the fm that deactivation is complete

        the fm will ignore cc if pc is set and there is no ic (because it is equivalent to getlsn and the cc epoch and set on the ra are not the same)

        on remote replicas, when activate is received the pc is cleared which makes the ic and the cc the same and 
        the fm will consider the ic/cc as the activated epoch
    */

    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I RD U N F 2:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [I/S RD U 2:1] Success C");

    scenarioTest_->ValidateFT(L"SP1", L"O Phase4_Activate 411/412/412 1:1 CMN [P/P/P RD U RAP F 1:1] [I/S/S RD U RA F 2:1]");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region AddPrimary

class TestStateMachine_AddPrimary
{
protected:
    TestStateMachine_AddPrimary() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_AddPrimary() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_AddPrimary::TestSetup()
{
    scenarioTest_ = ScenarioTest::Create();

    return true;
}

bool TestStateMachine_AddPrimary::TestCleanup()
{
    return scenarioTest_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_AddPrimarySuite, TestStateMachine_AddPrimary)

BOOST_AUTO_TEST_CASE(RAPReplicaOpenReply_ServiceTypeNotRegistered)
{
    // If a retryable error is returned from RAP during replica open, then RA should retry the message
    // and not send the reply back to FM (which would mark the replica as dropped), defect#:1201758
    scenarioTest_->AddFT(L"SP1", L"O None 411/000/422 1:1 MS [N/N/P IC U N 0 1:1]");
    scenarioTest_->GetFT(L"SP1").RetryableErrorStateObj.Test_Set(RetryableErrorStateName::ReplicaOpen, 1);

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaOpenReply>(L"SP1", L"411/422 [N/P RD U 1:1] FABRIC_E_SERVICE_TYPE_NOT_REGISTERED -");

    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/422 1:1 MS [N/N/P IC U N 0 1:1]");

    // This is the important validation to ensure no messages to FM are generated
    scenarioTest_->ValidateNoFMMessages();
}

BOOST_AUTO_TEST_CASE(RecreateOfReplicaOpenFailureShouldNotResultInReply)
{
    // Regression test for 1184423
    // Failure to open a persisted replica that was SB (not reopen but change role)
    // should result in a retry from FM and not that FM drops the replica
    // 1. FM sends AddPrimary
    // 2. RA sends Replica Open
    // 3. Replica reports fault
    // 4. RA reopens replica
    // 4. FM sends AddPrimary
    // 5. RA sends replica open
    // 6. Replica Open Fails (e.g. change role failure)
    // 7. RA should drop the replica and send replica dropped to FM
    scenarioTest_->UTContext.Hosting.AddFindServiceTypeExpectation(Default::GetInstance().SP1_SP1_STContext, ErrorCodeValue::Success);
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"SP1", L"411/422 [N/P RD U 1:1]");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReportFault>(L"SP1", L"411/422 [N/P RD U 1:1] transient");

    scenarioTest_->ValidateNoFMMessagesOfType<MessageType::AddPrimaryReply>();
    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"411/422 [N/P RD U 1:1] Success");
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(L"SP1", L"411/422 [N/P SB U 1:2] Success");
    scenarioTest_->ResetAll();

    // Set the error threshold to 1 to simulate the bug
    // Else this will simply retry
    scenarioTest_->GetFT(L"SP1").RetryableErrorStateObj.Test_SetThresholds(RetryableErrorStateName::ReplicaReopen, 100, 1, INT_MAX, INT_MAX);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"SP1", L"411/422 [N/P RD U 1:2]");
    scenarioTest_->ResetAll();

    scenarioTest_->DumpFT(L"SP1");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaOpenReply>(L"SP1", L"411/422 [N/P RD U 1:2] GlobalLeaseLost A");
    scenarioTest_->ValidateNoFMMessagesOfType<MessageType::AddPrimaryReply>();

    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/422 1:2 CMHa [N/N/P ID U N F 1:2]");
}

BOOST_AUTO_TEST_CASE(ReadyReplicaShouldSendBackReply)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"SP1", L"000/411 [N/P IB U 1:1]");

    scenarioTest_->ValidateFMMessage<MessageType::AddPrimaryReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success");
}

BOOST_AUTO_TEST_CASE(AddPrimaryIsDroppedIfNodeIsNotReady)
{
    auto & test = *scenarioTest_;

    test.DeactivateNode(1);

    test.ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"SP1", L"411/522 [N/P IB U 1:1]");

    test.ValidateFTIsNull(L"SP1");
    test.ValidateFMMessage<MessageType::AddPrimaryReply>();
}

BOOST_AUTO_TEST_CASE(AddInstanceIsDroppedIfNodeIsNotReady)
{
    auto & test = *scenarioTest_;

    test.DeactivateNode(1);

    test.ProcessFMMessageAndDrain<MessageType::AddInstance>(L"SL1", L"000/000 [N/N IB U 1:1]");

    test.ValidateFTIsNull(L"SL1");
    test.ValidateFMMessage<MessageType::AddInstanceReply>();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Remove Replica

class TestStateMachine_RemoveReplica
{
protected:
    TestStateMachine_RemoveReplica() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_RemoveReplica() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_RemoveReplica::TestSetup()
{
    scenarioTest_ = ScenarioTest::Create();
    return true;
}

bool TestStateMachine_RemoveReplica::TestCleanup()
{
    return scenarioTest_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_RemoveReplicaSuite, TestStateMachine_RemoveReplica)

BOOST_AUTO_TEST_CASE(RemoveReplicaSanityTest)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I IB U N F 2:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::RemoveReplica>(L"SP1", L"000/411 [N/I RD U 2:1]");
    scenarioTest_->ValidateRAPMessage<MessageType::ReplicatorRemoveIdleReplica>(L"SP1", L"000/411 [N/P RD U 1:1] [N/I DD U 2:1] -");

    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorRemoveIdleReplicaReply>(L"SP1", L"000/411 [N/P RD U 1:1] [N/I RD U 2:1] Success -");
    scenarioTest_->ValidateFMMessage<MessageType::RemoveReplicaReply>(L"SP1", L"000/411 [N/I RD U 2:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(RemoveSendsReplyIfReplicaNotFound)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::RemoveReplica>(L"SP1", L"000/411 [N/I RD U 2:1]");
    scenarioTest_->ValidateFMMessage<MessageType::RemoveReplicaReply>(L"SP1", L"000/411 [N/I RD U 2:1] Success");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(RemoveReplicaWithHigherReplicaIdUsesReplicaIdOnTheFTAndNotInTheMessage)
{
    // Regression test for #1732371
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I IB U N F 2:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::RemoveReplica>(L"SP1", L"000/411 [N/I RD U 2:2]");
    scenarioTest_->ValidateRAPMessage<MessageType::ReplicatorRemoveIdleReplica>(L"SP1", L"000/411 [N/P RD U 1:1] [N/I DD U 2:1] -");

    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorRemoveIdleReplicaReply>(L"SP1", L"000/411 [N/P RD U 1:1] [N/I RD U 2:1] Success -");
    scenarioTest_->ValidateFMMessage<MessageType::RemoveReplicaReply>(L"SP1", L"000/411 [N/I RD U 2:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Reopen

class TestStateMachine_Reopen
{
protected:
    TestStateMachine_Reopen() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_Reopen() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void ReopenNegativeTestHelper(wstring const & ft, wstring const & initial);

    void RestartNegativeTestHelper(wstring const & ft, wstring const & initial);

    void ReopenWaitsUntilNodeIsReadyTestHelper(wstring const & ftShortName, bool isFMNodeUpAckReceived, bool isFmmNodeUpAckReceived);

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_Reopen::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();
    return true;
}

bool TestStateMachine_Reopen::TestCleanup()
{
    holder_.reset();
    return true;
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReopenSuite, TestStateMachine_Reopen)


BOOST_AUTO_TEST_CASE(ReopenNegativeTest)
{
    // Reopen on Up replicas is No-OP
    ReopenNegativeTestHelper(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    // reopen on sl/sv replicas is no op
    ReopenNegativeTestHelper(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    ReopenNegativeTestHelper(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
}


BOOST_AUTO_TEST_CASE(RestartNegativeTest)
{
    // Restart on down replicas
    RestartNegativeTestHelper(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReopenWaitsUntilNodeIsReadyBeforeSendingMessage_NonFM)
{
    ReopenWaitsUntilNodeIsReadyTestHelper(L"FM", true, false);
}

BOOST_AUTO_TEST_CASE(ReopenWaitsUntilNodeIsReadyBeforeSendingMessage_FM)
{
    ReopenWaitsUntilNodeIsReadyTestHelper(L"SP1", false, true);
}

BOOST_AUTO_TEST_CASE(ReopenResetsLastMessageSendTimeForFT)
{
    GetScenarioTest().SetFindServiceTypeRegistrationSuccess(L"SP1");

    // If the replica restarts the replica up is sent regardless of the last message send time
    GetScenarioTest().UTContext.Config.PerNodeMinimumIntervalBetweenMessageToFM = TimeSpan::Zero;
    GetScenarioTest().AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    GetScenarioTest().ProcessAppHostClosedAndDrain(L"SP1");

    GetScenarioTest().ResetAll();
    GetScenarioTest().ProcessServiceTypeRegisteredAndDrain(L"SP1");

    GetScenarioTest().ValidateFT(L"SP1", L"O None 000/000/411 1:2 1 IMS [N/N/P SB U N F 1:2]");

    GetScenarioTest().ResetAll();
    GetScenarioTest().ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(L"SP1", L"000/411 [N/P SB U 1:2] Success -");

    GetScenarioTest().ValidateFMMessage<MessageType::ReplicaUp>(L"false false {SP1 000/000/411 [N/N/P SB U 1:2 -1 -1 (1.0:1.0:1)]} | ");

    GetScenarioTest().ResetAll();

    GetScenarioTest().ProcessAppHostClosedAndDrain(L"SP1");
    GetScenarioTest().ProcessServiceTypeRegisteredAndDrain(L"SP1");

    GetScenarioTest().ResetAll();
    GetScenarioTest().ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(L"SP1", L"000/411 [N/P SB U 1:3] Success -");
    GetScenarioTest().ValidateFMMessage<MessageType::ReplicaUp>(L"false false {SP1 000/000/411 [N/N/P SB U 1:3 -1 -1 (1.0:1.0:1)]} | ");
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

void TestStateMachine_Reopen::ReopenNegativeTestHelper(wstring const & ftShortName, wstring const & initial)
{
    ScenarioTest & scenarioTest = holder_->Recreate();

    scenarioTest.AddFT(ftShortName, initial);

    scenarioTest.SetFindServiceTypeRegistrationSuccess(ftShortName);

    scenarioTest.ExecuteJobItemHandler<JobItemContextBase>(ftShortName, [](HandlerParameters & handlerParameters, JobItemContextBase &)
    {
        handlerParameters.RA.ReopenDownReplica(handlerParameters);
        return true;
    });

    scenarioTest.ValidateNoRAPMessages();
    scenarioTest.ValidateFT(ftShortName, initial);
}

void TestStateMachine_Reopen::RestartNegativeTestHelper(wstring const & ftShortName, wstring const & initial)
{
    ScenarioTest & scenarioTest = holder_->Recreate();

    scenarioTest.AddFT(ftShortName, initial);

    scenarioTest.SetFindServiceTypeRegistrationSuccess(ftShortName);

    scenarioTest.ExecuteJobItemHandler<JobItemContextBase>(ftShortName, [](HandlerParameters & handlerParameters, JobItemContextBase &)
    {
        handlerParameters.RA.CloseLocalReplica(handlerParameters, ReplicaCloseMode::Restart, ActivityDescription::Empty);
        return true;
    });
    
    scenarioTest.ValidateNoRAPMessages();
    scenarioTest.ValidateFT(ftShortName, initial);
}

void TestStateMachine_Reopen::ReopenWaitsUntilNodeIsReadyTestHelper(wstring const & ftShortName, bool isFMNodeUpAckReceived, bool isFmmNodeUpAckReceived)
{
    GetScenarioTest().SetNodeUpAckProcessed(isFMNodeUpAckReceived, isFmmNodeUpAckReceived);
    GetScenarioTest().SetFindServiceTypeRegistrationSuccess(ftShortName);
    GetScenarioTest().AddFT(ftShortName, L"O None 000/000/411 1:1 SM [N/N/P RD U N F 1:1]");

    GetScenarioTest().RequestWorkAndDrain(GetScenarioTest().StateItemHelpers.ReplicaOpenRetryHelper);

    GetScenarioTest().ValidateNoMessages();
}

#pragma endregion

#pragma region Fabric Upgrade

class TestStateMachine_FabricUpgrade
{
protected:
    TestStateMachine_FabricUpgrade() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_FabricUpgrade() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void FabricUpgradeWaitsForCloseOfReplicasTest(std::wstring const & ft);
    void FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHosting(bool value);
    void FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHostingAtNodeUp(bool value);
    void UpgradeIsDroppedTestHelper(wstring const & upgradeMessage, wstring const & reply);
    template<MessageType::Enum TMsg>
    void MessageShouldBeDroppedIfRAIsNotReadyTestHelper(wstring const & message, bool isFM, bool isFmm)
    {
        scenarioTest_->SetNodeUpAckProcessed(isFM, isFmm);
        scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

        scenarioTest_->ProcessFMMessageAndDrain<TMsg>(message);
        VERIFY_IS_TRUE(scenarioTest_->RA.UpgradeMessageProcessorObj.Test_UpgradeMap.empty());
    }

    void UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(bool isFM, bool isFmm);

    void CancelShouldBeDroppedIfRAIsNotReadyTestHelper(bool isFM, bool isFmm);

    TEST_METHOD(Upgrade_TerminatesServiceHost);
    void SetupFTForEntityUpdateOnRollbackTest();
    void VerifyDeactivationInfoIsMarkedInvalid();
    void VerifyDeactivationInfoIsStillValid();
    void ProcessUpgradeMessageToVersionThatSupportsDeactivationInfo();
    void ProcessUpgradeMessageToVersionThatDoesNotSupportDeactivationInfo();
    void ProcessUpgradeMessage(wstring const & message);

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_FabricUpgrade::TestSetup()
{
    scenarioTest_ = ScenarioTest::Create();

    scenarioTest_->HostingHelper.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();

    return true;
}

bool TestStateMachine_FabricUpgrade::TestCleanup()
{
    return scenarioTest_->Cleanup();
}

void TestStateMachine_FabricUpgrade::FabricUpgradeWaitsForCloseOfReplicasTest(std::wstring const & ft)
{
    scenarioTest_->UTContext.Hosting.ValidateFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(true);

    scenarioTest_->AddFT(L"SP1", ft);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"6.0.0:cfg2:2 Rolling");

    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success");

    scenarioTest_->ResetAll();

    scenarioTest_->FireFabricUpgradeCloseCompletionCheckTimer(2);
    scenarioTest_->DrainJobQueues();

    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"6.0.0:cfg2:2");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]");

    scenarioTest_->ValidateFabricUpgradeIsCompleted(2);
}

void TestStateMachine_FabricUpgrade::FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHostingAtNodeUp(bool value)
{
    scenarioTest_->UTContext.Hosting.ValidateFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(value);

    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 2 2.0.0:cfg2:2", false);

    scenarioTest_->StateItemHelpers.ReliabilityHelper.ValidateNodeUpToFM();
    scenarioTest_->ValidateFabricUpgradeIsCompleted(2);

    bool shouldRestartReplicaValueToFabric = std::get<1>(scenarioTest_->StateItemHelpers.HostingHelper.HostingObj.UpgradeFabricAsyncApi.GetParametersAt(0));
    Verify::AreEqual(false, shouldRestartReplicaValueToFabric);
}

void TestStateMachine_FabricUpgrade::FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHosting(bool value)
{
    // regression test for #1405455
    scenarioTest_->UTContext.Hosting.ValidateFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(value);

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg2:2 Rolling");
    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"2.0.0:cfg2:2");
    scenarioTest_->ValidateFabricUpgradeIsCompleted(2);

    bool shouldRestartReplicaValueToFabric = std::get<1>(scenarioTest_->StateItemHelpers.HostingHelper.HostingObj.UpgradeFabricAsyncApi.GetParametersAt(0));
    Verify::AreEqual(value, shouldRestartReplicaValueToFabric);
}

void TestStateMachine_FabricUpgrade::UpgradeIsDroppedTestHelper(wstring const & upgradeMessage, wstring const & reply)
{
    scenarioTest_->UTContext.Hosting.DownloadFabricAsyncApi.SetCompleteAsynchronously();

    scenarioTest_->UTContext.NodeConfig.NodeVersion = Reader::ReadHelper<FabricVersionInstance>(L"2.0.0:cfg1:2");
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(upgradeMessage);
    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(reply);
}


void TestStateMachine_FabricUpgrade::UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(bool isFM, bool isFmm)
{
    MessageShouldBeDroppedIfRAIsNotReadyTestHelper<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg2:2 Rolling", isFM, isFmm);
}

void TestStateMachine_FabricUpgrade::CancelShouldBeDroppedIfRAIsNotReadyTestHelper(bool isFM, bool isFmm)
{
    MessageShouldBeDroppedIfRAIsNotReadyTestHelper<MessageType::CancelFabricUpgradeRequest>(L"3.0.0:cfg3:3 Rolling", isFM, isFmm);
}

void TestStateMachine_FabricUpgrade::ProcessUpgradeMessageToVersionThatSupportsDeactivationInfo()
{
    ProcessUpgradeMessage(L"5.5.5:cfg1:2 Rolling");
}

void TestStateMachine_FabricUpgrade::ProcessUpgradeMessageToVersionThatDoesNotSupportDeactivationInfo()
{
    ProcessUpgradeMessage(L"1.0.0:cfg1:2 Rolling");
}

void TestStateMachine_FabricUpgrade::ProcessUpgradeMessage(wstring const & message)
{
    auto & test = *scenarioTest_;
    test.ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(message);
}

void TestStateMachine_FabricUpgrade::SetupFTForEntityUpdateOnRollbackTest()
{
    auto & test = *scenarioTest_;
    test.UTContext.NodeConfig.NodeVersion = Reader::ReadHelper<FabricVersionInstance>(L"4.1.0:cfg:1");
    test.AddFT(L"SP1", L"C None 000/000/411 1:1 -");
    test.UTContext.Hosting.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();
}

void TestStateMachine_FabricUpgrade::VerifyDeactivationInfoIsMarkedInvalid()
{
    auto & test = *scenarioTest_;
    auto & ft = test.GetFT(L"SP1");
    Verify::IsTrue(ft.DeactivationInfo.IsInvalid, wformatString("DeactivationInfo {0}", ft));
}

void TestStateMachine_FabricUpgrade::VerifyDeactivationInfoIsStillValid()
{
    auto & test = *scenarioTest_;
    auto & ft = test.GetFT(L"SP1");
    Verify::IsTrue(ft.DeactivationInfo.IsDropped, wformatString("DeactivationInfo {0}", ft));
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_FabricUpgradeSuite, TestStateMachine_FabricUpgrade)

BOOST_AUTO_TEST_CASE(NormalFabricUpgradeTest)
{
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg2:2 Rolling");

    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"2.0.0:cfg2:2");

    scenarioTest_->ValidateFabricUpgradeIsCompleted(2);
}

BOOST_AUTO_TEST_CASE(FabricUpgradeWaitsForCloseOfReplicas)
{
    FabricUpgradeWaitsForCloseOfReplicasTest(L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(FabricUpgradeWaitsForCloseOfReplicas_IC)
{
    FabricUpgradeWaitsForCloseOfReplicasTest(L"O None 000/000/411 1:1 SM [N/N/P IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(FabricUpgradeWaitsForCloseOfReplicas_IC2)
{
    FabricUpgradeWaitsForCloseOfReplicasTest(L"O None 000/000/411 1:1 MS [N/N/P IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHostingAtNodeUp1)
{
    FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHostingAtNodeUp(true);
}

BOOST_AUTO_TEST_CASE(FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHostingAtNodeUp2)
{
    FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHostingAtNodeUp(false);
}

BOOST_AUTO_TEST_CASE(LowerInstanceIdUpgradeIsDropped)
{
    UpgradeIsDroppedTestHelper(L"2.0.0:cfg1:1 Rolling", L"2.0.0:cfg1:1");
}

BOOST_AUTO_TEST_CASE(EqualInstanceIdUpgradeIsDropped)
{
    UpgradeIsDroppedTestHelper(L"2.0.0:cfg1:2 Rolling", L"2.0.0:cfg1:2");
}

BOOST_AUTO_TEST_CASE(StalenessSanityChecks)
{
    // set this to false
    // if there is a bug and the RA actually does an upgrade it will cause this test to get stuck
    // because there will be an async op created by the hosting stub which is never completed
    scenarioTest_->UTContext.Hosting.SetAllAsyncApisToCompleteAsynchronously();

    // another message - still stale
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg1:1 Rolling");
    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"2.0.0:cfg1:1");

    // retry of this stale message
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg1:1 Rolling");
    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"2.0.0:cfg1:1");

    scenarioTest_->ResetAll();

    // now message which causes an upgrade
    scenarioTest_->UTContext.Hosting.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg2:2 Rolling");
    scenarioTest_->ValidateFabricUpgradeIsCompleted(2);
    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"2.0.0:cfg2:2");
}

BOOST_AUTO_TEST_CASE(FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHosting1)
{
    FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHosting(true);
}

BOOST_AUTO_TEST_CASE(FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHosting2)
{
    FabricUpgradePassesInCorrectValueOfShouldRestartReplicaToHosting(false);
}

BOOST_AUTO_TEST_CASE(FabricUpgradeDoesNotWaitForDownReplicas)
{
    scenarioTest_->UTContext.Hosting.ValidateFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(true);

    // Replica is down
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg2:2 Rolling");

    scenarioTest_->DrainJobQueues();

    scenarioTest_->ValidateNoRAPMessages();

    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"2.0.0:cfg2:2");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 {000:-1} 1:1 A [N/N/P SB D N F 1:1]");

    scenarioTest_->ValidateFabricUpgradeIsCompleted(2);
}

BOOST_AUTO_TEST_CASE(RetryOfNodeUpUpgradeCausesUpgradeReplyToBeSent)
{
    // Regression test: 1501082
    scenarioTest_->UTContext.Hosting.ValidateFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(false);

    scenarioTest_->SetNodeUpAckFromFMProcessed(false);

    // FM requests upgrade at node up
    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 2 2.0.0:cfg2:2", false);
    scenarioTest_->ValidateFabricUpgradeIsCompleted(2);
    scenarioTest_->ValidateNodeUpAckReceived(true, false);
    scenarioTest_->ResetAll();

    // RA will have sent NodeUp with the correct version
    // FM sends NodeUpAck
    scenarioTest_->SetNodeUpAckFromFMProcessed(true);

    // FM sends Upgrade with same version -> Reply should be a NodeFabricUpgradeReply
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg2:2 Rolling");
    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"2.0.0:cfg2:2");
}

BOOST_AUTO_TEST_CASE(NodeUpAckWithZeroInstanceComparesCodeVersion)
{
    scenarioTest_->UTContext.Hosting.SetAllAsyncApisToCompleteAsynchronously();

    scenarioTest_->SetNodeUpAckFromFMProcessed(false);
    scenarioTest_->UTContext.NodeConfig.NodeVersion = Reader::ReadHelper<FabricVersionInstance>(L"1.0.0:cfg1:0");
    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 2 1.0.0:cfg1:0", false);
    Verify::AreEqual(0, scenarioTest_->UTContext.Reliability.NodeUpToFMCount, L"NodeUp should not be retried and upgrade started");
}

BOOST_AUTO_TEST_CASE(NodeUpAckWithZeroInstanceFMRequestsUpgrade)
{
    scenarioTest_->UTContext.Hosting.SetAllAsyncApisToCompleteAsynchronously();

    scenarioTest_->SetNodeUpAckFromFMProcessed(false);
    scenarioTest_->UTContext.NodeConfig.NodeVersion = Reader::ReadHelper<FabricVersionInstance>(L"1.0.1:cfg1:0");
    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 2 1.0.0:cfg1:0", false);
    Verify::IsTrue(scenarioTest_->HostingHelper.HostingObj.DownloadFabricAsyncApi.Current != nullptr, L"Current is not null");

    scenarioTest_->HostingHelper.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();
    scenarioTest_->HostingHelper.HostingObj.DownloadFabricAsyncApi.FinishOperationWithSuccess(nullptr);
}

BOOST_AUTO_TEST_CASE(NodeUpWithMisMatchVersionInstance)
{
    // NodeConfig->NodeVersion = 1.0.1:cfg1:0
    // NodeUpAck comes with version 1.0.1:cfg1:1, this triggers upgrade
    // Upgrade completes
    // FabricDeployer changes NodeConfig->NodeVersion to 1.0.1:cfg1:1
    // NodeConfig-NodeVersion changes to 1.0.1:cfg1:0 // Eg: Topology changes invoke FabricDeployer which resets this value
    // NodeUpAck comes with 1.0.1:cfg1:1. Since this doesn't match NodeConfig->NodeVersion (1.0.1:cfg1:0), this triggers upgrade
    // There is upgrade for version 1.0.1:cfg1:1, NodeUp is sent.
    // ProcessTerminationService.ExitProcess called as we have mismatch between NodeConfig->NodeVersion and UpgradeSpecification.InstanceId

    scenarioTest_->UTContext.Hosting.SetAllAsyncApisToCompleteAsynchronously();
    scenarioTest_->SetNodeUpAckFromFMProcessed(false);
    
    scenarioTest_->UTContext.NodeConfig.NodeVersion = Reader::ReadHelper<FabricVersionInstance>(L"1.0.1:cfg1:0");
    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 2 1.0.1:cfg1:1", false);
    
    // manually set NodeConfig->NodeVersion to 1.0.1:cfg1:1 since test doesn't call fabricDeployer
    scenarioTest_->UTContext.NodeConfig.NodeVersion = Reader::ReadHelper<FabricVersionInstance>(L"1.0.1:cfg1:1");

    Verify::IsTrue(scenarioTest_->HostingHelper.HostingObj.DownloadFabricAsyncApi.Current != nullptr, L"Current is not null");
    scenarioTest_->HostingHelper.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();
    scenarioTest_->HostingHelper.HostingObj.DownloadFabricAsyncApi.FinishOperationWithSuccess(nullptr);

    scenarioTest_->UTContext.NodeConfig.NodeVersion = Reader::ReadHelper<FabricVersionInstance>(L"1.0.1:cfg1:0");

    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 2 1.0.1:cfg1:1", false);

    scenarioTest_->UTContext.ProcessTerminationServiceStubObj.Calls.VerifyCallCount(1);
}

BOOST_AUTO_TEST_CASE(CancelFabricUpgradeSanityTest)
{
    // Sanity to set instance id 3 as too high
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::CancelFabricUpgradeRequest>(L"2.0.0:cfg2:3 Rolling");
    scenarioTest_->ValidateFMMessage<MessageType::CancelFabricUpgradeReply>(L"2.0.0:cfg2:3");

    scenarioTest_->ResetAll();

    // instance id 2 is dropped
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg1:2 Rolling");
    scenarioTest_->ValidateNoMessages();
}

BOOST_AUTO_TEST_CASE(UpgradeShouldBeDroppedIfRAIsNotReady_Fmm)
{
    UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(true, false);
}

BOOST_AUTO_TEST_CASE(UpgradeShouldBeDroppedIfRAIsNotReady_FM)
{
    UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(false, true);
}

BOOST_AUTO_TEST_CASE(UpgradeShouldBeDroppedIfRAIsNotReady_FmmAndFM)
{
    UpgradeShouldBeDroppedIfRAIsNotReadyTestHelper(false, false);
}

BOOST_AUTO_TEST_CASE(CancelShouldBeDroppedIfRAIsNotReady_Fmm)
{
    CancelShouldBeDroppedIfRAIsNotReadyTestHelper(true, false);
}

BOOST_AUTO_TEST_CASE(CancelShouldBeDroppedIfRAIsNotReady_FM)
{
    CancelShouldBeDroppedIfRAIsNotReadyTestHelper(false, true);
}

BOOST_AUTO_TEST_CASE(CancelShouldBeDroppedIfRAIsNotReady_FmmAndFM)
{
    CancelShouldBeDroppedIfRAIsNotReadyTestHelper(false, false);
}

BOOST_AUTO_TEST_CASE(Upgrade_NodeUpAck_InformsFT)
{
    SetupFTForEntityUpdateOnRollbackTest();

    scenarioTest_->SetNodeUpAckFromFMProcessed(false);

    scenarioTest_->ProcessNodeUpAckAndDrain(L"true 2 2.0.0:cfg2:2", false);

    VerifyDeactivationInfoIsMarkedInvalid();
}

BOOST_AUTO_TEST_CASE(Upgrade_UpgradeMessage_InformsFT)
{
    SetupFTForEntityUpdateOnRollbackTest();

    ProcessUpgradeMessageToVersionThatDoesNotSupportDeactivationInfo();

    VerifyDeactivationInfoIsMarkedInvalid();
}

BOOST_AUTO_TEST_CASE(Upgrade_UpgradeMessage_InformsFT_NegativeTest)
{
    SetupFTForEntityUpdateOnRollbackTest();

    ProcessUpgradeMessageToVersionThatSupportsDeactivationInfo();

    VerifyDeactivationInfoIsStillValid();
}

BOOST_AUTO_TEST_CASE(Upgrade_UpgradeMessage_DoesNotInformFT_IfHostingDoesNotRestartReplica)
{
    auto & test = *scenarioTest_;

    SetupFTForEntityUpdateOnRollbackTest();
    test.HostingHelper.HostingObj.ValidateFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(false);

    ProcessUpgradeMessageToVersionThatDoesNotSupportDeactivationInfo();

    VerifyDeactivationInfoIsStillValid();
}

BOOST_AUTO_TEST_CASE(Upgrade_TerminatesServiceHost)
{
    scenarioTest_->UTContext.Hosting.ValidateFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(true);
    scenarioTest_->UTContext.Config.FabricUpgradeMaxReplicaCloseDuration = TimeSpan::FromSeconds(0);

    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    scenarioTest_->AddFT(L"SP2", L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeFabricUpgradeRequest>(L"2.0.0:cfg2:2 Rolling");

    scenarioTest_->FireFabricUpgradeCloseCompletionCheckTimer(2);
    scenarioTest_->DrainJobQueues();

    scenarioTest_->ValidateTerminateCall({ L"SP1", L"SP2" });

    scenarioTest_->ProcessAppHostClosedAndDrain(L"SP1");
    scenarioTest_->ProcessAppHostClosedAndDrain(L"SP2");
    scenarioTest_->FireFabricUpgradeCloseCompletionCheckTimer(2);
    scenarioTest_->DrainJobQueues();

    scenarioTest_->ValidateFMMessage<MessageType::NodeFabricUpgradeReply>(L"2.0.0:cfg2:2");

    scenarioTest_->ValidateFabricUpgradeIsCompleted(2);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Build Idle

class TestStateMachine_BuildIdle
{
protected:
    TestStateMachine_BuildIdle() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_BuildIdle() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_BuildIdle::TestSetup()
{
    scenarioTest_ = ScenarioTest::Create();
    return true;
}

bool TestStateMachine_BuildIdle::TestCleanup()
{
    return scenarioTest_->Cleanup();
}
BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_BuildIdleSuite, TestStateMachine_BuildIdle)

BOOST_AUTO_TEST_CASE(DuplicateBuildIdleReplyAfterReplicatorHasChangedRoleCausesReplicaOnRAToGoFromIBToSB)
{
    scenarioTest_->AddFT(L"SP1", L"O Phase2_Catchup 411/000/422 1:1 CM [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S SB U N F 3:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorBuildIdleReplicaReply>(L"SP1", L"411/422 [P/S RD U 1:1] [S/I IB U 3:1] RAProxyDemoteCompleted -");

    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 411/000/422 1:1 CM [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S SB U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildIdleReplyIsConsideredForSamePrimaryEpochButDifferentFailoverEpoch)
{
    /*
    Build started in epoch e1
    Another replica got built and I/S happened and epoch moved to e2
    Build completes and RAP sends reply in e1
    This reply should be processed
    */
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/I IB U N F 3:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorBuildIdleReplicaReply>(L"SP1", L"000/411 [N/P RD U 1:1] [N/I RD U 3:1] Success -");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/I RD U N F 3:1]");
    scenarioTest_->ValidateFMMessage<MessageType::AddReplicaReply>();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region BuildSecondary

class TestStateMachine_BuildSecondary
{
protected:
    TestStateMachine_BuildSecondary() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_BuildSecondary() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void SetupBuildSecondaryToBeActivatedTestFromNone(
        wstring const & incomingReplica, 
        wstring const & expected);

    void SetupBuildSecondaryToBeActivatedTestFromReconfigStage(
        wstring const & reconfigStage,
        wstring const & incomingReplica,
        wstring const & expected);

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_BuildSecondary::TestSetup()
{
    scenarioTest_ = ScenarioTest::Create();

    return true;
}

bool TestStateMachine_BuildSecondary::TestCleanup()
{
    return scenarioTest_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_BuildSecondarySuite, TestStateMachine_BuildSecondary)

BOOST_AUTO_TEST_CASE(BuildSecondary_ChangeRoleIdleToSecondary)
{
    // If a retryable error is returned from RAP during replica open, then RA should retry the message
    // and not send the reply back to FM (which would mark the replica as dropped), defect#:1201758

    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/S SB U N 0 1:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, L"SP1", L"411/411 [S/S IB U 1:1]");

    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/411 1:1 CMS (sender:2) [S/N/I IC U N 0 1:1]");

    // This is the important validation to ensure no messages to FM are generated
    scenarioTest_->ValidateNoFMMessages();

    scenarioTest_->ResetAll();

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicaOpenReply>(L"SP1", L"411/411 [S/I IB U 1:1] Success -");
    scenarioTest_->ValidateFT(L"SP1", L"O None 411/000/411 1:1 CM [S/N/I IB U N 0 1:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_CreateReplicaSecondaryLocalReplicaIdle)
{
    // Regression test: 1445159
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/412 2:1 CM [N/N/I SB U N 0 2:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(1, L"SP1", L"411/412 [S/S IB U 2:1]");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/412 2:1 CMS (sender:1) [N/N/I IC U N 0 2:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_StaleActivateMessage)
{
    // Regression Test: 1456393
    scenarioTest_->AddFT(L"SP1", L"O None 412/000/412 2:1 CM [S/N/I IB U N F 2:1] [P/N/P RD U N F 1:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::Activate>(1, L"SP1", L"411/412 [P/P RD U 1:1] [I/S SB U 2:1]");

    scenarioTest_->ValidateNoMessages();

    scenarioTest_->ValidateFT(L"SP1", L"O None 412/000/412 2:1 CM [S/N/I IB U N F 2:1] [P/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_NewerPCEpochActivateMessage)
{
    // Regression Test: 1481480
    scenarioTest_->AddFT(L"SP1", L"O None 411/423/423 2:1 CM [S/S/I IB U N F 2:1] [S/S/S RD U N F 3:1] [P/I/I RD D N F 1:1] [S/S/S RD U N F 4:1] [S/P/P RD U N F 5:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::Activate>(1, L"SP1", L"422/423 [P/P RD U 5:1] [S/S RD U 3:1] [S/S RD U 4:1] [S/I RD D 1:1] [S/S SB U 2:1]");

    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/423 2:1 CM [N/N/S SB U N F 2:1] [N/N/P RD U N F 5:1] [N/N/S RD U N F 3:1] [N/N/S RD U N F 4:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_DuplicateCreateReplicaReply)
{
    scenarioTest_->AddFT(L"SP1", L"O Phase4_Activate 412/412/412 1:1 CM [P/P/P RD U N F 1:1] [S/I/I RD U N F 2:1] [S/S/S RD U N F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplicaReply>(2, L"SP1", L"000/412 [N/I IB U 2:1] Success");

    scenarioTest_->ValidateNoMessages();
}

// Regression tests for 1224588

BOOST_AUTO_TEST_CASE(BuildSecondary_SecondaryDownDuringBuild)
{
    // Replica down during build

    scenarioTest_->AddFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB U N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S IB D 2:1] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB D N R 2:1] [I/N/S RD U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_SecondaryDroppedDuringBuild)
{
    // Replica dropped during build

    scenarioTest_->AddFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB U N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S DD D 2:1] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB D N R 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorRemoveIdleReplicaReply>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S IB D 2:1] Success -");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB D N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S DD D 2:1] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S DD D N F 2:1] [I/N/S RD U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_SecondaryRestartAndStandByDuringBuild)
{
    // Replica in StandBy (new instance) during build of previous instance

    scenarioTest_->AddFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB U N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S SB U 2:2] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB D N R 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorRemoveIdleReplicaReply>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S IB D 2:1] Success -");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB D N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S SB U 2:2] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CMNB [P/N/P RD U N F 1:1] [S/N/S SB U N F 2:2] [I/N/S RD U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_SecondaryRestartAndInBuildDuringBuild)
{
    // Replica in InBuild (new instance) during build of previous instance

    scenarioTest_->AddFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB U N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S IB U 2:2] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB D N R 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorRemoveIdleReplicaReply>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S IB D 2:1] Success -");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB D N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S IB U 2:2] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CMNB [P/N/P RD U N F 1:1] [S/N/S IC U N F 2:2] [I/N/S RD U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_SecondaryRestartAndDroppedDuringBuild)
{
    // Replica dropped (new instance) during build of previous instance

    scenarioTest_->AddFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB U N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S DD D 2:2] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB D N R 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorRemoveIdleReplicaReply>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S IB D 2:1] Success -");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB D N F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"412/414 [P/P RD U 1:1] [S/S DD D 2:2] [I/S RD U 3:1]");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 412/000/414 1:1 CMNB [P/N/P RD U N F 1:1] [S/N/S DD D N F 2:2] [I/N/S RD U N F 3:1]");
}

// Regression tests for 1611441

BOOST_AUTO_TEST_CASE(BuildSecondary_TransitionToBuildDuringDeactivatePhase)
{
    scenarioTest_->AddFT(L"SP1", L"O Phase3_Deactivate 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IC U RA F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::DeactivateReply>(2, L"SP1", L"412/414 [N/S SB U 2:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase3_Deactivate 412/000/414 1:1 CMJ [P/N/P RD U N F 1:1] [S/N/S IC U RA F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplicaReply>(2, L"SP1", L"412/414 [S/I IB U 2:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase3_Deactivate 412/000/414 1:1 CMJ [P/N/P RD U N F 1:1] [S/N/S IB U RA F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::DeactivateReply>(2, L"SP1", L"412/414 [N/S SB U 2:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase3_Deactivate 412/000/414 1:1 CMJ [P/N/P RD U N F 1:1] [S/N/S IB U RA F 2:1] [I/N/S RD U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_TransitionToBuildDuringActivatePhase)
{
    scenarioTest_->AddFT(L"SP1", L"O Phase4_Activate 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IC U RA F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, L"SP1", L"412/414 [N/S SB U 2:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase4_Activate 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IC U RA F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplicaReply>(2, L"SP1", L"412/414 [S/I IB U 2:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase4_Activate 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB U RA F 2:1] [I/N/S RD U N F 3:1]");

    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, L"SP1", L"412/414 [N/S SB U 2:1] Success");
    scenarioTest_->ValidateFT(L"SP1", L"O Phase4_Activate 412/000/414 1:1 CM [P/N/P RD U N F 1:1] [S/N/S IB U RA F 2:1] [I/N/S RD U N F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_EndToEnd)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/755 {522:3} 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD D N F 2:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::AddReplica>(L"SP1", L"000/755 [N/S IB U 2:2]");
    scenarioTest_->ValidateRAMessage<MessageType::CreateReplica>(2, L"SP1", L"000/755 {522:3} [N/S IB U 2:2]");

    scenarioTest_->ResetAll();
    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::CreateReplicaReply>(2, L"SP1", L"411/755 [S/I IB U 2:2] Success");
    scenarioTest_->ValidateRAPMessage<MessageType::ReplicatorBuildIdleReplica>(L"SP1", L"000/755 [N/P RD U 1:1] [N/I IB U 2:2] -");

    scenarioTest_->ResetAll();
    scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ReplicatorBuildIdleReplicaReply>(L"SP1", L"000/755 [N/P RD U 1:1] [S/I RD U 2:2] Success -");
    scenarioTest_->ValidateRAMessage<MessageType::Activate>(2, L"SP1", L"000/755 {522:3} [N/P RD U 1:1] [N/S RD U 2:2]");

    scenarioTest_->ResetAll();
    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, L"SP1", L"000/755 [N/S RD U 2:2] Success");
    scenarioTest_->ValidateFMMessage<MessageType::AddReplicaReply>(L"SP1", L"000/755 [N/S RD U 2:2] Success");
    scenarioTest_->ValidateRAPMessage<MessageType::UpdateConfiguration>(L"SP1", L"000/755 [N/P RD U 1:1] [N/P RD U 1:1] [N/S RD U 2:2]");
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/755 {522:3} 1:1 CMB [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:2]");
}


BOOST_AUTO_TEST_CASE(BuildSecondary_ToBeActivated_DoReconfiguration_IBSameInstance)
{
    SetupBuildSecondaryToBeActivatedTestFromNone(
        L"[S/S IB U 2:1]",
        L"O Phase2_Catchup 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB U N T 2:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_ToBeActivated_DoReconfiguration_Down)
{
    SetupBuildSecondaryToBeActivatedTestFromNone(
        L"[S/I RD D 2:1]",
        L"O Phase2_Catchup 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/I IB D N R 2:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_ToBeActivated_DoReconfiguration_RestartSB)
{
    SetupBuildSecondaryToBeActivatedTestFromNone(
        L"[S/S SB U 2:2]",
        L"O Phase2_Catchup 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB D N R 2:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_ToBeActivated_DoReconfiguration_RestartIB)
{
    SetupBuildSecondaryToBeActivatedTestFromNone(
        L"[S/S IB U 2:2]",
        L"O Phase2_Catchup 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB D N R 2:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_ToBeDeactivated_Phase3_DoReconfiguration_IBSameInstance)
{
    SetupBuildSecondaryToBeActivatedTestFromReconfigStage(
        L"O Phase3_Deactivate 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB U RA D 2:1] [S/N/S RD U RA F 3:1]",
        L"[S/S IB U 2:1]",
        L"O Phase3_Deactivate 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB U RA D 2:1] [S/N/S RD U RA F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_ToBeDeactivated_Phase3_DoReconfiguration_IBNewInstance)
{
    SetupBuildSecondaryToBeActivatedTestFromReconfigStage(
        L"O Phase3_Deactivate 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB U RA D 2:1] [S/N/S RD U RA F 3:1]",
        L"[S/S IB U 2:2]",
        L"O Phase3_Deactivate 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB D RA R 2:1] [S/N/S RD U RA F 3:1]");
}

BOOST_AUTO_TEST_CASE(BuildSecondary_ToBeDeactivated_Phase3_DoReconfiguration_IBDown)
{
    SetupBuildSecondaryToBeActivatedTestFromReconfigStage(
        L"O Phase3_Deactivate 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB U RA D 2:1] [S/N/S RD U RA F 3:1]",
        L"[S/S IB D 2:1]",
        L"O Phase3_Deactivate 411/000/412 {411:0} 1:1 CMN [P/N/P RD U N F 1:1] [S/N/S IB D RA R 2:1] [S/N/S RD U RA F 3:1]");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

void TestStateMachine_BuildSecondary::SetupBuildSecondaryToBeActivatedTestFromNone(wstring const & incomingReplica, wstring const & expected)
{
    SetupBuildSecondaryToBeActivatedTestFromReconfigStage(
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S IB U RA A 2:1]",
        incomingReplica,
        expected);
}

void TestStateMachine_BuildSecondary::SetupBuildSecondaryToBeActivatedTestFromReconfigStage(
    wstring const & initial,
    wstring const & incomingReplica,
    wstring const & expected)
{
    scenarioTest_->AddFT(L"SP1", initial);

    wstring msg = wformatString("411/412 [P/P RD U 1:1] {0}", incomingReplica);
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", msg);

    scenarioTest_->DumpFT(L"SP1");
    scenarioTest_->ValidateFT(L"SP1", expected);
}


#pragma endregion

#pragma region Endpoints

class TestStateMachine_Endpoints
{
protected:
    TestStateMachine_Endpoints() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_Endpoints() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTestUPtr scenarioTest_;
};

bool TestStateMachine_Endpoints::TestSetup()
{
    scenarioTest_ = ScenarioTest::Create();

    return true;
}

bool TestStateMachine_Endpoints::TestCleanup()
{
    return scenarioTest_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_EndpointsSuite, TestStateMachine_Endpoints)

BOOST_AUTO_TEST_CASE(Endpoints_DoReconfiguration)
{
    // defect#:1579052

    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N 0 1:1 s:s1 r:e1][N/N/S RD U N 0 2:1 s:s2 r:e2][N/N/I RD U N 0 3:1 s:s3 r:e3]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/412 [P/P RD U 1:1][S/S IB U 2:1][I/S RD U 3:1");

    scenarioTest_->ValidateFT(L"SP1", L"O Phase2_Catchup 411/000/412 1:1 CMN [P/N/P RD U N 0 1:1 s:s1 r:e1][S/N/S RD U N 0 2:1 s:s2 r:e2][I/N/S RD U N 0 3:1 s:s3 r:e3]");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region CreateReplica

class TestStateMachine_CreateReplica
{
protected:
    TestStateMachine_CreateReplica() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_CreateReplica() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void ValidateReplicaRecreateDueToEpochChangeInner(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & initialFT,
        wstring const & message,
        wstring const & expectedFT);

    void ValidateReplicaRecreateDueToEpochChange(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & initialFT,
        wstring const & message,
        wstring const & expectedFT);

    template<MessageType::Enum T>
    void ReplicaCreateTestHelper(wstring const & ft, wstring const & message, wstring const & expected)
    {
        ScenarioTest & scenarioTest = holder_->Recreate();

        scenarioTest.SetFindServiceTypeRegistrationState(ft, FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpenAndReopen);

        scenarioTest.ProcessFMMessageAndDrain<T>(ft, message);

        scenarioTest.ValidateFT(ft, expected);
    }

    void CreateReplicaTestHelper(wstring const & initial, wstring const & msg, wstring const & expected);

    ScenarioTest& GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_CreateReplica::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();
    return true;
}

bool TestStateMachine_CreateReplica::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_CreateReplica::ValidateReplicaRecreateDueToEpochChangeInner(
    wstring const & log,
    wstring const & ftShortName,
    wstring const & initialFT,
    wstring const & message,
    wstring const & expectedFT)
{
    ScenarioTest & scenarioTest = holder_->Recreate();
    scenarioTest.LogStep(wformatString("{0} {1}", log, ftShortName));
    scenarioTest.AddFT(ftShortName, initialFT);

    scenarioTest.ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, ftShortName, message);

    scenarioTest.ValidateNoFMMessages();
    scenarioTest.ValidateFT(ftShortName, expectedFT);
}

void TestStateMachine_CreateReplica::ValidateReplicaRecreateDueToEpochChange(
    wstring const & log,
    wstring const & initialFT,
    wstring const & message,
    wstring const & expectedFTSP,
    wstring const & expectedFTSV)
{
    ValidateReplicaRecreateDueToEpochChangeInner(
        log,
        L"SP1",
        initialFT,
        message,
        expectedFTSP);

    ValidateReplicaRecreateDueToEpochChangeInner(
        log,
        L"SV1",
        initialFT,
        message,
        expectedFTSV);
}

void TestStateMachine_CreateReplica::CreateReplicaTestHelper(wstring const & initial, wstring const & msg, wstring const & expected)
{
    auto & test = holder_->Recreate();
    auto shortName = L"SP1";

    test.AddFT(shortName, initial);

    test.ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, shortName, msg);

    test.ValidateFT(shortName, expected);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_CreateReplicaSuite, TestStateMachine_CreateReplica)

BOOST_AUTO_TEST_CASE(ReplicaRecreateDueToEpochChange)
{
    wstring const NoPCEpochInitial = L"O None 000/000/411 1:1 CM [N/N/I IB U N F 1:1]";
    wstring const NoPCEpochMessage = L"000/422 [N/S IB U 1:1]";
    wstring const NoPCEpochPersisted = L"O None 000/000/411 1:1 CMHr [N/N/I IB U N F 1:1]";
    wstring const NoPCEpochVolatile = L"O None 000/000/411 1:1 CMHd [N/N/I ID U N F 1:1]";


    // No PC Epoch
    ValidateReplicaRecreateDueToEpochChange(
        L"NoPCEpoch",
        NoPCEpochInitial,
        NoPCEpochMessage,
        NoPCEpochPersisted,
        NoPCEpochVolatile);

    wstring const CCEpochGreaterThanPCEpochInitial = L"O None 411/000/412 1:1 CM [S/N/I IB U N F 1:1]";
    wstring const CCEpochGreaterThanPCEpoch = L"000/433 [N/S IB U 1:1]";
    wstring const CCEpochGreaterThanPCEpochPersisted = L"O None 411/000/412 1:1 CMHr [S/N/I IB U N F 1:1]";
    wstring const CCEpochGreaterThanPCEpochVolatile = L"O None 411/000/412 1:1 CMHd [S/N/I ID U N F 1:1]";

    ValidateReplicaRecreateDueToEpochChange(
        L"CCEpochGreaterThanPCEpoch",
        CCEpochGreaterThanPCEpochInitial,
        CCEpochGreaterThanPCEpoch,
        CCEpochGreaterThanPCEpochPersisted,
        CCEpochGreaterThanPCEpochVolatile);

    // Data Loss version changes
    wstring const NoPCEpochDataLossChangeMessage = L"000/511 [N/S IB U 1:1]";
    ValidateReplicaRecreateDueToEpochChange(
        L"NoPCEpochdataLossChange",
        NoPCEpochInitial,
        NoPCEpochDataLossChangeMessage,
        NoPCEpochPersisted,
        NoPCEpochVolatile);

    wstring const PCEpochDataLossChangeMessage = L"411/512 [S/S IB U 1:1]";
    ValidateReplicaRecreateDueToEpochChange(
        L"PCEpochDataLossChange",
        CCEpochGreaterThanPCEpochInitial,
        PCEpochDataLossChangeMessage,
        CCEpochGreaterThanPCEpochPersisted,
        CCEpochGreaterThanPCEpochVolatile);
}

BOOST_AUTO_TEST_CASE(PersistedReplicaRestartInDataLoss)
{
    // Regression Test: #1464139
    // Replica exists on secondary RA as I, IB in epoch 411
    // Primary goes down
    // Reconfiguration is epoch 411/422
    // New Primary performs GetLSN which updates the epoch on this node to 422
    // Primary detects DataLoss
    // FM increments DataLossNumber
    // Primary sends CreateReplica with higher DataLoss number 522
    // This test validates that at this point Secondary restarts Replica so it can be built by new primary (so that replicator doesn't have to do so)

    GetScenarioTest().SetFindServiceTypeRegistrationSuccess(L"SP1");
    GetScenarioTest().AddFT(L"SP1", L"O None 000/000/422 1:1 CM [N/N/I IB U N F 1:1]");

    GetScenarioTest().ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, L"SP1", L"411/522 [S/S IB U 1:1]");

    GetScenarioTest().ValidateFT(L"SP1", L"O None 000/000/422 1:1 CMHr [N/N/I IB U N F 1:1]");

    // Complete Reopen
    GetScenarioTest().ResetAll();
    GetScenarioTest().ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"SP1", L"000/422 [N/I RD U 1:1] Success -");

    GetScenarioTest().DumpFT(L"SP1");
    GetScenarioTest().ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(L"SP1", L"000/422 [N/I SB U 1:2] Success -");

    // Replica Up will be sent to FM
    // FM will send DoReconfig with updated instance to Primary
    // Primary will send updated instance in CreateReplica
    GetScenarioTest().ResetAll();
    GetScenarioTest().ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, L"SP1", L"411/522 [S/S IB U 1:2]");

    // Validate the FT - the epoch should be updated and so the data loss number
    GetScenarioTest().ValidateFT(L"SP1", L"O None 000/000/522 {422:0} 1:2 CMS (sender:2) [N/N/I IC U N F 1:2]");
}

BOOST_AUTO_TEST_CASE(ReplicaCreateTest)
{
    ReplicaCreateTestHelper<MessageType::AddPrimary>(L"SP1", L"000/411 [N/P RD U 1:1]", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
    ReplicaCreateTestHelper<MessageType::AddPrimary>(L"SV1", L"000/411 [N/P RD U 1:1]", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
    ReplicaCreateTestHelper<MessageType::AddInstance>(L"SL1", L"000/411 [N/N RD U 1:1]", L"O None 000/000/411 {000:-1} 1:1 S [N/N/N IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(OpenReplicaReturnsReplyImmediately)
{
    GetScenarioTest().AddFT(L"SP1", L"O None 000/000/422 1:1 CM [N/N/I IB U N F 1:1]");
    GetScenarioTest().ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, L"SP1", L"000/422 [N/I IB U 1:1]");
    GetScenarioTest().ValidateRAMessage<MessageType::CreateReplicaReply>(2, L"SP1", L"000/422 [N/I IB U 1:1] Success");
}

BOOST_AUTO_TEST_CASE(NewSecondaryInBuildReplica_DoesNotSetDeactivationEpoch)
{
    CreateReplicaTestHelper(
        L"C None 000/000/411 1:1 -",
        L"412/413 [S/S IB U 1:1:3:2]",
        L"O None 000/000/413 {000:-2} 3:2 S (sender:2) [N/N/I IC U N F 1:1:3:2]");
}

BOOST_AUTO_TEST_CASE(NewIdleInBuildReplica_SetsDeactivationEpoch)
{
    CreateReplicaTestHelper(
        L"C None 000/000/411 1:1 -",
        L"000/413 [N/I IB U 1:1:3:2]",
        L"O None 000/000/413 3:2 S (sender:2) [N/N/I IC U N F 1:1:3:2]");
}

BOOST_AUTO_TEST_CASE(RebuildIdleReplica_SetsDeactivationEpoch)
{
    CreateReplicaTestHelper(
        L"O None 000/000/413 {413:0} 1:1 CM [N/N/I SB U N F 1:1:3:1]",
        L"000/533 [N/I IB U 1:1]",
        L"O None 000/000/533 {533:0} 1:1 CMS (sender:2) [N/N/I IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(RebuildSecondaryReplica_DoesNotSetDeactivationEpoch)
{
    CreateReplicaTestHelper(
        L"O None 000/000/413 {413:0} 1:1 CM [N/N/I SB U N F 1:1:3:1]",
        L"412/533 [S/S IB U 1:1]",
        L"O None 000/000/533 {413:0} 1:1 CMS (sender:2) [N/N/I IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(RebuildSecondaryReplica_NotInConfig_DoesNotSetDeactivationEpoch)
{
    CreateReplicaTestHelper(
        L"O None 000/000/413 {413:0} 1:1 CM [N/N/S SB U N F 1:1] [N/N/P RD U N F 2:1]",
        L"000/533 [N/S IB U 1:1]",
        L"O None 413/000/533 {413:0} 1:1 CMS (sender:2) [S/N/I IC U N F 1:1] [P/N/P RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(CreateReplica_UpgradingNode_IsIgnored)
{
    auto & test = GetScenarioTest();
    
    test.DeactivateNode(1);

    test.ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, L"SP1", L"000/411 [N/I IB U 1:1]");

    test.ValidateFTIsNull(L"SP1");
    test.ValidateRAMessage<MessageType::CreateReplicaReply>(2);
}

BOOST_AUTO_TEST_CASE(CreateReplica_PCIsSaved)
{
    CreateReplicaTestHelper(
        L"O None 000/000/411 1:1 CM [N/N/S SB U N F 1:1] [N/N/P RD U N F 2:1]",
        L"000/411 [N/S IB U 1:1]",
        L"O None 411/000/411 1:1 CMS [S/N/I IC U N F 1:1] [P/N/P RD U N F 2:1]");
}

BOOST_AUTO_TEST_CASE(CreateReplica_PCIsResetForIdle)
{
    CreateReplicaTestHelper(
        L"O None 000/000/411 1:1 CM [N/N/S SB U N F 1:1] [N/N/P RD U N F 2:1]",
        L"000/411 [N/I IB U 1:1]",
        L"O None 000/000/411 1:1 CMS [N/N/I IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(CreateReplica_PCIsResetIfIncomingRoleIsIdle)
{
    CreateReplicaTestHelper(
        L"O None 411/000/411 1:1 CM [S/N/S SB U N F 1:1] [P/N/P RD U N F 2:1]",
        L"000/411 [N/I IB U 1:1]",
        L"O None 000/000/411 1:1 CMS [N/N/I IC U N F 1:1]");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Lfum Reply

class TestStateMachine_LfumReply
{
public:
    void LfumReplyTestHelper(ScenarioTest & scenarioTest, GenerationNumberSource::Enum source, wstring const & generation);

    void ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::Enum source, wstring const & generation);

    void ProcessLfumReplyTestHelper(GenerationNumberSource::Enum source, wstring const & generation);

private:
    static const int sourceInitialSendGeneration = 1, sourceInitialReceiveGeneration = 2, sourceInitialProposedGeneration = 3;
    static const int otherInitialSendGeneration = 4, otherInitialReceiveGeneration = 5, otherInitialProposedGeneration = 6;
};

void TestStateMachine_LfumReply::LfumReplyTestHelper(ScenarioTest & scenarioTest, GenerationNumberSource::Enum source, wstring const & generation)
{
    auto other = scenarioTest.GenerationStateHelper.GetOther(source);

    scenarioTest.GenerationStateHelper.SetGeneration(source, 1, sourceInitialSendGeneration, sourceInitialReceiveGeneration, sourceInitialProposedGeneration);
    scenarioTest.GenerationStateHelper.SetGeneration(other, 1, otherInitialSendGeneration, otherInitialReceiveGeneration, otherInitialProposedGeneration);

    StateManagement::Reader r(generation, ReadContext());
    GenerationHeader header;
    auto rv = r.Read(L' ', header);
    Verify::IsTrueLogOnError(rv, L"Read should be valid");

    scenarioTest.RA.ProcessLFUMUploadReply(header);
}

void TestStateMachine_LfumReply::ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::Enum source, wstring const & generation)
{
    ScenarioTestHolder scenarioTestHolder;
    ScenarioTest & scenarioTest = scenarioTestHolder.ScenarioTestObj;

    LfumReplyTestHelper(scenarioTest, source, generation);

    scenarioTest.GenerationStateHelper.VerifyGeneration(source, 1, sourceInitialSendGeneration, sourceInitialReceiveGeneration, sourceInitialProposedGeneration);
    scenarioTest.GenerationStateHelper.VerifyGeneration(scenarioTest.GenerationStateHelper.GetOther(source), 1, otherInitialSendGeneration, otherInitialReceiveGeneration, otherInitialProposedGeneration);
}


void TestStateMachine_LfumReply::ProcessLfumReplyTestHelper(GenerationNumberSource::Enum source, wstring const & generation)
{
    ScenarioTestHolder scenarioTestHolder;
    ScenarioTest & scenarioTest = scenarioTestHolder.ScenarioTestObj;

    LfumReplyTestHelper(scenarioTest, source, generation);

    scenarioTest.GenerationStateHelper.VerifyGeneration(source, 1, sourceInitialReceiveGeneration, sourceInitialReceiveGeneration, sourceInitialProposedGeneration);
    scenarioTest.GenerationStateHelper.VerifyGeneration(scenarioTest.GenerationStateHelper.GetOther(source), 1, otherInitialSendGeneration, otherInitialReceiveGeneration, otherInitialProposedGeneration);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_LfumReplySuite, TestStateMachine_LfumReply)

BOOST_AUTO_TEST_CASE(LfumReplyStalenessCheck)
{
    // All generation messages should be rejected except those that match the receive generation
    // The source receive generation is 2
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FM, L"g:1:1,fm");
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FM, L"g:1:3,fm");
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FM, L"g:1:4,fm");
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FM, L"g:1:5,fm");
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FM, L"g:1:6,fm");

    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FMM, L"g:1:1,fmm");
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FMM, L"g:1:3,fmm");
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm");
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FMM, L"g:1:5,fmm");
    ProcessLfumReplyStalenessCheckTestHelper(GenerationNumberSource::FMM, L"g:1:6,fmm");
}

BOOST_AUTO_TEST_CASE(LfumReplySetsGeneration)
{
    ProcessLfumReplyTestHelper(GenerationNumberSource::FM, L"g:1:2,fm");
    ProcessLfumReplyTestHelper(GenerationNumberSource::FMM, L"g:1:2,fmm");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Generation Proposal

class TestStateMachine_GenerationProposal
{
public:

    void GenerationProposalReplyTestHelper(
        GenerationNumberSource::Enum source,
        bool isResultGfumTransfer);

    void GenerationProposalUpdatesGenerationTest(
        GenerationNumberSource::Enum source,
        int generationNumberInMessage,
        wstring const & message,
        bool isUpdated);

private:
    static const int sourceInitialSendGeneration = 1, sourceInitialReceiveGeneration = 2, sourceInitialProposedGeneration = 3;
    static const int otherInitialSendGeneration = 4, otherInitialReceiveGeneration = 5, otherInitialProposedGeneration = 6;
};

void TestStateMachine_GenerationProposal::GenerationProposalReplyTestHelper(
    GenerationNumberSource::Enum source,
    bool)
{
    ScenarioTestHolder scenarioTestHolder;
    ScenarioTest & scenarioTest = scenarioTestHolder.ScenarioTestObj;

    // Setup Code
    // Setup the initial generation on the node
    auto other = scenarioTest.GenerationStateHelper.GetOther(source);

    scenarioTest.GenerationStateHelper.SetGeneration(source, 1, sourceInitialSendGeneration, sourceInitialReceiveGeneration, sourceInitialProposedGeneration);
    scenarioTest.GenerationStateHelper.SetGeneration(other, 1, otherInitialSendGeneration, otherInitialReceiveGeneration, otherInitialProposedGeneration);

    // Create the input message
    wstring inputMessageBody;
    wstring expectedProposalBody;
    if (source == GenerationNumberSource::FM)
    {
        inputMessageBody = L"g:1:2,fm 1:2";
        expectedProposalBody = L"1:2 1:3";
    }
    else
    {
        inputMessageBody = L"g:1:2,fmm 1:2";
        expectedProposalBody = L"1:2 1:3";
    }

    Transport::MessageUPtr inputMessage = scenarioTest.ParseMessage<MessageType::GenerationProposal>(inputMessageBody);

    bool isGfumUpload = false;
    auto result = scenarioTest.RA.ProcessGenerationProposal(*inputMessage, isGfumUpload);

    Verify::AreEqual(source == GenerationNumberSource::FMM, isGfumUpload);
    Verify::Compare(expectedProposalBody, result, L"ReplyBody");
}

void TestStateMachine_GenerationProposal::GenerationProposalUpdatesGenerationTest(
    GenerationNumberSource::Enum source,
    int generationNumberInMessage,
    wstring const & message,
    bool isUpdated)
{
    ScenarioTestHolder scenarioTestHolder;
    ScenarioTest & scenarioTest = scenarioTestHolder.ScenarioTestObj;

    // Setup Code
    // Setup the initial generation on the node
    auto other = scenarioTest.GenerationStateHelper.GetOther(source);

    scenarioTest.GenerationStateHelper.SetGeneration(source, 1, sourceInitialSendGeneration, sourceInitialReceiveGeneration, sourceInitialProposedGeneration);
    scenarioTest.GenerationStateHelper.SetGeneration(other, 1, otherInitialSendGeneration, otherInitialReceiveGeneration, otherInitialProposedGeneration);

    Transport::MessageUPtr inputMessage = scenarioTest.ParseMessage<MessageType::GenerationProposal>(message);

    bool isGfumTransfer = false;
    auto result = scenarioTest.RA.ProcessGenerationProposal(*inputMessage, isGfumTransfer);

    // Verify the result
    if (isUpdated)
    {
        scenarioTest.GenerationStateHelper.VerifyGeneration(source, 1, sourceInitialSendGeneration, sourceInitialReceiveGeneration, generationNumberInMessage);
        scenarioTest.GenerationStateHelper.VerifyGeneration(other, 1, otherInitialSendGeneration, otherInitialReceiveGeneration, otherInitialProposedGeneration);
    }
    else
    {
        scenarioTest.GenerationStateHelper.VerifyGeneration(source, 1, sourceInitialSendGeneration, sourceInitialReceiveGeneration, sourceInitialProposedGeneration);
        scenarioTest.GenerationStateHelper.VerifyGeneration(other, 1, otherInitialSendGeneration, otherInitialReceiveGeneration, otherInitialProposedGeneration);
    }
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_GenerationProposalSuite, TestStateMachine_GenerationProposal)

BOOST_AUTO_TEST_CASE(GenerationProposalReplyMessageTest)
{
    GenerationProposalReplyTestHelper(GenerationNumberSource::FMM, true);
    GenerationProposalReplyTestHelper(GenerationNumberSource::FM, false);
}
BOOST_AUTO_TEST_CASE(GenerationProposalUpdatesGenerationTest1)
{
    // the initial proposed generation is 3
    // so only a gen proposal with 4 should cause an update
    GenerationProposalUpdatesGenerationTest(GenerationNumberSource::FMM, 2, L"g:1:2,fmm 1:2", false);
    GenerationProposalUpdatesGenerationTest(GenerationNumberSource::FMM, 3, L"g:1:3,fmm 1:3", false);
    GenerationProposalUpdatesGenerationTest(GenerationNumberSource::FMM, 4, L"g:1:4,fmm 1:4", true);

    GenerationProposalUpdatesGenerationTest(GenerationNumberSource::FM, 2, L"g:1:2,fm 1:2", false);
    GenerationProposalUpdatesGenerationTest(GenerationNumberSource::FM, 3, L"g:1:3,fm 1:3", false);
    GenerationProposalUpdatesGenerationTest(GenerationNumberSource::FM, 4, L"g:1:4,fm 1:4", true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

#pragma region Sanity Tests for non-existant FTs

// Validate that if a message that creates an FT is received the behavior is correct
class TestStateMachine_NullFT
{
public:
    template<MessageType::Enum T>
    void TestHelperForFMMessage(
        wstring const & ftShortName,
        wstring const & message,
        wstring const & ftExpected)
    {
        TestHelper<T>(ftShortName, message, ftExpected, true);
    }


    template<MessageType::Enum T>
    void TestHelperForRAMessage(
        wstring const & ftShortName,
        wstring const & message,
        wstring const & ftExpected)
    {
        TestHelper<T>(ftShortName, message, ftExpected, false);
    }

    template<MessageType::Enum T>
    void TestHelper(
        wstring const & ftShortName,
        wstring const & message,
        wstring const & ftExpected,
        bool isFM)
    {
        ScenarioTestHolder holder;
        ScenarioTest & scenarioTest = holder.ScenarioTestObj;

        if (isFM)
        {
            scenarioTest.ProcessFMMessageAndDrain<T>(ftShortName, message);
        }
        else
        {
            scenarioTest.ProcessRemoteRAMessageAndDrain<T>(2, ftShortName, message);
        }

        if (!ftExpected.empty())
        {
            scenarioTest.ValidateFT(ftShortName, ftExpected);
        }
        else
        {
            scenarioTest.ValidateFTIsNull(ftShortName);
        }
    }

};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_NullFTSuite, TestStateMachine_NullFT)

BOOST_AUTO_TEST_CASE(AddInstance)
{
    TestHelperForFMMessage<MessageType::AddInstance>(L"SL1", L"000/411 [N/N RD U 1:1]", L"O None 000/000/411 {000:-1} 1:1 S [N/N/N IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(CreateReplica)
{
    TestHelperForRAMessage<MessageType::CreateReplica>(L"SV1", L"000/411 [N/I RD U 1:1]", L"O None 000/000/411 1:1 S (sender:2) [N/N/I IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(AddPrimary)
{
    TestHelperForFMMessage<MessageType::AddPrimary>(L"SV1", L"000/411 [N/P RD U 1:1]", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(RemoveInstance)
{
    TestHelperForFMMessage<MessageType::RemoveInstance>(L"SL1", L"000/411 [N/N RD U 1:1]", L"C None 000/000/411 1:1 LP");
}

BOOST_AUTO_TEST_CASE(DeleteReplica)
{
    TestHelperForFMMessage<MessageType::DeleteReplica>(L"SP1", L"000/411 [N/P RD U 1:1]", L"C None 000/000/411 1:1 LP");
}

BOOST_AUTO_TEST_CASE(GetLSN)
{
    TestHelperForRAMessage<MessageType::GetLSN>(L"SP1", L"000/411 [N/S RD U 1:1]", L"");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion

namespace UpdateReplicatorConfiguration
{
    class TestStateMachine_UpdateReplicatorConfiguration
    {
    protected:
        TestStateMachine_UpdateReplicatorConfiguration() { BOOST_REQUIRE(TestSetup()); }
        TEST_METHOD_SETUP(TestSetup);
        ~TestStateMachine_UpdateReplicatorConfiguration() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_CLEANUP(TestCleanup);

        void RunRemoteReplicaComparisonTest(wstring const & ftRemoteReplicas, wstring const & replicatorRemoteReplicas, bool expected);
        void RunRemoteReplicaComparisonTestInner(wstring const & ftPrefix, wstring const & msg, bool expected);

        ScenarioTestHolderUPtr holder_;
    };

    bool TestStateMachine_UpdateReplicatorConfiguration::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestStateMachine_UpdateReplicatorConfiguration::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    void TestStateMachine_UpdateReplicatorConfiguration::RunRemoteReplicaComparisonTest(wstring const & ftRemoteReplicas, wstring const & replicatorRemoteReplicas, bool expected)
    {
        auto ftStr = wformatString("O Phase4_Activate 411/412/412 1:1 BM [P/P/P RD U N F 1:1] {0}", ftRemoteReplicas);
        auto msgStr = wformatString("411/412 [P/P RD U 1:1] [P/P RD U 1:1] {0} Success -", replicatorRemoteReplicas);

        RunRemoteReplicaComparisonTestInner(ftStr, msgStr, expected);
    }

    void TestStateMachine_UpdateReplicatorConfiguration::RunRemoteReplicaComparisonTestInner(wstring const & ft, wstring const & msg, bool expected)
    {
        auto & test = holder_->Recreate();
        test.AddFT(L"SP1", ft);

        test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", msg);

        test.DumpFT(L"SP1");

        Verify::AreEqualLogOnError(expected, test.GetFT(L"SP1").IsUpdateReplicatorConfiguration, L"IsUpdateReplicatorConfiguration");
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_UpdateReplicatorConfigurationSuite, TestStateMachine_UpdateReplicatorConfiguration)

    BOOST_AUTO_TEST_CASE(Success)
    {
        RunRemoteReplicaComparisonTest(
            L"[S/S/S RD U N F 2:1]",
            L"[S/S RD U 2:1]",
            false);
    }

    BOOST_AUTO_TEST_CASE(CatchupReplyMarksItAsUpdated)
    {
        RunRemoteReplicaComparisonTestInner(
            L"O Phase0_Demote 411/000/422 1:1 CMB [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:2]",
            L"411/422 [P/P RD U 1:1] [P/S RD U 1:1] [S/S IB U 2:2] Success C",
            false);
    }

    BOOST_AUTO_TEST_CASE(Replicator_ReplicaId_IsNotEqual_RA_ReplicaId)
    {
        // Replicator has a different replica id for the same node as the RA
        RunRemoteReplicaComparisonTest(
            L"[S/S/S RD U N F 2:1:3:1]",
            L"[S/S RD U 2:1:2:1]",
            true);
    }

    BOOST_AUTO_TEST_CASE(Replicator_InstanceId_IsNotEqual_RA_InstanceId)
    {
        // Replicator has a different replica id for the same node as the RA
        RunRemoteReplicaComparisonTest(
            L"[S/S/S RD U N F 2:2]",
            L"[S/S RD U 2:1]",
            true);
    }

    BOOST_AUTO_TEST_CASE(Replicator_IsUp_RA_IsDown)
    {
        RunRemoteReplicaComparisonTest(
            L"[S/S/S RD D N F 2:1]",
            L"[S/S RD U 2:1]",
            true);
    }

    BOOST_AUTO_TEST_CASE(Replicator_IsOther_RA_IsDropped)
    {
        RunRemoteReplicaComparisonTest(
            L"[S/S/S DD D N F 2:1]",
            L"[S/S RD U 2:1]",
            true);

        RunRemoteReplicaComparisonTest(
            L"[S/S/S DD D N F 2:1]",
            L"[S/S IB U 2:1]",
            true);

        RunRemoteReplicaComparisonTest(
            L"[S/S/S DD D N F 2:1]",
            L"[S/S SB U 2:1]",
            true);

        RunRemoteReplicaComparisonTest(
            L"[S/S/S DD D N F 2:1]",
            L"[S/S RD D 2:1]",
            true);
    }

    BOOST_AUTO_TEST_CASE(Replicator_IsOther_RA_IsInBuild)
    {
        RunRemoteReplicaComparisonTest(
            L"[S/S/S RD U N F 2:1]",
            L"[S/S IB U 2:1]",
            true);

        RunRemoteReplicaComparisonTest(
            L"[S/S/S RD U N F 2:1]",
            L"[S/S SB U 2:1]",
            true);
    }

    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
}

# pragma region Public API Report Fault

class TestStateMachine_ClientReportFault
{
protected:
    TestStateMachine_ClientReportFault() { BOOST_REQUIRE(TestSetup()); }

    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_ClientReportFault() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void ObliterateWithAppHostDownTestHelper(wstring const & initial);
    void ObliterateDirectTestHelper(wstring const & shortName);

    void TestNotReadyScenario(wstring const & ftShortName, wstring const & initial, wstring const & message)
    {
        TestScenarioWithError(ftShortName, initial, message, ErrorCodeValue::InvalidReplicaStateForReplicaOperation, L"Cannot close replica that is not open or has close in progress.");
    }

    void TestScenarioWithError(wstring const & ftShortName, wstring const & initial, wstring const & message, ErrorCode error, std::wstring errorMessage)
    {
        TestScenario(ftShortName, initial, message, error, initial, errorMessage);
    }

    void TestSuccessScenario(wstring const & ftShortName, wstring const & initial, wstring const & message, wstring const & final)
    {
        TestScenario(ftShortName, initial, message, ErrorCodeValue::Success, final);
    }

    void TestScenario(
        wstring const & ftShortName,
        wstring const & initial,
        wstring const & message,
        ErrorCode reply,
        wstring const & final);

    void TestScenario(
        wstring const & ftShortName,
        wstring const & initial,
        wstring const & message,
        ErrorCode reply,
        wstring const & final,
        std::wstring errorMessage);

    void TestScenario(
        wstring const & ftShortName,
        wstring const & initial,
        wstring const & message,
        ErrorCode reply);

    void TestScenarioInternal(
        wstring const & ftShortName,
        wstring const & initial,
        wstring const & message,
        ErrorCode reply,
        std::wstring errorMessage);

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_ClientReportFault::TestSetup()
{
    holder_ = make_unique<ScenarioTestHolder>();
    return true;
}

bool TestStateMachine_ClientReportFault::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_ClientReportFault::TestScenario(
    wstring const & ftShortName,
    wstring const & initial,
    wstring const & message,
    ErrorCode reply)
{
    TestScenarioInternal(ftShortName, initial, message, reply, L"");
}

void TestStateMachine_ClientReportFault::TestScenarioInternal(
    wstring const & ftShortName,
    wstring const & initial,
    wstring const & message,
    ErrorCode reply,
    std::wstring errorMessage)
{
    holder_->Recreate();
    auto & scenarioTest = GetScenarioTest();

    scenarioTest.AddFT(ftShortName, initial);

    auto msgReply = wformatString("{0} {1}", reply, errorMessage);

    scenarioTest.ProcessRequestReceiverContextAndDrain<MessageType::ClientReportFaultRequest>(L"z", message);
    scenarioTest.ValidateRequestReceiverContextReply<MessageType::ClientReportFaultReply>(L"z", msgReply);
}

void TestStateMachine_ClientReportFault::TestScenario(
    wstring const & ftShortName,
    wstring const & initial,
    wstring const & message,
    ErrorCode reply,
    wstring const & final)
{
    TestScenario(ftShortName, initial, message, reply, final, L"");
}

void TestStateMachine_ClientReportFault::TestScenario(
    wstring const & ftShortName,
    wstring const & initial,
    wstring const & message,
    ErrorCode reply,
    wstring const & final,
    std::wstring errorMessage)
{
    TestScenarioInternal(ftShortName, initial, message, reply, errorMessage);

    GetScenarioTest().ValidateFT(ftShortName, final);
}

void TestStateMachine_ClientReportFault::ObliterateWithAppHostDownTestHelper(wstring const & initial)
{
    TestScenario(
        L"SP1",
        initial,
        L"Node1 SP1 1 Permanent true",
        ErrorCodeValue::Success);

    auto & test = GetScenarioTest();
    test.ProcessAppHostClosedAndDrain(L"SP1");
    Verify::IsTrueLogOnError(test.GetFT(L"SP1").IsClosed, L"Closed");
    test.ValidateFMMessage<MessageType::ReplicaUp>();
}

void TestStateMachine_ClientReportFault::ObliterateDirectTestHelper(wstring const & ftShortName)
{
    TestScenario(
        ftShortName,
        L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]",
        wformatString("Node1 {0} 1 Permanent true", ftShortName),
        ErrorCodeValue::Success);

    auto & test = GetScenarioTest();
    Verify::IsTrueLogOnError(test.GetFT(ftShortName).IsClosed, L"Closed");

    if (ftShortName == L"FM")
    {
        test.ValidateFmmMessage<MessageType::ReplicaUp>();
    }
    else
    {
        test.ValidateFMMessage<MessageType::ReplicaUp>();
    }
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ClientReportFaultSuite, TestStateMachine_ClientReportFault)

BOOST_AUTO_TEST_CASE(MissingPartitionReturnsReplicaNotFound)
{
    auto & scenarioTest = GetScenarioTest();
    scenarioTest.ProcessRequestReceiverContextAndDrain<MessageType::ClientReportFaultRequest>(L"z", L"Node1 SL1 1 Permanent");
    scenarioTest.ValidateRequestReceiverContextReply<MessageType::ClientReportFaultReply>(L"z", L"FABRIC_E_REPLICA_DOES_NOT_EXIST Cannot close replica as provided partition does not exist.");
}

BOOST_AUTO_TEST_CASE(MissingReplicaReturnsReplicaNotFound)
{
    // Replica Id on node = 1
    // Replica Id in message = 2
    // Return NOT FOUND
    TestScenarioWithError(
        L"SP1",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"Node1 SP1 2 Permanent",
        ErrorCodeValue::REReplicaDoesNotExist,
        L"Cannot close replica as provided ReplicaId do not match.");
}

BOOST_AUTO_TEST_CASE(TransientFaultForStatelessCompletesWithUnsupported)
{
    TestScenarioWithError(
        L"SL1",
        L"C None 000/000/411 1:1 -",
        L"Node1 SL1 1 Transient",
        ErrorCodeValue::InvalidReplicaOperation,
        L"Non persisted replica cannot be restarted. Please remove this replica.");
}

BOOST_AUTO_TEST_CASE(TransientFaultForStatefulVolatileCompletesWithUnsupported)
{
    TestScenarioWithError(
        L"SV1",
        L"C None 000/000/411 1:1 -",
        L"Node1 SV1 1 Transient",
        ErrorCodeValue::InvalidReplicaOperation,
        L"Non persisted replica cannot be restarted. Please remove this replica.");
}

BOOST_AUTO_TEST_CASE(ForceNotSupportedForRestart)
{
    TestScenarioWithError(
        L"SP1",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"Node1 SP1 1 Transient true",
        ErrorCodeValue::ForceNotSupportedForReplicaOperation,
        L"Force parameter not supported for restarting replica.");
}

BOOST_AUTO_TEST_CASE(ForceNotSupportedForAdHoc)
{
    TestScenarioWithError(
        L"ASP1",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"Node1 ASP1 1 Permanent true",
        ErrorCodeValue::ForceNotSupportedForReplicaOperation,
        L"Cannot close replica using Force for AdHoc type replica.");
}

BOOST_AUTO_TEST_CASE(NotReadyTests)
{
    // Stateless
    // Only Ready and Up and !Closing replicas can be report faulted on
    TestNotReadyScenario(L"SL1", L"C None 000/000/411 1:1 -", L"Node1 SL1 1 Permanent");
    TestNotReadyScenario(L"SL1", L"O None 000/000/411 1:1 S [N/N/N IC U N F 1:1]", L"Node1 SL1 1 Permanent");
    TestNotReadyScenario(L"SL1", L"O None 000/000/411 1:1 CHd [N/N/N ID U N F 1:1]", L"Node1 SL1 1 Permanent");
    TestNotReadyScenario(L"SL1", L"O None 000/000/411 1:1 CHc [N/N/N RD U N F 1:1]", L"Node1 SL1 1 Permanent");

    // Stateful Volatile
    TestNotReadyScenario(L"SV1", L"C None 000/000/411 1:1 -", L"Node1 SV1 1 Permanent");
    TestNotReadyScenario(L"SV1", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]", L"Node1 SV1 1 Permanent");
    TestNotReadyScenario(L"SV1", L"O None 000/000/411 1:1 CHd [N/N/P ID U N F 1:1]", L"Node1 SV1 1 Permanent");
    TestNotReadyScenario(L"SV1", L"O None 000/000/411 1:1 CHc [N/N/P RD U N F 1:1]", L"Node1 SV1 1 Permanent");

    // Stateful Persisted
    TestNotReadyScenario(L"SP1", L"C None 000/000/411 1:1 -", L"Node1 SP1 1 Permanent");
    TestNotReadyScenario(L"SP1", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]", L"Node1 SP1 1 Permanent");
    TestNotReadyScenario(L"SP1", L"O None 000/000/411 1:1 CHd [N/N/P ID U N F 1:1]", L"Node1 SP1 1 Permanent");
    TestNotReadyScenario(L"SP1", L"O None 000/000/411 1:1 CHc [N/N/P RD U N F 1:1]", L"Node1 SP1 1 Permanent");
    TestNotReadyScenario(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]", L"Node1 SP1 1 Permanent");
    TestNotReadyScenario(L"SP1", L"O None 000/000/411 1:1 1 S [N/N/P SB U N F 1:1]", L"Node1 SP1 1 Permanent");
}

BOOST_AUTO_TEST_CASE(SuccessTests)
{
    TestSuccessScenario(
        L"SL1",
        L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]",
        L"Node1 SL1 1 Permanent",
        L"O None 000/000/411 1:1 CMHa [N/N/N ID U N F 1:1");

    TestSuccessScenario(
        L"SV1",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"Node1 SV1 1 Permanent",
        L"O None 000/000/411 1:1 CMHa [N/N/P ID U N F 1:1");

    TestSuccessScenario(
        L"SP1",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"Node1 SP1 1 Permanent",
        L"O None 000/000/411 1:1 CMHa [N/N/P ID U N F 1:1");

    TestSuccessScenario(
        L"SP1",
        L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
        L"Node1 SP1 1 Transient",
        L"O None 000/000/411 1:1 CMHr [N/N/P RD U N F 1:1");
}

BOOST_AUTO_TEST_CASE(Obliterate_DownReplicaTest)
{
    ObliterateWithAppHostDownTestHelper(L"O None 000/000/411 1:1 - [N/N/P RD D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(Obliterate_UpReplicaTest)
{
    ObliterateWithAppHostDownTestHelper(L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(Obliterate_ClosingReplicaTest)
{
    ObliterateWithAppHostDownTestHelper(L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(Obliterate_ICReplicaTest)
{
    ObliterateDirectTestHelper(L"SP1");
}

BOOST_AUTO_TEST_CASE(Obliterate_SystemReplicaTest)
{
    ObliterateDirectTestHelper(L"FM");
}

BOOST_AUTO_TEST_CASE(FTDeleteRacingWithReportFaultCompletesContext)
{
    // TODO: File bug to fix this
    //// In this test enqueue a job item but delete the FT before the job item runs
    //// RA should send a reply for the context
    //auto & scenarioTest = GetScenarioTest();

    //scenarioTest.AddFT(L"SP1", L"O None 000/000/411 1:1 MD [N/N/P SB U N F 1:1]");

    //// Create the context but don't execute
    //scenarioTest.ProcessRequestReceiverContext<MessageType::ClientReportFaultRequest>(L"z", L"Node1 SP1 1 Permanent");
    //scenarioTest.ValidateNoMessages();

    //auto entry = scenarioTest.GetLFUMEntry(L"SP1");
    //entry->OnDeleted();

    //// Now process the job item and verify that the reply is saying as replica not found
    //scenarioTest.DrainJobQueues();
    //scenarioTest.ValidateRequestReceiverContextReply<MessageType::ClientReportFaultReply>(L"z", L"FABRIC_E_REPLICA_DOES_NOT_EXIST");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()


# pragma endregion

namespace RuntimeDown
{
    class TestStateMachine_RuntimeDown
    {
    protected:
        TestStateMachine_RuntimeDown() { BOOST_REQUIRE(TestSetup()); }
        TEST_METHOD_SETUP(TestSetup);
        ~TestStateMachine_RuntimeDown() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_CLEANUP(TestCleanup);

        ScenarioTestHolderUPtr holder_;
    };

    bool TestStateMachine_RuntimeDown::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestStateMachine_RuntimeDown::TestCleanup()
    {
        holder_.reset();
        return true;
    }
    BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_RuntimeDownSuite, TestStateMachine_RuntimeDown)

    BOOST_AUTO_TEST_CASE(RuntimeDownSanityTest)
    {
        auto & test = holder_->ScenarioTestObj;

        test.AddFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
        test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

        test.ProcessRuntimeClosedAndDrain(L"SP1");

        auto & volatileFT = test.GetFT(L"SV1");
        auto & persistedFT = test.GetFT(L"SP1");
        Verify::IsTrueLogOnError(volatileFT.IsOpen && volatileFT.LocalReplicaClosePending.IsSet, wformatString("Volatile should not be closed {0}", volatileFT));
        Verify::IsTrueLogOnError(persistedFT.LocalReplicaClosePending.IsSet, wformatString("Persisted should close {0}", persistedFT));
        test.ValidateNoFMMessages();
    }
    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
}

namespace Deactivate
{
    class TestStateMachine_Deactivate
    {
    protected:
        TestStateMachine_Deactivate() { BOOST_REQUIRE(TestSetup()); }
        TEST_METHOD_SETUP(TestSetup);
        ~TestStateMachine_Deactivate() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_CLEANUP(TestCleanup);

        void AddFT(wstring const & deactivationInfo);
        void ValidateDeactivationInfo(wstring const & expected);
        void ProcessDeactivateWithDeactivationEpochAndDrain();
        void ProcessDeactivateWithoutDeactivationEpochAndDrain();

        ScenarioTestHolderUPtr holder_;
    };

    bool TestStateMachine_Deactivate::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestStateMachine_Deactivate::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    void TestStateMachine_Deactivate::AddFT(wstring const & deactivationInfo)
    {
        auto ftString = wformatString("O None 000/000/411 {0} 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1] [N/N/S RD U N F 3:1]", deactivationInfo);
        holder_->ScenarioTestObj.AddFT(L"SP1", ftString);
    }

    void TestStateMachine_Deactivate::ValidateDeactivationInfo(wstring const & expectedStr)
    {
        auto expected = Reader::ReadHelper<ReplicaDeactivationInfo>(expectedStr);

        auto const & ft = holder_->ScenarioTestObj.GetFT(L"SP1");
        Verify::AreEqualLogOnError(wformatString(expected), wformatString(ft.DeactivationInfo), wformatString("Expected {0}. Actual {1}. FT {2}", expected, ft.DeactivationInfo, ft));
    }
    void TestStateMachine_Deactivate::ProcessDeactivateWithDeactivationEpochAndDrain()
    {
        holder_->ScenarioTestObj.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, L"SP1", L"411/633 {522:7} false [S/S RD U 1:1 3 3] [P/S RD U 2:1 3 3] [S/P RD U 3:1 3 3]");
    }

    void TestStateMachine_Deactivate::ProcessDeactivateWithoutDeactivationEpochAndDrain()
    {
        holder_->ScenarioTestObj.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, L"SP1", L"411/633 {000:-1} false [S/S RD U 1:1 3 3] [P/S RD U 2:1 3 3] [S/P RD U 3:1 3 3]");
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_DeactivateSuite, TestStateMachine_Deactivate)

    BOOST_AUTO_TEST_CASE(DeactivationEpoch_BackwardCompatibility_SwapPrimary_SetsForReadyReplicaIfNotSet)
    {
        holder_->Recreate();

        AddFT(L"{000:-1}");

        ProcessDeactivateWithoutDeactivationEpochAndDrain();

        ValidateDeactivationInfo(L"633:3");
    }

    BOOST_AUTO_TEST_CASE(DeactivationEpoch_BackwardCompatibility_SwapPrimary_UpdatesForReadyReplicaIfSet)
    {
        holder_->Recreate();

        AddFT(L"{411:1}");

        ProcessDeactivateWithoutDeactivationEpochAndDrain();

        ValidateDeactivationInfo(L"633:3");
    }

    BOOST_AUTO_TEST_CASE(DeactivationEpoch_SwapPrimary_SetsFromDeactivationInfo)
    {
        holder_->Recreate();

        AddFT(L"{411:1}");

        ProcessDeactivateWithDeactivationEpochAndDrain();

        ValidateDeactivationInfo(L"522:7");
    }

    BOOST_AUTO_TEST_CASE(Deactivate_SI_Replica)
    {
        auto & test = holder_->Recreate();

        test.AddFT(L"SP1", L"O None 522/000/522 {411:1} 1:1 CM [S/N/I IB U N F 1:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, L"SP1", L"522/522 {522:2} false [P/P RD U 2:2] [S/S RD U 1:1]");

        test.ValidateFT(L"SP1", L"O None 522/522/522 {522:2} 1:1 CM [S/S/I RD U N F 1:1] [P/P/P RD U N F 2:2]");
    }

    BOOST_AUTO_TEST_CASE(Deactivate_NI_Replica_IS_Reconfig)
    {
        auto & test = holder_->Recreate();

        test.AddFT(L"SP1", L"O None 000/000/522 {411:1} 1:1 CM [N/N/I IB U N F 1:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, L"SP1", L"522/522 {522:2} false [P/P RD U 2:2] [I/S RD U 1:1]");

        test.ValidateFT(L"SP1", L"O None 000/000/522 {522:2} 1:1 CM [N/N/I IB U N F 1:1]");
    }

    BOOST_AUTO_TEST_CASE(Deactivate_NI_Replica_SS_Reconfig)
    {
        auto & test = holder_->Recreate();

        test.AddFT(L"SP1", L"O None 000/000/522 {411:1} 1:1 CM [N/N/I IB U N F 1:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, L"SP1", L"522/522 {522:2} false [P/P RD U 2:2] [S/S RD U 1:1]");

        test.ValidateFT(L"SP1", L"O None 522/522/522 {522:2} 1:1 CM [S/S/I RD U N F 1:1] [P/P/P RD U N F 2:2]");
    }

    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
}

namespace RefreshConfiguration
{
    class TestStateMachine_RefreshConfiguration
    {
    protected:
        TestStateMachine_RefreshConfiguration() { BOOST_REQUIRE(TestSetup()); }
        TEST_METHOD_SETUP(TestSetup);
        ~TestStateMachine_RefreshConfiguration() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_CLEANUP(TestCleanup);

        ScenarioTestHolderUPtr holder_;
    };

    bool TestStateMachine_RefreshConfiguration::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestStateMachine_RefreshConfiguration::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_RefreshConfigurationSuite, TestStateMachine_RefreshConfiguration)

    BOOST_AUTO_TEST_CASE(Deactivate_SI_Replica)
    {
        auto & test = holder_->Recreate();

        test.AddFT(L"SP1", L"O None 522/000/522 {411:1} 1:1 CM [S/N/I IB U N F 1:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, L"SP1", L"522/522 {522:2} false [P/P RD U 2:2] [S/S RD U 1:1]");

        test.ValidateFT(L"SP1", L"O None 522/522/522 {522:2} 1:1 CM [S/S/I RD U N F 1:1] [P/P/P RD U N F 2:2]");
    }

    BOOST_AUTO_TEST_CASE(Deactivate_SwapPrimary_Sanity)
    {
        auto & test = holder_->Recreate();
        test.AddFT(L"SP1", L"O None 000/000/411 2:1 CM [N/N/S RD U N F 2:1] [N/N/P RD U N F 1:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(1, L"SP1", L"411/522 {411:0} false [P/S RD U 1:1 -1 0] [S/P RD U 2:1 -1 0]");

        test.ValidateFT(L"SP1", L"O None 411/522/522 {411:0} 2:1 CM [S/P/P RD U N F 2:1] [P/S/S RD U N F 1:1]");
    }

    BOOST_AUTO_TEST_CASE(Deactivate_PSInBuild)
    {
        auto & test = holder_->Recreate();

        test.AddFT(L"SP1", L"O None 411/000/422 {411:0} 2:1 CM [P/N/I IB U N F 2:1] [S/N/S RD U N F 1:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(1, L"SP1", L"411/422 {411:0} false [P/S RD U 2:1] [S/P RD U 1:1]");

        test.ValidateFT(L"SP1", L"O None 411/422/422 {411:0} 2:1 CM [P/S/I RD U N F 2:1] [S/P/P RD U N F 1:1]");
    }

    BOOST_AUTO_TEST_CASE(Activate_PSInBuild)
    {
        auto & test = holder_->Recreate();

        test.AddFT(L"SP1", L"O None 411/422/422 {411:0} 2:1 CM [P/S/I RD U N F 2:1] [S/P/P RD U N F 1:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(1, L"SP1", L"411/422 {411:0} [P/S RD U 2:1] [S/P RD U 1:1]");

        test.ValidateFT(L"SP1", L"O None 411/000/422 {411:0} 2:1 CM [P/N/S RD U RAP F 2:1] [S/N/P RD U N F 1:1]");

        test.ValidateRAPMessage<MessageType::UpdateConfiguration>(L"SP1");
        test.ValidateNoRAMessage(1);
    }

    BOOST_AUTO_TEST_CASE(Activate_IS_Sanity)
    {
        auto & test = holder_->Recreate();
        test.AddFT(L"SP1", L"O None 000/000/411 2:1 CM [N/N/I IB U N F 2:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(1, L"SP1", L"411/412 {411:0} [I/S RD U 2:1] [P/P RD U 1:1]");

        test.ValidateFT(L"SP1", L"O None 411/000/412 {411:0} 2:1 CM [I/N/S RD U RAP F 2:1] [P/N/P RD U N F 1:1]");
    }

    BOOST_AUTO_TEST_CASE(Activate_SS_Sanity)
    {
        auto & test = holder_->Recreate();

        test.AddFT(L"SP1", L"O None 411/412/412 2:1 CM [S/S/I IB U N F 2:1] [P/P/P RD U N F 1:1]");

        test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(1, L"SP1", L"411/412 {411:0} [S/S RD U 2:1] [P/P RD U 1:1]");

        test.ValidateFT(L"SP1", L"O None 411/000/412 2:1 CM [S/N/S RD U RAP F 2:1] [P/N/P RD U N F 1:1]");

        test.ValidateRAPMessage<MessageType::UpdateConfiguration>(L"SP1");
        test.ValidateNoRAMessage(1);
    }

    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
}

namespace Regression
{
    class TestStateMachine_Regression : public StateMachineTestBase
    {
    protected:
        TestStateMachine_Regression() : StateMachineTestBase(static_cast<int>(UnitTestContext::Option::StubJobQueueManager)) {}
    };

    BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_RegressionSuite, TestStateMachine_Regression)

    BOOST_AUTO_TEST_CASE(AddReplica)
    {
        Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

        Test.ProcessFMMessageAndDrain<MessageType::AddReplica>(L"SP1", L"000/411 [N/I IB U 2:1]");

        Test.ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I IC U N F 2:1]");
    }

    BOOST_AUTO_TEST_CASE(FMNotReachingTargetReplicaSetSizeAfterRebuild)
    {
        // Regression test for 2101495
        /*
        RA on node 40 : 130526528450753792 received NodeUpAck from FMM : NodeVersionInstance = 1.0.960.0 : __default__ : 0. IsNodeActivated = true.ActivationSequenceNumber = 0
        [ActivityId:a1352206 - 2188 - 4c5c - bc99 - b675ec12ab01 : 18729]


        RA on node 40 : 130526528450753792 ActivationStateChange.IsActivated = true.CurrentSequenceNumber = 0. IsFmm = true

        2014 - 8 - 16 08 : 54 : 05.497 RA.FT@00000000 - 0000 - 0000 - 0000 - 000000000001 3456 RA on node 40:130526528450753792 NodeUpAck
        [New] 00000000 - 0000 - 0000 - 0000 - 000000000001 Opening None 0 : 0 / 0 : 0 / 130526527380565876 : 200000007 130526527400716870 : 130526527400716871 7 3 0 | [RO N N 0] 0 0 0 0[0 / 40320] 2014 - 08 - 16 08 : 52 : 35.551
        N / N / P SB U N 0 0 0 40 : 130526528450753792 130526527400716870 : 130526527400716871 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 50 : 130526527441018768 130526527471323313 : 130526527471323313 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 60 : 130526527441331177 130526527471323314 : 130526527471323314 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 5 : 130526527484288660 130526527543648119 : 130526527543648119 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 10 : 130526527483820031 130526527543648120 : 130526527543648120 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 20 : 130526527483351403 130526527543648121 : 130526527543648121 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 30 : 130526527483047306 130526527543648122 : 130526527543648122 1.0 : 1.0 : 0 - 1 - 1
        [Old] 00000000 - 0000 - 0000 - 0000 - 000000000001 Down None 0 : 0 / 0 : 0 / 130526527380565876 : 200000007 130526527400716870 : 130526527400716870 7 3 0 | [N N N 0] 0 0 0 0  2014 - 08 - 16 08 : 52 : 35.551
        N / N / P SB D N 0 0 0 40 : 130526528450753792 130526527400716870 : 130526527400716870 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 50 : 130526527441018768 130526527471323313 : 130526527471323313 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 60 : 130526527441331177 130526527471323314 : 130526527471323314 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 5 : 130526527484288660 130526527543648119 : 130526527543648119 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 10 : 130526527483820031 130526527543648120 : 130526527543648120 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 20 : 130526527483351403 130526527543648121 : 130526527543648121 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 30 : 130526527483047306 130526527543648122 : 130526527543648122 1.0 : 1.0 : 0 - 1 - 1

        [Activity:NodeUpAck_FMM]

        RA on node 40 : 130526528450753792 ReplicaUpSender Fmm - initial replica list 1 items.
        (00000000 - 0000 - 0000 - 0000 - 000000000001)

        RA on node 40:130526528450753792 processed message StatefulServiceReopenReply :
        00000000 - 0000 - 0000 - 0000 - 000000000001 0 : 0 / 130526527380565876 : 200000007
        N / P SB U 40 : 130526528450753792 130526527400716870 : 130526527400716871 - 1 - 1 1.0 : 1.0 : 0
        EC : S_OK State : -
        [New] 00000000 - 0000 - 0000 - 0000 - 000000000001 Open None 0 : 0 / 0 : 0 / 130526527380565876 : 200000007 130526527400716870 : 130526527400716871 7 3 0 | [N N ReplicaUp 0] 0 0 0 0  2014 - 08 - 16 08 : 52 : 35.551
        N / N / P SB U N 0 0 0 40 : 130526528450753792 130526527400716870 : 130526527400716871 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 50 : 130526527441018768 130526527471323313 : 130526527471323313 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 60 : 130526527441331177 130526527471323314 : 130526527471323314 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 5 : 130526527484288660 130526527543648119 : 130526527543648119 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 10 : 130526527483820031 130526527543648120 : 130526527543648120 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 20 : 130526527483351403 130526527543648121 : 130526527543648121 1.0 : 1.0 : 0 - 1 - 1
        N / N / S RD U N 0 0 0 30 : 130526527483047306 130526527543648122 : 130526527543648122 1.0 : 1.0 : 0 - 1 - 1

        [Activity:a1352206 - 2188 - 4c5c - bc99 - b675ec12ab01 : 19161]


        RA on node 40:130526528450753792 processed message ReplicaCloseReply :
        00000000 - 0000 - 0000 - 0000 - 000000000001 0 : 0 / 130526527380565876 : 200000007
        N / N RD U 40 : 130526528450753792 130526527400716870 : 130526527400716871 - 1 - 1 0.0 : 0.0 : 0
        EC : S_OK State : D |
        [New] 00000000 - 0000 - 0000 - 0000 - 000000000001 Closed None 0 : 0 / 0 : 0 / 130526527380565876 : 200000007 130526527400716870 : 130526527400716871 7 3 0 | [N N N 1] 0 0 0 0  2014 - 08 - 16 08 : 52 : 35.551

        [Activity:a1352206 - 2188 - 4c5c - bc99 - b675ec12ab01 : 19467]


        RA on node 40:130526528450753792 sending message DeleteReplicaReply to FMM, body :
        00000000 - 0000 - 0000 - 0000 - 000000000001 0 : 0 / 130526527380565876 : 200000007 N / N DD D 40 : 130526528450753792 130526527400716870 : 130526527400716871 - 1 - 1 0.0 : 0.0 : 0
        EC : S_OK
        [Activity:a1352206 - 2188 - 4c5c - bc99 - b675ec12ab01

        */

        // Regression test for RDBug 2237778: LastReplicaUp does not complete for replicas that are dropped due to FM rebuild if the FM has forgotten about the replica

        /* GenerationUpdate if it causes a drop of the FM replica should mark that it has been processed for LastReplicaUp */
        Test.SetNodeUpAckProcessed(false, false);
        Test.SetFindServiceTypeRegistrationSuccess(L"FM");
        Test.UTContext.Config.PerNodeMinimumIntervalBetweenMessageToFM = TimeSpan::Zero;

        Test.AddFT(L"FM", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
        Test.DumpFT(L"FM");

        Test.ExecuteUpdateStateOnLfumLoad(L"FM");
        Test.DumpFT(L"FM");

        Test.ProcessNodeUpAckAndDrain(L"true 2", true);
        Test.DumpFT(L"FM");

        Test.RequestWorkAndDrain(Test.StateItemHelpers.ReplicaOpenRetryHelper);

        Test.ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(L"FM", L"000/411 [N/P SB U 1:2] Success");
        Test.DumpFT(L"FM");

        int initialProposedGeneration = 4, initialSendGeneration = 2, initialReceiveGeneration = 3;
        Test.GenerationStateHelper.SetGeneration(GenerationNumberSource::FM, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);
        Test.GenerationStateHelper.SetGeneration(GenerationNumberSource::FMM, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);

        Test.ProcessFMMessageAndDrain<MessageType::GenerationUpdate>(L"g:1:4,fm 633", CreateNodeInstanceEx(2, 1));
        Test.DumpFT(L"FM");

        Test.ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(L"FM", L"000/411 [N/P SB U 1:2] Success D");
        Test.DumpFT(L"FM");

        // in the original test a delete replica was received
        // just directly invoke delete        
        Test.ExecuteJobItemHandler<JobItemContextBase>(
            L"FM",
            [&](HandlerParameters & handlerParameters, JobItemContextBase &)
        {            
            handlerParameters.RA.CloseLocalReplica(handlerParameters, ReplicaCloseMode::Delete, ReconfigurationAgent::InvalidNode, ActivityDescription::Empty);
            return true;
        });

        Test.DrainJobQueues();
        Test.DumpFT(L"FM");

        Test.StateItemHelpers.LastReplicaUpHelper.ValidateToBeUploadedReplicaSetIsEmpty(*FailoverManagerId::Fmm);
    }

    BOOST_AUTO_TEST_CASE(StaleActivateReplyShouldNotBeProcessed)
    {
        /*
            Regression Test: 1982577
            1. Swap primary gets cancelled because the new primary restarts and FM keeps it as SB
            2. Activate reply for P/S SB is delayed
            3. FM builds it as a secondary
            4. Old activate reply should not be processed
        */

        Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [P/S/S SB U RA F 2:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, L"SP1", L"000/422 [N/S SB U 2:1] Success");
        Test.DumpFT(L"SP1");

        Test.ProcessFMMessageAndDrain<MessageType::AddReplica>(L"SP1", L"000/422 [N/S IB U 2:1]");
        Test.DumpFT(L"SP1");

        // CreateReplicaReply
        Test.ProcessFMMessageAndDrain<MessageType::CreateReplicaReply>(L"SP1", L"422/422 [S/I IB U 2:1] Success");
        Test.DumpFT(L"SP1");

        // BuildIdle reply
        Test.ProcessRAPMessageAndDrain<MessageType::ReplicatorBuildIdleReplicaReply>(L"SP1", L"000/422 [N/P RD U 1:1] [N/I IB U 2:1] Success -");
        Test.DumpFT(L"SP1");

        // This message should be dropped
        Test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, L"SP1", L"000/422 [N/S SB U 2:1] Success");
        Test.ValidateFT(L"SP1", L"O None 000/000/422 1:1 CM [N/N/P RD U N F 1:1] [N/N/S IB U RA A 2:1]");
    }

    BOOST_AUTO_TEST_CASE(AddPrimaryIsProcessed_LowerEpoch_ReplicaDeleted)
    {
        // Regression test: 1227380
        Test.AddFT(L"SP1", L"C None 000/000/633 1:1 PL");

        Test.ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"SP1", L"411/422 [N/P IB U 1:1:2:2]");

        Test.ValidateFT(L"SP1", L"O None 411/000/422 2:2 S [N/N/P IC U N F 1:1:2:2]");
    }

    BOOST_AUTO_TEST_CASE(CreateReplicaIsProcessed_LowerEpoch_ReplicaDeleted)
    {
        Test.AddFT(L"SP1", L"C None 000/000/522 1:1 PL");

        Test.ProcessFMMessageAndDrain<MessageType::CreateReplica>(L"SP1", L"000/411 [N/I IB U 1:1:2:2]");

        Test.ValidateFT(L"SP1", L"O None 000/000/411 2:2 S [N/N/I IC U N F 1:1:2:2]");
    }

    BOOST_AUTO_TEST_CASE(StaleUCReplyShouldNotResetUpdateReplicatorConfiguration)
    {
        Test.AddFT(L"SP1", L"O Phase4_Activate 411/412/412 1:1 CMB [P/P/P RD U RAP F 1:1] [S/S/S RD U N F 2:1] [S/S/S RD U N F 3:1]");

        Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [S/S IB U 2:1] [S/S RD U 3:1] Success ER");

        // The bug was that the S/S IB would be treated as the replicator config being updated
        Test.ValidateFT(L"SP1", L"O Phase4_Activate 411/412/412 1:1 CMB [P/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [S/S/S RD U N F 3:1]");

        // Subsequent retry should complete the reconfig
        Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [S/S RD U 2:1] [S/S RD U 3:1] Success ER");
        Test.ValidateFT(L"SP1", L"O None 000/000/412 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD U N F 3:1]");
    }

    BOOST_AUTO_TEST_CASE(RefreshConfigurationShouldTakeTheCorrectPC)
    {

        /*
            1. Replica set is stable
            2. FM starts reconfig from 20006/30007
            [P/I 40 D] [S/S 20] [S/P 60] [S/S 10] [S/S 30]
            3. This reconfiguration completes on node 30. Node 30 state is:
            0000/30007 [N/S 20] [N/P 60] [N/S 10] [N/S 30]
            4. 60 goes down before the reconfiguration is fully completed.
            FM starts reconfiguraiton from 20006/30007
            [P/I 40 D] [S/P 20] [S/S 60 D] [S/S 10] [S/S 30]
            this reconfiguration completes catchup and then enters phase3 deactivate
            On node 30, deactivate is processed as below

            RA on node 30:130735978923460755 processed message Deactivate:
            00000000-0000-0000-0000-000000000001 130735978993505201:200000006/130735978993505201:400000008 1
            S/P RD U 20:130735978920767698 130735979042260037:130735979042260037 115 178 1.0:1.0:0
            P/I RD D 40:130735978924413265 130735979027942524:130735979027942524 -1 -1 1.0:1.0:0
            S/S RD D 60:130735978926921169 130735979042260038:130735979042260038 -1 -1 1.0:1.0:0
            S/S RD U 10:130735978920298889 130735979846624064:130735979846624064 118 178 1.0:1.0:0
            S/S RD U 30:130735978923460755 130735979846624065:130735979846624065 119 178 1.0:1.0:0
            0
            [New] 00000000-0000-0000-0000-000000000001 Available None 130735978993505201:300000007/130735978993505201:400000008/130735978993505201:400000008 130735979846624065:130735979846624065 5 3 1 | [N N N 0] 0 0 0 0  [130735978993505201:400000008] 2015-04-15 18:59:44.724
            S/S/S RD U N 0 0 0 30:130735978923460755 130735979846624065:130735979846624065 1.0:1.0:0 -1 -1 0:0
            S/P/P RD U N 0 0 0 20:130735978920767698 130735979042260037:130735979042260037 1.0:1.0:0 -1 -1 0:0
            P/S/S RD D N 0 0 0 60:130735978926921169 130735979042260038:130735979042260038 1.0:1.0:0 -1 -1 0:0
            S/S/S RD U N 0 0 0 10:130735978920298889 130735979846624064:130735979846624064 1.0:1.0:0 -1 -1 0:0
            P/I/I RD D N 0 0 0 40:130735978924413265 130735979027942524:130735979027942524 1.0:1.0:0 -1 -1 0:0
            [Old] 00000000-0000-0000-0000-000000000001 Available None 0:0/0:0/130735978993505201:300000007 130735979846624065:130735979846624065 5 3 1 | [N N N 0] 0 0 0 0  [0:0] 2015-04-15 18:59:44.724
            N/N/S RD U N 0 0 0 30:130735978923460755 130735979846624065:130735979846624065 1.0:1.0:0 -1 -1 0:0
            N/N/S RD U N 0 0 0 20:130735978920767698 130735979042260037:130735979042260037 1.0:1.0:0 -1 -1 0:0
            N/N/P RD U N 0 0 0 60:130735978926921169 130735979042260038:130735979042260038 1.0:1.0:0 -1 -1 0:0
            N/N/S RD U N 0 0 0 10:130735978920298889 130735979846624064:130735979846624064 1.0:1.0:0 -1 -1 0:0

            [Activity: 2f678d90-70ed-4f3c-bb8a-a878b837c47a:37042]
        */
        Test.AddFT(L"SP1", L"O None 000/000/422 3:1 CM [N/N/S RD U N F 3:1] [N/N/S RD U N F 2:1] [N/N/P RD U N F 6:1] [N/N/S RD U N F 1:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, L"SP1", L"311/533 {533:0} false [S/P RD U 2:1] [P/I RD D 4:1] [S/S RD D 6:1] [S/S RD U 1:1] [S/S RD U 3:1]");

        Test.DumpFT(L"SP1");

        Test.ValidateFT(L"SP1", L"O None 311/533/533 3:1 CM [S/S/S RD U N F 3:1] [S/P/P RD U N F 2:1] [P/I/I RD D N F 4:1] [S/S/S RD D N F 6:1] [S/S/S RD U N F 1:1]");
    }

    BOOST_AUTO_TEST_CASE(RefreshConfigurationShouldConsiderCCIfIncomingMessageHasNoPC)
    {
        /*
        1. Node has an older epoch 2f00056 and comes up. 
           RA on node 190:130931340076428050 NodeUpAck
	            [New] 983904a4-8125-45c1-aa72-db5cf65bbe39 Opening None 0:0/0:0/130931160133391274:2f00000056 130931160133391274:2f0000004e:0 130931295494215883:130931295494215885 5 5 0 | [RO N N 0] 0 0 0  2015-11-27 21:40:07.892
	            N/N/S SB U N 0 0 0 190:130931340076428050 130931295494215883:130931295494215885 1.0:1.0:0 -1 -1 0:0:-1
	            N/N/P RD U N 0 0 0 1b0:130931157572761370 130931275016069619:130931275016069620 1.0:1.0:0 -1 -1 0:0:-1
	            N/N/S RD U N 0 0 0 70:130931157572917596 130931160160395009:130931160160395010 1.0:1.0:0 -1 -1 0:0:-1
	            N/N/S RD U N 0 0 0 160:130931316318548242 130931266543090407:130931266543090410 1.0:1.0:0 -1 -1 0:0:-1
	            N/N/S RD U N 0 0 0 1f0:130931325607357039 130931328124007739:130931328124007739 1.0:1.0:0 -1 -1 0:0:-1
        
        2. Primary performs the following reconfig in which 190 is down
            RA on node 1b0:130931157572761370 processed message DoReconfiguration:
            983904a4-8125-45c1-aa72-db5cf65bbe39 130931160133391274:2f00000056/130931160133391274:2f00000057 0
            I/S RD U 120:130931328202206911 130931295494215882:130931295494215885 -1 -1 1.0:1.0:0
            S/S RD U 70:130931157572917596 130931160160395009:130931160160395010 -1 -1 1.0:1.0:0
            P/P RD U 1b0:130931157572761370 130931275016069619:130931275016069620 -1 -1 1.0:1.0:0
            S/I RD D 160:130931316318548242 130931266543090407:130931266543090410 -1 -1 1.0:1.0:0
            S/S RD D 190:130931325365675419 130931295494215883:130931295494215884 -1 -1 1.0:1.0:0
            S/S RD D 1f0:130931325607357039 130931328124007739:130931328124007739 -1 -1 1.0:1.0:0

            [New] 983904a4-8125-45c1-aa72-db5cf65bbe39 Available Phase3_Deactivate 130931160133391274:2f00000056/130931160133391274:2f00000057/130931160133391274:2f00000057 130931160133391274:2f0000004e:0 130931275016069619:130931275016069620 5 5 0 | [N N N 0] 0 0 0  2015-11-27 21:39:51.131
            P/P/P RD U N 0 0 0 1b0:130931157572761370 130931275016069619:130931275016069620 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S RD U N 0 0 0 70:130931157572917596 130931160160395009:130931160160395010 1.0:1.0:0 -1 -1 0:0:-1
            S/I/I RD D RA 0 0 0 160:130931316318548242 130931266543090407:130931266543090410 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S RD D RA 0 0 0 190:130931325365675419 130931295494215883:130931295494215884 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S RD D RA 0 0 0 1f0:130931325607357039 130931328124007739:130931328124007739 1.0:1.0:0 -1 -1 0:0:-1
            I/S/S RD U N 0 0 0 120:130931328202206911 130931295494215882:130931295494215885 1.0:1.0:0 -1 -1 0:0:-1


        2. Primary has moved on to epoch 2f00057 where it removed a down replica from 160 its view.
                [New] 983904a4-8125-45c1-aa72-db5cf65bbe39 Available None 0:0/0:0/130931160133391274:2f00000057 130931160133391274:2f0000004e:0 130931275016069619:130931275016069620 5 5 0 | [N N N 0] 0 0 0  2015-11-27 21:40:13.051
                N/N/P RD U N 0 0 0 1b0:130931157572761370 130931275016069619:130931275016069620 1.0:1.0:0 -1 -1 0:0:-1
                N/N/S RD U N 0 0 0 70:130931157572917596 130931160160395009:130931160160395010 1.0:1.0:0 -1 -1 0:0:-1
                N/N/S RD D N 0 0 0 190:130931325365675419 130931295494215883:130931295494215884 1.0:1.0:0 -1 -1 0:0:-1
                N/N/S SB U N 0 0 0 1f0:130931340062713476 130931328124007739:130931328124007740 1.0:1.0:0 -1 -1 0:0:-1
                N/N/S RD U N 0 0 0 120:130931328202206911 130931295494215882:130931295494215885 1.0:1.0:0 -1 -1 0:0:-1

        3. FM now performs AddSecondary and replica on 190 receives the following
                RA on node 190:130931340076428050 processed message Activate:
                983904a4-8125-45c1-aa72-db5cf65bbe39 0:0/130931160133391274:2f00000057 0
                N/P RD U 1b0:130931157572761370 130931275016069619:130931275016069620 -1 -1 1.0:1.0:0
                N/S RD U 70:130931157572917596 130931160160395009:130931160160395010 -1 -1 1.0:1.0:0
                N/S RD U 190:130931340076428050 130931295494215883:130931295494215885 -1 -1 1.0:1.0:0
                N/S RD U 1f0:130931340062713476 130931328124007739:130931328124007740 -1 -1 1.0:1.0:0
                N/S RD U 120:130931328202206911 130931295494215882:130931295494215885 -1 -1 1.0:1.0:0
                130931160133391274:2f0000004e:0
                [New] 983904a4-8125-45c1-aa72-db5cf65bbe39 Available None 130931160133391274:2f00000056/0:0/130931160133391274:2f00000057 130931160133391274:2f0000004e:0 130931295494215883:130931295494215885 5 5 0 | [N N N 0] 0 0 0  2015-11-27 21:40:07.892
                I/N/S RD U RAP 0 0 0 190:130931340076428050 130931295494215883:130931295494215885 1.0:1.0:0 -1 -1 0:0:-1
                P/N/P RD U N 0 0 0 1b0:130931157572761370 130931275016069619:130931275016069620 1.0:1.0:0 -1 -1 0:0:-1
                S/N/S RD U N 0 0 0 70:130931157572917596 130931160160395009:130931160160395010 1.0:1.0:0 -1 -1 0:0:-1
                S/N/N RD U N 0 0 0 160:130931316318548242 130931266543090407:130931266543090410 1.0:1.0:0 -1 -1 0:0:-1
                S/N/S RD U N 0 0 0 1f0:130931340062713476 130931328124007739:130931328124007740 1.0:1.0:0 -1 -1 0:0:-1
                [Old] 983904a4-8125-45c1-aa72-db5cf65bbe39 Available None 130931160133391274:2f00000056/0:0/130931160133391274:2f00000057 130931160133391274:2f0000004e:0 130931295494215883:130931295494215885 5 5 0 | [N N N 0] 0 0 0  2015-11-27 21:40:07.892
                S/N/I IB U N 0 0 0 190:130931340076428050 130931295494215883:130931295494215885 1.0:1.0:0 -1 -1 0:0:-1
                P/N/P RD U N 0 0 0 1b0:130931157572761370 130931275016069619:130931275016069620 1.0:1.0:0 -1 -1 0:0:-1
                S/N/S RD U N 0 0 0 70:130931157572917596 130931160160395009:130931160160395010 1.0:1.0:0 -1 -1 0:0:-1
                S/N/S RD U N 0 0 0 160:130931316318548242 130931266543090407:130931266543090410 1.0:1.0:0 -1 -1 0:0:-1
                S/N/S RD U N 0 0 0 1f0:130931325607357039 130931328124007739:130931328124007739 1.0:1.0:0 -1 -1 0:0:-1

                [Activity: c3c40ddc-6110-4021-a7da-7a0c22863b2f:4344102]

        In the above state transition the replica has pc 2f0056 and the incoming message has cc 2f00057. It should update its cc to include the replica on node 

        */
        // 1b0 - 1, 190 - 2, 70 - 3, 160 - 4, 1f0 -> 5, 120 - 6

        Test.AddFT(L"SP1", L"O None 411/000/412 2:1 CM [S/N/I IB U N F 2:1] [P/N/P RD U N F 1:1] [S/N/S RD U N F 3:1] [S/N/S RD U N F 4:1] [S/N/S RD U N F 5:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(1, L"SP1", L"000/412 {411:1} [N/P RD U 1:1] [N/S RD U 3:1] [N/S RD U 2:1] [N/S RD U 5:1] [N/S RD U 6:1]");

        Test.ValidateFT(L"SP1", L"O None 412/000/412 2:1 CM (sender:1) [S/N/S RD U RAP F 2:1] [P/N/P RD U N F 1:1] [S/N/S RD U N F 3:1] [S/N/S RD U N F 5:1] [S/N/S RD U N F 6:1]");
    }

    BOOST_AUTO_TEST_CASE(DeactivateShouldNotHaveMultiplePrimaries)
    {
        //00000000 - 0000 - 0000 - 0000 - 000000000001 130853853049714047:200000006 / 130853853049714047 : 400000008 1
        //    S / P RD U 30 : 130853852887320265 130853854030623385 : 130853854030623385 128 185 1.0 : 1.0 : 0
        //    P / I RD D 40 : 130853852887947382 130853853055718382 : 130853853055718382 - 1 - 1 1.0 : 1.0 : 0
        //    S / S RD U 20 : 130853852886194671 130853853119387457 : 130853853119387457 122 185 1.0 : 1.0 : 0
        //    S / S RD D 60 : 130853852895569512 130853853119387458 : 130853853119387458 - 1 - 1 1.0 : 1.0 : 0
        //    S / S RD U 10 : 130853852883520432 130853854030623384 : 130853854030623384 128 185 1.0 : 1.0 : 0
        //    0 130853853049714047 : 400000008 : 185
        //    [New] 00000000 - 0000 - 0000 - 0000 - 000000000001 Available None 130853853049714047 : 300000007 / 130853853049714047 : 400000008 / 130853853049714047 : 400000008 130853853049714047 : 400000008 : 185 130853853119387457 : 130853853119387457 5 3 1 | [N N N 0] 0 0 0  2015 - 08 - 30 05 : 15 : 12.174
        //    S / S / S RD U N 0 0 0 20 : 130853852886194671 130853853119387457 : 130853853119387457 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1
        //    P / S / S RD D N 0 0 0 60 : 130853852895569512 130853853119387458 : 130853853119387458 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1
        //    S / P / P RD U N 0 0 0 30 : 130853852887320265 130853854030623385 : 130853854030623385 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1
        //    S / S / S RD U N 0 0 0 10 : 130853852883520432 130853854030623384 : 130853854030623384 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1
        //    P / I / I RD D N 0 0 0 40 : 130853852887947382 130853853055718382 : 130853853055718382 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1
        //    [Old] 00000000 - 0000 - 0000 - 0000 - 000000000001 Available None 130853853049714047 : 300000007 / 0 : 0 / 130853853049714047 : 400000008 130853853049714047 : 300000007 : 185 130853853119387457 : 130853853119387457 5 3 1 | [N N N 0] 0 0 0  2015 - 08 - 30 05 : 15 : 12.174
        //    S / N / S RD U N 0 0 0 20 : 130853852886194671 130853853119387457 : 130853853119387457 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1
        //    P / N / P RD U N 0 0 0 60 : 130853852895569512 130853853119387458 : 130853853119387458 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1
        //    S / N / S RD U N 0 0 0 30 : 130853852887320265 130853854030623385 : 130853854030623385 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1
        //    S / N / S RD U N 0 0 0 10 : 130853852883520432 130853854030623384 : 130853854030623384 1.0 : 1.0 : 0 - 1 - 1 0 : 0 : -1

        Test.AddFT(L"SP1", L"O None 533/000/644 2:1 CM [S/N/S RD U N F 2:1] [P/N/P RD U N F 6:1] [S/N/S RD U N F 3:1] [S/N/S RD U N F 1:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(3, L"SP1", L"422/644 {422:0} false [S/P RD U 3:1] [P/I RD D 4:1] [S/S RD U 2:1] [S/S RD U 6:1] [S/S RD U 1:1]");

        Test.DumpFT(L"SP1");

        Test.ValidateFT(L"SP1", L"O None 422/644/644 2:1 CM [S/S/S RD U N F 2:1] [S/P/P RD U N F 3:1] [P/I/I RD D N F 4:1] [S/S/S RD U N F 6:1] [S/S/S RD U N F 1:1]");
    }

    BOOST_AUTO_TEST_CASE(DeactivateShouldNotHaveMultiplePrimaries2)
    {
        /*
        RA on node 500:130883124622673835 start processing message GetLSN [d44830e2-b145-479d-8e67-e6e375b06623:20347] with body 
	        2109b6db-7cea-4335-83c0-80865f3338de 130883124359825645:200000004/130883124359825645:400000006 0 
	        N/N RD U 500:130883124622673835 130883124378289413:130883124378289414 -1 -1 0.0:0.0:0
	        fabric:/test:TestPersistedStoreServiceType@ (1.0:1.0:0) [1]/3/1 SP 130883124359694254:0 05.000 Infinite 7:0:00:00.000 ()  0 () 1 () false FT: 
	        [New] 2109b6db-7cea-4335-83c0-80865f3338de Open None 130883124359825645:300000005/0:0/130883124359825645:400000006 130883124359825645:300000005:0 130883124378289413:130883124378289414 3 1 0 | [N N N 0] 0 0 0  2015-10-03 02:21:03.201
	        P/N/P SB U N 0 0 0 500:130883124622673835 130883124378289413:130883124378289414 1.0:1.0:0 -1 -1 0:0:-1
	        S/N/S RD U N 0 0 0 400:130883124207209111 130883124378289412:130883124378289412 1.0:1.0:0 -1 -1 0:0:-1
	        [Old] 2109b6db-7cea-4335-83c0-80865f3338de Open None 0:0/0:0/130883124359825645:300000005 130883124359825645:300000005:0 130883124378289413:130883124378289414 3 1 0 | [N N N 0] 0 0 0  2015-10-03 02:21:03.201
	        N/N/P SB U N 0 0 0 500:130883124622673835 130883124378289413:130883124378289414 1.0:1.0:0 -1 -1 0:0:-1
	        N/N/S RD U N 0 0 0 400:130883124207209111 130883124378289412:130883124378289412 1.0:1.0:0 -1 -1 0:0:-1
	

        RA on node 500:130883124622673835 start processing message CreateReplica [d44830e2-b145-479d-8e67-e6e375b06623:20439] with body 
	        2109b6db-7cea-4335-83c0-80865f3338de 130883124359825645:200000004/130883124359825645:400000006 0 
	        S/S IB U 500:130883124622673835 130883124378289413:130883124378289414 0 0 1.0:1.0:0
	        fabric:/test:TestPersistedStoreServiceType@ (1.0:1.0:0) [1]/3/1 SP 130883124359694254:0 05.000 Infinite 7:0:00:00.000 ()  0 () 1 () false 130883124359825645:300000005:0 FT: 
	        [New] 2109b6db-7cea-4335-83c0-80865f3338de Opening None 130883124359825645:300000005/0:0/130883124359825645:400000006 130883124359825645:300000005:0 130883124378289413:130883124378289414 3 1 0 | [CR N N 0] 0 0 0  2015-10-03 02:21:03.201
	        P/N/I IC U N 0 0 0 500:130883124622673835 130883124378289413:130883124378289414 1.0:1.0:0 -1 -1 0:0:-1
	        S/N/S RD U N 0 0 0 400:130883124207209111 130883124378289412:130883124378289412 1.0:1.0:0 -1 -1 0:0:-1
	        [Old] 2109b6db-7cea-4335-83c0-80865f3338de Open None 130883124359825645:300000005/0:0/130883124359825645:400000006 130883124359825645:300000005:0 130883124378289413:130883124378289414 3 1 0 | [N N N 0] 0 0 0  2015-10-03 02:21:03.201
	        P/N/P SB U N 0 0 0 500:130883124622673835 130883124378289413:130883124378289414 1.0:1.0:0 -1 -1 0:0:-1
	        S/N/S RD U N 0 0 0 400:130883124207209111 130883124378289412:130883124378289412 1.0:1.0:0 -1 -1 0:0:-1
	


        RA on node 500:130883124622673835 start processing message Deactivate [d44830e2-b145-479d-8e67-e6e375b06623:22043] with body 
	        2109b6db-7cea-4335-83c0-80865f3338de 130883124359825645:200000004/130883124359825645:400000006 0 
	        S/P RD U 400:130883124207209111 130883124378289412:130883124378289412 0 0 1.0:1.0:0
	        P/I RD D 600:130883124207334705 130883124368280451:130883124368280451 -1 -1 1.0:1.0:0
	        S/S RD U 500:130883124622673835 130883124378289413:130883124378289414 0 0 1.0:1.0:0
	        0 130883124359825645:300000005:0 FT: 
	        [New] 2109b6db-7cea-4335-83c0-80865f3338de Available None 130883124359825645:300000005/130883124359825645:400000006/130883124359825645:400000006 130883124359825645:300000005:0 130883124378289413:130883124378289414 3 1 0 | [N N N 0] 0 0 0  2015-10-03 02:21:03.201
	        P/S/I RD U N 0 0 0 500:130883124622673835 130883124378289413:130883124378289414 1.0:1.0:0 -1 -1 0:0:-1
	        S/P/P RD U N 0 0 0 400:130883124207209111 130883124378289412:130883124378289412 1.0:1.0:0 -1 -1 0:0:-1
	        P/I/I RD D N 0 0 0 600:130883124207334705 130883124368280451:130883124368280451 1.0:1.0:0 -1 -1 0:0:-1
	        [Old] 2109b6db-7cea-4335-83c0-80865f3338de Available None 130883124359825645:300000005/0:0/130883124359825645:400000006 130883124359825645:300000005:0 130883124378289413:130883124378289414 3 1 0 | [N N N 0] 0 0 0  2015-10-03 02:21:03.201
	        P/N/I IB U N 0 0 0 500:130883124622673835 130883124378289413:130883124378289414 1.0:1.0:0 -1 -1 0:0:-1
	        S/N/S RD U N 0 0 0 400:130883124207209111 130883124378289412:130883124378289412 1.0:1.0:0 -1 -1 0:0:-1
	    */

        Test.AddFT(L"SP1", L"O None 533/000/644 500:1 CM [P/N/I IB U N F 500:1] [S/N/S RD U N F 400:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(3, L"SP1", L"422/644 {422:0} false [S/P RD U 400:1] [P/I RD D 600:1] [S/S RD U 500:1]");

        Test.DumpFT(L"SP1");

        Test.ValidateFT(L"SP1", L"O None 422/644/644 500:1 CM [S/S/I RD U N F 500:1] [S/P/P RD U N F 400:1] [P/I/I RD D N F 600:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(3, L"SP1", L"422/644 {422:0} [S/P RD U 400:1] [P/I RD D 600:1] [S/S RD U 500:1]");

        Test.DumpFT(L"SP1");
    }

    BOOST_AUTO_TEST_CASE(ActivateShouldClearConfiguration)
    {
        /*
            2015-12-2 18:40:28.850	RA.FT@0f8ec1f5-4b86-471b-94fa-bb866426f3d5	6712	RA on node 100:130935511391433044 processed message ReplicaOpenReply:
            0f8ec1f5-4b86-471b-94fa-bb866426f3d5 130935550940751528:1100000088/130935550940751528:130000008c
            S/I IB U 100:130935511391433044 130935551047647995:130935551047647996 -1 -1 1.0:1.0:0
            State: - EC: S_OK:''
            [New] 0f8ec1f5-4b86-471b-94fa-bb866426f3d5 Available None 130935550940751528:1100000088/0:0/130935550940751528:130000008c 130935550940751528:1100000087:31626 130935551047647995:130935551047647996 5 1 0 | [N N N 0] 0 0 0  2015-12-02 18:40:16.619
            S/N/I IB U N 0 0 0 100:130935511391433044 130935551047647995:130935551047647996 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 10:130935550599465377 130935495840300056:130935495840300058 1.0:1.0:0 -1 -1 0:0:-1
            P/N/P RD D N 0 0 0 70:130935476462446278 130935532129893072:130935532129893073 1.0:1.0:0 -1 -1 0:0:-1

            [Activity: ad18004e-53a1-41fa-88c4-cde17047015a:11693470]

            2015-12-2 18:40:28.897	RA.FT@0f8ec1f5-4b86-471b-94fa-bb866426f3d5	1924	RA on node 90:130935542867603363 processed message ReplicatorBuildIdleReplicaReply:
            0f8ec1f5-4b86-471b-94fa-bb866426f3d5 130935550940751528:130000008b/130935550940751528:130000008c
            P/P RD U 90:130935542867603363 130935542969209766:130935542969209767 -1 -1 1.0:1.0:0
            S/I IB U 100:130935511391433044 130935551047647995:130935551047647996 -1 -1 1.0:1.0:0
            State: - EC: S_OK:''
            [New] 0f8ec1f5-4b86-471b-94fa-bb866426f3d5 Available Phase4_Activate 130935550940751528:130000008b/130935550940751528:130000008c/130935550940751528:130000008c 130935550940751528:130000008b:31660 130935542969209766:130935542969209767 5 1 0 | [N N N 0] 0 0 0  2015-12-02 18:40:28.819
            P/P/P RD U RAP 0 0 0 90:130935542867603363 130935542969209766:130935542969209767 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S RD U RA 0 0 0 10:130935550599465377 130935495840300056:130935495840300058 1.0:1.0:0 -1 -1 0:0:-1
            S/I/I RD D N 0 0 0 70:130935476462446278 130935532129893072:130935532129893074 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S IB U RA 0 1 0 100:130935511391433044 130935551047647995:130935551047647996 1.0:1.0:0 -1 -1 0:0:-1

            [Activity: ad18004e-53a1-41fa-88c4-cde17047015a:11693567]

            2015-12-2 18:40:28.913	RA.FT@0f8ec1f5-4b86-471b-94fa-bb866426f3d5	6712	RA on node 100:130935511391433044 processed message Activate:
            0f8ec1f5-4b86-471b-94fa-bb866426f3d5 130935550940751528:130000008b/130935550940751528:130000008c 0
            P/P RD U 90:130935542867603363 130935542969209766:130935542969209767 -1 -1 1.0:1.0:0
            S/S RD U 10:130935550599465377 130935495840300056:130935495840300058 -1 -1 1.0:1.0:0
            S/I RD D 70:130935476462446278 130935532129893072:130935532129893074 -1 -1 1.0:1.0:0
            S/S RD U 100:130935511391433044 130935551047647995:130935551047647996 -1 -1 1.0:1.0:0
            130935550940751528:130000008b:31660
            [New] 0f8ec1f5-4b86-471b-94fa-bb866426f3d5 Available None 130935550940751528:1100000088/0:0/130935550940751528:130000008c 130935550940751528:130000008b:31660 130935551047647995:130935551047647996 5 1 0 | [N N N 0] 0 0 0  2015-12-02 18:40:16.619
            I/N/S RD U RAP 0 0 0 100:130935511391433044 130935551047647995:130935551047647996 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 10:130935550599465377 130935495840300056:130935495840300058 1.0:1.0:0 -1 -1 0:0:-1
            P/N/I RD D N 0 0 0 70:130935476462446278 130935532129893072:130935532129893073 1.0:1.0:0 -1 -1 0:0:-1
            P/N/P RD U N 0 0 0 90:130935542867603363 130935542969209766:130935542969209767 1.0:1.0:0 -1 -1 0:0:-1

        [Activity: ad18004e-53a1-41fa-88c4-cde17047015a:11693639]

        Problematic state transition:
            0f8ec1f5-4b86-471b-94fa-bb866426f3d5 130935550940751528:130000008b/130935550940751528:130000008c 0
            P/P RD U 90:130935542867603363 130935542969209766:130935542969209767 -1 -1 1.0:1.0:0
            S/S RD U 10:130935550599465377 130935495840300056:130935495840300058 -1 -1 1.0:1.0:0
            S/I RD D 70:130935476462446278 130935532129893072:130935532129893074 -1 -1 1.0:1.0:0
            S/S RD U 100:130935511391433044 130935551047647995:130935551047647996 -1 -1 1.0:1.0:0
            130935550940751528:130000008b:31660
            [New] 0f8ec1f5-4b86-471b-94fa-bb866426f3d5 Available None 130935550940751528:1100000088/0:0/130935550940751528:130000008c 130935550940751528:130000008b:31660 130935551047647995:130935551047647996 5 1 0 | [N N N 0] 0 0 0  2015-12-02 18:40:16.619
            I/N/S RD U RAP 0 0 0 100:130935511391433044 130935551047647995:130935551047647996 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 10:130935550599465377 130935495840300056:130935495840300058 1.0:1.0:0 -1 -1 0:0:-1
            P/N/I RD D N 0 0 0 70:130935476462446278 130935532129893072:130935532129893073 1.0:1.0:0 -1 -1 0:0:-1
            P/N/P RD U N 0 0 0 90:130935542867603363 130935542969209766:130935542969209767 1.0:1.0:0 -1 -1 0:0:-1

            [Old] 0f8ec1f5-4b86-471b-94fa-bb866426f3d5 Available None 130935550940751528:1100000088/0:0/130935550940751528:130000008c 130935550940751528:1100000087:31626 130935551047647995:130935551047647996 5 1 0 | [N N N 0] 0 0 0  2015-12-02 18:40:16.619
            S/N/I IB U N 0 0 0 100:130935511391433044 130935551047647995:130935551047647996 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 10:130935550599465377 130935495840300056:130935495840300058 1.0:1.0:0 -1 -1 0:0:-1
            P/N/P RD D N 0 0 0 70:130935476462446278 130935532129893072:130935532129893073 1.0:1.0:0 -1 -1 0:0:-1

        90 - 1, 100 - 2, 10 - 3, 70 - 4
        */
        
        Test.AddFT(L"SP1", L"O None 411/000/433 2:1 CM [S/N/I IB U N F 2:1] [S/N/S RD U N F 3:1] [P/N/P RD D N F 4:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(1, L"SP1", L"432/433 {411:0} [P/P RD U 1:1] [S/S RD U 3:1] [S/I RD D 4:1] [S/S RD U 2:1]");

        Test.ValidateFT(L"SP1", L"O None 432/000/433 2:1 CM (sender:1) [S/N/S RD U RAP F 2:1] [P/N/P RD U N F 1:1] [S/N/S RD U N F 3:1] [S/N/I RD D N F 4:1]");
    }

    BOOST_AUTO_TEST_CASE(RefreshConfigurationIdleReplicaActivate)
    {
        /*
            1. Initial state = 0/e0 [N/P] [N/S] [N/I]
            2. FM starts e0/e1 [P/P] [S/S] [I/S] N
            3. Prior to activate P goes down
            4. e0/e2 [P/S D] [S/P] [I/S]
            5. Idle replica now receives Activate with P down. Idle has e2 as CC as it was updated as part of get lsn
        
        */
        Test.AddFT(L"SP1", L"O None 000/000/533 3:1 CM [N/N/I IB U N F 3:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(2, L"SP1", L"412/533 {533:0} [P/S RD D 1:1] [S/P RD U 2:1] [I/S RD U 3:1]");

        Test.DumpFT(L"SP1");
        
        Test.ValidateFT(L"SP1", L"O None 412/000/533 3:1 CM (sender:2) [I/N/S RD U RAP F 3:1] [P/N/S RD D N F 1:1] [S/N/P RD U N F 2:1]");
    }

    BOOST_AUTO_TEST_CASE(DoReconfigurationShouldNotAddStaleReplicas)
    {
        /*
        RA on node 3:130889044646747151 processed message DoReconfiguration:
        25d76252-8d2c-47fa-a881-55ca8ec4f711 130889068815603003:9700000204/130889068815603003:9a00000207 2 
        S/S DD D 4:130889061546470616 130889068909595934:130889068909595934 -1 -1 1.0:1.0:0
        S/S RD U 5:130888816934241309 130889068843407857:130889068843407857 -1 -1 1.0:1.0:0
        P/N DD D 2:130889057268717867 130889068843407856:130889068843407856 -1 -1 1.0:1.0:0
        S/S DD D 1:130889057276011610 130889068909595932:130889068909595932 -1 -1 1.0:1.0:0
        S/P RD U 3:130889044646747151 130889068909595933:130889068909595933 -1 -1 1.0:1.0:0
 
        [New] 25d76252-8d2c-47fa-a881-55ca8ec4f711 Available Phase1_GetLSN 130889068815603003:9800000205/0:0/130889068815603003:9a00000207 130889068815603003:9800000205:49 130889068909595933:130889068909595933 5 1 2 | [N N N 0] 0 0 0  2015-10-09 23:28:43.096
        S/N/P RD U RAP 0 0 0 3:130889044646747151 130889068909595933:130889068909595933 1.0:1.0:0 -1 -1 0:0:-1
        S/N/S DD D N 0 0 0 4:130889061546470616 130889068909595934:130889068909595934 1.0:1.0:0 -1 -1 0:0:-1
        S/N/S RD U RA 0 0 0 5:130888816934241309 130889068843407857:130889068843407857 1.0:1.0:0 -1 -1 0:0:-1
        P/N/N DD D N 0 0 0 2:130889057268717867 130889068843407856:130889068843407856 1.0:1.0:0 -1 -1 0:0:-1
        S/N/S DD D N 0 0 0 1:130889057276011610 130889068909595932:130889068909595932 1.0:1.0:0 -1 -1 0:0:-1
 
        [Old] 25d76252-8d2c-47fa-a881-55ca8ec4f711 Available None 130889068815603003:9800000205/0:0/130889068815603003:9900000206 130889068815603003:9800000205:49 130889068909595933:130889068909595933 5 1 2 | [N N N 0] 0 0 0  2015-10-09 23:27:23.609
        S/N/S RD U N 0 0 0 3:130889044646747151 130889068909595933:130889068909595933 1.0:1.0:0 -1 -1 0:0:-1
        S/N/S RD U N 0 0 0 5:130888816934241309 130889068843407857:130889068843407857 1.0:1.0:0 -1 -1 0:0:-1
        P/N/P RD U N 0 0 0 1:130889057276011610 130889068909595932:130889068909595932 1.0:1.0:0 -1 -1 0:0:-1
        S/N/S RD U N 0 0 0 4:130889061546470616 130889068909595934:130889068909595934 1.0:1.0:0 -1 -1 0:0:-1
 
        [Activity: cf7ff3a9-01ec-43aa-8b5a-05aa6bb5bfe8:19910483]
        */
        Test.AddFT(L"SP1", L"O None 533/000/644 3:1 CM [S/N/S RD U N F 3:1] [S/N/S RD U N F 5:1] [P/N/P RD U N F 1:1] [S/N/S RD U N F 4:1]");

        Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"422/755 [S/S DD D 4:1] [S/S RD U 5:1] [P/N DD D 2:1] [S/S DD D 1:1] [S/P RD U 3:1]");

        Test.ValidateFT(L"SP1", L"O Phase1_GetLSN 422/000/755 {644:0} 3:1 CNM [S/N/P RD U RAP F 3:1] [S/N/S DD D N F 4:1] [S/N/S RD U RA F 5:1] [P/N/N DD D N F 2:1] [S/N/S DD D N F 1:1]");
    }

    BOOST_AUTO_TEST_CASE(SwapPrimaryActivateShouldNotSendLSN)
    {
        /*
            RA on node 20:130897889617060552 ReconfigurationMessageRetry
	            [New] 42f3fb4a-8be4-4319-855a-d87ada826fd0 Available Phase4_Activate 130897892105886829:1400000170/130897892105886829:1500000171/130897892105886829:1500000171 130897892105886829:1500000171:0 130897892148855860:130897892148855862 5 1 0 | [N N N 0] 0 0 0  2015-10-20 20:46:14.017
	            P/S/S RD U N 0 0 0 20:130897889617060552 130897892148855860:130897892148855862 1.0:1.0:0 -1 0 0:0:-1
	            S/P/P RD U N 0 0 0 220:130897889614404404 130898410431753418:130898410431753418 1.0:1.0:0 -1 0 0:0:-1
	
	            [Activity: MessageRetry:5809]        

            If FM rebuild happens with this state then RA sends LSN to the FM as -1, 0

            FM puts this in its state and then after a rebuild, a subsequent DoReconfiguration contains only last LSN and 
            RA asserts as it has received an inconsistent LSN
        */

        Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1 -1 0] [P/S/S RD U N F 2:1 -1 0]");

        auto & ft = Test.GetFT(L"SP1");

        FailoverUnitInfo ftInfo;
        bool rv = ft.TryGetConfiguration(ft.LocalReplica.FederationNodeInstance, false, ftInfo);
        Verify::IsTrue(rv, L"TryGetConfiguration return value");

        TestLog::WriteInfo(wformatString("FTInfo {0}", ftInfo));

        for (auto it : ftInfo.Replicas)
        {
            Verify::AreEqual(FABRIC_INVALID_SEQUENCE_NUMBER, it.Description.FirstAcknowledgedLSN, L"FirstAckLSN");
            Verify::AreEqual(FABRIC_INVALID_SEQUENCE_NUMBER, it.Description.LastAcknowledgedLSN, L"LastAckLSN");
        }
    }

    BOOST_AUTO_TEST_CASE(DeactivateShouldClearLocalPCIfItIsStale)
    {
        /*
            RA on node 1:130904316282402562 processed message Deactivate:
            a895b853-d3b4-4c73-a39c-5788ae6fdac1 130903797360428758:2c0000010c/130903797360428758:2d0000010e 0
                S/P RD U 5:130903788138314935 130904246866527872:130904246866527873 68444 68447 1.0:1.0:0
                S/S RD U 1:130904316282402562 130904274957128694:130904274957128695 -1 -1 1.0:1.0:0
                S/S DD D 2:130904280109805487 130904289488506218:130904289488506218 -1 -1 1.0:1.0:0
                P/S RD U 3:130904315694882629 130904293996667420:130904293996667422 0 68444 1.0:1.0:0
                0 130903797360428758:2c0000010b:68439
            [New] a895b853-d3b4-4c73-a39c-5788ae6fdac1 Available None 130903797360428758:2900000107/130903797360428758:2d0000010e/130903797360428758:2d0000010e 130903797360428758:2c0000010b:68439 130904274957128694:130904274957128695 5 4 0 | [N N N 0] 0 0 0  2015-10-27 15:00:31.775
                P/S/I RD U N 0 0 0 1:130904316282402562 130904274957128694:130904274957128695 1.0:1.0:0 -1 -1 0:0:-1
                S/P/P RD U N 0 0 0 5:130903788138314935 130904246866527872:130904246866527873 1.0:1.0:0 -1 -1 0:0:-1
                S/S/S RD D N 0 0 0 2:130904280109805487 130904289488506218:130904289488506218 1.0:1.0:0 -1 -1 0:0:-1
                P/N/N RD U N 0 0 0 4:130904290157333746 130904248853299470:130904248853299472 1.0:1.0:0 -1 -1 0:0:-1
                S/S/S RD U N 0 0 0 3:130904315694882629 130904293996667420:130904293996667422 1.0:1.0:0 -1 -1 0:0:-1
            [Old] a895b853-d3b4-4c73-a39c-5788ae6fdac1 Available None 130903797360428758:2900000107/0:0/130903797360428758:2d0000010e 130903797360428758:2900000101:66398 130904274957128694:130904274957128695 5 4 0 | [N N N 0] 0 0 0  2015-10-27 15:00:31.775
                P/N/I IB U N 0 0 0 1:130904316282402562 130904274957128694:130904274957128695 1.0:1.0:0 -1 -1 0:0:-1
                S/N/S RD U N 0 0 0 5:130903788138314935 130904246866527872:130904246866527872 1.0:1.0:0 -1 -1 0:0:-1
                S/N/S RD U N 0 0 0 2:130904280109805487 130904289488506218:130904289488506218 1.0:1.0:0 -1 -1 0:0:-1
                S/N/P RD U N 0 0 0 4:130904290157333746 130904248853299470:130904248853299472 1.0:1.0:0 -1 -1 0:0:-1
                S/N/S RD U N 0 0 0 3:130904286681675385 130904293996667420:130904293996667420 1.0:1.0:0 -1 -1 0:0:-1
        */        

        Test.AddFT(L"SP1", L"O None 411/000/633 1:1 CM [P/N/I IB U N F 1:1] [S/N/S RD U N F 5:1] [S/N/S RD U N F 2:1] [S/N/P RD U N F 4:1] [S/N/S RD U N F 3:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(5, L"SP1", L"522/633 {411:0} false [S/P RD U 5:1] [S/S RD U 1:1] [S/S DD D 2:1] [P/S RD U 3:1]");

        Test.ValidateFT(L"SP1", L"O None 522/633/633 1:1 CM [S/S/I RD U N F 1:1] [S/P/P RD U N F 5:1] [S/S/S DD D N F 2:1] [P/S/S RD U N F 3:1]");
    }

    BOOST_AUTO_TEST_CASE(DeactivateShouldUpdateInstanceOfDownReplica)
    {
        /*
            RA on node 1a0:130937992115935007 processed message Deactivate:
            47187803-7fe7-402c-a79c-5c76bd1ff9f6 130937638539244860:320000002b/130937638539244860:3f0000002f 0
            S/P IB U 70:130937981777978107 130937895906930492:130937895906930499 0 0 1.0:1.0:0
            P/S SB D 190:130937958073787001 130937898297928348:130937898297928352 -1 -1 1.0:1.0:0
            S/S RD U 1a0:130937992115935007 130937880032872610:130937880032872616 0 0 1.0:1.0:0
            S/S SB D 40:130937983792701024 130937940830517651:130937940830517653 -1 -1 1.0:1.0:0
            0 130937638539244860:320000002a:0
            [New] 47187803-7fe7-402c-a79c-5c76bd1ff9f6 Available None 130937638539244860:320000002b/130937638539244860:3f0000002f/130937638539244860:3f0000002f 130937638539244860:320000002a:0 130937880032872610:130937880032872616 4 4 0 | [N N N 0] 0 0 0  2015-12-05 14:26:52.156
            S/S/I RD U N 0 0 0 1a0:130937992115935007 130937880032872610:130937880032872616 1.0:1.0:0 -1 -1 0:0:-1
            S/P/P SB U N 0 0 0 70:130937981777978107 130937895906930492:130937895906930499 1.0:1.0:0 -1 -1 0:0:-1
            P/S/S RD D N 0 0 0 190:130937958073787001 130937898297928348:130937898297928351 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S RD D N 0 0 0 40:130937983792701024 130937940830517651:130937940830517651 1.0:1.0:0 -1 -1 0:0:-1
            [Old] 47187803-7fe7-402c-a79c-5c76bd1ff9f6 Available None 130937638539244860:320000002b/130937638539244860:3b0000002e/130937638539244860:3f0000002f 130937638539244860:320000002a:0 130937880032872610:130937880032872616 4 4 0 | [N N N 0] 0 0 0  2015-12-05 14:26:52.156
            S/S/I IB U N 0 0 0 1a0:130937992115935007 130937880032872610:130937880032872616 1.0:1.0:0 -1 -1 0:0:-1
            S/P/P RD U N 0 0 0 70:130937955432850757 130937895906930492:130937895906930497 1.0:1.0:0 -1 -1 0:0:-1
            P/S/S RD D N 0 0 0 190:130937958073787001 130937898297928348:130937898297928351 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S RD D N 0 0 0 40:130937954773217800 130937940830517651:130937940830517651 1.0:1.0:0 -1 -1 0:0:-1
        */

        // 70 - 1, 1a0 - 2, 190 - 3, 40 - 4
        Test.AddFT(L"SP1", L"O None 411/422/433 2:1 CM [S/S/I IB U N F 2:1] [S/P/P RD U N F 1:1] [P/S/S RD D N F 3:1] [S/S/S RD D N F 4:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(1, L"SP1", L"411/433 {411:0} false [S/P IB U 1:1] [P/S SB D 3:1] [S/S RD U 2:1] [S/S SB D 4:1:4:2]");

        Test.ValidateFT(L"SP1", L"O None 411/433/433 2:1 CM [S/S/I RD U N F 2:1] [S/P/P IB U N F 1:1] [P/S/S SB D N F 3:1] [S/S/S SB D N F 4:1:4:2]");
    }

    BOOST_AUTO_TEST_CASE(PCRoleTest)
    {
        /*
            a0fd6741-e13c-418a-b387-a3f9efde8457 130909442586713288:45000001e9/130909442586713288:47000001eb 7
            S/P RD U 40:130908867315754815 130909447612948750:130909447612948750 51496 51559 1.0:1.0:0
            S/S RD U 230:130909453901749295 130909350204950575:130909350204950579 -1 -1 1.0:1.0:0
            P/I RD D 10:130909440983786986 130909402065748600:130909402065748603 -1 -1 1.0:1.0:0
            S/S RD U 140:130909425733006434 130909452858680927:130909452858680928 0 51559 1.0:1.0:0

            [Old] a0fd6741-e13c-418a-b387-a3f9efde8457 Available None 130909442586713288:45000001e9/0:0/130909442586713288:47000001eb 130909442586713288:45000001e0:50662 130909350204950575:130909350204950579 4 3 7 | [N N N 0] 0 0 0 2015-11-02 13:46:41.902
            S/N/I IB U N 0 0 0 230:130909453901749295 130909350204950575:130909350204950579 1.0:1.0:0 -1 -1 0:0:-1
            P/N/I RD D N 0 0 0 10:130909440983786986 130909402065748600:130909402065748603 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 40:130908867315754815 130909447612948750:130909447612948750 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 140:130909425733006434 130909452858680927:130909452858680927 1.0:1.0:0 -1 -1 0:0:-1

            [New] a0fd6741-e13c-418a-b387-a3f9efde8457 Available None 130909442586713288:45000001e9/0:0/130909442586713288:47000001eb 130909442586713288:47000001eb:51559 130909350204950575:130909350204950579 4 3 7 | [N N N 0] 0 0 0 2015-11-02 13:46:41.902
            I/N/S RD U RAP 0 0 0 230:130909453901749295 130909350204950575:130909350204950579 1.0:1.0:0 -1 -1 0:0:-1
            I/N/I RD D N 0 0 0 10:130909440983786986 130909402065748600:130909402065748603 1.0:1.0:0 -1 -1 0:0:-1
            S/N/P RD U N 0 0 0 40:130908867315754815 130909447612948750:130909447612948750 1.0:1.0:0 -1 51559 0:0:-1
            S/N/S RD U N 0 0 0 140:130909425733006434 130909452858680927:130909452858680928 1.0:1.0:0 -1 51559 0:0:-1
        */

        Test.AddFT(L"SP1", L"O None 411/000/422 230:1 CM [S/N/I IB U N F 230:1] [P/N/I RD D N F 10:1] [S/N/S RD U N F 40:1] [S/N/S RD U N F 140:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(40, L"SP1", L"411/422 {422:0} [S/P RD U 40:1] [S/S RD U 230:1] [P/I RD D 10:1] [S/S RD U 140:1]");

        Test.ValidateFT(L"SP1", L"O None 411/000/422 230:1 CM (sender:40) [S/N/S RD U RAP F 230:1] [S/N/P RD U N F 40:1] [P/N/I RD D N F 10:1] [S/N/S RD U N F 140:1]");
    }

    BOOST_AUTO_TEST_CASE(StaleDeactivateAfterUCFailure)
    {
        /*
            Initial State:
        	6385e7f6-aa07-4eee-ba8b-41b0ad741d14 Available None 130981995815155862:160000003d/0:0/130981995815155862:160000003e 130981995815155862:160000003a:933 130981995880004170:130981995880004171 5 5 1 | [N N N 0] 0 0 0 1.0:1.0:0 2016-01-25 12:46:34.854
	        S/N/I IB U N 0 0 0 0 10:130981992088299473 130981995880004170:130981995880004171 1.0:1.0:0 -1 -1 0:0:-1
	        P/N/P RD U N 0 0 0 0 3:130981995545969571 130981939471672026:130981939471672030 1.0:1.0:0 -1 -1 0:0:-1
	        S/N/S DD D N 0 0 0 0 40:130981990777489099 130981939548030712:130981939548030716 1.0:1.0:0 -1 -1 0:0:-1
	        S/N/S RD U N 0 0 0 0 20:130981984450963712 130981938027725187:130981938027725189 1.0:1.0:0 -1 -1 0:0:-1
	        S/N/S RD U N 0 0 0 0 90:130981475133974545 130981995880004169:130981995880004169 1.0:1.0:0 -1 -1 0:0:-1
        
            Activate:
            6385e7f6-aa07-4eee-ba8b-41b0ad741d14 0:0/130981995815155862:160000003e
            N/P RD U 3:130981995545969571 130981939471672026:130981939471672030 -1 -1 1.0:1.0:0
            N/S DD D 40:130981990777489099 130981939548030712:130981939548030716 -1 -1 1.0:1.0:0
            N/S RD U 20:130981984450963712 130981938027725187:130981938027725189 -1 -1 1.0:1.0:0
            N/S RD U 90:130981475133974545 130981995880004169:130981995880004169 -1 -1 1.0:1.0:0
            N/S RD U 10:130981992088299473 130981995880004170:130981995880004171 -1 -1 1.0:1.0:0
            130981995815155862:160000003a:933 FT:
            6385e7f6-aa07-4eee-ba8b-41b0ad741d14 Available None 130981995815155862:160000003d/0:0/130981995815155862:160000003e 130981995815155862:160000003a:933 130981995880004170:130981995880004171 5 5 1 | [N N N 0] 0 0 0 1.0:1.0:0 2016-01-25 12:46:34.854
            I/N/S RD U RAP 0 0 0 0 10:130981992088299473 130981995880004170:130981995880004171 1.0:1.0:0 -1 -1 0:0:-1
            P/N/P RD U N 0 0 0 0 3:130981995545969571 130981939471672026:130981939471672030 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S DD D N 0 0 0 0 40:130981990777489099 130981939548030712:130981939548030716 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 0 20:130981984450963712 130981938027725187:130981938027725189 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 0 90:130981475133974545 130981995880004169:130981995880004169 1.0:1.0:0 -1 -1 0:0:-1

            UC Reply:
            I/S RD U 10:130981992088299473 130981995880004170:130981995880004171 -1 -1 1.0:1.0:0
            I/S RD U 10:130981992088299473 130981995880004170:130981995880004171 -1 -1 1.0:1.0:0
            P/P RD U 3:130981995545969571 130981939471672026:130981939471672030 -1 -1 1.0:1.0:0
            S/S DD D 40:130981990777489099 130981939548030712:130981939548030716 -1 -1 1.0:1.0:0
            S/S RD U 20:130981984450963712 130981938027725187:130981938027725189 -1 -1 1.0:1.0:0
            S/S RD U 90:130981475133974545 130981995880004169:130981995880004169 -1 -1 1.0:1.0:0
            State: - EC: E_FAIL:''
            6385e7f6-aa07-4eee-ba8b-41b0ad741d14 Available None 130981995815155862:160000003d/0:0/130981995815155862:160000003e 130981995815155862:160000003a:933 130981995880004170:130981995880004171 5 5 1 | [N N N 0] 0 0 0 1.0:1.0:0 2016-01-25 12:46:34.854
            I/N/S RD U RAP 0 0 0 0 10:130981992088299473 130981995880004170:130981995880004171 1.0:1.0:0 -1 -1 0:0:-1
            P/N/P RD U N 0 0 0 0 3:130981995545969571 130981939471672026:130981939471672030 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S DD D N 0 0 0 0 40:130981990777489099 130981939548030712:130981939548030716 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 0 20:130981984450963712 130981938027725187:130981938027725189 1.0:1.0:0 -1 -1 0:0:-1
            S/N/S RD U N 0 0 0 0 90:130981475133974545 130981995880004169:130981995880004169 1.0:1.0:0 -1 -1 0:0:-1

            Deactivate:
            6385e7f6-aa07-4eee-ba8b-41b0ad741d14 130981995815155862:160000003e/130981995815155862:160000003f 1
            P/P RD U 3:130981995545969571 130981939471672026:130981939471672030 -1 -1 1.0:1.0:0
            S/N DD D 40:130981990777489099 130981939548030712:130981939548030716 -1 -1 1.0:1.0:0
            S/S RD U 20:130981984450963712 130981938027725187:130981938027725189 -1 -1 1.0:1.0:0
            I/S RD U 60:130981991238868367 130981995880004168:130981995880004168 -1 -1 1.0:1.0:0
            S/S RD U 90:130981475133974545 130981995880004169:130981995880004169 -1 -1 1.0:1.0:0
            S/S RD U 10:130981992088299473 130981995880004170:130981995880004171 -1 -1 1.0:1.0:0
            0 130981995815155862:160000003a:933 FT:
            6385e7f6-aa07-4eee-ba8b-41b0ad741d14 Available None 130981995815155862:160000003e/130981995815155862:160000003f/130981995815155862:160000003f 130981995815155862:160000003a:933 130981995880004170:130981995880004171 5 5 1 | [N N N 0] 0 0 0 1.0:1.0:0 2016-01-25 12:46:34.854
            S/S/S RD U N 0 0 0 0 10:130981992088299473 130981995880004170:130981995880004171 1.0:1.0:0 -1 -1 0:0:-1
            P/P/P RD U N 0 0 0 0 3:130981995545969571 130981939471672026:130981939471672030 1.0:1.0:0 -1 -1 0:0:-1
            S/N/N DD D N 0 0 0 0 40:130981990777489099 130981939548030712:130981939548030716 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S RD U N 0 0 0 0 20:130981984450963712 130981938027725187:130981938027725189 1.0:1.0:0 -1 -1 0:0:-1
            I/S/S RD U N 0 0 0 0 60:130981991238868367 130981995880004168:130981995880004168 1.0:1.0:0 -1 -1 0:0:-1
            S/S/S RD U N 0 0 0 0 90:130981475133974545 130981995880004169:130981995880004169 1.0:1.0:0 -1 -1 0:0:-1

            Activate:
            6385e7f6-aa07-4eee-ba8b-41b0ad741d14 130981995815155862:160000003e/130981995815155862:160000003f 1
            P/P RD U 3:130981995545969571 130981939471672026:130981939471672030 -1 -1 1.0:1.0:0
            S/N DD D 40:130981990777489099 130981939548030712:130981939548030716 -1 -1 1.0:1.0:0
            S/S RD U 20:130981984450963712 130981938027725187:130981938027725189 -1 -1 1.0:1.0:0
            I/S RD U 60:130981991238868367 130981995880004168:130981995880004168 -1 -1 1.0:1.0:0
            S/S RD U 90:130981475133974545 130981995880004169:130981995880004169 -1 -1 1.0:1.0:0
            S/S RD U 10:130981992088299473 130981995880004170:130981995880004171 -1 -1 1.0:1.0:0
            130981995815155862:160000003a:933 FT:
        */

        Test.AddFT(L"SP1", L"O None 411/000/412 10:1 CM [S/N/I IB U N F 10:1] [P/N/P RD U N F 3:1] [S/N/S DD D N F 40:1] [S/N/S RD U N F 20:1] [S/N/S RD U N F 90:1]");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(3, L"SP1", L"000/412 {411:0} [N/P RD U 3:1] [N/S DD D 40:1] [N/S RD U 20:1] [N/S RD U 90:1] [N/S RD U 10:1]");
        Test.DumpFT(L"SP1");

        Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"412/412 [S/S RD U 10:1] [S/S RD U 10:1] [P/P RD U 3:1] [S/S DD D 40:1] [S/S RD U 20:1] [S/S RD U 90:1] GlobalLeaseLost -");
        Test.DumpFT(L"SP1");

        Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(3, L"SP1", L"412/413 {411:0} false [P/P RD U 3:1] [S/N DD D 40:1] [S/S RD U 20:1] [I/S RD U 60:1] [S/S RD U 90:1] [S/S RD U 10:1]");
        Test.DumpFT(L"SP1");

        Test.ResetAll();
        Test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(3, L"SP1", L"412/413 {411:0} [P/P RD U 3:1] [S/N DD D 40:1] [S/S RD U 20:1] [I/S RD U 60:1] [S/S RD U 90:1] [S/S RD U 10:1]");
        Test.DumpFT(L"SP1");

        Test.ValidateRAPMessage<MessageType::UpdateConfiguration>(L"SP1");
        Test.ValidateNoRAMessage();
    }

    BOOST_AUTO_TEST_CASE(ReplicaDownShouldNotBeSentIfReplicaIsBeingDeleted)
    {
        Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

        Test.ProcessFMMessageAndDrain<MessageType::DeleteReplica>(L"SP1", L"000/411 [N/N RD U 1:1]");

        Test.ResetAll();
        Test.ProcessAppHostClosedAndDrain(L"SP1");
        Test.DumpFT(L"SP1");

        Test.ValidateNoFMMessages();
    }
    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
}
