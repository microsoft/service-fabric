// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(EndpointRef)

bool EndpointRef::operator==(EndpointRef const & other) const
{
    if (this->Name == other.Name)
    {
        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode EndpointRef::TryValidate(wstring const & traceId) const
{
    if (Name.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(NameNotSpecified), traceId));
    }

    return ErrorCodeValue::Success;
}
