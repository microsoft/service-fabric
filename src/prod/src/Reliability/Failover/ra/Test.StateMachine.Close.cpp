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
}

class TestStateMachine_ReplicaClose : public StateMachineTestBase
{
protected:
    TestStateMachine_ReplicaClose() : StateMachineTestBase()
    {
        Test.UTContext.Config.PerNodeMinimumIntervalBetweenMessageToFMEntry.Test_SetValue(TimeSpan::Zero);
    }

    void AddReadyPrimaryFT()
    {
        Test.AddFT(ft_, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    }

    void SetupFTRetryableErrorStateThreshold(RetryableErrorStateName::Enum state, int warningThreshold, int dropThreshold)
    {
        auto & ft = GetFT();
        ft.RetryableErrorStateObj.Test_SetThresholds(state, warningThreshold, dropThreshold, INT_MAX, INT_MAX);
    }

    void AddReadySecondaryFT()
    {
        Test.AddFT(ft_, L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1] [N/N/P RD U N F 2:1]");
    }

    void AddStateless()
    {
        Test.AddFT(ft_, L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    }

    void AddFT(std::wstring const & ftShortName, std::wstring const & ft)
    {
        ft_ = ftShortName;
        Test.AddFT(ftShortName, ft);
    }

    void AddReadyReplica()
    {
        if (ft_ == Stateless)
        {
            AddStateless();
        }
        else
        {
            AddReadyPrimaryFT();
        }
    }

    void TestSetup(wstring const & ftShortName)
    {
        ft_ = ftShortName;
    }

    void ProcessReportFault(FaultType::Enum faultType)
    {
        Test.ProcessRAPMessageAndDrain<MessageType::ReportFault>(ft_, wformatString("000/411 [N/P RD U 1:1] {0}", faultType));
    }

    void ProcessDeleteReplica()
    {
        Test.ProcessFMMessageAndDrain<MessageType::DeleteReplica>(ft_, L"000/411 [N/P RD U 1:1]");
    }

    void ProcessRemoveInstance()
    {
        Test.ProcessFMMessageAndDrain<MessageType::RemoveInstance>(ft_, L"000/411 [N/N RD U 1:1]");
    }

    void ProcessObliterate()
    {
        auto & ft = GetFT();

        Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            ft_,
            [&] (EntityExecutionContext & base)
            {
                auto & context = base.As<FailoverUnitEntityExecutionContext>(); 
                ft.StartCloseLocalReplica(ReplicaCloseMode::Obliterate, ReconfigurationAgent::InvalidNode, context, ActivityDescription::Empty);
            });
    }

    void ProcessReplicaCloseReply(ProxyMessageFlags::Enum flags)
    {
        ProcessReplicaCloseReply(flags, 1);        
    }

    void ProcessReplicaCloseReplyWithError(ProxyMessageFlags::Enum flags, ErrorCode errorCode)
    {
        Test.ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(ft_, wformatString("000/411 [N/P RD U 1:{0}] {1} {2}", 1, errorCode.ReadValue(), flags));
    }

    void ProcessReplicaCloseReply(ProxyMessageFlags::Enum flags, int64 instance)
    {
        ProcessReplicaCloseReply(flags, instance, 411);
    }

    void ProcessReplicaCloseReply(ProxyMessageFlags::Enum flags, int64 instance, int epoch)
    {
        Test.ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(ft_, wformatString("000/{2} [N/P RD U 1:{0}] Success {1}", instance, flags, epoch));
    }
    
    void ProcessDeactivateNode()
    {
        Test.ProcessFMMessageAndDrain<MessageType::NodeDeactivateRequest>(L"2 false");
    }

	void ProcessActivateNode()
	{
		Test.ProcessFMMessageAndDrain<MessageType::NodeActivateRequest>(L"3 false");
	}

    void ProcessAppHostDown()
    {
        Test.ProcessAppHostClosedAndDrain(ft_);
    }

    void ProcessServiceTypeRegistered()
    {
        Test.ProcessServiceTypeRegisteredAndDrain(ft_);
    }

    void ProcessReadWriteStatusRevoked()
    {
        Test.ProcessRAPMessageAndDrain<MessageType::ReadWriteStatusRevokedNotification>(ft_, L"000/411 [N/P RD U 1:1]");
    }

    void ProcessReplicaDownReply()
    {
        Test.ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {" + wformatString("{0} 000/000/411 [N/N/N DD D 1:1]", ft_) + L"} |");
    }

    void ProcessDeactivate()
    {
        Test.ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(2, ft_, L"411/412 {411:0} false [P/P RD U 2:1] [S/N RD U 1:1]");
    }

    void ProcessRuntimeClosed()
    {
        Test.ProcessRuntimeClosedAndDrain(ft_);
    }

    void ReplicaDownRetry()
    {
        Test.RA.GetFMMessageRetryComponent(*FailoverManagerId::Fm).Request(L"A");
    }

    void Reset()
    {
        Test.ResetAll();
    }

    void ValidateDeleteReplicaReply()
    {
        ValidateDeleteReplicaReply(1);
    }

    void ValidateDeleteReplicaReply(int64 instance)
    {
        Test.ValidateFMMessage<MessageType::DeleteReplicaReply>(ft_, wformatString("000/411 [N/N DD D 1:{0} n] Success", instance));
    }

    void ValidateDeactivateReply()
    {
        Test.ValidateRAMessage<MessageType::DeactivateReply>(2, ft_, L"411/412 [N/N DD D 1:1 n] Success");
    }

    void ValidateRemoveInstanceReply()
    {
        Test.ValidateFMMessage<MessageType::RemoveInstanceReply>(ft_, L"000/411 [N/N DD D 1:1 n] Success");
    }

    void ValidateReplicaDown()
    {
        ValidateReplicaDown(Reliability::ReplicaStates::StandBy);
    }

    void ValidateReplicaDown(Reliability::ReplicaStates::Enum state)
    {
        wstring role = state == Reliability::ReplicaStates::Dropped ? L"N" : L"P";
        bool isReportFromPrimary = Test.GetFT(ft_).LocalReplica.IsUp;
        if (state == Reliability::ReplicaStates::Dropped)
        {
            Test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {" + wformatString("{0} 000/000/411 [N/N/{2} {1} D 1:1 n]", ft_, state, role) + L"}");
        }
        else
        {
            Test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false {" + wformatString("{0} 000/000/411 [N/N/{2} {1} D 1:1] ({3})", ft_, state, role, isReportFromPrimary) + L" } |");
        }
    }

    void ValidateDeleted()
    {
        auto & ft = GetFT();

        Verify::AreEqual(FailoverUnitStates::Closed, ft.State, L"State");
        Verify::IsTrueLogOnError(ft.LocalReplicaDeleted, L"Deleted");
    }

    void ValidateDown()
    {
        auto & ft = GetFT();

        Verify::AreEqual(FailoverUnitStates::Open, ft.State, L"State");
        Verify::IsTrueLogOnError(!ft.LocalReplica.IsUp, L"Down");
    }

    void ValidateNoFMMessage()
    {
        Test.ValidateNoFMMessages();
    }

    void ValidateNoFMReplicaLifecycleMessage()
    {
        Test.ValidateNoFMMessagesOfType<MessageType::ReplicaUp>();
    }

    void ValidateDropped()
    {
        auto & ft = GetFT();

        Verify::AreEqual(FailoverUnitStates::Closed, ft.State, L"State");
        Verify::AreEqual(FMMessageStage::ReplicaDropped, ft.FMMessageStage, L"Dropped Pending");
    }
    
    void ValidateReplicaDropped()
    {
        ValidateReplicaDropped(1);
    }

    void ValidateReplicaDropped(int64 instance)
    {
        auto replica = wformatString("{0} 000/000/411 [N/N/N DD D 1:{1} n]", ft_, instance);
        Test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false | {" + replica + L"}");
    }

    void ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Enum type,
        Common::ErrorCodeValue::Enum error)
    {
        Test.ValidateReplicaHealthEvent(type, ft_, 1, 1, error);
    }

    void ValidateFTRetryableErrorState(RetryableErrorStateName::Enum stateName, int expectedFailureCount)
    {
        Test.ValidateFTRetryableErrorState(ft_, stateName, expectedFailureCount);
    }

    FailoverUnit & GetFT()
    {
        return Test.GetFT(ft_);
    }

    void Dump()
    {
        Test.DumpFT(ft_);
    }

    void DumpAndReset()
    {
        Dump();

        Reset();
    }

    void StatefulSimpleDeleteScenarioTest(wstring const & ft);
    void ReportFaultDuringDelete(FaultType::Enum faultType);
    void ReportFaultPermanentSanityTest(wstring const &ft);

    wstring ft_;
};

void TestStateMachine_ReplicaClose::StatefulSimpleDeleteScenarioTest(wstring const & ft)
{
    TestSetup(ft);

    AddReadyPrimaryFT();

    ProcessDeleteReplica();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop);

    ValidateDeleteReplicaReply();

    ValidateDeleted();

    Dump();
}

void TestStateMachine_ReplicaClose::ReportFaultDuringDelete(FaultType::Enum faultType)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeleteReplica();

    ProcessReportFault(faultType);

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop);

    ValidateDeleteReplicaReply();

    Dump();
}

void TestStateMachine_ReplicaClose::ReportFaultPermanentSanityTest(wstring const &ft)
{
    TestSetup(ft);

    AddReadyReplica();

    ProcessReportFault(FaultType::Permanent);

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop);

    ValidateReplicaDropped();

    ValidateDropped();

    Dump();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ReplicaCloseSuite, TestStateMachine_ReplicaClose)

BOOST_AUTO_TEST_CASE(DeleteReplicaCompletesDeleteAndSendsReply_SP)
{
    StatefulSimpleDeleteScenarioTest(Persisted);
}

BOOST_AUTO_TEST_CASE(DeleteReplicaCompletesDeleteAndSendsReply_SV)
{
    StatefulSimpleDeleteScenarioTest(Volatile);
}

BOOST_AUTO_TEST_CASE(RemoveInstanceSendsRemoveInstanceReply)
{
    TestSetup(Stateless);

    AddStateless();

    ProcessRemoveInstance();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop);

    ValidateRemoveInstanceReply();

    ValidateDeleted();

    Dump();
}

BOOST_AUTO_TEST_CASE(ReportFaultPermanentDuringDeleteCompletesDelete)
{
    ReportFaultDuringDelete(FaultType::Permanent);
}

BOOST_AUTO_TEST_CASE(ReportFaultTransientDuringDeleteCompletesDelete)
{
    ReportFaultDuringDelete(FaultType::Transient);
}

BOOST_AUTO_TEST_CASE(NodeDeactivateDuringDeleteCompletesDelete)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeleteReplica();

    ProcessDeactivateNode();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop);

    ValidateDeleteReplicaReply();

    Dump();
}

BOOST_AUTO_TEST_CASE(AppHostDownDuringDeleteTestScenario)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeleteReplica();

    ProcessAppHostDown();

    // Cannot send app host down as delete has been accepted
    ValidateNoFMMessage();

    DumpAndReset();

    // Now validate that when host comes back delete is compelted
    // but no message is sent until retry happesn from FM
    // TODO: this can be optimized by ra remembering the delete intent
    ProcessServiceTypeRegistered();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop);

    ValidateNoFMReplicaLifecycleMessage();
    
    DumpAndReset();

    ProcessDeleteReplica();

    ValidateDeleteReplicaReply();
} 

BOOST_AUTO_TEST_CASE(ObliterateDuringDeleteTestScenario)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeleteReplica();

    ProcessObliterate();

    ProcessAppHostDown();

    // Cannot send app host down as delete has been accepted
    ValidateNoFMReplicaLifecycleMessage();

    DumpAndReset();

    ProcessDeleteReplica();

    ValidateDeleteReplicaReply();
}

BOOST_AUTO_TEST_CASE(ReportFaultPermanentSanityTest_SP)
{
    ReportFaultPermanentSanityTest(Persisted);
}

BOOST_AUTO_TEST_CASE(ReportFaultPermanentSanityTest_SV)
{
    ReportFaultPermanentSanityTest(Volatile);
}

BOOST_AUTO_TEST_CASE(ReportFaultPermanentSanityTest_SL)
{
    ReportFaultPermanentSanityTest(Stateless);
}


BOOST_AUTO_TEST_CASE(ReportFaultPermanentWithAppHostDown)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessReportFault(FaultType::Permanent);

    ProcessAppHostDown();

    DumpAndReset();

    ProcessServiceTypeRegistered();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop);
    
    Dump();
    
            ValidateReplicaDropped();


    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(ReportFaultTransientWithAppHostDown)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessReportFault(FaultType::Transient);

    ProcessAppHostDown();

    DumpAndReset();

    ProcessServiceTypeRegistered();

    ProcessReplicaCloseReply(ProxyMessageFlags::None);

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(ReportFaultWithReadWriteStatusRevokedNotificationAcknowledgedBeforeClose)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessReportFault(FaultType::Transient);

    ProcessReadWriteStatusRevoked();

    ValidateReplicaDown();

    DumpAndReset();

    ProcessReplicaDownReply();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::None);

    ValidateNoFMReplicaLifecycleMessage();

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(ReportFaultWithReadWriteStatusRevokedNotificationNotAcknowledgedBeforeClose)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessReportFault(FaultType::Transient);

    ProcessReadWriteStatusRevoked();

    ValidateReplicaDown();  

    DumpAndReset();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::None);    

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(DeactivateNodeWithReadWriteStatusRevokedNotificationAcknowledgedBeforeClose)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeactivateNode();

    ProcessReadWriteStatusRevoked();

    ValidateReplicaDown();

    DumpAndReset();

    ProcessReplicaDownReply();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::None);

    ValidateNoFMReplicaLifecycleMessage();

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(DeactivateNodeWithDelayedReadWriteStatusRevokedNotification)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeactivateNode();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::None);

    ValidateReplicaDown();

    ProcessReplicaDownReply();

    DumpAndReset();

    ProcessReadWriteStatusRevoked();

    ValidateNoFMReplicaLifecycleMessage();
}

BOOST_AUTO_TEST_CASE(DeactivateNodeWithAppHostDownAfterReadWriteRevokedReply)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeactivateNode();

    ProcessReadWriteStatusRevoked();

    ProcessReplicaDownReply();

    DumpAndReset();

    ProcessAppHostDown();

    ValidateNoFMReplicaLifecycleMessage();
}

BOOST_AUTO_TEST_CASE(DeactivateNodeWithReportFaultPermanentDeactivatesNode)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeactivateNode();

    DumpAndReset();

    ProcessReportFault(FaultType::Permanent);

    ProcessReplicaCloseReply(ProxyMessageFlags::None);

    ValidateReplicaDown();
}

BOOST_AUTO_TEST_CASE(DeactivateNodeWithAppHostDownCompletes)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeactivateNode();

    DumpAndReset();

    ProcessAppHostDown();

    ValidateReplicaDown();
}

BOOST_AUTO_TEST_CASE(DeactivateNodeWithObliterate)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessDeactivateNode();

    DumpAndReset();

    ProcessObliterate();

    ProcessAppHostDown();

    ValidateReplicaDropped();
}

BOOST_AUTO_TEST_CASE(DeactivateSNSanity)
{
    TestSetup(Persisted);

    AddReadySecondaryFT();

    ProcessDeactivate();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop, 1, 412);

    ValidateDeactivateReply();

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(DeactivateReadWriteStautsRevoked)
{
    TestSetup(Persisted);

    AddReadySecondaryFT();

    ProcessDeactivate();

    DumpAndReset();

    ProcessReadWriteStatusRevoked();

    Test.ValidateFMMessage<MessageType::ReplicaUp>();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::Drop, 1, 412);

    ValidateDeactivateReply();

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(RuntimeClosedSanity)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessRuntimeClosed();

    DumpAndReset();

    ProcessReplicaCloseReply(ProxyMessageFlags::None);

    ValidateReplicaDown();
}

BOOST_AUTO_TEST_CASE(RuntimeClosedWithAppHostDownSanity)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessRuntimeClosed();
    
    DumpAndReset();

    ProcessAppHostDown();

    ValidateReplicaDown();
}

BOOST_AUTO_TEST_CASE(ReplicaCloseReplyWithErrorSanity)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    SetupFTRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaClose, 1, INT_MAX);

    ProcessReportFault(FaultType::Permanent);

    ProcessReplicaCloseReplyWithError(ProxyMessageFlags::None, ErrorCode(ErrorCodeValue::GlobalLeaseLost));

    ValidateFTRetryableErrorState(RetryableErrorStateName::ReplicaClose, 1);

    ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::CloseWarning, Common::ErrorCodeValue::GlobalLeaseLost);

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(AppHostDownDuringReportFaultPermanent)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    SetupFTRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaClose, 1, INT_MAX);

    ProcessReportFault(FaultType::Permanent);

    ProcessAppHostDown();

    ValidateFTRetryableErrorState(RetryableErrorStateName::ReplicaClose, 1);

    ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::CloseWarning, Common::ErrorCodeValue::ApplicationHostCrash);

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(AppHostDownDuringReportFaultPermanentDropReplicaIfDropThresholdHit)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    SetupFTRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaClose, 5, 1);

    ProcessReportFault(FaultType::Permanent);

    ProcessAppHostDown();

    ValidateDropped();

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(AppHostDownDuringReplicaDelete)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    SetupFTRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaDelete, 1, INT_MAX);

    ProcessDeleteReplica();

    ProcessAppHostDown();

    ValidateFTRetryableErrorState(RetryableErrorStateName::ReplicaDelete, 1);

    ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::CloseWarning, Common::ErrorCodeValue::ApplicationHostCrash);

    DumpAndReset();
}

BOOST_AUTO_TEST_CASE(AppHostDownWithReplicaCloseModeForceDelete)
{
	TestSetup(Persisted);

	AddReadyPrimaryFT();

	SetupFTRetryableErrorStateThreshold(RetryableErrorStateName::ReplicaDelete, 1, INT_MAX);

	ProcessDeleteReplica();

	// First Deactivate then Activate node. Close mode will be ForceDelete
	ProcessDeactivateNode();

	ProcessActivateNode();

	ProcessAppHostDown();

	ValidateFTRetryableErrorState(RetryableErrorStateName::ReplicaDelete, 1);

	ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::CloseWarning, Common::ErrorCodeValue::ApplicationHostCrash);

	DumpAndReset();
}

BOOST_AUTO_TEST_CASE(AppHostDownDuringDroppingFTWithNodeDeactivated)
{
    TestSetup(Persisted);

    AddReadyPrimaryFT();

    ProcessReportFault(FaultType::Permanent);

    ProcessDeactivateNode();

    ProcessAppHostDown();

    ValidateDown();
}

BOOST_AUTO_TEST_CASE(StaleReplicaOpenReply_AfterAppHostClosed)
{
    AddFT(L"SP1", L"O None 000/000/411 1:1 MS (sender:1) [N/N/I IC U N F 1:1]");

    ProcessAppHostDown();

    Test.ProcessRAPMessageAndDrain<MessageType::ReplicaOpenReply>(L"SP1", L"000/411 [N/I RD U 1:1] Success -");
}

BOOST_AUTO_TEST_CASE(StaleReplicaReOpenReply_AfterAppHostClosed)
{
    AddFT(L"SP1", L"O None 000/000/411 1:1 MS (sender:1) [N/N/I IC U N F 1:1]");

    ProcessAppHostDown();

    Test.ProcessRAPMessageAndDrain<MessageType::StatefulServiceReopenReply>(L"SP1", L"000/411 [N/I RD U 1:1] Success -");
}

BOOST_AUTO_TEST_CASE(SlowProgressAfterReadQuorumClearsMessageRetryFlag)
{
    // Regression test for 11232660
    Test.UTContext.Clock.SetManualMode();

    AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/I RD U N F 1:1]");

    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [I/P RD U 1:1] [P/S SB U 2:1] [S/S RD U 3:1] [S/S RD U 4:1]");

    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(2, L"SP1", L"411/422 [N/S SB U 2:1 2 2] Success");
    Test.ProcessRemoteRAMessageAndDrain<MessageType::GetLSNReply>(4, L"SP1", L"411/422 [N/S SB U 4:1 2 2] Success");

    Test.ResetAll();

    Test.UTContext.Clock.AdvanceTime(Test.UTContext.Config.RemoteReplicaProgressQueryWaitDuration + TimeSpan::FromSeconds(1));

    // Processing message retry should not assert because flag should have been cleared
    Test.ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1", L"411/422 [I/P RD U 1:1] [P/S SB U 2:1] [S/S RD D 3:1] [S/S RD U 4:1]");

    Dump();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
