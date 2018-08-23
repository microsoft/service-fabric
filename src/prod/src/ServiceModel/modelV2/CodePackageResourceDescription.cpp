//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(CodePackageResources)
INITIALIZE_SIZE_ESTIMATION(CodePackageResourceDescription)

bool CodePackageResources::operator==(CodePackageResources const & other) const
{
    if (memoryInGB_ == other.memoryInGB_
        && cpu_ == other.cpu_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CodePackageResourceDescription::operator==(CodePackageResourceDescription const & other) const
{
    if (resourceRequests_ == other.resourceRequests_
        && (
            (resourceLimitsPtr_ == nullptr && other.resourceLimitsPtr_ == nullptr) 
            || (resourceLimitsPtr_ && other.resourceLimitsPtr_ && *resourceLimitsPtr_ == *other.resourceLimitsPtr_)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode CodePackageResourceDescription::TryValidate(wstring const &traceId) const
{
    if (resourceRequests_.MemoryInGB <= 0)
    {
        return Common::ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ContainerResourceRequestMemoryNotValid), traceId));
    }

    if (resourceRequests_.Cpu <= 0)
    {
        return Common::ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ContainerResourceRequestCpuNotValid), traceId));
    }

    return Common::ErrorCodeValue::Success;

}
