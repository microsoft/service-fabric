// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ApplicationTypeRequestTracker");

struct ApplicationTypeRequestTracker::PendingRequestEntry
{
    PendingRequestEntry(
        ServiceModelVersion const & version,
        ProcessApplicationTypeContextAsyncOperationSPtr const & provisionAsyncOperation) 
        : Version(version) 
        , ProvisionAsyncOperation(provisionAsyncOperation)
    { 
    }

    __declspec (property(get=get_IsProvision)) bool IsProvision;
    bool get_IsProvision() const { return (this->ProvisionAsyncOperation.get() != nullptr); }
    
    ServiceModelVersion Version;
    ProcessApplicationTypeContextAsyncOperationSPtr ProvisionAsyncOperation;
};

//
// ApplicationTypeRequestTracker
//

ApplicationTypeRequestTracker::ApplicationTypeRequestTracker(Store::PartitionedReplicaId const & partitionedReplicaId) 
    : Store::PartitionedReplicaTraceComponent<TraceTaskCodes::ClusterManager>(partitionedReplicaId)
    , pendingRequests_()
    , lock_()
{ 
}

ErrorCode ApplicationTypeRequestTracker::TryStartProvision(
    ServiceModelTypeName const & typeName, 
    ServiceModelVersion const & version, 
    ProcessApplicationTypeContextAsyncOperationSPtr const & provisionAsyncOperation)
{
    return TryStartRequest(typeName, version, provisionAsyncOperation);
}

ErrorCode ApplicationTypeRequestTracker::TryStartUnprovision(ServiceModelTypeName const & typeName, ServiceModelVersion const & version)
{
    return TryStartRequest(typeName, version, nullptr);
}

ErrorCode ApplicationTypeRequestTracker::TryStartRequest(
    ServiceModelTypeName const & typeName, 
    ServiceModelVersion const & version, 
    ProcessApplicationTypeContextAsyncOperationSPtr const & provisionAsyncOperation)
{
    bool isProvisionRequest = (provisionAsyncOperation.get() != nullptr);

    AcquireExclusiveLock lock(lock_);

    auto result = pendingRequests_.insert(make_pair(typeName, make_shared<PendingRequestEntry>(version, provisionAsyncOperation)));
    if (!result.second)
    {
        shared_ptr<PendingRequestEntry> const & entry = result.first->second;

        bool isProvisionExisting = entry->IsProvision;

        if (isProvisionRequest != isProvisionExisting || entry->Version != version)
        {

            WriteInfo(
                TraceComponent, 
                "{0}: cannot {1} ({2}:{3}) - version ({4}) still {5}", 
                this->TraceId, 
                isProvisionRequest ? "provision" : "unprovision",
                typeName, 
                version, 
                entry->Version,
                isProvisionExisting ? "provisioning" : "unprovisioning");

            return ErrorCodeValue::CMRequestAlreadyProcessing;
        }
    }

    return ErrorCodeValue::Success;
}

void ApplicationTypeRequestTracker::TryCancel(ServiceModelTypeName const & typeName)
{
    shared_ptr<PendingRequestEntry> entry;
    {
        AcquireExclusiveLock lock(lock_);

        auto it = pendingRequests_.find(typeName);
        if (it != pendingRequests_.end() && it->second->IsProvision)
        {
            entry = it->second;
        }
    }

    if (entry.get() != nullptr)
    {
        entry->ProvisionAsyncOperation->ExternalCancel();
    }
}

void ApplicationTypeRequestTracker::FinishRequest(ServiceModelTypeName const & typeName, ServiceModelVersion const & version)
{
    AcquireExclusiveLock lock(lock_);

    auto it = pendingRequests_.find(typeName);
    if (it != pendingRequests_.end() && it->second->Version == version)
    {
        pendingRequests_.erase(it);
    }
}

void ApplicationTypeRequestTracker::Clear()
{
    AcquireExclusiveLock lock(lock_);

    pendingRequests_.clear();
}
