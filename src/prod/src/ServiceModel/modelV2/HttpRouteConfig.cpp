//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(HttpRouteConfig)

ErrorCode HttpRouteConfig::TryValidate(wstring const &traceId) const
{
    ErrorCode error = this->destination_.TryValidate(GetNextTraceId(traceId, Name));
    if (!error.IsSuccess())
    {
        return error;
    }

    return this->routeMatchRule_.TryValidate(GetNextTraceId(traceId, Name));
}

void HttpRouteConfig::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0}",
        Name);
}
