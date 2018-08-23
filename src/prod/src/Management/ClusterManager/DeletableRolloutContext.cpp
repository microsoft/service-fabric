// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("DeletableRolloutContext");

DeletableRolloutContext::DeletableRolloutContext(
    RolloutContextType::Enum contextType)
    : RolloutContext(contextType)
    , isForceDelete_(false)
    , isConvertToForceDelete_(false)
{
}

DeletableRolloutContext::DeletableRolloutContext(
    RolloutContextType::Enum contextType,
    bool const isForceDelete)
    : RolloutContext(contextType)
    , isForceDelete_(isForceDelete)
    , isConvertToForceDelete_(false)
{
}

DeletableRolloutContext::DeletableRolloutContext(
    DeletableRolloutContext const & other)
    : RolloutContext(other)
    , isForceDelete_(other.isForceDelete_)
    , isConvertToForceDelete_(other.isConvertToForceDelete_)
{
}

DeletableRolloutContext::DeletableRolloutContext(
    RolloutContextType::Enum contextType,
    ComponentRoot const & replica,
    ClientRequestSPtr const & clientRequest)
    : RolloutContext(contextType, replica, clientRequest)
    , isForceDelete_(false)
    , isConvertToForceDelete_(false)
{
}

DeletableRolloutContext::DeletableRolloutContext(
    RolloutContextType::Enum contextType,
    Store::ReplicaActivityId const & replicaActivityId)
    : RolloutContext(contextType, replicaActivityId)
    , isForceDelete_(false)
    , isConvertToForceDelete_(false)
{
}

bool DeletableRolloutContext::ShouldTerminateProcessing()
{
    return this->OperationRetryStopwatch.IsRunning
        && ManagementConfig::GetConfig().RolloutFailureTimeout.SubtractWithMaxAndMinValueCheck(this->OperationRetryStopwatch.Elapsed) <= TimeSpan::Zero;
}

