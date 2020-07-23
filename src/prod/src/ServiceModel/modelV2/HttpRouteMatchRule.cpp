//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(HttpRouteMatchRule)

ErrorCode HttpRouteMatchRule::TryValidate(wstring const &traceId) const
{
    for (auto &header : routeMatchHeaders_)
    {
        auto error = header.TryValidate(traceId);
        if (!error.IsSuccess()) { return error; }
    }

    return this->routeMatchPath_.TryValidate(traceId);
}
