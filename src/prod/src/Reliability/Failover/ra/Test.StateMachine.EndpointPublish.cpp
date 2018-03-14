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
    wstring const ShortName = L"SP1";

    namespace State
    {
        wstring const Closed(L"C None 000/000/411 1:1 -");
        wstring const OpeningPrimary(L"O None 411/000/422 1:1 MS [N/N/P IB U N F 1:1]");
        wstring const Ready(L"O None 000/000/422 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");
        wstring const NormalReconfigPhase2Catchup(L"O Phase2_Catchup 411/000/412 1:1 CMN [P/N/P RD U N F 1:1] [I/N/S RD U N F 2:1]");
        wstring const SwapReconfigPhase2Catchup(L"O Phase2_Catchup 411/000/422 1:1 CMN [P/N/S RD U N F 1:1] [S/N/P RD U N F 2:1]");
        wstring const PromoteReconfigPhase2Catchup(L"O Phase2_Catchup 411/000/422 1:1 CMN [S/N/P RD U N F 1:1] [P/N/S RD U N F 2:1]");
        wstring const PromoteReconfigPhase3Deactivate(L"O Phase3_Deactivate 411/422/422 1:1 CMN [S/P/P RD U N F 1:1] [P/S/S RD U N F 2:1]");
    }

    namespace UCReply
    {
        wstring const NormalReconfigPhase2 = L"411/412 [P/P RD U 1:1] [P/P RD U 1:1] [I/S RD U 2:1] Success C";
        wstring const SwapReconfigPhase2 = L"411/422 [P/S RD U 1:1] [P/S RD U 1:1] [S/P RD U 2:1] Success C";
        wstring const PromoteReconfigPhase2 = L"411/422 [S/P RD U 1:1] [S/P RD U 1:1] [P/S RD U 2:1] Success C";
        wstring const PromoteReconfigPhase3 = L"411/422 [S/P RD U 1:1] [S/P RD U 1:1] [P/S RD U 2:1] Success -";
    }

    namespace StateExpectation
    {
        enum Enum
        {
            None,
            PublishPending,
            PublishAndMessagePending,
            Complete
        };
    }
}

class TestStateMachine_EndpointPublish : public StateMachineTestBase
{
protected:
    TestStateMachine_EndpointPublish()
    {
        Test.UTContext.Clock.SetManualMode();
    }

    void ResetAndFireReconfigTimer()
    {
        Test.ResetAll();
        FireReconfigTimer();
    }

    void DumpFT()
    {
        Test.DumpFT(ShortName);
    }

    void FireReconfigTimer()
    {
        Test.StateItemHelpers.ReconfigurationMessageRetryHelper.RequestWork();
        Test.DrainJobQueues();
    }

    void AddFT(wstring const & state)
    {
        Test.AddFT(ShortName, state);

        if (state == State::NormalReconfigPhase2Catchup)
        {
            ucReply_ = UCReply::NormalReconfigPhase2;
        }
        else if (state == State::SwapReconfigPhase2Catchup)
        {
            ucReply_ = UCReply::SwapReconfigPhase2;
        }
        else if (state == State::PromoteReconfigPhase2Catchup)
        {
            ucReply_ = UCReply::PromoteReconfigPhase2;
        }
        else if (state == State::PromoteReconfigPhase3Deactivate)
        {
            ucReply_ = UCReply::PromoteReconfigPhase3;
        }

        if (GetFT().IsReconfiguring)
        {
            SetReconfigStartTimeToNow();
        }
    }

    void ProcessAppHostDown()
    {
        Test.ProcessAppHostClosedAndDrain(ShortName);
    }

    void ProcessUCReply()
    {
        Verify::IsTrueLogOnError(!ucReply_.empty(), L"UC reply cannot be empty");
        Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", ucReply_);
    }

    void ProcessProxyEndpointUpdate()
    {
        ProcessProxyEndpointUpdate(L"422");
    }

    void ProcessProxyEndpointUpdate(wstring const & ccEpoch)
    {
        auto msg = wformatString("000/{0} [N/P RD U 1:1 zxc]", ccEpoch);

        Test.ProcessRAPMessageAndDrain<MessageType::ProxyReplicaEndpointUpdated>(ShortName, msg);
    }

    void ProcessReplicaEndpointUpdateReply()
    {
        ProcessReplicaEndpointUpdateReply(L"422", L"Success");
    }

    void ProcessReplicaEndpointUpdateReply(wstring const & ccEpoch, wstring const & errorCode)
    {
        auto msg = wformatString("000/{0} [N/P RD U 1:1] {1}", ccEpoch, errorCode);

        Test.ProcessFMMessageAndDrain<MessageType::ReplicaEndpointUpdatedReply>(ShortName, msg);
    }
    
    void SetReconfigStartTimeToNow()
    {
        auto time = GetClock().SetManualMode();

        GetFT().Test_GetReconfigurationState().Test_SetStartTime(time);
    }

    void AdvanceTimeByConfiguration()
    {
        auto value = Test.UTContext.Config.MaxWaitBeforePublishEndpointDuration;
        GetClock().AdvanceTime(value);
    }

    void VerifyState(StateExpectation::Enum expectation)
    {
        auto isEndpointPublishPending = GetFT().IsEndpointPublishPending;
        auto actualMessageStage = GetFT().FMMessageStage;

        bool expectedIsEndpointPublishPending = expectation != StateExpectation::None && expectation != StateExpectation::Complete;
        FMMessageStage::Enum expectedMessageStage = expectation == StateExpectation::PublishAndMessagePending ? FMMessageStage::EndpointAvailable : FMMessageStage::None;
        Verify::AreEqualLogOnError(expectedIsEndpointPublishPending, isEndpointPublishPending, L"IsEndpointUpdatePending");
        Verify::AreEqualLogOnError(expectedMessageStage, actualMessageStage, L"Messagestage");
    }

    void VerifyNoFMMessage()
    {
        Test.ValidateNoFMMessages();
    }

    void VerifyFMMessage()
    {
        Test.ValidateFMMessage<MessageType::ReplicaEndpointUpdated>();
    }

    void NoopTestHelper(wstring const & state, MessageType::Enum type)
    {
        AddFT(state);

        AdvanceTimeByConfiguration();

        if (type == MessageType::UpdateConfiguration)
        {
            ProcessUCReply();
        }
        else if (type == MessageType::ProxyReplicaEndpointUpdated)
        {
            ProcessProxyEndpointUpdate();
        }

        VerifyState(StateExpectation::None);

        VerifyNoFMMessage();
    }

    void FMReplyStaleTestHelper(wstring const & ccEpoch, wstring const & errorCode)
    {
        AddFT(State::PromoteReconfigPhase2Catchup);

        AdvanceTimeByConfiguration();

        ProcessProxyEndpointUpdate();

        ProcessReplicaEndpointUpdateReply(ccEpoch, errorCode);

        VerifyState(StateExpectation::PublishAndMessagePending);
    }

    void SetupForEndpointReplyTest()
    {
        AddFT(State::PromoteReconfigPhase2Catchup);

        AdvanceTimeByConfiguration();

        ProcessProxyEndpointUpdate();

        ProcessReplicaEndpointUpdateReply();
    }

    TestClock & GetClock()
    {
        return Test.UTContext.Clock;
    }

    FailoverUnit & GetFT()
    {
        return Test.GetFT(ShortName);
    }

    wstring ucReply_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_EndpointPublishSuite, TestStateMachine_EndpointPublish)

BOOST_AUTO_TEST_CASE(EndpointUpdateToFMMessageContentsTest)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    ProcessProxyEndpointUpdate(L"422");

    AdvanceTimeByConfiguration();

    ResetAndFireReconfigTimer();

    Test.ValidateFMMessage<MessageType::ReplicaEndpointUpdated>(L"SP1", L"411/422 [S/P RD U 1:1 -1 -1 (1.0:1.0:1) zxc]");
}

BOOST_AUTO_TEST_CASE(EndpointUpdateReplyToRAPMessageContentsTest)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    ProcessProxyEndpointUpdate();

    Test.ValidateRAPMessage<MessageType::ProxyReplicaEndpointUpdatedReply>(L"SP1", L"000/422 [N/P RD U 1:1 -1 -1 (1.0:1.0:1) zxc]");
}

BOOST_AUTO_TEST_CASE(PublishPending_UCReplyDuringPhase2Catchup)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    ProcessUCReply();

    VerifyState(StateExpectation::PublishPending);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(PublishPending_ProxyEndpointUpdateDuringPhase2Catchup)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    ProcessProxyEndpointUpdate();

    VerifyState(StateExpectation::PublishPending);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(PublishAndMessagePending_MessageAfterTimeout)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    AdvanceTimeByConfiguration();

    ProcessProxyEndpointUpdate();

    VerifyState(StateExpectation::PublishAndMessagePending);

    VerifyFMMessage();
}

BOOST_AUTO_TEST_CASE(PublishAndMessagePending_TimeoutAfterEndpointUpdate)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    ProcessProxyEndpointUpdate();

    AdvanceTimeByConfiguration();

    ResetAndFireReconfigTimer();

    VerifyState(StateExpectation::PublishAndMessagePending);

    VerifyFMMessage();
}

BOOST_AUTO_TEST_CASE(Noop_UCReply_DuringPhase2Swap)
{
    NoopTestHelper(State::SwapReconfigPhase2Catchup, MessageType::UpdateConfiguration);
}

BOOST_AUTO_TEST_CASE(Noop_UCReply_DuringPhase2Other)
{
    NoopTestHelper(State::NormalReconfigPhase2Catchup, MessageType::UpdateConfiguration);
}

BOOST_AUTO_TEST_CASE(Noop_UCReply_DuringPhase3)
{
    NoopTestHelper(State::PromoteReconfigPhase3Deactivate, MessageType::UpdateConfiguration);
}

BOOST_AUTO_TEST_CASE(Noop_ProxyEndpointUpdate_DuringPhase2Swap)
{
    NoopTestHelper(State::SwapReconfigPhase2Catchup, MessageType::ProxyReplicaEndpointUpdated);
}

BOOST_AUTO_TEST_CASE(Noop_ProxyEndpointUpdate_DuringPhase2Other)
{
    NoopTestHelper(State::NormalReconfigPhase2Catchup, MessageType::ProxyReplicaEndpointUpdated);
}

BOOST_AUTO_TEST_CASE(Noop_ProxyEndpointUpdate_DuringPhase3)
{
    NoopTestHelper(State::PromoteReconfigPhase3Deactivate, MessageType::ProxyReplicaEndpointUpdated);
}

BOOST_AUTO_TEST_CASE(Noop_ProxyEndpointUpdate_StaleEpoch)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    AdvanceTimeByConfiguration();

    ProcessProxyEndpointUpdate(L"411");

    VerifyState(StateExpectation::None);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(Noop_ProxyEndpointUpdate_NotReconfiguring)
{
    AddFT(State::Ready);

    AdvanceTimeByConfiguration();

    ProcessProxyEndpointUpdate();

    VerifyState(StateExpectation::None);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(Noop_ProxyEndpointUpdateAfterAcknowledgement)
{
    SetupForEndpointReplyTest();

    Test.ResetAll();

    ProcessProxyEndpointUpdate();

    VerifyState(StateExpectation::Complete);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(ProxyEndpointReplyIsSent_EvenIfStale)
{
    AddFT(State::Ready);

    ProcessProxyEndpointUpdate();

    Test.ValidateRAPMessage<MessageType::ProxyReplicaEndpointUpdatedReply>(ShortName);
}

BOOST_AUTO_TEST_CASE(Timer_PublishPending_DoesNotSendMessage)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    ProcessProxyEndpointUpdate();

    ResetAndFireReconfigTimer();

    VerifyState(StateExpectation::PublishPending);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(Timer_None_Noop)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    AdvanceTimeByConfiguration();

    ResetAndFireReconfigTimer();

    VerifyState(StateExpectation::None);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(Timer_Complete_Noop)
{
    SetupForEndpointReplyTest();

    ResetAndFireReconfigTimer();

    VerifyState(StateExpectation::Complete);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(Reply_Stale_IfError)
{
    FMReplyStaleTestHelper(L"422", L"GlobalLeaseLost");
}

BOOST_AUTO_TEST_CASE(Reply_Stale_IfEpochMismatch)
{
    FMReplyStaleTestHelper(L"411", L"Success");
}

BOOST_AUTO_TEST_CASE(Reply_Ignored_IfEndpointUpdateNotPending)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    AdvanceTimeByConfiguration();

    ProcessProxyEndpointUpdate();

    ProcessAppHostDown();

    ProcessReplicaEndpointUpdateReply(L"422", L"Success");

    DumpFT();

    Verify::AreEqual(FMMessageStage::ReplicaDown, GetFT().FMMessageStage, L"Endpoint state");
}

BOOST_AUTO_TEST_CASE(Reply_Duplicate)
{
    SetupForEndpointReplyTest();

    Test.ResetAll();

    ProcessReplicaEndpointUpdateReply();

    VerifyState(StateExpectation::Complete);
}

BOOST_AUTO_TEST_CASE(Reply_Processed)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    AdvanceTimeByConfiguration();

    ProcessProxyEndpointUpdate();

    Test.ResetAll();

    ProcessReplicaEndpointUpdateReply();

    VerifyState(StateExpectation::Complete);

    VerifyNoFMMessage();
}

BOOST_AUTO_TEST_CASE(ReconfigurationCompleteClearsEndpointPublish)
{
    AddFT(State::PromoteReconfigPhase2Catchup);

    AdvanceTimeByConfiguration();

    ProcessUCReply();

    DumpFT();

    Test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, ShortName, L"411/422 [P/S RD U 2:1] Success");
    
    DumpFT();

    Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ShortName, L"411/422 [S/P RD U 1:1] [S/P RD U 1:1] [P/S RD U 2:1] Success ER");

    DumpFT();

    VerifyState(StateExpectation::None);
}
BOOST_AUTO_TEST_CASE(MultipleReconfigurationsWork)
{
    
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
