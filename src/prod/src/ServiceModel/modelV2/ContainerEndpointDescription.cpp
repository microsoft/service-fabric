//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ContainerEndpointDescription)

bool ContainerEndpointDescription::operator==(ContainerEndpointDescription const & other) const
{
    if (name_ == other.name_
        && port_ == other.port_
        && useDynamicHostPort_ == other.useDynamicHostPort_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode ContainerEndpointDescription::TryValidate(wstring const &traceId) const
{
    if (name_.length() == 0)
    {
        return Common::ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ContainerEndpointNameNotSpecified), traceId));
    }

    return Common::ErrorCodeValue::Success;
}


