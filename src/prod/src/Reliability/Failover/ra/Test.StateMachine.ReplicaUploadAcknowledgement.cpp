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
    const wstring Persisted = L"SP1";
}

class TestStateMachine_ReplicaUploadAcknowledgement : public StateMachineTestBase
{
protected:
    template<MessageType::Enum E>
    void ReplicaAcknowledgementOnFTMessageTestHelper(wstring const & shortName, wstring const & initial, wstring const & message)
    {
        RestartAndProcessNodeUpAck(shortName, initial);

        Test.ProcessFMMessageAndDrain<E>(shortName, message);

        VerifyUploadIsComplete(shortName);
    }

    void RestartAndProcessNodeUpAck(wstring const & shortName, wstring const & initial)
    {
        Test.SetFindServiceTypeRegistrationSuccess(shortName);

        Test.AddFT(shortName, initial);

        Test.RestartRA();

        Test.ResetAll();

        auto & ft = Test.GetFT(shortName);

        Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            shortName,
            [&](EntityExecutionContext & base)
        {
            auto & context = base.As<FailoverUnitEntityExecutionContext>();
            ft.UpdateStateOnLFUMLoad(Test.UTContext.FederationWrapper.Instance, context);
            ft.ProcessNodeUpAck(vector<UpgradeDescription>(), true, context);

            if (ft.IsOpen)
            {
                if (ft.LocalReplicaOpenPending.IsSet)
                {
                    ft.ProcessReplicaOpenRetry(nullptr, context);
                }
                else if (ft.LocalReplicaClosePending.IsSet)
                {
                    ft.ProcessReplicaCloseRetry(nullptr, context);
                }
            }
        });

        Test.SetNodeUpAckProcessed(true, true);

        Verify::IsTrueLogOnError(ft.ReplicaUploadStateObj.IsUploadPending, wformatString("Upload Pending not true after node up ack\r\n{0}", ft));
    }

    void ProcessReopenReplyFromRAP()
    {
        Test.ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(Persisted, L"000/411 [N/P SB U 1:2] Success -");
    }

    void VerifyUploadIsComplete(wstring const & shortName)
    {
        auto & ft = Test.GetFT(shortName);

        TestLog::WriteInfo(wformatString("State: {0}", ft));
        Verify::IsTrueLogOnError(!ft.ReplicaUploadStateObj.IsUploadPending, L"Upload Pending true");
    }
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReplicaUploadAcknowledgementSuite, TestStateMachine_ReplicaUploadAcknowledgement)

BOOST_AUTO_TEST_CASE(AddPrimary)
{
    ReplicaAcknowledgementOnFTMessageTestHelper<MessageType::AddPrimary>(Persisted, L"C None 000/000/411 1:1 -", L"000/422 [N/P IB U 1:1:2:1]");
}

BOOST_AUTO_TEST_CASE(AddInstance)
{
    ReplicaAcknowledgementOnFTMessageTestHelper<MessageType::AddInstance>(L"SL1", L"C None 000/000/411 1:1 -", L"000/411 [N/N RD U 1:1:2:1]");
}

BOOST_AUTO_TEST_CASE(DoReconfiguration)
{
    RestartAndProcessNodeUpAck(Persisted, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    ProcessReopenReplyFromRAP();

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(Persisted, L"411/412 [P/P IB U 1:2:1:1] [S/S IB U 2:1]");

    VerifyUploadIsComplete(Persisted);
}

BOOST_AUTO_TEST_CASE(DeleteReplica)
{
    ReplicaAcknowledgementOnFTMessageTestHelper<MessageType::DeleteReplica>(Persisted, L"C None 000/000/411 1:1 -", L"000/411 [N/P RD U 1:1]");
}

BOOST_AUTO_TEST_CASE(CreateReplica)
{
    RestartAndProcessNodeUpAck(Persisted, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    ProcessReopenReplyFromRAP();

    Test.ProcessRemoteRAMessageAndDrain<MessageType::CreateReplica>(2, Persisted, L"411/411 [S/S IB U 1:2]");

    VerifyUploadIsComplete(Persisted);
}

BOOST_AUTO_TEST_CASE(GetLSN)
{
    RestartAndProcessNodeUpAck(Persisted, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    ProcessReopenReplyFromRAP();

    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(2, Persisted, L"411/422 [N/N RD U 1:2]");

    VerifyUploadIsComplete(Persisted);
}

BOOST_AUTO_TEST_CASE(Deactivate)
{
    RestartAndProcessNodeUpAck(Persisted, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    ProcessReopenReplyFromRAP();

    Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, Persisted, L"411/412 {411:0} false [S/S SB U 1:2:1:2] [P/P RD U 2:1]");

    VerifyUploadIsComplete(Persisted);
}

BOOST_AUTO_TEST_CASE(Activate)
{
    RestartAndProcessNodeUpAck(Persisted, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    ProcessReopenReplyFromRAP();

    Test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(2, Persisted, L"411/412 {411:0} [S/S SB U 1:2:1:2] [P/P RD U 2:1]");

    VerifyUploadIsComplete(Persisted);
}

BOOST_AUTO_TEST_CASE(ReplicaDown)
{
    RestartAndProcessNodeUpAck(Persisted, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    ProcessReopenReplyFromRAP();
    Test.ProcessAppHostClosedAndDrain(Persisted);

    Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/N SB D 1:2:1:2]} |");

    VerifyUploadIsComplete(Persisted);
}

BOOST_AUTO_TEST_CASE(ReplicaUp)
{
    RestartAndProcessNodeUpAck(Persisted, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");

    ProcessReopenReplyFromRAP();
    
    Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1 000/000/411 [N/N/P RD U 1:2:1:2]} | ");

    VerifyUploadIsComplete(Persisted);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
