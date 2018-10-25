//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(ContainerVolumeRef)

#ifndef PLATFORM_UNIX
    // Absolute path to a directory or a drive, can contain spaces.
    wregex ContainerVolumeRef::DestinationPathRegEx = wregex(L"[a-zA-Z]:(?:\\\\[^\\\\/:*?\"<>\\r\\n]+)*\\\\?");

#endif

bool ContainerVolumeRef::operator==(ContainerVolumeRef const & other) const
{
    if (name_ == other.name_
        && destinationPath_ == other.destinationPath_
        && readonly_ == other.readonly_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ContainerVolumeRef::operator!=(ContainerVolumeRef const & other) const
{
    return !(*this == other);
}

ErrorCode ContainerVolumeRef::TryValidate(wstring const &traceId) const
{
    if (name_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(VolumeNameNotSpecified), traceId));
    }

    bool validDestinationPath = true;
    if (destinationPath_.empty())
    {
        validDestinationPath = false;
    }

#ifndef PLATFORM_UNIX // start WINDOWS specific validation.

    if (validDestinationPath
        && !std::regex_match(destinationPath_, ContainerVolumeRef::DestinationPathRegEx))
    {
        validDestinationPath = false;
    }

#endif // end WINDOWS specific validation.

    return validDestinationPath ?
        ErrorCode::Success() :
        ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_MODELV2_RC(VolumeDestinationPathInvalidFormat),
                GetNextTraceId(traceId, name_),
                destinationPath_));
}

