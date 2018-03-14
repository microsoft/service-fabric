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
    const wstring FT = L"SP1";
}

class TestStateMachine_ReadWriteStatusRevokedNotification : public StateMachineTestBase
{
protected:
    void AddClosingFT()
    {
        Test.AddFT(FT, L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");
    }

    void AddReadySecondaryFTWithConfiguration()
    {
        Test.AddFT(FT, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");
    }

    void ProcessMessage()
    {
        Test.ProcessRAPMessageAndDrain<MessageType::ReadWriteStatusRevokedNotification>(FT, L"000/411 [N/P RD U 1:1]");
    }

    void ValidateReply()
    {
        Test.ValidateRAPMessage<MessageType::ReadWriteStatusRevokedNotificationReply>(FT, L"000/411 [N/P RD U 1:1] -");
    }
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReadWriteStatusRevokedNotificationSuite, TestStateMachine_ReadWriteStatusRevokedNotification)

BOOST_AUTO_TEST_CASE(ReadWriteNotificationIsProcessed)
{
    AddClosingFT();

    ProcessMessage();

    Verify::AreEqualLogOnError(FMMessageStage::ReplicaDown, Test.GetFT(FT).FMMessageStage, L"IsProcessed");
}

BOOST_AUTO_TEST_CASE(ReplyIsSentIfProcessed)
{
    AddClosingFT();

    ProcessMessage();

    ValidateReply();
}

BOOST_AUTO_TEST_CASE(ReplyIsSentIfNotProcessed)
{
    AddReadySecondaryFTWithConfiguration();

    ProcessMessage();

    ValidateReply();
}

BOOST_AUTO_TEST_CASE(LowerEpochIsConsidered)
{
    // Important for deactivate - it may be that the RA has higher epoch due to processing deactivate S/N
    // But RAP does not
    AddReadySecondaryFTWithConfiguration();

    Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, FT, L"411/412 {411:0} false [P/P RD U 2:1] [S/N RD U 1:1]");

    Test.ResetAll();
    ProcessMessage();

    ValidateReply();
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
