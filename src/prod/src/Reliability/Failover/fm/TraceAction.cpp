// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

TraceMovementAction::TraceMovementAction(LoadBalancingComponent::FailoverUnitMovement && movement, Common::Guid decisionId)
    : movement_(move(movement)), decisionId_(decisionId)
{
}

int TraceMovementAction::OnPerformAction(FailoverManager & fm)
{
    UNREFERENCED_PARAMETER(fm);

    return 0;
}

void TraceMovementAction::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("{0}:{1}", decisionId_, movement_.Actions);
}

void TraceMovementAction::WriteToEtw(uint16 contextSequenceId) const
{
    FailoverManager::FMEventSource->StateMachineAction(contextSequenceId, wformatString("{0}:{1}", decisionId_, movement_.Actions));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TraceDataLossAction::TraceDataLossAction(FailoverUnitId const& failoverUnitId, Epoch dataLossEpoch)
    : failoverUnitId_(failoverUnitId), dataLossEpoch_(dataLossEpoch)
{
}

int TraceDataLossAction::OnPerformAction(FailoverManager & fm)
{
    fm.FTEvents.FTDataLoss(failoverUnitId_.Guid, dataLossEpoch_);

    return 0;
}

void TraceDataLossAction::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("DataLoss:{0}", dataLossEpoch_.DataLossVersion);
}

void TraceDataLossAction::WriteToEtw(uint16 contextSequenceId) const
{
    FailoverManager::FMEventSource->StateMachineAction(contextSequenceId, L"DataLoss");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TraceAutoScaleAction::TraceAutoScaleAction(int targetReplicaSetSize)
    : targetReplicaSetSize_(targetReplicaSetSize)
{
}

int TraceAutoScaleAction::OnPerformAction(FailoverManager & fm)
{
    UNREFERENCED_PARAMETER(fm);

    return 0;
}

void TraceAutoScaleAction::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("AutoScale:{0}", targetReplicaSetSize_);
}

void TraceAutoScaleAction::WriteToEtw(uint16 contextSequenceId) const
{
    FailoverManager::FMEventSource->StateMachineAction(contextSequenceId, wformatString("AutoScale: {0}", targetReplicaSetSize_));
}