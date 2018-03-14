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
    const wstring UnknownAction = L"unknown123";
    const wstring NormalAction = L"AddPrimary";
    const wstring ProcessDuringCloseAction = L"ReplicaCloseReply";
    const wstring DeprecatedAction = L"ChangeConfigurationReply";

    namespace ContextType
    {
        enum Enum
        {
            OneWay,
            RequestReply,
        };
    }

    namespace State
    {
        enum Enum
        {
            None,
            Accepted,
            Rejected,
            Ignored,
            Replied
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case None:
                w << L"None";
                break;

            case Accepted:
                w << L"Accepted";
                break;

            case Rejected:
                w << L"Rejected";
                break;

            case Ignored:
                w << L"Ignored";
                break;

            case Replied:
                w << L"Replied";
                break;

            default:
                Assert::CodingError("Unknown test context state value {0}", static_cast<int>(e));
            }
        }
    }

    class TestValidator
    {
    public:
        TestValidator() : 
        state_(State::None)
        {
        }

        void VerifyState(State::Enum expected)
        {
            VerifyStateIs(expected, L"Validate Accepted");
        }

        void OnReject()
        {
            OnAction(State::Rejected);
        }

        void OnIgnore()
        {
            OnAction(State::Ignored);
        }

        void OnAccept()
        {
            OnAction(State::Accepted);
        }

        void OnReply()
        {
            OnAction(State::Replied);
        }
    
    private:
        void OnAction(State::Enum e)
        {
            VerifyStateIs(State::None, wformatString(e));
            state_ = e;
        }

        void VerifyStateIs(State::Enum expected, wstring const & tag)
        {
            Verify::AreEqual(expected, state_, wformatString("Verifying state is {0} at {1}. Current: {2}", expected, tag, state_));
        }

        State::Enum state_;
    };

    class TestOneWayContext : public Federation::OneWayReceiverContext
    {
    public:
        TestOneWayContext(TestValidator * validator) :
        Federation::OneWayReceiverContext(Federation::PartnerNodeSPtr(), Federation::NodeInstance(), Transport::MessageId()),
        validator_(validator)
        {
        }

        virtual void Accept()
        {
            validator_->OnAccept();
        }

        virtual void Reject(Common::ErrorCode const & , Common::ActivityId const & )
        {
            validator_->OnReject();
        }

        virtual void Ignore()
        {
            validator_->OnIgnore();
        }

    private:
        TestValidator * validator_;
    };

    class TestRequestReplyContext : public Federation::RequestReceiverContext
    {
    public:
        TestRequestReplyContext(TestValidator * validator) :
        Federation::RequestReceiverContext(Federation::SiteNodeSPtr(), Federation::PartnerNodeSPtr(), Federation::NodeInstance(), Transport::MessageId()),
        validator_(validator)
        {
        }

        virtual void Reject(Common::ErrorCode const &, Common::ActivityId const &)
        {
            validator_->OnReject();
        }

        virtual void Ignore()
        {
            validator_->OnIgnore();
        }

        virtual void Reply(Transport::MessageUPtr && )
        {
            validator_->OnReply();
        }

    private:
        TestValidator * validator_;
    };
}

class TestMessageHandler : public StateMachineTestBase
{
protected:
    TestMessageHandler()
    {
        Test.UTContext.FederationWrapper.ValidateRequestReplyContext = false;
    }

    void RunAndVerify(wstring const & action, ContextType::Enum e, State::Enum expected)
    {
        validator_ = TestValidator();

        auto msg = make_unique<Transport::Message>(FailoverUnit());
        msg->Headers.Add(Transport::ActionHeader(action));
        msg->Headers.Add(ScenarioTest::GetDefaultGenerationHeader());

        if (e == ContextType::OneWay)
        {
            auto context = make_unique<TestOneWayContext>(&validator_);
            Test.RA.MessageHandlerObj.ProcessTransportRequest(msg, move(context));
        }
        else if (e == ContextType::RequestReply)
        {
            auto context = make_unique<TestRequestReplyContext>(&validator_);
            Test.RA.MessageHandlerObj.ProcessTransportRequestResponseRequest(msg, move(context));
        }

        // Make the test fail to deserialize the body
        // So catch the assert failure thrown by transport or ra
        try
        {
            Test.DrainJobQueues();
        }
        catch (system_error const &)
        {
        }

        validator_.VerifyState(expected);
    }
    
    void NodeLifeCycleTestHelper(RALifeCycleState::Enum raState, wstring const & action, ContextType::Enum contextType, State::Enum expected)
    {
        Test.StateItemHelpers.LifeCycleStateHelper.SetState(raState);

        RunAndVerify(action, contextType, expected);
    }

    void RunMessageQueueFullTest(ContextType::Enum contextType, State::Enum expected)
    {
        Test.UTContext.ThreadpoolStubObj.FailEnqueueIntoMessageQueue = true;

        RunAndVerify(NormalAction, contextType, expected);
    }

    TestValidator validator_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestMessageHandlerSuite, TestMessageHandler)

BOOST_AUTO_TEST_CASE(OneWay_Deprecated_IsRejected)
{
    RunAndVerify(DeprecatedAction, ContextType::OneWay, State::Rejected);
}

BOOST_AUTO_TEST_CASE(OneWay_Unknown_IsRejected)
{
    RunAndVerify(UnknownAction, ContextType::OneWay, State::Rejected);
}

BOOST_AUTO_TEST_CASE(OneWay_Known_IsAccepted)
{
    // Important to call accept to stop federation retries
    RunAndVerify(NormalAction, ContextType::OneWay, State::Accepted);
}

BOOST_AUTO_TEST_CASE(OneWay_NodeClosed_IsRejected)
{
    NodeLifeCycleTestHelper(RALifeCycleState::Closed, NormalAction, ContextType::OneWay, State::Rejected);
}

BOOST_AUTO_TEST_CASE(OneWay_NodeClosedProcessDuringClose_IsRejected)
{
    NodeLifeCycleTestHelper(RALifeCycleState::Closed, ProcessDuringCloseAction, ContextType::OneWay, State::Rejected);
}

BOOST_AUTO_TEST_CASE(OneWay_NodeClosing_IsRejected)
{
    NodeLifeCycleTestHelper(RALifeCycleState::Closing, NormalAction, ContextType::OneWay, State::Rejected);
}

BOOST_AUTO_TEST_CASE(OneWay_NodeClosingProcessDuringClose_IsAccepted)
{
    NodeLifeCycleTestHelper(RALifeCycleState::Closing, ProcessDuringCloseAction, ContextType::OneWay, State::Accepted);
}

BOOST_AUTO_TEST_CASE(OneWay_QueueFull_IsRejected)
{
    RunMessageQueueFullTest(ContextType::OneWay, State::Rejected);
}

BOOST_AUTO_TEST_CASE(RequestReply_Deprecated_IsRejected)
{
    RunAndVerify(DeprecatedAction, ContextType::RequestReply, State::Rejected);
}

BOOST_AUTO_TEST_CASE(RequestReply_Unknown_IsRejected)
{
    RunAndVerify(UnknownAction, ContextType::RequestReply, State::Rejected);
}

BOOST_AUTO_TEST_CASE(RequestReply_Known_IsNoOp)
{
    RunAndVerify(NormalAction, ContextType::RequestReply, State::None);
}

BOOST_AUTO_TEST_CASE(RequestReply_NodeClosed_IsRejected)
{
    NodeLifeCycleTestHelper(RALifeCycleState::Closed, NormalAction, ContextType::RequestReply, State::Rejected);
}

BOOST_AUTO_TEST_CASE(RequestReply_NodeClosedProcessDuringClose_IsRejected)
{
    NodeLifeCycleTestHelper(RALifeCycleState::Closed, ProcessDuringCloseAction, ContextType::RequestReply, State::Rejected);
}

BOOST_AUTO_TEST_CASE(RequestReply_NodeClosing_IsRejected)
{
    NodeLifeCycleTestHelper(RALifeCycleState::Closing, NormalAction, ContextType::RequestReply, State::Rejected);
}

BOOST_AUTO_TEST_CASE(RequestReply_NodeClosingProcessDuringClose_IsAccepted)
{
    NodeLifeCycleTestHelper(RALifeCycleState::Closing, ProcessDuringCloseAction, ContextType::RequestReply, State::None);
}

BOOST_AUTO_TEST_CASE(RequestReply_QueueFull_IsRejected)
{
    RunMessageQueueFullTest(ContextType::RequestReply, State::Rejected);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
