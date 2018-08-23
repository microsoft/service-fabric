// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(PartitionEntityHealthInformation)

PartitionEntityHealthInformation::PartitionEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_PARTITION)
    , partitionId_(Guid::Empty())
{
}

PartitionEntityHealthInformation::PartitionEntityHealthInformation(Guid const& partitionId)
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_PARTITION)
    , partitionId_(partitionId)
{
}

ErrorCode PartitionEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto partitionHealthReport = heap.AddItem<FABRIC_PARTITION_HEALTH_REPORT>();
    partitionHealthReport->PartitionId = partitionId_.AsGUID();
    partitionHealthReport->HealthInformation = commonHealthInformation;
            
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_PARTITION;
    healthReport.Value = partitionHealthReport.GetRawPointer();
    return ErrorCode::Success();
}

ErrorCode PartitionEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    UNREFERENCED_PARAMETER(attributes);

    kind_ = healthReport.Kind;
    auto partitionHealthReport = reinterpret_cast<FABRIC_PARTITION_HEALTH_REPORT *>(healthReport.Value);
    partitionId_ = Guid(partitionHealthReport->PartitionId);

    return commonHealthInformation.FromCommonPublicApi(*partitionHealthReport->HealthInformation);
}

wstring const& PartitionEntityHealthInformation::get_EntityId() const
{
    if (entityId_.empty())
    {
       entityId_ = wformatString("{0}", partitionId_);
    }

    return entityId_;
}
