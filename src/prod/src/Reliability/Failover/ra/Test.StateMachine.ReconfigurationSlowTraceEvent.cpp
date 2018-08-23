// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"
#include "Reliability/Failover/ra/Diagnostics.FailoverUnitEventData.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Diagnostics;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;


class TestStateMachine_ReconfigurationSlowTraceEvent : public StateMachineTestBase
{
protected:
    TestStateMachine_ReconfigurationSlowTraceEvent()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestStateMachine_ReconfigurationSlowTraceEvent()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Enum reason, std::wstring reportContents);
    void VerifyEventData(FailoverUnitReconfigurationStage::Enum reason, std::wstring reportContents);
    void ValidateReconfigurationComplete();
    void ValidateEmptyEvents();

};

bool TestStateMachine_ReconfigurationSlowTraceEvent::TestSetup()
{
    Test.UTContext.Config.ReconfigurationHealthReportThresholdEntry.Test_SetValue(TimeSpan::FromSeconds(5));
    Test.UTContext.Config.RemoteReplicaProgressQueryWaitDurationEntry.Test_SetValue(TimeSpan::FromSeconds(5));
    Test.UTContext.Config.ServiceReconfigurationApiTraceWarningThresholdEntry.Test_SetValue(TimeSpan::FromSeconds(5));
    return true;
}

bool TestStateMachine_ReconfigurationSlowTraceEvent::TestCleanup()
{
    return true;
}

void TestStateMachine_ReconfigurationSlowTraceEvent::ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Enum stage, std::wstring reportContents)
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

    VerifyEventData(stage, reportContents);

    Test.StateItemHelpers.EventWriterHelper.Reset();
}

void TestStateMachine_ReconfigurationSlowTraceEvent::VerifyEventData(FailoverUnitReconfigurationStage::Enum stage, std::wstring reportContents)
{    
    auto eventDataPtr = Test.StateItemHelpers.EventWriterHelper.GetEvent<ReconfigurationSlowEventData>();
    auto eventData = *eventDataPtr;
    Verify::AreEqual(wformatString(stage), eventData.ReconfigurationPhase);
    Verify::StringContains(eventData.TraceDescription, reportContents, L"Reconfiguration Slow Trace contents missing expected string.");
    Verify::StringContains(eventData.TraceDescription, L"Reconfiguration phase start time: ", L"Missing Phase time elapsed");
    Verify::StringContains(eventData.TraceDescription, L"Reconfiguration start time: ", L"Missing reconfiguration start time");
}

void TestStateMachine_ReconfigurationSlowTraceEvent::ValidateReconfigurationComplete()
{
    auto eventDataPtr = Test.StateItemHelpers.EventWriterHelper.GetEvent<ReconfigurationCompleteEventData>();
    auto eventData = *eventDataPtr;
    Verify::AreEqual(wformatString(eventData.TraceEventType), wformatString(TraceEventType::ReconfigurationComplete));
}

void TestStateMachine_ReconfigurationSlowTraceEvent::ValidateEmptyEvents()
{
    Verify::IsTrue(Test.StateItemHelpers.EventWriterHelper.IsTracedEventsEmpty());
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReconfigurationSlowTraceEventSuite, TestStateMachine_ReconfigurationSlowTraceEvent)

BOOST_AUTO_TEST_CASE(Phase4_ReplicaMessageStateRAReplyPendingHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase4_Activate, L"Waiting for response from 1 replicas");
}

BOOST_AUTO_TEST_CASE(Phase4_ReplicaMessageStateRAReplyPendingInBuildHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S IB U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase4_Activate, L"Waiting for response from 1 replicas");
}

BOOST_AUTO_TEST_CASE(Phase4_LocalReplicaNotRepliedHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U RAP F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase4_Activate, L"Waiting for response from the local replica");
}

BOOST_AUTO_TEST_CASE(Phase4_ClearReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase4_Activate, L"Waiting for response from 1 replicas");

    Test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(3, L"SP1", L"411/422 [P/S RD U 3:1] Success");

    ValidateReconfigurationComplete();
}


BOOST_AUTO_TEST_CASE(Phase3_PcBelowReadQuorumHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U RA F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase3_Deactivate, L"Waiting for response from 2 replicas");
}


BOOST_AUTO_TEST_CASE(Phase3_ClearReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase3_Deactivate, L"Waiting for response from 3 replicas");

    Test.ProcessRemoteRAMessageAndDrain<MessageType::DeactivateReply>(3, L"SP1", L"411/422 [P/S RD U 3:1] Success");

    ValidateEmptyEvents();
}

BOOST_AUTO_TEST_CASE(Phase2_ClearReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase2_Catchup 411/422/422 1:1 CM [S/N/P RD U N F 1:1] [S/N/S RD U N F 2:1] [P/N/S RD U N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase2_Catchup, L"Waiting for response from the local replica");

    Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [S/P RD U 1:1] [S/S RD U 2:1] [P/S RD U 3:1] Success C");

    ValidateEmptyEvents();
}

BOOST_AUTO_TEST_CASE(Phase1_DataLossReconfigurationStuckWarningHealthReportTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S DD D N F 2:1] [S/N/S DD D N F 3:1] [S/N/S DD D N F 4:1] [P/N/S DD D N F 5:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase1_GetLSN, L"Waiting for response from Failover Manager");
}

BOOST_AUTO_TEST_CASE(Phase1_ClearReportGetLSNCompleteTest)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [S/N/S RD U N F 2:1] [P/N/S RD U RA F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase1_GetLSN, L"Waiting for response from 2 replicas");

    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, L"SP1", L"411/422 [P/S RD U 3:1 1 1] Success");

    ValidateEmptyEvents();
}

BOOST_AUTO_TEST_CASE(Phase0_ClearDemoteStuckReportTest)
{
    Test.AddFT(L"SP1", L"O Phase0_Demote 411/000/422 {411:0} 1:1 CMBN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1] [S/N/S DD D N F 3:1]");

    ExecuteReconfigurationCheckAndVerifyTraceEvents(FailoverUnitReconfigurationStage::Phase0_Demote, L"Waiting for response from the local replica");

    Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/422 [P/S RD U 1:1] [P/S RD U 1:1] [S/P RD U 2:1] [S/S DD D 3:1] Success C");

    ValidateEmptyEvents();
}



BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
