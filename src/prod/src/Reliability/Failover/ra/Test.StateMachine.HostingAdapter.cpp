// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestHostingAdapter : public StateMachineTestBase
{
protected:

    void TestRunner(std::wstring const & shortName)
    {
        auto & test = GetScenarioTest();

        test.AddFT(shortName, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
        auto & ft = test.GetFT(shortName);

        auto & hosting = test.RA.HostingAdapterObj;
        hosting.TerminateServiceHost(L"a", Hosting::TerminateServiceHostReason::FabricUpgrade, ft.ServiceTypeRegistration);
    }

    void ServiceTypeRegisteredIsIgnoredForFTTestHelper(wstring const & tag, wstring const & shortName, wstring const & initial, wstring const & serviceType)
    {
        Recreate();

        Test.LogStep(tag);

        Test.AddFT(shortName, initial);

        Test.ProcessServiceTypeRegisteredAndDrain(serviceType);

        Test.ValidateFT(shortName, initial);
    }

    void EventIsProcessedTestHelper(
        wstring const & tag,
        wstring const & initial,
        RALifeCycleState::Enum lifeCycleState,
        bool isNodeUpAckFromFMReceived,
        bool isNodeUpAckFromFmmReceived,
        bool isEventProcessed,
        function<void()> runner,
        function<bool(FailoverUnit const &)> verifier)
    {
        Recreate();

        Test.LogStep(tag);
        Test.SetNodeUpAckProcessed(isNodeUpAckFromFMReceived, isNodeUpAckFromFmmReceived);
        Test.SetLifeCycleState(lifeCycleState);

        Test.AddFT(L"SP1", initial);

        runner();

        Test.DumpFT(L"SP1");
        
        auto actual = verifier(Test.GetFT(L"SP1"));
        Verify::AreEqual(isEventProcessed, actual, L"IsEventProcessed");
    }

    void ServiceTypeRegistrationHelper(
        wstring const & tag, 
        RALifeCycleState::Enum lifeCycleState,
        bool isNodeUpAckFromFMReceived, 
        bool isNodeUpAckFromFmmReceived, 
        bool isEventProcessed)
    {
        EventIsProcessedTestHelper(
            tag,
            L"O None 000/000/411 1:1 1 - [N/N/P SB D N F 1:1]",
            lifeCycleState,
            isNodeUpAckFromFMReceived,
            isNodeUpAckFromFmmReceived,
            isEventProcessed,
            [this]() {Test.ProcessServiceTypeRegisteredAndDrain(L"SP1"); },
            [](FailoverUnit const & ft) { return ft.LocalReplica.IsUp; });
    }

    void RuntimeHelper(
        wstring const & tag,
        RALifeCycleState::Enum lifeCycleState,
        bool isNodeUpAckFromFMReceived,
        bool isNodeUpAckFromFmmReceived,
        bool isEventProcessed)
    {
        EventIsProcessedTestHelper(
            tag,
            L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
            lifeCycleState,
            isNodeUpAckFromFMReceived,
            isNodeUpAckFromFmmReceived,
            isEventProcessed,
            [this]() {Test.ProcessRuntimeClosedAndDrain(L"SP1"); },
            [](FailoverUnit const & ft) { return ft.LocalReplicaClosePending.IsSet; });
    }

    void AppHostHelper(
        wstring const & tag,
        RALifeCycleState::Enum lifeCycleState,
        bool isNodeUpAckFromFMReceived,
        bool isNodeUpAckFromFmmReceived,
        bool isEventProcessed)
    {
        EventIsProcessedTestHelper(
            tag,
            L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
            lifeCycleState,
            isNodeUpAckFromFMReceived,
            isNodeUpAckFromFmmReceived,
            isEventProcessed,
            [this]() {Test.ProcessAppHostClosedAndDrain(L"SP1"); },
            [](FailoverUnit const & ft) { return !ft.LocalReplica.IsUp; });
    }    

    void CommitFailureTestHelper(bool isRuntimeDown, int expectedCount)
    {
        Recreate();

        Test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

        Test.EnableLfumStoreFaultInjection();

        if (isRuntimeDown)
        {
            Test.ProcessRuntimeClosedAndDrain(L"SP1");
        }
        else
        {
            Test.ProcessAppHostClosedAndDrain(L"SP1");
        }

        Test.UTContext.ProcessTerminationServiceStubObj.Calls.VerifyCallCount(expectedCount);
    }

    void SetupExclusiveServiceType(ScenarioTest & test)
    {
        ServiceTypeReadContext iso = Default::GetInstance().SL1_SP1_STContext;
        iso.SD.PackageActivationMode = ServiceModel::ServicePackageActivationMode::ExclusiveProcess;

        SingleFTReadContext sl1Iso = Default::GetInstance().SL1_FTContext;
        sl1Iso.FUID = FailoverUnitId(Guid(L"ABABABAB-ABAB-ABAB-ABAB-ABABABABABAB"));
        sl1Iso.ShortName = L"SL1Iso";
        sl1Iso.STInfo = iso;
        test.AddFTContext(L"SL1Iso", sl1Iso);

        sl1Iso.FUID = FailoverUnitId(Guid(L"ACACACAC-ACAC-ACAC-ACAC-ACACACACACAC"));
        sl1Iso.ShortName = L"SL1Iso2";
        sl1Iso.STInfo = iso;
        test.AddFTContext(L"SL1Iso2", sl1Iso);

        SingleFTReadContext sl1a = Default::GetInstance().SL1_FTContext;
        sl1a.FUID = FailoverUnitId(Guid(L"ADADADAD-ADAD-ADAD-ADAD-ADADADADADAD"));
        test.AddFTContext(L"SL1a", sl1a);
    }

    void VerifyServiceTypeRegistered(ScenarioTest & test, initializer_list<wstring> registeredFTs, initializer_list<wstring> pendingFTs)
    {
        for (auto const & it : registeredFTs)
        {
            auto & ft = test.GetFT(it);
            Verify::IsTrue(ft.ServiceTypeRegistration != nullptr, wformatString("{0} should have str", it));
        }

        for (auto const & it : pendingFTs)
        {
            auto & ft = test.GetFT(it);
            Verify::IsTrue(ft.ServiceTypeRegistration == nullptr, wformatString("{0} should have not str", it));
        }
    }

    void SetupForServiceTypeRegisteredTest(ScenarioTest & test)
    {
        SetupExclusiveServiceType(test);

        test.SetFindServiceTypeRegistrationState(L"SL1Iso", FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);
        test.SetFindServiceTypeRegistrationState(L"SL1Iso2", FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);
        test.SetFindServiceTypeRegistrationState(L"SL1", FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);
        test.SetFindServiceTypeRegistrationState(L"SL1a", FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);

        test.ProcessFMMessageAndDrain<MessageType::AddInstance>(L"SL1Iso", L"000/411 [N/N IB U 1:1]");
        test.ProcessFMMessageAndDrain<MessageType::AddInstance>(L"SL1Iso2", L"000/411 [N/N IB U 1:1]");
        test.ProcessFMMessageAndDrain<MessageType::AddInstance>(L"SL1", L"000/411 [N/N IB U 1:1]");
        test.ProcessFMMessageAndDrain<MessageType::AddInstance>(L"SL1a", L"000/411 [N/N IB U 1:1]");

        test.ResetAll();
    }

    ScenarioTest & GetScenarioTest() { return Test; }
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestHostingAdapterSuite,TestHostingAdapter)

BOOST_AUTO_TEST_CASE(TerminateActsAsPassThroughTest)
{
    TestRunner(L"SP1");
    GetScenarioTest().ValidateTerminateCall(L"SP1");
}

BOOST_AUTO_TEST_CASE(ActivationContextIsPassedInDuringFind)
{
    ScenarioTestHolder holder;
    auto & test = Test;

    SetupExclusiveServiceType(test);

    test.ProcessFMMessageAndDrain<MessageType::AddInstance>(L"SL1Iso", L"000/411 [N/N IB U 1:1]");

    auto actual = test.HostingHelper.HostingObj.FindServiceTypeApi.GetParametersAt(0).ActivationContext;
    Verify::IsTrue(actual.IsExclusive, L"Isolation context must be exclusive here");

    auto & ft = test.GetFT(L"SL1Iso");
    Verify::AreEqual(ft.FailoverUnitId.Guid, actual.ActivationGuid, L"ActivationGuid");
}

BOOST_AUTO_TEST_CASE(ServiceTypeRegistrationIfExclusiveFTNotPresent)
{
    ScenarioTestHolder holder;
    auto & test = Test;

    SetupExclusiveServiceType(test);

    test.ProcessServiceTypeRegisteredAndDrain(L"SL1Iso");
}

BOOST_AUTO_TEST_CASE(ServiceTypeRegistrationWorksIsAddedForFTInPresenceOfExclusiveMode)
{
    ScenarioTestHolder holder;
    auto & test = Test;

    SetupForServiceTypeRegisteredTest(test);

    test.ProcessServiceTypeRegisteredAndDrain(L"SL1Iso");

    VerifyServiceTypeRegistered(test, { L"SL1Iso" }, { L"SL1Iso2", L"SL1", L"SL1a" });
}

BOOST_AUTO_TEST_CASE(ServiceTypeRegistrationWorksIsAddedForSharedInPresenceOfExclusiveMode)
{
    ScenarioTestHolder holder;
    auto & test = Test;

    SetupForServiceTypeRegisteredTest(test);

    test.ProcessServiceTypeRegisteredAndDrain(L"SL1");

    VerifyServiceTypeRegistered(test, { L"SL1", L"SL1a" }, { L"SL1Iso", L"SL1Iso2" });
}

BOOST_AUTO_TEST_CASE(ServiceTypeIgnoreFTTest)
{
    // FT is closed
    ServiceTypeRegisteredIsIgnoredForFTTestHelper(L"FT is closed", L"SP1", L"C None 000/000/411 1:1 -", L"SP1");
    ServiceTypeRegisteredIsIgnoredForFTTestHelper(L"Volatile FT", L"SL1", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]", L"SV1");
    ServiceTypeRegisteredIsIgnoredForFTTestHelper(L"Stateless FT", L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]", L"SL1");
    ServiceTypeRegisteredIsIgnoredForFTTestHelper(L"FT in another package", L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]", L"SP2");

    ServiceTypeRegisteredIsIgnoredForFTTestHelper(L"SP but Up", L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]", L"SP1");
}

BOOST_AUTO_TEST_CASE(AppHostDownStalenessChecks)
{
    AppHostHelper(L"Open FMAndNotFMM", RALifeCycleState::Open, true, false, true);
    AppHostHelper(L"Open FMAndFMM", RALifeCycleState::Open, true, true, true);
    AppHostHelper(L"Open NotFMAndNotFMM", RALifeCycleState::Open, false, false, false);
    AppHostHelper(L"Open NotFMAndFMM", RALifeCycleState::Open, false, true, false);

    AppHostHelper(L"CLosing FMAndNotFMM", RALifeCycleState::Closing, true, false, true);
    AppHostHelper(L"CLosing FMAndFMM", RALifeCycleState::Closing, true, true, true);
    AppHostHelper(L"CLosing NotFMAndNotFMM", RALifeCycleState::Closing, false, false, false);
    AppHostHelper(L"CLosing NotFMAndFMM", RALifeCycleState::Closing, false, true, false);

    AppHostHelper(L"Closed NotFMAndNotFMM", RALifeCycleState::Closed, false, false, false);
}
BOOST_AUTO_TEST_CASE(RuntimeDownStalenessChecks)
{
    RuntimeHelper(L"Open FMAndNotFMM", RALifeCycleState::Open, true, false, true);
    RuntimeHelper(L"Open FMAndFMM", RALifeCycleState::Open, true, true, true);
    RuntimeHelper(L"Open NotFMAndNotFMM", RALifeCycleState::Open, false, false, false);
    RuntimeHelper(L"Open NotFMAndFMM", RALifeCycleState::Open, false, true, false);

    RuntimeHelper(L"CLosing FMAndNotFMM", RALifeCycleState::Closing, true, false, true);
    RuntimeHelper(L"CLosing FMAndFMM", RALifeCycleState::Closing, true, true, true);
    RuntimeHelper(L"CLosing NotFMAndNotFMM", RALifeCycleState::Closing, false, false, false);
    RuntimeHelper(L"CLosing NotFMAndFMM", RALifeCycleState::Closing, false, true, false);

    RuntimeHelper(L"Closed NotFMAndNotFMM", RALifeCycleState::Closed, false, false, false);
}

BOOST_AUTO_TEST_CASE(ServiceTypeRegisteredStalenessChecks)
{
    ServiceTypeRegistrationHelper(L"Open FMAndNotFMM", RALifeCycleState::Open, true, false, true);
    ServiceTypeRegistrationHelper(L"Open FMAndFMM", RALifeCycleState::Open, true, true, true);
    ServiceTypeRegistrationHelper(L"Open NotFMAndNotFMM", RALifeCycleState::Open, false, false, false);
    ServiceTypeRegistrationHelper(L"Open NotFMAndFMM", RALifeCycleState::Open, false, true, false);

    ServiceTypeRegistrationHelper(L"CLosing FMAndNotFMM", RALifeCycleState::Closing, true, false, false);
    ServiceTypeRegistrationHelper(L"CLosing FMAndFMM", RALifeCycleState::Closing, true, true, false);
    ServiceTypeRegistrationHelper(L"CLosing NotFMAndNotFMM", RALifeCycleState::Closing, false, false, false);
    ServiceTypeRegistrationHelper(L"CLosing NotFMAndFMM", RALifeCycleState::Closing, false, true, false);

    ServiceTypeRegistrationHelper(L"Closed NotFMAndNotFMM", RALifeCycleState::Closed, false, false, false);
}

BOOST_AUTO_TEST_CASE(ServiceTypeRegisteredIsIgnoredIfNodeIsDeactivated)
{
    ScenarioTestHolder holder;

    auto & test = Test;
    test.DeactivateNode(10);

    test.AddFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P RD D N F 1:1]");

    test.ProcessServiceTypeRegisteredAndDrain(L"SP1");

    test.ValidateFT(L"SP1", L"O None 000/000/411 1:1 - [N/N/P RD D N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ServiceTypeRegisteredEnqueuesOnlyJobItemsForAffectedFT)
{
    ScenarioTestHolder holder;

    auto & test = Test;
    test.AddFT(L"SP1", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");
    test.AddFT(L"SP2", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]");

    test.AddInsertedFT(L"SL1");
    test.AddDeletedFT(L"SL2");

    test.ProcessServiceTypeRegistered(L"SP1");

    auto actual = test.UTContext.ThreadpoolStubObj.FTQueue.Count;
    Verify::AreEqual(2, actual, L"Only one item should be enqueued");

    test.DrainJobQueues();
}

BOOST_AUTO_TEST_CASE(AppHostDownEnqueuesJobItemsOnlyForImpactedFT)
{
    ScenarioTestHolder holder;

    auto & test = Test;
    test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    test.AddFT(L"SP2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    test.AddFT(L"SV1", L"O None 411/000/422 1:1 S [N/N/P IC U N F 1:1]");

    test.AddInsertedFT(L"SL1");
    test.AddDeletedFT(L"SL2");
    test.ProcessAppHostClosed(L"SP1");

    auto actual = test.UTContext.ThreadpoolStubObj.FTQueue.Count;
    Verify::AreEqual(3, actual, L"Only one item should be enqueued");

    test.DrainJobQueues();
}

BOOST_AUTO_TEST_CASE(RuntimeDownEnqueuesJobItemsOnlyForImpactedFT)
{
    ScenarioTestHolder holder;

    auto & test = Test;
    test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    test.AddFT(L"SP2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    test.AddFT(L"SV1", L"O None 411/000/422 1:1 S [N/N/P IC U N F 1:1]");

    test.AddInsertedFT(L"SL1");
    test.AddDeletedFT(L"SL2");

    test.ProcessRuntimeClosed(L"SP1");

    auto actual = test.UTContext.ThreadpoolStubObj.FTQueue.Count;
    Verify::AreEqual(3, actual, L"Only one item should be enqueued");

    test.DrainJobQueues();
}

BOOST_AUTO_TEST_CASE(AppHostDown_CommitFailure_ShouldFailfast)
{
    CommitFailureTestHelper(false, 1);
}

BOOST_AUTO_TEST_CASE(RuntimeDown_CommitFailure_ShouldFailfast)
{
    // runtime down does not commit
    CommitFailureTestHelper(true, 0);
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

