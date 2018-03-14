// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ClientServerTransport;
using namespace Common;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace Transport;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Management;
using namespace Management::RepairManager;

StringLiteral const TraceComponent("HealthCheckAsyncOperation");

HealthCheckAsyncOperation::HealthCheckAsyncOperation(
    RepairManagerServiceReplica & owner,    
    Store::ReplicaActivityId const & activityId,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , ReplicaActivityTraceComponent(activityId)
    , owner_(owner)
{
}

void HealthCheckAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    bool needed = IsHealthCheckNeeded();
    if (needed)
    {
        this->PerformHealthCheck(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr);
    }    
}

bool HealthCheckAsyncOperation::IsHealthCheckNeeded()
{
    wstring message;

    bool needed = false;

    if (!RepairManagerConfig::GetConfig().EnableClusterHealthQuery)
    {
        message = L"EnableClusterHealthQuery config setting is disabled";
    }
    else if (owner_.State != RepairManagerState::Started)
    {
        message = wformatString("Invalid state {0} to start health checks", owner_.State);
    }
    else
    {
        auto lastQueriedAt = owner_.GetHealthLastQueriedTime(); // LQA
        auto now = DateTime::Now();
        auto idle = RepairManagerConfig::GetConfig().HealthCheckOnIdleDuration;

        needed = now - lastQueriedAt < idle;
        message = wformatString(
            "Health last queried at (LQA): {0}, Now: {1}, idle timeout (HealthCheckOnIdleDuration): {2}",
            lastQueriedAt, now, idle);
    }

    WriteInfo(TraceComponent, "{0} IsHealthCheckNeeded: {1}. {2}.", this->ReplicaActivityId, needed, message);
    return needed;
}

void HealthCheckAsyncOperation::PerformHealthCheck(__in AsyncOperationSPtr const & thisSPtr)
{
    ClusterHealthQueryDescription clusterHealthQueryDescription;

    auto operation = owner_.HealthClient->BeginGetClusterHealth(
        clusterHealthQueryDescription,
        RepairManagerConfig::GetConfig().MaxCommunicationTimeout,
        [this](AsyncOperationSPtr const & operation) 
        { 
            this->OnPerformHealthCheckComplete(operation, false);
        },
        thisSPtr);
    
    this->OnPerformHealthCheckComplete(operation, true);
}

void HealthCheckAsyncOperation::OnPerformHealthCheckComplete(
    __in AsyncOperationSPtr const & operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    ClusterHealth clusterHealth;    
    auto error = owner_.HealthClient->EndGetClusterHealth(operation, clusterHealth);
    
    WriteInfo(TraceComponent,
        "{0} EndGetClusterHealth completed: error = {1}, aggregated health state: {2}",
        this->ReplicaActivityId,
        error,
        clusterHealth.AggregatedHealthState);

    bool isClusterHealthy = false;

    if (error.IsSuccess())
    {
        // we are okay with warnings since that is the behavior currently in HealthAggregator::IsClusterHealthy
        // that is used by CM for its health checks.
        isClusterHealthy =
            clusterHealth.AggregatedHealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_OK ||
            clusterHealth.AggregatedHealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_WARNING;
    }

    UpdateStore(thisSPtr, error, isClusterHealthy);
}

void HealthCheckAsyncOperation::UpdateStore(__in AsyncOperationSPtr const & thisSPtr, __in ErrorCode clusterHealthError, __in bool isClusterHealthy)
{
    auto storeTx = owner_.CreateTransaction(this->ActivityId);

    HealthCheckStoreData healthCheckStoreData;
    auto error = owner_.GetHealthCheckStoreData(storeTx, this->ReplicaActivityId, healthCheckStoreData);
    if (!error.IsSuccess())
    {
        storeTx.Rollback();
        this->TryComplete(thisSPtr, error);
        return;
    }

    bool modified = ModifyHealthCheckStoreData(clusterHealthError, isClusterHealthy, healthCheckStoreData);
    if (!modified)
    {
        storeTx.CommitReadOnly();
        this->TryComplete(thisSPtr, clusterHealthError);
        return;
    }    

    error = storeTx.UpdateOrInsertIfNotFound(healthCheckStoreData);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent, "{0} UpdateOrInsertIfNotFound failed: error = {1}", this->ReplicaActivityId, error);
        storeTx.Rollback();
        this->TryComplete(thisSPtr, error);
        return;
    }

    auto operation = StoreTransaction::BeginCommit(
        move(storeTx),
        TimeSpan::MaxValue,
        [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
        thisSPtr);
    this->OnCommitComplete(operation, true);
}

/// <summary>
/// In memory modification of the health check store data.
/// The return value indicates if any modification actually happened.
/// </summary>
bool HealthCheckAsyncOperation::ModifyHealthCheckStoreData(
    __in ErrorCode clusterHealthError,
    __in bool isClusterHealthy,
    __inout HealthCheckStoreData & healthCheckStoreData)
{
    auto now = DateTime::Now();    
    auto expiry = RepairManagerConfig::GetConfig().HealthCheckSampleValidDuration;
    auto lea = healthCheckStoreData.LastErrorAt;
    auto lha = healthCheckStoreData.LastHealthyAt;
    auto tolerance = RepairManagerConfig::GetConfig().HealthCheckClockSkewTolerance;

    // if for some reason, the persisted store data is in the future beyond a tolerance, then reset the persisted store data to now.
    // please see comments on RepairManagerConfig::GetConfig().HealthCheckClockSkewTolerance for details
    if (lea - now > tolerance) 
    { 
        WriteWarning(TraceComponent,
            "LastErrorAt {0} is greater than current time {1} beyond HealthCheckClockSkewTolerance {2}, resetting LastErrorAt to current time (or now)", lea, now, tolerance);
        lea = now; 
    }
    if (lha - now > tolerance) 
    { 
        WriteWarning(TraceComponent,
            "LastHealthyAt {0} is greater than current time {1} beyond HealthCheckClockSkewTolerance {2}, resetting LastHealthyAt to current time (or now)", lha, now, tolerance);
        lha = now; 
    }

    // if LastHealthyAt is too stale, then it is considered unhealthy, so update LastErrorAt to now    
    if (now - lha > expiry)
    {
        WriteInfo(TraceComponent,
            "LastHealthyAt {0} is older than current time {1} beyond HealthCheckSampleValidDuration {2} and is considered stale, updating LastErrorAt to current time (or now)", lha, now, expiry);
        lea = now;
    }

    if (clusterHealthError.IsSuccess())
    {
        if (isClusterHealthy)
        {
            lha = now;
        }
        else
        {
            WriteInfo(TraceComponent, "Cluster is unhealthy, updating LastErrorAt {0} to current time (or now)", lea);
            lea = now;
        }
    }
    // else, we don't update anything. Instead, we let LastHealthyAt get stale and update LastErrorAt (now - lha > expiry)

    bool modified = lea != healthCheckStoreData.LastErrorAt || lha != healthCheckStoreData.LastHealthyAt;

    WriteInfo(
        TraceComponent,
        "{0} HealthCheckStoreData modified: {1}, isClusterHealthy = {2}, clusterHealthError: {3}, now: {4}, LastErrorAt[old: {5}, new: {6}], LastHealthyAt[old: {7}, new: {8}]",
        this->ReplicaActivityId, modified, isClusterHealthy, clusterHealthError, now, healthCheckStoreData.LastErrorAt, lea, healthCheckStoreData.LastHealthyAt, lha);

    healthCheckStoreData.LastErrorAt = lea;
    healthCheckStoreData.LastHealthyAt = lha;

    return modified;
}

void HealthCheckAsyncOperation::OnCommitComplete(
    __in AsyncOperationSPtr const & operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    // if EndCommit was successful, propagate the error where we MAY HAVE FAILED to get cluster health
    this->TryComplete(operation->Parent, error);
}
