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

class TestUnit_EndpointPublishState : public TestBase
{
protected:
    TestUnit_EndpointPublishState()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestUnit_EndpointPublishState()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void InvokeStart()
    {
        // for the purpose of testing endpoint publish it is sufficient to start and finish a failover reconfig
        auto context = CreateContext();
        reconfigState_->Start(ReconfigurationType::Failover, TimeSpan::Zero, GetClock());
        endpointPublishState_->StartReconfiguration(context);
    }

    void InvokeClear()
    {
        auto context = CreateContext();
        endpointPublishState_->Clear(context);
    }

    void FinishReconfig()
    {
        auto context = CreateContext();
        reconfigState_->FinishWithChangeConfiguration(Default::GetInstance().NodeInstance, GetClock(), context.ActionQueue);
    }

    void InvokeEndpointUpdate()
    {
        auto context = CreateContext();
        wasMessageSent_ = endpointPublishState_->OnEndpointUpdated(GetClock(), context);
    }

    void InvokeTimer()
    {
        auto context = CreateContext();
        endpointPublishState_->OnReconfigurationTimer(GetClock(), context, wasMessageSent_, wasRetryRequired_);
    }

    void InvokeFmReply()
    {
        auto context = CreateContext();
        endpointPublishState_->OnFmReply(context);
    }

    void VerifyState(StateExpectation::Enum expectation)
    {
        auto isEndpointPublishPending = endpointPublishState_->IsEndpointPublishPending;
        auto actualMessageStage = fmMessageState_.FMMessageStateObj.MessageStage;

        bool expectedIsEndpointPublishPending = expectation != StateExpectation::None && expectation != StateExpectation::Complete;
        FMMessageStage::Enum expectedMessageStage = expectation == StateExpectation::PublishAndMessagePending ? FMMessageStage::EndpointAvailable : FMMessageStage::None;
        Verify::AreEqualLogOnError(expectedIsEndpointPublishPending, isEndpointPublishPending, L"IsEndpointUpdatePending");
        Verify::AreEqualLogOnError(expectedMessageStage, actualMessageStage, L"Messagestage");
    }

    void VerifyMessageSent(bool expected)
    {
        Verify::AreEqual(expected, wasMessageSent_, L"Wasmessagesent");
    }

    void VerifyMessageSentAndRetryRequired(bool expectedWasMessageSent, bool expectedWasRetryRequired)
    {
        VerifyMessageSent(expectedWasMessageSent);

        Verify::AreEqual(expectedWasRetryRequired, wasRetryRequired_, L"WasRetryRequired");
    }

    void AdvanceTime()
    {
        GetClock().AdvanceTime(utContext_->Config.MaxWaitBeforePublishEndpointDuration);
    }

    TestClock & GetClock()
    {
        return utContext_->Clock;
    }

    UnitTestContextUPtr utContext_;
    FailoverUnitDescription ftDesc_;
    ServiceDescription sdDesc_;
    unique_ptr<ReconfigurationState> reconfigState_;
    FMMessageStateHolder fmMessageState_;
    unique_ptr<EndpointPublishState> endpointPublishState_;
    StopwatchTime start_;

    bool wasMessageSent_;
    bool wasRetryRequired_;
};

bool TestUnit_EndpointPublishState::TestSetup()
{
    utContext_ = UnitTestContext::Create();
    reconfigState_ = make_unique<ReconfigurationState>(&ftDesc_, &sdDesc_);
    
    endpointPublishState_ = make_unique<EndpointPublishState>(&utContext_->Config.MaxWaitBeforePublishEndpointDurationEntry, reconfigState_.get(), &fmMessageState_.FMMessageStateObj);

    start_ = utContext_->Clock.SetManualMode();
    wasMessageSent_ = false;
    wasRetryRequired_ = false;
    return true;
}

bool TestUnit_EndpointPublishState::TestCleanup()
{
    return utContext_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestUnit_EndpointPublishStateSuite, TestUnit_EndpointPublishState)

BOOST_AUTO_TEST_CASE(StartingAReconfigurationIsNoChange)
{
    InvokeStart();

    VerifyState(StateExpectation::None);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(FinishOfReconfigWithNoPrimaryEndpointIsNoChange)
{
    InvokeStart();

    InvokeClear();

    VerifyState(StateExpectation::None);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(StartAfterMultipleReconfigurationsIsNoChange)
{
    InvokeStart();

    InvokeClear();

    InvokeStart();

    VerifyState(StateExpectation::None);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(EndpointUpdateBeforeTimerDoesNotSendMessage)
{
    InvokeStart();

    InvokeEndpointUpdate();

    VerifyInMemoryCommit();

    VerifyMessageSent(false);
}

BOOST_AUTO_TEST_CASE(EndpointUpdateAfterTimerSendsMessage)
{
    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    VerifyInMemoryCommit();

    VerifyMessageSent(true);
}

BOOST_AUTO_TEST_CASE(EndpointUpdateAfterEndpointUpdateAndPublishSendsMessage)
{
    InvokeStart();

    InvokeEndpointUpdate();

    AdvanceTime();

    InvokeEndpointUpdate();

    VerifyInMemoryCommit();

    VerifyMessageSent(true);
}

BOOST_AUTO_TEST_CASE(MultipleEndpointUpdateAfterPublishSendsOnlyOneMessage)
{
    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    InvokeEndpointUpdate();

    VerifyNoCommit();

    VerifyMessageSent(false);
}

BOOST_AUTO_TEST_CASE(TimerBeforeEndpointUpdateIsNoop)
{
    InvokeStart();

    InvokeTimer();

    VerifyMessageSentAndRetryRequired(false, false);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(MultipleTimerBeforeEndpointUpdateIsNoop)
{
    InvokeStart();

    InvokeTimer();

    InvokeTimer();

    VerifyMessageSentAndRetryRequired(false, false);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(TimerAfterEndpointUpdateButBeforePublishInterval)
{
    InvokeStart();

    InvokeEndpointUpdate();

    InvokeTimer();

    VerifyMessageSentAndRetryRequired(false, true);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(MultipleTimerAfterEndpointUpdateButBeforePublishInterval)
{
    InvokeStart();

    InvokeEndpointUpdate();

    InvokeTimer();

    InvokeTimer();

    VerifyMessageSentAndRetryRequired(false, true);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(TimerAfterPublishIntervalSendsMessage)
{
    InvokeStart();

    InvokeEndpointUpdate();

    AdvanceTime();

    InvokeTimer();

    VerifyMessageSentAndRetryRequired(true, false);

    VerifyInMemoryCommit();
}

BOOST_AUTO_TEST_CASE(MultipleTimerAfterPublish)
{
    InvokeStart();

    InvokeEndpointUpdate();

    AdvanceTime();

    InvokeTimer();

    InvokeTimer();

    VerifyMessageSentAndRetryRequired(false, false);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(ClearAfterEndpointUpdate)
{
    InvokeStart();

    InvokeEndpointUpdate();

    InvokeClear();

    VerifyInMemoryCommit();

    InvokeStart();

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(TimerAfterFmReply)
{
    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    InvokeFmReply();

    VerifyInMemoryCommit();

    InvokeTimer();

    VerifyMessageSentAndRetryRequired(false, false);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(UpdateAfterFmReply)
{
    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    InvokeFmReply();

    InvokeEndpointUpdate();

    VerifyMessageSent(false);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(MultipleFmReply)
{
    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    InvokeFmReply();

    InvokeFmReply();

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(NextReconfigurationAfterFmReply)
{
    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    InvokeFmReply();

    FinishReconfig();

    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    VerifyMessageSent(true);
}

BOOST_AUTO_TEST_CASE(NextReconfigurationAfterClear)
{
    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    InvokeClear();

    FinishReconfig();

    InvokeStart();

    AdvanceTime();

    InvokeEndpointUpdate();

    VerifyMessageSent(true);
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
    
