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
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestHealthReportReplica : public TestBase
{
protected:
    TestHealthReportReplica()
    {
    }


    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    HealthReportReplica GenerateHealthReportReplica(
        ReplicaRole::Enum previousRole,
        ReplicaRole::Enum currentRole,
        bool isUp,
        ReconfigurationAgentComponent::ReplicaStates::Enum state,
        int64 replicaId,
        int64 nodeId)
    {
        auto nodeInstance = CreateNodeInstanceEx(nodeId, 1);
        auto rv = Replica(nodeInstance, replicaId, 1);
        rv.PreviousConfigurationRole = previousRole;
        rv.CurrentConfigurationRole = currentRole;
        rv.IsUp = isUp;
        rv.State = state;

        return HealthReportReplica(rv);
    }
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestHealthReportReplicaSuite, TestHealthReportReplica)

BOOST_AUTO_TEST_CASE(HealthReportReplicaStringRepresentationTest)
{
    //NodeId is 14 in string because its a hex value
    wstring expected = L"S/P InBuild 14 2";
    auto hrp = GenerateHealthReportReplica(ReplicaRole::Secondary, ReplicaRole::Primary, true, ReconfigurationAgentComponent::ReplicaStates::InBuild, 2, 20);

    Verify::AreEqual(expected, hrp.GetStringRepresentation());
}

BOOST_AUTO_TEST_CASE(HealthReportReplicaStringRepresentation2Test)
{
    //NodeId is f in string because its a hex value
    wstring expected = L"I/S Down f 6";
    auto hrp = GenerateHealthReportReplica(ReplicaRole::Idle, ReplicaRole::Secondary, false, ReconfigurationAgentComponent::ReplicaStates::Ready, 6, 15);

    Verify::AreEqual(expected, hrp.GetStringRepresentation());
}

BOOST_AUTO_TEST_CASE(HealthReportReplicaStringRepresentation3Test)
{
    //NodeId is f in string because its a hex value
    wstring expected = L"P/S Dropped 10 3";
    auto hrp = GenerateHealthReportReplica(ReplicaRole::Primary, ReplicaRole::Secondary, false, ReconfigurationAgentComponent::ReplicaStates::Dropped, 3, 16);

    Verify::AreEqual(expected, hrp.GetStringRepresentation());
}

BOOST_AUTO_TEST_CASE(EqualityOperatorTest)
{
    auto baseHrp = GenerateHealthReportReplica(ReplicaRole::Secondary, ReplicaRole::Primary, true, ReconfigurationAgentComponent::ReplicaStates::InBuild, 2, 20);
    auto equalHrp = GenerateHealthReportReplica(ReplicaRole::Secondary, ReplicaRole::Primary, true, ReconfigurationAgentComponent::ReplicaStates::InBuild, 2, 20);
    Verify::IsTrue(baseHrp == equalHrp, L"Difference found in equal HealthReportReplicas");

    auto difPCRole = GenerateHealthReportReplica(ReplicaRole::Primary, ReplicaRole::Primary, true, ReconfigurationAgentComponent::ReplicaStates::InBuild, 2, 20);
    Verify::IsTrue(baseHrp != difPCRole, L"PC Role");

    auto difCCRole = GenerateHealthReportReplica(ReplicaRole::Secondary, ReplicaRole::Secondary, true, ReconfigurationAgentComponent::ReplicaStates::InBuild, 2, 20);
    Verify::IsTrue(baseHrp != difCCRole, L"CC Role");

    auto downReplica = GenerateHealthReportReplica(ReplicaRole::Secondary, ReplicaRole::Primary, false, ReconfigurationAgentComponent::ReplicaStates::InBuild, 2, 20);
    Verify::IsTrue(baseHrp != downReplica, L"Up/Down difference");

    auto difStateReplica = GenerateHealthReportReplica(ReplicaRole::Secondary, ReplicaRole::Primary, true, ReconfigurationAgentComponent::ReplicaStates::Ready, 2, 20);
    Verify::IsTrue(baseHrp != difStateReplica, L"Replica State");

    auto difReplicaId = GenerateHealthReportReplica(ReplicaRole::Secondary, ReplicaRole::Primary, true, ReconfigurationAgentComponent::ReplicaStates::InBuild, 3, 20);
    Verify::IsTrue(baseHrp != difReplicaId, L"ReplicaId");

    auto difNodeId = GenerateHealthReportReplica(ReplicaRole::Secondary, ReplicaRole::Primary, true, ReconfigurationAgentComponent::ReplicaStates::InBuild, 2, 21);
    Verify::IsTrue(baseHrp != difNodeId, L"NodeId");
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
