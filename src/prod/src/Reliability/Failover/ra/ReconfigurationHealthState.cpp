// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Health;


ReconfigurationHealthState::ReconfigurationHealthState(const FailoverUnitId * failoverUnitId, Reliability::ServiceDescription * serviceDescription, int64 * localReplicaId, int64 * localReplicaInstanceId) :
    failoverUnitId_(failoverUnitId),
    serviceDesc_(serviceDescription),
    localReplicaId_(localReplicaId),
    localReplicaInstanceId_(localReplicaInstanceId)
{
}

ReconfigurationAgentComponent::ReconfigurationHealthState::ReconfigurationHealthState(const FailoverUnitId * failoverUnitId, Reliability::ServiceDescription * serviceDescription, int64 * localReplicaId, int64 * localReplicaInstanceId, ReconfigurationHealthState const & other) :
    failoverUnitId_(failoverUnitId),
    serviceDesc_(serviceDescription),
    localReplicaId_(localReplicaId),
    localReplicaInstanceId_(localReplicaInstanceId),
    currentReport_(other.currentReport_)
{
}

void ReconfigurationHealthState::OnPhaseChanged(Infrastructure::StateMachineActionQueue & queue, ReconfigurationStuckDescriptorSPtr clearReport)
{
    if (currentReport_)
    {
        currentReport_.reset();
        IHealthSubsystemWrapper::EnqueueReplicaHealthAction(
            ReplicaHealthEvent::Enum::ClearReconfigurationStuckWarning,
            *failoverUnitId_,
            serviceDesc_->IsStateful,
            *localReplicaId_,
            *localReplicaInstanceId_,
            queue,
            clearReport);
    }
}

void ReconfigurationHealthState::OnReconfigurationPhaseHealthReport(Infrastructure::StateMachineActionQueue & queue, ReconfigurationStuckDescriptorSPtr newReport)
{
    if (!currentReport_ || *newReport != *currentReport_)
    {
        currentReport_ = newReport;
        IHealthSubsystemWrapper::EnqueueReplicaHealthAction(
            ReplicaHealthEvent::Enum::ReconfigurationStuckWarning,
            *failoverUnitId_,
            serviceDesc_->IsStateful,
            *localReplicaId_,
            *localReplicaInstanceId_,
            queue,
            currentReport_);
    }
}
