// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(AverageLoadScalingTrigger)

AverageLoadScalingTrigger::AverageLoadScalingTrigger()
    : AutoScalingTrigger(AutoScalingTriggerKind::AverageLoad)
{
}

AverageLoadScalingTrigger::AverageLoadScalingTrigger(
    AutoScalingMetricSPtr autoScalingMetric,
    double lowerLoadThreshold,
    double upperLoadThreshold,
    uint scaleIntervalInSeconds)
    : AutoScalingTrigger(AutoScalingTriggerKind::AverageLoad),
      autoScalingMetric_(autoScalingMetric),
      lowerLoadThreshold_(lowerLoadThreshold),
      upperLoadThreshold_(upperLoadThreshold),
      scaleIntervalInSeconds_(scaleIntervalInSeconds)
{
}

bool AverageLoadScalingTrigger::Equals(AutoScalingTriggerSPtr const & other, bool ignoreDynamicContent) const
{
    if (other == nullptr)
    {
        return false;
    }

    if (kind_ != other->Kind)
    {
        return false;
    }

    auto castedOther = static_pointer_cast<AverageLoadScalingTrigger> (other);

    if (!ignoreDynamicContent)
    {
        if (autoScalingMetric_ != castedOther->autoScalingMetric_ ||
            upperLoadThreshold_ != castedOther->upperLoadThreshold_ ||
            lowerLoadThreshold_ != castedOther->lowerLoadThreshold_ ||
            scaleIntervalInSeconds_ != castedOther->scaleIntervalInSeconds_)
        {
            return false;
        }
    }

    return true;
}

Common::ErrorCode AverageLoadScalingTrigger::Validate() const
{
    auto error = autoScalingMetric_->Validate();
    if (!error.IsSuccess())
    {
        return error;
    }
    if (lowerLoadThreshold_ < 0.0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_LowerLoadThreshold), lowerLoadThreshold_));
    }
    if (upperLoadThreshold_ <= 0.0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_UpperLoadThreshold), upperLoadThreshold_));
    }
    if (lowerLoadThreshold_ > upperLoadThreshold_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_MinMaxInstances), lowerLoadThreshold_, upperLoadThreshold_));
    }
    return ErrorCodeValue::Success;
}
