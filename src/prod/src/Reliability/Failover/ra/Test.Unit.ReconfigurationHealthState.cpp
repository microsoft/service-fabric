// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Common::ApiMonitoring;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;
using namespace Health;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;


class TestReconfigurationHealthState
{
protected:
    TestReconfigurationHealthState()
        : localReplicaId_(1),
        localReplicaInstanceId_(2),
        rhs_(&failoverUnitId_, &serviceDescription_, &localReplicaId_, &localReplicaInstanceId_),
        reconfigurationStartTime_(627741666350000000),
        reconfigurationPhaseStartTime_(627741666340000000)
    {
        BOOST_REQUIRE(TestSetup());
    }


    ~TestReconfigurationHealthState()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    void AddReplicas(wstring const & s)
    {
        vector<Replica> v;
        Reader r(s, ReadContext());
        bool check = r.ReadVector(L'[', L']', v);
        Verify::IsTrueLogOnError(check, L"Failed to read vector");

        holder_ = ReplicaStoreHolder(move(v));
    }

    void VerifyQueueCount(int count, StateMachineActionQueue & queue)
    {
        if (count > 0)
        {
            Verify::IsTrue(!queue.IsEmpty);
        }

        Verify::AreEqual(count, queue.ActionQueue.size());
    }

    void VerifyHealthReportType(int index, Health::ReplicaHealthEvent::Enum healthEventType)
    {
        HealthSubsystemWrapperStub & healthsub = dynamic_cast<HealthSubsystemWrapperStub &>(scenarioTest_.ScenarioTestObj.RA.HealthSubsystem);
        Verify::AreEqual(healthEventType, healthsub.ReplicaHealthEvents[index].Type);
    }

    void VerifyHealthReportCount(int count)
    {
        HealthSubsystemWrapperStub & healthsub = dynamic_cast<HealthSubsystemWrapperStub &>(scenarioTest_.ScenarioTestObj.RA.HealthSubsystem);
        Verify::AreEqual(count, healthsub.ReplicaHealthEvents.size());
    }

    ReconfigurationStuckDescriptorSPtr CreateStuckHealthReportDescriptor()
    {
        return factory_.CreateReconfigurationStuckHealthReportDescriptor(
            ReconfigurationProgressStages::Phase1_WaitingForReadQuorum,
            holder_.ReplicaStoreObj.ConfigurationReplicas,
            reconfigurationStartTime_,
            reconfigurationPhaseStartTime_);
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    ReconfigurationHealthState rhs_;
    int64 localReplicaId_;
    int64 localReplicaInstanceId_;
    ReplicaStoreHolder holder_;
    ReconfigurationStuckHealthReportDescriptorFactory factory_;
    ScenarioTestHolder scenarioTest_;
    EntityEntryBaseSPtr id1_;
    FailoverUnitId failoverUnitId_;
    ServiceDescription serviceDescription_;

    Common::StopwatchTime reconfigurationStartTime_;
    Common::StopwatchTime reconfigurationPhaseStartTime_;
};

bool TestReconfigurationHealthState::TestSetup()
{
    id1_ = make_shared<LocalFailoverUnitMapEntry>(FailoverUnitId(), scenarioTest_.ScenarioTestObj.RA.LfumStore);
    return true;
}

bool TestReconfigurationHealthState::TestCleanup()
{
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestReconfigurationHealthStateSuite, TestReconfigurationHealthState)

BOOST_AUTO_TEST_CASE(ReconfigurationHealthStateQueueSanityTest)
{
    StateMachineActionQueue queue;

    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 2]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor();
    
    rhs_.OnReconfigurationPhaseHealthReport(queue, desc);

    VerifyQueueCount(1, queue);
        
    queue.ExecuteAllActions(L"activityId", id1_, scenarioTest_.ScenarioTestObj.RA);
    
    VerifyHealthReportCount(1);
        
    VerifyHealthReportType(0, Health::ReplicaHealthEvent::ReconfigurationStuckWarning);
}

BOOST_AUTO_TEST_CASE(ReconfigurationHealthStateStateChange)
{
    StateMachineActionQueue queue;

    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 2]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor();

    rhs_.OnReconfigurationPhaseHealthReport(queue, desc);

    VerifyQueueCount(1, queue);

    rhs_.OnPhaseChanged(queue, factory_.CreateReconfigurationStuckHealthReportDescriptor());

    VerifyQueueCount(2, queue);

    queue.ExecuteAllActions(L"activityId", id1_, scenarioTest_.ScenarioTestObj.RA);

    VerifyHealthReportCount(2);
    VerifyHealthReportType(0, Health::ReplicaHealthEvent::ReconfigurationStuckWarning);
    VerifyHealthReportType(1, Health::ReplicaHealthEvent::ClearReconfigurationStuckWarning);
}

BOOST_AUTO_TEST_CASE(ReconfigurationHealthStateSameDescriptor)
{
    StateMachineActionQueue queue;

    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 2]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor();

    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 2]");

    ReconfigurationStuckDescriptorSPtr desc2 = CreateStuckHealthReportDescriptor();

	Verify::IsTrue(*desc == *desc2, L"Report descriptors are not equal!");

    rhs_.OnReconfigurationPhaseHealthReport(queue, desc);

    rhs_.OnReconfigurationPhaseHealthReport(queue, desc2);

    VerifyQueueCount(1, queue);

    queue.ExecuteAllActions(L"activityId", id1_, scenarioTest_.ScenarioTestObj.RA);
    
    VerifyHealthReportCount(1);
    VerifyHealthReportType(0, Health::ReplicaHealthEvent::ReconfigurationStuckWarning);
}

BOOST_AUTO_TEST_CASE(ReconfigurationHealthStateSameDescriptorAfterReset)
{
    StateMachineActionQueue queue;

    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 2]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor();

	AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 2]");

    ReconfigurationStuckDescriptorSPtr desc2 = CreateStuckHealthReportDescriptor();
	
    Verify::IsTrue(*desc == *desc2, L"Report descriptors are not equal!");

    rhs_.OnReconfigurationPhaseHealthReport(queue, desc);

    rhs_.OnPhaseChanged(queue, factory_.CreateReconfigurationStuckHealthReportDescriptor());

    rhs_.OnReconfigurationPhaseHealthReport(queue, desc2);

    VerifyQueueCount(3, queue);

    queue.ExecuteAllActions(L"activityId", id1_, scenarioTest_.ScenarioTestObj.RA);

    VerifyHealthReportCount(3);
    VerifyHealthReportType(0, Health::ReplicaHealthEvent::ReconfigurationStuckWarning);
    VerifyHealthReportType(1, Health::ReplicaHealthEvent::ClearReconfigurationStuckWarning);
    VerifyHealthReportType(2, Health::ReplicaHealthEvent::ReconfigurationStuckWarning);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
