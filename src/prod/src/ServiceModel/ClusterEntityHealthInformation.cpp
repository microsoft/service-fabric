// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ClusterEntityHealthInformation)

ClusterEntityHealthInformation::ClusterEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_CLUSTER)
{
}

ClusterEntityHealthInformation::ClusterEntityHealthInformation(ClusterEntityHealthInformation && other)
    : EntityHealthInformation(move(other))
{
}

ClusterEntityHealthInformation & ClusterEntityHealthInformation::operator = (ClusterEntityHealthInformation && other)
{
    EntityHealthInformation::operator=(move(other));

    return *this;
}

ErrorCode ClusterEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * healthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto clusterHealthReport = heap.AddItem<FABRIC_CLUSTER_HEALTH_REPORT>();
    clusterHealthReport->HealthInformation = healthInformation;

    healthReport.Kind = kind_;
    healthReport.Value = clusterHealthReport.GetRawPointer();
    return ErrorCode::Success();
}

ErrorCode ClusterEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    UNREFERENCED_PARAMETER(attributes);

    kind_ = healthReport.Kind;

    auto clusterHealthReport = reinterpret_cast<FABRIC_CLUSTER_HEALTH_REPORT *>(healthReport.Value);
    
    return commonHealthInformation.FromCommonPublicApi(*clusterHealthReport->HealthInformation);
}

wstring const& ClusterEntityHealthInformation::get_EntityId() const
{
    return entityId_;
}
