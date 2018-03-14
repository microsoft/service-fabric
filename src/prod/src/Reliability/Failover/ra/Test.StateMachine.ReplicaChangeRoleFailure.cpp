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
    const int ErrorThreshold = 3;
    const int RestartThreshold = 5;
    const wstring DefaultFT = L"SP1";
    const wstring Volatile = L"SV1";
    const ErrorCodeValue::Enum DefaultError = ErrorCodeValue::GlobalLeaseLost;
	const ErrorCodeValue::Enum RAPStateChangedOnDataLossError = ErrorCodeValue::RAProxyStateChangedOnDataLoss;
}

class TestStateMachine_ReplicaChangeRoleFailure : public StateMachineTestBase
{
protected:
    TestStateMachine_ReplicaChangeRoleFailure()
    {
        Test.UTContext.Config.ReplicaChangeRoleFailureErrorReportThreshold = ErrorThreshold;
        Test.UTContext.Config.ReplicaChangeRoleFailureRestartThreshold = RestartThreshold;
        ft_ = DefaultFT;
    }

    void ProcessUCReplyOnSwap(ErrorCodeValue::Enum err, int count)
    {
        auto msg = wformatString("411/422 [P/P RD U 1:1] [P/S RD U 1:1] [S/P RD U 2:1]  [S/S RD U 3:1] {0} C", err);
        ProcessUpdateConfigurationReply(msg, count);
    }

    void ProcessUCReplyOnFailover(ErrorCodeValue::Enum err, int count)
    {
        auto msg = wformatString(L"411/422 [S/P RD U 1:1] [S/P RD U 1:1] [P/I RD D 2:1] [S/S RD U 3:1] {0} C", err);
        ProcessUpdateConfigurationReply(msg, count);
    }

	void ProcessUCReplyOnFailoverWithDataLoss(ErrorCodeValue::Enum err, int count)
	{
		auto msg = wformatString(L"411/522 [S/P RD U 1:1 2 2] [S/P RD U 1:1 2 2] [P/I ID D 2:1] [S/I ID D 3:1] {0} C", err);
		ProcessUpdateConfigurationReply(msg, count);
	}

    void ProcessUpdateConfigurationReply(wstring const & message, int count)
    {
        for (int i = 0; i < count; i++)
        {
            Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(ft_, message);
        }
    }

    void ProcessCancelCatchup(ErrorCodeValue::Enum err, int count)
    {
        auto msg = wformatString(L"411/422 [P/S RD U 1:1] [P/S RD U 1:1] {0} -", err);

        for (int i = 0; i < count; i++)
        {
            Test.ProcessRAPMessageAndDrain<MessageType::CancelCatchupReplicaSetReply>(ft_, msg);
        }
    }

    void SetupReadyPrimaryReplica()
    {
        SetupReadyPrimaryReplica(DefaultFT);
    }

    void SetupReadyPrimaryReplica(wstring const & ft)
    {
        ft_ = ft;
        Test.AddFT(ft_, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/S RD U N F 2:1] [N/N/S RD U N F 3:1]");
    }

    void ProcessSwapPrimary()
    {
        Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ft_, L"411/422 [P/S RD U 1:1] [S/P RD U 2:1] [S/S RD U 3:1]");
    }

    void ProcessFailover()
    {
        Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ft_, L"411/422 [S/P RD U 1:1] [P/I RD D 2:1] [S/S RD U 3:1]"); //GetLSN
        Test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(ft_, L"411/422 [S/P RD U 1:1 1 1] Success"); //CatchUp
    }

	void ProcessFailoverWithDataLoss()
	{
		Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ft_, L"411/522 [S/P RD U 1:1] [P/I ID D 2:1] [S/I ID D 3:1]"); //GetLSN
		Test.ProcessRAPMessageAndDrain<MessageType::ReplicatorUpdateEpochAndGetStatusReply>(ft_, L"411/522 [S/P RD U 1:1 2 2] Success"); //CatchUp
	}

    void ProcessAbortSwap()
    {
        Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(ft_, L"411/433 [P/P RD U 1:1] [S/S RD D 2:1] [S/S RD U 3:1]");
    }

    void ValidateFTRetryableErrorState(RetryableErrorStateName::Enum stateName, int expectedFailureCount)
    {
        Test.ValidateFTRetryableErrorState(ft_, stateName, expectedFailureCount);
    }

    void ValidateErrorHealthEvent()
    {
        Test.ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Error, ft_, DefaultError);
    }

    void ValidateHealthEventCleared()
    {
        Test.ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::ClearError, ft_, ErrorCodeValue::Success);
    }

    void Dump()
    {
        Test.DumpFT(ft_);
    }

    void ValidateFTCloseMode(ReplicaCloseMode expected)
    {
        auto & ft = Test.GetFT(ft_);

        Verify::AreEqual(expected, ft.CloseMode, wformatString("ft close mode mistmatch\r\n{0}", ft));
    }

    void VerifyRemoteReplicasAreMarkedToBeRestartedAppropriately()
    {
        auto & ft = Test.GetFT(ft_);

        auto & replicaStore = ft.Test_GetReplicaStore();

        for (auto const & it : replicaStore.ConfigurationRemoteReplicas)
        {
            if (it.IsUp && it.IsInConfiguration && it.IsReady)
            {
                // only ready and inconfig replicas 
                Verify::IsTrue(it.ToBeRestarted, wformatString("Replica ({0}) should be marked for restart if it is up, inConfig and ready.", it.ReplicaId));
            }
            else
            {
                Verify::IsTrue(!it.ToBeRestarted, wformatString("Replica ({0}) should not be marked for restart.", it.ReplicaId));
            }
        }
    }

    void ErrorReportClearedOnCancelCatchupCompletingTestHelper(ErrorCodeValue::Enum err)
    {
        SetupReadyPrimaryReplica();

        ProcessSwapPrimary();

        ProcessUCReplyOnSwap(ErrorCodeValue::GlobalLeaseLost, ErrorThreshold);

        ProcessAbortSwap();

        Test.ResetAll();
        Dump();

        ProcessCancelCatchup(err, 1);

        ValidateHealthEventCleared();
    }

    wstring ft_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReplicaChangeRoleFailureSuite, TestStateMachine_ReplicaChangeRoleFailure)

BOOST_AUTO_TEST_CASE(GenerateHealthReportTest_SwapPrimary)
{
    SetupReadyPrimaryReplica();

    ProcessSwapPrimary();

    Test.ResetAll();
    Dump();

    ProcessUCReplyOnSwap(DefaultError, ErrorThreshold);

    ValidateFTRetryableErrorState(RetryableErrorStateName::ReplicaChangeRoleAtCatchup, ErrorThreshold);

    ValidateErrorHealthEvent();
}

BOOST_AUTO_TEST_CASE(GenerateHealthReportTest_Failover)
{
    SetupReadyPrimaryReplica();

    ProcessFailover();

    Test.ResetAll();
    Dump();

    ProcessUCReplyOnFailover(DefaultError, ErrorThreshold);
    ValidateFTRetryableErrorState(RetryableErrorStateName::ReplicaChangeRoleAtCatchup, ErrorThreshold);

    ValidateErrorHealthEvent();
}

BOOST_AUTO_TEST_CASE(DoNotGenerateHealthReportTest)
{
    SetupReadyPrimaryReplica();

    ProcessFailover();

    Test.ResetAll();
    Dump();

    ProcessUCReplyOnFailover(DefaultError, ErrorThreshold - 1);

    ValidateFTRetryableErrorState(RetryableErrorStateName::ReplicaChangeRoleAtCatchup, ErrorThreshold - 1);

    Test.ValidateNoReplicaHealthEvent();
}

BOOST_AUTO_TEST_CASE(DoNotGenerateHealthReportIfErrorIsRAProxyStateChangedOnDataLoss)
{
	SetupReadyPrimaryReplica();

	ProcessFailoverWithDataLoss();

	Test.ResetAll();
	Dump();

	ProcessUCReplyOnFailoverWithDataLoss(RAPStateChangedOnDataLossError, ErrorThreshold);

	Test.ValidateNoReplicaHealthEvent();
}

BOOST_AUTO_TEST_CASE(ClearHealthReportTest)
{
    SetupReadyPrimaryReplica();

    ProcessFailover();

    Test.ResetAll();
    Dump();

    ProcessUCReplyOnFailover(DefaultError, ErrorThreshold);
    
    ValidateFTRetryableErrorState(RetryableErrorStateName::ReplicaChangeRoleAtCatchup, ErrorThreshold);

    ValidateErrorHealthEvent();

    Test.ResetAll();
    Dump();

    ProcessUCReplyOnFailover(ErrorCodeValue::Success, 1);

    ValidateHealthEventCleared();
}

BOOST_AUTO_TEST_CASE(VolatileReplicaIsDroppedTest)
{
    SetupReadyPrimaryReplica(Volatile);

    ProcessFailover();

    Test.ResetAll();
    Dump();

    ProcessUCReplyOnFailover(DefaultError, RestartThreshold);

    ValidateFTCloseMode(ReplicaCloseMode::Drop);
}

BOOST_AUTO_TEST_CASE(RestartReplicaTest)
{
    SetupReadyPrimaryReplica();

    ProcessFailover();

    Test.ResetAll();
    Dump();

    ProcessUCReplyOnFailover(DefaultError, RestartThreshold);

    ValidateFTCloseMode(ReplicaCloseMode::Restart);
}

BOOST_AUTO_TEST_CASE(RestartReplicaNegativeTest)
{
    SetupReadyPrimaryReplica();

    ProcessFailover();

    Test.ResetAll();
    Dump();

    ProcessUCReplyOnFailover(DefaultError, RestartThreshold - 1);

    ValidateFTCloseMode(ReplicaCloseMode::None);
}

BOOST_AUTO_TEST_CASE(AbortSwapSuccessClearsErrorHealthReport)
{
    ErrorReportClearedOnCancelCatchupCompletingTestHelper(ErrorCodeValue::Success);
}

BOOST_AUTO_TEST_CASE(AbortSwapDemoteCompletClearsErrorHealthReport)
{
    ErrorReportClearedOnCancelCatchupCompletingTestHelper(ErrorCodeValue::RAProxyDemoteCompleted);
}

BOOST_AUTO_TEST_CASE(AbortSwapContinuesHealthReport)
{
    SetupReadyPrimaryReplica();

    ProcessSwapPrimary();

    ProcessUCReplyOnSwap(DefaultError, ErrorThreshold - 1);

    ProcessAbortSwap();

    Test.ResetAll();
    Dump();

    ProcessCancelCatchup(DefaultError, 1);

    ValidateErrorHealthEvent();
}

BOOST_AUTO_TEST_CASE(AbortSwapFailureCausesRestart)
{
    SetupReadyPrimaryReplica();

    ProcessSwapPrimary();

    ProcessAbortSwap();

    Test.ResetAll();
    Dump();

    ProcessCancelCatchup(DefaultError, RestartThreshold);

    ValidateFTCloseMode(ReplicaCloseMode::Restart);
}

BOOST_AUTO_TEST_CASE(OnDataLossErrorInUpdateConfigReplyWhenSecondaryIsNotReady)
{
    Test.AddFT(L"SP1", L"O Phase2_Catchup 411/000/522 1:1 CMB [P/N/S RD U N F 1:1] [S/N/P IB U N F 2:1] [S/N/S RD U N F 3:1]");

    Test.ProcessRAPMessageAndDrain<MessageType::UpdateConfigurationReply>(L"SP1", L"411/522 [P/S RD U 1:1 2 2] [S/P IC U 2:1] [S/S RD U 3:1] RAProxyStateChangedOnDataLoss C");

    VerifyRemoteReplicasAreMarkedToBeRestartedAppropriately();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
