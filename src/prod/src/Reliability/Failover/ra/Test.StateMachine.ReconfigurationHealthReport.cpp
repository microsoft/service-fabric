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

class TestStateMachine_ReconfigurationHealthReport : public StateMachineTestBase
{
protected:
    TestStateMachine_ReconfigurationHealthReport() : StateMachineTestBase()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestStateMachine_ReconfigurationHealthReport()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Enum reason, int replicaCount=-1);
    void VerifyHealthReportDescriptor(ReconfigurationProgressStages::Enum reason, int replicaCount);
    void ValidateClearHealthEvent();

};

bool TestStateMachine_ReconfigurationHealthReport::TestSetup()
{
    Test.UTContext.Config.ReconfigurationHealthReportThresholdEntry.Test_SetValue(TimeSpan::FromSeconds(5));
    Test.UTContext.Config.RemoteReplicaProgressQueryWaitDurationEntry.Test_SetValue(TimeSpan::FromSeconds(5));
    Test.UTContext.Config.ServiceReconfigurationApiTraceWarningThresholdEntry.Test_SetValue(TimeSpan::FromSeconds(5));
    return true;
}

bool TestStateMachine_ReconfigurationHealthReport::TestCleanup()
{
    return true;
}

void TestStateMachine_ReconfigurationHealthReport::ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Enum reason, int replicaCount)
{
    Test.UTContext.Clock.SetManualMode();
    Test.UTContext.Clock.AdvanceTimeBySeconds(10);

    Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
        L"SP1",
        [&](EntityExecutionContext & base)
    { 
        auto & context = base.As<FailoverUnitEntityExecutionContext>();
        Test.GetFT(L"SP1").CheckReconfigurationProgressAndHealthReportStatus(context);
    });

    Test.ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::ReconfigurationStuckWarning, L"SP1", 1, 1);

    VerifyHealthReportDescriptor(reason, replicaCount);

    Test.StateItemHelpers.HealthHelper.Reset();
}

void TestStateMachine_ReconfigurationHealthReport::VerifyHealthReportDescriptor(ReconfigurationProgressStages::Enum reason, int replicaCount)
{
    auto reportInfo = Test.StateItemHelpers.HealthHelper.GetLastReplicaHealthEvent();
    try
    {
        ReconfigurationStuckHealthReportDescriptor const & desc = dynamic_cast<ReconfigurationStuckHealthReportDescriptor const &>(reportInfo.Descriptor);
        Verify::AreEqual(reason, desc.ReconfigurationStuckReason);
        if (replicaCount > -1)
        {
            Verify::AreEqual(replicaCount, desc.Test_GetReplicaCount());
        }
    }
    catch (const std::bad_cast &)
    {
        Verify::Fail(L"Health Report Descriptor of has an unexpected type.");
    }
}

void TestStateMachine_ReconfigurationHealthReport::ValidateClearHealthEvent()
{
    Test.ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::ClearReconfigurationStuckWarning, L"SP1", 1, 1);
}


BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReconfigurationHealthReportSuite, TestStateMachine_ReconfigurationHealthReport)

BOOST_AUTO_TEST_CASE(Phase4_ReplicatorRemovePendingHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD D N R 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated);
}

BOOST_AUTO_TEST_CASE(Phase4_ReplicaMessageStateRAReplyPendingHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase4_UpReadyReplicasPending);
}

BOOST_AUTO_TEST_CASE(Phase4_ReplicaMessageStateRAReplyPendingInBuildHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S IB U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase4_ReplicaStuckIB);
}

BOOST_AUTO_TEST_CASE(Phase4_LocalReplicaNotRepliedHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U RAP F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase4_LocalReplicaNotReplied);
}

BOOST_AUTO_TEST_CASE(Phase4_ClearReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase4_UpReadyReplicasPending);

    Test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(3, L"SP1", L"411/422 [P/S RD U 3:1] Success");

    ValidateClearHealthEvent();
}

BOOST_AUTO_TEST_CASE(Phase3_UpReadyReplicasWaitingHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase3_WaitingForReplicas);
}

BOOST_AUTO_TEST_CASE(Phase3_PcBelowReadQuorumHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U RA F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase3_PCBelowReadQuorum);
}


BOOST_AUTO_TEST_CASE(Phase3_ClearReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase3_WaitingForReplicas);

    Test.ProcessRemoteRAMessageAndDrain<MessageType::DeactivateReply>(3, L"SP1", L"411/422 [P/S RD U 3:1] Success");

    ValidateClearHealthEvent();
}

BOOST_AUTO_TEST_CASE(Phase2_CatchupStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase2_Catchup 411/422/422 1:1 CM [S/N/P RD U N F 1:1] [S/N/S RD U N F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase2_NoReplyFromRap);
}

BOOST_AUTO_TEST_CASE(Phase2_ClearReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase2_Catchup 411/422/422 1:1 CM [S/N/P RD U N F 1:1] [S/N/S RD U N F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase2_NoReplyFromRap);

    Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [S/P RD U 1:1] [S/S RD U 2:1] [P/S RD U 3:1] Success C");

    ValidateClearHealthEvent();
}

BOOST_AUTO_TEST_CASE(Phase1_DataLossReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S DD D N F 2:1] [S/N/S DD D N F 3:1] [S/N/S DD D N F 4:1] [P/N/S DD D N F 5:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_DataLoss);
}

BOOST_AUTO_TEST_CASE(Phase1_DataLossExchangeReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/511/511 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S RD U RA F 2:1 1 1] [S/N/S DD D N F 3:1] [S/N/S DD D N F 4:1] [P/N/S DD D N F 5:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_DataLoss);
}

BOOST_AUTO_TEST_CASE(Phase1_GetLSNPendingNotExpiredReconfigurationStuckTest)
{
    Test.UTContext.Config.RemoteReplicaProgressQueryWaitDurationEntry.Test_SetValue(TimeSpan::FromSeconds(50));

    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S RD D N F 2:1] [P/N/S RD D N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);
}

BOOST_AUTO_TEST_CASE(Phase1_GetLSNPendingReconfigurationStuckTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S RD U RA F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);
}

BOOST_AUTO_TEST_CASE(Phase1_GetLSNPendingIBReconfigurationStuckTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S IB U RA F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);
}

BOOST_AUTO_TEST_CASE(Phase1_GetLSNPendingSBReconfigurationStuckTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S SB D RA F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);
}

BOOST_AUTO_TEST_CASE(Phase1_GetLSNPendingDDReconfigurationStuckTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S DD D N F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);
}


BOOST_AUTO_TEST_CASE(Phase1_ClearReportGetLSNCompleteTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S RD U N F 2:1] [P/N/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);

    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, L"SP1", L"411/422 [P/S RD U 3:1 1 1] Success");

    ValidateClearHealthEvent();
}

BOOST_AUTO_TEST_CASE(Phase1_ClearReportGetLSNCompleteChangeConfigurationTest)
{
    auto & test = Test;
    test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S RD U RA F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);

    test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(2, L"SP1", L"411/422 [S/S RD U 2:1 1 2] Success");

    ValidateClearHealthEvent();
}

BOOST_AUTO_TEST_CASE(Phase1_ClearReportCloseReplicaTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S RD U N F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);

    Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
        L"SP1",
        [&](EntityExecutionContext & base)
    {
        auto & context = base.As<FailoverUnitEntityExecutionContext>();
        Test.GetFT(L"SP1").StartCloseLocalReplica(ReplicaCloseMode::Close, ReconfigurationAgent::InvalidNode, context, ActivityDescription::Empty);
    });

    ValidateClearHealthEvent();
}

BOOST_AUTO_TEST_CASE(Phase0_DemoteStuckReportTest)
{
    Test.AddFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CM [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase0_NoReplyFromRAP);
}

BOOST_AUTO_TEST_CASE(Phase0_ClearDemoteStuckReportTest)
{
    Test.AddFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CMBN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase0_NoReplyFromRAP);

    Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1] [P/S RD U 1:1] [S/P RD U 2:1] [S/S DD D 3:1] Success C");

    ValidateClearHealthEvent();
}

BOOST_AUTO_TEST_CASE(TestUpdateHealthReportGeneration_Phase1_WaitingForReadQuorum)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S RD U RA F 2:1] [P/N/S RD U RA F 3:1] [S/N/S RD U RA F 4:1] [S/N/S RD U RA F 5:1]");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, 4);

    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, L"SP1", L"411/422 [P/S RD U 3:1 1 1] Success");

    ExecuteReconfigurationCheckAndVerifyHealthReportIsWarning(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, 3);

    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(4, L"SP1", L"411/422 [S/S RD U 4:1 1 1] Success");

    ValidateClearHealthEvent();
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
