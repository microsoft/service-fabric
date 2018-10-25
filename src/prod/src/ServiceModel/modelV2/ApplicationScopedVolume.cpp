//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ApplicationScopedVolume)

bool ApplicationScopedVolume::operator==(ApplicationScopedVolume const & other) const
{
    if (!ContainerVolumeRef::operator==(other))
    {
        return false;
    }

    if ((creationParameters_ == nullptr) != (other.creationParameters_ == nullptr))
    {
        return false;
    }

    if ((creationParameters_ != nullptr) &&
        !(*creationParameters_ == *(other.creationParameters_)))
    {
        return false;
    }

    return true;
}

bool ApplicationScopedVolume::operator!=(ApplicationScopedVolume const & other) const
{
    return !(*this == other);
}

ErrorCode ApplicationScopedVolume::TryValidate(wstring const &traceId) const
{
    ErrorCode error = ContainerVolumeRef::TryValidate(traceId);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (creationParameters_ == nullptr)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(VolumeCreationParametersNotSpecified), GetNextTraceId(traceId, name_)));
    }

    return creationParameters_->Validate(name_);
}

