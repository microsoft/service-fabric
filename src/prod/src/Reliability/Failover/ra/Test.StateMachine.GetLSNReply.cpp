// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::ReplicaUp;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestStateMachine_GetLSNReply
{
protected:
        TestStateMachine_GetLSNReply() { BOOST_REQUIRE(TestSetup()); }
        TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_GetLSNReply() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void RunUpReplicaReplyGetLSNTest(wstring const & replicaInitial, wstring const & reply, wstring const & replicaExpected);
    void RunUpReplicaReplyGetLSNTest(wstring const & reply, wstring const & replicaExpected);
    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_GetLSNReply::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();
    return true;
}

bool TestStateMachine_GetLSNReply::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_GetLSNReply::RunUpReplicaReplyGetLSNTest(wstring const & replicaInitial, wstring const & reply, wstring const & replicaExpected)
{
    auto ftMaker = [](wstring const & remoteReplica) { return wformatString("O Phase1_GetLSN 411/000/522 1:1 CM [S/N/P RD U N F 1:1] {0} [S/N/S RD U RA F 5:1]", remoteReplica); };

    auto shortName = L"SP1";
    auto & test = holder_->Recreate();

    test.AddFT(shortName, ftMaker(replicaInitial));

    test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(2, shortName, reply);

    test.ValidateFT(shortName, ftMaker(replicaExpected));
}

void TestStateMachine_GetLSNReply::RunUpReplicaReplyGetLSNTest(wstring const & reply, wstring const & replicaExpected)
{
    RunUpReplicaReplyGetLSNTest(L"[S/N/S RD U RA F 2:1:2:1]", reply, replicaExpected);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_GetLSNReplySuite, TestStateMachine_GetLSNReply)

BOOST_AUTO_TEST_CASE(NoDeactivationInfo_SetsLSN)
{
    RunUpReplicaReplyGetLSNTest(L"411/522 {000:-1} [N/N RD U 2:1 5 5] Success", L"[S/N/S RD U N F 2:1 5 5 000:-1]");
}

BOOST_AUTO_TEST_CASE(DeactivationInfo_SetsLSN)
{
    RunUpReplicaReplyGetLSNTest(L"411/522 {411:7} [N/N RD U 2:1 5 5] Success", L"[S/N/S RD U N F 2:1 5 5 411:7]");
}

BOOST_AUTO_TEST_CASE(Dropped_MarksReplicaAsDropped)
{
    RunUpReplicaReplyGetLSNTest(L"411/522 {000:-1} [N/N DD U 2:1 -1 -1] Success", L"[S/N/S DD D N F 2:1]");
}

BOOST_AUTO_TEST_CASE(NotFound_MarksReplicaAsNotFound)
{
    RunUpReplicaReplyGetLSNTest(L"411/522 {000:-1} [N/N RD U 2:1:3:1 -1 -1] NotFound", L"[S/N/S RD U N F 2:1 -2 -2]");
}

BOOST_AUTO_TEST_CASE(NotFound_DoesNotChangeReplicaState)
{
    RunUpReplicaReplyGetLSNTest(L"[S/N/S IB U RA F 2:1:2:1]", L"411/522 {000:-1} [N/N RD U 2:1:3:1 -1 -1] NotFound", L"[S/N/S IB U N F 2:1 -2 -2]");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
