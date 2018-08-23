// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(StatelessInstanceEntityHealthInformation)

StatelessInstanceEntityHealthInformation::StatelessInstanceEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE)
    , partitionId_(Guid::Empty())
    , replicaId_(FABRIC_INVALID_REPLICA_ID)
{
}

StatelessInstanceEntityHealthInformation::StatelessInstanceEntityHealthInformation(
    Common::Guid const& partitionId,
    FABRIC_REPLICA_ID replicaId)
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
{
}

ErrorCode StatelessInstanceEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto instanceHealthReport = heap.AddItem<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_REPORT>();
    instanceHealthReport->PartitionId = partitionId_.AsGUID();
    instanceHealthReport->InstanceId = replicaId_;
    instanceHealthReport->HealthInformation = commonHealthInformation;

    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE;
    healthReport.Value = instanceHealthReport.GetRawPointer();
    return ErrorCode::Success();
}

ErrorCode StatelessInstanceEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    UNREFERENCED_PARAMETER(attributes);

    kind_ = healthReport.Kind;
    auto instanceHealthReport = reinterpret_cast<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_REPORT *>(healthReport.Value);

    partitionId_ = Guid(instanceHealthReport->PartitionId);
    replicaId_= instanceHealthReport->InstanceId;

    return commonHealthInformation.FromCommonPublicApi(*instanceHealthReport->HealthInformation);
}

wstring const& StatelessInstanceEntityHealthInformation::get_EntityId() const
{
    if (entityId_.empty())
    {
        entityId_ = wformatString("{0}{1}{2}", partitionId_, EntityHealthInformation::Delimiter, replicaId_);
    }

    return entityId_;
}
