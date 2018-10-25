// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::ReplicaUp;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

#pragma region UpdateServiceDescription

//
// Tests the scenarios around updating service description
// In these tests we define an additional ft context called
// SL1.2 which contains everything the same as SL1 except for the sd
class TestStateMachine_UpdateServiceDescription
{
protected:
    TestStateMachine_UpdateServiceDescription() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_UpdateServiceDescription() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void ReplicaDropUpdateServiceDescriptionFlagTestHelper(bool initialIsSet);
    void ReplicaDownUpdateServiceDescriptionFlagTestHelper(bool initialIsSet);
    void ReplicaCloseUpdateServiceDescriptionFlagTestHelper(bool initialIsSet);
    void UpdateServiceDescriptionFlagTestHelper(bool initialIsSet, ScenarioTest::ExecuteUnderLockTargetFunctionPtr funcPtr);

    void SetFlag(bool isSet);

    void PerformMessageRetryTest(bool isSet);
    void SetupFTForMessageRetryTest(bool isSet);

    void SetupFTForMessageReplyTest(bool isSet);
    void RunMessageReplyCanProcessTest(wstring const & messageBody, bool expected, bool initialFlagState = true);
    void RunMessageReplyRAHookupTest(wstring const & messageBody, bool expected);

    void SetupFTForStalenessTest();
    void ExecuteStalenessTest(ServiceDescription const & newSD);
    void ExecuteUpdateVersionBasedStalenessTest(int initialVersion, int incomingVersion);

    void SetupAndRunServiceDescriptionUpdateTest(std::wstring const & initialState);
    void ValidateServiceDescriptionUpdated();
    void RunServiceDescriptionFlagTest(std::wstring const & initialState, bool expected);

    void ValidateFlagState(bool expected, wstring const & shortName);
    void ValidateMessageIsSent();
    void ValidateMessageIsNotSent();

    void ValidateFlagStateAndVersion(bool expected, wstring const & shortName);

    static const int UpdatedServiceDescriptionVersion;
    wstring SuccessUpdateServiceDescriptionReply;
    ScenarioTestUPtr scenarioTest_;
};

const int TestStateMachine_UpdateServiceDescription::UpdatedServiceDescriptionVersion = 2;

bool TestStateMachine_UpdateServiceDescription::TestSetup()
{
	scenarioTest_ = ScenarioTest::Create();

	SingleFTReadContext copy = Default::GetInstance().SP1_FTContext;
	copy.STInfo.SD.UpdateVersion = UpdatedServiceDescriptionVersion;
	scenarioTest_->AddFTContext(L"SP1.2", copy);

	SuccessUpdateServiceDescriptionReply = L"000/411 [N/P RD U 1:1] 2 Success";
	return true;
}

bool TestStateMachine_UpdateServiceDescription::TestCleanup()
{
	return scenarioTest_->Cleanup();
}

void TestStateMachine_UpdateServiceDescription::UpdateServiceDescriptionFlagTestHelper(bool initialIsSet, ScenarioTest::ExecuteUnderLockTargetFunctionPtr funcPtr)
{
	scenarioTest_->AddFT(L"SP1.2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

	SetFlag(initialIsSet);

	scenarioTest_->ExecuteUnderLock(L"SP1.2", funcPtr);

	auto & ft = scenarioTest_->GetFT(L"SP1.2");
	Verify::IsTrueLogOnError(!ft.LocalReplicaServiceDescriptionUpdatePending.IsSet, L"UpdateSD must have been reset");
}

void TestStateMachine_UpdateServiceDescription::ReplicaCloseUpdateServiceDescriptionFlagTestHelper(bool initialIsSet)
{
	UpdateServiceDescriptionFlagTestHelper(
		initialIsSet,
		[](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent & ra)
	{
        HandlerParameters handlerParameters("a", ft, ra, queue, nullptr, DefaultActivityId);
		ra.CloseLocalReplica(handlerParameters, ReplicaCloseMode::Close, ActivityDescription::Empty);
	});
}

void TestStateMachine_UpdateServiceDescription::ReplicaDropUpdateServiceDescriptionFlagTestHelper(bool initialIsSet)
{
	UpdateServiceDescriptionFlagTestHelper(
		initialIsSet,
		[this](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent & ra)
	{
        HandlerParameters handlerParameters("a", ft, ra, queue, nullptr, DefaultActivityId);
        ra.CloseLocalReplica(handlerParameters, ReplicaCloseMode::Delete, ActivityDescription::Empty);
	});
}

void TestStateMachine_UpdateServiceDescription::ReplicaDownUpdateServiceDescriptionFlagTestHelper(bool initialIsSet)
{
	UpdateServiceDescriptionFlagTestHelper(
		initialIsSet,
		[this](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent & ra)
	{
        HandlerParameters handlerParameters("a", ft, ra, queue, nullptr, DefaultActivityId);
        ra.CloseLocalReplica(handlerParameters, ReplicaCloseMode::AppHostDown, ActivityDescription::Empty);
	});
}

void TestStateMachine_UpdateServiceDescription::SetFlag(bool isSet)
{
	if (isSet)
	{
		scenarioTest_->ExecuteUnderLock(
			L"SP1.2",
			[](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent &)
		{
			ft->Test_GetLocalReplicaServiceDescriptionUpdatePending().SetValue(true, queue);
		});
	}
	else
	{
		Verify::IsTrueLogOnError(!scenarioTest_->GetFT(L"SP1").LocalReplicaServiceDescriptionUpdatePending.IsSet, L"Flag should be unset");
	}
}

void TestStateMachine_UpdateServiceDescription::ValidateFlagStateAndVersion(bool expected, wstring const & shortName)
{
	ValidateFlagState(expected, shortName);

	auto const & ft = scenarioTest_->GetFT(shortName);
	Verify::AreEqualLogOnError(ft.ServiceDescription.UpdateVersion, UpdatedServiceDescriptionVersion, L"ServiceDescriptionUpdateVersion");
}

void TestStateMachine_UpdateServiceDescription::SetupFTForMessageReplyTest(bool isSet)
{
	SetupFTForMessageRetryTest(isSet);
}

void TestStateMachine_UpdateServiceDescription::RunMessageReplyCanProcessTest(wstring const & messageBody, bool expected, bool initialFlagState)
{
	SetupFTForMessageRetryTest(initialFlagState);

	auto body = scenarioTest_->ReadObject<ProxyUpdateServiceDescriptionReplyMessageBody>(L"SP1.2", messageBody);

	bool actual = false;
	scenarioTest_->ExecuteUnderLock(
		L"SP1.2",
		[&actual, &body](LockedFailoverUnitPtr & ft, StateMachineActionQueue &, ReconfigurationAgent &)
	{
		actual = ft->CanProcessUpdateServiceDescriptionReply(body);
	});

	Verify::AreEqualLogOnError(expected, actual, wformatString("Failed match for {0}. Expected = {1}. Actual = {2}", messageBody, expected, actual));
}

void TestStateMachine_UpdateServiceDescription::RunMessageReplyRAHookupTest(wstring const & messageBody, bool expected)
{
	SetupFTForMessageReplyTest(true);
	scenarioTest_->ProcessRAPMessageAndDrain<MessageType::ProxyUpdateServiceDescriptionReply>(L"SP1.2", messageBody);
	ValidateFlagState(expected, L"SP1.2");
}

void TestStateMachine_UpdateServiceDescription::SetupFTForMessageRetryTest(bool isSet)
{
	scenarioTest_->AddFT(L"SP1.2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
	SetFlag(isSet);
}

void TestStateMachine_UpdateServiceDescription::PerformMessageRetryTest(bool isSet)
{
	SetupFTForMessageRetryTest(isSet);

    scenarioTest_->RA.UpdateServiceDescriptionMessageRetryWorkManager.BackgroundWorkManager.Request(L"a");
    scenarioTest_->DrainJobQueues();
}

void TestStateMachine_UpdateServiceDescription::SetupFTForStalenessTest()
{
	scenarioTest_->AddClosedFT(L"SP1");
}

void TestStateMachine_UpdateServiceDescription::ExecuteStalenessTest(ServiceDescription const & newSD)
{
	auto & ft = scenarioTest_->GetFT(L"SP1");
	bool result = ft.ValidateUpdateServiceDescription(newSD);
	VERIFY_IS_FALSE(result);
}

void TestStateMachine_UpdateServiceDescription::ValidateMessageIsSent()
{
	scenarioTest_->ValidateRAPMessage<MessageType::ProxyUpdateServiceDescription>(L"SP1.2", L"000/411 [N/P RD U 1:1] - s");
}

void TestStateMachine_UpdateServiceDescription::ValidateMessageIsNotSent()
{
	scenarioTest_->ValidateNoRAPMessages();
}

void TestStateMachine_UpdateServiceDescription::ExecuteUpdateVersionBasedStalenessTest(int initialVersion, int incomingVersion)
{
	SetupFTForStalenessTest();

	auto & ft = scenarioTest_->GetFT(L"SP1");
	auto initialSD = ft.ServiceDescription;
	initialSD.UpdateVersion = initialVersion;

	auto finalSD = initialSD;
	finalSD.UpdateVersion = incomingVersion;

	ft.Test_SetServiceDescription(initialSD);

	ExecuteStalenessTest(finalSD);
}

void TestStateMachine_UpdateServiceDescription::SetupAndRunServiceDescriptionUpdateTest(std::wstring const & initialState)
{
	scenarioTest_->AddFT(L"SP1", initialState);

	auto sd = scenarioTest_->ReadContext.GetSingleFTContext(L"SP1.2")->STInfo.SD;

	scenarioTest_->ExecuteUnderLock(
		L"SP1",
		[sd](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent&)
	{
		ft.EnableUpdate();
		ft->UpdateServiceDescription(queue, sd);
	});
}

void TestStateMachine_UpdateServiceDescription::ValidateServiceDescriptionUpdated()
{
	Verify::AreEqual(UpdatedServiceDescriptionVersion, scenarioTest_->GetFT(L"SP1").ServiceDescription.UpdateVersion, L"ServiceDescription: UpdateVersion");
}

void TestStateMachine_UpdateServiceDescription::RunServiceDescriptionFlagTest(std::wstring const & initialState, bool expected)
{
	SetupAndRunServiceDescriptionUpdateTest(initialState);
	ValidateFlagState(expected, L"SP1");
}

void TestStateMachine_UpdateServiceDescription::ValidateFlagState(bool expected, wstring const & shortName)
{
	Verify::AreEqual(expected, scenarioTest_->GetFT(shortName).LocalReplicaServiceDescriptionUpdatePending.IsSet, L"Flag state");
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_UpdateServiceDescriptionSuite,TestStateMachine_UpdateServiceDescription)

BOOST_AUTO_TEST_CASE(UpdatedServiceDescriptionInMessageCausesUpdateToFT_ReplicaMessageBody)
{
    // Simple test to ensure that the service description is updated
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    // Note the use of SL1.2
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::AddReplica>(L"SP1.2", L"000/411 [N/I RD U 2:1]");

    ValidateFlagStateAndVersion(true, L"SP1");
}

BOOST_AUTO_TEST_CASE(UpdatedServiceDescriptionInMessageCausesUpdateToFT_ConfigurationMessageBody)
{
    // Simple test to ensure that the service description is updated
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1] [N/N/I RD U N F 2:1]");

    // Note the use of SL1.2
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::DoReconfiguration>(L"SP1.2", L"411/412 [P/P RD U 1:1] [I/S RD U 2:1]");
    
    ValidateFlagStateAndVersion(true, L"SP1");
}

BOOST_AUTO_TEST_CASE(UpdatedServiceDescriptionInMessageCausesUpdateToFT_DeactivateMessageBody)
{
    // Simple test to ensure that the service description is updated
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 2:1 CM [N/N/S RD U N F 2:1] [N/N/P RD U N F 1:1]");

    // Note the use of SL1.2
    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::Deactivate>(1, L"SP1.2", L"411/412 {411:0} false [P/P RD U 1:1] [S/N RD U 2:1]");

    ValidateFlagStateAndVersion(false, L"SP1");
}

BOOST_AUTO_TEST_CASE(UpdatedServiceDescriptionInMessageCausesUpdateToFT_ActivateMessageBody)
{
    // Simple test to ensure that the service description is updated
    scenarioTest_->AddFT(L"SP1", L"O None 411/411/412 2:1 CM [S/S/S RD U N F 2:1] [P/P/P RD U N F 1:1] [S/S/N RD U N F 3:1]");

    // Note the use of SL1.2
    scenarioTest_->ProcessRemoteRAMessageAndDrain<MessageType::Activate>(1, L"SP1.2", L"411/412 [P/P RD U 1:1] [S/S RD U 2:1] [S/N RD U 3:1]");

    ValidateFlagStateAndVersion(true, L"SP1");
}

BOOST_AUTO_TEST_CASE(UpdateServiceRequestTest)
{
    // Two FTs (SL1 and SL3) of the same service description (SL1)
    // SL3 is closed, SL1 is open
    // UpdateServiceRequest comes in and is processed
    // Validate that both the FTs get updated
    // In this test case the instance is the same
    scenarioTest_->AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    scenarioTest_->AddClosedFT(L"SL3");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpdateServiceRequest>(L"SL1 2 1");

    // Reply must be sent
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpdateServiceReply>(L"SL1 2 1");
    scenarioTest_->ValidateFT(L"SL3", L"C None 000/000/411 1:1 -2");
    scenarioTest_->ValidateFT(L"SL1", L"O None 000/000/411 1:1 CM2 [N/N/N RD U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(UpdateServiceRequestIfNoFTs)
{
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpdateServiceRequest>(L"SL1 2 1");

    // Reply must be sent
    scenarioTest_->ValidateFMMessage<MessageType::NodeUpdateServiceReply>(L"SL1 2 1");
}

BOOST_AUTO_TEST_CASE(ReplyIsSentToFMM)
{
    // For FM FT reply should be sent to FMM
    scenarioTest_->ProcessFMMessageAndDrain<MessageType::NodeUpdateServiceRequest>(L"FM 2 1");

    // Reply must be sent to FMM
    scenarioTest_->ValidateFmmMessage<MessageType::NodeUpdateServiceReply>(L"FM 2 1");
}

BOOST_AUTO_TEST_CASE(ReplicaUpReplyInNormalListUpdatesServiceDescription)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CMK [N/N/P SB U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1.2 000/000/411 [N/N/P SB U 1:1]} | {SP2 000/000/411 [N/N/P SB U 1:1]}");

    // SD is updated
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM2 [N/N/P SB U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(ReplicaUpReplyInDroppedListUpdatesServiceDescription)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 S [N/N/P SB U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false | {SP1.2 000/000/411 [N/N/P SB U 1:1]}");

    // SD is updated
    scenarioTest_->ValidateFT(L"SP1", L"C None 000/000/411 1:1 P2L");
}

BOOST_AUTO_TEST_CASE(LowerServiceDescriptionInReplicaUpReplyDoesNotUpdateServiceDescription_NormalList)
{
    // Local FT has version 3. Message has version 2
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 CMK3 [N/N/P SB U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false {SP1.2 000/000/411 [N/N/P SB U 1:1]} | {SP2 000/000/411 [N/N/P SB U 1:1]}");

    // SD is unchanged
    scenarioTest_->ValidateFT(L"SP1", L"O None 000/000/411 1:1 CM3 [N/N/P SB U N F 1:1]");
}

BOOST_AUTO_TEST_CASE(LowerServiceDescriptionInReplicaUpReplyDoesNotUpdateServiceDescription_DroppedList)
{
    scenarioTest_->AddFT(L"SP1", L"O None 000/000/411 1:1 S3 [N/N/P SB U N F 1:1]");

    scenarioTest_->ProcessFMMessageAndDrain<MessageType::ReplicaUpReply>(L"false false | {SP1.2 000/000/411 [N/N/P SB U 1:1]}");

    scenarioTest_->ValidateFT(L"SP1", L"C None 000/000/411 1:1 P3L");
}

BOOST_AUTO_TEST_CASE(Staleness_UpdateServiceDescriptionForDifferentServiceIsIgnored)
{
    SetupFTForStalenessTest();

    auto sd = Default::GetInstance().SP2_SP2_STContext.SD;
    sd.UpdateVersion = sd.UpdateVersion + 1;

    ExecuteStalenessTest(sd);
}

BOOST_AUTO_TEST_CASE(Staleness_UpdateServiceDescriptionForDifferentInstanceIsIgnored)
{
    SetupFTForStalenessTest();

    auto sd = scenarioTest_->GetFT(L"SP1").ServiceDescription;
    sd.Instance = sd.Instance + 1;

    ExecuteStalenessTest(sd);
}

BOOST_AUTO_TEST_CASE(Staleness_UpdateServiceDescriptionForEqualVersionIsIgnored)
{
    ExecuteUpdateVersionBasedStalenessTest(2, 2);
}

BOOST_AUTO_TEST_CASE(Staleness_UpdateServiceDescriptionForLowerVersionIsIgnored)
{
    ExecuteUpdateVersionBasedStalenessTest(3, 2);
}

BOOST_AUTO_TEST_CASE(Update_MethodSetsTheServiceDescription)
{
    SetupAndRunServiceDescriptionUpdateTest(L"C None 000/000/411 1:1 -");
    ValidateServiceDescriptionUpdated();
}

BOOST_AUTO_TEST_CASE(Flag_OnUpdateService_IsNotSet_IfReplicaIsDown)
{
    RunServiceDescriptionFlagTest(L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]", false);
}

BOOST_AUTO_TEST_CASE(Flag_OnUpdateService_IsNotSet_IfReplicaIsClosed)
{
    RunServiceDescriptionFlagTest(L"C None 000/000/411 1:1 -", false);
}

BOOST_AUTO_TEST_CASE(Flag_OnUpdateService_IsNotSet_IfReplicaIsClosing)
{
    RunServiceDescriptionFlagTest(L"O None 000/000/411 1:1 HcCM [N/N/P RD U N F 1:1]", false);
}

BOOST_AUTO_TEST_CASE(Flag_OnUpdateService_IsSet_IfReplicaIsOpeningWithSTR)
{
    RunServiceDescriptionFlagTest(L"O None 000/000/411 1:1 SM [N/N/P RD U N F 1:1]", true);
}

BOOST_AUTO_TEST_CASE(Flag_OnUpdateService_IsNotSet_IfReplicaIsOpeningWithoutSTR)
{
    RunServiceDescriptionFlagTest(L"O None 000/000/411 1:1 S [N/N/P RD U N F 1:1]", false);
}

BOOST_AUTO_TEST_CASE(Flag_OnUpdateService_IsSet_IfRepliaIsOpen)
{
    RunServiceDescriptionFlagTest(L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]", true);
}

BOOST_AUTO_TEST_CASE(Flag_OnUpdateService_IsSet_IfAlreadySet)
{
    RunServiceDescriptionFlagTest(L"O None 000/000/411 1:1 CMT [N/N/P RD U N F 1:1]", true);
}

BOOST_AUTO_TEST_CASE(Flag_OnSet_SendsMessageToRAP)
{
    SetupAndRunServiceDescriptionUpdateTest(L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    ValidateMessageIsSent();
}

BOOST_AUTO_TEST_CASE(Flag_OnNotBeingSet_DoesNotSendMessageToRAP)
{
    SetupAndRunServiceDescriptionUpdateTest(L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]");

    ValidateMessageIsNotSent();
}

BOOST_AUTO_TEST_CASE(Flag_OnUpdateService_IfSet_SendsMessageToRAP)
{
    // The scenario is that an update arrived from i1 to i2. Flag was set
    // Another update arrived from i2 to i3. Flag should stay set and message should be resent
    SetupAndRunServiceDescriptionUpdateTest(L"O None 000/000/411 1:1 CMT [N/N/P RD U N F 1:1]");

    ValidateMessageIsSent();
}

BOOST_AUTO_TEST_CASE(ReplicaClose_ResetsUpdatePendingFlag)
{
    ReplicaCloseUpdateServiceDescriptionFlagTestHelper(true);
}

BOOST_AUTO_TEST_CASE(ReplicaClose_DoesNotSetUpdatePendingFlag)
{
    ReplicaCloseUpdateServiceDescriptionFlagTestHelper(false);
}

BOOST_AUTO_TEST_CASE(ReplicaDrop_ResetsUpdatePendingFlag)
{
    ReplicaDropUpdateServiceDescriptionFlagTestHelper(true);
}

BOOST_AUTO_TEST_CASE(ReplicaDrop_DoesNotSetUpdatePendingFlag)
{
    ReplicaDropUpdateServiceDescriptionFlagTestHelper(false);
}

BOOST_AUTO_TEST_CASE(ReplicaDown_ResetsUpdatePendingFlag)
{
    ReplicaDownUpdateServiceDescriptionFlagTestHelper(true);
}

BOOST_AUTO_TEST_CASE(ReplicaDown_DoesNotSetUpdatePendingFlag)
{
    ReplicaDownUpdateServiceDescriptionFlagTestHelper(false);
}

BOOST_AUTO_TEST_CASE(MessageRetry_MessageIsNotSentIfFlagIsNotSet)
{
    PerformMessageRetryTest(true);
    ValidateMessageIsSent();
}

BOOST_AUTO_TEST_CASE(MessageRetry_MessageIsSentIfFlagIsSet)
{
    PerformMessageRetryTest(false);
    ValidateMessageIsNotSent();
}

BOOST_AUTO_TEST_CASE(BGMR_Hookup)
{
    SetupFTForMessageRetryTest(true);

    scenarioTest_->RequestWorkAndDrain(scenarioTest_->StateItemHelpers.UpdateServiceDescriptionRetryHelper);

    ValidateMessageIsSent();
}

BOOST_AUTO_TEST_CASE(Reply_RA_CanProcess_True_Hookup)
{
    RunMessageReplyRAHookupTest(SuccessUpdateServiceDescriptionReply, false);
}

BOOST_AUTO_TEST_CASE(Reply_RA_CanProcess_False_Hookup)
{
    RunMessageReplyRAHookupTest(L"000/411 [N/P RD U 1:1] 2 GlobalLeaseLost", true);
}

BOOST_AUTO_TEST_CASE(Reply_CanProcess_False_Error)
{
    RunMessageReplyCanProcessTest(L"000/411 [N/P RD U 1:1] 2 GlobalLeaseLost", false);
}

BOOST_AUTO_TEST_CASE(Reply_CanProcess_False_LowerVersion)
{
    RunMessageReplyCanProcessTest(L"000/411 [N/P RD U 1:1] 1 Success", false);
}

BOOST_AUTO_TEST_CASE(Reply_CanProcess_False_FlagIsNotSet)
{
    RunMessageReplyCanProcessTest(SuccessUpdateServiceDescriptionReply, false, false /* initial Flag State */);
}

BOOST_AUTO_TEST_CASE(Reply_CanProcess_False_ReplicaIdMismatch)
{
    RunMessageReplyCanProcessTest(L"000/411 [N/P RD U 2:1] 2 Success", false);
}

BOOST_AUTO_TEST_CASE(Reply_CanProcess_False_ReplicaInstanceIdMismatch)
{
    RunMessageReplyCanProcessTest(L"000/411 [N/P RD U 1:2] 2 Success", false);
}

BOOST_AUTO_TEST_CASE(Reply_CanProcess_Success)
{
    RunMessageReplyCanProcessTest(SuccessUpdateServiceDescriptionReply, true);
}

BOOST_AUTO_TEST_CASE(Reply_Process_UnsetsFlag)
{
    SetupFTForMessageReplyTest(true);

    scenarioTest_->ExecuteUnderLock(
        L"SP1.2",
        [this](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent &)
        {
            auto body = scenarioTest_->ReadObject<ProxyUpdateServiceDescriptionReplyMessageBody>(L"SP1.2", SuccessUpdateServiceDescriptionReply);
            ft->ProcessUpdateServiceDescriptionReply(queue, body);
        });

    ValidateFlagState(false, L"SP1.2");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion
