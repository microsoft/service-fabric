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


class TestReconfigurationStuckHealthReportDescriptorFactory
{
protected:

    void AddReplicas(wstring const & s)
    {
        vector<Replica> v;
        Reader r(s, ReadContext());
        bool check = r.ReadVector(L'[', L']', v);
        Verify::IsTrueLogOnError(check, L"Failed to read vector");

        holder_ = ReplicaStoreHolder(move(v));
    }

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

    ~TestReconfigurationStuckHealthReportDescriptorFactory()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    ReconfigurationStuckDescriptorSPtr CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Enum stage);

    ReplicaStoreHolder holder_;
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

ReconfigurationStuckDescriptorSPtr TestReconfigurationStuckHealthReportDescriptorFactory::CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Enum stage)
{
    return factory_.CreateReconfigurationStuckHealthReportDescriptor(
        stage,
        holder_.ReplicaStoreObj.ConfigurationReplicas,
        reconfigurationStartTime_,
        reconfigurationPhaseStartTime_);
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestReconfigurationStuckHealthReportDescriptorFactorySuite, TestReconfigurationStuckHealthReportDescriptorFactory)

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryGetLSNWaitingForReadQuorum)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 2]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);
       
    VerifyDescriptorReasonAndReplicaSetSize(desc, 5, ReconfigurationProgressStages::Phase1_WaitingForReadQuorum);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryDataLoss)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase1_DataLoss);
    
    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase1_DataLoss);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryPhaseZeroNoReplyFromRAP)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase0_NoReplyFromRAP);
    
    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase0_NoReplyFromRAP);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryNoReplyFromRap)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase2_NoReplyFromRap);
   
    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase2_NoReplyFromRap);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryPCBelowReadQuorum)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase3_PCBelowReadQuorum);
     
    VerifyDescriptorReasonAndReplicaSetSize(desc, 4, ReconfigurationProgressStages::Phase3_PCBelowReadQuorum);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryWaitingForReplicas)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase3_WaitingForReplicas);
    
    VerifyDescriptorReasonAndReplicaSetSize(desc, 4, ReconfigurationProgressStages::Phase3_WaitingForReplicas);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryActivateNotCompleted)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_UpReadyReplicasPending);
    
    VerifyDescriptorReasonAndReplicaSetSize(desc, 3, ReconfigurationProgressStages::Phase4_UpReadyReplicasPending);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryLocalReplicaNotReplied)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_LocalReplicaNotReplied);
     
    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase4_LocalReplicaNotReplied);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryReplicaPendingRestart)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    bool alternate = true;
    for (auto & replica : holder_.ReplicaStoreObj.ConfigurationReplicas)
    {
        if (alternate)
        {
            replica.ToBeRestarted = true;
        }
        alternate = !alternate;
    }

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_ReplicaPendingRestart);
      
    VerifyDescriptorReasonAndReplicaSetSize(desc, 2, ReconfigurationProgressStages::Phase4_ReplicaPendingRestart);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryReplicaStuckIB)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 3]");

    bool alternate = true;
    for (auto & replica : holder_.ReplicaStoreObj.ConfigurationReplicas)
    {
        if (alternate)
        {
            replica.State = ReconfigurationAgentComponent::ReplicaStates::InBuild;
        }
        alternate = !alternate;
    }

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_ReplicaStuckIB);

    VerifyDescriptorReasonAndReplicaSetSize(desc, 3, ReconfigurationProgressStages::Phase4_ReplicaStuckIB);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryUpdateReplicatorConfigTrue)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_ReplicatorConfigurationUpdatePending);

    VerifyDescriptorReasonAndReplicaSetSize(desc, 0, ReconfigurationProgressStages::Phase4_ReplicatorConfigurationUpdatePending);
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorFactoryUpReadyReplicasActivated)
{
    AddReplicas(L"[S/P 1] [S/S 2]");

    ReconfigurationStuckDescriptorSPtr desc = CreateStuckHealthReportDescriptor(ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated);

    VerifyDescriptorReasonAndReplicaSetSize(desc, 2, ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated);
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
