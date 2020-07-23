//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(HttpRouteMatchPath)

ErrorCode HttpRouteMatchPath::TryValidate(wstring const &traceId) const
{
    if (value_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ValueNotSpecified), traceId));
    }

    if (type_ == PathMatchType::Enum::Invalid)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(PathMatchTypeNotSpecified), traceId));
    }

    return ErrorCode::Success();
}