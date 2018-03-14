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
    wstring const DefaultFT(L"SP1");
}

class TestInMemoryCommits
{
protected:
    TestInMemoryCommits()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestInMemoryCommits()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void ValidateDiskCommit()
    {
        Verify::AreEqual(1, GetScenarioTest().RA.PerfCounters.CommitsPerSecond.Value, L"Commits");
    }

    void AddFT(wstring const & state)
    {
        GetScenarioTest().AddFT(DefaultFT, state);
    }

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    void RunAndValidateDiskCommit(function<void()> func)
    {
        RunAndValidateCommit(func, false);
    }

    void RunAndValidateInMemoryCommit(function<void()> func)
    {
        RunAndValidateCommit(func, true);
    }

    void RunAndValidateCommit(function<void()> func, bool validateInMemory)
    {
        auto preInMemory = GetScenarioTest().RA.PerfCounters.InMemoryCommitsPerSecond.Value;
        auto preDisk = GetScenarioTest().RA.PerfCounters.CommitsPerSecond.Value;

        func();
        GetScenarioTest().DumpFT(DefaultFT);

        auto memoryCount = GetScenarioTest().RA.PerfCounters.InMemoryCommitsPerSecond.Value - preInMemory;
        auto diskCount = GetScenarioTest().RA.PerfCounters.CommitsPerSecond.Value - preDisk;
        TestLog::WriteInfo(wformatString("InMemory: {0}. Disk: {1}", memoryCount, diskCount));
        
        if (validateInMemory)
        {
            Verify::AreEqual(1, memoryCount, L"Memory commits");
        }
        else
        {
            Verify::AreEqual(1, diskCount, L"Disk commits");
        }
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestInMemoryCommits::TestSetup()
{
    holder_ = ScenarioTestHolder::Create(UnitTestContext::Option::TestAssertEnabled | UnitTestContext::Option::StubJobQueueManager);
    return true;
}

bool TestInMemoryCommits::TestCleanup()
{
    holder_.reset();
    return true;
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestInMemoryCommitsSuite, TestInMemoryCommits)

namespace GetLSNUtility
{
    wstring const Initial = L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/P RD U N F 3:1]";
    wstring const AfterProcessingMessage = L"O None 411/000/422 1:1 CM [S/N/S RD U N F 1:1] [S/N/S RD U N F 2:1] [P/N/P RD U N F 3:1]";

    void ProcessGetLSN(ScenarioTest & test)
    {
        test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSN>(2, DefaultFT, L"411/422 [S/S RD U 1:1]");
    }
}

BOOST_AUTO_TEST_CASE(GetLSN_Initial)
{
    auto & test = GetScenarioTest();
    AddFT(GetLSNUtility::Initial);

    RunAndValidateDiskCommit([&](){ GetLSNUtility::ProcessGetLSN(test); });
}

BOOST_AUTO_TEST_CASE(GetLSN_Retry)
{
    auto & test = GetScenarioTest();
    AddFT(GetLSNUtility::AfterProcessingMessage);

    RunAndValidateInMemoryCommit([&]() { GetLSNUtility::ProcessGetLSN(test); });
}

namespace ReplicatorGetStatusSecondaryUtlity
{
    wstring const Initial = GetLSNUtility::AfterProcessingMessage;

    void ProcessReplicatorGetStatus(ScenarioTest & test)
    {
        test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(DefaultFT, L"411/422 [S/S RD U 1:1 1 1] Success -");
    }
}

BOOST_AUTO_TEST_CASE(ReplicatorGetStatusReply_Secondary)
{
    AddFT(ReplicatorGetStatusSecondaryUtlity::Initial);

    RunAndValidateInMemoryCommit([&]() { ReplicatorGetStatusSecondaryUtlity::ProcessReplicatorGetStatus(GetScenarioTest()); });
}

namespace DeactivateUtility
{
    wstring const Initial = GetLSNUtility::AfterProcessingMessage;
    wstring const AfterProcessingMessage = L"O None 411/422/422 {422:0} 1:1 CM [S/S/S RD U N F 1:1] [S/P/P RD U N F 2:1] [P/I/I RD D N F 3:1]";

    void ProcessDeactivate(ScenarioTest & test)
    {
        test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, DefaultFT, L"411/422 {422:0} false [S/S RD U 1:1] [S/P RD U 2:1] [P/I RD D 3:1]");
    }
}

BOOST_AUTO_TEST_CASE(Deactivate_Initial)
{
    AddFT(DeactivateUtility::Initial);

    RunAndValidateDiskCommit([&]() { DeactivateUtility::ProcessDeactivate(GetScenarioTest()); });

}

BOOST_AUTO_TEST_CASE(Deactivate_Deactivate)
{
    AddFT(DeactivateUtility::AfterProcessingMessage);

    RunAndValidateInMemoryCommit([&]() { DeactivateUtility::ProcessDeactivate(GetScenarioTest()); });
}

namespace ActivateUtility
{
    wstring const Initial = DeactivateUtility::AfterProcessingMessage;
    wstring const AfterProcessingMessage = L"O None 000/000/422 {422:0} 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]";

    void ProcessDeactivate(ScenarioTest & test)
    {
        test.ProcessRemoteRAMessageAndDrain<MessageType::Activate>(2, DefaultFT, L"411/422 {422:0} [S/S RD U 1:1] [S/P RD U 2:1] [P/I RD D 3:1]");
    }
}

BOOST_AUTO_TEST_CASE(Activate_Initial)
{
    AddFT(ActivateUtility::Initial);

    RunAndValidateDiskCommit([&]() { ActivateUtility::ProcessDeactivate(GetScenarioTest()); });
}

BOOST_AUTO_TEST_CASE(Activate_Retry)
{
    AddFT(ActivateUtility::AfterProcessingMessage);

    RunAndValidateInMemoryCommit([&]() { ActivateUtility::ProcessDeactivate(GetScenarioTest()); });
}

namespace DoReconfigurationUtility
{
    wstring const Initial = L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1] [N/N/S RD U N F 3:1]";
    wstring const AfterInitialProcess = L"O Phase1_GetLSN 411/000/422 1:1 CM [S/N/S RD U RAP F 1:1] [P/N/I RD D RA F 2:1] [S/N/S RD U RA F 3:1]";

    void AddAfterInitialProcess(ScenarioTest & test)
    {
        test.AddFT(DefaultFT, AfterInitialProcess);

        auto &ft = test.GetFT(DefaultFT);

        ft.Test_GetReconfigurationState().Test_SetStartTime(Stopwatch::Now());
    }

    void ProcessPromote(ScenarioTest& test)
    {
        test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(DefaultFT, L"411/422 [S/P RD U 1:1] [P/I RD D 2:1] [S/S RD U 3:1]");
    }

    void ProcessPromoteWithUpdate(ScenarioTest & test)
    {
        test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(DefaultFT, L"411/422 [S/P RD U 1:1] [P/I RD D 2:1] [S/S RD D 3:1]");
    }
}

BOOST_AUTO_TEST_CASE(DoReconfig_InitialPromote)
{
    AddFT(DoReconfigurationUtility::Initial);

    RunAndValidateDiskCommit([&]() { DoReconfigurationUtility::ProcessPromote(GetScenarioTest()); });
}

//BOOST_AUTO_TEST_CASE(InitialPromoteRetry)
//{
//    DoReconfigurationUtility::AddAfterInitialProcess(GetScenarioTest());
//
//    RunAndValidateInMemoryCommit([&]() { DoReconfigurationUtility::ProcessPromote(GetScenarioTest()); });
//}

BOOST_AUTO_TEST_CASE(DoReconfig_InitialPromoteWithUpdate)
{
    DoReconfigurationUtility::AddAfterInitialProcess(GetScenarioTest());

    RunAndValidateDiskCommit([&]() { DoReconfigurationUtility::ProcessPromoteWithUpdate(GetScenarioTest()); });
}

namespace ReplicatorGetStatusPrimaryUtility
{
    wstring const InitialOtherReplicaPending = L"O Phase1_GetLSN 411/000/422 1:1 CM [S/N/P RD U RAP F 1:1] [P/N/I RD D RA F 2:1] [S/N/S RD U RA F 3:1]";
    wstring const InitialOtherReplicaNotPending = L"O Phase1_GetLSN 411/000/422 1:1 CM [S/N/P RD U RAP F 1:1] [P/N/I RD D RA F 2:1] [S/N/S RD U N F 3:1 1 1]";

    void Add(ScenarioTest & test, wstring const & initial)
    {
        test.AddFT(DefaultFT, initial);
    }

    void Process(ScenarioTest & test)
    {
        ReplicatorGetStatusSecondaryUtlity::ProcessReplicatorGetStatus(test);
    }

    void ProcessWithLowerLSN(ScenarioTest & test)
    {
        test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(L"SP1", L"411/422 [S/P RD U 1:1 0 0] Success");
    }
}

BOOST_AUTO_TEST_CASE(ReplicatorGetStatusReply_InitialOtherReplicaPending)
{
    ReplicatorGetStatusPrimaryUtility::Add(GetScenarioTest(), ReplicatorGetStatusPrimaryUtility::InitialOtherReplicaPending);

    RunAndValidateInMemoryCommit([&]() { ReplicatorGetStatusPrimaryUtility::Process(GetScenarioTest()); });
}

BOOST_AUTO_TEST_CASE(FinishPhase1GetLSN_MoveToPhase2)
{
    ReplicatorGetStatusPrimaryUtility::Add(GetScenarioTest(), ReplicatorGetStatusPrimaryUtility::InitialOtherReplicaNotPending);

    RunAndValidateDiskCommit([&]() { ReplicatorGetStatusPrimaryUtility::Process(GetScenarioTest()); });
}

BOOST_AUTO_TEST_CASE(FinishPhase1GetLSN_ChangeConfiguration)
{
    ReplicatorGetStatusPrimaryUtility::Add(GetScenarioTest(), ReplicatorGetStatusPrimaryUtility::InitialOtherReplicaNotPending);

    RunAndValidateDiskCommit([&]() { ReplicatorGetStatusPrimaryUtility::ProcessWithLowerLSN(GetScenarioTest()); });
}

namespace GetLSNReplyUtility
{
    wstring const InitialLocalReplicaPending = L"O Phase1_GetLSN 411/000/422 1:1 CM [S/N/P RD U RAP F 1:1] [P/N/I RD D RA F 2:1] [S/N/S RD U RA F 3:1]";
    wstring const InitialLocalReplicaNotPending = L"O Phase1_GetLSN 411/000/422 1:1 CM [S/N/P RD U N F 1:1 1 1] [P/N/I RD D RA F 2:1] [S/N/S RD U RA F 3:1]";

    void Add(ScenarioTest & test, wstring const & initial)
    {
        test.AddFT(DefaultFT, initial);
    }

    void Process(ScenarioTest & test)
    {
        test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, DefaultFT, L"411/422 [N/N RD U 3:1 1 1] Success");
    }

    void ProcessWithRemoteReplicaDropped(ScenarioTest & test)
    {
        test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(3, DefaultFT, L"411/422 {000:-2} [N/N DD D 3:1 1 1] Success");
    }
}

BOOST_AUTO_TEST_CASE(GetLSNReply_InitialLocalReplicaPending)
{
    GetLSNReplyUtility::Add(GetScenarioTest(), GetLSNReplyUtility::InitialLocalReplicaPending);

    RunAndValidateInMemoryCommit([&]() { GetLSNReplyUtility::Process(GetScenarioTest()); });
}

BOOST_AUTO_TEST_CASE(GetLSNReply_InitialLocalReplicaNOtPending)
{
    GetLSNReplyUtility::Add(GetScenarioTest(), GetLSNReplyUtility::InitialLocalReplicaNotPending);

    RunAndValidateDiskCommit([&]() { GetLSNReplyUtility::Process(GetScenarioTest()); });
}

BOOST_AUTO_TEST_CASE(GetLSNReply_InitialLocalReplicaPendingRemoteReplicaDropped)
{
    GetLSNReplyUtility::Add(GetScenarioTest(), GetLSNReplyUtility::InitialLocalReplicaNotPending);

    RunAndValidateDiskCommit([&]() { GetLSNReplyUtility::ProcessWithRemoteReplicaDropped(GetScenarioTest()); });
}

namespace DeactivateReplyUtility_DeactivationInfoUpdate
{
    wstring const Phase0 = L"O Phase0_Demote 411/000/422 1:1 CM [P/N/S RD U N F 1:1] [S/N/S IB U N T 2:1] [S/N/P RD U N F 3:1]";
    wstring const Phase2 = L"O Phase2_Catchup 411/000/422 1:1 CM [S/N/P RD U N F 1:1] [S/N/S IB U N T 2:1]";
    wstring const Phase3 = L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S IB U RA T 2:1]";

    void Process(ScenarioTest & test)
    {
        test.ProcessRemoteRAMessageAndDrain<MessageType::DeactivateReply>(2, DefaultFT, L"411/422 [S/S IB U 2:1] Success");
    }
}

BOOST_AUTO_TEST_CASE(DeactivateReply_DeactivationInfoUpdate_Phase2)
{
    auto & test = GetScenarioTest();
    AddFT(DeactivateReplyUtility_DeactivationInfoUpdate::Phase2);

    RunAndValidateDiskCommit([&]() { DeactivateReplyUtility_DeactivationInfoUpdate::Process(test); });
}

BOOST_AUTO_TEST_CASE(DeactivateReply_DeactivationInfoUpdate_Phase3)
{
    auto & test = GetScenarioTest();
    AddFT(DeactivateReplyUtility_DeactivationInfoUpdate::Phase3);

    RunAndValidateDiskCommit([&]() { DeactivateReplyUtility_DeactivationInfoUpdate::Process(test); });
}

BOOST_AUTO_TEST_CASE(DeactivateReply_DeactivationInfoUpdate_Phase0)
{
    auto & test = GetScenarioTest();
    AddFT(DeactivateReplyUtility_DeactivationInfoUpdate::Phase0);

    RunAndValidateDiskCommit([&]() { DeactivateReplyUtility_DeactivationInfoUpdate::Process(test); });
}

namespace DeactivateReplyUtility_Phase3
{
    wstring const InitialWithOtherRemotePending = L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U RA F 2:1] [P/S/S RD U RA F 3:1]";
    wstring const InitialWithOtherRemoteNotPending = L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U RA F 2:1] [P/S/S RD U N F 3:1]";
    wstring const InitialAfterProcessWithOtherRemotePending = L"O Phase3_Deactivate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]";

    void Process(ScenarioTest & test)
    {
        test.ProcessRemoteRAMessageAndDrain<MessageType::DeactivateReply>(2, DefaultFT, L"411/422 [S/S RD U 2:1] Success");
    }
}

BOOST_AUTO_TEST_CASE(DeactivateReply_Phase3_InitialWithOtherPending)
{
    auto & test = GetScenarioTest();

    AddFT(DeactivateReplyUtility_Phase3::InitialWithOtherRemotePending);

    RunAndValidateInMemoryCommit([&]() { DeactivateReplyUtility_Phase3::Process(test); });
}

BOOST_AUTO_TEST_CASE(DeactivateReply_Phase3_InitialAfterProcessWithOtherRemotePending)
{
    auto & test = GetScenarioTest();

    AddFT(DeactivateReplyUtility_Phase3::InitialAfterProcessWithOtherRemotePending);

    RunAndValidateInMemoryCommit([&]() { DeactivateReplyUtility_Phase3::Process(test); });
}

BOOST_AUTO_TEST_CASE(DeactivateReply_Phase3_InitialWithOtherRemoteNotPending)
{
    auto & test = GetScenarioTest();

    AddFT(DeactivateReplyUtility_Phase3::InitialWithOtherRemoteNotPending);

    RunAndValidateDiskCommit([&]() { DeactivateReplyUtility_Phase3::Process(test); });
}

namespace ActivateReplyUtility_Phase4
{
    wstring const InitialWithOtherRemotePending = L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U RA F 2:1] [P/S/S RD U RA F 3:1]";
    wstring const InitialWithOtherRemoteNotPending = L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U RA F 2:1] [P/S/S RD U N F 3:1]";
    wstring const InitialAfterProcessWithOtherRemotePending = L"O Phase4_Activate 411/422/422 1:1 CM [S/P/P RD U N F 1:1] [S/S/S RD U N F 2:1] [P/S/S RD U RA F 3:1]";

    void Process(ScenarioTest & test)
    {
        test.ProcessRemoteRAMessageAndDrain<MessageType::ActivateReply>(2, DefaultFT, L"411/422 [S/S RD U 2:1] Success");
    }
}

BOOST_AUTO_TEST_CASE(ActivateReply_Phase4_InitialWithOtherPending)
{
    auto & test = GetScenarioTest();

    AddFT(ActivateReplyUtility_Phase4::InitialWithOtherRemotePending);

    RunAndValidateInMemoryCommit([&]() { ActivateReplyUtility_Phase4::Process(test); });
}

BOOST_AUTO_TEST_CASE(ActivateReply_Phase4_InitialAfterProcessWithOtherRemotePending)
{
    auto & test = GetScenarioTest();

    AddFT(ActivateReplyUtility_Phase4::InitialAfterProcessWithOtherRemotePending);

    RunAndValidateInMemoryCommit([&]() { ActivateReplyUtility_Phase4::Process(test); });
}

BOOST_AUTO_TEST_CASE(ActivateReply_Phase4_InitialWithOtherRemoteNotPending)
{
    auto & test = GetScenarioTest();

    AddFT(ActivateReplyUtility_Phase4::InitialWithOtherRemoteNotPending);

    RunAndValidateDiskCommit([&]() { ActivateReplyUtility_Phase4::Process(test); });
}

BOOST_AUTO_TEST_CASE(StartCloseOfReadyReplica)
{
    auto & test = GetScenarioTest();

    AddFT(L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    RunAndValidateInMemoryCommit([&]()
        {
            test.ProcessRAPMessageAndDrain<MessageType::ReportFault>(DefaultFT, L"000/411 [N/P RD U 1:1] transient");
        });
}

BOOST_AUTO_TEST_CASE(StartDropOfReadyReplica)
{
    auto & test = GetScenarioTest();

    AddFT(L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    RunAndValidateDiskCommit([&]()
        {
            test.ProcessFMMessageAndDrain<MessageType::DeleteReplica>(DefaultFT, L"000/411 [N/P RD U 1:1]");
        });
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
