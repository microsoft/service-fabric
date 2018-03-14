// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ReplicaDownTask::ReplicaDownTask(
    ReplicaDownOperationSPtr const& operation,
    FailoverManager & fm,
    FailoverUnitId const& failoverUnitId,
    ReplicaDescription && replicaDescription,
    Federation::NodeInstance const& from)
    : fm_(fm)
    , operation_(operation)
    , failoverUnitId_(failoverUnitId)
    , replicaDescription_(replicaDescription)
    , from_(from)
{
}

void ReplicaDownTask::CheckFailoverUnit(
    LockedFailoverUnitPtr & lockedFailoverUnit,
    vector<StateMachineActionUPtr> & actions)
{
    fm_.MessageEvents.FTReplicaDownProcess(lockedFailoverUnit->Id.Guid, from_);

    ASSERT_IF(!operation_, "Task already completed");

    fm_.MessageProcessor.ReplicaDownProcessor(lockedFailoverUnit, replicaDescription_);

    actions.push_back(make_unique<ReplicaDownAction>(*this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ReplicaDownTask::ReplicaDownAction::ReplicaDownAction(ReplicaDownTask & task)
    : task_(task)
{
}

int ReplicaDownTask::ReplicaDownAction::OnPerformAction(FailoverManager & fm)
{
    ReplicaDownOperationSPtr operation = move(task_.operation_);
    operation->AddResult(fm, task_.failoverUnitId_, move(task_.replicaDescription_));

    return 0;
}

void ReplicaDownTask::ReplicaDownAction::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("ReplicaDown:{0}", task_.from_.Id);
}

void ReplicaDownTask::ReplicaDownAction::WriteToEtw(uint16 contextSequenceId) const
{
    FailoverManager::FMEventSource->StateMachineAction(contextSequenceId, wformatString(*this));
}
