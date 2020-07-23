//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(HttpRouteMatchHeader)

ErrorCode HttpRouteMatchHeader::TryValidate(wstring const &traceId) const
{
    if (name_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(NameNotSpecified), traceId));
    }

    return ErrorCodeValue::Success;
}

void HttpRouteMatchHeader::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0}",
        Name);
}
