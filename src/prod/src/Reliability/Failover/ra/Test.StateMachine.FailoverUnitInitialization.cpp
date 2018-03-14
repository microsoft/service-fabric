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
    wstring const Persisted = L"SP1";
    wstring const Volatile = L"SV1";
    wstring const Stateless = L"SL1";
    wstring const FM = L"FM";
}

class TestStateMachine_FailoverUnitInitialization : public StateMachineTestBase
{
protected:
    FailoverUnit & Run(wstring const & shortName, wstring const & initial)
    {
        Test.AddFT(shortName, initial);

        Test.RestartRA();

        return Test.GetFT(shortName);
    }

    void RunAndVerify(wstring const & shortName, wstring const & initial, wstring const & expected)
    {
        Run(shortName, initial);

        Test.ValidateFT(shortName, expected);
    }

    void ServiceDescriptionIsFixedForBackwardCompatibilityTest(
        function<void(FailoverUnit&)> modifier)
    {
        wstring const shortName = L"SP1";
        wstring const initial = L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]";

        auto & test = Test;

        test.AddFT(shortName, initial);

        auto * ft = &test.GetFT(shortName);
        modifier(*ft);

        auto nodeInstanceCopy = Default::GetInstance().NodeInstance;
        nodeInstanceCopy.IncrementInstanceId();

        test.ExecuteUpdateStateOnLfumLoad(shortName, nodeInstanceCopy);

        ft = &test.GetFT(shortName);
        if (ft->IsOpen)
        {
            Verify::AreEqualLogOnError(ft->ServiceDescription.PackageVersionInstance, ft->LocalReplica.PackageVersionInstance, L"PackageVersionInstance");
        }
    }

    ServiceModel::ServicePackageVersionInstance MakeHigher(ServiceModel::ServicePackageVersionInstance const & input)
    {
        return ServiceModel::ServicePackageVersionInstance(input.Version, input.InstanceId + 1);
    }

    void VerifyFMMessageRetrySet(wstring const& shortName, FailoverManagerId const& expectedFMId)
    {        
        auto actual = Test.GetFT(shortName).FMMessageStateObj.FMMessageRetryPending.Id;
        auto expected = EntitySetIdentifier(EntitySetName::FailoverManagerMessageRetry, expectedFMId);
        Verify::AreEqual(expected, actual, wformatString("FM Set id for {0}. {1}. {2}", shortName, expected, actual));
    }

    void VerifyReplicaUploadPendingSet(wstring const& shortName, FailoverManagerId const& expectedFMId)
    {
        auto actual = Test.GetFT(shortName).ReplicaUploadStateObj.ReplicaUploadPending.Id;
        auto expected = EntitySetIdentifier(EntitySetName::ReplicaUploadPending, expectedFMId);
        Verify::AreEqual(expected, actual, wformatString("FM Set id for {0}. {1}. {2}", shortName, expected, actual));
    }

    void RunAndVerifySet(wstring const & shortName, FailoverManagerId const & expectedFMId)
    {
        Recreate();

        Run(shortName, L"C None 000/000/411 1:1 -");

        VerifyFMMessageRetrySet(shortName, expectedFMId);
        VerifyReplicaUploadPendingSet(shortName, expectedFMId);
    }

    void RunInsertNewFTAndVerifySet(wstring const & shortName, FailoverManagerId const & expectedFMId)
    {
        Recreate();

        if (shortName == Stateless)
        {
            Test.ProcessFMMessageAndDrain<MessageType::AddInstance>(shortName, L"000/411 [N/N IB U 1:1]");
        }
        else
        {
            Test.ProcessFMMessageAndDrain<MessageType::AddPrimary>(shortName, L"411/422 [N/P IB U 1:1]");
        }

        VerifyFMMessageRetrySet(shortName, expectedFMId);
        VerifyReplicaUploadPendingSet(shortName, expectedFMId);
    }
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_FailoverUnitInitializationSuite, TestStateMachine_FailoverUnitInitialization)

BOOST_AUTO_TEST_CASE(SanityTest)
{
    RunAndVerify(
        Persisted, 
        L"O Phase2_Catchup 022/033/044 1:1 BJ [N/N/P RD U N F 1:1]",
        L"O None 022/033/044 1:1 1 - [N/N/P SB D N F 1:2:1:1]");
}

BOOST_AUTO_TEST_CASE(LocalReplicaDeletedIsPreserved)
{
    auto & ft = Run(Persisted, L"O Phase2_Catchup 022/033/044 1:1 BJHlL [N/N/P ID U N F 1:1]");
    Verify::IsTrue(ft.LocalReplicaDeleted, L"LocalReplicaDeleted");
}

BOOST_AUTO_TEST_CASE(IDStaysID)
{
    auto & ft = Run(Persisted, L"O Phase2_Catchup 022/033/044 1:1 BJ [N/N/P ID U N F 1:1]");
    Verify::AreEqual(ReconfigurationAgentComponent::ReplicaStates::InDrop, ft.LocalReplica.State, L"State");
}

BOOST_AUTO_TEST_CASE(RDGoesToSB)
{
    auto & ft = Run(Persisted, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    Verify::AreEqual(ReconfigurationAgentComponent::ReplicaStates::StandBy, ft.LocalReplica.State, L"State");
}

BOOST_AUTO_TEST_CASE(NodeInstanceIsUpdated)
{
    auto & ft = Run(Persisted, L"O Phase2_Catchup 022/033/044 1:1 BJ [N/N/P ID U N F 1:1]");
    Verify::AreEqual(2, ft.LocalReplica.FederationNodeInstance.InstanceId, L"Node Instance");
}

BOOST_AUTO_TEST_CASE(DownReplicaInstanceIdIsSet)
{
    auto & ft = Run(Persisted, L"O None 000/000/411 1:1 - [N/N/P RD U N F 1:1]");
    Verify::AreEqual(1, ft.FMMessageStateObj.DownReplicaInstanceId, L"DownReplicaInstance");
}

BOOST_AUTO_TEST_CASE(StatelessIsClosed)
{
    RunAndVerify(
        Stateless, 
        L"O None 000/000/411 1:1 - [N/N/N RD U N F 1:1]",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(VolatileIsClosed)
{
    RunAndVerify(
        Volatile,
        L"O None 000/000/411 1:1 - [N/N/P RD U N F 1:1]",
        L"C None 000/000/411 1:1 -");
}

BOOST_AUTO_TEST_CASE(VolatileIsDeleted)
{
    RunAndVerify(
        Volatile,
        L"O None 000/000/411 1:1 CMHlL [N/N/P ID U N F 1:1]",
        L"C None 000/000/411 1:1 LP");
}

BOOST_AUTO_TEST_CASE(SetIdentifierVerifier_LfumLoad)
{
    RunAndVerifySet(Stateless, *FailoverManagerId::Fm);
    RunAndVerifySet(Volatile, *FailoverManagerId::Fm);
    RunAndVerifySet(Persisted, *FailoverManagerId::Fm);
    RunAndVerifySet(FM, *FailoverManagerId::Fmm);
}

BOOST_AUTO_TEST_CASE(SetIdentifierVerifier_Insert)
{
    RunInsertNewFTAndVerifySet(Stateless, *FailoverManagerId::Fm);
    RunInsertNewFTAndVerifySet(Volatile, *FailoverManagerId::Fm);
    RunInsertNewFTAndVerifySet(Persisted, *FailoverManagerId::Fm);
    RunInsertNewFTAndVerifySet(FM, *FailoverManagerId::Fmm);
}

BOOST_AUTO_TEST_CASE(ServiceDesc_BackwardCompatibility_SP_SDIsLower)
{
    ServiceDescriptionIsFixedForBackwardCompatibilityTest(
        [&](FailoverUnit & ft)
    {
        ft.LocalReplica.PackageVersionInstance = MakeHigher(ft.LocalReplica.PackageVersionInstance);
    });
}

BOOST_AUTO_TEST_CASE(ServiceDesc_BackwardCompatibility_SP_VersionIsLower)
{
    ServiceDescriptionIsFixedForBackwardCompatibilityTest(
        [&](FailoverUnit & ft)
    {
        ft.Test_GetServiceDescription().PackageVersionInstance = MakeHigher(ft.LocalReplica.PackageVersionInstance);
    });
}

BOOST_AUTO_TEST_CASE(ServiceDesc_BackwardCompatibility_SP_BothAreEqual)
{
    ServiceDescriptionIsFixedForBackwardCompatibilityTest([&](FailoverUnit &){});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
