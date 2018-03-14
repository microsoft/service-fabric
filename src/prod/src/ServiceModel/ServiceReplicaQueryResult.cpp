// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServiceReplicaQueryResult)

ServiceReplicaQueryResult::ServiceReplicaQueryResult()
    : serviceResultKind_(FABRIC_SERVICE_KIND_INVALID)
    , replicaStatus_(FABRIC_QUERY_SERVICE_REPLICA_STATUS_INVALID)
    , replicaAddress_()
    , nodeName_()
    , replicaId_(FABRIC_INVALID_REPLICA_ID)
    , replicaRole_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , instanceId_(FABRIC_INVALID_REPLICA_ID)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , lastInBuildDurationInSeconds_(0)
{
}

ServiceReplicaQueryResult::ServiceReplicaQueryResult(ServiceReplicaQueryResult && other)
    : serviceResultKind_(other.serviceResultKind_)
    , replicaId_(other.replicaId_)
    , replicaRole_(other.replicaRole_)
    , instanceId_(other.instanceId_)
    , replicaStatus_(other.replicaStatus_)
    , replicaAddress_(move(other.replicaAddress_))
    , nodeName_(move(other.nodeName_))
    , healthState_(other.healthState_)
    , lastInBuildDurationInSeconds_(other.lastInBuildDurationInSeconds_)
{
}

ServiceReplicaQueryResult::ServiceReplicaQueryResult(
    int64 instanceId, 
    FABRIC_QUERY_SERVICE_REPLICA_STATUS status,
    wstring const & address,
    wstring const & nodeName,
    int64 lastInBuildDurationInSeconds)
    : serviceResultKind_(FABRIC_SERVICE_KIND_STATELESS)
    , replicaId_()
    , replicaRole_()
    , instanceId_(instanceId)
    , replicaStatus_(status)
    , replicaAddress_(address)
    , nodeName_(nodeName)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , lastInBuildDurationInSeconds_(lastInBuildDurationInSeconds)
{
}

ServiceReplicaQueryResult::ServiceReplicaQueryResult(
    int64 replicaId, 
    FABRIC_REPLICA_ROLE replicaRole, 
    FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus,
    wstring const & address,
    wstring const & nodeName,
    int64 lastInBuildDurationInSeconds)
    : serviceResultKind_(FABRIC_SERVICE_KIND_STATEFUL)
    , replicaId_(replicaId)
    , replicaRole_(replicaRole)
    , instanceId_()
    , replicaStatus_(replicaStatus)
    , replicaAddress_(address)
    , nodeName_(nodeName)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , lastInBuildDurationInSeconds_(lastInBuildDurationInSeconds)
{
}

ServiceReplicaQueryResult const & ServiceReplicaQueryResult::operator = (ServiceReplicaQueryResult && other)
{
    if (&other != this)
    {
        serviceResultKind_ = other.serviceResultKind_;
        instanceId_ = other.instanceId_;
        replicaId_ = other.replicaId_;
        replicaRole_ = other.replicaRole_;
        replicaStatus_ = other.replicaStatus_;
        replicaAddress_ = move(other.replicaAddress_);
        nodeName_ = move(other.nodeName_);
        healthState_ = other.healthState_;
        lastInBuildDurationInSeconds_ = other.lastInBuildDurationInSeconds_;
    }

    return *this;
}

ServiceReplicaQueryResult ServiceReplicaQueryResult::CreateStatelessServiceInstanceQueryResult(
    int64 instanceId, 
    FABRIC_QUERY_SERVICE_REPLICA_STATUS status,
    wstring const & address,
    wstring const & nodeName,
    int64 lastInBuildDurationInSeconds)
{
    return ServiceReplicaQueryResult(instanceId, status, address, nodeName, lastInBuildDurationInSeconds);
}

ServiceReplicaQueryResult ServiceReplicaQueryResult::CreateStatefulServiceReplicaQueryResult(
    int64 replicaId, 
    FABRIC_REPLICA_ROLE replicaRole, 
    FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus,
    wstring const & address,
    wstring const & nodeName,
    int64 lastInBuildDurationInSeconds)
{
    return ServiceReplicaQueryResult(replicaId, replicaRole, replicaStatus, address, nodeName, lastInBuildDurationInSeconds);
}

void ServiceReplicaQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM & publicServiceReplicaQueryResult) const 
{
    publicServiceReplicaQueryResult.Kind = serviceResultKind_;

    if (serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        auto statelessInstance = heap.AddItem<FABRIC_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM>();
        statelessInstance->InstanceId = instanceId_;
        statelessInstance->ReplicaStatus = replicaStatus_;
        statelessInstance->ReplicaAddress = heap.AddString(replicaAddress_);
        statelessInstance->NodeName = heap.AddString(nodeName_);
        statelessInstance->AggregatedHealthState = healthState_;
        statelessInstance->LastInBuildDurationInSeconds = lastInBuildDurationInSeconds_;

        publicServiceReplicaQueryResult.Value = statelessInstance.GetRawPointer();
    }
    else if(serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        auto statefulReplica = heap.AddItem<FABRIC_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM>();
        statefulReplica->ReplicaId = replicaId_;
        statefulReplica->ReplicaRole = replicaRole_;
        statefulReplica->ReplicaStatus = replicaStatus_;
        statefulReplica->ReplicaAddress = heap.AddString(replicaAddress_);
        statefulReplica->NodeName = heap.AddString(nodeName_);
        statefulReplica->AggregatedHealthState = healthState_;
        statefulReplica->LastInBuildDurationInSeconds = lastInBuildDurationInSeconds_;

        publicServiceReplicaQueryResult.Value = statefulReplica.GetRawPointer();
    }    
}

ErrorCode ServiceReplicaQueryResult::FromPublicApi(__in FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM const& replicaQueryResult)
{
    ErrorCode error = ErrorCode::Success();
    serviceResultKind_ = replicaQueryResult.Kind;  
    if (serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        auto statefulReplicaQueryResult = reinterpret_cast<FABRIC_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM*>(replicaQueryResult.Value);

        replicaId_ = statefulReplicaQueryResult->ReplicaId;
        replicaRole_ = statefulReplicaQueryResult->ReplicaRole;
        replicaStatus_ = statefulReplicaQueryResult->ReplicaStatus;
        replicaAddress_ = statefulReplicaQueryResult->ReplicaAddress;
        nodeName_ = statefulReplicaQueryResult->NodeName;
        healthState_ = statefulReplicaQueryResult->AggregatedHealthState;
        lastInBuildDurationInSeconds_ = statefulReplicaQueryResult->LastInBuildDurationInSeconds;
    }
    else if(serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        auto statelessInstanceQueryResult = reinterpret_cast<FABRIC_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM*>(replicaQueryResult.Value);

        instanceId_ = statelessInstanceQueryResult->InstanceId;
        replicaStatus_ = statelessInstanceQueryResult->ReplicaStatus;
        replicaAddress_ = statelessInstanceQueryResult->ReplicaAddress;
        nodeName_ = statelessInstanceQueryResult->NodeName;
        healthState_ = statelessInstanceQueryResult->AggregatedHealthState;
        lastInBuildDurationInSeconds_ = statelessInstanceQueryResult->LastInBuildDurationInSeconds;
    }

    return error;
}

void ServiceReplicaQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());    
}

std::wstring ServiceReplicaQueryResult::ToString() const
{
    wstring replicaString = L"";

    if (serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        replicaString = wformatString("ReplicaId = '{0}', ReplicaRole = '{1}', ReplicaStatus = '{2}', ReplicaAddress = '{3}', NodeName = '{4}', HealthState = '{5}', LastInBuildDurationInSeconds = '{6}', ReplicaInstanceId= '{7}'",
            replicaId_,
            static_cast<int>(replicaRole_),
            static_cast<int>(replicaStatus_),
            replicaAddress_,
            nodeName_,
            healthState_,
            lastInBuildDurationInSeconds_);
    }
    else if(serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        replicaString = wformatString("InstanceId = '{0}', Status = '{1}', Address = '{2}', NodeName = '{3}', HealthState = '{4}', LastInBuildDurationInSeconds = '{5}'",
            instanceId_,
            static_cast<int>(replicaStatus_),
            replicaAddress_,
            nodeName_,
            healthState_,
            lastInBuildDurationInSeconds_);
    }

    return wformatString("Kind = {0}, {1}",
        static_cast<int>(serviceResultKind_),
        replicaString);
}

std::wstring ServiceReplicaQueryResult::CreateContinuationToken() const
{
    if (serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        return PagingStatus::CreateContinuationToken<FABRIC_REPLICA_ID>(replicaId_);
    }
    else
    {
        return PagingStatus::CreateContinuationToken<FABRIC_REPLICA_ID>(instanceId_);
    }
}
