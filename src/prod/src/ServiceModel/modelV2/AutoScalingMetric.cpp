// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(AutoScalingMetric)

AutoScalingMetric::AutoScalingMetric()
    : kind_(AutoScalingMetricKind::Resource)
{

}

AutoScalingMetric::AutoScalingMetric(AutoScalingMetricKind::Enum kind)
    : kind_(kind)
{
}

bool AutoScalingMetric::operator == (AutoScalingMetricSPtr const & other) const
{
    if (other == nullptr)
    {
        return false;
    }
    return kind_ == other->kind_;
}

bool AutoScalingMetric::Equals(AutoScalingMetricSPtr const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    if (other == nullptr)
    {
        return false;
    }
    return kind_ == other->kind_;
}

Serialization::IFabricSerializable * AutoScalingMetric::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(AutoScalingMetricKind::Enum))
    {
        return nullptr;
    }

    AutoScalingMetricKind::Enum kind = *(reinterpret_cast<AutoScalingMetricKind::Enum const *>(typeInformation.buffer));

    switch (kind)
    {
    case AutoScalingMetricKind::Resource:
        return new AutoScalingResourceMetric();
    default:
        return nullptr;
    }
}

NTSTATUS AutoScalingMetric::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

AutoScalingMetricSPtr AutoScalingMetric::CreateSPtr(AutoScalingMetricKind::Enum kind)
{
    switch (kind)
    {
    case AutoScalingMetricKind::Resource:
        return make_shared<AutoScalingResourceMetric>();
    default:
        return nullptr;
    }
}

Common::ErrorCode AutoScalingMetric::Validate() const
{
    return ErrorCodeValue::InvalidServiceScalingPolicy;
}

