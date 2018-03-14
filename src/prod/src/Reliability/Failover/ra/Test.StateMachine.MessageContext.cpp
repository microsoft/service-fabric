// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Common;
using namespace std;
using namespace ReliabilityUnitTest;
using namespace StateManagement;
using namespace Infrastructure;

class TestRAMessageContext
{
protected:
    TestRAMessageContext() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestRAMessageContext() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTestUPtr scenarioTest_;
};

bool TestRAMessageContext::TestSetup()
{
	scenarioTest_ = ScenarioTest::Create();
	return true;
}

bool TestRAMessageContext::TestCleanup()
{
	return scenarioTest_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestRAMessageContextSuite,TestRAMessageContext)

BOOST_AUTO_TEST_CASE(GenerationHeaderShouldBeCheckedForTcpMessage)
{
    // Regression test for #1227401
    scenarioTest_->AddClosedFT(L"SP1");

    auto message = scenarioTest_->ParseMessage<MessageType::AddPrimary>(L"SP1", L"000/411 [N/P RD U 1:1]");

    ReplicaMessageBody body;
    bool isBodyValid = message->GetBody(body);
    VERIFY_IS_TRUE(isBodyValid);

    GenerationHeader incomingGenerationHeader = GenerationHeader::ReadFromMessage(*message);

    GenerationNumber invalidGeneration(incomingGenerationHeader.Generation.Value - 1, ReconfigurationAgent::InvalidNode.Id);
    GenerationHeader invalidGenerationHeader(invalidGeneration, incomingGenerationHeader.IsForFMReplica);

    auto metadata = scenarioTest_->RA.MessageMetadataTable.LookupMetadata(message);

    MessageContext<ReplicaMessageBody, FailoverUnit> context(
        metadata, 
        message->Action, 
        move(body), 
        invalidGenerationHeader, 
        Default::GetInstance().NodeInstance, 
        &ReconfigurationAgent::AddPrimaryMessageProcessor);

    LockedFailoverUnitPtr ft(scenarioTest_->GetLFUMEntry(L"SP1")->Test_CreateLock());
    StateMachineActionQueue queue;
    HandlerParameters params("b", ft, scenarioTest_->RA, queue, nullptr, L"A");

    bool result = context.Process(params);
    VERIFY_IS_FALSE(result);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
