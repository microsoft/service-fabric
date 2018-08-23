// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(StatefulReplicaEntityHealthInformation)

StatefulReplicaEntityHealthInformation::StatefulReplicaEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA)
    , partitionId_(Guid::Empty())
    , replicaId_(FABRIC_INVALID_REPLICA_ID)
    , replicaInstanceId_(FABRIC_INVALID_INSTANCE_ID)
{
}

StatefulReplicaEntityHealthInformation::StatefulReplicaEntityHealthInformation(
    Common::Guid const& partitionId,
    FABRIC_REPLICA_ID replicaId,
    FABRIC_INSTANCE_ID replicaInstanceId)
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , replicaInstanceId_(replicaInstanceId)
{
}

ErrorCode StatefulReplicaEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto replicaHealthInformation = heap.AddItem<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_REPORT>();
    replicaHealthInformation->PartitionId = partitionId_.AsGUID();
    replicaHealthInformation->ReplicaId = replicaId_;
    replicaHealthInformation->HealthInformation = commonHealthInformation;

    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA;
    healthReport.Value = replicaHealthInformation.GetRawPointer();
    return ErrorCode::Success();
}

ErrorCode StatefulReplicaEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    UNREFERENCED_PARAMETER(attributes);

    kind_ = healthReport.Kind;
    auto replicaHealthInformation = reinterpret_cast<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_REPORT *>(healthReport.Value);
    partitionId_ = Guid(replicaHealthInformation->PartitionId);
    replicaId_= replicaHealthInformation->ReplicaId;

    return commonHealthInformation.FromCommonPublicApi(*replicaHealthInformation->HealthInformation);
}

wstring const& StatefulReplicaEntityHealthInformation::get_EntityId() const
{
    if (entityId_.empty())
    {
        entityId_ = wformatString("{0}{1}{2}", partitionId_, EntityHealthInformation::Delimiter, replicaId_);
    }

    return entityId_;
}
