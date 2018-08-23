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


class TestReconfigurationStuckHealthReportDescriptorFactory : public StateMachineTestBase
{
protected:

    void VerifyDescriptorReasonAndReplicaSetSize(ReconfigurationStuckDescriptorSPtr const desc, int size, ReconfigurationProgressStages::Enum reason)
    {
        Verify::AreEqual(size, desc->Replicas.size());
        Verify::AreEqual(reason, desc->ReconfigurationStuckReason);
    }

    TestReconfigurationStuckHealthReportDescriptorFactory() :
        reconfigurationStartTime_(Common::Stopwatch::Now()),
        reconfigurationPhaseStartTime_(Common::Stopwatch::Now() - Common::TimeSpan::FromMinutes(1))
    {
        BOOST_REQUIRE(TestSetup());
    }

    void VerifyReplicaExists(ReconfigurationStuckDescriptorSPtr const desc, int64 replicaId)
    {
        bool found = false;
        for (HealthReportReplica const & r : desc->Replicas)
        {
            if (r.Test_GetReplicaID() == replicaId)
            {
                found = true;
            }
        }
        Verify::IsTrue(found, wformatString(L"ReplicaID {0} not found on HealthReportReplica vector!", replicaId));
    }

    ~TestReconfigurationStuckHealthReportDescriptorFactory()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    ReconfigurationStuckDescriptorSPtr CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Enum stage, wstring ftId);

    ReconfigurationStuckHealthReportDescriptorFactory factory_;

    Common::StopwatchTime reconfigurationStartTime_;
    Common::StopwatchTime reconfigurationPhaseStartTime_;
};

bool TestReconfigurationStuckHealthReportDescriptorFactory::TestSetup()
{
    return true;
}

bool TestReconfigurationStuckHealthReportDescriptorFactory::TestCleanup()
{
    return true;
}

ReconfigurationStuckDescriptorSPtr TestReconfigurationStuckHealthReportDescriptorFactory::CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Enum stage, wstring ftId)
{
    return factory_.CreateReconfigurationStuckHealthReportDescriptor(
        stage,
        Test.GetFT(ftId).Test_GetReplicaStore().ConfigurationReplicas,
        reconfigurationStartTime_,
        reconfigurationPhaseStartTime_);
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestReconfigurationStuckHealthReportDescriptorFactorySuite, TestReconfigurationStuckHealthReportDescriptorFactory)

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryGetLSNWaitingForReadQuorum)
{
    //Replica 2 has an LSN set, so it won't show up on the health report
    //Replica 4 is dropped so it won't be in the health report
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1 001 002] [P/I/I RD U N F 3:1] [I/S/S DD D N F 4:1] [S/S/S RD U N F 5:1]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, L"SP1");
       
    VerifyDescriptorReasonAndReplicaSetSize(desc, 3, ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);
    VerifyReplicaExists(desc, 1);
    VerifyReplicaExists(desc, 3);
    VerifyReplicaExists(desc, 5);


}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryDataLoss)
{
    Test.AddFT(L"SP1", L"O Phase1_GetLSN 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1 001 002] [P/I/I RD U N F 3:1] [I/S/S DD D N F 4:1]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase1_DataLoss, L"SP1");
    
    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase1_DataLoss);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryPhaseZeroNoReplyFromRAP)
{
    Test.AddFT(L"SP1", L"O Phase0_Demote 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1 001 002] [P/I/I RD U N F 3:1] [I/S/S DD D N F 4:1]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase0_NoReplyFromRAP, L"SP1");
    
    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase0_NoReplyFromRAP);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryNoReplyFromRap)
{
    Test.AddFT(L"SP1", L"O Phase2_Catchup 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1 001 002] [P/I/I RD U N F 3:1] [I/S/S DD D N F 4:1]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase2_NoReplyFromRap, L"SP1");
   
    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase2_NoReplyFromRap);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryPCBelowReadQuorum)
{
    //Replica 1 is not waiting for RA reply. Replica 3 not on the PC. Replica 4 is dropped
    Test.AddFT(L"SP1", L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U RA F 2:1] [I/S/S RD U RA F 3:1] [P/S/S DD D RA F 4:1]");
    
    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase3_PCBelowReadQuorum, L"SP1");
     
    VerifyDescriptorReasonAndReplicaSetSize(desc, 1, ReconfigurationProgressStages::Phase3_PCBelowReadQuorum);
    VerifyReplicaExists(desc, 2);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryWaitingForReplicas)
{
    //Replica 2 is ToBeRestarted. Replica 3 is Down. Replica 4 is IB.
    Test.AddFT(L"SP1", L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N B 2:1] [I/S/S SB D N F 3:1] [P/S/S IB U N F 4:1]");
    
    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase3_WaitingForReplicas, L"SP1");

    // After this setup only replica [S/P 1] should be in the health report
    VerifyDescriptorReasonAndReplicaSetSize(desc, 1, ReconfigurationProgressStages::Phase3_WaitingForReplicas);
    VerifyReplicaExists(desc, 1);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryUpReadyReplicasPending)
{
    //Replica 2 is ReplicatorRemovePending. Replica 3 is InBuild. Replica 4 is RestartPending. Replica 5 is Down. Replica 6 is StandBy. Replica 7 has messageStage = None. Replica 8 is not on the cc.
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U RA F 1:1] [S/S/S IB D RA R 2:1] [P/S/S IB U RA F 3:1] [I/S/S RD U RA B 4:1] [S/S/S SB D RA F 5:1] [S/S/S SB U RA F 6:1] [S/S/S RD U N F 7:1] [S/I/I RD U RA F 8:1]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_UpReadyReplicasPending, L"SP1");
    
    VerifyDescriptorReasonAndReplicaSetSize(desc, 1, ReconfigurationProgressStages::Phase4_UpReadyReplicasPending);
    VerifyReplicaExists(desc, 1);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryLocalReplicaNotReplied)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U RAP F 1:1] [S/S/S IB D RA R 2:1] [P/S/S IB U RA F 3:1] [I/S/S RD U RA B 4:1] [S/S/S SB D RA F 5:1] [S/S/S IB U RA F 6:1] [S/S/S RD U RA F 7:1]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_LocalReplicaNotReplied, L"SP1");
     
    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase4_LocalReplicaNotReplied);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryReplicaPendingRestart)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U RA F 1:1] [S/S/S IB D RA R 2:1] [P/S/S IB U RA F 3:1] [I/S/S RD U RA B 4:1] [S/S/S SB D RA F 5:1] [S/S/S RD U RA B 6:1] [S/S/S RD U RA F 7:1]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_ReplicaPendingRestart, L"SP1");
      
    VerifyDescriptorReasonAndReplicaSetSize(desc, 2, ReconfigurationProgressStages::Phase4_ReplicaPendingRestart);
    VerifyReplicaExists(desc, 4);
    VerifyReplicaExists(desc, 6);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryReplicaStuckIB)
{
    //Replica 2 is ReplicatorRemovePending. Replica 3 is not on the CC. Replica 4 is ToBeRestarted. Replica 5 is down. Replica 1 is Ready. Replica 7 is messagestage none.
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U RA F 1:1] [S/S/S IB D RA R 2:1] [P/I/I IB U RA F 3:1] [I/S/S RD U RA B 4:1] [S/S/S IB D RA F 5:1] [S/S/S IB U RA F 6:1] [S/S/S IB U N F 7:1]");
    
    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_ReplicaStuckIB, L"SP1");

    VerifyDescriptorReasonAndReplicaSetSize(desc, 1, ReconfigurationProgressStages::Phase4_ReplicaStuckIB);
    VerifyReplicaExists(desc, 6);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryUpdateReplicatorConfigTrue)
{
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P IB U RA F 1:1] [S/S/S IB D RA R 2:1] [P/I/I IB U RA F 3:1] [I/S/S RD U RA B 4:1] [S/S/S IB D RA F 5:1] [S/S/S RD U RA F 6:1] [S/S/S IB U N F 7:1]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_ReplicatorConfigurationUpdatePending, L"SP1");

    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase4_ReplicatorConfigurationUpdatePending);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryUpReadyReplicasActivated)
{
    //Replica 3 is not on the CC. Replica 4 is Ready and Up. Replica 5 is Down. Replica 6 is Ready and Up. Replica is not pending message from the RA.
    Test.AddFT(L"SP1", L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P SB U RA F 1:1] [S/S/S IB D N R 2:1] [P/I/I SB U RA F 3:1] [I/S/S RD U RA B 4:1] [S/S/S SB D RA F 5:1] [S/S/S RD U RA F 6:1] [S/S/S SB U N F 7:1]");
    
    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated, L"SP1");

    VerifyDescriptorReasonAndReplicaSetSize(desc, 2, ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated);
    VerifyReplicaExists(desc, 1);
    VerifyReplicaExists(desc, 2);
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
