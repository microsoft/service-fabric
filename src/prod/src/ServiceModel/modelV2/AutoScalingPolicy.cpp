//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel::ModelV2;
using namespace Common;

INITIALIZE_SIZE_ESTIMATION(AutoScalingPolicy);

bool AutoScalingPolicy::operator==(AutoScalingPolicy const & other) const
{
    if (autoScalingName_ == other.autoScalingName_
        && autoScalingTrigger_ == other.autoScalingTrigger_
        && autoScalingMechanism_ == other.autoScalingMechanism_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

Common::ErrorCode AutoScalingPolicy::TryValidate(wstring const &traceId) const
{
    if (autoScalingTrigger_ == nullptr || autoScalingMechanism_ == nullptr || autoScalingName_ == L"")
    {
        return ErrorCodeValue::InvalidServiceScalingPolicy;
    }
    if (autoScalingTrigger_->Kind != AutoScalingTriggerKind::AverageLoad)
    {
        return Common::ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(TriggerNotSupported), traceId));
    }
    if (autoScalingMechanism_->Kind != AutoScalingMechanismKind::AddRemoveReplica)
    {
        return Common::ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(MechanismNotSupported), traceId));
    }
    auto error = autoScalingMechanism_->Validate();
    if (!error.IsSuccess())
    {
        return error;
    }
    error = autoScalingTrigger_->Validate();
    if (!error.IsSuccess())
    {
        return error;
    }

    return Common::ErrorCodeValue::Success;
}
