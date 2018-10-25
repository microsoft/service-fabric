// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Infrastructure;

ScenarioTest::ScenarioTest(UnitTestContextUPtr && utContext)
: utContext_(move(utContext)),
  stateItemHelper_(make_unique<StateItemHelper::StateItemHelperCollection>(*utContext_)),
  entityExecutionContextTestUtility_(*this)
{
    // FT contexts need the RA to be passed in
    // FT cannot be created without the config
    for (auto it : Default::GetInstance().AllDefaultFTReadContexts)
    {
        AddFTContext(it->ShortName, *it);
    }

    readContext_.AddSTContexts(Default::GetInstance().AllDefaultSDReadContexts);
}

ScenarioTestUPtr ScenarioTest::Create()
{
    return Create(UnitTestContext::Option::StubJobQueueManager);
}

ScenarioTestUPtr ScenarioTest::Create(UnitTestContext::Option options)
{
    // Force the node to be ready and activated
    // the activation is enabled by default because the node is active by default
    UnitTestContextUPtr utContext = UnitTestContext::Create(options);
    utContext->RA.Test_SetNodeUpAckFromFmmProcessed(true);
    utContext->RA.Test_SetNodeUpAckFromFMProcessed(true);

    ScenarioTest* ptr = new ScenarioTest(move(utContext));
    return ScenarioTestUPtr(ptr);
}

void ScenarioTest::ProcessNodeUpAckAndDrain(wstring const & message, bool isFMM)
{
    ProcessNodeUpAck(message, isFMM);
    DrainJobQueues();
}

void ScenarioTest::ProcessNodeUpAck(wstring const & message, bool isFMM)
{
    Transport::MessageUPtr msg = ReadMessage<MessageType::NodeUpAck>(message, readContext_);
    LogStep(wformatString("NodeUpAck isFMM {0}", isFMM));
    utContext_->RA.ProcessNodeUpAck(move(msg), isFMM);
}

bool ScenarioTest::Cleanup()
{
    utContext_->Cleanup();
    return true;
}

shared_ptr<NodeUpOperationFactory> ScenarioTest::RestartRA()
{
    return utContext_->RestartRA();
}

void ScenarioTest::LogStep(wstring const & step)
{
    TestLog::WriteInfo(wformatString("##########################################"));
    TestLog::WriteInfo(wformatString("{0}", step));
}

void ScenarioTest::AddFTContext(wstring const & ftShortName, StateManagement::SingleFTReadContext const & ftReadContext)
{
    auto context = ftReadContext;
    context.RA = &utContext_->RA;
    context.ShortName = ftShortName;
    readContext_.AddFTContext(ftShortName, context);
}

LocalFailoverUnitMapEntrySPtr ScenarioTest::AddFT(std::wstring const & ftShortName, std::wstring const & ft)
{
    SingleFTReadContext const * thisFTContext = readContext_.GetSingleFTContext(ftShortName);
    Verify::IsTrueLogOnError(thisFTContext != nullptr, wformatString("Failed to find ft short name {0}", ftShortName));

    FailoverUnitContainerUPtr ftContainer;
    if (!FailoverUnitContainer::Create(ft, *thisFTContext, ftContainer))
    {
        Verify::Fail(wformatString("Unable to create FTContainer for {0} {1}", ftShortName, ft));
    }

    ftContainer->InsertIntoLFUM(*utContext_);

    auto rv = ftContainer->Entry;
    ftContainer.reset();

    return rv;
}

LocalFailoverUnitMapEntrySPtr ScenarioTest::AddDeletedFT(std::wstring const & ftShortName)
{
    auto entry = AddInsertedFT(ftShortName);

    auto lock = entry->Test_CreateLock();
    lock.MarkForDelete();

    auto op = RA.LocalFailoverUnitMapObj.BeginCommit(
        entry,
        lock.CreateCommitDescription(),
        entry->State,
        entry->Id,
        TimeSpan::MaxValue,
        [](AsyncOperationSPtr const &) {},
        nullptr);

    ASSERT_IF(!op->CompletedSynchronously, "Must be in memory delete");

    Diagnostics::CommitEntityPerformanceData commitPerfData;
    RA.LocalFailoverUnitMapObj.EndCommit(op, commitPerfData);

    return entry;
}
LocalFailoverUnitMapEntrySPtr ScenarioTest::AddInsertedFT(std::wstring const & ftShortName)
{
    SingleFTReadContext const * thisFTContext = readContext_.GetSingleFTContext(ftShortName);
    Verify::IsTrueLogOnError(thisFTContext != nullptr, wformatString("Failed to find ft short name {0}", ftShortName));

    return static_pointer_cast<LocalFailoverUnitMapEntry>(RA.LocalFailoverUnitMapObj.GetOrCreateEntityMapEntry(thisFTContext->FUID, true));
}

LocalFailoverUnitMapEntrySPtr ScenarioTest::AddClosedFT(std::wstring const & ftShortName)
{
    SingleFTReadContext const * thisFTContext = readContext_.GetSingleFTContext(ftShortName);
    Verify::IsTrueLogOnError(thisFTContext != nullptr, wformatString("Failed to find ft short name {0}", ftShortName));

    FailoverUnitContainerUPtr ftContainer = FailoverUnitContainer::CreateClosedFT(*thisFTContext);
    ftContainer->InsertIntoLFUM(*utContext_);

    auto rv = ftContainer->Entry;
    ftContainer.reset();

    return rv;
}

FailoverUnit & ScenarioTest::GetFT(wstring const & ftShortName)
{
    SingleFTReadContext const * thisFTContext = readContext_.GetSingleFTContext(ftShortName);
    Verify::IsTrueLogOnError(thisFTContext != nullptr, wformatString("Failed to find ft short name {0}", ftShortName));

    auto currentFT = utContext_->RA.LocalFailoverUnitMapObj.GetEntry(thisFTContext->FUID);    
    return *currentFT->As<LocalFailoverUnitMapEntry>().State.Test_Data.Data;
}

StateManagement::SingleFTReadContext const & ScenarioTest::GetSingleFTContext(std::wstring const & ftShortName)
{
    StateManagement::SingleFTReadContext const * ftContext = readContext_.GetSingleFTContext(ftShortName);
    if (ftContext == nullptr)
    {
        Verify::Fail(Common::wformatString("Unable to find ftShortName {0}", ftShortName));
    }

    return *ftContext;
}

EntityEntryBaseList ScenarioTest::GetFailoverUnitEntries(initializer_list<wstring> const & shortNames)
{
    EntityEntryBaseList partitionIds;

    for (auto const & shortName : shortNames)
    {
        partitionIds.push_back(GetLFUMEntry(shortName));
    }

    return partitionIds;
}

void ScenarioTest::DumpFT(wstring const & ftShortName)
{
    FailoverUnit & ft = GetFT(ftShortName);

    auto str = wformatString("FailoverUnit {0}\r\n{1}", ftShortName, ft);
    TestLog::WriteInfo(str);
}

void ScenarioTest::ValidateFT(wstring const & ftShortName, wstring const & expectedFT)
{
      TestLog::WriteInfo(wformatString("{0}", Default::Delimiter));
    SingleFTReadContext const * thisFTContext = readContext_.GetSingleFTContext(ftShortName);
    Verify::IsTrueLogOnError(thisFTContext != nullptr, wformatString("Failed to find ft short name {0}", ftShortName));

    auto expected = Reader::ReadHelper<FailoverUnitSPtr>(expectedFT, *thisFTContext);

    // find the existing FT
    auto currentFT = utContext_->RA.LocalFailoverUnitMapObj.GetEntry(thisFTContext->FUID);
    FailoverUnitContainer::Compare(*expected, *currentFT->As<LocalFailoverUnitMapEntry>().State.Test_Data.Data);
}

void ScenarioTest::ValidateFTNotPresent(wstring const & ftShortName)
{
    SingleFTReadContext const * thisFTContext = readContext_.GetSingleFTContext(ftShortName);
    Verify::IsTrueLogOnError(thisFTContext != nullptr, wformatString("Failed to find ft short name {0}", ftShortName));

    auto currentFT = utContext_->RA.LocalFailoverUnitMapObj.GetEntry(thisFTContext->FUID);
    Verify::IsTrueLogOnError(currentFT == nullptr, wformatString("FT {0} was found in LFUM even though it was expected it would not be there", ftShortName));
}

void ScenarioTest::ValidateFTIsNull(wstring const & ftShortName)
{
    SingleFTReadContext const * thisFTContext = readContext_.GetSingleFTContext(ftShortName);
    Verify::IsTrueLogOnError(thisFTContext != nullptr, wformatString("Failed to find ft short name {0}", ftShortName));

    auto lfumEntry = GetLFUMEntry(ftShortName);
    Verify::IsTrueLogOnError(lfumEntry->State.Test_Data.Data.get() == nullptr, wformatString("FT {0} was not null", ftShortName));
}

void ScenarioTest::ValidateFTRetryableErrorState(std::wstring const & shortName, RetryableErrorStateName::Enum stateName, int expectedFailureCount)
{
    auto & ft = GetFT(shortName);

    Verify::AreEqualLogOnError(stateName, ft.RetryableErrorStateObj.CurrentState, L"RetryableErrorState:Current State");
    Verify::AreEqualLogOnError(expectedFailureCount, ft.RetryableErrorStateObj.CurrentFailureCount, L"RetryableErrorState: Current Fail Count");
}

void ScenarioTest::ValidateFTSTRRetryableErrorState(std::wstring const & shortName, RetryableErrorStateName::Enum stateName, int expectedFailureCount)
{
    auto & ft = GetFT(shortName);

    Verify::AreEqualLogOnError(stateName, ft.ServiceTypeRegistrationRetryableErrorStateObj.CurrentState, L"RetryableErrorState:Current State");
    Verify::AreEqualLogOnError(expectedFailureCount, ft.ServiceTypeRegistrationRetryableErrorStateObj.CurrentFailureCount, L"RetryableErrorState: Current Fail Count");
}

void ScenarioTest::SetFTRetryableErrorState(std::wstring const & shortName, RetryableErrorStateName::Enum stateName)
{
    auto & ft = GetFT(shortName);

    ft.RetryableErrorStateObj.Test_Set(stateName, 0);
    ft.RetryableErrorStateObj.Test_SetThresholds(stateName, INT_MAX, INT_MAX, INT_MAX, INT_MAX);
}

void ScenarioTest::DrainJobQueues()
{
    utContext_->ThreadpoolStubObj.DrainAll(utContext_->RA);
}

LocalFailoverUnitMapEntrySPtr ScenarioTest::GetLFUMEntry(wstring const & ftShortName)
{
    // find the context
    auto context = readContext_.GetSingleFTContext(ftShortName);
    if (context == nullptr)
    {
        Verify::Fail(wformatString("Could not find ftShortName {0} in context", ftShortName));
    }

    return dynamic_pointer_cast<LocalFailoverUnitMapEntry>(utContext_->RA.LocalFailoverUnitMapObj.GetEntry(context->FUID));
}

void ScenarioTest::ExecuteUnderLock(std::wstring const & ftShortName, ExecuteUnderLockTargetFunctionPtr func)
{
    ExecuteUnderLock(ftShortName, func, true);
}

void ScenarioTest::ExecuteUnderLock(std::wstring const & ftShortName, ExecuteUnderLockTargetFunctionPtr func, bool callEnableUpdate)
{
    auto entry = GetLFUMEntry(ftShortName);
    if (entry == nullptr)
    {
        Verify::Fail(wformatString("Could not find entry for {0}", ftShortName));
    }

    ExecuteUnderLockHelper<FailoverUnit>(entry, func, callEnableUpdate);
}

#pragma region Hosting

void ScenarioTest::SetFindServiceTypeRegistrationSuccess(wstring const & ftShortName)
{
    SetFindServiceTypeRegistrationState(ftShortName, FindServiceTypeRegistrationError::CategoryName::Success);
}

void ScenarioTest::SetFindServiceTypeRegistrationState(wstring const & ftShortName, FindServiceTypeRegistrationError::CategoryName::Enum category)
{
    auto context = readContext_.GetSingleFTContext(ftShortName);
    if (context == nullptr)
    {
        Verify::Fail(wformatString("Could not find ftShortName {0} in context", ftShortName));
    }

    UTContext.Hosting.AddFindServiceTypeExpectation(context->STInfo, category);
}

void ScenarioTest::ProcessRuntimeClosed(wstring const & ftShortName)
{
    auto context = GetSingleFTContext(ftShortName);
    RA.ProcessRuntimeClosed(context.STInfo.HostId, context.STInfo.RuntimeId);
}

void ScenarioTest::ProcessAppHostClosed(wstring const & ftShortName, ActivityDescription const & activityDescription)
{
    auto context = GetSingleFTContext(ftShortName);
    RA.ProcessAppHostClosed(context.STInfo.HostId, activityDescription);
}

void ScenarioTest::ProcessServiceTypeRegistered(wstring const & ftShortName)
{
    auto & context = GetSingleFTContext(ftShortName);
    auto registration = context.STInfo.CreateServiceTypeRegistration(context.FUID.Guid);
    RA.ProcessServiceTypeRegistered(registration);
}

void ScenarioTest::ProcessServiceTypeDisabled(
    wstring const & ftShortName,
    uint64 sequenceNumber,
    ServiceModel::ServicePackageActivationMode::Enum activationMode)
{
    auto & context = GetSingleFTContext(ftShortName);
    auto serviceTypeId = context.STInfo.ServiceTypeId;

    ServiceModel::ServicePackageActivationContext activationContext;
    if (activationMode == ServiceModel::ServicePackageActivationMode::ExclusiveProcess)
    {
        activationContext = ServiceModel::ServicePackageActivationContext(context.FUID.Guid);
    }

    RA.ProcessServiceTypeDisabled(serviceTypeId, sequenceNumber, activationContext);
}

void ScenarioTest::ProcessServiceTypeEnabled(
    wstring const & ftShortName,
    uint64 sequenceNumber,
    ServiceModel::ServicePackageActivationMode::Enum activationMode)
{
    auto & context = GetSingleFTContext(ftShortName);
    auto serviceTypeId = context.STInfo.ServiceTypeId;

    ServiceModel::ServicePackageActivationContext activationContext;
    if (activationMode == ServiceModel::ServicePackageActivationMode::ExclusiveProcess)
    {
        activationContext = ServiceModel::ServicePackageActivationContext(context.FUID.Guid);
    }

    RA.ProcessServiceTypeEnabled(serviceTypeId, sequenceNumber, activationContext);
}

void ScenarioTest::ProcessRuntimeClosedAndDrain(wstring const & ftShortName)
{
    LogStep(L"Runtime Down");
    ProcessRuntimeClosed(ftShortName);
    DrainJobQueues();
}

void ScenarioTest::ProcessAppHostClosedAndDrain(wstring const & ftShortName, ActivityDescription const & activityDescription)
{
    LogStep(L"Apphost Down");
    ProcessAppHostClosed(ftShortName, activityDescription);
    DrainJobQueues();
}

void ScenarioTest::ProcessServiceTypeRegisteredAndDrain(wstring const & ftShortName)
{
    LogStep(L"ServiceType Registered");
    ProcessServiceTypeRegistered(ftShortName);
    DrainJobQueues();
}

void ScenarioTest::ValidateNoTerminateCalls()
{
    HostingHelper.ValidateNoTerminateCalls();
}

void ScenarioTest::ValidateTerminateCall(std::wstring const & ftShortName)
{
    auto context = GetSingleFTContext(ftShortName);
    HostingHelper.ValidateTerminateCall(context.STInfo.HostId);
}

void ScenarioTest::ValidateTerminateCall(std::initializer_list<wstring> const & shortNames)
{
    vector<wstring> hostIds;

    for (auto const & it : shortNames)
    {
        auto context = GetSingleFTContext(it);
        hostIds.push_back(context.STInfo.HostId);
    }

    HostingHelper.ValidateTerminateCall(hostIds);
}

#pragma endregion

void ScenarioTest::ValidateNodeUpAckReceived(bool fromFMM, bool fromFM)
{
    Verify::IsTrue(fromFMM == RA.IsReady(FailoverUnitId(Reliability::Constants::FMServiceGuid)), L"IsReady FMM");
    Verify::IsTrue(fromFM == RA.IsReady(StateManagement::Default::GetInstance().SP1_FTContext.FUID), L"IsReady FM");
}

#pragma region Node Activation/Deactivation status

void ScenarioTest::ValidateNodeIsActivated(uint64 expectedSequenceNumber)
{
    ValidateNodeActivationState(true, expectedSequenceNumber);
}

void ScenarioTest::ValidateNodeIsDeactivated(uint64 expectedSequenceNumber)
{
    ValidateNodeActivationState(false, expectedSequenceNumber);
}

void ScenarioTest::ValidateNodeActivationState(bool isActivated, uint64 expectedSequenceNumber)
{
    ValidateNodeActivationState(true, isActivated, expectedSequenceNumber);
    ValidateNodeActivationState(false, isActivated, expectedSequenceNumber);
}

void ScenarioTest::ValidateNodeIsActivated(bool isFmm, uint64 expectedSequenceNumber)
{
    ValidateNodeActivationState(isFmm, true, expectedSequenceNumber);
}

void ScenarioTest::ValidateNodeIsDeactivated(bool isFmm, uint64 expectedSequenceNumber)
{
    ValidateNodeActivationState(isFmm, false, expectedSequenceNumber);
}

void ScenarioTest::ValidateNodeActivationState(bool isFmm, bool isActivated, uint64 expectedSequenceNumber)
{
    auto actual = RA.NodeStateObj.GetNodeDeactivationState(FailoverManagerId(isFmm)).DeactivationInfo;
    Node::NodeDeactivationInfo expected(isActivated, expectedSequenceNumber);

    Verify::Compare(expected, actual, L"Activation State");
}

void ActivateHelper(ReconfigurationAgent & ra, int64 sequenceNumber, bool isActivated)
{
    Node::NodeDeactivationInfo info(isActivated, sequenceNumber);

    ra.NodeStateObj.GetNodeDeactivationState(*FailoverManagerId::Fmm).TryStartChange(L"", info);
    ra.NodeStateObj.GetNodeDeactivationState(*FailoverManagerId::Fm).TryStartChange(L"", info);
}

void ScenarioTest::ActivateNode(int64 sequenceNumber)
{
    ActivateHelper(RA, sequenceNumber, true);
}

void ScenarioTest::DeactivateNode(int64 sequenceNumber)
{
    ActivateHelper(RA, sequenceNumber, false);
}

void ScenarioTest::SetNodeActivationState(bool isActivated, int64 sequenceNumber)
{
    ActivateHelper(RA, sequenceNumber, isActivated);
}

void ScenarioTest::SetNodeUpAckFromFMProcessed(bool value)
{
    RA.Test_SetNodeUpAckFromFMProcessed(value);
}

void ScenarioTest::SetNodeUpAckFromFmmProcessed(bool value)
{
    RA.Test_SetNodeUpAckFromFmmProcessed(value);
}

void ScenarioTest::SetNodeUpAckProcessed(bool fm, bool fmm)
{
    SetNodeUpAckFromFMProcessed(fm);
    SetNodeUpAckFromFmmProcessed(fmm);
}

void ScenarioTest::FireNodeDeactivationProgressCheckTimer(bool isFmm)
{
    auto op = RA.NodeStateObj.GetNodeDeactivationState(FailoverManagerId(isFmm)).Test_GetAsyncOp();
    FireCloseCompletionCheckTimerAndDrain(op);
}

void ScenarioTest::ValidateNodeDeactivationProgressCheckNotRunning(bool isFmm)
{
    auto op = RA.NodeStateObj.GetNodeDeactivationState(FailoverManagerId(isFmm)).Test_GetAsyncOp();
    Verify::IsTrue(op == nullptr, L"Operation must be null");
}

void ScenarioTest::SetLifeCycleState(StateItemHelper::RALifeCycleState::Enum e)
{
    StateItemHelpers.LifeCycleStateHelper.SetState(e);
}

#pragma endregion

#pragma region Upgrade

UpgradeMessageProcessor::UpgradeElement const * GetUpgradeElement(ReconfigurationAgent & ra, wstring const & id, uint64 instanceId)
{
    UpgradeMessageProcessor::UpgradeMessageProcessorMap const & upgradeMap = ra.UpgradeMessageProcessorObj.Test_UpgradeMap;

    auto it = upgradeMap.find(id);
    if (it == upgradeMap.end())
    {
        Verify::Fail(wformatString("Upgrade not found {0}:{1}", id, instanceId));
        return nullptr;
    }

    return &it->second;
}

Reliability::ReconfigurationAgentComponent::Upgrade::UpgradeMessageProcessor::UpgradeElement const * ScenarioTest::GetUpgradeElementForApplicationUpgrade(uint64 instanceId)
{
    return GetUpgradeElement(RA, Default::GetInstance().ApplicationIdentifier.ToString(), instanceId);
}

Reliability::ReconfigurationAgentComponent::Upgrade::UpgradeMessageProcessor::UpgradeElement const * ScenarioTest::GetUpgradeElementForFabricUpgrade(uint64 instanceId)
{
    return GetUpgradeElement(RA, *Constants::FabricApplicationId, instanceId);
}

void ValidateUpgradeIsCompleted(ReconfigurationAgent & ra, wstring const & appId, uint64 instanceId)
{
    auto ptr = GetUpgradeElement(ra, appId, instanceId);
    if (ptr == nullptr)
    {
        return;
    }

    Verify::AreEqualLogOnError(instanceId, ptr->Current->InstanceId, L"Instance id mismatch");
    Verify::AreEqualLogOnError(UpgradeStateName::Completed, ptr->Current->CurrentState, L"State");
}

void ScenarioTest::ValidateFabricUpgradeIsCompleted(uint64 instanceId)
{
    ValidateUpgradeIsCompleted(RA, *Constants::FabricApplicationId, instanceId);
}

void ScenarioTest::ValidateApplicationUpgradeIsCompleted(uint64 instanceId)
{
    ValidateUpgradeIsCompleted(RA, Default::GetInstance().ApplicationIdentifier.ToString(), instanceId);
}

void FireCompletionCheckTimer(ScenarioTest & scenarioTest, UpgradeStateName::Enum stateName, wstring const & appId, uint64 instanceId)
{
    auto ptr = GetUpgradeElement(scenarioTest.RA, appId, instanceId);

    Verify::AreEqualLogOnError(stateName, ptr->Current->CurrentState, L"State");
    ptr->Current->OnTimer();

    scenarioTest.DrainJobQueues();
}

void ScenarioTest::FireApplicationUpgradeCloseCompletionCheckTimer(uint64 instanceId)
{
    auto ptr = GetUpgradeElement(RA, Default::GetInstance().ApplicationIdentifier.ToString(), instanceId);
    FireCloseCompletionCheckTimerAndDrain(ptr->Current->Test_AsyncOp);
}

void ScenarioTest::FireApplicationUpgradeDropCompletionCheckTimer(uint64 instanceId)
{
    auto ptr = GetUpgradeElement(RA, Default::GetInstance().ApplicationIdentifier.ToString(), instanceId);
    FireCloseCompletionCheckTimerAndDrain(ptr->Current->Test_AsyncOp);
}

void ScenarioTest::FireApplicationUpgradeReplicaDownCompletionCheckTimer(uint64 instanceId)
{
    FireCompletionCheckTimer(*this, UpgradeStateName::ApplicationUpgrade_WaitForReplicaDownCompletionCheck, Default::GetInstance().ApplicationIdentifier.ToString(), instanceId);
}

void ScenarioTest::FireFabricUpgradeCloseCompletionCheckTimer(uint64 instanceId)
{
    auto ptr = GetUpgradeElement(RA, *Constants::FabricApplicationId, instanceId);
    auto casted = dynamic_pointer_cast<MultipleReplicaCloseAsyncOperation>(ptr->Current->Test_AsyncOp);
    FireCloseCompletionCheckTimerAndDrain(casted->CloseCompletionCheckAsyncOperation);
}

#pragma endregion

#pragma region Reliability

void ScenarioTest::ValidateLfumUploadData(StateItemHelper::GenerationNumberSource::Enum source, size_t index, wstring const & ftShortName, wstring const & ftInfoStr)
{
    auto context = readContext_.GetSingleFTContext(ftShortName);
    if (context == nullptr)
    {
        Verify::Fail(wformatString("{0} not found", ftShortName));
        return;
    }

    Reader r(ftInfoStr, *context);

    FailoverUnitInfo ftInfoExpected;
    if (!r.Read(L'\0', ftInfoExpected))
    {
        Verify::Fail(L"Failed to parse FTInfo");
        return;
    }

    stateItemHelper_->ReliabilityHelper.ValidateLfumUploadData(source, index, ftInfoExpected);
}

#pragma endregion

#pragma region Store

void ScenarioTest::EnableLfumStoreFaultInjection(Common::ErrorCodeValue::Enum error)
{
    utContext_->FaultInjectedLfumStore.EnableFaultInjection(error);
}

void ScenarioTest::DisableLfumStoreFaultInjection()
{
    utContext_->FaultInjectedLfumStore.DisableFaultInjection();
}

#pragma endregion

#pragma region Replica Up

void ScenarioTest::ExecuteUpdateStateOnLfumLoad(std::wstring const & ftShortName)
{
    ExecuteUpdateStateOnLfumLoad(ftShortName, Default::GetInstance().NodeInstance);
}

void ScenarioTest::ExecuteUpdateStateOnLfumLoad(std::wstring const & ftShortName, Federation::NodeInstance const & nodeInstance)
{
  ExecuteUnderLock(
        ftShortName, 
        [nodeInstance](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent & ra)
        {
            auto executionContext = Infrastructure::EntityTraits<FailoverUnit>::CreateEntityExecutionContext(ra, queue, ft.UpdateContextObj, ft.Current);
            ft->UpdateStateOnLFUMLoad(nodeInstance, executionContext);
        });    
}

#pragma endregion

void ScenarioTest::FireCloseCompletionCheckTimerAndDrain(AsyncOperationSPtr const & op)
{
    ASSERT_IF(op == nullptr, "Op cant be null");

    auto casted = dynamic_pointer_cast<MultipleReplicaCloseCompletionCheckAsyncOperation>(op);
    casted->Test_StartCompletionCheck(casted);

    DrainJobQueues();
}
