// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;

class TestCancelUpgradeContext
{
protected:
    TestCancelUpgradeContext() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestCancelUpgradeContext() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTestUPtr scenarioTest_;
};

bool TestCancelUpgradeContext::TestSetup()
{
	scenarioTest_ = ScenarioTest::Create();
	return true;
}

bool TestCancelUpgradeContext::TestCleanup()
{
	return scenarioTest_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestCancelUpgradeContextSuite,TestCancelUpgradeContext)

BOOST_AUTO_TEST_CASE(CancelFabricUpgradeInstanceIdIsCorrect)
{
    auto body = Reader::ReadHelper<ServiceModel::FabricUpgradeSpecification>(L"2.0.0:cfg2:2 Rolling");
    auto context = CancelFabricUpgradeContext::Create(move(body), scenarioTest_->RA);
    Verify::AreEqual(2, context->InstanceId);
}

BOOST_AUTO_TEST_CASE(CancelApplicationUpgradeInstanceIdIsCorrect)
{
    auto body = Reader::ReadHelper<UpgradeDescription>(L"2 Rolling false | ");
    auto context = CancelApplicationUpgradeContext::Create(move(body), scenarioTest_->RA);
    Verify::AreEqual(2, context->InstanceId);
}

BOOST_AUTO_TEST_CASE(CancelFabricUpgradeSendsCorrectReply)
{
    auto body = StateManagement::Reader::ReadHelper<ServiceModel::FabricUpgradeSpecification>(L"2.0.0:cfg2:2 Rolling");
    auto context = CancelFabricUpgradeContext::Create(move(body), scenarioTest_->RA);
    context->SendReply();
    scenarioTest_->ValidateFMMessage<MessageType::CancelFabricUpgradeReply>(L"2.0.0:cfg2:2");
}

BOOST_AUTO_TEST_CASE(CancelApplicationUpgradeSendsCorrectReply)
{
    auto body = Reader::ReadHelper<UpgradeDescription>(L"2 Rolling false | ");
    auto context = CancelApplicationUpgradeContext::Create(move(body), scenarioTest_->RA);
    context->SendReply();
    scenarioTest_->ValidateFMMessage<MessageType::CancelApplicationUpgradeReply>(L"2 |");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
