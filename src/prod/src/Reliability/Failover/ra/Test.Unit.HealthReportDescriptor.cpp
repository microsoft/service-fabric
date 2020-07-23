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

class TestHealthReportDescriptor : public TestBase
{
protected:
    TestHealthReportDescriptor() : hc_(L"TestNodeName"),
        reconfigurationStartTime_(Common::Stopwatch::Now()),
        reconfigurationPhaseStartTime_(Common::Stopwatch::Now()-Common::TimeSpan::FromMinutes(1))
    {
    }

    void PopulateHealthReportReplicaVector()
    {
        vector<HealthReportReplica> v;

        for (Replica & r : holder_.Replicas)
        {
            v.push_back(HealthReportReplica(r));
        }

        healthReportReplicas_ = v;        
    }

    void AddReplicas(wstring const & s)
    {
        vector<Replica> v;
        Reader r(s, ReadContext());
        bool check = r.ReadVector(L'[', L']', v);
        Verify::IsTrueLogOnError(check, L"Failed to read vector");

        holder_ = ReplicaStoreHolder(move(v));
        PopulateHealthReportReplicaVector();
    }
        
    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    ReplicaStoreHolder holder_;
    HealthContext hc_;
    vector<HealthReportReplica> healthReportReplicas_;

    Common::StopwatchTime reconfigurationStartTime_;
    Common::StopwatchTime reconfigurationPhaseStartTime_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestHealthReportDescriptorSuite, TestHealthReportDescriptor)

BOOST_AUTO_TEST_CASE(TransitionHealthReportDescriptorWarningErrorNoMessageTest_ApiFailure)
{
    ApiNameDescription apiNameDescription(InterfaceName::Enum::IStatefulServiceReplica, ApiName::Enum::ChangeRole, L"TestMetaData");
    auto proxyErrorCode = ProxyErrorCode::Create(ErrorCode(ErrorCodeValue::NotFound), move(apiNameDescription));
    TransitionHealthReportDescriptor apiFailureDescriptor(proxyErrorCode);

    Verify::AreEqual(L" TestNodeName. -2147017488", apiFailureDescriptor.GenerateReportDescriptionInternal(hc_));
}

BOOST_AUTO_TEST_CASE(TransitionHealthReportDescriptorWarningErrorTest_ApiFailure)
{
    ApiNameDescription apiNameDescription(InterfaceName::Enum::IStatefulServiceReplica, ApiName::Enum::ChangeRole, L"TestMetaData");
    auto proxyErrorCode = ProxyErrorCode::Create(ErrorCode(ErrorCodeValue::NotFound, L"TestErrorMessage"), move(apiNameDescription));
	TransitionHealthReportDescriptor apiFailureDescriptor(proxyErrorCode);

    Verify::AreEqual(L" TestNodeName. API call: IStatefulServiceReplica.ChangeRole(TestMetaData); Error = TestErrorMessage", apiFailureDescriptor.GenerateReportDescriptionInternal(hc_));
}

BOOST_AUTO_TEST_CASE(TransitionHealthReportDescriptorWarningErrorTest_AppHostCrash)
{
	ErrorCode errorCode(ErrorCodeValue::Enum::ApplicationHostCrash);
	TransitionHealthReportDescriptor descriptor(errorCode);

	Verify::AreEqual(L" TestNodeName. The application host has crashed.", descriptor.GenerateReportDescriptionInternal(hc_));
}

BOOST_AUTO_TEST_CASE(TransitionHealthReportDescriptorWarningErrorTest_FSR)
{
	ErrorCode errorCode(ErrorCodeValue::Enum::ServiceTypeNotFound);
	TransitionHealthReportDescriptor descriptor(errorCode);

	Verify::AreEqual(L" TestNodeName. Launching application host has failed.", descriptor.GenerateReportDescriptionInternal(hc_));
}

BOOST_AUTO_TEST_CASE(PartitionHealthEventDescriptorTest)
{
    PartitionHealthEventDescriptor partitionHealthEventDescriptor;
    Verify::AreEqual(L"TestNodeName.", partitionHealthEventDescriptor.GenerateReportDescriptionInternal(hc_));
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorGetLSNDataLossTest)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    ReconfigurationStuckHealthReportDescriptor descriptor(ReconfigurationProgressStages::Phase1_DataLoss, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);

    auto descriptorResult = descriptor.GenerateReportDescriptionInternal(hc_);

    Verify::StringContains(descriptorResult, L"Waiting for response from Failover Manager", L"Reason Text Difference");
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorWaitingForReadQuorumTest)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4] [S/S 5] [S/S 6]");

    ReconfigurationStuckHealthReportDescriptor descriptor(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);

    auto descriptorResult = descriptor.GenerateReportDescriptionInternal(hc_);

    Verify::StringContains(descriptorResult, L"Waiting for response from 6 replicas", L"Reason Text Difference");
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorNoReplyFromRapTest)
{
    AddReplicas(L"[S/S 1] [S/P 2] [P/I 3] [I/S 4] [S/S 2]");

    ReconfigurationStuckHealthReportDescriptor descriptor(ReconfigurationProgressStages::Phase2_NoReplyFromRap, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);

    auto descriptorResult = descriptor.GenerateReportDescriptionInternal(hc_);

    Verify::StringContains(descriptorResult, L"Waiting for response from the local replica", L"Reason Text Difference");
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorReplicaListTest)
{
    AddReplicas(L"[P/S 1] [I/P 2]");

    ReconfigurationStuckHealthReportDescriptor descriptor(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);

    auto descriptorResult = descriptor.GenerateReportDescriptionInternal(hc_);

    Verify::StringContains(descriptorResult, L"Waiting for response from 2 replicas", L"Reason Text Difference");
    Verify::StringContains(descriptorResult, L"Pending Replicas:", L"Pending Replicas Header");
    Verify::StringContains(descriptorResult, L"P/S InBuild 1 1", L"Replica 1");
    Verify::StringContains(descriptorResult, L"I/P InBuild 2 2", L"Replica 2");
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckContainsTimeElapsed)
{
    AddReplicas(L"[P/S 1] [I/P 2]");

    ReconfigurationStuckHealthReportDescriptor descriptor(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);

    auto descriptorResult = descriptor.GenerateReportDescriptionInternal(hc_);

    wstring expectedPhaseTotal = wformatString("Reconfiguration phase start time: {0}", reconfigurationPhaseStartTime_);
    wstring expectedTotal = wformatString("Reconfiguration start time: {0}", reconfigurationStartTime_);

    //Removing the seconds and milliseconds
    //This is because when a StopwatchTime is converted to string, it might have an error of ~1 millisecond
    //This error makes it unsafe to use the millisecond and potentially the seconds on some edge cases on this test
    //Removing them reduces drastically the chance of having a false negative. 
    expectedPhaseTotal = expectedPhaseTotal.substr(0, expectedPhaseTotal.size() - 6);
    expectedTotal = expectedTotal.substr(0, expectedPhaseTotal.size() - 6);

    Verify::StringContains(descriptorResult, expectedPhaseTotal, L"Total Time Elapsed");
    Verify::StringContains(descriptorResult, expectedTotal, L"Phase Time Elapsed");
}

BOOST_AUTO_TEST_CASE(ReconfigurationStuckHealthReportDescriptorEqualityTest)
{
    AddReplicas(L"[P/S 1] [I/P 2]");
    ReconfigurationStuckHealthReportDescriptor baseDescriptor (ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);

    AddReplicas(L"[P/S 1] [I/P 2]");
    ReconfigurationStuckHealthReportDescriptor identicalDescriptor (ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);
    Verify::IsTrue(baseDescriptor == identicalDescriptor, L"Difference in identical descriptors");

    AddReplicas(L"[P/S 1] [I/P 2] [S/S 3]");
    ReconfigurationStuckHealthReportDescriptor replicaNumbers (ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);
    Verify::IsTrue(baseDescriptor != replicaNumbers, L"Difference not found in different replica vector sizes");

    AddReplicas(L"[P/S 1] [S/P 2]");
    ReconfigurationStuckHealthReportDescriptor differentReplica (ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);
    Verify::IsTrue(baseDescriptor != differentReplica, L"Difference not found in different replicas");

    AddReplicas(L"[P/S 1] [I/P 2]");
    ReconfigurationStuckHealthReportDescriptor differentReason (ReconfigurationProgressStages::Phase1_DataLoss, move(healthReportReplicas_), reconfigurationStartTime_, reconfigurationPhaseStartTime_);
    Verify::IsTrue(baseDescriptor != differentReason, L"Difference not found in different reconfiguration stuck reason");

    AddReplicas(L"[P/S 1] [I/P 2]");
    ReconfigurationStuckHealthReportDescriptor differentReconfigurationStartTime(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), Stopwatch::Now(), reconfigurationPhaseStartTime_);
    Verify::IsTrue(baseDescriptor != differentReconfigurationStartTime, L"Difference not found in different reconfiguration start time");

    AddReplicas(L"[P/S 1] [I/P 2]");
    ReconfigurationStuckHealthReportDescriptor differentReconfigurationPhaseStartTime(ReconfigurationProgressStages::Phase1_WaitingForReadQuorum, move(healthReportReplicas_), reconfigurationStartTime_, Stopwatch::Now());
    Verify::IsTrue(baseDescriptor != differentReconfigurationPhaseStartTime, L"Difference not found in different reconfiguration phase start time");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
