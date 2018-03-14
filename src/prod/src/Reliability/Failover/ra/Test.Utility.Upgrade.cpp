// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade::ReliabilityUnitTest;

const bool UpgradeStubBase::DefaultSnapshotWereReplicasClosed = true;

UpgradeStubBase::UpgradeStubBase(
    ReconfigurationAgent & ra, 
    uint64 instanceId, 
    UpgradeStateDescriptionUPtr && state1, 
    UpgradeStateDescriptionUPtr && state2, 
    UpgradeStateDescriptionUPtr && state3) :
    IUpgrade(ra, instanceId),
    AsyncOperationStateAsyncApi(L"AsyncOperationStateApi"),
    activityId_(L"A"),
    startState_(state1->Name)
{
    AddToMap(move(state1));
    AddToMap(move(state2));
    AddToMap(move(state3));
}

void UpgradeStubBase::AddToMap(UpgradeStateDescriptionUPtr && state)
{
    if (state == nullptr)
    {
        return;
    }

    auto name = state->Name;

    pair<UpgradeStateName::Enum, UpgradeStateDescriptionUPtr> p = make_pair(move(name), move(state));

    stateMap_.insert(move(p));
}

void UpgradeStubBase::SendReply()
{
    if (ResendReplyFunction != nullptr)
    {
        ResendReplyFunction();
    }
}

UpgradeStateName::Enum UpgradeStubBase::GetStartState(RollbackSnapshotUPtr && rollbackSnapshot)
{
    rollbackSnapshotPassedAtStart_ = move(rollbackSnapshot);
    return startState_;
}

UpgradeStateDescription const & UpgradeStubBase::GetStateDescription(UpgradeStateName::Enum state) const
{
    auto it = stateMap_.find(state);

    ASSERT_IF(it == stateMap_.end(), "The state must have been inserted into the map");

    return *it->second;
}

UpgradeStateName::Enum UpgradeStubBase::EnterState(UpgradeStateName::Enum state, AsyncStateActionCompleteCallback asyncCallback)
{
    // the state must be defined
    ASSERT_IF(stateMap_.find(state) == stateMap_.end(), "Unknown state");
    ASSERT_IF(EnterStateFunction == nullptr, "EnterStateFunction must be defined");
    return EnterStateFunction(state, asyncCallback);
}

AsyncOperationSPtr UpgradeStubBase::EnterAsyncOperationState(
    UpgradeStateName::Enum state,
    AsyncCallback const & asyncCallback)
{
    ASSERT_IF(stateMap_.find(state) == stateMap_.end(), "Unknown state");

    if (EnterAsyncOperationStateFunction != nullptr)
    {
        EnterAsyncOperationStateFunction(state);
    }
    
    return AsyncOperationStateAsyncApi.OnCall(nullptr, asyncCallback, nullptr);
}

UpgradeStateName::Enum UpgradeStubBase::ExitAsyncOperationState(
    UpgradeStateName::Enum state,
    AsyncOperationSPtr const & asyncOp)
{
    ASSERT_IF(stateMap_.find(state) == stateMap_.end(), "Unknown state");
    ASSERT_IF(ExitAsyncOperationStateFunction == nullptr, "exit func must be defined");
    
    return ExitAsyncOperationStateFunction(state, asyncOp);
}

RollbackSnapshotUPtr UpgradeStubBase::CreateRollbackSnapshot(UpgradeStateName::Enum state) const
{
    return make_unique<RollbackSnapshot>(state, UpgradeStubBase::DefaultSnapshotWereReplicasClosed);
}
