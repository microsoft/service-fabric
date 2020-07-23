// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

AddRemoveReplicaScalingMechanism::AddRemoveReplicaScalingMechanism()
    : AutoScalingMechanism(AutoScalingMechanismKind::AddRemoveReplica)
{
}

AddRemoveReplicaScalingMechanism::AddRemoveReplicaScalingMechanism(
    int minCount,
    int maxCount,
    int scaleIncrement)
    : AutoScalingMechanism(AutoScalingMechanismKind::AddRemoveReplica),
    minCount_(minCount),
    maxCount_(maxCount),
    scaleIncrement_(scaleIncrement)
{
}

bool AddRemoveReplicaScalingMechanism::Equals(AutoScalingMechanismSPtr const & other, bool ignoreDynamicContent) const
{
    if (other == nullptr)
    {
        return false;
    }

    if (kind_ != other->Kind)
    {
        return false;
    }

    auto castedOther = static_pointer_cast<AddRemoveReplicaScalingMechanism> (other);

    if (!ignoreDynamicContent)
    {
        if (minCount_ != castedOther->minCount_ ||
            maxCount_ != castedOther->maxCount_ ||
            scaleIncrement_ != castedOther->scaleIncrement_)
        {
            return false;
        }
    }

    return true;
}

Common::ErrorCode AddRemoveReplicaScalingMechanism::Validate() const
{
    if (minCount_ < 0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_MODELV2_RC(AutoScalingPolicy_MinReplicaCount), minCount_));
    }
    if (maxCount_ < -1 || maxCount_ == 0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_MODELV2_RC(AutoScalingPolicy_MaxReplicaCount), maxCount_));
    }

    if (scaleIncrement_ <= 0)
    {
        return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Increment));
    }
    if (maxCount_ > 0 && minCount_ > maxCount_)
    {
        return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_MODELV2_RC(AutoScalingPolicy_MinMaxReplicas), minCount_, maxCount_));
    }
    return ErrorCodeValue::Success;
}
