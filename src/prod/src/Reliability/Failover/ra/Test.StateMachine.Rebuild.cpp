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

class TestStateMachine_Rebuild : public StateMachineTestBase
{
protected:
    template<MessageType::Enum T>
    void RebuildDropFMReplicaTestHelper()
    {
        // Higher Epoch and Higher Replica Id should drop the FM service replica
        GetScenarioTest().AddFT(L"FM", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

        GetScenarioTest().ProcessFMMessageAndDrain<T>(L"FM", L"000/422 [N/P RD U 1:2:1]");

        GetScenarioTest().ValidateFT(L"FM", L"O None 000/000/411 1:1 CMHb [N/N/P ID U N F 1:1]");
    }

    template<MessageType::Enum T>
    void RebuildDropFMReplicaEpochStalenessTestHelper()
    {
        // Lower Epoch is No-Op
        GetScenarioTest().AddFT(L"FM", L"O None 000/000/422 1:1 CM [N/N/P RD U N F 1:1]");

        GetScenarioTest().ProcessFMMessageAndDrain<T>(L"FM", L"000/411 [N/P RD U 1:2:1]");

        GetScenarioTest().ValidateFT(L"FM", L"O None 000/000/422 1:1 CM [N/N/P RD U N F 1:1]");
    }

    template<MessageType::Enum T>
    void RebuildDropFMReplicaReplicaIdStalenessTestHelper(bool isReopened)
    {
        // AddPrimary with lower replica id is no op
        GetScenarioTest().AddFT(L"FM", L"O None 000/000/411 2:1 CM [N/N/P RD U N F 1:1:2:1]");

        GetScenarioTest().ProcessFMMessageAndDrain<T>(L"FM", L"000/422 [N/P RD U 1:1]");

        if (!isReopened)
        {
            GetScenarioTest().ValidateFT(L"FM", L"O None 000/000/411 2:1 CM [N/N/P RD U N F 1:1:2:1]");
        }
        else
        {
            GetScenarioTest().ValidateFT(L"FM", L"O None 000/000/411 2:1 CMHr [N/N/P RD U N F 1:1:2:1]");
        }
    }

    void Rebuild_GenerationUpdate_IsProcessedIfRANotReady(bool isNodeUpAckFromFMReceived, bool isNodeUpAckFromFmmReceived, GenerationNumberSource::Enum source, int sourceGenerationNumber, wstring const & body);

    void Rebuild_GenerationUpdate_StalenessCheck(GenerationNumberSource::Enum source, int sourceGenerationNumber, wstring const & body, wstring const & rejectBody);

    void Rebuild_GenerationUpdate_GenerationIsSetTestInner(GenerationNumberSource::Enum source, wstring const & message, bool isSameNode);
    void Rebuild_GenerationUpdate_GenerationIsSetTestHelper(GenerationNumberSource::Enum source, wstring const & message, bool isSameNode);

    void Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::Enum source, wstring const & message, bool isSameNode);

    ScenarioTest & GetScenarioTest()
    {
        return Test;
    }
};

void TestStateMachine_Rebuild::Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::Enum source, wstring const & message, bool isSameNode)
{
    Recreate();

    Test.AddFT(L"FM", L"O None 000/000/522 1:1 CM [N/N/P RD U N F 1:1]");

    Rebuild_GenerationUpdate_GenerationIsSetTestInner(source, message, isSameNode);

    Test.ValidateFT(L"FM", L"O None 000/000/522 1:1 CM [N/N/P RD U N F 1:1]");
}

void TestStateMachine_Rebuild::Rebuild_GenerationUpdate_IsProcessedIfRANotReady(
    bool isNodeUpAckFromFMReceived,
    bool isNodeUpAckFromFmmReceived,
    GenerationNumberSource::Enum source,
    int sourceGenerationNumber,
    wstring const & body)
{
    Recreate();

    Test.SetNodeUpAckProcessed(isNodeUpAckFromFMReceived, isNodeUpAckFromFmmReceived);

    Test.GenerationStateHelper.SetProposedGeneration(source, GenerationInfo(sourceGenerationNumber, 1));

    Test.ProcessFMMessageAndDrain<MessageType::GenerationUpdate>(body);

    if (source == GenerationNumberSource::FM)
    {
        Test.ValidateNoFmmMessages();
        Test.StateItemHelpers.FMMessageHelper.ValidateCount(1);
    }
    else
    {
        Test.ValidateNoFMMessages();
        Test.StateItemHelpers.FMMMessageHelper.ValidateCount(1);
    }
}
void TestStateMachine_Rebuild::Rebuild_GenerationUpdate_StalenessCheck(GenerationNumberSource::Enum source, int sourceGenerationNumber, wstring const & body, wstring const & rejectBody)
{
    GetScenarioTest().GenerationStateHelper.SetProposedGeneration(source, GenerationInfo(sourceGenerationNumber, 1));

    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::GenerationUpdate>(body);

    if (source == GenerationNumberSource::FM)
    {
        GetScenarioTest().ValidateNoFmmMessages();
        GetScenarioTest().ValidateFMMessage<MessageType::GenerationUpdateReject>(rejectBody);
    }
    else
    {
        GetScenarioTest().ValidateNoFMMessages();
        GetScenarioTest().ValidateFmmMessage<MessageType::GenerationUpdateReject>(rejectBody);
    }
}

void TestStateMachine_Rebuild::Rebuild_GenerationUpdate_GenerationIsSetTestInner(
    GenerationNumberSource::Enum source, 
    wstring const & message, 
    bool isSameNode)
{
    int initialProposedGeneration = 4, initialSendGeneration = 2, initialReceiveGeneration = 3;
    int generationInMessage = 4;

    GenerationNumberSource::Enum other = Test.GenerationStateHelper.GetOther(source);

    Test.GenerationStateHelper.SetGeneration(source, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);
    Test.GenerationStateHelper.SetGeneration(other, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);

    NodeInstance senderNode = Test.RA.NodeInstance;
    if (!isSameNode)
    {
        senderNode = NodeInstance(NodeId(LargeInteger(senderNode.Id.IdValue.High, senderNode.Id.IdValue.Low + 1)), 1);
    }

    // Proposed generation matches
    Test.ProcessFMMessageAndDrain<MessageType::GenerationUpdate>(message, senderNode);

    // Validate that the generation number was set
    Test.GenerationStateHelper.VerifyGeneration(source, 1, initialSendGeneration, generationInMessage, initialProposedGeneration);
    Test.GenerationStateHelper.VerifyGeneration(other, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);

    // Validate that LFUM upload was done
    // The contents will be validated in another test
    Test.StateItemHelpers.ReliabilityHelper.ValidateLfumUploadCount(source);
    Test.StateItemHelpers.ReliabilityHelper.ValidateNoLfumUpload(other);
}

void TestStateMachine_Rebuild::Rebuild_GenerationUpdate_GenerationIsSetTestHelper(GenerationNumberSource::Enum source, wstring const & message, bool isSameNode)
{
    Recreate();

    Rebuild_GenerationUpdate_GenerationIsSetTestInner(source, message, isSameNode);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_RebuildSuite, TestStateMachine_Rebuild)

BOOST_AUTO_TEST_CASE(Rebuild_PrimaryCandidateWithOldPC)
{
    // Defect 1579052: If a replica receives DoReconfiguration with newer PC, then it should discard its local PC
    // and use the one in the message

    GetScenarioTest().AddFT(L"SP1", L"O None 411/444/444 {411:0} 1:1 CM [P/S/I RD U N 0 1:1] [S/P/P RD U N 0 2:1] [S/S/S SB U N 0 3:1]");

    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"433/455 [S/P IB U 1:1] [P/S IB U 2:1] [S/S IB D 3:1]");

    GetScenarioTest().ValidateFT(L"SP1", L"O Phase1_GetLSN 433/000/455 {411:0} 1:1 CMN [S/N/P RD U RAP 0 1:1] [P/N/S IC U RA 0 2:1] [S/N/S SB D RA 0 3:1]");
}

BOOST_AUTO_TEST_CASE(Rebuild_AddPrimary_DropsFMReplica)
{
    RebuildDropFMReplicaTestHelper<MessageType::AddPrimary>();
}

BOOST_AUTO_TEST_CASE(Rebuild_AddPrimary_StalenessEpoch)
{
    RebuildDropFMReplicaEpochStalenessTestHelper<MessageType::AddPrimary>();
}

BOOST_AUTO_TEST_CASE(Rebuild_AddPrimary_StalenessReplicaId)
{
    RebuildDropFMReplicaReplicaIdStalenessTestHelper<MessageType::AddPrimary>(false);
}

BOOST_AUTO_TEST_CASE(Rebuild_AddPrimary_ClosedFMReplica)
{
    GetScenarioTest().AddFT(L"FM", L"C None 000/000/411 1:1 -");

    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"FM", L"000/422 [N/P RD U 1:1:2:2]");

    GetScenarioTest().ValidateFT(L"FM", L"O None 000/000/422 2:2 S [N/N/P IC U N F 1:1:2:2]");
}

BOOST_AUTO_TEST_CASE(Rebuild_AddPrimary_FTDoesNotExist)
{
    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::AddPrimary>(L"FM", L"000/422 [N/P RD U 1:1:2:1]");

    GetScenarioTest().ValidateFT(L"FM", L"O None 000/000/422 2:1 S [N/N/P IC U N F 1:1:2:1]");
}

BOOST_AUTO_TEST_CASE(Rebuild_CreateReplica_DropsFMReplica)
{
    RebuildDropFMReplicaTestHelper<MessageType::CreateReplica>();
}

BOOST_AUTO_TEST_CASE(Rebuild_CreateReplica_StalenessEpoch)
{
    RebuildDropFMReplicaEpochStalenessTestHelper<MessageType::CreateReplica>();
}

BOOST_AUTO_TEST_CASE(Rebuild_CreateReplica_StalenessReplicaId)
{
    RebuildDropFMReplicaReplicaIdStalenessTestHelper<MessageType::CreateReplica>(true);
}

BOOST_AUTO_TEST_CASE(Rebuild_CreateReplica_ClosedFMReplica)
{
    GetScenarioTest().AddFT(L"FM", L"C None 000/000/411 1:1 -");

    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::CreateReplica>(L"FM", L"000/422 [N/I RD U 1:1:2:2]");

    GetScenarioTest().ValidateFT(L"FM", L"O None 000/000/422 2:2 S [N/N/I IC U N F 1:1:2:2]");
}

BOOST_AUTO_TEST_CASE(Rebuild_CreateReplica_FTDoesNotExist)
{
    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::CreateReplica>(L"FM", L"000/422 [N/I RD U 1:1:2:1]");

    GetScenarioTest().ValidateFT(L"FM", L"O None 000/000/422 2:1 S [N/N/I IC U N F 1:1:2:1]");
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_RAReadyTests)
{
    // in all of these the RA should process and send a reject
    // switch the is ready states in all combinations to ensure that RA processes this regardless of ready or not
    // FMM tests
    Rebuild_GenerationUpdate_IsProcessedIfRANotReady(false, false, GenerationNumberSource::FMM, 1, L"g:1:2,fmm 000");
    Rebuild_GenerationUpdate_IsProcessedIfRANotReady(false, true, GenerationNumberSource::FMM, 1, L"g:1:2,fmm 000");
    Rebuild_GenerationUpdate_IsProcessedIfRANotReady(true, false, GenerationNumberSource::FMM, 1, L"g:1:2,fmm 000");
    Rebuild_GenerationUpdate_IsProcessedIfRANotReady(true, true, GenerationNumberSource::FMM, 1, L"g:1:2,fmm 000");

    Rebuild_GenerationUpdate_IsProcessedIfRANotReady(false, false, GenerationNumberSource::FM, 1, L"g:1:2,fm 000");
    Rebuild_GenerationUpdate_IsProcessedIfRANotReady(false, true, GenerationNumberSource::FM, 1, L"g:1:2,fm 000");
    Rebuild_GenerationUpdate_IsProcessedIfRANotReady(true, false, GenerationNumberSource::FM, 1, L"g:1:2,fm 000");
    Rebuild_GenerationUpdate_IsProcessedIfRANotReady(true, true, GenerationNumberSource::FM, 1, L"g:1:2,fm 000");
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FMM_LowerGenerationSendsReject)
{
    Rebuild_GenerationUpdate_StalenessCheck(GenerationNumberSource::FMM, 1, L"g:1:2,fmm 000", L"1:1");
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FMM_HigherGenerationSendsReject)
{
    Rebuild_GenerationUpdate_StalenessCheck(GenerationNumberSource::FMM, 3, L"g:1:2,fmm 000", L"1:3");
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FM_LowerGenerationSendsReject)
{
    Rebuild_GenerationUpdate_StalenessCheck(GenerationNumberSource::FM, 1, L"g:1:2,fm 000", L"1:1");
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FM_HigherGenerationSendsReject)
{
    Rebuild_GenerationUpdate_StalenessCheck(GenerationNumberSource::FM, 3, L"g:1:2,fm 000", L"1:3");
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FMM_SetsGeneration)
{
    Rebuild_GenerationUpdate_GenerationIsSetTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm 000", true);
    Rebuild_GenerationUpdate_GenerationIsSetTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm 000", false);
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FM_SetsGeneration)
{
    Rebuild_GenerationUpdate_GenerationIsSetTestHelper(GenerationNumberSource::FM, L"g:1:4,fm 000", true);
    Rebuild_GenerationUpdate_GenerationIsSetTestHelper(GenerationNumberSource::FM, L"g:1:4,fm 000", false);
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FMReplicaDropNegativeTest)
{
    // Epoch on FM replica is 522
    // The FM replica is only dropped if the message that comes in:
    //  - is not from the same node as the current node
    //  - is not target for the FM replica set (i.e. the source of the message is FM and not FMM)
    //  - the epoch in the message is greater

    // Negative cases for message coming in from FMM
    // Epoch is greater in message
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm 633", false);
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm 633", true);

    // Epoch is equal to replica epoch in message
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm 522", false);
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm 522", true);

    // Epoch is lower to replica epoch in message
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm 411", false);
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FMM, L"g:1:4,fmm 411", true);

    // Message source is FM
    // Epoch is greater in message but source is the same node
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FM, L"g:1:4,fm 633", true);

    // Epoch is equal
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FM, L"g:1:4,fm 522", false);
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FM, L"g:1:4,fm 522", true);

    // Epoch is lower
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FM, L"g:1:4,fm 411", false);
    Rebuild_GenerationUpdate_FMReplicaDropNegativeTestHelper(GenerationNumberSource::FM, L"g:1:4,fm 411", true);
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FMReplicaDropPositiveTest)
{
    Test.AddFT(L"FM", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    GenerationNumberSource::Enum source = GenerationNumberSource::FM;
    int initialProposedGeneration = 4, initialSendGeneration = 2, initialReceiveGeneration = 3;

    GenerationNumberSource::Enum other = Test.GenerationStateHelper.GetOther(source);

    Test.GenerationStateHelper.SetGeneration(source, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);
    Test.GenerationStateHelper.SetGeneration(other, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);

    // The sender node is different
    NodeInstance senderNode = Test.RA.NodeInstance;
    senderNode = NodeInstance(NodeId(LargeInteger(senderNode.Id.IdValue.High, senderNode.Id.IdValue.Low + 1)), 1);

    // Proposed generation matches and epoch is higher and message is from different node
    Test.ProcessFMMessageAndDrain<MessageType::GenerationUpdate>(L"g:1:4,fm 633", senderNode);

    // Validate that the generation number was not set
    Test.GenerationStateHelper.VerifyGeneration(source, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);
    Test.GenerationStateHelper.VerifyGeneration(other, 1, initialSendGeneration, initialReceiveGeneration, initialProposedGeneration);

    // Validate that FM FT is being dropped (and not deleted)
    Test.ValidateFT(L"FM", L"O None 000/000/411 1:1 CMHb [N/N/P ID U N F 1:1]");

    // Verify no LFUM uploads
    Test.StateItemHelpers.ReliabilityHelper.ValidateNoLfumUpload(GenerationNumberSource::FM);
    Test.StateItemHelpers.ReliabilityHelper.ValidateNoLfumUpload(GenerationNumberSource::FMM);
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FMM_GenerationSetUploadsFMReplica)
{
    int initialReceive = 2;
    int numberInMessage = 3;
    int initialPropose = numberInMessage;
    int dontCareNumber = 120;

    GetScenarioTest().AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");
    GetScenarioTest().AddFT(L"FM", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    auto source = GenerationNumberSource::FMM;
    GetScenarioTest().GenerationStateHelper.SetGeneration(source, 1, dontCareNumber, initialReceive, initialPropose);

    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::GenerationUpdate>(L"g:1:3,fmm 411");

    GetScenarioTest().StateItemHelpers.ReliabilityHelper.ValidateLfumUpload(
        source,
        GenerationInfo(3, 1),
        1,
        true);

    GetScenarioTest().ValidateLfumUploadData(
        source,
        0,
        source == GenerationNumberSource::FMM ? L"FM" : L"SP1",
        L"000/000/411 [N/N/P RD U 1:1 -1 -1 (1.0:1.0:0)] [N/N/S RD U 2:1 -1 -1 (1.0:1.0:0)] (true)");

    GetScenarioTest().StateItemHelpers.ReliabilityHelper.ValidateNoLfumUpload(
        GetScenarioTest().GenerationStateHelper.GetOther(source));
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_FMM_GenerationSetUploadsNothingIfLfumEmpty)
{
    int initialReceive = 2;
    int numberInMessage = 3;
    int initialPropose = numberInMessage;
    int dontCareNumber = 120;

    GetScenarioTest().AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    auto source = GenerationNumberSource::FMM;
    GetScenarioTest().GenerationStateHelper.SetGeneration(source, 1, dontCareNumber, initialReceive, initialPropose);

    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::GenerationUpdate>(L"g:1:3,fmm 411");

    // AnyReplicaFound has to be true here
    // Look at the usage of AnyReplicaFound in the FM for the fm_.IsMaster cases
    GetScenarioTest().StateItemHelpers.ReliabilityHelper.ValidateLfumUpload(
        source,
        GenerationInfo(3, 1),
        0,
        true);

    GetScenarioTest().StateItemHelpers.ReliabilityHelper.ValidateNoLfumUpload(
        GetScenarioTest().GenerationStateHelper.GetOther(source));
}

BOOST_AUTO_TEST_CASE(Rebuild_GenerationUpdate_StateIsUploadedEvenIfNodeIsClosing)
{
    int initialReceive = 2;
    int numberInMessage = 3;
    int initialPropose = numberInMessage;
    int dontCareNumber = 120;

    GetScenarioTest().AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");
    GetScenarioTest().AddFT(L"FM", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1]");

    auto source = GenerationNumberSource::FM;
    GetScenarioTest().GenerationStateHelper.SetGeneration(source, 1, dontCareNumber, initialReceive, initialPropose);

    // Process the message but do not drain the queues
    // Then start closing the replicas
    GetScenarioTest().ProcessFMMessage<MessageType::GenerationUpdate>(L"g:1:3,fm 411");

    GetScenarioTest().StateItemHelpers.LifeCycleStateHelper.SetState(RALifeCycleState::Closing);

    GetScenarioTest().DrainJobQueues();

    GetScenarioTest().StateItemHelpers.ReliabilityHelper.ValidateLfumUpload(
        source,
        GenerationInfo(3, 1),
        1,
        true);

    GetScenarioTest().ValidateLfumUploadData(
        source,
        0,
        L"SP1",
        L"000/000/411 [N/N/P RD U 1:1 -1 -1 (1.0:1.0:1)] [N/N/S RD U 2:1 -1 -1 (1.0:1.0:1)] (true)");
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
