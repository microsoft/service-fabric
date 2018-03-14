// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Health;
using namespace Infrastructure;

ReportReplicaHealthStateMachineAction::ReportReplicaHealthStateMachineAction(
    ReplicaHealthEvent::Enum type,
    FailoverUnitId ftId,
    bool isStateful,
    uint64 replicaId,
    uint64 instanceId,
    IHealthDescriptorSPtr const & reportDescriptor) :
    type_(type),
    isStateful_(isStateful),
    replicaId_(replicaId),
    instanceId_(instanceId),
    ftId_(ftId),
    reportDescriptor_(move(reportDescriptor))
{
}

void ReportReplicaHealthStateMachineAction::OnPerformAction(
    std::wstring const &, 
    Infrastructure::EntityEntryBaseSPtr const & entity, 
    ReconfigurationAgent & ra)
{
    UNREFERENCED_PARAMETER(entity);                                                   

    auto error = ra.HealthSubsystem.ReportReplicaEvent(type_, ftId_, isStateful_, replicaId_, instanceId_, *reportDescriptor_);

    RAEventSource::Events->ReplicaHealth(
        ftId_.Guid,
        ra.NodeInstance,
        type_, 
        error,
        replicaId_,
        instanceId_);
}

void ReportReplicaHealthStateMachineAction::OnCancelAction(ReconfigurationAgent&)
{
}
