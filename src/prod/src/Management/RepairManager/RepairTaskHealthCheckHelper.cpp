// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::RepairManager;

StringLiteral const TraceComponent("RepairTaskHealthCheckHelper");

/// <summary>
/// Core computation of health check state of the repair task. This is static and takes in
/// a few more parameters just for unit-testability.
/// <summary>
RepairTaskHealthCheckState::Enum RepairTaskHealthCheckHelper::ComputeHealthCheckState(
    __in Store::ReplicaActivityId const & replicaActivityId,
    __in wstring const & taskId,
    __in DateTime const & lastErrorAt,
    __in DateTime const & lastHealthyAt,
    __in DateTime const & healthCheckStart,
    __in DateTime const & windowStart,
    __in TimeSpan const & timeout)
{    
    auto healthCheckState = RepairTaskHealthCheckState::InProgress;
    auto now = DateTime::Now();    
    auto expiry = RepairManagerConfig::GetConfig().HealthCheckSampleValidDuration;
    auto stable = RepairManagerConfig::GetConfig().HealthCheckStableDuration;

    if (now - healthCheckStart > timeout)
    {
        healthCheckState = RepairTaskHealthCheckState::TimedOut;
    }
    else if (now - lastHealthyAt < expiry && now - windowStart > stable)
    {
        healthCheckState = RepairTaskHealthCheckState::Succeeded;
    }

    wstring message = wformatString(
        "now = {0}, lastErrorAt = {1}, lastHealthyAt = {2}, healthCheckStart = {3}, windowStart = {4}, expiry = {5}, stable = {6}, timeout = {7}",
        now, lastErrorAt, lastHealthyAt, healthCheckStart, windowStart, expiry, stable, timeout);

    WriteInfo(TraceComponent,
        "{0} Health check computation done, id = {1}, returning = {2}, {3}",
        replicaActivityId, taskId, healthCheckState, message);

    return healthCheckState;
}
