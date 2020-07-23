// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(AutoScalingResourceMetric)

AutoScalingResourceMetric::AutoScalingResourceMetric()
    : AutoScalingMetric(AutoScalingMetricKind::Resource)
{
}

AutoScalingResourceMetric::AutoScalingResourceMetric(
    std::wstring const& name)
    : AutoScalingMetric(AutoScalingMetricKind::Resource),
    name_(name)
{
}

bool AutoScalingResourceMetric::Equals(AutoScalingMetricSPtr const & other, bool ignoreDynamicContent) const
{
    if (other == nullptr)
    {
        return false;
    }

    if (kind_ != other->Kind)
    {
        return false;
    }

    auto castedOther = static_pointer_cast<AutoScalingResourceMetric> (other);

    if (!ignoreDynamicContent)
    {
        if (name_ != castedOther->name_)
        {
            return false;
        }
    }

    return true;
}

Common::ErrorCode AutoScalingResourceMetric::Validate() const
{
    return ErrorCodeValue::Success;
}
