// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;

INITIALIZE_SIZE_ESTIMATION(ServicePartitionQueryResult)

ServicePartitionQueryResult::ServicePartitionQueryResult()
    : serviceResultKind_(FABRIC_SERVICE_KIND_INVALID)
    , partitionInformation_()
    , targetReplicaSetSize_(0)
    , minReplicaSetSize_(0)
    , instanceCount_(0)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , partitionStatus_(FABRIC_QUERY_SERVICE_PARTITION_STATUS_INVALID)
    , lastQuorumLossDurationInSeconds_(0)
    , renameAsPrimaryEpoch_(false)
{
}

ServicePartitionQueryResult::ServicePartitionQueryResult(ServicePartitionQueryResult && other)
    : serviceResultKind_(other.serviceResultKind_)
    , partitionInformation_(move(other.partitionInformation_))
    , targetReplicaSetSize_(move(other.targetReplicaSetSize_))
    , minReplicaSetSize_(move(other.minReplicaSetSize_))
    , instanceCount_(move(other.instanceCount_))
    , healthState_(move(other.healthState_))
    , partitionStatus_(move(other.partitionStatus_))
    , lastQuorumLossDurationInSeconds_(move(other.lastQuorumLossDurationInSeconds_))
    , currentConfigurationEpoch_(move(other.currentConfigurationEpoch_))
    , renameAsPrimaryEpoch_(false)
{    
}

ServicePartitionQueryResult::ServicePartitionQueryResult(
    ServicePartitionInformation const & partitionInformation,
    uint targetReplicaSetSize,
    uint minReplicaSetSize,
    FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus,
    int64 lastQuorumLossDurationInSeconds,
    const Epoch & currentConfigurationEpoch)
    : serviceResultKind_(FABRIC_SERVICE_KIND_STATEFUL)
    , partitionInformation_(partitionInformation)
    , targetReplicaSetSize_(targetReplicaSetSize)
    , minReplicaSetSize_(minReplicaSetSize)
    , instanceCount_(0)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , partitionStatus_(partitionStatus)
    , lastQuorumLossDurationInSeconds_(lastQuorumLossDurationInSeconds)
    , currentConfigurationEpoch_(currentConfigurationEpoch)
    , renameAsPrimaryEpoch_(false)
{
}

ServicePartitionQueryResult::ServicePartitionQueryResult(
    ServicePartitionInformation const & partitionInformation,
    int instanceCount,
    FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus)
    : serviceResultKind_(FABRIC_SERVICE_KIND_STATELESS)
    , partitionInformation_(partitionInformation)
    , targetReplicaSetSize_(0)
    , minReplicaSetSize_(0)
    , instanceCount_(instanceCount)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , partitionStatus_(partitionStatus)
    , lastQuorumLossDurationInSeconds_(0)
    , renameAsPrimaryEpoch_(false)
{
}

ServicePartitionQueryResult const & ServicePartitionQueryResult::operator = (ServicePartitionQueryResult && other)
{
    if (&other != this)
    {
        serviceResultKind_ = other.serviceResultKind_ ;
        partitionInformation_ = move(other.partitionInformation_);
        targetReplicaSetSize_ = other.targetReplicaSetSize_;
        minReplicaSetSize_ = other.minReplicaSetSize_;
        instanceCount_ = other.instanceCount_;
        healthState_ = other.healthState_;
        partitionStatus_ = other.partitionStatus_;
        lastQuorumLossDurationInSeconds_ = other.lastQuorumLossDurationInSeconds_;
        renameAsPrimaryEpoch_ = other.renameAsPrimaryEpoch_;
    }

    return *this;
}

ServicePartitionQueryResult ServicePartitionQueryResult::CreateStatelessServicePartitionQueryResult(
    ServicePartitionInformation const & partitionInformation,
    int instanceCount,
    FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus)
{
    return ServicePartitionQueryResult(partitionInformation, instanceCount, partitionStatus);
}

ServicePartitionQueryResult ServicePartitionQueryResult::CreateStatefulServicePartitionQueryResult(
    ServicePartitionInformation const & partitionInformation,
    uint targetReplicaSetSize,
    uint minReplicaSetSize,
    FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus,
    int64 lastQuorumLossDurationInSeconds,
    const Epoch & currentConfigurationEpoch)
{
    return ServicePartitionQueryResult(partitionInformation, targetReplicaSetSize, minReplicaSetSize, partitionStatus, lastQuorumLossDurationInSeconds, currentConfigurationEpoch);
}

// Rename property CurrentConfigurationEpoch to PrimaryEpoch in serializer
void ServicePartitionQueryResult::SetRenameAsPrimaryEpoch(bool renameAsPrimaryEpoch)
{
    renameAsPrimaryEpoch_ = renameAsPrimaryEpoch;
}

void ServicePartitionQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM & publicServicePartitionQueryResult) const 
{
    publicServicePartitionQueryResult.Kind = serviceResultKind_;

    if (serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        auto statefulServicePartition = heap.AddItem<FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM>();
        auto partitionInformation = heap.AddItem<FABRIC_SERVICE_PARTITION_INFORMATION>();
        partitionInformation_.ToPublicApi(heap, *partitionInformation);
        statefulServicePartition->PartitionInformation = partitionInformation.GetRawPointer();
        statefulServicePartition->TargetReplicaSetSize = targetReplicaSetSize_;
        statefulServicePartition->MinReplicaSetSize = minReplicaSetSize_;
        statefulServicePartition->HealthState = healthState_;
        statefulServicePartition->PartitionStatus = partitionStatus_;
        statefulServicePartition->LastQuorumLossDurationInSeconds = lastQuorumLossDurationInSeconds_;

        auto statefulServicePartitionEx1 = heap.AddItem<FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM_EX1>();
        statefulServicePartitionEx1->PrimaryEpoch = currentConfigurationEpoch_.ToPrimaryEpoch().ToPublic();
        statefulServicePartition->Reserved = statefulServicePartitionEx1.GetRawPointer();

        publicServicePartitionQueryResult.Value = statefulServicePartition.GetRawPointer();
    }
    else if(serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        auto statelessServicePartition = heap.AddItem<FABRIC_STATELESS_SERVICE_PARTITION_QUERY_RESULT_ITEM>();
        auto partitionInformation = heap.AddItem<FABRIC_SERVICE_PARTITION_INFORMATION>();
        partitionInformation_.ToPublicApi(heap, *partitionInformation);
        statelessServicePartition->PartitionInformation = partitionInformation.GetRawPointer();
        statelessServicePartition->InstanceCount = instanceCount_;
        statelessServicePartition->HealthState = healthState_;
        statelessServicePartition->PartitionStatus = partitionStatus_;

        publicServicePartitionQueryResult.Value = statelessServicePartition.GetRawPointer();
    }
}

ErrorCode ServicePartitionQueryResult::FromPublicApi(__in FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM const& partitionQueryResult)
{
    ErrorCode error = ErrorCode::Success();
    serviceResultKind_ = partitionQueryResult.Kind;
    if (serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        auto statefulPartitionQueryResult = reinterpret_cast<FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM*>(partitionQueryResult.Value);
        auto err = partitionInformation_.FromPublicApi(*statefulPartitionQueryResult->PartitionInformation);

        if (!err.IsSuccess()) { return err; }

        targetReplicaSetSize_ = statefulPartitionQueryResult->TargetReplicaSetSize;
        minReplicaSetSize_ = statefulPartitionQueryResult->MinReplicaSetSize;
        healthState_ = statefulPartitionQueryResult->HealthState;
        partitionStatus_ = statefulPartitionQueryResult->PartitionStatus;
        lastQuorumLossDurationInSeconds_ = statefulPartitionQueryResult->LastQuorumLossDurationInSeconds;
        if (statefulPartitionQueryResult->Reserved)
        {
            auto statefulPartitionQueryResultEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM_EX1*>(statefulPartitionQueryResult->Reserved);
            err = currentConfigurationEpoch_.FromPublicApi((*statefulPartitionQueryResultEx1).PrimaryEpoch);
            if (!err.IsSuccess()) { return err; }
        }
    }
    else if(serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        auto statelessPartitionQueryResult = reinterpret_cast<FABRIC_STATELESS_SERVICE_PARTITION_QUERY_RESULT_ITEM*>(partitionQueryResult.Value);
        auto err = partitionInformation_.FromPublicApi(*statelessPartitionQueryResult->PartitionInformation);

        if (!err.IsSuccess()) { return err; }

        instanceCount_ = statelessPartitionQueryResult->InstanceCount;
        healthState_ = statelessPartitionQueryResult->HealthState;
        partitionStatus_ = statelessPartitionQueryResult->PartitionStatus;
    }

    return ErrorCode::Success();
}

void ServicePartitionQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());    
}

std::wstring ServicePartitionQueryResult::ToString() const
{
    wstring partitionString = L"";

    if (serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        partitionString = wformatString("PartitionInformation = ( {0} ), TargetReplicaSetSize = '{1}', MinReplicaSetSize = '{2}', HealthState = '{3}', PartitionStatus = '{4}', LastQuorumLossDurationInSeconds = '{5}'",
            partitionInformation_.ToString(),
            targetReplicaSetSize_,
            minReplicaSetSize_,
            healthState_,
            partitionStatus_,
            lastQuorumLossDurationInSeconds_);
    }
    else if(serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        partitionString = wformatString("PartitionInformation = ( {0} ), InstanceCount = '{1}', HealthState = '{2}', PartitionStatus = '{3}'",
            partitionInformation_.ToString(),
            instanceCount_,
            healthState_,
            partitionStatus_);
    }

    return wformatString("Kind = {0}, {1}",
        static_cast<int>(serviceResultKind_),
        partitionString);
}

std::wstring ServicePartitionQueryResult::CreateContinuationToken() const
{
    return PagingStatus::CreateContinuationToken<Guid>(partitionInformation_.PartitionId);
}
