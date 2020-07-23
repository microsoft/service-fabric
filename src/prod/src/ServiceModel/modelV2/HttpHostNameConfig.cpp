//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(HttpHostNameConfig)

ErrorCode HttpHostNameConfig::TryValidate(wstring const &traceId) const
{
    ErrorCode error = ErrorCodeValue::Success;

    if (routes_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(HttpRoutesNotSpecified), traceId));
    }

    for (auto &route : routes_)
    {
        error = route.TryValidate(traceId);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
}

void HttpHostNameConfig::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0}",
        Name);
}
