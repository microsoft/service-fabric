// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

StateMachineActionQueue::StateMachineActionQueue() 
    : actionQueue_(),
      isConsumed_(false)
{}

StateMachineActionQueue::~StateMachineActionQueue()
{
}

void StateMachineActionQueue::Enqueue(StateMachineActionUPtr && action)
{
    ASSERT_IF(isConsumed_, "Cannot enqueue actions in action queue after it has been consumed");

    actionQueue_.push_back(std::move(action));
}

void StateMachineActionQueue::ExecuteAllActions(
    wstring const & activityId, 
    EntityEntryBaseSPtr const & entity, 
    ReconfigurationAgent & reconfigurationAgent)
{
    ASSERT_IF(activityId.empty(), "ActivityId cannot be empty at execute all actions");
    ASSERT_IF(isConsumed_, "Cannot consume an already consumed action queue");

    for (auto & it : actionQueue_)
    {
        it->PerformAction(activityId, entity, reconfigurationAgent);
    }

    isConsumed_ = true;
}

void StateMachineActionQueue::AbandonAllActions(ReconfigurationAgent & reconfigurationAgent)
{
    ASSERT_IF(isConsumed_, "Cannot consume an already consumed action queue");

    for (auto & it : actionQueue_)
    {
        it->CancelAction(reconfigurationAgent);
    }

    isConsumed_ = true;
}
