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

namespace
{
    wstring const ShortName = L"SP1";
}

class TestReconfigurationStatistics
{
protected:
    TestReconfigurationStatistics()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestReconfigurationStatistics()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    void AdvanceTime(int seconds)
    {
        GetScenarioTest().UTContext.Clock.AdvanceTimeBySeconds(seconds);
    }

    void Verify(
        ReconfigurationType::Enum type,
        ReconfigurationResult::Enum result,
        int phase0, 
        int phase1,
        int phase2,
        int phase3,
        int phase4)
    {
        auto & ft = GetScenarioTest().GetFT(ShortName);        
        auto lastTracePtr = holder_->ScenarioTestObj.StateItemHelpers.EventWriterHelper.GetEvent<ReconfigurationCompleteEventData>();
        auto lastTrace = *lastTracePtr;
        Verify::AreEqual(ft.FailoverUnitId.Guid, lastTrace.FtId, L"PartitionId");
        Verify::AreEqual(type, lastTrace.Type, L"Type");
        Verify::AreEqual(result, lastTrace.Result, L"Result");
        Verify::AreEqual(phase0, lastTrace.PerfData.Phase0Duration.TotalSeconds(), L"Phase0");
        Verify::AreEqual(phase1, lastTrace.PerfData.Phase1Duration.TotalSeconds(), L"Phase1");
        Verify::AreEqual(phase2, lastTrace.PerfData.Phase2Duration.TotalSeconds(), L"Phase2");
        Verify::AreEqual(phase3, lastTrace.PerfData.Phase3Duration.TotalSeconds(), L"Phase3");
        Verify::AreEqual(phase4, lastTrace.PerfData.Phase4Duration.TotalSeconds(), L"Phase4");       
    }

    void VerifyNoEvent()
    {
        Verify::IsTrue(holder_->ScenarioTestObj.StateItemHelpers.EventWriterHelper.IsTracedEventsEmpty());
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestReconfigurationStatistics::TestSetup()
{
    holder_ = ScenarioTestHolder::Create(UnitTestContext::Option::TestAssertEnabled | UnitTestContext::Option::StubJobQueueManager);
    GetScenarioTest().UTContext.Clock.SetManualMode();
    return true;
}

bool TestReconfigurationStatistics::TestCleanup()
{
    return true;
}

BOOST_AUTO_TEST_SUITE(Functional)

BOOST_FIXTURE_TEST_SUITE(TestReconfigurationStatisticsSuite, TestReconfigurationStatistics)

BOOST_AUTO_TEST_CASE(TestOtherReconfiguration)
{
    auto & test = GetScenarioTest();

    test.AddFT(ShortName, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I RD U N F 2:1]");

    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ShortName, L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]");

    AdvanceTime(2);

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ShortName, L"411/412 [P/P RD U 1:1] [I/S RD U 2:1] Success C");

    AdvanceTime(3);

    test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, ShortName, L"411/412 [I/S RD U 2:1] Success");
    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ShortName, L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [I/S RD U 2:1] Success ER");

    // Validation
    test.DumpFT(ShortName);

    Verify(ReconfigurationType::Other, ReconfigurationResult::Completed, 0, 0, 2, 0, 3);
}

BOOST_AUTO_TEST_CASE(TestSwapPrimaryReconfiguration)
{
    auto & test = GetScenarioTest();

    test.AddFT(ShortName, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    test.ProcessRemoteRAMessageAndDrain<MessageType::DoReconfiguration>(2, ShortName, L"411/422 [S/P RD U 1:1] [P/S RD U 2:1] 311");

    AdvanceTime(2);

    test.ProcessRAPMessage<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(ShortName, L"411/422 [S/P RD U 1:1 0 0] Success -");
    test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(2, ShortName, L"411/422 {411:0} [P/S RD U 2:1 0 0] Success");

    AdvanceTime(3);

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ShortName, L"411/422 [S/P RD U 1:1] [P/S RD U 2:1] Success C");

    AdvanceTime(5);

    test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, ShortName, L"411/422 [P/S RD U 2:1] Success");
    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ShortName, L"411/422 [S/P RD U 1:1] [P/S RD U 2:1] Success ER");

    // Validation
    test.DumpFT(ShortName);

    Verify(ReconfigurationType::SwapPrimary, ReconfigurationResult::Completed, 311, 2, 3, 0, 5);
}

BOOST_AUTO_TEST_CASE(TestFailover)
{
    auto & test = GetScenarioTest();

    test.AddFT(ShortName, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ShortName, L"411/422 [S/P RD U 1:1] [P/I RD D 2:1]");

    AdvanceTime(2);

    test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(ShortName, L"411/422 [S/P RD U 1:1 1 1] Success -");

    AdvanceTime(4);

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ShortName, L"411/422 [S/P RD U 1:1] [P/I RD D 2:1] Success C");

    AdvanceTime(3);

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ShortName, L"411/422 [S/P RD U 1:1] [S/P RD U 1:1] [P/I RD D 2:1] Success ER");

    // Validation
    test.DumpFT(ShortName);

    Verify(ReconfigurationType::Failover, ReconfigurationResult::Completed, 0, 2, 4, 0, 3);
}

BOOST_AUTO_TEST_CASE(TestChangeConfig)
{
    auto & test = GetScenarioTest();

    test.AddFT(ShortName, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1] [N/N/S RD U N F 3:1]");

    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ShortName, L"411/422 [S/P RD U 1:1] [P/I RD D 2:1] [S/S RD U 3:1]");

    AdvanceTime(2);

    test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(ShortName, L"411/422 [S/P RD U 1:1 1 1] Success -");
    test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, ShortName, L"411/422 [S/S RD U 3:1 1 2] Success");

    // Validation
    test.DumpFT(ShortName);

    Verify(ReconfigurationType::Failover, ReconfigurationResult::ChangeConfiguration, 0, 2, 0, 0, 0);
}

BOOST_AUTO_TEST_CASE(TestAbortSwapPrimary)
{
    auto & test = GetScenarioTest();

    test.AddFT(ShortName, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ShortName, L"411/422 [P/S RD U 1:1] [S/P RD U 2:1]");

    AdvanceTime(2);

    test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ShortName, L"411/433 [P/P RD U 1:1] [S/S IB U 2:1]");

    AdvanceTime(3);

    test.ProcessRAPMessageAndDrain<MessageType::CancelCatchupReplicaSetReply>(ShortName, L"411/422 [P/S RD U 1:1] Success -");

    // Validation
    test.DumpFT(ShortName);

    Verify(ReconfigurationType::SwapPrimary, ReconfigurationResult::AbortSwapPrimary, 5, 0, 0, 0, 0);
}

BOOST_AUTO_TEST_CASE(TestSecondaryIdleToActive)
{
    auto & test = GetScenarioTest();

    test.AddFT(ShortName, L"O None 000/000/411 1:1 CM [N/N/I IB U N F 1:1]");

    test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(2, ShortName, L"411/412 [P/P RD U 2:1] [I/S RD U 1:1]");

    test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ShortName, L"411/412 [I/S RD U 1:1] [P/P RD U 2:1] [I/S RD U 1:1] Success -");

    test.DumpFT(ShortName);

    VerifyNoEvent();
}

BOOST_AUTO_TEST_CASE(TestSecondarySecondaryToIdle)
{
    auto & test = GetScenarioTest();

    test.AddFT(ShortName, L"O None 000/000/411 1:1 CM [N/N/S SB U N F 1:1] [N/N/P RD U N F 2:1]");

    test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, ShortName, L"411/412 {411:0} false [P/P RD U 2:1] [S/I SB U 1:1]");

    test.DumpFT(ShortName);

    VerifyNoEvent();
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
